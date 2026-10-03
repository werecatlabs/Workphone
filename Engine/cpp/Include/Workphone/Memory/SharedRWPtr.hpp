#ifndef SharedRWPtr_h__
#define SharedRWPtr_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>

namespace workphone
{

    /**
     * @brief Thread-safe container that combines an atomic shared pointer with a read-write spin mutex.
     *
     * SharedRWPtr<T> holds an atomic reference to a shared object of type T and provides explicit
     * read/write locking primitives for coordinated access to the pointed object.
     *
     * Typical usage:
     * - Use lock() / unlock() to obtain an exclusive (write) lock when modifying the object or
     *   replacing the stored pointer.
     * - Use lockRead() / unlockRead() to obtain a shared (read) lock for concurrent readers.
     * - Use operator->() to obtain a strong SharedPtr<T> snapshot of the current pointer. The
     *   returned SharedPtr keeps the object alive even after releasing locks.
     *
     * Notes:
     * - The mutex is declared mutable to allow const accessors (for example the const operator->())
     *   to acquire read locks when needed.
     * - The class itself does not manage object lifetime beyond the semantics of the contained
     *   AtomicSharedPtr<T> and SharedPtr<T>.
     *
     * @tparam T Type of the managed object.
     */
    template <typename T>
    class SharedRWPtr
    {
    public:
        /**
         * @brief Default-constructs an empty SharedRWPtr.
         *
         * The contained atomic pointer is default-initialized (typically null).
         */
        SharedRWPtr();

        /**
         * @brief Default destructor.
         *
         * Destroys the contained atomic pointer and mutex. Does not block — callers should ensure
         * no concurrent access occurs during destruction.
         */
        ~SharedRWPtr();

        /**
         * @brief Acquire the exclusive (write) lock.
         *
         * Blocks until the write lock is obtained. Use this before modifying the underlying
         * object or replacing the stored pointer to ensure exclusive access.
         */
        void lock();

        /**
         * @brief Release the exclusive (write) lock.
         *
         * Call after a corresponding lock() when exclusive access is no longer required.
         */
        void unlock();

        /**
         * @brief Acquire the shared (read) lock.
         *
         * Multiple readers may hold the read lock concurrently. Use this to protect read-only
         * access to the underlying object while writers are excluded.
         */
        void lockRead();

        /**
         * @brief Release the shared (read) lock.
         *
         * Call after a corresponding lockRead() when read access is no longer required.
         */
        void unlockRead();

        /**
         * @brief Obtain a strong SharedPtr<T> snapshot of the current atomic pointer.
         *
         * The returned SharedPtr<T> increments the reference count of the underlying object
         * (if non-null) and thus keeps it alive even after the SharedRWPtr is unlocked or
         * the stored atomic pointer is changed by another thread.
         *
         * This operation is safe to call without holding the external mutex because it uses
         * the atomic pointer's load() to obtain a consistent snapshot.
         *
         * @return SharedPtr<T> Strong reference to the currently stored object (may be null).
         */
        SharedPtr<T> operator->();

        /**
         * @brief Const overload of operator-> returning a strong SharedPtr<T>.
         *
         * Allows const call-sites to obtain a snapshot of the current pointer. The operation
         * uses the atomic pointer's load() and does not require external locking, although
         * callers may still choose to hold a read lock for additional synchronization guarantees.
         *
         * @return const SharedPtr<T> Strong reference to the currently stored object (may be null).
         */
        const SharedPtr<T> operator->() const;

        /** @brief Read/write spin mutex protecting coordinated access. Mutable to allow locking from
         * const contexts. */
        mutable SpinRWMutex mutex;

        /** @brief Atomic wrapper around a SharedPtr<T> for lock-free snapshot loads/stores. */
        AtomicSharedPtr<T> ptr;
    };

    template <typename T>
    SharedRWPtr<T>::SharedRWPtr() = default;

    template <typename T>
    SharedRWPtr<T>::~SharedRWPtr() = default;

    // Out-of-class template definitions
    template <typename T>
    void SharedRWPtr<T>::lock()
    {
        mutex.lock();
    }

    template <typename T>
    void SharedRWPtr<T>::unlock()
    {
        mutex.unlock();
    }

    template <typename T>
    void SharedRWPtr<T>::lockRead()
    {
        mutex.lock_shared();
    }

    template <typename T>
    void SharedRWPtr<T>::unlockRead()
    {
        mutex.unlock_shared();
    }

    template <typename T>
    SharedPtr<T> SharedRWPtr<T>::operator->()
    {
        return ptr.load();
    }

    template <typename T>
    const SharedPtr<T> SharedRWPtr<T>::operator->() const
    {
        return ptr.load();
    }

}  // namespace workphone

#endif  // SharedRWPtr_h__
