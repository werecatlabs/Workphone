#ifndef __WP_AtomicSharedPtr_h__
#define __WP_AtomicSharedPtr_h__

#include <Workphone/Memory/SharedPtr.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>

namespace workphone
{

    /**
     * @brief A thread-safe wrapper around SharedPtr<T> providing atomic store, load, and exchange
     * operations.
     *
     * AtomicSharedPtr<T> protects a SharedPtr<T> with an internal SpinRWMutex so that concurrent
     * reads and writes to the shared pointer are safe across threads. Unlike AtomicSmartPtr<T>,
     * which relies on intrusive reference counting and a single atomic raw pointer, AtomicSharedPtr<T>
     * must protect both the typed pointer and the associated control block simultaneously; a spin
     * read-write lock is therefore used in place of a single compare-and-swap instruction.
     *
     * - Multiple concurrent readers are permitted (load, get, operator->, operator bool, operator!).
     * - Writers (store, exchange, operator=) acquire an exclusive write lock.
     * - is_lock_free() always returns false for this implementation.
     *
     * @tparam T The type of the element managed by the shared pointer.
     */
    template <class T>
    class AtomicSharedPtr
    {
    public:
        using this_type = AtomicSharedPtr<T>;  ///< Type of this atomic shared pointer.
        using element_type = T;                ///< Type of the managed object.
        using value_type = T;                  ///< Value type of the managed object.
        using pointer = T *;                   ///< Pointer to the managed object.

        /**
         * @brief Default-constructs an empty AtomicSharedPtr.
         *
         * The stored SharedPtr is null-initialized.
         */
        AtomicSharedPtr();

        /**
         * @brief Copy-constructs from another AtomicSharedPtr<T>.
         *
         * Performs a thread-safe load on `other` and stores the result.
         *
         * @param other Source atomic shared pointer.
         */
        AtomicSharedPtr( const AtomicSharedPtr<T> &other );

        /**
         * @brief Constructs from a SharedPtr<T>.
         *
         * @param other Source shared pointer to store.
         */
        AtomicSharedPtr( const SharedPtr<T> &other );

        /**
         * @brief Destructor.
         *
         * The stored SharedPtr's reference count is decremented automatically when the
         * member is destroyed. No explicit cleanup is required.
         */
        ~AtomicSharedPtr() = default;

        /**
         * @brief Returns whether this implementation is lock-free.
         *
         * This implementation uses a SpinRWMutex and is therefore never lock-free.
         *
         * @return false.
         */
        bool is_lock_free() const;

        /**
         * @brief Thread-safely stores a new SharedPtr<T>.
         *
         * Acquires the write lock, replaces the stored pointer, then releases the lock.
         *
         * @param ptr The shared pointer to store.
         */
        void store( const SharedPtr<T> &ptr );

        /**
         * @brief Thread-safely loads and returns a copy of the stored SharedPtr<T>.
         *
         * Acquires the read lock, copies the stored pointer, then releases the lock.
         * The returned SharedPtr keeps the managed object alive independently of this
         * AtomicSharedPtr.
         *
         * @return A SharedPtr<T> snapshot of the currently stored pointer (may be null).
         */
        SharedPtr<T> load() const;

        /**
         * @brief Assigns a SharedPtr<T> to this atomic shared pointer.
         *
         * Equivalent to store(ptr). This operation is noexcept.
         *
         * @param ptr The shared pointer to assign.
         */
        void operator=( const SharedPtr<T> &ptr ) noexcept;

        /**
         * @brief Implicitly converts this atomic shared pointer to a SharedPtr<T>.
         *
         * Performs a thread-safe load and returns the result.
         *
         * @return The currently stored shared pointer.
         */
        operator SharedPtr<T>() const noexcept;

        /**
         * @brief Atomically replaces the stored pointer with ptr and returns the previous value.
         *
         * Acquires the write lock, swaps the stored pointer with ptr, releases the lock, then
         * returns the old value.
         *
         * @param ptr The new shared pointer to store.
         * @return The SharedPtr<T> that was stored before the exchange.
         */
        SharedPtr<T> exchange( const SharedPtr<T> &ptr );

        /**
         * @brief Accesses members of the managed object.
         *
         * Loads the raw pointer under a read lock. The caller must ensure that the managed
         * object remains alive for the duration of the member access.
         *
         * @return Raw pointer to the managed object.
         */
        T *operator->();

