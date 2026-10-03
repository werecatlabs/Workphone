#ifndef SharedMutex_h__
#define SharedMutex_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <iostream>
#include <mutex>
#include <condition_variable>

namespace workphone
{

    /** A shared mutex that allows multiple readers or one writer. */
    class WPCore_API SharedMutex
    {
    public:
        /** A scoped lock for a SharedMutex. */
        class WPCore_API ScopedLock
        {
        public:
            /** Constructor. */
            ScopedLock( SharedMutex &m, bool write = true );

            /** Destructor. */
            ~ScopedLock();

        protected:
            /** The mutex. */
            SharedMutex &m_mutex;
        };

        /** Constructor. */
        SharedMutex();

        /** Destructor. */
        ~SharedMutex();

        /** Lock the mutex for reading. */
        void lock_shared();

        /** Unlock the mutex after reading. */
        void unlock_shared();

        /** Lock the mutex for writing. */
        void lock();

        /** Unlock the mutex after writing. */
        void unlock();

    private:
        /** The mutex. */
        std::mutex mutex_;

        /** The condition variable for readers. */
        std::condition_variable readCondition_;

        /** The condition variable for writers. */
        std::condition_variable writeCondition_;

        /** The number of readers. */
        bool writeLocked_ = false;

        /** The number of readers. */
        s32 readCount_ = 0;
    };

}  // namespace workphone

#endif  // SharedMutex_h__
