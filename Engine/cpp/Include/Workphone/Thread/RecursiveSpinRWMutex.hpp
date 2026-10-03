#ifndef RecursiveSpinRWMutex_h__
#define RecursiveSpinRWMutex_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <atomic>
#include <thread>
#include <cassert>
#include <unordered_map>

namespace workphone
{

    /**
     * @class RecursiveSpinRWMutex
     * @brief A lightweight recursive reader-writer mutex implemented with atomics and spinning.
     *
     * This mutex supports:
     * - Multiple concurrent readers.
     * - A single writer that may recursively acquire the write lock.
     * - Per-thread, per-instance recursive reader counts.
     * - Writer can acquire read locks (downgradeable).
     * - Safe lock upgrade attempts (reader to writer).
     *
     * Notes:
     * - Readers use a per-instance, per-thread recursion counter via thread_local map.
     * - The `state` atomic holds the number of readers when >= 0, and -1 when a writer holds the lock.
     * - `writer_owner` and `writer_recursion` track write ownership and recursion depth for the owning
     * thread.
     *
     * Edge cases handled:
     * - Multiple mutex instances per thread (separate recursion counters).
     * - Lock upgrade from reader to writer (safely releases read first).
     * - Writer holding read locks (counted separately, no global decrement needed).
     * - Starvation prevention via writer preference.
     *
     * This implementation is intended for scenarios where locks are held for short durations and
     * the overhead of blocking primitives is undesirable. It uses spinning and should not be used
     * for long-held locks or when thread preemption would make busy-waiting inefficient.
     *
     * @note This class is NOT a full replacement for platform-provided RW locks in all workloads.
     */
    class WPCore_API RecursiveSpinRWMutex
    {
    public:
        /**
         * @brief Construct a new RecursiveSpinRWMutex.
         *
         * Initializes internal atomics and counters to the unlocked state.
         */
        RecursiveSpinRWMutex();
        ~RecursiveSpinRWMutex();

        // Non-copyable, non-movable
        RecursiveSpinRWMutex( const RecursiveSpinRWMutex & ) = delete;
        RecursiveSpinRWMutex &operator=( const RecursiveSpinRWMutex & ) = delete;
        RecursiveSpinRWMutex( RecursiveSpinRWMutex && ) = delete;
        RecursiveSpinRWMutex &operator=( RecursiveSpinRWMutex && ) = delete;

        // --------------------
        // Read lock
        // --------------------

        /**
         * @brief Acquire the lock for reading.
         *
         * If the calling thread already holds the read lock, the per-thread recursion
         * count is incremented and the global reader count is not further changed
         * beyond the first acquisition. If a writer holds or is acquiring the lock,
         * readers will spin until the writer releases it.
         *
         * Writers can also acquire read locks recursively without deadlock.
         */
        void lock_shared();

        /**
         * @brief Release a previously acquired read lock.
         *
         * Decrements the per-thread recursion counter; when it reaches zero the
         * global reader count is decremented so writers may proceed.
         *
         * @pre The calling thread must have previously acquired a read lock.
         */
        void unlock_shared();

        /**
         * @brief Try to acquire an exclusive (write) lock without blocking.
         *
         * @return true if the exclusive lock was acquired by the calling thread; false otherwise.
         *
         * If the calling thread already owns the mutex, this behaves like a non-blocking recursive
         * acquisition and increments the recursion count.
         *
         * @note If the caller holds a read lock, this will attempt to upgrade. On failure,
         *       the read lock is preserved (not lost).
         */
        bool try_lock();

        /**
         * @brief Try to acquire a shared/non-exclusive (read) lock without blocking.
         *
         * @return true if a shared lock was acquired; false otherwise.
         */
        bool try_lock_shared();

        // --------------------
        // Write lock
        // --------------------

        /**
         * @brief Acquire the lock for writing.
         *
         * If the calling thread already owns the write lock (`writer_owner`), the
         * `writer_recursion` counter is incremented (recursive write locking).
         * Otherwise, the thread signals that a writer is waiting and spins until
         * it can atomically claim exclusive ownership.
         *
         * While a writer holds the lock, `state == -1` to indicate exclusive access.
         *
         * @warning If the caller holds a read lock, this will attempt to upgrade.
         *          Upgrading can cause deadlock if another thread also holds a read lock
         *          and attempts to upgrade simultaneously. Use try_lock() for safe upgrades.
         */
        void lock();

        /**
         * @brief Release a previously acquired write lock.
         *
         * Decrements the write recursion counter and, when it reaches zero, clears
         * the writer ownership and restores `state` to zero so readers may proceed.
         *
         * @pre The calling thread must be the current write owner.
         */
        void unlock();

        /**
         * @brief Check if the current thread holds the write lock.
         * @return true if the calling thread owns the write lock.
         */
        bool is_write_locked_by_current_thread() const;

        /**
         * @brief Check if the current thread holds any read lock on this mutex.
         * @return true if the calling thread has read lock recursion > 0.
         */
        bool is_read_locked_by_current_thread() const;

    private:
        /**
         * @brief Get the per-thread, per-instance reader recursion counter.
         * @return Reference to the calling thread's reader recursion counter for this mutex.
         */
        unsigned &local_reader_recursion();

        /**
         * @brief Get the per-thread, per-instance reader recursion counter (const version).
         * @return The calling thread's reader recursion counter for this mutex.
         */
        unsigned local_reader_recursion() const;

        /**
         * @brief Clear the per-thread reader recursion counter for this mutex.
         */
        void clear_local_reader_recursion();

        /**
         * @brief Global state for readers/writer.
         *
         * Value semantics:
         * - -1 means a writer currently holds the lock (exclusive).
         * - >= 0 means the number of active readers.
         */
        std::atomic<int> state{ 0 };

        /**
         * @brief Number of writers currently waiting to acquire the lock.
         *
         * Helps give writers priority (or aid in backoff) when many readers are present.
         */
        std::atomic<int> writers_waiting{ 0 };

        // Writer ownership
        /**
         * @brief Thread id of the current writer owner.
         *
         * Default constructed `std::thread::id{}` means no owner.
         */
        std::atomic<std::thread::id> writer_owner{};

        /**
         * @brief Recursive write lock count for the owning thread.
         *
         * Incremented when the owning thread calls `lock()` recursively and
         * decremented on `unlock()`. When this reaches zero the ownership is released.
         */
        unsigned writer_recursion{ 0 };

        /**
         * @brief Tracks how many "virtual" read locks the writer holds.
         *
         * When a writer calls lock_shared(), we don't modify global state but track it here.
         * This allows proper unlock_shared() behavior when the writer releases read locks.
         */
        unsigned writer_read_recursion{ 0 };

#if defined _DEBUG
        String debugStr;
#endif

        /**
         * @brief Platform-agnostic CPU relaxation hint used while spinning.
         *
         * May use architecture-specific pause instructions or yield to reduce
         * power consumption and improve hyper-threading performance while busy-waiting.
         */
        static void _cpu_relax();

        /**
         * @brief Maximum spin iterations before yielding to OS scheduler.
         */
        static constexpr unsigned MAX_SPIN_COUNT = 1000;
    };

}  // namespace workphone

#endif  // RecursiveSpinRWMutex_h__
