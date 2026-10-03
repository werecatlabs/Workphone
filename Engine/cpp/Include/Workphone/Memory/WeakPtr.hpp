#ifndef _WP_WeakPtr_H_
#define _WP_WeakPtr_H_

#include <Workphone/Core/Exception.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <type_traits>

namespace workphone
{

    /**
     * @brief A weak pointer to a dynamically-allocated object of type T.
     *
     * This class allows access to an object of type T that is dynamically allocated and managed by
     * a smart pointer, but without owning the object. A weak pointer is useful when a dependent object
     * needs access to a target object but does not want to keep the target object alive. A weak pointer
     * can be safely converted to a smart pointer for temporary access to the target object. If the
     * target object is no longer available, the weak pointer can be checked using expired().
     *
     * @tparam T The type of the object pointed to.
     */
    template <class T>
    class WeakPtr
    {
    public:
        using this_type = WeakPtr<T>;  ///< This type.
        using element_type = T;        ///< The type of the pointed-to object.
        using value_type = T;          ///< The type of the pointed-to object.
        using pointer = T *;           ///< Pointer to the pointed-to object.

        /**
         * @brief Constructs a null weak pointer.
         */
        WeakPtr();

        /**
         * @brief Constructs a weak pointer from a raw pointer to the object.
         *
         * @param object A raw pointer to the object.
         */
        WeakPtr( T *object );

        /** Constructor creates a smart pointer from the copy passed in.
         */
        WeakPtr( const WeakPtr &other );

        /**
         * @brief Constructs a weak pointer from another weak pointer.
         *
         * @param other Another weak pointer.
         */
        WeakPtr( const SmartPtr<T> &other );

        /**
         * @brief Constructs a weak pointer from a smart pointer.
         *
         * @param other A smart pointer.
         */
        WeakPtr( const RawPtr<T> &object );

        /**
         * @brief Constructs a weak pointer from a raw pointer to another object.
         *
         * @tparam B The type of the pointed-to object.
         * @param pointer A raw pointer to the other object.
         */
        template <class B>
        WeakPtr( B *pointer );

        /**
         * @brief Constructs a weak pointer from another weak pointer to another object.
         *
         * @tparam B The type of the pointed-to object.
         * @param object Another weak pointer to the other object.
         */
        template <class B>
        WeakPtr( const WeakPtr<B> &object );

        /**
         * @brief Constructs a weak pointer from a smart pointer to another object.
         *
         * @tparam B The type of the pointed-to object.
         * @param object A smart pointer to the other object.
         */
        template <class B>
        WeakPtr( const SmartPtr<B> &object );

        /**
         * @brief Non-virtual destructor.
         */
        ~WeakPtr();

        /**
         * @brief Returns the number of shared_ptr instances that share ownership of the managed object.
         *
         * @return The number of shared_ptr instances that share ownership of the managed object.
         */
        u32 use_count() const noexcept;

        /**
         * @brief Checks whether the object pointed to by this weak pointer has expired.
         *
         * An object is expired if it has been deleted by its shared pointer or if its shared pointer has
         * been reset.
         *
         * @return `true` if the object has expired, `false` otherwise.
         */
        bool expired() const noexcept;

        /**
         * @brief Returns a shared pointer to the managed object, if it still exists.
         *
         * @return FBSmartPtr<T> A shared pointer to the managed object if it still exists,
         *                       or a null shared pointer if the managed object has been destroyed.
         */
        SmartPtr<T> lock() const noexcept;

        // implicit conversions
        T &operator*() const;
        T *operator->();
        const T *operator->() const;

        bool operator!() const;
        operator bool() const;

        // assignment
        WeakPtr &operator=( T *other ) throw();
        WeakPtr &operator=( const WeakPtr &other ) throw();
        // FBWeakPtr &operator=( const FBSmartPtr<T> &other ) throw();

        // comparisons
        bool operator==( T *other ) const;
        bool operator!=( T *other ) const;

        bool operator==( const WeakPtr &other ) const;
        bool operator!=( const WeakPtr &other ) const;

        bool operator==( const RawPtr<T> &other ) const;

        template <class B>
        bool operator==( const RawPtr<B> &other ) const;

        /**
         * Returns a raw pointer to the managed object, or nullptr if the object has been destroyed.
         * @return
         *      A raw pointer to the managed object, or nullptr if the object has been destroyed.
         */
        T *get() const;

    protected:
        /**
         * Pointer to the owned object of the weak pointer.
         * Will be null if the weak pointer is empty.
         */
        T *m_pointer = nullptr;
    };

