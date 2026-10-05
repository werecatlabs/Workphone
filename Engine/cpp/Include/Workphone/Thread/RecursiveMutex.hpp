#ifndef RecursiveMutex_h__
#define RecursiveMutex_h__

#include <Workphone/Atomics/AtomicTypes.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <unordered_map>

namespace workphone
{
    /**
     * @class RecursiveMutex
     * @brief A recursive reader-writer mutex with upgrade capability.
     *
     * This synchronization primitive supports:
     * - Recursive exclusive (writer) locking by a single owning thread.
     * - Multiple concurrent shared (reader) locks across threads.
     * - An upgrade lock that allows a reader to be promoted to a writer.
     *
     * The implementation keeps per-thread shared lock counts to allow safe
     * nested acquire/release of shared locks by the same thread. Upgrade
     * semantics aim to prevent writer starvation and make reader->writer
     * promotion possible when no other readers are present.
     *
     * Usage notes:
     * - Use `lock`/`unlock` for exclusive access.
     * - Use `lock_shared`/`unlock_shared` for shared (read) access.
     * - `try_lock` / `try_lock_shared` provide non-blocking attempts.
     * - `try_lock_upgrade` / `unlock_upgrade` implement an upgradeable reader.
     * - `try_unlock_shared_and_lock` attempts an atomic promotion from shared to exclusive.
     *
     * Thread-safety:
     * - All public methods are thread-safe.
     * - Callers must ensure balanced lock/unlock calls for the lock type acquired.
     */
    class WPCore_API RecursiveMutex
    {
    public:
        /**
         * @brief RAII scoped exclusive lock for `RecursiveMutex`.
         *
         * Acquires an exclusive lock on construction (by calling `lock`) and
         * releases it on destruction (by calling `unlock`). The object is
         * non-copyable; conversion to `bool` indicates whether the object
         * currently holds the lock.
         */
        class WPCore_API ScopedLock
        {
        public:
            /**
             * @brief Construct and acquire exclusive lock.
             * @param mutex Reference to the `RecursiveMutex` to lock.
             */
            explicit ScopedLock( RecursiveMutex &mutex );

            ScopedLock( const ScopedLock & ) = delete;
            ScopedLock &operator=( const ScopedLock & ) = delete;

            /**
             * @brief Release the exclusive lock if held.
             */
            ~ScopedLock();

            /**
             * @brief Bool conversion.
             * @return true if the scoped object is valid (holds a reference).
             */
            operator bool();

        protected:
            RecursiveMutex &m_mutex;
        };

        /**
         * @brief RAII scoped shared lock for `RecursiveMutex`.
         *
         * Acquires a shared (reader) lock on construction and releases it on
         * destruction. Conversion to `bool` indicates validity.
         */
        class WPCore_API ScopedSharedLock
        {
        public:
            /**
             * @brief Construct and acquire a shared lock.
             * @param mutex Reference to the `RecursiveMutex` to lock.
             */
            explicit ScopedSharedLock( RecursiveMutex &mutex );

            ScopedSharedLock( const ScopedSharedLock & ) = delete;
            ScopedSharedLock &operator=( const ScopedSharedLock & ) = delete;

            /**
             * @brief Release the shared lock if held.
             */
            ~ScopedSharedLock();

            /**
             * @brief Bool conversion.
             * @return true if the scoped object is valid (holds a reference).
             */
            operator bool();

        protected:
            RecursiveMutex &m_mutex;
        };

        /**
         * @brief Construct a `RecursiveMutex`.
         *
         * Initializes internal state. No threads hold locks after construction.
         */
        RecursiveMutex();

        /**
         * @brief Destroy the `RecursiveMutex`.
         *
         * Behaviour is undefined if destroyed while locks are still held by any thread.
         */
        ~RecursiveMutex();

        /* Exclusive (writer) locking interface */

        /**
         * @brief Acquire an exclusive (writer) lock.
         *
         * Blocks the calling thread until it can obtain exclusive access.
         * If the calling thread already owns the exclusive lock it will be
         * allowed to re-enter (recursive behavior) and an internal recursion
         * counter is incremented.
         */
        void lock();

        /**
         * @brief Try to acquire an exclusive (writer) lock without blocking.
         * @return true if the lock was acquired, false otherwise.
         *
         * This will succeed immediately only if no other thread holds the
         * exclusive lock and there are no active readers (except possibly the
         * calling thread if it already owned it).
         */
        bool try_lock();

