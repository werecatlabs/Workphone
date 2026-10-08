#ifndef WorkphoneThreadDiagnostics_h__
#define WorkphoneThreadDiagnostics_h__

#include <Workphone/WorkphoneConfig.hpp>

#if !WP_FINAL

#    include <Workphone/WorkphoneTypes.hpp>
#    include <cstdlib>
#    include <mutex>
#    include <thread>
#    include <unordered_map>

namespace workphone::thread_diagnostics
{
    inline void check( bool condition, const char *message )
    {
        if( condition )
        {
            return;
        }

        WP_UNUSED( message );

#    if defined( WP_PLATFORM_WIN32 ) && defined( _MSC_VER )
        __debugbreak();
#    else
        std::abort();
#    endif
    }

    inline auto trackerMutex() -> std::mutex &
    {
        // Static mutex destructors must be able to use diagnostics at shutdown.
        static auto *mutex = new std::mutex;
        return *mutex;
    }

    inline auto exclusiveOwners() -> std::unordered_map<const void *, std::thread::id> &
    {
        static auto *owners = new std::unordered_map<const void *, std::thread::id>;
        return *owners;
    }

    inline auto sharedTotals() -> std::unordered_map<const void *, s32> &
    {
        static auto *totals = new std::unordered_map<const void *, s32>;
        return *totals;
    }

    inline auto sharedLocksByThread()
        -> std::unordered_map<std::thread::id, std::unordered_map<const void *, s32>> &
    {
        // TLS is destroyed before global guards. Keep their ownership records alive too.
        static auto *locks =
            new std::unordered_map<std::thread::id, std::unordered_map<const void *, s32>>;
        return *locks;
    }

    inline auto currentThreadSharedLocks() -> std::unordered_map<const void *, s32> &
    {
        return sharedLocksByThread()[std::this_thread::get_id()];
    }

    inline auto currentThreadSharedCount( const void *lock ) -> s32
    {
        auto thread = sharedLocksByThread().find( std::this_thread::get_id() );
        if( thread == sharedLocksByThread().end() )
            return 0;
        auto &locks = thread->second;
        auto it = locks.find( lock );
        return it != locks.end() ? it->second : 0;
    }

    inline void assertUnlockedOnDestroy( const void *lock, const char *lockName )
    {
        std::lock_guard<std::mutex> guard( trackerMutex() );

        WP_UNUSED( lockName );

        auto ownerIt = exclusiveOwners().find( lock );
        check( ownerIt == exclusiveOwners().end() || ownerIt->second == std::thread::id{},
               "Thread lock destroyed while exclusively owned" );

        auto sharedIt = sharedTotals().find( lock );
        check( sharedIt == sharedTotals().end() || sharedIt->second == 0,
               "Thread lock destroyed while shared readers are active" );

        exclusiveOwners().erase( lock );
        sharedTotals().erase( lock );
    }

    inline void assertCanAcquireExclusive( const void *lock, const char *lockName )
    {
        std::lock_guard<std::mutex> guard( trackerMutex() );

        WP_UNUSED( lockName );

        const auto currentThread = std::this_thread::get_id();
        auto ownerIt = exclusiveOwners().find( lock );
        check( ownerIt == exclusiveOwners().end() || ownerIt->second != currentThread,
               "Thread attempted to recursively acquire a non-recursive exclusive lock" );
        check( currentThreadSharedCount( lock ) == 0,
               "Thread attempted to acquire an exclusive lock while holding a shared lock" );
    }

    inline void markExclusiveAcquired( const void *lock, const char *lockName )
    {
        std::lock_guard<std::mutex> guard( trackerMutex() );

        WP_UNUSED( lockName );

        auto ownerIt = exclusiveOwners().find( lock );
        check( ownerIt == exclusiveOwners().end() || ownerIt->second == std::thread::id{},
               "Exclusive lock acquired while already owned" );

        auto sharedIt = sharedTotals().find( lock );
        check( sharedIt == sharedTotals().end() || sharedIt->second == 0,
               "Exclusive lock acquired while shared readers are active" );

        exclusiveOwners()[lock] = std::this_thread::get_id();
    }

    inline void markExclusiveReleased( const void *lock, const char *lockName )
    {
        std::lock_guard<std::mutex> guard( trackerMutex() );

        WP_UNUSED( lockName );

        auto ownerIt = exclusiveOwners().find( lock );
        check( ownerIt != exclusiveOwners().end() && ownerIt->second == std::this_thread::get_id(),
               "Exclusive lock released by a thread that does not own it" );

        if( ownerIt != exclusiveOwners().end() )
        {
            exclusiveOwners().erase( ownerIt );
        }
    }

    inline void assertCanAcquireShared( const void *lock, const char *lockName )
    {
        std::lock_guard<std::mutex> guard( trackerMutex() );

        WP_UNUSED( lockName );

        const auto currentThread = std::this_thread::get_id();
        auto ownerIt = exclusiveOwners().find( lock );
        check( ownerIt == exclusiveOwners().end() || ownerIt->second != currentThread,
               "Thread attempted to acquire a shared lock while holding the exclusive lock" );
    }

    inline void markSharedAcquired( const void *lock, const char *lockName )
    {
        std::lock_guard<std::mutex> guard( trackerMutex() );

        WP_UNUSED( lockName );

        auto ownerIt = exclusiveOwners().find( lock );
        check( ownerIt == exclusiveOwners().end() || ownerIt->second == std::thread::id{},
               "Shared lock acquired while an exclusive writer is active" );

        ++sharedTotals()[lock];
        ++currentThreadSharedLocks()[lock];
    }

    inline void markSharedReleased( const void *lock, const char *lockName )
    {
        std::lock_guard<std::mutex> guard( trackerMutex() );

        WP_UNUSED( lockName );

        auto &threadLocks = currentThreadSharedLocks();
        auto threadIt = threadLocks.find( lock );
        check( threadIt != threadLocks.end() && threadIt->second > 0,
               "Shared lock released by a thread that does not hold it" );

        if( threadIt != threadLocks.end() && --threadIt->second == 0 )
        {
            threadLocks.erase( threadIt );
        }

        auto totalIt = sharedTotals().find( lock );
        check( totalIt != sharedTotals().end() && totalIt->second > 0,
               "Shared lock released with no active shared readers" );

        if( totalIt != sharedTotals().end() && --totalIt->second == 0 )
        {
            sharedTotals().erase( totalIt );
        }
        if( threadLocks.empty() )
            sharedLocksByThread().erase( std::this_thread::get_id() );
    }
}  // namespace workphone::thread_diagnostics

#endif

#endif  // WorkphoneThreadDiagnostics_h__
