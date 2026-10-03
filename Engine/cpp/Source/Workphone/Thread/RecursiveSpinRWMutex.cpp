#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Thread/RecursiveSpinRWMutex.hpp>
#include <unordered_map>

#if !WP_FINAL
#    include "Workphone/Thread/ThreadDiagnostics.hpp"
#endif

#if defined _DEBUG
#    include <Workphone/System/DebugUtil.hpp>
#endif

namespace workphone
{

    RecursiveSpinRWMutex::RecursiveSpinRWMutex() = default;

    RecursiveSpinRWMutex::~RecursiveSpinRWMutex()
    {
#if !WP_FINAL
        thread_diagnostics::check( state.load( std::memory_order_relaxed ) == 0,
                                   "RecursiveSpinRWMutex destroyed while locked" );
        thread_diagnostics::check( writer_owner.load( std::memory_order_relaxed ) == std::thread::id{},
                                   "RecursiveSpinRWMutex destroyed while write locked" );
        thread_diagnostics::check( writer_recursion == 0,
                                   "RecursiveSpinRWMutex destroyed with non-zero writer recursion" );
        thread_diagnostics::check( writer_read_recursion == 0,
                                   "RecursiveSpinRWMutex destroyed with writer-held read locks" );
#endif

        // Verify mutex is not held on destruction
        assert( state.load( std::memory_order_relaxed ) == 0 &&
                "RecursiveSpinRWMutex destroyed while locked" );
        assert( writer_owner.load( std::memory_order_relaxed ) == std::thread::id{} &&
                "RecursiveSpinRWMutex destroyed while write-locked" );
    }

    void RecursiveSpinRWMutex::lock_shared()
    {
        const auto tid = std::this_thread::get_id();

#if defined _DEBUG
        debugStr = DebugUtil::getStackTrace();
#endif

        // Case 1: Writer recursion - writer can acquire read locks freely
        // These are tracked separately and don't affect global state
        if( writer_owner.load( std::memory_order_acquire ) == tid )
        {
            ++writer_read_recursion;
            return;
        }

        // Case 2: Recursive read - already hold a read lock on this specific mutex
        auto &recursion = local_reader_recursion();
        if( recursion > 0 )
        {
            ++recursion;
            return;
        }

        // Case 3: First read acquisition - must acquire global state
        unsigned spin_count = 0;
        for( ;; )
        {
            // Writer preference: wait if writers are queued
            while( writers_waiting.load( std::memory_order_acquire ) > 0 )
            {
                if( ++spin_count > MAX_SPIN_COUNT )
                {
                    std::this_thread::yield();
                    spin_count = 0;
                }
                else
                {
                    _cpu_relax();
                }
            }

            int expected = state.load( std::memory_order_relaxed );

            // Can only acquire if no writer holds the lock (state >= 0)
            if( expected >= 0 )
            {
                if( state.compare_exchange_weak( expected, expected + 1, std::memory_order_acquire,
                                                 std::memory_order_relaxed ) )
                {
                    recursion = 1;
                    return;
                }
            }

            if( ++spin_count > MAX_SPIN_COUNT )
            {
                std::this_thread::yield();
                spin_count = 0;
            }
            else
            {
                _cpu_relax();
            }
        }
    }

    void RecursiveSpinRWMutex::unlock_shared()
    {
        const auto tid = std::this_thread::get_id();

        // Case 1: Writer releasing a "virtual" read lock
        if( writer_owner.load( std::memory_order_acquire ) == tid )
        {
#if !WP_FINAL
            thread_diagnostics::check( writer_read_recursion > 0,
                                       "RecursiveSpinRWMutex writer shared unlock without shared lock" );
#endif

            assert( writer_read_recursion > 0 &&
                    "unlock_shared called by writer without matching lock_shared" );
            --writer_read_recursion;
            return;
        }

        // Case 2: Regular reader releasing
        auto &recursion = local_reader_recursion();

#if !WP_FINAL
        thread_diagnostics::check( recursion > 0,
                                   "RecursiveSpinRWMutex shared unlock without matching shared lock" );
#endif

        assert( recursion > 0 && "unlock_shared called without matching lock_shared" );

        if( --recursion == 0 )
        {
            // Last recursive unlock - release global reader count
            [[maybe_unused]] int prev = state.fetch_sub( 1, std::memory_order_release );
            assert( prev > 0 && "State inconsistency: unlock_shared with no readers" );
        }
    }

