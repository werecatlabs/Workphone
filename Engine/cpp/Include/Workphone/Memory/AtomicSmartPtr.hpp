#ifndef __WP_AtomicSmartPtr_h__
#define __WP_AtomicSmartPtr_h__

#include <Workphone/Memory/SmartPtr.hpp>
#include <atomic>

namespace workphone
{

    /**
     * An atomic smart pointer that provides atomic store, load, and exchange
     * operations using std::atomic<T*> instead of spin locks.
     *
     * @tparam T The type of the element pointed to by this smart pointer.
     */
    template <class T>
    class AtomicSmartPtr
    {
    public:
        using this_type = AtomicSmartPtr<T>;  ///< The type of this smart pointer.
        using element_type = T;               ///< The type of the pointed-to element.
        using value_type = T;                 ///< The value type of the pointed-to element.
        using pointer = T *;                  ///< The type of the pointer to the pointed-to element.

        /**
         * Constructs an atomic smart pointer.
         */
        AtomicSmartPtr();

        AtomicSmartPtr( const AtomicSmartPtr<T> &other );

        AtomicSmartPtr( const SmartPtr<T> &other );

        /** Destroys the atomic smart pointer. */
        ~AtomicSmartPtr();

        /**
         * Returns true if the object is lock-free.
         *
         * @return True if the object is lock-free.
         */
        bool is_lock_free() const;

        /**
         * Atomically stores the given smart pointer into the atomic smart pointer.
         *
         * @param ptr The smart pointer to store.
         */
        void store( const SmartPtr<T> ptr );

        /**
         * Atomically loads the smart pointer stored in this atomic smart pointer.
         *
         * @return The smart pointer stored in this atomic smart pointer.
         */
        SmartPtr<T> load() const;

        /**
         * Atomically assigns the given smart pointer to this atomic smart pointer.
         *
         * @param ptr The smart pointer to assign.
         */
        void operator=( const SmartPtr<T> ptr ) noexcept;

        /**
         * Converts this atomic smart pointer to a smart pointer.
         *
         * @return The smart pointer that this atomic smart pointer holds.
         */
        operator SmartPtr<T>() const noexcept;

        /**
         * Atomically exchanges the given smart pointer with the smart pointer stored in this atomic
         * smart pointer.
         *
         * @param ptr The smart pointer to exchange.
         *
         * @return The smart pointer that was stored in this atomic smart pointer before the exchange.
         */
        SmartPtr<T> exchange( const SmartPtr<T> ptr );

        /**
         * @brief Accesses the members of the object that the smart pointer is managing.
         *
         * @return A pointer to the object that the smart pointer is managing.
         */
        T *operator->();

        /**
         * @brief Accesses the members of the object that the smart pointer is managing.
         *
         * @return A pointer to the object that the smart pointer is managing.
         */
        const T *operator->() const;

        /**
         * @brief Checks whether the smart pointer is null.
         *
         * @return `true` if the smart pointer is null, `false` otherwise.
         */
        bool operator!() const;

        /**
         * @brief Implicitly converts the smart pointer to a `bool` value.
         *
         * @return `true` if the smart pointer is not null, `false` otherwise.
         */
        operator bool() const;

        /**
         * @brief Copy-assign from another AtomicSmartPtr.
         *
         * The assignment is implemented via store(other.load()) to ensure a
         * thread-safe copy of the other's SmartPtr.
         */
        AtomicSmartPtr &operator=( const AtomicSmartPtr &other );

        /** @brief Equality comparison between two SharedPtr instances. */
        bool operator==( const SmartPtr<T> &other ) const;

        /** @brief Inequality comparison between two SharedPtr instances. */
        bool operator!=( const SmartPtr<T> &other ) const;

        /** @brief Equality comparison with a raw pointer. */
        bool operator==( T *other ) const;

        /** @brief Inequality comparison with a raw pointer. */
        bool operator!=( T *other ) const;

        /** @brief Equality comparison with nullptr. */
        bool operator==( std::nullptr_t ) const noexcept;

        /** @brief Inequality comparison with nullptr. */
        bool operator!=( std::nullptr_t ) const noexcept;

        T *get() const;

    protected:
        std::atomic<T *> m_pointer;
    };

    template <class T>
    AtomicSmartPtr<T>::AtomicSmartPtr() : m_pointer( nullptr )
    {
    }

    template <class T>
    AtomicSmartPtr<T>::AtomicSmartPtr( const AtomicSmartPtr<T> &other ) : m_pointer( nullptr )
    {
        auto p = other.load();
        store( p );
    }

    template <class T>
    AtomicSmartPtr<T>::AtomicSmartPtr( const SmartPtr<T> &other ) : m_pointer( nullptr )
    {
        store( other );
    }

    template <class T>
    AtomicSmartPtr<T>::~AtomicSmartPtr()
    {
        // Release the reference on the currently stored pointer
        if( auto currentPtr = m_pointer.exchange( nullptr ) )
        {
            WP_ASSERT( currentPtr->isAlive() );
            currentPtr->removeReference();
        }
    }

    template <class T>
    bool AtomicSmartPtr<T>::is_lock_free() const
    {
        return m_pointer.is_lock_free();
    }

