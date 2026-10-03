#ifndef __FBSmartPtr_h__
#define __FBSmartPtr_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <functional>

namespace workphone
{

    /**
     * @brief A smart pointer class.
     *
     * This class manages a pointer to an object of type T. It provides
     * automatic memory management, and automatically deletes the object
     * when it is no longer needed.
     *
     * @tparam T The type of object that the smart pointer will manage.
     */
    template <class T>
    class SmartPtr
    {
    public:
        using this_type = SmartPtr<T>;  // Type of this smart pointer
        using element_type = T;         // Type of the managed object
        using value_type = T;           // Type of the managed object
        using pointer = T *;            // Type of the pointer to the managed object

        /**
         * @brief Default constructor. Creates a null pointer.
         */
        SmartPtr();

        /**
         * @brief Constructor that creates a smart pointer from the raw pointer passed in.
         * @param object The raw pointer to the object.
         */
        SmartPtr( T *other );

        /**
         * @brief Constructor that creates a smart pointer from the copy passed in.
         * @param other The other smart pointer object.
         */
        SmartPtr( const SmartPtr &other );

        /**
         * @brief Constructor that creates a smart pointer from the move passed in.
         * @param other The other smart pointer object.
         */
        SmartPtr( const SmartPtr &&other );

        /**
         * @brief Constructor that creates a raw pointer from the copy passed in.
         *
         * @param other The other raw pointer object.
         */
        template <typename B>
        SmartPtr( RawPtr<B> other );

        /**
         * @brief Constructor that creates a smart pointer from another type. Performs a static cast.
         *
         * @tparam B The type of object that the other smart pointer is managing.
         * @param other The other smart pointer object.
         */
        template <typename B>
        SmartPtr( B *other );

        /**
         * @brief Constructor that creates a smart pointer from another type. Performs a static cast.
         *
         * @tparam B The type of object that the other smart pointer is managing.
         * @param other The other smart pointer object.
         */
        template <typename B>
        SmartPtr( const SmartPtr<B> &other );

        /**
         * @brief Non-virtual destructor.
         */
        ~SmartPtr();

        // implicit conversions

        /**
         * @brief Dereferences the smart pointer.
         *
         * @return The object that the smart pointer is managing.
         */
        T &operator*() const;

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
         * @brief Assigns a raw pointer to the smart pointer.
         *
         * @param other The raw pointer to assign.
         * @return A reference to the smart pointer.
         */
        SmartPtr &operator=( T *other ) throw();

        /**
         * Copy-assignment operator.
         * Assigns a new object to the smart pointer, decrementing the reference count of the old object
         * and incrementing the reference count of the new object.
         *
         * @param other The smart pointer to assign to this smart pointer.
         * @returns A reference to this smart pointer.
         * @throw No-throw guarantee.
         */
        SmartPtr &operator=( const SmartPtr &other ) throw();

        /**
         * @brief Copy-assignment operator for smart pointers of different types.
         * @param other The smart pointer to assign from.
         */
        template <typename B>
        SmartPtr &operator=( const SmartPtr<B> &other ) throw();

        /**
         * Compares the stored pointer with a raw pointer for equality.
         * @param other A raw pointer to compare with the stored pointer.
         * @return true if the stored pointer is equal to the raw pointer, false otherwise.
         */
        bool operator==( T *other ) const;

        /**
         * Compares the underlying pointer to a raw pointer for inequality.
         * @param other A raw pointer to compare with.
         * @return True if the underlying pointer is not equal to the given raw pointer, false otherwise.
         */
        bool operator!=( T *other ) const;

        /**
         * Compares this smart pointer with another for equality.
         * @param other The smart pointer to compare with.
         * @return True if the two smart pointers point to the same object, false otherwise.
         */
        bool operator==( const SmartPtr &other ) const;

