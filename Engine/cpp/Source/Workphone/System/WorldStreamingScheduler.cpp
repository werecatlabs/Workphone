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
    struct StreamBudgetState
    {
        mutable std::mutex mutex;
        // Retained service generations have independent queue locks.
        std::condition_variable_any wake;
        std::array<u64, size_t( StreamMemoryClass::Count )> used{}, limits{};
        std::array<u64, size_t( StreamMemoryClass::Count )> peakUsed{}, rejectedReservations{};
    };

    namespace detail
    {
        const ContainerAllocationPolicy &sourceContainerPolicy() noexcept
        {
            static const ContainerAllocationPolicy policy{ &allocateSourceBudgetStorage,
                                                           &freeSourceBudgetStorage };
            return policy;
        }
    }  // namespace detail

    namespace
    {
        thread_local u64 sourceDenials = 0;

        std::shared_ptr<StreamBudgetState> sharedAllocationLedger()
        {
            // This one bootstrap ledger outlives services and late String-pool
            // destruction. Its own fixed storage cannot use the allocator it owns.
            struct Ledger
            {
                union
                {
                    std::shared_ptr<StreamBudgetState> state;
                };

                Ledger() : state( std::make_shared<StreamBudgetState>() )
                {
                    state->limits = WorldStreamLimits{}.bytes;
                }

                ~Ledger()
                {
                }
            };

            static Ledger ledger;
            return ledger.state;
        }

        bool admitSourceStorage( const std::shared_ptr<StreamBudgetState> &state, u64 bytes )
        {
            constexpr auto index = size_t( StreamMemoryClass::SourceCpu );
            std::lock_guard<std::mutex> lock( state->mutex );
            if( bytes > state->limits[index] - state->used[index] )
            {
                if( state->rejectedReservations[index] != std::numeric_limits<u64>::max() )
                {
                    ++state->rejectedReservations[index];
                }
                return false;
            }
            state->used[index] += bytes;
            state->peakUsed[index] = std::max( state->peakUsed[index], state->used[index] );
            return true;
        }

        void releaseSourceStorage( const std::shared_ptr<StreamBudgetState> &state, u64 bytes ) noexcept
        {
            // Source storage is admitted by callers before demand publication.
            // Only working/result capacity can unblock worker dispatch.
            std::lock_guard<std::mutex> lock( state->mutex );
            state->used[size_t( StreamMemoryClass::SourceCpu )] -= bytes;
        }

        struct SourceStorageHeader
        {
            // Direct ownership avoids recursively allocating a lease/control
            // block for the lease/control block's own SourceCpu allocation.
            std::shared_ptr<StreamBudgetState> state;
            u64 bytes;
            size_t allocationAlignment;
        };

        size_t sourceStorageOffset( size_t alignment )
        {
            if( !alignment || ( alignment & ( alignment - 1 ) ) ||
                alignment - 1 > std::numeric_limits<size_t>::max() - sizeof( SourceStorageHeader ) )
            {
                throw std::bad_alloc();
            }
            return ( sizeof( SourceStorageHeader ) + alignment - 1 ) & ~( alignment - 1 );
        }
    }  // namespace

    u64 sourceBudgetDenialCount() noexcept
    {
        return sourceDenials;
    }

    size_t sourceBudgetStorageBytes( size_t bytes, size_t alignment )
    {
        const auto offset = sourceStorageOffset( alignment );
        if( bytes > std::numeric_limits<size_t>::max() - offset )
        {
            throw std::bad_alloc();
        }
        return offset + bytes;
    }

    void *allocateSourceBudgetStorage( size_t bytes, size_t alignment )
    {
        const auto total = sourceBudgetStorageBytes( bytes, alignment );
        auto state = sharedAllocationLedger();
        if( !admitSourceStorage( state, total ) )
        {
            ++sourceDenials;
            throw std::bad_alloc();
        }
        const auto allocationAlignment = std::max( alignment, alignof( SourceStorageHeader ) );
        void *pointer;
        try
        {
            pointer = ::operator new( total, std::align_val_t( allocationAlignment ) );
        }
        catch( ... )
        {
            releaseSourceStorage( state, total );
            throw;
        }
        ::new( pointer ) SourceStorageHeader{ std::move( state ), total, allocationAlignment };
        return static_cast<char *>( pointer ) + sourceStorageOffset( alignment );
    }

    void freeSourceBudgetStorage( void *pointer, size_t alignment ) noexcept
    {
        if( !pointer )
        {
            return;
        }
        auto header = reinterpret_cast<SourceStorageHeader *>( static_cast<char *>( pointer ) -
                                                               sourceStorageOffset( alignment ) );
        auto state = std::move( header->state );
        const auto bytes = header->bytes;
        const auto allocationAlignment = header->allocationAlignment;
        header->~SourceStorageHeader();
        ::operator delete( header, std::align_val_t( allocationAlignment ) );
        // Physical storage is gone before the reservation becomes available.
        releaseSourceStorage( state, bytes );
    }

    namespace
    {
        void streamError( String &error, const char *message ) noexcept
        {
            try
            {
                error = message;
            }
            catch( const std::bad_alloc & )
            {
                error = "Budget exhausted";
            }
        }

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

    struct StreamBudgetLease::Storage
    {
        StreamBudgetLease lease;
        Storage( std::shared_ptr<StreamBudgetState> state, StreamMemoryClass category ) :
            lease( std::move( state ), category, 0 )
        {
        }
    };

    StreamBudget StreamBudgetLease::create( std::shared_ptr<StreamBudgetState> state,
                                            StreamMemoryClass category )
    {
        auto owner = std::allocate_shared<Storage>( SourceBudgetAllocator<Storage>{},
                                                    std::move( state ), category );
        return StreamBudget( owner, &owner->lease );
    }

    StreamBudget WorldStreamingScheduler::reserveSharedSource( u64 bytes )
    {
        try
        {
            auto state = sharedAllocationLedger();
            auto lease = StreamBudgetLease::create( state, StreamMemoryClass::SourceCpu );
            if( !admitSourceStorage( state, bytes ) )
            {
                return {};
            }
            lease->m_bytes = bytes;
            return lease;
        }
        catch( const std::bad_alloc & )
        {
            return {};
        }
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
            if( m_class != StreamMemoryClass::SourceCpu )
            {
                m_state->wake.notify_all();
            }
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
        struct BudgetedPayload
        {
            StreamBudget lease;
            std::shared_ptr<const void> payload;
        };

        struct Job
        {
            StreamBudget pending, work, result;
            std::shared_ptr<BudgetedPayload> held;
            WorldStreamRequest request;
            std::shared_ptr<std::atomic<bool>> token;
            u64 generation = 0, order = 0;
        };

        struct Ready
        {
            WorldStreamResult result;
            u64 reserved = 0;
        };

        using TokenMap = std::map<u64, std::shared_ptr<std::atomic<bool>>, std::less<u64>,
            SourceBudgetAllocator<std::pair<const u64, std::shared_ptr<std::atomic<bool>>>>>;

        struct Producer
        {
            struct Executing {u64 key=0;std::shared_ptr<std::atomic<bool>> token;};
            WorldStreamProducerLimits limits;
            std::shared_ptr<std::atomic<bool>> token = std::allocate_shared<std::atomic<bool>>(
                SourceBudgetAllocator<std::atomic<bool>>{}, false );
            Array<Job> pending{ Allocator<Job>( sourceBudget ) };
            Array<Ready> ready{ Allocator<Ready>( sourceBudget ) };
            TokenMap tokens;
            // Fixed by the global <=16 worker limit. Old-generation callbacks
            // stay visible to scoped cancellation/drain after replace()/cancel().
            std::array<Executing,16> executing{};
            WorldStreamProducerStats stats;
            u64 id = 0, activeBytes = 0, lastDispatch = 0;
            bool closed = false;
        };

        WorldStreamLimits limits;
        std::shared_ptr<StreamBudgetState> budget;
        mutable std::mutex mutex;
        std::map<u64, std::shared_ptr<Producer>, std::less<u64>,
                 SourceBudgetAllocator<std::pair<const u64, std::shared_ptr<Producer>>>> producers;
        Array<std::thread> workers{ Allocator<std::thread>( sourceBudget ) };
        bool stopping = false;
        u64 nextId = 1, dispatch = 0, completed = 0, cancelled = 0;
        u32 pendingCount = 0, activeCount = 0, readyCount = 0, burst = 0;

        explicit Impl( const WorldStreamLimits &settings, bool sharedLedger ) :
            limits( settings ),
            budget( sharedLedger ? sharedAllocationLedger() :
                std::allocate_shared<StreamBudgetState>( SourceBudgetAllocator<StreamBudgetState>{} ) )
        {
            if( !limits.workers || limits.workers > 16 || !limits.maximumProducers ||
                limits.maximumProducers > 4096 || !limits.maximumPending ||
                limits.maximumPending > 65536 || !limits.maximumReady || limits.maximumReady > 256 )
            {
                throw std::invalid_argument( "Invalid world streaming worker/queue limits." );
            }
            if( !sharedLedger )
            {
                budget->limits = limits.bytes;
            }
        }

        bool reserveJob( Job &job )
        {
            // All handles were admitted before replace() published demand.
            // Dispatch at the SourceCpu cap needs no new wrapper allocation.
            std::lock_guard<std::mutex> lock( budget->mutex );
            constexpr auto w = size_t( StreamMemoryClass::WorkingCpu );
            constexpr auto r = size_t( StreamMemoryClass::ResultCpu );
            const auto workBytes = job.request.workingBytes;
            const auto resultBytes = job.request.maximumResultBytes;
            if( workBytes > budget->limits[w] - budget->used[w] ||
                resultBytes > budget->limits[r] - budget->used[r] )
            {
                return false;
            }
            budget->used[w] += workBytes;
            budget->used[r] += resultBytes;
            budget->peakUsed[w] = std::max( budget->peakUsed[w], budget->used[w] );
            budget->peakUsed[r] = std::max( budget->peakUsed[r], budget->used[r] );
            job.work->m_bytes = workBytes;
            job.result->m_bytes = resultBytes;
            return true;
        }

        void run()
        {
            streamWorker = this;
            struct Eligible {Producer *producer=nullptr;size_t index=0;};
            std::array<Eligible,4096> eligible{};
            for( ;; )
            {
                Job job;
                std::shared_ptr<Producer> selected;
                {
                    std::unique_lock<std::mutex> lock( mutex );
                    for( ;; )
                    {
                        if( stopping )
                        {
                            return;
                        }
                        if( readyCount + activeCount < limits.maximumReady )
                        {
                            u64 availableWork=0,availableResult=0;
                            {
                                std::lock_guard<std::mutex> admission(budget->mutex);
                                const auto w=size_t(StreamMemoryClass::WorkingCpu),r=size_t(StreamMemoryClass::ResultCpu);
                                availableWork=budget->limits[w]-budget->used[w];
                                availableResult=budget->limits[r]-budget->used[r];
                            }
                            size_t eligibleCount = 0;
                            for( const auto &entry : producers )
                            {
                                const auto &p = entry.second;
                                if( !p->closed && !p->pending.empty() &&
                                    p->stats.active < p->limits.maximumActive &&
                                    p->ready.size() + p->stats.active < p->limits.maximumReady )
                                {
                                    const auto &front=p->pending.front().request;
                                    const auto bytes=front.maximumResultBytes;
                                    size_t index=0;
                                    if(bytes>p->limits.maximumReadyBytes||
                                        p->stats.readyBytes+p->activeBytes>p->limits.maximumReadyBytes-bytes||
                                        front.workingBytes>availableWork||bytes>availableResult) {
                                        // A cached completion may already own the very
                                        // reservation blocking this queue's cold front.
                                        // Pending order is priority/FIFO, so its first
                                        // zero-cost job is the highest-priority fallback.
                                        index=p->pending.size();
                                        for(size_t i=1;i<p->pending.size();++i)
                                            if(!p->pending[i].request.workingBytes&&!p->pending[i].request.maximumResultBytes){index=i;break;}
                                    }
                                    if(index<p->pending.size())eligible[eligibleCount++]={p.get(),index};
                                }
                            }
                            const bool fairTurn = burst >= 4;
                            std::sort( eligible.begin(), eligible.begin() + eligibleCount,
                                       [&]( const auto &a, const auto &b ) {
                                           const auto *aproducer=a.producer,*bproducer=b.producer;
                                           const bool older = aproducer->lastDispatch == bproducer->lastDispatch
                                                                  ? aproducer->id < bproducer->id
                                                                  : aproducer->lastDispatch < bproducer->lastDispatch;
                                           if( fairTurn )
                                           {
                                               return older;
                                           }
                                           const auto ap = aproducer->pending[a.index].request.priority,
                                                      bp = bproducer->pending[b.index].request.priority;
                                           return ap == bp ? older : ap > bp;
                                       } );
                            for( size_t candidate = 0; candidate < eligibleCount; ++candidate )
                            {
                                auto *p = eligible[candidate].producer;
                                const auto index=eligible[candidate].index;
                                if( !reserveJob( p->pending[index] ) )
                                {
                                    continue;
                                }
                                selected = producers.at( p->id );
                                job = std::move( p->pending[index] );
                                for( size_t i = index+1; i < p->pending.size(); ++i )
                                {
                                    p->pending[i - 1] = std::move( p->pending[i] );
                                }
                                p->pending.pop_back();
                                --pendingCount;
                                ++p->stats.active;
                                ++activeCount;
                                p->activeBytes += job.request.maximumResultBytes;
                                for(auto &active:p->executing)if(!active.token){active.key=job.request.key;active.token=job.token;break;}
                                p->lastDispatch = ++dispatch;
                                burst = fairTurn ? 0 : burst + 1;
                                break;
                            }
                        }
                        if( selected )
                        {
                            break;
                        }
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
                    {
                        throw std::runtime_error(
                            "Streaming producer exceeded its reserved result size." );
                    }
                    if( result.payload )
                    {
                        job.held->lease = job.result;
                        job.held->payload = std::move( result.payload );
                        result.payload = std::shared_ptr<const void>( job.held, job.held->payload.get() );
                    }
                }
                catch( const std::exception &e )
                {
                    result.payload.reset();
                    result.bytes = 0;
                    streamError( result.error, e.what() );
                }
                catch( ... )
                {
                    result.payload.reset();
                    result.bytes = 0;
                    streamError( result.error, "Streaming producer threw an unknown exception." );
                }
                result.key = job.request.key;
                result.generation = job.generation;
                job.work.reset();
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
                result = {};
                const auto activeBytes = job.request.maximumResultBytes;
                const auto finishedKey = job.request.key;
                const auto finishedToken = job.token.get();
                job = {};
                {
                    std::lock_guard<std::mutex> lock( mutex );
                    const auto token = selected->tokens.find( finishedKey );
                    if( token != selected->tokens.end() && token->second.get() == finishedToken )
                        selected->tokens.erase( token );
                    for(auto &active:selected->executing)if(active.token.get()==finishedToken){active.token.reset();break;}
                    --selected->stats.active;
                    --activeCount;
                    selected->activeBytes -= activeBytes;
                    selected.reset();
                }
                budget->wake.notify_all();
            }
        }
    };

    WorldStreamingScheduler::WorldStreamingScheduler( const WorldStreamLimits &limits ) :
        WorldStreamingScheduler( limits, false )
    {
    }

    WorldStreamingScheduler::WorldStreamingScheduler( const WorldStreamLimits &limits,
                                                      bool sharedLedger ) :
        m_impl( std::allocate_shared<Impl>( SourceBudgetAllocator<Impl>{}, limits, sharedLedger ) )
    {
        // Do not reallocate the thread inventory after any workers have started.
        m_impl->workers.reserve( limits.workers );
        try
        {
            for( u32 i = 0; i < limits.workers; ++i )
            {
                m_impl->workers.emplace_back( [impl = m_impl] { impl->run(); } );
            }
        }
        catch( ... )
        {
            {
                std::lock_guard<std::mutex> lock( m_impl->mutex );
                m_impl->stopping = true;
            }
            m_impl->budget->wake.notify_all();
            for( auto &worker : m_impl->workers )
            {
                worker.join();
            }
            throw;
        }
    }

    WorldStreamingScheduler::~WorldStreamingScheduler()
    {
        {
            std::lock_guard<std::mutex> lock( m_impl->mutex );
            m_impl->stopping = true;
            for( auto &entry : m_impl->producers )
            {
                entry.second->token->store( true );
            }
        }
        m_impl->budget->wake.notify_all();
        for( auto &worker : m_impl->workers )
        {
            if( worker.get_id() == std::this_thread::get_id() )
            {
                worker.detach();
            }
            else
            {
                worker.join();
            }
        }
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
        {
            service.instance =
                std::shared_ptr<WorldStreamingScheduler>( new WorldStreamingScheduler( {}, true ),
                    std::default_delete<WorldStreamingScheduler>{},
                    SourceBudgetAllocator<WorldStreamingScheduler>{} );
        }
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
            streamError( error, "Invalid streaming memory class." );
            return {};
        }
        try
        {
            auto state = m_impl->budget;
            auto lease = StreamBudgetLease::create( state, category );
            u64 available = 0;
            bool denied = false;
            {
                std::lock_guard<std::mutex> lock( state->mutex );
                available = state->limits[index] - state->used[index];
                denied = bytes > available;
                if( denied )
                {
                    if( state->rejectedReservations[index] != std::numeric_limits<u64>::max() )
                    {
                        ++state->rejectedReservations[index];
                    }
                }
                else
                {
                    state->used[index] += bytes;
                    state->peakUsed[index] = std::max( state->peakUsed[index], state->used[index] );
                    lease->m_bytes = bytes;
                }
            }
            // Engine String slabs use this same ledger. Never allocate or
            // release their storage while holding its mutex.
            if( denied )
            {
                try
                {
                    error = String( "Shared streaming budget for " ) + memoryName( category ) +
                            " is exhausted: requested " + std::to_string( bytes ) +
                            " bytes, available " + std::to_string( available ) + " bytes.";
                }
                catch( const std::bad_alloc & )
                {
                    streamError( error, "Budget exhausted" );
                }
                return {};
            }
            error.clear();
            return lease;
        }
        catch( const std::exception &e )
        {
            streamError( error, e.what() );
            return {};
        }
    }

    bool WorldStreamingScheduler::ownsBudget( const StreamBudget &lease ) const
    {
        return lease && lease->m_state == m_impl->budget;
    }

    bool WorldStreamingScheduler::setBudget( StreamMemoryClass category, u64 bytes, String &error )
    {
        const auto index = size_t( category );
        if( index >= size_t( StreamMemoryClass::Count ) )
        {
            streamError( error, "Invalid streaming memory class." );
            return false;
        }
        std::lock_guard<std::mutex> queue( m_impl->mutex );
        for( const auto &entry : m_impl->producers )
        {
            for( const auto &job : entry.second->pending )
            {
                if( ( category == StreamMemoryClass::WorkingCpu && job.request.workingBytes > bytes ) ||
                    ( category == StreamMemoryClass::ResultCpu &&
                      job.request.maximumResultBytes > bytes ) )
                {
                    streamError( error, "Streaming budget cannot make pending work impossible." );
                    return false;
                }
            }
        }
        bool belowLive = false;
        {
            std::lock_guard<std::mutex> lock( m_impl->budget->mutex );
            belowLive = bytes < m_impl->budget->used[index];
            if( !belowLive )
            {
                m_impl->budget->limits[index] = bytes;
            }
        }
        if( belowLive )
        {
            streamError( error, "Streaming budget cannot shrink below live reservations." );
            return false;
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
            streamError( error, "Invalid streaming producer limits." );
            return {};
        }
        try
        {
            auto state = std::allocate_shared<Impl::Producer>( SourceBudgetAllocator<Impl::Producer>{} );
            state->limits = limits;
            state->ready.reserve( limits.maximumReady );
            auto producer =
                std::shared_ptr<WorldStreamProducer>( new WorldStreamProducer( shared_from_this(), 0 ),
                    std::default_delete<WorldStreamProducer>{},
                    SourceBudgetAllocator<WorldStreamProducer>{} );
            std::lock_guard<std::mutex> lock( m_impl->mutex );
            if( m_impl->stopping || m_impl->producers.size() >= m_impl->limits.maximumProducers )
            {
                streamError( error, "Shared streaming producer limit is exhausted." );
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
            streamError( error, e.what() );
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
        {
            return;
        }
        drain();
        std::lock_guard<std::mutex> lock( m_scheduler->m_impl->mutex );
        m_scheduler->m_impl->producers.erase( m_id );
    }

    bool WorldStreamProducer::replace( const Array<WorldStreamRequest> &requests, String &error )
    {
        auto &p = *m_scheduler->m_impl;
        Array<WorldStreamingScheduler::Impl::Job> candidate{
            Allocator<WorldStreamingScheduler::Impl::Job>( sourceBudget ) };
        Array<WorldStreamingScheduler::Impl::Job> retired{
            Allocator<WorldStreamingScheduler::Impl::Job>( sourceBudget ) };
        std::array<WorldStreamingScheduler::Impl::Ready, 256> retiredResults;
        try
        {
            WorldStreamingScheduler::Impl::TokenMap tokens;
            std::set<u64, std::less<u64>, SourceBudgetAllocator<u64>> keys;
            u64 resultLimit;
            {
                std::lock_guard<std::mutex> lock( p.mutex );
                const auto &state = *p.producers.at( m_id );
                if( requests.size() > state.limits.maximumPending )
                {
                    streamError( error, "Streaming demand exceeds producer request limit." );
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
                    streamError( error, "Invalid, duplicate or over-budget streaming request." );
                    return false;
                }
                auto reservation = m_scheduler->reserve(
                    StreamMemoryClass::PendingCpu, std::max<u64>( 256, request.pendingBytes ), error );
                if( !reservation )
                {
                    return false;
                }
                WorldStreamingScheduler::Impl::Job job;
                job.pending = std::move( reservation );
                job.work = StreamBudgetLease::create( p.budget, StreamMemoryClass::WorkingCpu );
                job.result = StreamBudgetLease::create( p.budget, StreamMemoryClass::ResultCpu );
                job.held = std::allocate_shared<WorldStreamingScheduler::Impl::BudgetedPayload>(
                    SourceBudgetAllocator<WorldStreamingScheduler::Impl::BudgetedPayload>{} );
                job.request = request;
                job.token = std::allocate_shared<std::atomic<bool>>(
                    SourceBudgetAllocator<std::atomic<bool>>{}, false );
                tokens.emplace(request.key,job.token);
                job.order = candidate.size();
                candidate.push_back( std::move( job ) );
            }
            std::sort( candidate.begin(), candidate.end(), []( const auto &a, const auto &b ) {
                return a.request.priority == b.request.priority ? a.order < b.order :
                    a.request.priority > b.request.priority;
            } );
            {
                std::lock_guard<std::mutex> lock( p.mutex );
                auto &state = *p.producers.at( m_id );
                if( p.stopping || state.closed ||
                    candidate.size() >
                        p.limits.maximumPending - ( p.pendingCount - state.pending.size() ) )
                {
                    streamError( error, "Shared streaming pending request limit is exhausted." );
                    return false;
                }
                // Budget updates take the same queue->budget lock order. Recheck
                // after preparation so a concurrent limit change cannot admit a
                // request which could never dispatch.
                bool impossible = false;
                {
                    std::lock_guard<std::mutex> budget( p.budget->mutex );
                    for( const auto &job : candidate )
                    {
                        if( job.request.workingBytes >
                                p.budget->limits[size_t( StreamMemoryClass::WorkingCpu )] ||
                            job.request.maximumResultBytes >
                                p.budget->limits[size_t( StreamMemoryClass::ResultCpu )] )
                        {
                            impossible = true;
                            break;
                        }
                    }
                }
                if( impossible )
                {
                    streamError( error,
                                 "Streaming request exceeds the current CPU working or result budget." );
                    return false;
                }
                if( state.stats.generation == std::numeric_limits<u64>::max() )
                {
                    streamError( error, "Streaming generation exhausted." );
                    return false;
                }
                ++state.stats.generation;
                for( auto &job : candidate )
                {
                    job.generation = state.stats.generation;
                }
                state.token->store( true );
                for(const auto &entry:state.tokens) entry.second->store(true);
                for(const auto &active:state.executing)if(active.token)active.token->store(true);
                state.tokens.swap(tokens);
                p.cancelled += state.pending.size();
                state.stats.cancelled += state.pending.size();
                p.pendingCount -= u32( state.pending.size() );
                p.readyCount -= u32( state.ready.size() );
                retired.swap( state.pending );
                for( size_t i = 0; i < state.ready.size(); ++i )
                {
                    retiredResults[i] = std::move( state.ready[i] );
                }
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
            streamError( error, e.what() );
            return false;
        }
    }

    bool WorldStreamProducer::enqueue( const Array<WorldStreamRequest> &requests, String &error )
    {return reconcile(requests,{},error);}

    bool WorldStreamProducer::reconcile(const Array<WorldStreamRequest> &requests,
                                        const Array<u64> &cancelKeys,String &error)
    {
        auto &p=*m_scheduler->m_impl;
        Array<WorldStreamingScheduler::Impl::Job> candidate{Allocator<WorldStreamingScheduler::Impl::Job>(sourceBudget)};
        Array<WorldStreamingScheduler::Impl::Job> retired{Allocator<WorldStreamingScheduler::Impl::Job>(sourceBudget)};
        Array<WorldStreamingScheduler::Impl::Ready> retiredReady{Allocator<WorldStreamingScheduler::Impl::Ready>(sourceBudget)};
        WorldStreamingScheduler::Impl::TokenMap tokens;
        std::set<u64,std::less<u64>,SourceBudgetAllocator<u64>> remove;
        try {
            WorldStreamProducerLimits limits;
            {std::lock_guard<std::mutex> lock(p.mutex);limits=p.producers.at(m_id)->limits;}
            if(requests.size()>limits.maximumPending) {streamError(error,"Streaming addition exceeds producer request limit.");return false;}
            if(cancelKeys.size()>u64(limits.maximumPending)+limits.maximumReady+limits.maximumActive)
            {streamError(error,"Streaming retirement exceeds producer key capacity.");return false;}
            remove.insert(cancelKeys.begin(),cancelKeys.end());retired.reserve(remove.size());retiredReady.reserve(limits.maximumReady);
            const auto budgets=m_scheduler->stats().limits;
            for(const auto &request:requests) {
                if(!request.execute||tokens.count(request.key)||request.maximumResultBytes>limits.maximumReadyBytes||
                    request.maximumResultBytes>budgets[size_t(StreamMemoryClass::ResultCpu)]||
                    request.workingBytes>budgets[size_t(StreamMemoryClass::WorkingCpu)])
                {streamError(error,"Invalid, duplicate or over-budget streaming addition.");return false;}
                WorldStreamingScheduler::Impl::Job job;
                job.pending=m_scheduler->reserve(StreamMemoryClass::PendingCpu,std::max<u64>(256,request.pendingBytes),error);
                if(!job.pending)return false;
                job.work=StreamBudgetLease::create(p.budget,StreamMemoryClass::WorkingCpu);
                job.result=StreamBudgetLease::create(p.budget,StreamMemoryClass::ResultCpu);
                job.held=std::allocate_shared<WorldStreamingScheduler::Impl::BudgetedPayload>(
                    SourceBudgetAllocator<WorldStreamingScheduler::Impl::BudgetedPayload>{});
                job.request=request;
                job.token=std::allocate_shared<std::atomic<bool>>(SourceBudgetAllocator<std::atomic<bool>>{},false);
                tokens.emplace(request.key,job.token);candidate.push_back(std::move(job));
            }
            {
                std::lock_guard<std::mutex> lock(p.mutex);auto &state=*p.producers.at(m_id);
                const auto removed=std::count_if(state.pending.begin(),state.pending.end(),[&](const auto &job){return remove.count(job.request.key)!=0;});
                if(p.stopping||state.closed||candidate.size()>state.limits.maximumPending-(state.pending.size()-removed)||
                    candidate.size()>p.limits.maximumPending-(p.pendingCount-removed))
                {streamError(error,"Shared streaming pending request limit is exhausted.");return false;}
                for(const auto &job:candidate) {
                    if(state.tokens.count(job.request.key)||std::any_of(state.executing.begin(),state.executing.end(),
                        [&](const auto &active){return active.token&&active.key==job.request.key;})||std::any_of(state.ready.begin(),state.ready.end(),
                        [&](const auto &ready){return ready.result.key==job.request.key;}))
                    {streamError(error,"Streaming addition reuses an outstanding key.");return false;}
                }
                bool impossible=false;
                {
                    std::lock_guard<std::mutex> budget(p.budget->mutex);
                    for(const auto &job:candidate)
                        if(job.request.workingBytes>p.budget->limits[size_t(StreamMemoryClass::WorkingCpu)]||
                            job.request.maximumResultBytes>p.budget->limits[size_t(StreamMemoryClass::ResultCpu)])
                        {impossible=true;break;}
                }
                if(impossible){streamError(error,"Streaming addition exceeds the current CPU budget.");return false;}
                // Reserve before the commit. Moving admitted jobs and merging
                // preallocated token nodes cannot allocate after publication.
                state.pending.reserve(state.pending.size()-removed+candidate.size());
                for(const auto key:remove){const auto token=state.tokens.find(key);if(token!=state.tokens.end())token->second->store(true);}
                for(const auto &active:state.executing)if(active.token&&remove.count(active.key))active.token->store(true);
                for(size_t i=0;i<state.pending.size();) {
                    if(remove.count(state.pending[i].request.key)) {
                        state.tokens.erase(state.pending[i].request.key);retired.push_back(std::move(state.pending[i]));
                        for(size_t j=i+1;j<state.pending.size();++j)state.pending[j-1]=std::move(state.pending[j]);
                        state.pending.pop_back();
                    }else ++i;
                }
                for(size_t i=0;i<state.ready.size();) {
                    if(remove.count(state.ready[i].result.key)) {
                        state.stats.readyBytes-=state.ready[i].reserved;retiredReady.push_back(std::move(state.ready[i]));
                        for(size_t j=i+1;j<state.ready.size();++j)state.ready[j-1]=std::move(state.ready[j]);
                        state.ready.pop_back();--p.readyCount;
                    }else ++i;
                }
                p.pendingCount-=u32(removed);p.cancelled+=removed;state.stats.cancelled+=removed;
                u64 order=0;for(auto &job:state.pending)job.order=order++;
                for(auto &job:candidate) {job.generation=state.stats.generation;job.order=order++;state.pending.push_back(std::move(job));}
                state.tokens.merge(tokens);p.pendingCount+=u32(candidate.size());
                std::sort(state.pending.begin(),state.pending.end(),[](const auto &a,const auto &b){
                    return a.request.priority==b.request.priority?a.order<b.order:a.request.priority>b.request.priority;});
            }
            p.budget->wake.notify_all();error.clear();return true;
        }catch(const std::exception &failure){streamError(error,failure.what());return false;}
    }

    bool WorldStreamProducer::cancel(u64 key)
    {
        auto &p=*m_scheduler->m_impl;WorldStreamingScheduler::Impl::Job retired;
        WorldStreamingScheduler::Impl::Ready ready;bool found=false;
        {
            std::lock_guard<std::mutex> lock(p.mutex);auto &state=*p.producers.at(m_id);
            const auto token=state.tokens.find(key);if(token!=state.tokens.end()){token->second->store(true);found=true;}
            for(const auto &active:state.executing)if(active.token&&active.key==key){active.token->store(true);found=true;}
            const auto pending=std::find_if(state.pending.begin(),state.pending.end(),[&](const auto &job){return job.request.key==key;});
            if(pending!=state.pending.end()) {
                const auto at=size_t(pending-state.pending.begin());retired=std::move(*pending);
                for(size_t i=at+1;i<state.pending.size();++i)state.pending[i-1]=std::move(state.pending[i]);
                state.pending.pop_back();state.tokens.erase(key);
                --p.pendingCount;++p.cancelled;++state.stats.cancelled;found=true;
            }
            const auto completed=std::find_if(state.ready.begin(),state.ready.end(),[&](const auto &value){return value.result.key==key;});
            if(completed!=state.ready.end()) {
                const auto at=size_t(completed-state.ready.begin());ready=std::move(*completed);
                for(size_t i=at+1;i<state.ready.size();++i)state.ready[i-1]=std::move(state.ready[i]);
                state.ready.pop_back();--p.readyCount;
                state.stats.readyBytes-=ready.reserved;found=true;
            }
        }
        p.budget->wake.notify_all();return found;
    }

    bool WorldStreamProducer::setPriority(u64 key,s32 priority)
    {
        auto &p=*m_scheduler->m_impl;bool found=false;
        {
            std::lock_guard<std::mutex> lock(p.mutex);auto &state=*p.producers.at(m_id);
            for(auto &job:state.pending)if(job.request.key==key){job.request.priority=priority;found=true;break;}
            if(found)std::sort(state.pending.begin(),state.pending.end(),[](const auto &a,const auto &b){
                return a.request.priority==b.request.priority?a.order<b.order:a.request.priority>b.request.priority;});
        }
        if(found)p.budget->wake.notify_all();return found;
    }

    bool WorldStreamProducer::contains(u64 key) const
    {
        auto &p=*m_scheduler->m_impl;std::lock_guard<std::mutex> lock(p.mutex);const auto &state=*p.producers.at(m_id);
        return state.tokens.count(key)||std::any_of(state.executing.begin(),state.executing.end(),[&](const auto &active){return active.token&&active.key==key;})||
            std::any_of(state.ready.begin(),state.ready.end(),[&](const auto &value){return value.result.key==key;});
    }

    void WorldStreamProducer::drain(u64 key)
    {
        cancel(key);auto &p=*m_scheduler->m_impl;std::unique_lock<std::mutex> lock(p.mutex);
        auto state=p.producers.at(m_id);if(streamWorker==&p)return;
        p.budget->wake.wait(lock,[&]{return std::none_of(state->executing.begin(),state->executing.end(),
            [&](const auto &active){return active.token&&active.key==key;});});
    }

    bool WorldStreamProducer::take( WorldStreamResult &result )
    {
        auto &p = *m_scheduler->m_impl;
        WorldStreamingScheduler::Impl::Ready ready;
        {
            std::lock_guard<std::mutex> lock( p.mutex );
            auto &state = *p.producers.at( m_id );
            if( state.ready.empty() )
            {
                return false;
            }
            ready = std::move( state.ready.front() );
            for( size_t i = 1; i < state.ready.size(); ++i )
            {
                state.ready[i - 1] = std::move( state.ready[i] );
            }
            state.ready.pop_back();
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
        Array<WorldStreamingScheduler::Impl::Job> retired{
            Allocator<WorldStreamingScheduler::Impl::Job>( sourceBudget ) };
        std::array<WorldStreamingScheduler::Impl::Ready, 256> results;
        {
            std::lock_guard<std::mutex> lock( p.mutex );
            auto &state = *p.producers.at( m_id );
            state.token->store( true );
            for(const auto &entry:state.tokens) entry.second->store(true);
            for(const auto &active:state.executing)if(active.token)active.token->store(true);
            state.tokens.clear();
            if( state.stats.generation != std::numeric_limits<u64>::max() )
            {
                ++state.stats.generation;
            }
            p.cancelled += state.pending.size();
            state.stats.cancelled += state.pending.size();
            p.pendingCount -= u32( state.pending.size() );
            p.readyCount -= u32( state.ready.size() );
            retired.swap( state.pending );
            for( size_t i = 0; i < state.ready.size(); ++i )
            {
                results[i] = std::move( state.ready[i] );
            }
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
        {
            return;  // Already executing callbacks own their state until return.
        }
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
