#ifndef TryLockGuard_h__
#define TryLockGuard_h__

#include <Workphone/WorkphoneTypes.hpp>

namespace workphone
{
    /**
     * @file TryLockGuard.hpp
     * @brief RAII helper that attempts to acquire a mutex with try_lock().
     *
     * This class performs a non-blocking attempt to acquire a lock on a mutex-like
     * object in its constructor (via `try_lock()`). If the lock is acquired, the
     * guard records this and automatically calls `unlock()` in its destructor.
     *
     * Requirements:
     * - The template parameter `T` must provide the following member functions:
     *   - `bool try_lock()`  — attempts to acquire the lock, returns true on success.
     *   - `void unlock()`    — releases the lock.
     * - The mutex pointer passed to the constructor must be valid and remain valid
     *   for the lifetime of the `TryLockGuard` instance.
     *
     * Thread-safety:
     * - Distinct `TryLockGuard` instances may be used concurrently.
     * - The class itself does not provide synchronization for concurrent accesses to
     *   the same guard object; callers must ensure correct external synchronization.
     *
     * Exception safety:
     * - The class itself does not throw. Exception safety depends on `T::try_lock()`
     *   and `T::unlock()` implementations.
     *
     * @tparam T Mutex-like type implementing `try_lock()` and `unlock()`.
     */
    template <class T>
    class TryLockGuard
    {
    public:
        /**
         * @brief Construct the guard and attempt to acquire the mutex.
         *
         * The constructor calls `mutex->try_lock()` and stores the result.
         * No blocking occurs — if the mutex is already held the guard will record
         * that it did not acquire the lock.
         *
         * @param object Pointer to the mutex-like object to try to lock.
         *
         * @note `object` must remain valid for the lifetime of this guard.
         */
        TryLockGuard( T *object );

        TryLockGuard( const TryLockGuard & ) = delete;
        TryLockGuard &operator=( const TryLockGuard & ) = delete;

        /**
         * @brief Destructor releases the lock if it was acquired.
         *
         * If the constructor successfully acquired the lock (i.e. `m_locked == true`),
         * the destructor will call `m_object->unlock()`. If the lock was not acquired
         * no action is taken.
         */
        ~TryLockGuard();

        /**
         * @brief Query whether the guard successfully acquired the lock.
         * @return true if `try_lock()` succeeded in the constructor, false otherwise.
         */
        bool locked() const;

    private:
        T *m_object;    //!< Pointer to the mutex-like object (non-owning).
        bool m_locked;  //!< true if the mutex was successfully locked by this guard.
    };

    template <class T>
    TryLockGuard<T>::TryLockGuard( T *object ) : m_object( object ), m_locked( m_object->try_lock() )
    {
    }

    template <class T>
    TryLockGuard<T>::~TryLockGuard()
    {
        if( m_locked )
        {
            m_object->unlock();
        }
    }

    template <class T>
    bool TryLockGuard<T>::locked() const
    {
        return m_locked;
    }

}  // namespace workphone

#endif  // TryLockGuard_h__