        /**
         * @brief Release an exclusive (writer) lock.
         *
         * Decrements the recursion counter and releases ownership when the
         * counter reaches zero, waking waiting threads as necessary.
         *
         * @warning Calling `unlock` without owning the exclusive lock is undefined behavior.
         */
        void unlock();

        /* Shared (reader) locking interface */

        /**
         * @brief Acquire a shared (reader) lock.
         *
         * Multiple threads may hold shared locks concurrently. The implementation
         * tracks per-thread shared lock counts so the same thread may acquire
         * shared locks nestedly and must call `unlock_shared` the same number
         * of times.
         */
        void lock_shared();

        /**
         * @brief Try to acquire a shared (reader) lock without blocking.
         * @return true if a shared lock was acquired, false otherwise.
         *
         * This will fail if there is an exclusive owner or an upgrade in progress
         * that blocks new readers.
         */
        bool try_lock_shared();

        /**
         * @brief Release a shared (reader) lock.
         *
         * Decrements the per-thread shared lock count and the global shared
         * count. If the global shared count reaches zero and writers are waiting,
         * they may be notified.
         *
         * @warning Calling `unlock_shared` more times than `lock_shared` by the
         * same thread is undefined behavior.
         */
        void unlock_shared();

        /**
         * @brief Acquire a special shared lock mode intended for write promotion.
         *
         * This mode may be used to indicate an intent to upgrade (reader->writer)
         * and can interact with the upgrade state machine. Specific semantics are
         * implementation dependent and intended for internal use in this class.
         */
        void lock_shared_write();

        /**
         * @brief Release the special shared-write mode acquired by `lock_shared_write`.
         */
        void unlock_shared_write();

        /* Upgrade (reader -> writer) interface */

        /**
         * @brief Try to acquire the upgradeable lock.
         * @return true if the upgrade lock was successfully acquired.
         *
         * An upgrade lock identifies a thread that intends to promote from reader
         * to writer. Typically only one upgrade lock may be held at a time.
         */
        bool try_lock_upgrade();

        /**
         * @brief Release the upgrade lock.
         *
         * If the upgrade lock was held while also being a shared lock, the shared
         * count tracking must be updated accordingly by the caller.
         */
        void unlock_upgrade();

        /* Conversion operations */

        /**
         * @brief Attempt to atomically convert a held shared lock into an exclusive lock.
         * @return true if conversion succeeded and the caller now owns the exclusive lock.
         *
         * This method tries to upgrade the calling thread's shared lock to an
         * exclusive lock without releasing the mutex to other threads in the
         * middle of the operation. It will only succeed when no other readers
         * exist (except the caller) and no exclusive owner is present.
         */
        bool try_unlock_shared_and_lock();

    protected:
        /**
         * @brief Internal mutex protecting the `RecursiveMutex` state.
         *
         * All mutable state in this object is guarded by `m_mutex`.
         */
        std::mutex m_mutex;

        /**
         * @brief Condition variable used to block/wake waiting threads.
         *
         * Waiters wait on `m_condition` while attempting to acquire exclusive,
         * shared, or upgrade locks.
         */
        std::condition_variable m_condition;

        /* Exclusive lock state */

        /**
         * @brief Thread id of the exclusive lock owner.
         *
         * Default constructed to empty id when no owner exists.
         */
        std::thread::id m_owner_thread = std::thread::id();

        /**
         * @brief Thread id holding the upgrade lock (reader that may promote).
         *
         * Default constructed to empty id when no upgrade is held.
         */
        std::thread::id m_upgrade_thread = std::thread::id();

        /**
         * @brief Recursion count for the exclusive owner.
         *
         * Incremented when the exclusive owner re-enters `lock` and decremented
         * on `unlock`. When zero no exclusive owner exists.
         */
        atomic_s32 m_count = 0;

        /**
         * @brief Global count of active shared (reader) locks.
         *
         * This is the sum of per-thread shared lock counts.
         */
        atomic_s32 m_shared_count = 0;

        /**
         * @brief Flag indicating an upgrade operation is in progress.
         *
         * When true, new readers may be blocked to allow promotion.
         */
        atomic_bool m_upgrade_mode = false;

        /**
         * @brief Per-thread shared lock counters.
         *
         * Tracks how many times each thread has acquired a shared lock so that
         * nested shared lock/unlock operations are safe and balanced.
         */
        std::unordered_map<std::thread::id, atomic_s32> m_shared_locks_per_thread;
    };

}  // namespace workphone

#endif  // RecursiveMutex_h__
