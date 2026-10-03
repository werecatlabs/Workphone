#ifndef SpinRWMutex_h__
#define SpinRWMutex_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <atomic>

namespace workphone
{

    /**
     * A read-write mutex implemented using spin locks.
     */
    class WPCore_API SpinRWMutex
    {
    public:
        /**
         * A scoped lock for the SpinRWMutex.
         */
        class WPCore_API ScopedLock
        {
        public:
            /**
             * Constructs a lock for the specified SpinRWMutex.
             * @param m The SpinRWMutex to lock.
             * @param write True if the lock should be exclusive for writing; false if it's a shared read
             * lock.
             */
            ScopedLock( SpinRWMutex &m, bool write = true );

            /**
             * Releases the lock.
             */
            ~ScopedLock();

            /**
             * Returns true if the lock was successfully acquired; false otherwise.
             */
            operator bool() const;

        protected:
            /** The mutex. */
            SpinRWMutex &m_mutex;

            /** True if the lock is a write lock. */
            bool m_write = true;
        };

        /**
         * Constructor.
         */
        SpinRWMutex();

        /**
         * Destructor.
         */
        ~SpinRWMutex();

        /**
         * Locks the mutex for shared read access.
         */
        void lock_shared();

        /**
         * Unlocks the shared read lock.
         */
        void unlock_shared();

        /**
         * Locks the mutex for exclusive write access.
         */
        void lock();

        /**
         * Unlocks the exclusive write lock.
         */
        void unlock();

    protected:
        ///< The number of readers currently holding a shared lock.
        std::atomic<s32> readers;

        ///< The number of writers currently holding an exclusive lock.
        std::atomic<s32> writers;

        ///< True if there are pending write lock requests.
        std::atomic<bool> writeRequest;
    };

}  // namespace workphone

#endif  // SpinRWMutex_h__