        /**
         * @brief Const overload of operator->.
         *
         * @return Const raw pointer to the managed object.
         */
        const T *operator->() const;

        /**
         * @brief Checks whether the stored pointer is null.
         *
         * @return true if the stored pointer is null, false otherwise.
         */
        bool operator!() const;

        /**
         * @brief Implicit conversion to bool.
         *
         * @return true if the stored pointer is not null, false otherwise.
         */
        operator bool() const;

        /**
         * @brief Copy-assigns from another AtomicSharedPtr<T>.
         *
         * Performs a thread-safe load on `other` and stores the result into this object.
         *
         * @param other Source atomic shared pointer.
         * @return Reference to this.
         */
        AtomicSharedPtr &operator=( const AtomicSharedPtr<T> &other );

        /**
         * @brief Returns the raw pointer held by the stored SharedPtr<T>.
         *
         * Acquires a read lock to obtain the raw pointer. May be null.
         *
         * @return Raw pointer to the managed object, or nullptr if empty.
         */
        T *get() const;

    protected:
        mutable SpinRWMutex m_mutex;  ///< Spin read-write mutex protecting m_ptr.
        SharedPtr<T> m_ptr;           ///< The stored shared pointer.
    };

    template <class T>
    AtomicSharedPtr<T>::AtomicSharedPtr()
    {
    }

    template <class T>
    AtomicSharedPtr<T>::AtomicSharedPtr( const AtomicSharedPtr<T> &other )
    {
        store( other.load() );
    }

    template <class T>
    AtomicSharedPtr<T>::AtomicSharedPtr( const SharedPtr<T> &other )
    {
        store( other );
    }

    template <class T>
    bool AtomicSharedPtr<T>::is_lock_free() const
    {
        return false;
    }

    template <class T>
    void AtomicSharedPtr<T>::store( const SharedPtr<T> &ptr )
    {
        SpinRWMutex::ScopedLock lock( m_mutex, true );
        m_ptr = ptr;
    }

    template <class T>
    SharedPtr<T> AtomicSharedPtr<T>::load() const
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return m_ptr;
    }

    template <class T>
    void AtomicSharedPtr<T>::operator=( const SharedPtr<T> &ptr ) noexcept
    {
        store( ptr );
    }

    template <class T>
    AtomicSharedPtr<T>::operator SharedPtr<T>() const noexcept
    {
        return load();
    }

    template <class T>
    SharedPtr<T> AtomicSharedPtr<T>::exchange( const SharedPtr<T> &ptr )
    {
        SpinRWMutex::ScopedLock lock( m_mutex, true );
        SharedPtr<T> old = m_ptr;
        m_ptr = ptr;
        return old;
    }

    template <class T>
    WPForceInline T *AtomicSharedPtr<T>::operator->()
    {
#if WP_ENABLE_PTR_EXCEPTIONS
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        T *ptr = m_ptr.get();
        WP_ASSERT( ptr );

        if( !ptr )
        {
            throw Exception( "Null pointer exception." );
        }

        return ptr;
#else
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        WP_ASSERT( m_ptr.get() );
        return m_ptr.get();
#endif
    }

    template <class T>
    WPForceInline const T *AtomicSharedPtr<T>::operator->() const
    {
#if WP_ENABLE_PTR_EXCEPTIONS
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        T *ptr = m_ptr.get();
        WP_ASSERT( ptr );

        if( !ptr )
        {
            throw Exception( "Null pointer exception." );
        }

        return ptr;
#else
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        WP_ASSERT( m_ptr.get() );
        return m_ptr.get();
#endif
    }

    template <class T>
    WPForceInline bool AtomicSharedPtr<T>::operator!() const
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return !m_ptr;
    }

    template <class T>
    WPForceInline AtomicSharedPtr<T>::operator bool() const
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return static_cast<bool>( m_ptr );
    }

    template <class T>
    AtomicSharedPtr<T> &AtomicSharedPtr<T>::operator=( const AtomicSharedPtr<T> &other )
    {
        if( this != &other )
        {
            store( other.load() );
        }
        return *this;
    }

    template <class T>
    T *AtomicSharedPtr<T>::get() const
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return m_ptr.get();
    }

}  // namespace workphone

#endif  // __WP_AtomicSharedPtr_h__
