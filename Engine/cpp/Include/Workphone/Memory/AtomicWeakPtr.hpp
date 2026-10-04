#ifndef __WP_AtomicWeakPtr_h__
#define __WP_AtomicWeakPtr_h__

#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <atomic>

namespace workphone
{

    /**
     * @brief A thread-safe, atomic wrapper around a weak pointer to an object of type `T`.
     *
     * `AtomicWeakPtr<T>` stores a raw `T*` in an `std::atomic<T*>` and updates the weak-reference
     * bookkeeping on the pointed-to object by calling `addWeakReference()` and
     * `removeWeakReference()` when the stored pointer changes or the `AtomicWeakPtr` is destroyed.
     *
     * This type provides atomic store/load/exchange semantics for the underlying pointer and
     * conversion to/from `WeakPtr<T>`. It is intended to make reading and updating a weak
     * pointer safe from multiple threads (at pointer-level atomicity).
     *
     * @tparam T Type of the referenced object.
     *
     * @note The correctness of reference-count changes relies on the `T` implementation
     *       of `addWeakReference()` / `removeWeakReference()` being thread-safe.
     * @note This class provides atomic pointer-level semantics. It does not automatically
     *       synchronize higher-level operations (for example, reading the pointer and then
     *       performing multiple operations on the pointee still require external synchronization
     *       if you need compound atomicity).
     */
    template <class T>
    class AtomicWeakPtr
    {
    public:
        using this_type = AtomicWeakPtr<T>;
        using element_type = T;
        using value_type = T;
        using pointer = T *;

        /**
         * @brief Default-construct an empty `AtomicWeakPtr`.
         *
         * The stored pointer is initialized to `nullptr`.
         */
        AtomicWeakPtr();

        /**
         * @brief Copy-construct from another `AtomicWeakPtr`.
         *
         * The value is copied by performing an atomic load on `other` and then storing
         * it into `this` using `store()`, so the weak-reference bookkeeping will be updated.
         *
         * @param other Source `AtomicWeakPtr` to copy from.
         */
        AtomicWeakPtr( const AtomicWeakPtr<T> &other );

        /**
         * @brief Construct from an existing `WeakPtr<T>`.
         *
         * The stored pointer is set to `other.get()` and the object's weak-reference count
         * will be incremented (via `addWeakReference()`).
         *
         * @param other The `WeakPtr<T>` to copy the pointer from.
         */
        AtomicWeakPtr( const WeakPtr<T> &other );

        /**
         * @brief Destructor.
         *
         * If the stored pointer is not null at destruction time, `removeWeakReference()` is
         * called on the pointee. Because the pointer is stored in an atomic, the current
         * stored pointer is obtained atomically before calling `removeWeakReference()`.
         */
        ~AtomicWeakPtr();

        /**
         * @brief Returns whether the underlying pointer storage is lock-free.
         *
         * @return `true` if the internal `std::atomic<T*>` is lock-free on this platform,
         *         otherwise `false`.
         */
        bool is_lock_free() const;

        /**
         * @brief Atomically store a `WeakPtr<T>` into this object.
         *
         * The operation will:
         *  - call `removeWeakReference()` on the previously stored pointer (if non-null),
         *  - atomically store the new raw pointer from `ptr.get()`,
         *  - call `addWeakReference()` on the newly stored pointer (if non-null).
         *
         * @param ptr The `WeakPtr<T>` whose pointer will be stored.
         *
         * @note Between the calls to `removeWeakReference()` and the final `addWeakReference()`,
         *       other threads may observe intermediate states; callers that require stronger
         *       invariants must provide external synchronization.
         */
        void store( const WeakPtr<T> &ptr );

        /**
         * @brief Atomically load the stored pointer and return it as a `WeakPtr<T>`.
         *
         * This calls the `WeakPtr<T>` constructor from a raw `T*` obtained via an atomic load.
         *
         * @return A `WeakPtr<T>` representing the currently stored pointer (may be empty).
         */
        WeakPtr<T> load() const;

        /**
         * @brief Copy-assignment from another `AtomicWeakPtr`.
         *
         * Performs `store(ptr.load())` to ensure reference bookkeeping is performed for the
         * new pointer.
         *
         * @param ptr The `AtomicWeakPtr` to copy from.
         */
        void operator=( const AtomicWeakPtr<T> &ptr ) noexcept;

        /**
         * @brief Copy-assignment from a `WeakPtr<T>`.
         *
         * Performs `store(ptr)`.
         *
         * @param ptr The `WeakPtr<T>` to copy from.
         */
        void operator=( const WeakPtr<T> &ptr ) noexcept;

        /**
         * @brief Convert to `WeakPtr<T>`.
         *
         * Performs an atomic load of the stored raw pointer and returns a `WeakPtr<T>`
         * constructed from it.
         *
         * @return A `WeakPtr<T>` representing the currently stored pointer.
         */
        operator WeakPtr<T>() const noexcept;

