#ifndef FBSafePtr_h__
#define FBSafePtr_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <type_traits>
#include <atomic>

namespace workphone
{

    /**
     * @brief A safe pointer class.
     *
     * This class manages a pointer to an object of type T. It provides
     * automatic memory management, and automatically deletes the object
     * when it is no longer needed.
     *
     * @tparam T The type of object that the smart pointer will manage.
     */
    template <class T>
    class SafePtr
    {
    public:
        using this_type = SafePtr<T>;  // Type of this smart pointer
        using element_type = T;        // Type of the managed object
        using value_type = T;          // Type of the managed object
        using pointer = T *;           // Type of the pointer to the managed object

        /**
         * @brief Default constructor. Creates a null pointer.
         */
        SafePtr();

        /**
         * @brief Constructor that creates a smart pointer from the raw pointer passed in.
         *
         * @param object The raw pointer to the object.
         */
        SafePtr( T *other );

        /**
         * @brief Constructor that creates a smart pointer from the copy passed in.
         *
         * @param other The other smart pointer object.
         */
        SafePtr( const SafePtr &other );
        SafePtr( const SafePtr &&other );

        /**
         * @brief Constructor that creates a raw pointer from the copy passed in.
         *
         * @param other The other raw pointer object.
         */
        SafePtr( SmartPtr<T> other );

        /**
         * @brief Constructor that creates a raw pointer from the copy passed in.
         *
         * @param other The other raw pointer object.
         */
        SafePtr( RawPtr<T> other );

        /**
         * @brief Constructor that creates a smart pointer from another type. Performs a static cast.
         *
         * @tparam B The type of object that the other smart pointer is managing.
         * @param other The other smart pointer object.
         */
        template <typename B>
        SafePtr( B *other );

        /**
         * @brief Constructor that creates a smart pointer from another type. Performs a static cast.
         *
         * @tparam B The type of object that the other smart pointer is managing.
         * @param other The other smart pointer object.
         */
        template <typename B>
        SafePtr( const SafePtr<B> &other );

        /**
         * @brief Non-virtual destructor.
         */
        ~SafePtr();

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

        // assignment

        /**
         * @brief Assigns a raw pointer to the smart pointer.
         *
         * @param other The raw pointer to assign.
         * @return A reference to the smart pointer.
         */
        SafePtr &operator=( T *other ) throw();

        /**
         * Copy-assignment operator.
         * Assigns a new object to the smart pointer, decrementing the reference count of the old object
         * and incrementing the reference count of the new object.
         *
         * @param other The smart pointer to assign to this smart pointer.
         * @returns A reference to this smart pointer.
         * @throw No-throw guarantee.
         */
        SafePtr &operator=( const SafePtr &other ) throw();

        template <typename B>
        SafePtr &operator=( const SafePtr<B> &other ) throw();

        // comparisons

        /**
         * Compares the stored pointer with a raw pointer for equality.
         *
         * @param other A raw pointer to compare with the stored pointer.
         * @return true if the stored pointer is equal to the raw pointer, false otherwise.
         */
        bool operator==( T *other ) const;

        /**
         * Compares the underlying pointer to a raw pointer for inequality.
         *
         * @param other A raw pointer to compare with.
         *
         * @return True if the underlying pointer is not equal to the given raw pointer, false otherwise.
         */
        bool operator!=( T *other ) const;

        /**
         * Compares this smart pointer with another for equality.
         * @param other The smart pointer to compare with.
         * @return True if the two smart pointers point to the same object, false otherwise.
         */
        bool operator==( const SafePtr &other ) const;

        /**
         * Comparison operator. Checks if this smart pointer is not equal to another one.
         *
         * @param other Another smart pointer to compare to.
         * @return true if this pointer is not equal to the other pointer, false otherwise.
         */
        bool operator!=( const SafePtr &other ) const;

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
        bool operator==( const SafePtr<B> &other ) const;

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

    protected:
        /**
         * A raw pointer to the object held by the smart pointer.
         *
         * This member variable holds a raw pointer to the object that is managed by the smart pointer.
         * If the smart pointer is null, this member variable is also null. When the smart pointer is
         * destroyed, this member variable is not automatically deleted or reset, and it is the user's
         * responsibility to manage the lifetime of the pointed object appropriately.
         */
        std::atomic<T *> m_pointer = nullptr;
    };

    template <class T>
    SafePtr<T>::SafePtr() = default;

    template <class T>
    SafePtr<T>::SafePtr( T *other )
    {
        if( other )
        {
            if( other->isAlive() )
            {
                m_pointer = other;
                other->addReference();
                other->lock();
            }
        }
    }

    template <class T>
    SafePtr<T>::SafePtr( const SafePtr &other )
    {
        if( auto p = other.get() )
        {
            if( p->isAlive() )
            {
                m_pointer = p;
                p->addReference();
                p->lock();
            }
        }
    }

    template <class T>
    SafePtr<T>::SafePtr( const SafePtr &&other )
    {
        if( auto p = other.get() )
        {
            m_pointer = p;
            p->addReference();
            p->lock();
        }
    }

