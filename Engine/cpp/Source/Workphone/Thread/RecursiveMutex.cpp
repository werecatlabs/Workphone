#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <unordered_map>
#include <thread>

namespace workphone
{

    RecursiveMutex::RecursiveMutex()
    {
        m_count = 0;
        m_shared_count = 0;
        m_upgrade_mode = false;
    }

    RecursiveMutex::~RecursiveMutex()
    {
        // Ensure no locks are held at destruction time
        WP_ASSERT( m_count == 0 && m_shared_count == 0 && !m_upgrade_mode );
    }

    void RecursiveMutex::lock()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex );

        if( m_owner_thread == current_thread )
        {
            // Recursive lock by same thread
            ++m_count;
            return;
        }

        // Check if this thread holds shared locks - would cause self-deadlock
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it != m_shared_locks_per_thread.end() && it->second > 0 )
        {
            // Thread holds shared locks - use try_unlock_shared_and_lock() instead
            // or assert/throw to indicate improper usage
            WP_ASSERT(
                false &&
                "Cannot call lock() while holding shared locks - use try_unlock_shared_and_lock()" );
            return;
        }

        // Wait until no thread has exclusive access AND no shared readers AND not in upgrade mode
        m_condition.wait( lock,
                          [this] { return m_count == 0 && m_shared_count == 0 && !m_upgrade_mode; } );

        m_owner_thread = current_thread;
        m_count = 1;
    }

    bool RecursiveMutex::try_lock()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex, std::try_to_lock );

        if( !lock.owns_lock() )
        {
            return false;
        }

        if( m_owner_thread == current_thread )
        {
            // Recursive lock by same thread
            ++m_count;
            return true;
        }

        // Check if this thread holds shared locks - would cause self-deadlock with blocking lock()
        // For try_lock, we detect this as improper usage rather than silently failing
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it != m_shared_locks_per_thread.end() && it->second > 0 )
        {
            // Thread holds shared locks - use try_unlock_shared_and_lock() instead
            WP_ASSERT(
                false &&
                "Cannot call try_lock() while holding shared locks - use try_unlock_shared_and_lock()" );
            return false;
        }

        // Check if we can acquire exclusive lock
        if( m_count == 0 && m_shared_count == 0 && !m_upgrade_mode )
        {
            m_owner_thread = current_thread;
            m_count = 1;
            return true;
        }

        return false;
    }

    void RecursiveMutex::lock_shared_write()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex );

        // If this thread already has exclusive access, treat as recursive shared lock
        if( m_owner_thread == current_thread && m_count > 0 )
        {
            m_shared_locks_per_thread[current_thread]++;
            return;
        }

        // Check if this thread already holds shared locks - potential deadlock scenario
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it != m_shared_locks_per_thread.end() && it->second > 0 )
        {
            // Thread already holds shared locks - cannot safely acquire upgrade capability
            // This could deadlock if another thread is waiting to upgrade
            WP_ASSERT( false && "Cannot call lock_shared_write() while already holding shared locks" );
            return;
        }

        // Check if this thread already holds the upgrade lock
        if( m_upgrade_thread == current_thread && m_upgrade_mode )
        {
            // Already has upgrade capability, just add shared lock count
            ++m_shared_count;
            m_shared_locks_per_thread[current_thread]++;
            return;
        }

        // Wait until no thread has exclusive access and no other upgrade is in progress
        m_condition.wait( lock, [this] { return m_count == 0 && !m_upgrade_mode; } );

        // Acquire shared lock with upgrade capability
        ++m_shared_count;
        m_shared_locks_per_thread[current_thread]++;
        m_upgrade_mode = true;
        m_upgrade_thread = current_thread;
    }

    void RecursiveMutex::unlock_shared_write()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex );

        // Check per-thread shared lock tracking
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it == m_shared_locks_per_thread.end() || it->second == 0 )
        {
            // Thread doesn't hold any shared locks - this is an error condition
            WP_ASSERT( false && "unlock_shared_write() called without holding shared locks" );
            return;
        }

        // If this thread has exclusive access, handle shared unlock differently
        if( m_owner_thread == current_thread && m_count > 0 )
        {
            // Decrement per-thread shared count only
            --it->second;
            if( it->second == 0 )
            {
                m_shared_locks_per_thread.erase( it );
            }
            return;
        }

        // Verify this thread actually holds the upgrade lock
        if( m_upgrade_thread != current_thread || !m_upgrade_mode )
        {
            // Thread called unlock_shared_write() but doesn't hold upgrade capability
            // They should use unlock_shared() instead
            WP_ASSERT(
                false &&
                "unlock_shared_write() called without holding upgrade lock - use unlock_shared()" );
            return;
        }

        // Normal shared unlock
        --it->second;
        --m_shared_count;

        // Only release upgrade capability when this is the last shared lock for this thread
        if( it->second == 0 )
        {
            m_upgrade_mode = false;
            m_upgrade_thread = std::thread::id();
            m_shared_locks_per_thread.erase( it );
        }

        // Notify waiting threads
        if( m_shared_count == 0 || !m_upgrade_mode )
        {
            m_condition.notify_all();
        }
    }

    void RecursiveMutex::lock_shared()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex );

        // If this thread already has exclusive access, convert to shared semantics
        if( m_owner_thread == current_thread && m_count > 0 )
        {
            // Thread already has exclusive lock, track shared access separately
            m_shared_locks_per_thread[current_thread]++;
            return;
        }

        // If this thread holds the upgrade lock, allow recursive shared acquisition
        if( m_upgrade_thread == current_thread && m_upgrade_mode )
        {
            // Upgrade thread can acquire additional shared locks without waiting
            ++m_shared_count;
            m_shared_locks_per_thread[current_thread]++;
            return;
        }

        // Check if this thread already holds shared locks (recursive shared lock)
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it != m_shared_locks_per_thread.end() && it->second > 0 )
        {
            // Thread already has shared locks, allow recursive acquisition
            ++m_shared_count;
            ++( it->second );
            return;
        }

        // Wait until no thread has exclusive access and not in upgrade mode
        m_condition.wait( lock, [this] { return m_count == 0 && !m_upgrade_mode; } );

        // Increment shared reader count and track per-thread
        ++m_shared_count;
        m_shared_locks_per_thread[current_thread]++;
    }

    bool RecursiveMutex::try_lock_shared()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex, std::try_to_lock );

        if( !lock.owns_lock() )
        {
            return false;
        }

        // If this thread already has exclusive access, convert to shared semantics
        if( m_owner_thread == current_thread && m_count > 0 )
        {
            m_shared_locks_per_thread[current_thread]++;
            return true;
        }

        // If this thread holds the upgrade lock, allow shared acquisition
        if( m_upgrade_thread == current_thread && m_upgrade_mode )
        {
            ++m_shared_count;
            m_shared_locks_per_thread[current_thread]++;
            return true;
        }

        // Check if this thread already holds shared locks (recursive shared lock)
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it != m_shared_locks_per_thread.end() && it->second > 0 )
        {
            ++m_shared_count;
            ++( it->second );
            return true;
        }

        // Check if we can acquire shared lock (no exclusive owner, no upgrade in progress)
        if( m_count == 0 && !m_upgrade_mode )
        {
            ++m_shared_count;
            m_shared_locks_per_thread[current_thread]++;
            return true;
        }

        return false;
    }

    void RecursiveMutex::unlock_shared()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex );

        // Check per-thread shared lock tracking
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it == m_shared_locks_per_thread.end() || it->second == 0 )
        {
            // Thread doesn't hold any shared locks - this is an error condition
            WP_ASSERT( false && "unlock_shared() called without holding shared locks" );
            return;
        }

        // If this thread has exclusive access, handle shared unlock differently
        if( m_owner_thread == current_thread && m_count > 0 )
        {
            // Decrement per-thread shared count only (not global m_shared_count)
            --it->second;
            if( it->second == 0 )
            {
                m_shared_locks_per_thread.erase( it );
            }
            return;
        }

        // Normal shared unlock
        --it->second;
        --m_shared_count;

        // Track if we need to notify and why
        bool should_notify = false;

        // Clean up per-thread tracking if no more shared locks
        if( it->second == 0 )
        {
            // If this thread held the upgrade lock, release it
            if( m_upgrade_thread == current_thread && m_upgrade_mode )
            {
                m_upgrade_mode = false;
                m_upgrade_thread = std::thread::id();
                should_notify = true;
            }
            m_shared_locks_per_thread.erase( it );
        }

        // Notify waiting threads if this was the last shared reader or upgrade was released
        if( m_shared_count == 0 || should_notify )
        {
            m_condition.notify_all();
        }
    }

    void RecursiveMutex::unlock()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex );

        // Verify the calling thread owns the exclusive lock
        if( current_thread != m_owner_thread )
        {
            WP_ASSERT( false && "unlock() called by thread that doesn't own the exclusive lock" );
            return;
        }

        // Verify count is positive (prevent underflow)
        if( m_count <= 0 )
        {
            WP_ASSERT( false && "unlock() called but exclusive lock count is already zero" );
            return;
        }

        --m_count;

        if( m_count == 0 )
        {
            m_owner_thread = std::thread::id();

            // Note: Any shared locks acquired while holding exclusive remain valid
            // in m_shared_locks_per_thread but don't contribute to m_shared_count,
            // so they won't block new exclusive lockers. This is by design - the
            // caller should ensure balanced lock_shared/unlock_shared calls.

            // Notify all waiting threads (both exclusive and shared)
            m_condition.notify_all();
        }
    }

    bool RecursiveMutex::try_lock_upgrade()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex, std::try_to_lock );

        if( !lock.owns_lock() )
        {
            return false;
        }

        // If this thread already holds exclusive lock, upgrade is meaningless but not an error
        if( m_owner_thread == current_thread && m_count > 0 )
        {
            // Already have exclusive access - no upgrade needed
            return true;
        }

        // If this thread already holds the upgrade lock, return success (idempotent)
        if( m_upgrade_thread == current_thread && m_upgrade_mode )
        {
            return true;
        }

        // Must hold at least one shared lock to upgrade
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it == m_shared_locks_per_thread.end() || it->second == 0 )
        {
            // Thread doesn't hold shared locks - cannot upgrade
            WP_ASSERT( false && "try_lock_upgrade() requires holding a shared lock first" );
            return false;
        }

        // Check if upgrade is possible (no exclusive lock, no other upgrade in progress)
        if( m_count == 0 && !m_upgrade_mode )
        {
            m_upgrade_mode = true;
            m_upgrade_thread = current_thread;
            return true;
        }

        // Another thread holds exclusive or upgrade lock
        return false;
    }

    void RecursiveMutex::unlock_upgrade()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex );

        // If this thread holds exclusive lock, upgrade unlock is a no-op
        // (matches try_lock_upgrade() returning true for exclusive owners)
        if( m_owner_thread == current_thread && m_count > 0 )
        {
            // Thread has exclusive access - no upgrade state to release
            return;
        }

        // Verify this thread actually holds the upgrade lock
        if( m_upgrade_thread != current_thread || !m_upgrade_mode )
        {
            WP_ASSERT( false && "unlock_upgrade() called by thread that doesn't hold the upgrade lock" );
            return;
        }

        // Release upgrade capability
        m_upgrade_mode = false;
        m_upgrade_thread = std::thread::id();

        // Notify waiting threads that may have been blocked by upgrade mode
        m_condition.notify_all();
    }

    bool RecursiveMutex::try_unlock_shared_and_lock()
    {
        std::thread::id current_thread = std::this_thread::get_id();
        std::unique_lock<std::mutex> lock( m_mutex );

        // Must hold at least one shared lock
        auto it = m_shared_locks_per_thread.find( current_thread );
        if( it == m_shared_locks_per_thread.end() || it->second == 0 )
        {
            return false;
        }

        // Check if we can acquire exclusive lock (only this thread holds shared locks)
        if( m_count == 0 && m_shared_count == it->second && !m_upgrade_mode )
        {
            // Convert all our shared locks to one exclusive lock
            m_shared_count -= it->second;
            m_shared_locks_per_thread.erase( it );

            m_owner_thread = current_thread;
            m_count = 1;
            return true;
        }

        return false;
    }

    RecursiveMutex::ScopedLock::ScopedLock( RecursiveMutex &mutex ) : m_mutex( mutex )
    {
        m_mutex.lock();
    }

    RecursiveMutex::ScopedLock::~ScopedLock()
    {
        m_mutex.unlock();
    }

    RecursiveMutex::ScopedLock::operator bool()
    {
        return true;
    }

    RecursiveMutex::ScopedSharedLock::ScopedSharedLock( RecursiveMutex &mutex ) : m_mutex( mutex )
    {
        m_mutex.lock_shared();
    }

    RecursiveMutex::ScopedSharedLock::~ScopedSharedLock()
    {
        m_mutex.unlock_shared();
    }

    RecursiveMutex::ScopedSharedLock::operator bool()
    {
        return true;
    }

}  // namespace workphone