        /**
         * @brief Atomically replace the stored pointer with `ptr` and return the previous value.
         *
         * The operation will:
         *  - call `removeWeakReference()` on the previous pointer (if non-null),
         *  - atomically exchange the stored pointer with `ptr.get()`,
         *  - call `addWeakReference()` on the new pointer (if non-null),
         *  - return a `WeakPtr<T>` constructed from the previous raw pointer.
         *
         * @param ptr The `WeakPtr<T>` whose pointer will replace the currently stored pointer.
         * @return A `WeakPtr<T>` representing the previous stored pointer (may be empty).
         */
        WeakPtr<T> exchange( const WeakPtr<T> &ptr );

        /**
         * @brief Get the raw pointer currently stored (atomic load).
         *
         * @return The raw `T*` that is currently stored (may be nullptr).
         *
         * @note This returns the raw pointer only - it does not alter reference counts.
         */
        T *get() const;

        /**
         * @brief Forcibly zero the stored pointer without calling removeWeakReference().
         *
         * Use this only when the pointee is known to have already been destroyed (e.g.
         * during teardown when the state manager is torn down before the graphics system)
         * and calling removeWeakReference() on the dangling pointer would cause a
         * use-after-free crash.
         */
        void forceReset();

    private:
        // Pooled objects can remain readable after destruction. Readability alone
        // is insufficient: never update their counters or dispatch through their vtable.
        static bool isObjectAlive( T *ptr )
        {
            return ptr && ptr->isAlive() && ptr->getReferences() > 0;
        }

    protected:
        std::atomic<T *> m_pointer;
    };

    template <class T>
    AtomicWeakPtr<T>::AtomicWeakPtr()
    {
        m_pointer = nullptr;
    }

    template <class T>
    AtomicWeakPtr<T>::AtomicWeakPtr( const AtomicWeakPtr<T> &other )
    {
        m_pointer = nullptr;
        *this = other;
    }

    template <class T>
    AtomicWeakPtr<T>::AtomicWeakPtr( const WeakPtr<T> &other )
    {
        m_pointer = nullptr;
        *this = other;
    }

    template <class T>
    AtomicWeakPtr<T>::~AtomicWeakPtr()
    {
        T *ptr = m_pointer.exchange( nullptr );
        if( isObjectAlive( ptr ) )
            ptr->ISharedObject::removeWeakReference();
    }

    template <class T>
    bool AtomicWeakPtr<T>::is_lock_free() const
    {
        return m_pointer.is_lock_free();
    }

    template <class T>
    void AtomicWeakPtr<T>::store( const WeakPtr<T> &ptr )
    {
        T *newPtr = ptr.get();
        if( !isObjectAlive( newPtr ) )
            newPtr = nullptr;

        if( newPtr )
        {
            WP_ASSERT( newPtr->getWeakReferences() >= 0 );
            newPtr->addWeakReference();
        }

        T *oldPtr = m_pointer.exchange( newPtr );

        if( isObjectAlive( oldPtr ) && oldPtr->getWeakReferences() > 0 )
            oldPtr->ISharedObject::removeWeakReference();
    }

    template <class T>
    WeakPtr<T> AtomicWeakPtr<T>::load() const
    {
        T *ptr = m_pointer.load();
        if( !isObjectAlive( ptr ) )
            return WeakPtr<T>();
        // Safety: wrap WeakPtr construction in case object is being destroyed
        try
        {
            return WeakPtr<T>( ptr );
        }
        catch( ... )
        {
            // Object is being destroyed, return empty WeakPtr
            return WeakPtr<T>();
        }
    }

    template <class T>
    void AtomicWeakPtr<T>::operator=( const AtomicWeakPtr<T> &ptr ) noexcept
    {
        if( this != &ptr )
        {
            store( ptr.load() );
        }
    }

    template <class T>
    void AtomicWeakPtr<T>::operator=( const WeakPtr<T> &ptr ) noexcept
    {
        store( ptr );
    }

    template <class T>
    AtomicWeakPtr<T>::operator WeakPtr<T>() const noexcept
    {
        return load();
    }

    template <class T>
    WeakPtr<T> AtomicWeakPtr<T>::exchange( const WeakPtr<T> &ptr )
    {
        T *newPtr = ptr.get();
        if( !isObjectAlive( newPtr ) )
            newPtr = nullptr;

        if( newPtr )
        {
            WP_ASSERT( newPtr->getWeakReferences() >= 0 );
            newPtr->addWeakReference();
        }

        T *oldPtr = m_pointer.exchange( newPtr );
        if( !isObjectAlive( oldPtr ) )
            return WeakPtr<T>();
        WP_ASSERT( oldPtr->getWeakReferences() > 0 );

        WeakPtr<T> result( oldPtr );
        if( oldPtr )
        {
            oldPtr->ISharedObject::removeWeakReference();
        }

        return result;
    }

    template <class T>
    T *AtomicWeakPtr<T>::get() const
    {
        return m_pointer.load();
    }

    template <class T>
    void AtomicWeakPtr<T>::forceReset()
    {
        m_pointer.exchange( nullptr );
    }

}  // namespace workphone

#endif  // __WP_AtomicWeakPtr_h__
