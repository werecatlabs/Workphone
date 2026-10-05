#ifndef SpinMutex_h__
#define SpinMutex_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <atomic>

namespace workphone
{

    /**
     * A spinlock mutex implementation that uses atomic_flag to achieve synchronization.
     */
    class WPCore_API SpinMutex
    {
    public:
        /**
         * A lock guard that automatically locks and unlocks the SpinMutex.
         */
        class WPCore_API ScopedLock
        {
        public:
            /**
             * Constructs a ScopeLock and acquires the SpinMutex lock.
             * @param m The SpinMutex to lock.
             */
            ScopedLock( SpinMutex &m );

            ScopedLock( const ScopedLock & ) = delete;
            ScopedLock &operator=( const ScopedLock & ) = delete;

            /**
             * Releases the SpinMutex lock.
             */
            ~ScopedLock();

        protected:
            /**< The SpinMutex that is locked by the ScopeLock. */
            SpinMutex &m_mutex;
        };

        /** Constructor */
        SpinMutex();

        /** Destructor */
        ~SpinMutex();

        /**
         * Locks the SpinMutex.
         */
        void lock();

        /**
         * Unlocks the SpinMutex.
         */
        void unlock();

    private:
        /**< The atomic flag used to synchronize access to the SpinMutex. */
        std::atomic_flag locked = ATOMIC_FLAG_INIT;
    };

}  // namespace workphone

#endif  // SpinMutex_h__
