#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <thread>

#if !WP_FINAL
#    include "Workphone/Thread/ThreadDiagnostics.hpp"
#endif

namespace workphone
{
    SpinRWMutex::SpinRWMutex() : readers( 0 ), writers( 0 ), writeRequest( false )
    {
    }

    SpinRWMutex::~SpinRWMutex()
    {
#if !WP_FINAL
        thread_diagnostics::check( readers.load( std::memory_order_relaxed ) == 0,
                                   "SpinRWMutex destroyed while readers are active" );
        thread_diagnostics::check( writers.load( std::memory_order_relaxed ) == 0,
                                   "SpinRWMutex destroyed while a writer is active" );
        thread_diagnostics::check( !writeRequest.load( std::memory_order_relaxed ),
                                   "SpinRWMutex destroyed while a writer is waiting" );
        //thread_diagnostics::assertUnlockedOnDestroy( this, "SpinRWMutex" );
#endif
    }

    void SpinRWMutex::lock_shared()
    {
#if !WP_FINAL
        thread_diagnostics::assertCanAcquireShared( this, "SpinRWMutex" );
#endif

        while( writers.load( std::memory_order_acquire ) > 0 )
        {
            std::this_thread::yield();
        }

        readers.fetch_add( 1, std::memory_order_acquire );

#if !WP_FINAL
        //thread_diagnostics::markSharedAcquired( this, "SpinRWMutex" );
        //thread_diagnostics::check( writers.load( std::memory_order_acquire ) == 0,
        //                            "SpinRWMutex shared lock overlaps an active writer" );
#endif
    }

    void SpinRWMutex::unlock_shared()
    {
#if !WP_FINAL
        //thread_diagnostics::markSharedReleased( this, "SpinRWMutex" );
        const auto previousReaders = readers.fetch_sub( 1, std::memory_order_release );
        //thread_diagnostics::check( previousReaders > 0,
        //                           "SpinRWMutex shared unlock called with no active readers" );
#else
        readers.fetch_sub( 1, std::memory_order_release );
#endif
    }

    void SpinRWMutex::lock()
    {
#if !WP_FINAL
        thread_diagnostics::assertCanAcquireExclusive( this, "SpinRWMutex" );
#endif

        while( writeRequest.exchange( true, std::memory_order_acquire ) )
        {
            std::this_thread::yield();
        }

        while( readers.load( std::memory_order_acquire ) != 0 )
        {
            std::this_thread::yield();
        }

        const auto previousWriters = writers.fetch_add( 1, std::memory_order_acquire );

#if !WP_FINAL
        //thread_diagnostics::check( previousWriters == 0,
        //                            "SpinRWMutex exclusive lock acquired while another writer is active" );
        //thread_diagnostics::check( readers.load( std::memory_order_acquire ) == 0,
        //                            "SpinRWMutex exclusive lock acquired while readers are active" );
        // thread_diagnostics::markExclusiveAcquired( this, "SpinRWMutex" );
#else
        WP_UNUSED( previousWriters );
#endif

        writeRequest.store( false, std::memory_order_release );
    }

    void SpinRWMutex::unlock()
    {
#if !WP_FINAL
        //thread_diagnostics::markExclusiveReleased( this, "SpinRWMutex" );
        const auto previousWriters = writers.fetch_sub( 1, std::memory_order_release );
        //thread_diagnostics::check( previousWriters == 1,
        //                           "SpinRWMutex exclusive unlock called without one active writer" );
#else
        writers.fetch_sub( 1, std::memory_order_release );
#endif
    }

    SpinRWMutex::ScopedLock::ScopedLock( SpinRWMutex &m, bool write /*= true*/ ) :
        m_mutex( m ),
        m_write( write )
    {
        if( m_write )
        {
            m_mutex.lock();
        }
        else
        {
            m_mutex.lock_shared();
        }
    }

    SpinRWMutex::ScopedLock::~ScopedLock()
    {
        if( m_write )
        {
            m_mutex.unlock();
        }
        else
        {
            m_mutex.unlock_shared();
        }
    }

    SpinRWMutex::ScopedLock::operator bool() const
    {
        // Return true if the lock was successfully acquired
        return m_write ? ( m_mutex.writers.load( std::memory_order_acquire ) > 0 )
                       : ( m_mutex.readers.load( std::memory_order_acquire ) > 0 );
    }
}  // namespace workphone
