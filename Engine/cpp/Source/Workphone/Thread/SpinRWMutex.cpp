#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <thread>

#if !WP_FINAL
#    include <Workphone/Thread/ThreadDiagnostics.hpp>
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
                                   "SpinRWMutex destroyed while acquisition is in progress" );
#endif
    }

    void SpinRWMutex::lock_shared()
    {
#if !WP_FINAL
        thread_diagnostics::assertCanAcquireShared( this, "SpinRWMutex" );
#endif
        for( ;; )
        {
            // Register readers under the same gate used by writers. Checking the
            // writer count and incrementing readers must be one admission step.
            while( writeRequest.exchange( true, std::memory_order_acquire ) )
                std::this_thread::yield();

            if( writers.load( std::memory_order_acquire ) == 0 )
            {
                readers.fetch_add( 1, std::memory_order_relaxed );
                writeRequest.store( false, std::memory_order_release );
                return;
            }

            writeRequest.store( false, std::memory_order_release );
            std::this_thread::yield();
        }
    }

    void SpinRWMutex::unlock_shared()
    {
        const auto previousReaders = readers.fetch_sub( 1, std::memory_order_release );
#if !WP_FINAL
        thread_diagnostics::check( previousReaders > 0,
                                   "SpinRWMutex shared unlock called with no active readers" );
#else
        WP_UNUSED( previousReaders );
#endif
    }

    void SpinRWMutex::lock()
    {
#if !WP_FINAL
        thread_diagnostics::assertCanAcquireExclusive( this, "SpinRWMutex" );
#endif
        while( writeRequest.exchange( true, std::memory_order_acquire ) )
            std::this_thread::yield();

        // Holding the admission gate stops new readers and other writers from
        // entering while existing owners drain. Unlock does not need the gate.
        while( readers.load( std::memory_order_acquire ) != 0 ||
               writers.load( std::memory_order_acquire ) != 0 )
            std::this_thread::yield();

        writers.store( 1, std::memory_order_relaxed );
        writeRequest.store( false, std::memory_order_release );
    }

    void SpinRWMutex::unlock()
    {
        const auto previousWriters = writers.exchange( 0, std::memory_order_release );
#if !WP_FINAL
        thread_diagnostics::check( previousWriters == 1,
                                   "SpinRWMutex exclusive unlock called without an active writer" );
#else
        WP_UNUSED( previousWriters );
#endif
    }

    SpinRWMutex::ScopedLock::ScopedLock( SpinRWMutex &m, bool write ) :
        m_mutex( m ), m_write( write )
    {
        if( m_write )
            m_mutex.lock();
        else
            m_mutex.lock_shared();
    }

    SpinRWMutex::ScopedLock::~ScopedLock()
    {
        if( m_write )
            m_mutex.unlock();
        else
            m_mutex.unlock_shared();
    }

    SpinRWMutex::ScopedLock::operator bool() const
    {
        return m_write ? (m_mutex.writers.load( std::memory_order_acquire ) > 0)
                       : (m_mutex.readers.load( std::memory_order_acquire ) > 0);
    }
}  // namespace workphone
