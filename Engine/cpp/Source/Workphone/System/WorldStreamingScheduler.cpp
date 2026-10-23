#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

namespace workphone
{
    namespace
    {
        struct SourceStorageHeader
        {
            StreamBudget budget;
            size_t allocationAlignment;
        };
        size_t sourceStorageOffset( size_t alignment )
        {
            if( !alignment || ( alignment & ( alignment - 1 ) ) ||
                alignment - 1 > std::numeric_limits<size_t>::max() - sizeof( SourceStorageHeader ) )
                throw std::bad_alloc();
            return ( sizeof( SourceStorageHeader ) + alignment - 1 ) & ~( alignment - 1 );
        }
    }
    size_t sourceBudgetStorageBytes( size_t bytes, size_t alignment )
    {
        const auto offset = sourceStorageOffset( alignment );
        if( bytes > std::numeric_limits<size_t>::max() - offset ) throw std::bad_alloc();
        return offset + bytes;
    }
    void *allocateSourceBudgetStorage( size_t bytes, size_t alignment )
    {
        const auto total = sourceBudgetStorageBytes( bytes, alignment );
        auto budget = WorldStreamingScheduler::reserveSharedSource( total );
        if( !budget ) throw std::bad_alloc();
        const auto allocationAlignment = std::max( alignment, alignof( SourceStorageHeader ) );
        auto pointer = ::operator new( total, std::align_val_t( allocationAlignment ) );
        ::new( pointer ) SourceStorageHeader{ std::move( budget ), allocationAlignment };
        return static_cast<char *>( pointer ) + sourceStorageOffset( alignment );
    }
    void freeSourceBudgetStorage( void *pointer, size_t alignment ) noexcept
    {
        if( !pointer ) return;
        auto header = reinterpret_cast<SourceStorageHeader *>(
            static_cast<char *>( pointer ) - sourceStorageOffset( alignment ) );
        auto budget = std::move( header->budget );
        const auto allocationAlignment = header->allocationAlignment;
        header->~SourceStorageHeader();
        ::operator delete( header, std::align_val_t( allocationAlignment ) );
        // Physical storage is gone before the reservation becomes available.
    }
    namespace
    {
        thread_local const void *streamWorker = nullptr;
        const char *memoryName( StreamMemoryClass category )
        {
            switch( category )
            {
            case StreamMemoryClass::SourceCpu:
                return "source CPU data";
            case StreamMemoryClass::PendingCpu:
                return "pending CPU jobs";
            case StreamMemoryClass::WorkingCpu:
                return "CPU working storage";
            case StreamMemoryClass::ResultCpu:
                return "CPU result storage";
            case StreamMemoryClass::Gpu:
                return "GPU resources";
            case StreamMemoryClass::Collision:
                return "collision geometry";
            default:
                return "unknown resource";
            }
        }
    }  // namespace
    struct StreamBudgetState
    {
        mutable std::mutex mutex;
        std::condition_variable wake;
        std::array<u64, size_t( StreamMemoryClass::Count )> used{}, limits{};
        std::array<u64, size_t( StreamMemoryClass::Count )> peakUsed{}, rejectedReservations{};
    };
    namespace
    {
        std::shared_ptr<StreamBudgetState> sharedAllocationLedger()
        {
            static auto state = [] {
                auto value = std::make_shared<StreamBudgetState>();
                value->limits = WorldStreamLimits{}.bytes;
                return value;
            }();
            return state;
        }
    }
    StreamBudget WorldStreamingScheduler::reserveSharedSource( u64 bytes )
    {
        auto state = sharedAllocationLedger();
        auto lease = StreamBudget( new StreamBudgetLease(state, StreamMemoryClass::SourceCpu, 0) );
        const auto index = size_t(StreamMemoryClass::SourceCpu);
        std::lock_guard<std::mutex> lock(state->mutex);
        if( bytes > state->limits[index] || state->used[index] > state->limits[index] - bytes )
        {
            if( state->rejectedReservations[index] != std::numeric_limits<u64>::max() )
                ++state->rejectedReservations[index];
            return {};
        }
        state->used[index] += bytes;
        state->peakUsed[index] = std::max(state->peakUsed[index], state->used[index]);
        lease->m_bytes = bytes;
        return lease;
    }
    StreamBudgetLease::StreamBudgetLease( std::shared_ptr<StreamBudgetState> state,
                                          StreamMemoryClass category, u64 bytes ) :
        m_state( std::move( state ) ),
        m_class( category ),
        m_bytes( bytes )
    {
    }
    StreamBudgetLease::~StreamBudgetLease()
    {
        if( m_bytes )
        {
            {
                std::lock_guard<std::mutex> lock( m_state->mutex );
                m_state->used[size_t( m_class )] -= m_bytes;
            }
            m_state->wake.notify_all();
        }
    }
    u64 StreamBudgetLease::bytes() const
    {
        return m_bytes;
    }
    StreamMemoryClass StreamBudgetLease::memoryClass() const
    {
        return m_class;
    }