        /**
         * Comparison operator. Checks if this smart pointer is not equal to another one.
         *
         * @param other Another smart pointer to compare to.
         * @return true if this pointer is not equal to the other pointer, false otherwise.
         */
        bool operator!=( const SmartPtr &other ) const;

        /**
         * @brief Comparison operator for a raw pointer and a smart pointer.
         *
         * Compares the internal raw pointer of the smart pointer with the raw pointer passed in.
         *
         * @param other The raw pointer to compare with the internal pointer of the smart pointer.
         *
         * @return True if the internal pointer of the smart pointer is equal to the raw pointer passed
         * in, false otherwise.
         */
        bool operator==( const RawPtr<T> &other ) const;

        /**
         * Compares this smart pointer with a raw pointer.
         *
         * @tparam B The type of the pointed object to compare with.
         * @param other A raw pointer to compare with.
         * @return True if the raw pointer is equal to the pointer held by this smart pointer, false
         * otherwise.
         *
         * This function compares the raw pointer `other` with the pointer held by this smart pointer. If
         * the raw pointer is null, the function returns false. If the pointer held by this smart pointer
         * is null, the function returns true if `other` is also null. Otherwise, the function returns
         * true if the raw pointer is equal to the pointer held by this smart pointer (i.e., if both
         * pointers point to the same object in memory), and false otherwise.
         */
        template <class B>
        bool operator==( const RawPtr<B> &other ) const;

        /**
         * @brief Overloads the equality operator to compare two smart pointers.
         *
         * This function overloads the equality operator to compare two smart pointers
         * of different types, by comparing the objects they point to.
         *
         * @tparam B The type of the object that the other smart pointer points to.
         * @param other The other smart pointer to compare to.
         * @return true if the smart pointers are equal (i.e., point to the same object),
         * false otherwise.
         */
        template <typename B>
        bool operator==( const SmartPtr<B> &other ) const;

        /**
         * Orders smart pointers by their stored pointer address.
         *
         * This gives ordered containers such as std::set a strict weak ordering.
         */
        bool operator<( const SmartPtr &other ) const;

        /**
         * Returns the raw pointer held by this smart pointer.
         *
         * @return A pointer to the object managed by this smart pointer, or a null pointer
         *         if this smart pointer is null.
         *
         * This function returns the raw pointer to the object managed by this smart pointer.
         * If this smart pointer is null, the function returns a null pointer. Note that this
         * function returns a raw pointer and does not transfer ownership of the pointer to the
         * caller.
         */
        T *get() const;

        /**
         * @brief Relinquishes this pointer without changing the object's reference count.
         *
         * This is intended for owner-controlled bulk teardown where the object storage is
         * destroyed independently of intrusive reference counting.
         *
         * @return The previously stored raw pointer.
         */
        T *detach() noexcept;

    protected:
        /**
         * A raw pointer to the object held by the smart pointer.
         *
         * This member variable holds a raw pointer to the object that is managed by the smart pointer.
         * If the smart pointer is null, this member variable is also null. When the smart pointer is
         * destroyed, this member variable is not automatically deleted or reset, and it is the user's
         * responsibility to manage the lifetime of the pointed object appropriately.
         */
        T *m_pointer = nullptr;
    };

    template <class T>
    SmartPtr<T>::SmartPtr() = default;

    template <class T>
    SmartPtr<T>::SmartPtr( T *other )
    {
#if WP_GARBAGE_COLLECTION
        if( other )
        {
            if( other->isAlive() )
            {
                m_pointer = other;
                other->addReference();
            }
        }
#else
        m_pointer = other;
#endif
    }

    template <class T>
    SmartPtr<T>::SmartPtr( const SmartPtr &other )
    {
#if WP_GARBAGE_COLLECTION
        if( auto p = other.get() )
        {
            if( p->isAlive() )
            {
                m_pointer = p;
                p->addReference();
            }
        }
#else
        m_pointer = other.get();
#endif
    }

