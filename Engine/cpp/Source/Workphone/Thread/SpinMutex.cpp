#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Thread/SpinMutex.hpp>
#include <thread>

#if !WP_FINAL
#    include "Workphone/Thread/ThreadDiagnostics.hpp"
#endif

namespace workphone
{

    SpinMutex::SpinMutex() = default;
    SpinMutex::~SpinMutex()
    {
#if !WP_FINAL
        thread_diagnostics::assertUnlockedOnDestroy( this, "SpinMutex" );
#endif
    }

    void SpinMutex::lock()
    {
#if !WP_FINAL
        thread_diagnostics::assertCanAcquireExclusive( this, "SpinMutex" );
#endif

        while( locked.test_and_set( std::memory_order_acquire ) )
        {
            std::this_thread::yield();
        }

#if !WP_FINAL
        thread_diagnostics::markExclusiveAcquired( this, "SpinMutex" );
#endif
    }

    void SpinMutex::unlock()
    {
#if !WP_FINAL
        thread_diagnostics::markExclusiveReleased( this, "SpinMutex" );
#endif

        locked.clear( std::memory_order_release );
    }

    SpinMutex::ScopedLock::ScopedLock( SpinMutex &m ) : m_mutex( m )
    {
        m_mutex.lock();
    }

    SpinMutex::ScopedLock::~ScopedLock()
    {
        m_mutex.unlock();
    }

}  // namespace workphone