    struct WorldStreamingScheduler::Impl
    {
        struct Job
        {
            StreamBudget pending;
            WorldStreamRequest request;
            std::shared_ptr<std::atomic<bool>> token;
            u64 generation = 0;
        };
        struct Ready
        {
            WorldStreamResult result;
            u64 reserved = 0;
        };
        struct Producer
        {
            WorldStreamProducerLimits limits;
            std::shared_ptr<std::atomic<bool>> token = std::make_shared<std::atomic<bool>>( false );
            Array<Job> pending;
            Array<Ready> ready;
            WorldStreamProducerStats stats;
            u64 id = 0, activeBytes = 0, lastDispatch = 0;
            bool closed = false;
        };
        struct BudgetedPayload
        {
            StreamBudget lease;
            std::shared_ptr<const void> payload;
        };
        WorldStreamLimits limits;
        std::shared_ptr<StreamBudgetState> budget = std::make_shared<StreamBudgetState>();
        mutable std::mutex mutex;
        std::map<u64, std::shared_ptr<Producer>> producers;
        Array<std::thread> workers;
        bool stopping = false;
        u64 nextId = 1, dispatch = 0, completed = 0, cancelled = 0;
        u32 pendingCount = 0, activeCount = 0, readyCount = 0, burst = 0;
        explicit Impl( const WorldStreamLimits &settings, bool sharedLedger ) : limits( settings )
        {
            if( !limits.workers || limits.workers > 16 || !limits.maximumProducers ||
                limits.maximumProducers > 4096 || !limits.maximumPending ||
                limits.maximumPending > 65536 || !limits.maximumReady || limits.maximumReady > 256 )
                throw std::invalid_argument( "Invalid world streaming worker/queue limits." );
            if( sharedLedger ) budget = sharedAllocationLedger();
            else budget->limits = limits.bytes;
        }
        bool reserveJob( u64 workBytes, u64 resultBytes, StreamBudget &work, StreamBudget &result )
        {
            try
            {
                // Construct control blocks before locking. A failed admission
                // releases only zero-byte handles and does not wake another
                // blocked worker. Work and result capacity commit together.
                auto candidateWork =
                    StreamBudget( new StreamBudgetLease( budget, StreamMemoryClass::WorkingCpu, 0 ) );
                auto candidateResult =
                    StreamBudget( new StreamBudgetLease( budget, StreamMemoryClass::ResultCpu, 0 ) );
                {
                    std::lock_guard<std::mutex> lock( budget->mutex );
                    constexpr auto w = size_t( StreamMemoryClass::WorkingCpu );
                    constexpr auto r = size_t( StreamMemoryClass::ResultCpu );
                    if( workBytes > budget->limits[w] - budget->used[w] ||
                        resultBytes > budget->limits[r] - budget->used[r] )
                        return false;
                    budget->used[w] += workBytes;
                    budget->used[r] += resultBytes;
                    budget->peakUsed[w] = std::max( budget->peakUsed[w], budget->used[w] );
                    budget->peakUsed[r] = std::max( budget->peakUsed[r], budget->used[r] );
                    candidateWork->m_bytes = workBytes;
                    candidateResult->m_bytes = resultBytes;
                }
                work = std::move( candidateWork );
                result = std::move( candidateResult );
                return true;
            }
            catch( ... )
            {
                return false;
            }
        }
        void run()
        {
            streamWorker = this;
            std::array<Producer *, 4096> eligible{};
            for( ;; )
            {
                Job job;
                std::shared_ptr<Producer> selected;
                StreamBudget workReservation, resultReservation;
                {
                    std::unique_lock<std::mutex> lock( mutex );
                    for( ;; )
                    {
                        if( stopping )
                            return;
                        if( readyCount + activeCount < limits.maximumReady )
                        {
                            size_t eligibleCount = 0;
                            for( const auto &entry : producers )
                            {
                                const auto &p = entry.second;
                                if( !p->closed && !p->pending.empty() &&
                                    p->stats.active < p->limits.maximumActive &&
                                    p->ready.size() + p->stats.active < p->limits.maximumReady )
                                {
                                    const auto bytes = p->pending.front().request.maximumResultBytes;
                                    if( bytes <= p->limits.maximumReadyBytes &&
                                        p->stats.readyBytes + p->activeBytes <=
                                            p->limits.maximumReadyBytes - bytes )
                                        eligible[eligibleCount++] = p.get();
                                }
                            }
                            const bool fairTurn = burst >= 4;
                            std::sort( eligible.begin(), eligible.begin() + eligibleCount,
                                       [&]( const auto *a, const auto *b ) {
                                           const bool older = a->lastDispatch == b->lastDispatch
                                                                  ? a->id < b->id
                                                                  : a->lastDispatch < b->lastDispatch;
                                           if( fairTurn )
                                               return older;
                                           const auto ap = a->pending.front().request.priority,
                                                      bp = b->pending.front().request.priority;
                                           return ap == bp ? older : ap > bp;
                                       } );
                            for( size_t candidate = 0; candidate < eligibleCount; ++candidate )
                            {
                                auto *p = eligible[candidate];
                                StreamBudget work, result;
                                if( !reserveJob( p->pending.front().request.workingBytes,
                                                 p->pending.front().request.maximumResultBytes, work,
                                                 result ) )
                                    continue;
                                selected = producers.at( p->id );
                                workReservation = std::move( work );
                                resultReservation = std::move( result );
                                job = std::move( p->pending.front() );
                                p->pending.erase( p->pending.begin() );
                                --pendingCount;
                                ++p->stats.active;
                                ++activeCount;
                                p->activeBytes += job.request.maximumResultBytes;
                                p->lastDispatch = ++dispatch;
                                burst = fairTurn ? 0 : burst + 1;
                                break;
                            }
                        }
                        if( selected )
                            break;
                        // Budget leases can be released independently of queue mutex.
                        // Timed recheck also covers a release immediately before waiting.
                        budget->wake.wait_for( lock, std::chrono::milliseconds( 20 ) );
                    }
                }
                WorldStreamResult result;
                try
                {
                    result = job.request.execute( StreamCancellation( job.token ) );
                    result.key = job.request.key;
                    result.generation = job.generation;
                    if( result.bytes > job.request.maximumResultBytes ||
                        ( result.bytes && !result.payload ) )
                        throw std::runtime_error(
                            "Streaming producer exceeded its reserved result size." );
                    if( result.payload )
                    {
                        auto held = std::make_shared<BudgetedPayload>();
                        held->lease = resultReservation;
                        held->payload = result.payload;
                        result.payload = std::shared_ptr<const void>( held, held->payload.get() );
                    }
                }
                catch( const std::exception &e )
                {
                    result.payload.reset();
                    result.bytes = 0;
                    result.error = e.what();
                }
                catch( ... )
                {
                    result.payload.reset();
                    result.bytes = 0;
                    result.error = "Streaming producer threw an unknown exception.";
                }
                result.key = job.request.key;
                result.generation = job.generation;
                workReservation.reset();
                {
                    std::lock_guard<std::mutex> lock( mutex );
                    if( selected->closed || job.token->load() ||
                        selected->stats.generation != job.generation )
                    {
                        ++selected->stats.cancelled;
                        ++cancelled;
                    }
                    else
                    {
                        const auto bytes = result.payload ? job.request.maximumResultBytes : 0;
                        try
                        {
                            selected->ready.push_back( { std::move( result ), bytes } );
                            selected->stats.readyBytes += bytes;
                            ++selected->stats.completed;
                            ++completed;
                            ++readyCount;
                        }
                        catch( ... )
                        {
                            // Queue allocation cannot terminate a worker or publish a
                            // partially-owned candidate. Account a discarded result.
                            ++selected->stats.cancelled;
                            ++cancelled;
                        }
                    }
                }
                // Drain includes releasing captured inputs and discarded result
                // storage, not merely returning from execute(). Destructors run
                // outside the queue mutex and before active reaches zero.
                result.payload.reset();
                resultReservation.reset();
                job.request.execute = {};
                job.pending.reset();
                {
                    std::lock_guard<std::mutex> lock( mutex );
                    --selected->stats.active;
                    --activeCount;
                    selected->activeBytes -= job.request.maximumResultBytes;
                }
                budget->wake.notify_all();
            }
        }
    };