    template <class T>
    SmartPtr<T>::SmartPtr( const SmartPtr &&other )
    {
#if WP_GARBAGE_COLLECTION
        if( auto p = other.get() )
        {
            WP_ASSERT( p->isAlive() );
            m_pointer = p;
            p->addReference();
        }
#else
        m_pointer = other.get();
#endif
    }

    template <typename T>
    template <typename B>
    SmartPtr<T>::SmartPtr( RawPtr<B> other )
    {
        *this = (B *)other.get();
    }

    template <typename T>
    template <typename B>
    SmartPtr<T>::SmartPtr( B *other )
    {
#if WP_GARBAGE_COLLECTION
        WP_ASSERT( other->template isDerived<T>() );

        if( auto p = (T *)other )
        {
            if( p->isAlive() )
            {
                p->addReference();
                m_pointer = p;
            }
        }
#else
        m_pointer = (T *)other;
#endif
    }

    template <typename T>
    template <typename B>
    SmartPtr<T>::SmartPtr( const SmartPtr<B> &other )
    {
#if WP_GARBAGE_COLLECTION
        if( auto pointer = other.get() )
        {
            WP_ASSERT( dynamic_cast<T *>( pointer ) );
            auto object = (T *)pointer;
            WP_ASSERT( object->isAlive() );
            if( object->isAlive() )
            {
                object->addReference();
                m_pointer = object;
            }
        }
#else
        m_pointer = (T *)other.get();
#endif
    }

    template <class T>
    SmartPtr<T>::~SmartPtr()
    {
#if WP_GARBAGE_COLLECTION
        if( auto p = get() )
        {
            // Only remove a reference if the object is still alive. It's possible
            // the object has already been finalized/marked dead; in that case
            // removing a reference may be unsafe and the pointer should simply
            // be dropped.
            if( p->isAlive() )
            {
#    if WP_TRACK_REFERENCES
                p->removeReference( this, __FILE__, __LINE__, __FUNCTION__ );
#    else
                p->removeReference();
#    endif
            }

            m_pointer = nullptr;
        }
#else
        m_pointer = nullptr;
#endif
    }

    template <class T>
    WPForceInline T &SmartPtr<T>::operator*() const
    {
        WP_ASSERT( m_pointer );

#if WP_ENABLE_PTR_EXCEPTIONS
        if( !m_pointer )
        {
            throw Exception( "Null pointer exception." );
        }
#endif

        return *m_pointer;
    }

    template <class T>
    WPForceInline T *SmartPtr<T>::operator->()
    {
        WP_ASSERT( m_pointer );

#if WP_ENABLE_PTR_EXCEPTIONS
        if( !m_pointer )
        {
            throw Exception( "Null pointer exception." );
        }
#endif

        return m_pointer;
    }

    template <class T>
    WPForceInline const T *SmartPtr<T>::operator->() const
    {
        WP_ASSERT( m_pointer );

#if WP_ENABLE_PTR_EXCEPTIONS
        if( !m_pointer )
        {
            throw Exception( "Null pointer exception." );
        }
#endif

        return m_pointer;
    }

    template <class T>
    WPForceInline bool SmartPtr<T>::operator!() const
    {
        return m_pointer == nullptr;
    }

    template <class T>
    WPForceInline SmartPtr<T>::operator bool() const
    {
        return m_pointer != nullptr;
    }

    template <class T>
    SmartPtr<T> &SmartPtr<T>::operator=( T *other ) throw()
    {
#if WP_GARBAGE_COLLECTION
        if( m_pointer != other )
        {
            T *pNewObject = nullptr;
            if( other )
            {
#    if WP_TRACK_REFERENCES
                other->addReference( this, __FILE__, __LINE__, __FUNCTION__ );
#    else
                other->addReference();
#    endif

                pNewObject = other;
            }

            if( auto pObject = get() )
            {
#    if WP_TRACK_REFERENCES
                pObject->removeReference( this, __FILE__, __LINE__, __FUNCTION__ );
#    else
                pObject->removeReference();
#    endif
            }

            m_pointer = pNewObject;
        }

        return *this;
#else
        m_pointer = other;
        return *this;
#endif
    }

