#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <thread>
#if defined( _MSC_VER )
#    include <intrin.h>
#endif

#if WP_USE_ONETBB
//#    include <tbb/tbb_thread.h>
#elif WP_USE_TBB
#    include <tbb/tbb_thread.h>
#else
#    include <thread>
#endif

#if WP_USE_BOOST
#    include <boost/thread.hpp>

#    if defined( WP_USE_INTERLOCKED_FUNCTIONS ) && defined( WP_USE_BOOST )
#        include <boost/detail/interlocked.hpp>
#    endif
#endif

namespace workphone
{
    WP_THREAD_LOCAL_STORAGE s32 CURRENT_TASK_ID = 0;
    WP_THREAD_LOCAL_STORAGE u32 CURRENT_THREAD_ID = 0;
    WP_THREAD_LOCAL_STORAGE u32 CURRENT_THREAD_FLAGS = 0;

    const u32 Thread::Primary_Flag = 1 << 1;
    const u32 Thread::Ai_Flag = 1 << 2;
    const u32 Thread::Animation_Flag = 1 << 3;
    const u32 Thread::Application_Flag = 1 << 4;
    const u32 Thread::Collision_Flag = 1 << 5;
    const u32 Thread::Controls_Flag = 1 << 6;
    const u32 Thread::Dynamics_Flag = 1 << 7;
    const u32 Thread::GarbageCollect_Flag = 1 << 8;
    const u32 Thread::Input_Flag = 1 << 9;
    const u32 Thread::Physics_Flag = 1 << 10;
    const u32 Thread::None_Flag = 1 << 11;
    const u32 Thread::Render_Flag = 1 << 12;
    const u32 Thread::Sound_Flag = 1 << 13;

    FixedArray<u32, static_cast<u32>( TaskId::Count )> Thread::m_taskFlags;
    SpinRWMutex Thread::m_taskFlagsMutex;

    auto Thread::getCurrentTask() -> TaskId
    {
        WP_ASSERT( CURRENT_TASK_ID >= 0 );
        WP_ASSERT( CURRENT_TASK_ID < static_cast<int>( TaskId::Count ) );
        return static_cast<TaskId>( CURRENT_TASK_ID );
    }

    void Thread::setCurrentTask( TaskId task )
    {
        WP_ASSERT( static_cast<s32>( task ) >= 0 );
        WP_ASSERT( static_cast<s32>( task ) < static_cast<int>( TaskId::Count ) );
        CURRENT_TASK_ID = static_cast<u32>( task );
    }

    auto Thread::getTaskFlags() -> u32
    {
        SpinRWMutex::ScopedLock lock( m_taskFlagsMutex, false );
        auto task = static_cast<u32>( getCurrentTask() );
        return m_taskFlags[task];
    }

    void Thread::setTaskFlags( u32 taskFlags )
    {
        SpinRWMutex::ScopedLock lock( m_taskFlagsMutex, true );
        auto task = static_cast<u32>( getCurrentTask() );
        m_taskFlags[task] = taskFlags;
    }

    auto Thread::getTaskFlags( TaskId task ) -> u32
    {
        SpinRWMutex::ScopedLock lock( m_taskFlagsMutex, false );
        return m_taskFlags[static_cast<u32>( task )];
    }

    void Thread::setTaskFlags( TaskId task, u32 taskFlags )
    {
        SpinRWMutex::ScopedLock lock( m_taskFlagsMutex, true );
        m_taskFlags[static_cast<u32>( task )] = taskFlags;
    }

    auto Thread::getTaskFlag( u32 flag ) -> bool
    {
        SpinRWMutex::ScopedLock lock( m_taskFlagsMutex, false );
        auto task = static_cast<u32>( getCurrentTask() );
        return ( m_taskFlags[task] & flag ) != 0;
    }

    void Thread::setTaskFlag( u32 flag, bool value )
    {
        SpinRWMutex::ScopedLock lock( m_taskFlagsMutex, true );

        auto task = static_cast<u32>( getCurrentTask() );

        if( value )
        {
            m_taskFlags[task] |= flag;
        }
        else
        {
            m_taskFlags[task] &= ~flag;
        }
    }