    WorldStreamingScheduler::WorldStreamingScheduler( const WorldStreamLimits &limits ) :
        WorldStreamingScheduler( limits, false ) {}
    WorldStreamingScheduler::WorldStreamingScheduler( const WorldStreamLimits &limits, bool sharedLedger ) :
        m_impl( std::make_shared<Impl>( limits, sharedLedger ) )
    {
        try
        {
            for( u32 i = 0; i < limits.workers; ++i )
                m_impl->workers.emplace_back( [impl = m_impl] { impl->run(); } );
        }
        catch( ... )
        {
            {
                std::lock_guard<std::mutex> lock( m_impl->mutex );
                m_impl->stopping = true;
            }
            m_impl->budget->wake.notify_all();
            for( auto &worker : m_impl->workers )
                worker.join();
            throw;
        }
    }
    WorldStreamingScheduler::~WorldStreamingScheduler()
    {
        {
            std::lock_guard<std::mutex> lock( m_impl->mutex );
            m_impl->stopping = true;
            for( auto &entry : m_impl->producers )
                entry.second->token->store( true );
        }
        m_impl->budget->wake.notify_all();
        for( auto &worker : m_impl->workers )
            if( worker.get_id() == std::this_thread::get_id() )
                worker.detach();
            else
                worker.join();
    }
    namespace
    {
        struct SharedScheduler
        {
            std::mutex mutex;
            std::shared_ptr<WorldStreamingScheduler> instance;
        };
        SharedScheduler &sharedScheduler()
        {
            static SharedScheduler service;
            return service;
        }
    }  // namespace
    std::shared_ptr<WorldStreamingScheduler> WorldStreamingScheduler::shared()
    {
        auto &service = sharedScheduler();
        std::lock_guard<std::mutex> lock( service.mutex );
        if( !service.instance )
            service.instance = std::shared_ptr<WorldStreamingScheduler>( new WorldStreamingScheduler({}, true) );
        return service.instance;
    }
    void WorldStreamingScheduler::shutdownShared()
    {
        std::shared_ptr<WorldStreamingScheduler> retiring;
        auto &service = sharedScheduler();
        {
            std::lock_guard<std::mutex> lock( service.mutex );
            retiring = std::move( service.instance );
        }
        // Join outside the service mutex: releasing a worker callback can
        // release other service consumers.
        retiring.reset();
    }
    StreamBudget WorldStreamingScheduler::reserve( StreamMemoryClass category, u64 bytes, String &error )
    {
        const auto index = size_t( category );
        if( index >= size_t( StreamMemoryClass::Count ) )
        {
            error = "Invalid streaming memory class.";
            return {};
        }
        try
        {
            auto state = m_impl->budget;
            auto lease = StreamBudget( new StreamBudgetLease( state, category, 0 ) );
            std::lock_guard<std::mutex> lock( state->mutex );
            if( bytes > state->limits[index] || state->used[index] > state->limits[index] - bytes )
            {
                if( state->rejectedReservations[index] != std::numeric_limits<u64>::max() )
                    ++state->rejectedReservations[index];
                error = String( "Shared streaming budget for " ) + memoryName( category ) +
                        " is exhausted: requested " + std::to_string( bytes ) + " bytes, available " +
                        std::to_string( state->limits[index] - state->used[index] ) + " bytes.";
                return {};
            }
            // A zero-byte lease is still a successful admission token.
            state->used[index] += bytes;
            state->peakUsed[index] = std::max( state->peakUsed[index], state->used[index] );
            lease->m_bytes = bytes;
            error.clear();
            return lease;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return {};
        }
    }
    bool WorldStreamingScheduler::setBudget( StreamMemoryClass category, u64 bytes, String &error )
    {
        const auto index = size_t( category );
        if( index >= size_t( StreamMemoryClass::Count ) )
        {
            error = "Invalid streaming memory class.";
            return false;
        }
        std::lock_guard<std::mutex> queue( m_impl->mutex );
        for( const auto &entry : m_impl->producers )
            for( const auto &job : entry.second->pending )
                if( ( category == StreamMemoryClass::WorkingCpu && job.request.workingBytes > bytes ) ||
                    ( category == StreamMemoryClass::ResultCpu &&
                      job.request.maximumResultBytes > bytes ) )
                {
                    error = "Streaming budget cannot make pending work impossible.";
                    return false;
                }
        {
            std::lock_guard<std::mutex> lock( m_impl->budget->mutex );
            if( bytes < m_impl->budget->used[index] )
            {
                error = "Streaming budget cannot shrink below live reservations.";
                return false;
            }
            m_impl->budget->limits[index] = bytes;
        }
        m_impl->budget->wake.notify_all();
        error.clear();
        return true;
    }
    std::shared_ptr<WorldStreamProducer> WorldStreamingScheduler::createProducer(
        const WorldStreamProducerLimits &limits, String &error )
    {
        if( !limits.maximumPending || !limits.maximumActive || !limits.maximumReady ||
            limits.maximumPending > m_impl->limits.maximumPending ||
            limits.maximumActive > m_impl->limits.workers ||
            limits.maximumReady > m_impl->limits.maximumReady )
        {
            error = "Invalid streaming producer limits.";
            return {};
        }
        try
        {
            auto state = std::make_shared<Impl::Producer>();
            state->limits = limits;
            state->ready.reserve( limits.maximumReady );
            auto producer =
                std::shared_ptr<WorldStreamProducer>( new WorldStreamProducer( shared_from_this(), 0 ) );
            std::lock_guard<std::mutex> lock( m_impl->mutex );
            if( m_impl->stopping || m_impl->producers.size() >= m_impl->limits.maximumProducers )
            {
                error = "Shared streaming producer limit is exhausted.";
                return {};
            }
            const auto id = m_impl->nextId++;
            state->id = id;
            m_impl->producers.emplace( id, std::move( state ) );
            producer->m_id = id;
            error.clear();
            return producer;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return {};
        }
    }
    WorldStreamStats WorldStreamingScheduler::stats() const
    {
        WorldStreamStats result;
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        result.workers = u32( m_impl->workers.size() );
        result.maximumPending = m_impl->limits.maximumPending;
        result.maximumReady = m_impl->limits.maximumReady;
        result.producers = u32( m_impl->producers.size() );
        result.pending = m_impl->pendingCount;
        result.active = m_impl->activeCount;
        result.ready = m_impl->readyCount;
        result.completed = m_impl->completed;
        result.cancelled = m_impl->cancelled;
        std::lock_guard<std::mutex> budget( m_impl->budget->mutex );
        result.used = m_impl->budget->used;
        result.limits = m_impl->budget->limits;
        result.peakUsed = m_impl->budget->peakUsed;
        result.rejectedReservations = m_impl->budget->rejectedReservations;
        return result;
    }