    template <class T>
    SmartPtr<T> &SmartPtr<T>::operator=( const SmartPtr &other ) throw()
    {
#if WP_GARBAGE_COLLECTION
        T *pNewObject = nullptr;
        if( auto otherObject = other.get() )
        {
#    if WP_TRACK_REFERENCES
            // If tracking is enabled, add a reference to the new object and remove a reference from
            otherObject->addReference( this, __FILE__, __LINE__, __FUNCTION__ );
#    else
            // If tracking is not enabled, only increment the reference count of the new object.
            otherObject->addReference();
#    endif

            pNewObject = otherObject;
        }

        if( auto pObject = get() )
        {
#    if WP_TRACK_REFERENCES
            // If tracking is enabled, remove the reference from the old object.
            pObject->removeReference( this, __FILE__, __LINE__, __FUNCTION__ );
#    else
            // If tracking is not enabled, only decrement the reference count of the old object.
            pObject->removeReference();
#    endif
        }

        // Set the new object as the managed object of this smart pointer.
        m_pointer = pNewObject;

        return *this;
#else
        m_pointer = other.get();
        return *this;
#endif
    }

    template <class T>
    template <typename B>
    SmartPtr<T> &SmartPtr<T>::operator=( const SmartPtr<B> &other ) throw()
    {
#if WP_GARBAGE_COLLECTION
        T *object = nullptr;
        if( auto pointer = other.get() )
        {
            WP_ASSERT( dynamic_cast<T *>( pointer ) );
            object = (T *)pointer;
            if( object->isAlive() )
            {
                return *this = object;
            }

            object = nullptr;
        }

        return *this = object;
#else
        m_pointer = (T *)other.get();
        return *this;
#endif
    }

    template <class T>
    bool SmartPtr<T>::operator==( T *other ) const
    {
        return m_pointer == other;
    }

    template <class T>
    bool SmartPtr<T>::operator!=( T *other ) const
    {
        return m_pointer != other;
    }

    template <class T>
    bool SmartPtr<T>::operator==( const SmartPtr &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    bool SmartPtr<T>::operator!=( const SmartPtr &other ) const
    {
        return m_pointer != other.get();
    }

    template <class T>
    bool SmartPtr<T>::operator==( const RawPtr<T> &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    template <class B>
    bool SmartPtr<T>::operator==( const RawPtr<B> &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    template <typename B>
    bool SmartPtr<T>::operator==( const SmartPtr<B> &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    bool SmartPtr<T>::operator<( const SmartPtr &other ) const
    {
        return std::less<T *>()( m_pointer, other.get() );
    }

    template <class T>
    T *SmartPtr<T>::get() const
    {
        return m_pointer;
    }

    template <class T>
    T *SmartPtr<T>::detach() noexcept
    {
        auto pointer = m_pointer;
        m_pointer = nullptr;
        return pointer;
    }

    /**
     * Returns the raw pointer held by the given smart pointer.
     *
     * This function takes a smart pointer of type FBSmartPtr<T>, and returns the raw pointer to the
     * object it holds. This is equivalent to calling the `get()` method of the smart pointer object. The
     * purpose of this function is to provide a convenient way to pass the raw pointer to functions that
     * require it, while still allowing the smart pointer to manage the lifetime of the pointed object.
     *
     * @tparam T The type of the pointed object.
     * @param pointer The smart pointer to extract the raw pointer from.
     * @return The raw pointer held by the smart pointer.
     */
    template <class T>
    T *get_pointer( const SmartPtr<T> &pointer )
    {
        return pointer.get();
    }

    template <class B, class T>
    bool operator==( B *p, const SmartPtr<T> &pointer )
    {
        return p == pointer.get();
    }

}  // namespace workphone

#endif  // FBSmartPtr_h__