    template <class T>
    void AtomicSmartPtr<T>::store( const SmartPtr<T> ptr )
    {
        T *newPtr = ptr.get();

        // Add reference to the new pointer if it's not null
        if( newPtr )
        {
            WP_ASSERT( newPtr->isAlive() );
            newPtr->addReference();
        }

        // Atomically exchange the pointer
        T *oldPtr = m_pointer.exchange( newPtr );

        // Remove reference from the old pointer if it's not null
        if( oldPtr )
        {
            WP_ASSERT( oldPtr->isAlive() );
            oldPtr->removeReference();
        }
    }

    template <class T>
    SmartPtr<T> AtomicSmartPtr<T>::load() const
    {
        T *ptr = m_pointer.load();
        return SmartPtr<T>( ptr );
    }

    template <class T>
    void AtomicSmartPtr<T>::operator=( const SmartPtr<T> ptr ) noexcept
    {
        store( ptr );
    }

    template <class T>
    AtomicSmartPtr<T>::operator SmartPtr<T>() const noexcept
    {
        return load();
    }

    template <class T>
    SmartPtr<T> AtomicSmartPtr<T>::exchange( const SmartPtr<T> ptr )
    {
        T *newPtr = ptr.get();

        // Add reference to the new pointer if it's not null
        if( newPtr && newPtr->isAlive() )
        {
            newPtr->addReference();
        }
        WP_ASSERT( !newPtr || newPtr->isAlive() );

        // Atomically exchange the pointer
        T *oldPtr = m_pointer.exchange( newPtr );
        WP_ASSERT( !oldPtr || oldPtr->isAlive() );

        // Create SmartPtr from old pointer.
        // The SmartPtr constructor will add a reference, and we need to remove our
        // atomic storage reference afterwards.
        SmartPtr<T> result( oldPtr );
        if( oldPtr )
        {
            // Remove the reference that was held by the atomic storage
            // (SmartPtr constructor added its own reference)
            oldPtr->removeReference();
        }

        return result;
    }

    template <class T>
    WPForceInline T *AtomicSmartPtr<T>::operator->()
    {
#if WP_ENABLE_PTR_EXCEPTIONS
        T *ptr = m_pointer.load();
        WP_ASSERT( ptr );
        WP_ASSERT( !ptr || ptr->isAlive() );

        if( !ptr )
        {
            throw Exception( "Null pointer exception." );
        }

        return ptr;
#else
        T *ptr = m_pointer.load();

        WP_ASSERT( ptr );
        WP_ASSERT( !ptr || ptr->isAlive() );
        return ptr;
#endif
    }

    template <class T>
    WPForceInline const T *AtomicSmartPtr<T>::operator->() const
    {
#if WP_ENABLE_PTR_EXCEPTIONS
        T *ptr = m_pointer.load();
        WP_ASSERT( ptr );
        WP_ASSERT( !ptr || ptr->isAlive() );

        if( !ptr )
        {
            throw Exception( "Null pointer exception." );
        }

        return ptr;
#else
        T *ptr = m_pointer.load();

        WP_ASSERT( ptr );
        WP_ASSERT( !ptr || ptr->isAlive() );
        return ptr;
#endif
    }

    template <class T>
    WPForceInline bool AtomicSmartPtr<T>::operator!() const
    {
        T *ptr = m_pointer.load();
        return ptr == nullptr;
    }

    template <class T>
    WPForceInline AtomicSmartPtr<T>::operator bool() const
    {
        T *ptr = m_pointer.load();
        return ptr != nullptr;
    }

    template <class T>
    AtomicSmartPtr<T> &AtomicSmartPtr<T>::operator=( const AtomicSmartPtr<T> &other )
    {
        if( this != &other )
        {
            store( other.load() );
        }

        return *this;
    }

    template <class T>
    bool AtomicSmartPtr<T>::operator==( const SmartPtr<T> &other ) const
    {
        T *ptr = m_pointer.load();
        return ptr == other.get();
    }

    template <class T>
    bool AtomicSmartPtr<T>::operator!=( const SmartPtr<T> &other ) const
    {
        T *ptr = m_pointer.load();
        return ptr != other.get();
    }

    template <class T>
    bool AtomicSmartPtr<T>::operator==( T *other ) const
    {
        T *ptr = m_pointer.load();
        return ptr == other;
    }

    template <class T>
    bool AtomicSmartPtr<T>::operator!=( T *other ) const
    {
        T *ptr = m_pointer.load();
        return ptr != other;
    }

    template <class T>
    bool AtomicSmartPtr<T>::operator==( std::nullptr_t ) const noexcept
    {
        T *ptr = m_pointer.load();
        return ptr == nullptr;
    }

    template <class T>
    bool AtomicSmartPtr<T>::operator!=( std::nullptr_t ) const noexcept
    {
        T *ptr = m_pointer.load();
        return ptr != nullptr;
    }

    template <class T>
    T *AtomicSmartPtr<T>::get() const
    {
        T *ptr = m_pointer.load();
        return ptr;
    }

}  // namespace workphone

#endif  // __AtomicSmartPtr_h__