    WorldStreamProducer::WorldStreamProducer( std::shared_ptr<WorldStreamingScheduler> scheduler,
                                              u64 id ) :
        m_scheduler( std::move( scheduler ) ),
        m_id( id )
    {
    }
    WorldStreamProducer::~WorldStreamProducer()
    {
        if( !m_id )
            return;
        drain();
        std::lock_guard<std::mutex> lock( m_scheduler->m_impl->mutex );
        m_scheduler->m_impl->producers.erase( m_id );
    }
    bool WorldStreamProducer::replace( const Array<WorldStreamRequest> &requests, String &error )
    {
        auto &p = *m_scheduler->m_impl;
        Array<WorldStreamingScheduler::Impl::Job> candidate, retired;
        std::array<WorldStreamingScheduler::Impl::Ready, 256> retiredResults;
        try
        {
            auto token = std::make_shared<std::atomic<bool>>( false );
            std::set<u64> keys;
            u64 resultLimit;
            {
                std::lock_guard<std::mutex> lock( p.mutex );
                const auto &state = *p.producers.at( m_id );
                if( requests.size() > state.limits.maximumPending )
                {
                    error = "Streaming demand exceeds producer request limit.";
                    return false;
                }
                resultLimit = state.limits.maximumReadyBytes;
            }
            const auto budgets = m_scheduler->stats().limits;
            for( const auto &request : requests )
            {
                if( !request.execute || !keys.insert( request.key ).second ||
                    request.maximumResultBytes > resultLimit ||
                    request.maximumResultBytes > budgets[size_t( StreamMemoryClass::ResultCpu )] ||
                    request.workingBytes > budgets[size_t( StreamMemoryClass::WorkingCpu )] )
                {
                    error = "Invalid, duplicate or over-budget streaming request.";
                    return false;
                }
                auto reservation = m_scheduler->reserve(
                    StreamMemoryClass::PendingCpu, std::max<u64>( 256, request.pendingBytes ), error );
                if( !reservation )
                    return false;
                candidate.push_back( { std::move( reservation ), request, token, 0 } );
            }
            std::stable_sort( candidate.begin(), candidate.end(), []( const auto &a, const auto &b ) {
                return a.request.priority > b.request.priority;
            } );
            {
                std::lock_guard<std::mutex> lock( p.mutex );
                auto &state = *p.producers.at( m_id );
                if( p.stopping || state.closed ||
                    candidate.size() >
                        p.limits.maximumPending - ( p.pendingCount - state.pending.size() ) )
                {
                    error = "Shared streaming pending request limit is exhausted.";
                    return false;
                }
                // Budget updates take the same queue->budget lock order. Recheck
                // after preparation so a concurrent limit change cannot admit a
                // request which could never dispatch.
                {
                    std::lock_guard<std::mutex> budget( p.budget->mutex );
                    for( const auto &job : candidate )
                        if( job.request.workingBytes >
                                p.budget->limits[size_t( StreamMemoryClass::WorkingCpu )] ||
                            job.request.maximumResultBytes >
                                p.budget->limits[size_t( StreamMemoryClass::ResultCpu )] )
                        {
                            error =
                                "Streaming request exceeds the current CPU working or result budget.";
                            return false;
                        }
                }
                if( state.stats.generation == std::numeric_limits<u64>::max() )
                {
                    error = "Streaming generation exhausted.";
                    return false;
                }
                ++state.stats.generation;
                for( auto &job : candidate )
                    job.generation = state.stats.generation;
                state.token->store( true );
                state.token = token;
                p.cancelled += state.pending.size();
                state.stats.cancelled += state.pending.size();
                p.pendingCount -= u32( state.pending.size() );
                p.readyCount -= u32( state.ready.size() );
                retired.swap( state.pending );
                for( size_t i = 0; i < state.ready.size(); ++i )
                    retiredResults[i] = std::move( state.ready[i] );
                state.ready.clear();  // Preserve preallocated completion slots.
                state.pending.swap( candidate );
                p.pendingCount += u32( state.pending.size() );
                state.stats.readyBytes = 0;
            }
            p.budget->wake.notify_all();
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    bool WorldStreamProducer::take( WorldStreamResult &result )
    {
        auto &p = *m_scheduler->m_impl;
        WorldStreamingScheduler::Impl::Ready ready;
        {
            std::lock_guard<std::mutex> lock( p.mutex );
            auto &state = *p.producers.at( m_id );
            if( state.ready.empty() )
                return false;
            ready = std::move( state.ready.front() );
            state.ready.erase( state.ready.begin() );
            --p.readyCount;
            state.stats.readyBytes -= ready.reserved;
        }
        result = std::move( ready.result );
        p.budget->wake.notify_all();
        return true;
    }
    std::function<StreamBudget( StreamMemoryClass, u64, String & )>
    WorldStreamProducer::budgetAllocator() const
    {
        return [scheduler = m_scheduler]( StreamMemoryClass category, u64 bytes, String &error ) {
            return scheduler->reserve( category, bytes, error );
        };
    }
    StreamBudget WorldStreamProducer::reserve( StreamMemoryClass category, u64 bytes, String &error )
    {
        return m_scheduler->reserve( category, bytes, error );
    }
    void WorldStreamProducer::cancel()
    {
        auto &p = *m_scheduler->m_impl;
        Array<WorldStreamingScheduler::Impl::Job> retired;
        std::array<WorldStreamingScheduler::Impl::Ready, 256> results;
        {
            std::lock_guard<std::mutex> lock( p.mutex );
            auto &state = *p.producers.at( m_id );
            state.token->store( true );
            if( state.stats.generation != std::numeric_limits<u64>::max() )
                ++state.stats.generation;
            p.cancelled += state.pending.size();
            state.stats.cancelled += state.pending.size();
            p.pendingCount -= u32( state.pending.size() );
            p.readyCount -= u32( state.ready.size() );
            retired.swap( state.pending );
            for( size_t i = 0; i < state.ready.size(); ++i )
                results[i] = std::move( state.ready[i] );
            state.ready.clear();
            state.stats.readyBytes = 0;
        }
        p.budget->wake.notify_all();
    }
    void WorldStreamProducer::drain()
    {
        cancel();
        auto &p = *m_scheduler->m_impl;
        std::unique_lock<std::mutex> lock( p.mutex );
        auto state = p.producers.at( m_id );
        if( streamWorker == &p )
            return;  // Already executing callbacks own their state until return.
        p.budget->wake.wait( lock, [&] { return state->stats.active == 0; } );
    }
    WorldStreamProducerStats WorldStreamProducer::stats() const
    {
        auto &p = *m_scheduler->m_impl;
        std::lock_guard<std::mutex> lock( p.mutex );
        const auto &state = *p.producers.at( m_id );
        auto result = state.stats;
        result.pending = u32( state.pending.size() );
        result.ready = u32( state.ready.size() );
        return result;
    }
}  // namespace workphone