    void Thread::sleep( time_interval seconds )
    {
#if 0
		if (seconds == 0)
			return;

		static LARGE_INTEGER s_freq = { 0,0 };

		if (s_freq.QuadPart == 0)
			QueryPerformanceFrequency(&s_freq);

		LARGE_INTEGER from, now;
		QueryPerformanceCounter(&from);
		int ticks_to_wait = (int)((double)s_freq.QuadPart / (1.0 / seconds));
		bool done = false;

		int ticks_passed;
		int ticks_left;

		do
		{
			QueryPerformanceCounter(&now);
			ticks_passed = (int)((__int64)now.QuadPart - (__int64)from.QuadPart);
			ticks_left = ticks_to_wait - ticks_passed;

			if (now.QuadPart < from.QuadPart)    // time wrap
				done = true;

			if (ticks_passed >= ticks_to_wait)
				done = true;

			if (!done)
			{
				if (ticks_left > (int)s_freq.QuadPart * 2 / 1000)
					tbb::this_tbb_thread::sleep(tbb::tick_count::interval_t(seconds * 0.25));
				else
					tbb::this_tbb_thread::yield();
			}
		} while (!done);
#else

#    if WP_USE_TBB
        if( seconds > 0.0 )
        {
            tbb::this_tbb_thread::sleep( tbb::tick_count::interval_t( seconds ) );
        }
#    elif WP_USE_ONETBB
        if( seconds > time_interval( 0.0 ) )
        {
            std::chrono::duration<double, std::milli> t( seconds * 1000 );
            std::this_thread::sleep_for( t );
        }
#    else
        if( seconds > time_interval( 0.0 ) )
        {
            std::chrono::duration<double, std::milli> t( seconds * 1000 );
            std::this_thread::sleep_for( t );
        }
#    endif
#endif
    }

    void Thread::interlockedExchange( volatile long *target, long value )
    {
#if defined( _MSC_VER )
        _InterlockedExchange( target, value );
#else
        __atomic_exchange_n( target, value, __ATOMIC_SEQ_CST );
#endif
    }

    void Thread::interlockedExchangePointer( void **target, void *value )
    {
#if defined( _MSC_VER )
        _InterlockedExchangePointer( target, value );
#else
        __atomic_exchange_n( target, value, __ATOMIC_SEQ_CST );
#endif
    }

    long Thread::interlockedCompareExchange( volatile long *target, long expected, long desired )
    {
#if defined( _MSC_VER )
        return _InterlockedCompareExchange( target, desired, expected );
#else
        __atomic_compare_exchange_n( target, &expected, desired, false,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST );
        return expected;
#endif
    }

    long Thread::interlockedDecrement( volatile long *value )
    {
#if defined( _MSC_VER )
        return _InterlockedDecrement( value );
#else
        return __atomic_sub_fetch( value, 1, __ATOMIC_SEQ_CST );
#endif
    }

    long Thread::interlockedIncrement( volatile long *value )
    {
#if defined( _MSC_VER )
        return _InterlockedIncrement( value );
#else
        return __atomic_add_fetch( value, 1, __ATOMIC_SEQ_CST );
#endif
    }

    void Thread::yield()
    {
#if WP_USE_TBB
        tbb::this_tbb_thread::yield();
#elif WP_USE_ONETBB
        std::this_thread::yield();
#else
        std::this_thread::yield();
#endif
    }

    auto Thread::getCurrentThreadId() -> Thread::ThreadId
    {
        return static_cast<ThreadId>( CURRENT_THREAD_ID );
    }

    void Thread::setCurrentThreadId( ThreadId threadId )
    {
        CURRENT_THREAD_ID = static_cast<u32>( threadId );
    }

    auto Thread::hardware_concurrency() -> u32
    {
#if WP_USE_BOOST
        return boost::thread::hardware_concurrency();
#elif WP_USE_TBB
        return tbb::tbb_thread::hardware_concurrency();
#else
        return std::thread::hardware_concurrency();
#endif
    }

    auto Thread::physical_concurrency() -> u32
    {
#if WP_USE_BOOST
        return boost::thread::physical_concurrency();
#elif WP_USE_TBB
        return tbb::tbb_thread::hardware_concurrency();
#else
        return std::thread::hardware_concurrency();
#endif
    }

    auto Thread::getTaskName( TaskId id ) -> String
    {
        switch( id )
        {
        case TaskId::Primary:
            return String( "PRIMARY" );
        case TaskId::Render:
            return String( "RENDER" );
        case TaskId::Physics:
            return String( "PHYSICS" );
        case TaskId::Controls:
            return String( "CONTROLS" );
        case TaskId::Collision:
            return String( "COLLISION" );
        case TaskId::Application:
            return String( "APPLICATION" );
        case TaskId::Ai:
            return String( "AI" );
        case TaskId::Animation:
            return String( "ANIMATION" );
        case TaskId::Sound:
            return String( "SOUND" );
        case TaskId::Dynamics:
            return String( "DYNAMICS" );
        case TaskId::GarbageCollect:
            return String( "GARBAGE_COLLECT" );
        case TaskId::Fluid:
            return String( "FLUID" );
        case TaskId::Input:
            return String( "INPUT" );
        case TaskId::None:
            return String( "NONE" );
        case TaskId::SoftBody:
            return String( "SOFT_BODY" );
        case TaskId::Count:
        default:
            break;
        }

        return String( "UNKNOWN" );
    }
}  // namespace workphone
