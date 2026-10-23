#ifndef WPWorldStreamingContracts_h__
#define WPWorldStreamingContracts_h__

#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <chrono>
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
    }
}  // namespace world_streaming_contracts
#endif
