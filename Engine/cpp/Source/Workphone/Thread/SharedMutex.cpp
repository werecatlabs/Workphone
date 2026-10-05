#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Thread/SharedMutex.hpp>

#if !WP_FINAL
#    include "Workphone/Thread/ThreadDiagnostics.hpp"

#    include <mutex>
#    include <unordered_map>
#endif

namespace workphone
{
    SharedMutex::SharedMutex() = default;
    SharedMutex::~SharedMutex()
    {
#if !WP_FINAL
        std::lock_guard<std::mutex> lock( mutex_ );
        thread_diagnostics::check( !writeLocked_, "SharedMutex destroyed while write locked" );
        thread_diagnostics::check( readCount_ == 0, "SharedMutex destroyed while read locked" );
        thread_diagnostics::assertUnlockedOnDestroy( this, "SharedMutex" );
#endif
    }

    void SharedMutex::lock_shared()
    {
#if !WP_FINAL
        thread_diagnostics::assertCanAcquireShared( this, "SharedMutex" );
#endif

        std::unique_lock<std::mutex> lock( mutex_ );
        while( writeLocked_ )
        {
            readCondition_.wait( lock );
        }
        ++readCount_;

#if !WP_FINAL
        thread_diagnostics::markSharedAcquired( this, "SharedMutex" );
        thread_diagnostics::check( !writeLocked_, "SharedMutex shared lock overlaps an active writer" );
#endif
    }

    void SharedMutex::unlock_shared()
    {
        std::lock_guard<std::mutex> lock( mutex_ );

#if !WP_FINAL
        thread_diagnostics::check( readCount_ > 0,
                                   "SharedMutex shared unlock called with no active readers" );
        thread_diagnostics::markSharedReleased( this, "SharedMutex" );
#endif

        --readCount_;
        if( readCount_ == 0 )
        {
            writeCondition_.notify_one();
        }
    }

    void SharedMutex::lock()
    {
#if !WP_FINAL
        thread_diagnostics::assertCanAcquireExclusive( this, "SharedMutex" );
#endif

        std::unique_lock<std::mutex> lock( mutex_ );
        while( writeLocked_ || readCount_ > 0 )
        {
            writeCondition_.wait( lock );
        }
        writeLocked_ = true;

#if !WP_FINAL
        thread_diagnostics::markExclusiveAcquired( this, "SharedMutex" );
#endif
    }

    void SharedMutex::unlock()
    {
        std::lock_guard<std::mutex> lock( mutex_ );

#if !WP_FINAL
        thread_diagnostics::check( writeLocked_,
                                   "SharedMutex exclusive unlock called without an active writer" );
        thread_diagnostics::markExclusiveReleased( this, "SharedMutex" );
#endif

        writeLocked_ = false;
        readCondition_.notify_all();
        writeCondition_.notify_one();
    }

    SharedMutex::ScopedLock::ScopedLock( SharedMutex &m, bool write ) : m_mutex( m ), m_write( write )
    {
        if( write )
        {
            m_mutex.lock();
        }
        else
        {
            m_mutex.lock_shared();
        }
    }

    SharedMutex::ScopedLock::~ScopedLock()
    {
        if( m_write )
            m_mutex.unlock();
        else
            m_mutex.unlock_shared();
    }

}  // namespace workphone
