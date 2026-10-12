#ifndef WPWorldStreamingScheduler_h__
#define WPWorldStreamingScheduler_h__

#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <array>
#include <atomic>
#include <functional>
#include <memory>

namespace workphone
{
    enum class StreamMemoryClass : u32 { SourceCpu, PendingCpu, WorkingCpu, ResultCpu, Gpu, Collision, Count };
    struct WorldStreamLimits
    {
        u32 workers = 2, maximumProducers = 128, maximumPending = 2048, maximumReady = 64;
        // GPU covers the union of active/staged buffers. These are reservations for
        // participating producers, not a cap on unrelated engine/caller allocations.
        std::array<u64, size_t( StreamMemoryClass::Count )> bytes = {
            512ull<<20, 16ull<<20, 128ull<<20, 64ull<<20, 512ull<<20, 512ull<<20 };
    };
    struct WorldStreamStats
    {
        u32 workers = 0, producers = 0, pending = 0, active = 0, ready = 0;
        u64 completed = 0, cancelled = 0;
        std::array<u64, size_t( StreamMemoryClass::Count )> used{}, limits{};
    };
    struct StreamBudgetState;
    class WPCore_API StreamBudgetLease final
    {
    public:
        ~StreamBudgetLease();
        u64 bytes() const;
        StreamMemoryClass memoryClass() const;
        StreamBudgetLease( const StreamBudgetLease & ) = delete;
        StreamBudgetLease &operator=( const StreamBudgetLease & ) = delete;
    private:
        friend class WorldStreamingScheduler;
        StreamBudgetLease( std::shared_ptr<StreamBudgetState>, StreamMemoryClass, u64 );
        std::shared_ptr<StreamBudgetState> m_state;
        StreamMemoryClass m_class;
        u64 m_bytes;
    };
    using StreamBudget = std::shared_ptr<StreamBudgetLease>;

    class StreamCancellation
    {
    public:
        bool cancelled() const { return m_cancelled && m_cancelled->load(); }
        explicit StreamCancellation( std::shared_ptr<std::atomic<bool>> value ) : m_cancelled( std::move( value ) ) {}
    private:
        std::shared_ptr<std::atomic<bool>> m_cancelled;
    };
    struct WorldStreamResult
    {
        u64 key = 0, generation = 0, bytes = 0;
        std::shared_ptr<const void> payload;  // Returned payload retains its result reservation.
        String error;
    };
    struct WorldStreamRequest
    {
        u64 key = 0;
        s32 priority = 0;
        u64 pendingBytes = 256, workingBytes = 0, maximumResultBytes = 0;
        std::function<WorldStreamResult( const StreamCancellation & )> execute;
    };
    struct WorldStreamProducerLimits
    {
        u32 maximumPending = 256, maximumActive = 1, maximumReady = 8;
        u64 maximumReadyBytes = 8ull<<20;
    };
    struct WorldStreamProducerStats
    {
        u64 generation = 0, completed = 0, cancelled = 0, readyBytes = 0;
        u32 pending = 0, active = 0, ready = 0;
    };
    class WorldStreamProducer;

    /** Shared cross-producer CPU preparation and resource admission. A fixed worker
     * pool gives priority jobs a bounded burst, then services the least recently
     * dispatched eligible producer. Jobs must poll cancellation and never access GPU
     * or mutable scene objects. Consumers take immutable results on their owner thread.
     */
    class WPCore_API WorldStreamingScheduler final : public std::enable_shared_from_this<WorldStreamingScheduler>
    {
    public:
        explicit WorldStreamingScheduler( const WorldStreamLimits &limits = {} );
        ~WorldStreamingScheduler();
        static std::shared_ptr<WorldStreamingScheduler> shared();
        /// Release the shared service after its consumers have been unloaded,
        /// while worker threads can still join (before DLL process teardown).
        static void shutdownShared();
        StreamBudget reserve( StreamMemoryClass memoryClass, u64 bytes, String &error );
        bool setBudget( StreamMemoryClass memoryClass, u64 bytes, String &error );
        std::shared_ptr<WorldStreamProducer> createProducer( const WorldStreamProducerLimits &, String &error );
        WorldStreamStats stats() const;
        WorldStreamingScheduler( const WorldStreamingScheduler & ) = delete;
        WorldStreamingScheduler &operator=( const WorldStreamingScheduler & ) = delete;
    private:
        friend class WorldStreamProducer;
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };

    class WPCore_API WorldStreamProducer final
    {
    public:
        ~WorldStreamProducer();
        // Demand mutation, consumption and destruction belong to one consumer
        // owner thread. Worker callbacks may inspect cancellation and return data;
        // they must not call producer mutation/drain or destroy their producer.
        // Atomic demand replacement: rejection preserves existing demand/results.
        bool replace( const Array<WorldStreamRequest> &requests, String &error );
        // Input/resource leases use this producer's service, including when tests
        // or an isolated application supply a scheduler other than shared().
        StreamBudget reserve( StreamMemoryClass memoryClass, u64 bytes, String &error );
        bool take( WorldStreamResult &result );
        // A retained loader can reserve against this service after its submitting
        // producer is destroyed. The callable owns the scheduler, not the producer.
        std::function<StreamBudget(StreamMemoryClass,u64,String &)> budgetAllocator() const;
        void cancel();
        void drain();  // Owner thread only; cancels, then waits for executing callbacks.
        WorldStreamProducerStats stats() const;
        WorldStreamProducer( const WorldStreamProducer & ) = delete;
        WorldStreamProducer &operator=( const WorldStreamProducer & ) = delete;
    private:
        friend class WorldStreamingScheduler;
        WorldStreamProducer( std::shared_ptr<WorldStreamingScheduler>, u64 );
        std::shared_ptr<WorldStreamingScheduler> m_scheduler;
        u64 m_id;
    };
}
#endif