    template <class T>
    SafePtr<T>::SafePtr( SmartPtr<T> other )
    {
        *this = other.get();
    }

    template <class T>
    SafePtr<T>::SafePtr( RawPtr<T> other )
    {
        *this = other.get();
    }

    template <class T>
    template <typename B>
    SafePtr<T>::SafePtr( B *other )
    {
        auto object = (T *)other;

        if( object )
        {
            WP_ASSERT( dynamic_cast<T *>( object ) );

            if( object->isAlive() )
            {
                object->addReference();
                object->lock();
                m_pointer = object;
            }
        }
    }

    template <class T>
    template <typename B>
    SafePtr<T>::SafePtr( const SafePtr<B> &other )
    {
        if( auto pointer = other.get() )
        {
            WP_ASSERT( dynamic_cast<T *>( pointer ) );
            auto object = (T *)pointer;
            if( object->isAlive() )
            {
                object->addReference();
                object->lock();
                m_pointer = object;
            }
        }
    }

    template <class T>
    SafePtr<T>::~SafePtr()
    {
        if( auto p = get() )
        {
            p->unlock();

#if WP_TRACK_REFERENCES
            p->removeReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
            p->removeReference();
#endif

            m_pointer = nullptr;
        }
    }

    template <class T>
    WPForceInline T &SafePtr<T>::operator*() const
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
    WPForceInline T *SafePtr<T>::operator->()
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
    WPForceInline const T *SafePtr<T>::operator->() const
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
    WPForceInline bool SafePtr<T>::operator!() const
    {
        return m_pointer == nullptr;
    }

    template <class T>
    WPForceInline SafePtr<T>::operator bool() const
    {
        return m_pointer != nullptr;
    }

    template <class T>
    SafePtr<T> &SafePtr<T>::operator=( T *other ) throw()
    {
        if( m_pointer != other )
        {
            T *pNewObject = nullptr;
            if( other )
            {
#if WP_TRACK_REFERENCES
                other->addReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
                other->addReference();
#endif

                other->lock();
                pNewObject = other;
            }

            if( auto pObject = get() )
            {
                pObject->unlock();

#if WP_TRACK_REFERENCES
                pObject->removeReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
                pObject->removeReference();
#endif
            }

            m_pointer = pNewObject;
        }

        return *this;
    }

    template <class T>
    SafePtr<T> &SafePtr<T>::operator=( const SafePtr &other ) throw()
    {
        T *pNewObject = nullptr;
        if( auto otherObject = other.get() )
        {
#if WP_TRACK_REFERENCES
            // If tracking is enabled, add a reference to the new object and remove a reference from
            otherObject->addReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
            // If tracking is not enabled, only increment the reference count of the new object.
            otherObject->addReference();
#endif

            otherObject->lock();
            pNewObject = otherObject;
        }

        if( auto pObject = get() )
        {
            pObject->unlock();

#if WP_TRACK_REFERENCES
            // If tracking is enabled, remove the reference from the old object.
            pObject->removeReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
            // If tracking is not enabled, only decrement the reference count of the old object.
            pObject->removeReference();
#endif
        }

        // Set the new object as the managed object of this smart pointer.
        m_pointer = pNewObject;

        return *this;
    }

    template <class T>
    template <typename B>
    SafePtr<T> &SafePtr<T>::operator=( const SafePtr<B> &other ) throw()
    {
        if( auto pointer = other.get() )
        {
            WP_ASSERT( dynamic_cast<T *>( pointer ) );
            auto object = (T *)pointer;
            if( object->isAlive() )
            {
                object->addReference();
                object->lock();
                m_pointer = object;
            }
        }

        return *this;
    }

    template <class T>
    bool SafePtr<T>::operator==( T *other ) const
    {
        return m_pointer == other;
    }

    template <class T>
    bool SafePtr<T>::operator!=( T *other ) const
    {
        return m_pointer != other;
    }

    template <class T>
    bool SafePtr<T>::operator==( const SafePtr &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    bool SafePtr<T>::operator!=( const SafePtr &other ) const
    {
        return m_pointer != other.get();
    }

    template <class T>
    bool SafePtr<T>::operator==( const RawPtr<T> &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    template <class B>
    bool SafePtr<T>::operator==( const RawPtr<B> &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    template <typename B>
    bool SafePtr<T>::operator==( const SafePtr<B> &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    T *SafePtr<T>::get() const
    {
        return m_pointer;
    }

    /**
     * Returns the raw pointer held by the given smart pointer.
     *
     * This function takes a smart pointer of type FBSafePtr<T>, and returns the raw pointer to the
     * object it holds. This is equivalent to calling the `get()` method of the smart pointer object. The
     * purpose of this function is to provide a convenient way to pass the raw pointer to functions that
     * require it, while still allowing the smart pointer to manage the lifetime of the pointed object.
     *
     * @tparam T The type of the pointed object.
     * @param pointer The smart pointer to extract the raw pointer from.
     * @return The raw pointer held by the smart pointer.
     */
    template <class T>
    T *get_pointer( const SafePtr<T> &pointer )
    {
        return pointer.get();
    }

}  // namespace workphone

#endif  // FBSafePtr_h__