    bool RecursiveSpinRWMutex::try_lock()
    {
        const auto tid = std::this_thread::get_id();

        // Case 1: Recursive write - already own the lock
        if( writer_owner.load( std::memory_order_acquire ) == tid )
        {
            ++writer_recursion;

#if defined _DEBUG
            debugStr = DebugUtil::getStackTrace();
#endif
            return true;
        }

        // Case 2: Check for lock upgrade (reader trying to become writer)
        auto &recursion = local_reader_recursion();
        const bool was_reader = ( recursion > 0 );

        if( was_reader )
        {
            // Attempt upgrade: we need state to be exactly 1 (only us as reader)
            // to safely upgrade without releasing first
            int expected = 1;
            if( state.compare_exchange_strong( expected, -1, std::memory_order_acquire,
                                               std::memory_order_relaxed ) )
            {
                // Successful upgrade - we were the only reader
                writer_owner.store( tid, std::memory_order_relaxed );
                writer_recursion = 1;
                // Transfer read recursion to writer_read_recursion
                writer_read_recursion = recursion;
                recursion = 0;

#if defined _DEBUG
                debugStr = DebugUtil::getStackTrace();
#endif
                return true;
            }

            // Upgrade failed - other readers exist, keep our read lock
            return false;
        }

        // Case 3: Not a reader, try direct acquisition
        int expected = 0;
        if( state.compare_exchange_strong( expected, -1, std::memory_order_acquire,
                                           std::memory_order_relaxed ) )
        {
            writer_owner.store( tid, std::memory_order_relaxed );
            writer_recursion = 1;

#if defined _DEBUG
            debugStr = DebugUtil::getStackTrace();
#endif
            return true;
        }

        return false;
    }

    bool RecursiveSpinRWMutex::try_lock_shared()
    {
        const auto tid = std::this_thread::get_id();

        // Case 1: Writer can always acquire read locks
        if( writer_owner.load( std::memory_order_acquire ) == tid )
        {
            ++writer_read_recursion;
            return true;
        }

        // Case 2: Already hold read lock on this mutex
        auto &recursion = local_reader_recursion();
        if( recursion > 0 )
        {
            ++recursion;
            return true;
        }

        // Case 3: Writer preference - fail immediately if writers waiting
        if( writers_waiting.load( std::memory_order_acquire ) > 0 )
        {
            return false;
        }

        // Case 4: Try to acquire without blocking
        int expected = state.load( std::memory_order_relaxed );
        if( expected >= 0 &&
            state.compare_exchange_strong( expected, expected + 1, std::memory_order_acquire,
                                           std::memory_order_relaxed ) )
        {
            recursion = 1;
            return true;
        }

        return false;
    }

