#ifndef RecursiveSpinMutex_h__
#define RecursiveSpinMutex_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <atomic>
#include <thread>

namespace workphone
{

    /**
     * @brief A lightweight recursive spin mutex.
     *
     * This mutex implementation uses busy-waiting (spinning) and supports:
     *  - Exclusive (recursive) locking via `lock`, `try_lock`, and `unlock`.
     *  - Non-exclusive/shared locking via `lock_shared`, `try_lock_shared`, and `unlock_shared`.
     *
     * Notes:
     *  - Exclusive ownership is tracked per-thread using `owner` and `recursion`.
     *  - Recursive locking allows the owning thread to `lock` multiple times; ownership
     *    must be released the same number of times by calling `unlock`.
     *  - Being a spin-based mutex, prefer it for short critical sections to avoid wasting CPU.
     *  - This type intentionally favors low-overhead sync; it does not provide fairness guarantees.
     *
     * @thread_safety Locking operations are thread-safe; callers are responsible for correct usage.
     */
    class WPCore_API RecursiveSpinMutex
    {
    public:
        /**
         * @brief Construct a new RecursiveSpinMutex.
         *
         * Initializes internal state; no thread owns the mutex after construction.
         */
        RecursiveSpinMutex();

        /**
         * @brief Destroy the RecursiveSpinMutex.
         *
         * Behavior is undefined if destroyed while still owned/locked by any thread.
         */
        ~RecursiveSpinMutex();

        /**
         * @brief Acquire an exclusive (write) lock.
         *
         * Blocks by spinning until the calling thread becomes the owner. If the calling thread
         * already owns the mutex, the recursion count is incremented (recursive lock).
         */
        void lock();

        /**
         * @brief Release an exclusive (write) lock previously acquired by this thread.
         *
         * Decrements the recursion count; if the recursion count reaches zero, ownership is
         * released so other threads may acquire the mutex.
         */
        void unlock();

        /**
         * @brief Acquire a non-exclusive/shared (read) lock.
         *
         * Blocks by spinning until a shared lock can be obtained. Multiple threads may hold
         * shared locks concurrently. The exact semantics of interaction between shared and
         * exclusive locks are implementation-defined; consult the .cpp implementation for details.
         */
        void lock_shared();

        /**
         * @brief Try to acquire an exclusive (write) lock without blocking.
         *
         * @return true if the exclusive lock was acquired by the calling thread; false otherwise.
         *
         * If the calling thread already owns the mutex, this behaves like a non-blocking recursive
         * acquisition and increments the recursion count.
         */
        bool try_lock();

        /**
         * @brief Try to acquire a shared/non-exclusive (read) lock without blocking.
         *
         * @return true if a shared lock was acquired; false otherwise.
         */
        bool try_lock_shared();

        /**
         * @brief Release a shared/non-exclusive (read) lock previously acquired.
         *
         * Releases one unit of shared ownership. If this thread held multiple shared locks
         * (if supported), call this once per acquisition.
         */
        void unlock_shared();

    private:
        /// Thread id of the current exclusive owner (or default-constructed id when unlocked).
        std::atomic<std::thread::id> owner;

        /// Recursive acquisition count for the exclusive owner.
        unsigned recursion{ 0 };

        /// Hint to the CPU inside spin loop to reduce power/priority impact.
        static void _cpu_relax();
    };

}  // namespace workphone

#endif  // RecursiveSpinMutex_h__
