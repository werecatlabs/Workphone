#ifndef WPWorldStreamingContracts_h__
#define WPWorldStreamingContracts_h__

#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include <cstdio>

namespace world_streaming_contracts
{
    using namespace workphone;
    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }
    template <class Predicate>
    inline void await( Predicate predicate, const char *message )
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
        while( !predicate() )
        {
            require( std::chrono::steady_clock::now() < deadline, message );
            std::this_thread::yield();
        }
    }
    // Declare after producers. Failure unwinding opens the worker gate before any
    // producer destructor drains it, so a failed assertion cannot hang the suite.
    struct Gate
    {
        std::promise<void> promise;
        std::shared_future<void> future = promise.get_future().share();
        bool opened = false;
        void open()
        {
            if( !opened )
            {
                promise.set_value();
                opened = true;
            }
        }
        ~Gate()
        {
            open();
        }
    };
    inline WorldStreamRequest valueRequest( u64 key, s32 priority = 0 )
    {
        WorldStreamRequest request;
        request.key = key;
        request.priority = priority;
        request.workingBytes = 64;
        request.maximumResultBytes = 64;
        request.execute = [key]( const StreamCancellation & ) {
            WorldStreamResult result;
            result.payload = std::make_shared<const u64>( key );
            result.bytes = sizeof( u64 );
            return result;
        };
        return request;
    }
    struct SharedSourceLimit
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        u64 original = scheduler->stats().limits[size_t( StreamMemoryClass::SourceCpu )];
        String error;

        SharedSourceLimit()
        {
            // Diagnostics must not create a new character slab during a denial sweep.
            error.reserve( 512 );
        }
        u64 used() const
        {
            return scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )];
        }
        void set( u64 bytes )
        {
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, bytes, error ),
                     "Shared SourceCpu test limit changes only above live storage." );
        }
        ~SharedSourceLimit()
        {
            scheduler->setBudget( StreamMemoryClass::SourceCpu, original, error );
        }
    };

    inline void sourceMetadataContracts()
    {
        SharedSourceLimit source;
        String error;
        error.reserve( 512 );
        const auto baseline = source.used();
        const auto storageBytes = sourceBudgetStorageBytes( 73, 64 );
        source.set( baseline + storageBytes );
        const auto releaseStorage = []( void *p ) { freeSourceBudgetStorage( p, 64 ); };
        std::unique_ptr<void, decltype( releaseStorage )> storage(
            allocateSourceBudgetStorage( 73, 64 ), releaseStorage );
        require( reinterpret_cast<std::uintptr_t>( storage.get() ) % 64 == 0 &&
                     source.used() == baseline + storageBytes,
                 "Source allocation admits its exact header, payload and alignment without a recursive lease." );
        bool denied = false;
        try
        {
            auto *unexpected = allocateSourceBudgetStorage( 1, alignof( u64 ) );
            freeSourceBudgetStorage( unexpected, alignof( u64 ) );
        }
        catch( const std::bad_alloc & )
        {
            denied = true;
        }
        storage.reset();
        require( denied && source.used() == baseline,
                 "Full-cap Source denial and physical free return the exact admitted storage." );

        source.set( baseline );
        require( !WorldStreamingScheduler::reserveSharedSource( 0 ) && source.used() == baseline,
                 "The allocation-only lease API rejects even zero-byte control storage at a full Source cap." );
        source.set( source.original );
        auto sharedLease = WorldStreamingScheduler::reserveSharedSource( 64 );
        require( sharedLease && sharedLease->bytes() == 64 && source.used() > baseline + 64,
                 "An allocation-only Source lease charges its own body and control storage as well as its declared bytes." );
        const auto sharedLeaseBytes = source.used() - baseline;
        std::weak_ptr<StreamBudgetLease> weakSharedLease = sharedLease;
        source.set( source.used() );
        require( !WorldStreamingScheduler::reserveSharedSource( 1 ),
                 "Allocation-only Source admission rejects a second lease without recursion or oversubscription." );
        sharedLease.reset();
        require( weakSharedLease.expired() && source.used() == baseline + sharedLeaseBytes - 64,
                 "Allocation-only lease bytes retire independently of its still-live weak control allocation." );
        weakSharedLease.reset();
        require( source.used() == baseline,
                 "Allocation-only Source control storage returns after the last weak owner leaves." );
        source.set( baseline + sharedLeaseBytes );
        sharedLease = WorldStreamingScheduler::reserveSharedSource( 64 );
        require( sharedLease && source.used() == baseline + sharedLeaseBytes,
                 "The allocation-only Source lease retries successfully at its exact combined payload and metadata cap." );
        sharedLease.reset();
        source.set( baseline );
        denied = false;
        try
        {
            auto rejected = std::make_shared<WorldStreamingScheduler>();
        }
        catch( const std::bad_alloc & )
        {
            denied = true;
        }
        require( denied && source.used() == baseline,
                 "Private scheduler metadata construction fails cleanly at the shared Source cap." );
        size_t constructorRejections = 0, constructorAdmissions = 0;
        for( u64 headroom = 0; headroom <= 8192; headroom += 256 )
        {
            source.set( baseline + headroom );
            try
            {
                WorldStreamLimits limits;
                limits.workers = 1;
                auto scheduler = std::make_shared<WorldStreamingScheduler>( limits );
                ++constructorAdmissions;
                source.set( source.used() );
            }
            catch( const std::bad_alloc & )
            {
                ++constructorRejections;
            }
            require( source.used() == baseline,
                     "Every constructor headroom rejection unwinds partial implementation, ledger and worker storage." );
        }
        require( constructorRejections && constructorAdmissions,
                 "A bounded constructor sweep covers both partial admission failures and full-cap teardown." );
        source.set( source.original );
        {
            WorldStreamLimits limits;
            limits.workers = 1;
            auto scheduler = std::make_shared<WorldStreamingScheduler>( limits );
            const auto schedulerBytes = source.used();
            require( schedulerBytes > baseline,
                     "Private scheduler implementation, ledger and worker inventory use shared SourceCpu." );
            source.set( schedulerBytes );
            require( !scheduler->createProducer( {}, error ) && scheduler->stats().producers == 0 &&
                         source.used() == schedulerBytes,
                     "Producer wrapper denial leaves the scheduler inventory unchanged." );
            require( !scheduler->reserve( StreamMemoryClass::Gpu, 0, error ) &&
                         scheduler->stats().used[size_t( StreamMemoryClass::Gpu )] == 0,
                     "Even a zero-byte resource reservation must admit its SourceCpu control storage." );
            source.set( source.original );
            auto producer = scheduler->createProducer( {}, error );
            require( producer != nullptr && source.used() > schedulerBytes,
                     "Producer wrapper, cancellation token and completion slots retry after Source recovery." );
            std::weak_ptr<WorldStreamProducer> weakProducer = producer;
            source.set( source.used() );
            producer.reset();
            require( weakProducer.expired() && scheduler->stats().producers == 0 &&
                         source.used() > schedulerBytes,
                     "A weak producer retains admitted control storage after its live inventory retires." );
            weakProducer.reset();
            require( source.used() == schedulerBytes,
                     "Last weak producer release returns its physical control storage at the cap." );

            source.set( source.original );
            auto lease = scheduler->reserve( StreamMemoryClass::Gpu, 64, error );
            require( lease != nullptr && source.used() > schedulerBytes,
                     "A resource lease admits its body and control block through shared SourceCpu." );
            std::weak_ptr<StreamBudgetLease> weakLease = lease;
            source.set( source.used() );
            lease.reset();
            require( weakLease.expired() &&
                         scheduler->stats().used[size_t( StreamMemoryClass::Gpu )] == 0 &&
                         source.used() > schedulerBytes,
                     "Resource bytes retire with the last strong lease while its weak control remains charged." );
            weakLease.reset();
            require( source.used() == schedulerBytes,
                     "Weak lease destruction retires the remaining SourceCpu control allocation." );
            source.set( source.used() );
        }
        require( source.used() == baseline,
                 "Private scheduler teardown returns its metadata without allocating at the Source cap." );
        source.set( source.original );

        {
            WorldStreamLimits limits;
            limits.workers = 1;
            auto scheduler = std::make_shared<WorldStreamingScheduler>( limits );
            auto blocker = scheduler->createProducer( {}, error );
            auto producer = scheduler->createProducer( {}, error );
            require( blocker && producer, "Metadata replacement fixture creates both producers." );
            Gate gate;
            auto started = std::make_shared<std::promise<void>>();
            auto entered = started->get_future();
            auto blocking = valueRequest( 300 );
            blocking.execute = [release = gate.future, started]( const StreamCancellation & ) {
                started->set_value();
                release.wait();
                return WorldStreamResult{};
            };
            require( blocker->replace( { blocking }, error ) &&
                         entered.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready,
                     "Metadata replacement fixture blocks its sole worker." );
            require( producer->replace( { valueRequest( 301 ) }, error ),
                     "Original generation is queued before metadata pressure." );
            Array<WorldStreamRequest> replacement{ valueRequest( 302 ), valueRequest( 303 ),
                                                   valueRequest( 304 ) };
            size_t rejections = 0, admissions = 0;
            for( u64 headroom = 0; headroom <= 16384; headroom += 256 )
            {
                const auto before = source.used();
                const auto previous = producer->stats();
                source.set( before + headroom );
                if( producer->replace( replacement, error ) )
                {
                    ++admissions;
                }
                else
                {
                    ++rejections;
                    const auto after = producer->stats();
                    require( !error.empty() && after.generation == previous.generation &&
                                 after.pending == previous.pending && source.used() == before,
                             "Every partial metadata denial unwinds without replacing the live generation or leaking SourceCpu." );
                }
                source.set( source.original );
            }
            require( rejections && admissions,
                     "Metadata headroom sweep covers both partial rejection and successful staging." );
            source.set( source.used() );
            producer->cancel();
            require( producer->stats().pending == 0 && producer->stats().ready == 0,
                     "Cancelling admitted pending wrappers needs no SourceCpu headroom." );
            gate.open();
            blocker->drain();
            source.set( source.original );

            struct PreparedResult
            {
                std::shared_ptr<WorldStreamingScheduler> shared;
                std::shared_ptr<const u64> value = std::make_shared<const u64>( 305 );
                std::atomic<bool> capped{ false };
            };
            auto prepared = std::make_shared<PreparedResult>();
            prepared->shared = source.scheduler;
            auto request = valueRequest( 305 );
            request.execute = [prepared]( const StreamCancellation & ) {
                String error;
                const auto used = prepared->shared->stats().used[size_t( StreamMemoryClass::SourceCpu )];
                prepared->capped.store( prepared->shared->setBudget( StreamMemoryClass::SourceCpu,
                                                                    used, error ) );
                WorldStreamResult result;
                result.payload = prepared->value;
                result.bytes = sizeof( u64 );
                return result;
            };
            require( producer->replace( { request, request }, error ) == false,
                     "Duplicate prepared keys remain invalid before publication." );
            auto next = request;
            next.key = 306;
            require( producer->replace( { request, next }, error ),
                     "Prepared requests admit every internal result/control wrapper before execution." );
            await( [&] { return producer->stats().ready == 2 && producer->stats().active == 0; },
                   "Both decoded outputs publish after the callback clamps SourceCpu to live storage." );
            require( prepared->capped.load(), "The result callback installs the exact SourceCpu cap." );
            source.set( source.used() );
            WorldStreamResult result, second;
            require( producer->take( result ) && result.key == 305 && result.payload && result.error.empty() &&
                         producer->take( second ) && second.key == 306 && second.payload && second.error.empty(),
                     "Completion front removal and result ownership transfer succeed at the full Source cap." );
            producer->drain();
            std::weak_ptr<const void> weakResult = result.payload;
            result = {};
            second = {};
            require( weakResult.expired() &&
                         scheduler->stats().used[size_t( StreamMemoryClass::ResultCpu )] == 0,
                     "Result bytes retire when the last strong result alias leaves, despite retained weak ownership." );
            const auto weakBytes = source.used();
            weakResult.reset();
            require( source.used() < weakBytes,
                     "The last weak result alias releases its admitted result-envelope control block." );
            source.set( source.original );
            require( producer->replace( { valueRequest( 307 ) }, error ),
                     "The same worker accepts more work after full-cap result transfer and teardown." );
            await( [&] { return producer->stats().ready == 1 && producer->stats().active == 0; },
                   "Worker remains usable after SourceCpu recovery." );
            producer->drain();
            source.set( source.used() );
        }
        require( source.used() == baseline,
                 "Queue, token, result-wrapper and weak-control ownership all retire after the scheduler closes." );
        std::puts( "World streaming SourceCpu metadata admission, full-cap result publication and retirement: PASS" );
    }
    inline void budgetReleaseProgress( const std::shared_ptr<WorldStreamingScheduler> &consumer,
                                      const std::shared_ptr<WorldStreamingScheduler> &reservationOwner,
                                      StreamMemoryClass memoryClass )
    {
        String error;
        const auto category = size_t( memoryClass );
        const auto before = consumer->stats();
        struct RestoreLimit
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;
            StreamMemoryClass memoryClass;
            u64 bytes;
            ~RestoreLimit()
            {
                String error;
                scheduler->setBudget( memoryClass, bytes, error );
            }
        } restore{ consumer, memoryClass, before.limits[category] };
        require( consumer->setBudget( memoryClass, before.used[category] + 64, error ),
                 "The dispatch budget permits exactly one additional reservation." );
        auto held = reservationOwner->reserve( memoryClass, 64, error );
        require( held && consumer->ownsBudget( held ),
                 "The blocking reservation belongs to the consumer's budget ledger." );
        auto producer = consumer->createProducer( {}, error );
        require( producer != nullptr, "The budget-release consumer is admitted." );
        auto executed = std::make_shared<std::atomic<u32>>( 0 );
        auto request = valueRequest( 400 );
        request.execute = [executed]( const StreamCancellation & ) {
            ++*executed;
            WorldStreamResult result;
            result.payload = std::make_shared<const u64>( 400 );
            result.bytes = sizeof( u64 );
            return result;
        };
        require( producer->replace( { request }, error ), "Budget-blocked demand is published once." );
        require( producer->stats().pending == 1 && producer->stats().active == 0 && !executed->load(),
                 "A fully reserved dispatch budget keeps its demand pending." );

        auto source = reservationOwner->reserve( StreamMemoryClass::SourceCpu, 73, error );
        require( source != nullptr, "An unrelated source reservation is admitted." );
        auto physical = allocateSourceBudgetStorage( 73, 64 );
        freeSourceBudgetStorage( physical, 64 );
        source.reset();
        require( producer->stats().pending == 1 && !executed->load(),
                 "Source-only physical and lease retirement cannot satisfy a dispatch budget." );

        held.reset();
        await( [&] { const auto stats = producer->stats(); return stats.ready == 1 && stats.active == 0; },
               "Working/result release resumes published demand without another queue mutation." );
        WorldStreamResult result;
        require( executed->load() == 1 && producer->take( result ) && result.key == 400 && result.payload,
                 "The previously blocked request executes exactly once after capacity returns." );
        result = {};
        producer->drain();
        require( consumer->stats().used[category] == before.used[category],
                 "The resumed request returns its complete dispatch reservation." );
    }

    inline void budgetWakeContracts()
    {
        WorldStreamLimits limits;
        limits.workers = 1;
        auto isolated = std::make_shared<WorldStreamingScheduler>( limits );
        budgetReleaseProgress( isolated, isolated, StreamMemoryClass::WorkingCpu );
        budgetReleaseProgress( isolated, isolated, StreamMemoryClass::ResultCpu );
        {
            String error;
            auto producer = isolated->createProducer( {}, error );
            require( producer != nullptr, "The source-only cancellation producer is admitted." );
            auto source = isolated->reserve( StreamMemoryClass::SourceCpu, 123, error );
            require( source != nullptr, "The cancellation callback retains a source lease." );
            std::weak_ptr<StreamBudgetLease> retired = source;
            auto started = std::make_shared<std::promise<void>>();
            auto entered = started->get_future();
            auto request = valueRequest( 401 );
            request.workingBytes = request.maximumResultBytes = 0;
            request.execute = [source, started]( const StreamCancellation &token ) {
                started->set_value();
                while( !token.cancelled() ) std::this_thread::yield();
                return WorldStreamResult{};
            };
            require( producer->replace( { request }, error ), "Source-only cancellation demand is published." );
            request.execute = {};
            source.reset();
            require( entered.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready,
                     "The source-only callback starts before cancellation." );
            producer->drain();
            require( retired.expired() && !producer->stats().active && !producer->stats().ready &&
                         isolated->stats().used[size_t( StreamMemoryClass::SourceCpu )] == 0,
                     "Drain observes callback completion and source retirement without a source-release wakeup." );
        }
        isolated.reset();  // Idle-worker shutdown must retain its explicit wakeup.

        auto first = WorldStreamingScheduler::shared();
        WorldStreamingScheduler::shutdownShared();
        auto restarted = WorldStreamingScheduler::shared();
        require( first != restarted, "The retained and restarted services have independent worker queues." );
        budgetReleaseProgress( restarted, first, StreamMemoryClass::WorkingCpu );
        budgetReleaseProgress( first, restarted, StreamMemoryClass::ResultCpu );
        std::puts( "World streaming source-only retirement, dispatch-budget release and retained-generation progress: PASS" );
    }

    inline void run()
    {
        WorldStreamLimits limits;
        limits.workers = 1;
        limits.maximumProducers = 4;
        limits.maximumPending = 16;
        limits.maximumReady = 32;
        limits.bytes.fill( 4096 );
        auto scheduler = std::make_shared<WorldStreamingScheduler>( limits );
        WorldStreamProducerLimits producerLimits;
        producerLimits.maximumPending = 8;
        producerLimits.maximumReady = 8;
        producerLimits.maximumReadyBytes = 512;
        String error;
        auto blocker = scheduler->createProducer( producerLimits, error );
        auto high = scheduler->createProducer( producerLimits, error );
        auto low = scheduler->createProducer( producerLimits, error );
        require( blocker && high && low && scheduler->stats().workers == 1,
                 "Shared scheduler creates one worker for several producers." );
        {
            Gate gate;
            auto started = std::make_shared<std::promise<void>>();
            auto entered = started->get_future();
            auto request = valueRequest( 1 );
            request.execute = [release = gate.future, started]( const StreamCancellation & ) {
                started->set_value();
                release.wait();
                return WorldStreamResult{};
            };
            require( blocker->replace( { request }, error ), "Gate job is admitted." );
            require( entered.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready,
                     "Gate job starts on the sole worker." );
            struct Order
            {
                std::mutex mutex;
                std::vector<u64> keys;
            };
            auto order = std::make_shared<Order>();
            Array<WorldStreamRequest> urgent;
            const auto tracked = [&]( u64 key, s32 priority ) {
                auto job = valueRequest( key, priority );
                job.execute = [order, key]( const StreamCancellation & ) {
                    {
                        std::lock_guard<std::mutex> lock( order->mutex );
                        order->keys.push_back( key );
                    }
                    return WorldStreamResult{};
                };
                return job;
            };
            for( u64 i = 10; i < 16; ++i )
                urgent.push_back( tracked( i, 100 ) );
            require( high->replace( urgent, error ) && low->replace( { tracked( 99, -100 ) }, error ),
                     "Both high and low priority demand are pending before releasing the worker." );
            gate.open();
            await(
                [&] {
                    return high->stats().ready == 6 && low->stats().ready == 1 &&
                           scheduler->stats().active == 0;
                },
                "Both producers must complete without starvation." );
            {
                std::lock_guard<std::mutex> lock( order->mutex );
                const auto lowAt = std::find( order->keys.begin(), order->keys.end(), 99 );
                require( order->keys.size() == 7 && order->keys.front() == 10 &&
                             lowAt != order->keys.end() && lowAt - order->keys.begin() <= 4,
                         "Priority is honored, then bounded fair dispatch serves the waiting "
                         "low-priority producer." );
            }
            blocker->drain();
            high->drain();
            low->drain();
        }
        {
            Gate gate;
            auto started = std::make_shared<std::promise<void>>();
            auto entered = started->get_future();
            auto request = valueRequest( 2 );
            request.execute = [release = gate.future, started]( const StreamCancellation & ) {
                started->set_value();
                release.wait();
                return WorldStreamResult{};
            };
            require( blocker->replace( { request }, error ), "Second gate is admitted." );
            require( entered.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready,
                     "Second gate starts." );
            require( high->replace( { valueRequest( 42 ) }, error ), "Original demand is queued." );
            const auto generation = high->stats().generation;
            auto impossible = valueRequest( 43 );
            impossible.workingBytes = 4097;
            require( !high->replace( { impossible }, error ) && !error.empty(),
                     "Work that can never fit its shared budget is rejected." );
            impossible = valueRequest( 43 );
            impossible.maximumResultBytes = 513;
            require( !high->replace( { impossible }, error ),
                     "Producer result cap is enforced before demand replacement." );
            require( !high->replace( { valueRequest( 43 ), valueRequest( 43 ) }, error ),
                     "Duplicate demand keys are rejected." );
            Array<WorldStreamRequest> oversized;
            for( u64 i = 0; i < 9; ++i )
                oversized.push_back( valueRequest( i ) );
            require(
                !high->replace( oversized, error ) && high->stats().generation == generation &&
                    high->stats().pending == 1,
                "Rejected replacements retain the complete original generation and pending request." );
            require( !scheduler->setBudget( StreamMemoryClass::WorkingCpu, 63, error ),
                     "Budget reduction cannot make already queued work impossible." );
            gate.open();
            await( [&] { return high->stats().ready == 1 && scheduler->stats().active == 0; },
                   "Original demand still completes after rejected replacements." );
            WorldStreamResult result;
            require( high->take( result ) && result.key == 42 && result.generation == generation &&
                         result.payload && *std::static_pointer_cast<const u64>( result.payload ) == 42,
                     "Taking a result returns original identity and immutable payload." );
            auto alias = std::shared_ptr<const u64>( result.payload,
                                                     static_cast<const u64 *>( result.payload.get() ) );
            result = {};
            blocker->drain();
            high->drain();
            require( scheduler->stats().used[size_t( StreamMemoryClass::ResultCpu )] == 64,
                     "A caller-retained result alias keeps its reserved result capacity after take and "
                     "drain." );
            require( !scheduler->setBudget( StreamMemoryClass::ResultCpu, 63, error ),
                     "Live result reservations prevent shrinking their shared budget." );
            alias.reset();
            require( scheduler->stats().used[size_t( StreamMemoryClass::ResultCpu )] == 0,
                     "Last result alias release returns its reservation immediately." );
        }
        {
            require( scheduler->setBudget( StreamMemoryClass::ResultCpu, 64, error ),
                     "Shared result cap can be lowered after all aliases retire." );
            require( high->replace( { valueRequest( 60 ), valueRequest( 61 ) }, error ),
                     "Several individually feasible results may queue behind a shared cap." );
            await(
                [&] {
                    const auto value = high->stats();
                    return value.ready == 1 && value.active == 0;
                },
                "First bounded result completes before the second is admitted." );
            WorldStreamResult held;
            require( high->take( held ) && held.key == 60 && high->stats().pending == 1,
                     "Taking a retained result does not bypass the global result budget." );
            require( scheduler->stats().used[size_t( StreamMemoryClass::ResultCpu )] == 64,
                     "Global result capacity remains reserved by the caller's immutable payload." );
            held = {};
            await( [&] { return high->stats().ready == 1 && high->stats().active == 0; },
                   "Releasing the result wakes queued work without replacing demand." );
            WorldStreamResult next;
            require( high->take( next ) && next.key == 61,
                     "Previously blocked demand completes in order." );
            next = {};
            high->drain();
            require( scheduler->setBudget( StreamMemoryClass::ResultCpu, 4096, error ),
                     "Result budget restored after bounded scheduling contract." );
        }
        {
            auto oversized = valueRequest( 70 );
            oversized.execute = []( const StreamCancellation & ) {
                WorldStreamResult result;
                result.payload = std::make_shared<const u64>( 70 );
                result.bytes = 65;  // Violates this job's declared maximum of 64.
                return result;
            };
            require( high->replace( { oversized, valueRequest( 71 ) }, error ),
                     "Declared bounded requests are accepted before execution." );
            await( [&] { return high->stats().ready == 2 && high->stats().active == 0; },
                   "An invalid producer result must not terminate the shared worker." );
            WorldStreamResult result;
            require(
                high->take( result ) && result.key == 70 && !result.payload && !result.error.empty(),
                "Oversized producer output becomes an explicit failure without a partial payload." );
            require( high->take( result ) && result.key == 71 && result.payload && result.error.empty(),
                     "The same worker continues servicing valid work after a rejected output." );
            result = {};
            high->drain();
        }
        {
            // Callback capture owns a source lease. drain must retire the capture as
            // well as the cancelled output before reporting the producer inactive.
            auto lease = scheduler->reserve( StreamMemoryClass::SourceCpu, 123, error );
            require( lease != nullptr, "Source lease is admitted." );
            std::weak_ptr<StreamBudgetLease> retired = lease;
            auto started = std::make_shared<std::promise<void>>();
            auto entered = started->get_future();
            auto cancelled = std::make_shared<std::atomic_bool>( false );
            auto request = valueRequest( 50 );
            request.execute = [lease, started, cancelled]( const StreamCancellation &token ) {
                started->set_value();
                while( !token.cancelled() )
                    std::this_thread::yield();
                cancelled->store( true );
                WorldStreamResult result;
                result.payload = std::make_shared<const u64>( 50 );
                result.bytes = sizeof( u64 );
                return result;
            };
            require( low->replace( { request }, error ), "Cancellable source job is admitted." );
            request.execute = {};
            lease.reset();
            require( entered.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready,
                     "Cancellable source job starts." );
            const auto generation = low->stats().generation;
            low->drain();
            WorldStreamResult discarded;
            require( cancelled->load() && retired.expired() && !low->take( discarded ) &&
                         low->stats().generation > generation && low->stats().active == 0,
                     "Drain cancels execution, retires captured source ownership and prevents stale "
                     "publication." );
            require( scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )] == 0 &&
                         scheduler->stats().used[size_t( StreamMemoryClass::WorkingCpu )] == 0 &&
                         scheduler->stats().used[size_t( StreamMemoryClass::ResultCpu )] == 0,
                     "Drain releases cancelled source, working and discarded result reservations." );
        }
        {
            auto gpu = scheduler->reserve( StreamMemoryClass::Gpu, 4096, error );
            auto alias = gpu;
            require( gpu && !scheduler->reserve( StreamMemoryClass::Gpu, 1, error ),
                     "Shared admission rejects combined allocations above the class budget." );
            const auto pressure = scheduler->stats();
            require( pressure.peakUsed[size_t( StreamMemoryClass::Gpu )] == 4096 &&
                         pressure.rejectedReservations[size_t( StreamMemoryClass::Gpu )] == 1,
                     "Memory telemetry records the physical reservation peak and explicit rejection." );
            gpu.reset();
            require( scheduler->stats().used[size_t( StreamMemoryClass::Gpu )] == 4096,
                     "Shared active/staged ownership charges one actual allocation." );
            alias.reset();
            require( scheduler->setBudget( StreamMemoryClass::Gpu, 0, error ),
                     "Released class budget can shrink to zero." );
            require( scheduler->reserve( StreamMemoryClass::Gpu, 0, error ) != nullptr &&
                         !scheduler->reserve( StreamMemoryClass::Gpu, 1, error ),
                     "Zero-byte admission succeeds without bypassing zero-byte limits." );
            const auto retired = scheduler->stats();
            require( retired.used[size_t( StreamMemoryClass::Gpu )] == 0 &&
                         retired.peakUsed[size_t( StreamMemoryClass::Gpu )] == 4096 &&
                         retired.rejectedReservations[size_t( StreamMemoryClass::Gpu )] == 2 &&
                         retired.peakUsed[size_t( StreamMemoryClass::WorkingCpu )] >= 64 &&
                         retired.peakUsed[size_t( StreamMemoryClass::ResultCpu )] >= 64,
                     "Retirement and budget changes preserve peaks, including worker reservations." );
        }
        auto fourth = scheduler->createProducer( producerLimits, error );
        require( fourth && !scheduler->createProducer( producerLimits, error ) && !error.empty(),
                 "Global producer count is bounded independently of each producer's queue." );
        fourth.reset();
        blocker.reset();
        high.reset();
        low.reset();
        const auto final = scheduler->stats();
        require( final.producers == 0 && !final.pending && !final.active && !final.ready &&
                     std::all_of( final.used.begin(), final.used.end(),
                                  []( u64 bytes ) { return bytes == 0; } ),
                 "Producer destruction drains work and returns every participating reservation." );
        std::puts(
            "World streaming priority/fairness, atomic rejection, cancellation and budget lifetime: "
            "PASS" );
        sourceMetadataContracts();
        budgetWakeContracts();
    }
}  // namespace world_streaming_contracts
#endif