    void RecursiveSpinRWMutex::lock()
    {
        const auto tid = std::this_thread::get_id();

#if defined _DEBUG
        debugStr = DebugUtil::getStackTrace();
#endif

        // Case 1: Recursive write
        if( writer_owner.load( std::memory_order_acquire ) == tid )
        {
            ++writer_recursion;
            return;
        }

        // Case 2: Lock upgrade (reader trying to become writer)
        // WARNING: This can deadlock if multiple readers try to upgrade simultaneously!
        auto &recursion = local_reader_recursion();
        const bool was_reader = ( recursion > 0 );
        unsigned saved_recursion = recursion;

        if( was_reader )
        {
            // Release our read contribution first to avoid self-deadlock
            // This creates a brief window where we hold no lock
            state.fetch_sub( 1, std::memory_order_release );
            recursion = 0;
        }

        // Signal that a writer is waiting (for reader preference)
        writers_waiting.fetch_add( 1, std::memory_order_relaxed );

        unsigned spin_count = 0;
        for( ;; )
        {
            int expected = 0;
            if( state.compare_exchange_weak( expected, -1, std::memory_order_acquire,
                                             std::memory_order_relaxed ) )
            {
                writer_owner.store( tid, std::memory_order_relaxed );
                writer_recursion = 1;
                writers_waiting.fetch_sub( 1, std::memory_order_relaxed );

                // Restore read recursion as writer-held reads
                if( was_reader )
                {
                    writer_read_recursion = saved_recursion;
                }

                return;
            }

            if( ++spin_count > MAX_SPIN_COUNT )
            {
                std::this_thread::yield();
                spin_count = 0;
            }
            else
            {
                _cpu_relax();
            }
        }
    }

    void RecursiveSpinRWMutex::unlock()
    {
#if !WP_FINAL
        thread_diagnostics::check(
            writer_owner.load( std::memory_order_relaxed ) == std::this_thread::get_id(),
            "RecursiveSpinRWMutex unlocked by a thread that does not own it" );
        thread_diagnostics::check( writer_recursion > 0,
                                   "RecursiveSpinRWMutex unlocked with zero writer recursion" );
#endif

        assert( writer_owner.load( std::memory_order_relaxed ) == std::this_thread::get_id() &&
                "unlock called by non-owner thread" );

        assert( writer_recursion > 0 && "unlock called with zero write recursion" );

        if( --writer_recursion == 0 )
        {
#if !WP_FINAL
            thread_diagnostics::check(
                writer_read_recursion == 0,
                "RecursiveSpinRWMutex writer unlocked while still holding shared locks" );
#endif

            // Verify no outstanding read locks held by writer
            assert( writer_read_recursion == 0 &&
                    "Writer releasing write lock while still holding read locks" );

            writer_owner.store( std::thread::id{}, std::memory_order_relaxed );
            state.store( 0, std::memory_order_release );
        }
    }

    bool RecursiveSpinRWMutex::is_write_locked_by_current_thread() const
    {
        return writer_owner.load( std::memory_order_acquire ) == std::this_thread::get_id();
    }

    bool RecursiveSpinRWMutex::is_read_locked_by_current_thread() const
    {
        const auto tid = std::this_thread::get_id();

        // Check if we're the writer with read locks
        if( writer_owner.load( std::memory_order_acquire ) == tid )
        {
            return writer_read_recursion > 0;
        }

        // Check regular read lock
        return local_reader_recursion() > 0;
    }

    void RecursiveSpinRWMutex::_cpu_relax()
    {
#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __i386__ ) || defined( _M_IX86 )
        _mm_pause();
#elif defined( __aarch64__ ) || defined( _M_ARM64 )
        asm volatile( "yield" );
#elif defined( __arm__ ) || defined( _M_ARM )
        asm volatile( "yield" );
#else
        std::this_thread::yield();
#endif
    }

    unsigned &RecursiveSpinRWMutex::local_reader_recursion()
    {
        // Per-thread map from mutex instance to recursion count
        // This fixes the critical bug where all mutexes shared the same counter
        static thread_local std::unordered_map<const RecursiveSpinRWMutex *, unsigned> recursion_map;
        return recursion_map[this];
    }

    unsigned RecursiveSpinRWMutex::local_reader_recursion() const
    {
        static thread_local std::unordered_map<const RecursiveSpinRWMutex *, unsigned> recursion_map;
        auto it = recursion_map.find( this );
        return ( it != recursion_map.end() ) ? it->second : 0;
    }

    void RecursiveSpinRWMutex::clear_local_reader_recursion()
    {
        static thread_local std::unordered_map<const RecursiveSpinRWMutex *, unsigned> recursion_map;
        recursion_map.erase( this );
    }

}  // namespace workphone