    template <class T>
    WeakPtr<T>::WeakPtr() = default;

    template <class T>
    WeakPtr<T>::WeakPtr( T *object )
    {
        if( object )
        {
            // Safety: catch exceptions when accessing partially destroyed objects
            try
            {
                if( object->getWeakReferences() != -1 )
                {
                    m_pointer = object;
                    object->addWeakReference();
                }
            }
            catch( ... )
            {
                // Object is being destroyed, store pointer without weak reference bookkeeping
                m_pointer = object;
            }
        }
    }

    template <class T>
    WeakPtr<T>::WeakPtr( const WeakPtr &other )
    {
        auto pObject = other.get();
        if( pObject )
        {
            if( pObject->getWeakReferences() != -1 )
            {
                m_pointer = pObject;
                pObject->addWeakReference();
            }
        }
    }

    template <class T>
    WeakPtr<T>::WeakPtr( const SmartPtr<T> &other )
    {
        auto object = other.get();
        if( object )
        {
            if( object->getWeakReferences() != -1 )
            {
                m_pointer = object;
                object->addWeakReference();
            }
        }
    }

    template <class T>
    WeakPtr<T>::WeakPtr( const RawPtr<T> &object )
    {
        *this = object.get();
    }

    template <class T>
    template <class B>
    WeakPtr<T>::WeakPtr( B *pointer )
    {
        T *object = nullptr;

#if WP_ENABLE_PTR_EXCEPTIONS
        try
        {
            T *p = static_cast<T *>( pointer );  // static type checking
            p = p;                               // avoid warning

            object = (T *)dynamic_cast<T *>( pointer );
        }
        catch( std::exception & )
        {
            throw;
        }
#else
        object = static_cast<T *>( pointer );
#endif

        if( object )
        {
            // Safety: catch exceptions when object is partially destroyed
            try
            {
                if( object->getWeakReferences() != -1 )
                {
                    object->addWeakReference();
                    m_pointer = object;
                }
            }
            catch( ... )
            {
                // Object being destroyed, store pointer without weak ref bookkeeping
                m_pointer = object;
            }
        }
    }

    template <class T>
    template <class B>
    WeakPtr<T>::WeakPtr( const WeakPtr<B> &rkPointer )
    {
        T *object = nullptr;

#if WP_ENABLE_PTR_EXCEPTIONS
        try
        {
            B *pointer = rkPointer.get();
            T *p = static_cast<T *>( pointer );  // static type checking
            p = p;                               // avoid warning

            object = static_cast<T *>( dynamic_cast<T *>( pointer ) );
        }
        catch( std::exception & )
        {
            throw;
        }
#else
        B *pointer = rkPointer.get();
        object = static_cast<T *>( pointer );
#endif

        if( object )
        {
            if( object->getWeakReferences() != -1 )
            {
                object->addWeakReference();
                m_pointer = object;
            }
        }
    }

    template <class T>
    template <class B>
    WeakPtr<T>::WeakPtr( const SmartPtr<B> &other )
    {
        T *object = nullptr;

#if WP_ENABLE_PTR_EXCEPTIONS
        try
        {
            B *pointer = other.get();
            T *p = static_cast<T *>( pointer );  // static type checking
            p = p;                               // avoid warning

            object = static_cast<T *>( dynamic_cast<T *>( pointer ) );
        }
        catch( std::exception & )
        {
            throw;
        }
#else
        B *pointer = other.get();
        object = static_cast<T *>( pointer );
#endif

        if( object )
        {
            if( object->getWeakReferences() != -1 )
            {
                object->addWeakReference();
                m_pointer = object;
            }
        }
    }

    template <class T>
    WeakPtr<T>::~WeakPtr()
    {
        auto object = get();
        if( object && object->isAlive() && object->getReferences() > 0 )
        {
#if WP_TRACK_REFERENCES
            // A weak pointer may outlive the derived object's destructor while its
            // pooled storage is still addressable. Avoid virtual dispatch through
            // that object's already-destroyed vtable; the base bookkeeping remains
            // valid for a live ISharedObject.
            object->ISharedObject::removeWeakReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
            object->ISharedObject::removeWeakReference();
#endif

            m_pointer = nullptr;
        }
    }

    template <class T>
    u32 WeakPtr<T>::use_count() const noexcept
    {
        if( auto object = get() )
        {
            if( object->isAlive() )
            {
                const auto references = object->getReferences();

                // ISharedObject changes its reference count to a negative sentinel before
                // running destruction.  Returning that signed value as u32 made an expired
                // weak pointer appear to have billions of owners, allowing lock() to
                // resurrect an object whose destructor had already run.
                if( references > 0 )
                {
                    return static_cast<u32>( references );
                }
            }
        }

        return 0;
    }

