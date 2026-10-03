#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

#if !WP_FINAL
#    include "Workphone/Thread/ThreadDiagnostics.hpp"
#endif

namespace workphone
{

    RecursiveSpinMutex::RecursiveSpinMutex() = default;

    RecursiveSpinMutex::~RecursiveSpinMutex()
    {
#if !WP_FINAL
        thread_diagnostics::check( owner.load( std::memory_order_relaxed ) == std::thread::id{},
                                   "RecursiveSpinMutex destroyed while owned" );
        thread_diagnostics::check( recursion == 0,
                                   "RecursiveSpinMutex destroyed with non-zero recursion" );
#endif
    }

    void RecursiveSpinMutex::lock()
    {
        std::thread::id this_id = std::this_thread::get_id();

        // Fast path: already owned by this thread
        if( owner.load( std::memory_order_relaxed ) == this_id )
        {
            ++recursion;
            return;
        }

        // Spin until we acquire ownership
        while( true )
        {
            std::thread::id no_owner;
            if( owner.compare_exchange_weak( no_owner, this_id, std::memory_order_acquire ) )
            {
                recursion = 1;
                return;
            }

            _cpu_relax();
        }
    }

    void RecursiveSpinMutex::unlock()
    {
#if !WP_FINAL
        thread_diagnostics::check( owner.load( std::memory_order_relaxed ) == std::this_thread::get_id(),
                                   "RecursiveSpinMutex unlocked by a thread that does not own it" );
        thread_diagnostics::check( recursion > 0, "RecursiveSpinMutex unlocked with zero recursion" );
#endif

        if( owner.load( std::memory_order_relaxed ) != std::this_thread::get_id() )
        {
            std::terminate();  // unlocking from wrong thread
        }

        if( --recursion == 0 )
        {
            owner.store( std::thread::id{}, std::memory_order_release );
        }
    }

    void RecursiveSpinMutex::lock_shared()
    {
        lock();
    }

    bool RecursiveSpinMutex::try_lock()
    {
        std::thread::id this_id = std::this_thread::get_id();

        // Fast path: already owned by this thread
        if( owner.load( std::memory_order_relaxed ) == this_id )
        {
            ++recursion;
            return true;
        }

        // Try to acquire ownership (non-blocking)
        std::thread::id no_owner;
        if( owner.compare_exchange_strong( no_owner, this_id, std::memory_order_acquire ) )
        {
            recursion = 1;
            return true;
        }

        // Failed to acquire
        return false;
    }

    bool RecursiveSpinMutex::try_lock_shared()
    {
        return try_lock();
    }

    void RecursiveSpinMutex::unlock_shared()
    {
        unlock();
    }

    void RecursiveSpinMutex::_cpu_relax()
    {
#if defined( __x86_64__ ) || defined( _M_X64 )
        _mm_pause();
#elif defined( __aarch64__ )
        asm volatile( "yield" );
#else
        // fallback
        std::this_thread::yield();
#endif
    }

}  // namespace workphone