    template <class T>
    bool WeakPtr<T>::expired() const noexcept
    {
        return use_count() == 0;
    }

    template <class T>
    SmartPtr<T> WeakPtr<T>::lock() const noexcept
    {
        return expired() ? nullptr : SmartPtr<T>( m_pointer );
    }

    template <class T>
    T &WeakPtr<T>::operator*() const
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
    T *WeakPtr<T>::operator->()
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
    const T *WeakPtr<T>::operator->() const
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
    bool WeakPtr<T>::operator!() const
    {
        return m_pointer == nullptr;
    }

    template <class T>
    WeakPtr<T>::operator bool() const
    {
        return m_pointer != nullptr;
    }

    template <class T>
    WeakPtr<T> &WeakPtr<T>::operator=( T *other ) throw()
    {
        if( m_pointer != other )
        {
            T *pNewObject = nullptr;
            if( other )
            {
#if WP_TRACK_REFERENCES
                other->addWeakReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
                other->addWeakReference();
#endif

                pNewObject = other;
            }

            auto pObject = this->get();
            if( pObject && pObject->isAlive() && pObject->getReferences() > 0 )
            {
#if WP_TRACK_REFERENCES
                pObject->ISharedObject::removeWeakReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
                pObject->ISharedObject::removeWeakReference();
#endif
            }

            m_pointer = pNewObject;
        }

        return *this;
    }

    template <class T>
    WeakPtr<T> &WeakPtr<T>::operator=( const WeakPtr &other ) throw()
    {
        T *pNewObject = nullptr;

        if( auto otherObject = other.get() )
        {
#if WP_TRACK_REFERENCES
            otherObject->addWeakReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
            otherObject->addWeakReference();
#endif

            pNewObject = otherObject;
        }

        if( auto pObject = get(); pObject && pObject->isAlive() && pObject->getReferences() > 0 )
        {
#if WP_TRACK_REFERENCES
            pObject->ISharedObject::removeWeakReference( this, __FILE__, __LINE__, __FUNCTION__ );
#else
            pObject->ISharedObject::removeWeakReference();
#endif
        }

        m_pointer = pNewObject;

        return *this;
    }

    /*
    template <class T>
    FBWeakPtr<T> &FBWeakPtr<T>::operator=( const FBSmartPtr<T> &other ) throw()
    {
        T *pNewObject = nullptr;
        auto otherObject = other.get();
        if( otherObject )
        {
#    if WP_TRACK_REFERENCES
            otherObject->addWeakReference( this, __FILE__, __LINE__, __FUNCTION__ );
#    else
            otherObject->addWeakReference();
#    endif

            pNewObject = otherObject;
        }

        auto pObject = this->m_object;
        if( pObject )
        {
#    if WP_TRACK_REFERENCES
            pObject->removeWeakReference( this, __FILE__, __LINE__, __FUNCTION__ );
#    else
            pObject->removeWeakReference();
#    endif
        }

        m_object = pNewObject;

        return *this;
    }
    */

    template <class T>
    bool WeakPtr<T>::operator==( T *other ) const
    {
        return m_pointer == other;
    }

    template <class T>
    bool WeakPtr<T>::operator!=( T *other ) const
    {
        return m_pointer != other;
    }

    template <class T>
    bool WeakPtr<T>::operator==( const WeakPtr &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    bool WeakPtr<T>::operator!=( const WeakPtr &other ) const
    {
        return m_pointer != other.get();
    }

    template <class T>
    bool WeakPtr<T>::operator==( const RawPtr<T> &other ) const
    {
        return m_pointer == other.get();
    }

    template <class T>
    template <class B>
    bool WeakPtr<T>::operator==( const RawPtr<B> &other ) const
    {
        return get() == other.get();
    }

    template <class T>
    T *WeakPtr<T>::get() const
    {
        return m_pointer;
    }

    /**
     * Get a raw pointer to the object pointed to by the given weak pointer.
     *
     * @tparam T The type of the object pointed to by the weak pointer.
     * @param pointer The weak pointer to the object.
     * @return A raw pointer to the object pointed to by the weak pointer.
     */
    template <class T>
    T *get_pointer( const WeakPtr<T> &pointer )
    {
        return pointer.get();
    }
}  // namespace workphone

#endif
