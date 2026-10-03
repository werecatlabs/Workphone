#ifndef __WP_RawPointer_h__
#define __WP_RawPointer_h__

#include <Workphone/WorkphoneConfig.hpp>
#include <Workphone/Core/Exception.hpp>
#include <atomic>
#include <type_traits>

namespace workphone
{

    /**
     * @brief A smart pointer that does not own the pointed-to object.
     *
     * The RawPtr class is a smart pointer that does not own the pointed-to object, and thus does not
     * manage its lifetime. It is a lightweight alternative to std::unique_ptr and std::shared_ptr, and
     * can be used in cases where you don't need to manage the lifetime of an object, but still need a
     * way to access it and perform operations on it.
     *
     * The RawPtr class is implemented using a raw pointer and provides the basic functionality of a
     * smart pointer, such as dereferencing, pointer arithmetic, and comparison operators. It also
     * provides a constructor that takes a raw pointer and initializes the smart pointer with it.
     *
     * @tparam T The type of the pointed-to object.
     */
    template <typename T>
    class RawPtr
    {
    public:
        /**
         * @brief Constructs a new `RawPtr` instance with a null pointer.
         */
        RawPtr() = default;

        /**
         * @brief Constructs a new RawPtr object with the given raw pointer.
         *
         * @param object The raw pointer to manage.
         */
        RawPtr( T *other );

        /**
         * @brief Copy constructor for RawPtr.
         * @tparam T The type of object being managed by the pointer.
         * @param other The RawPtr object to be copied.
         */
        RawPtr( const RawPtr &other );

        /**
         * @brief Construct a new RawPtr object from a raw pointer of a derived type.
         *
         * @tparam B The type of the derived class.
         * @param other A raw pointer of the derived type.
         * @note The constructor will only compile if the derived type is convertible to
         * the base type T. This guarantees that the conversion is safe.
         */
        template <typename B>
        RawPtr( B *pointer );

        /**
         * Copy constructor from a RawPtr of different type.
         * The constructor is enabled only if the source type B is convertible to the destination type T.
         *
         * @tparam B Type of the source pointer.
         * @param other A reference to the RawPtr object to copy from.
         */
        template <typename B>
        RawPtr( const RawPtr<B> &other );

        /**
         * @brief Destructor for the `RawPtr` class.
         *
         * The destructor sets the internal pointer to `nullptr`.
         */
        ~RawPtr();

        // implicit conversions
        T &operator*() const;

        /**
         * Overloads the arrow operator for dereferencing the managed object pointer.
         *
         * If the managed object pointer is null and `WP_ENABLE_PTR_EXCEPTIONS` is defined,
         * this function will throw an `Exception` with the message "Null pointer exception."
         *
         * @return a pointer to the managed object
         */
        T *operator->();

        /**
         * @brief Dereferences the pointer to access members of the object it points to.
         *
         * If the pointer is null and WP_ENABLE_PTR_EXCEPTIONS is defined, this function throws an
         * Exception with the message "Null pointer exception.". Otherwise, the behavior is undefined.
         *
         * @return A pointer to the object the RawPtr points to.
         */
        constexpr T *operator->() const;

        bool operator!() const;
        operator T *() const;

        // assignment
        RawPtr &operator=( T *other ) throw();
        RawPtr &operator=( const RawPtr &other ) throw();

        // comparisons
        bool operator==( T *other ) const;
        bool operator!=( T *other ) const;
        bool operator==( const RawPtr &other ) const;
        bool operator!=( const RawPtr &other ) const;

        /**
         * @brief Returns a raw pointer to the managed object.
         *
         * @return A raw pointer to the managed object.
         */
        T *get() const;

    protected:
        /**
         * @brief A raw pointer to the object that is being managed by this `RawPtr`.
         *
         * This is a non-owning pointer, and the object it points to is not automatically
         * deleted when this `RawPtr` is destroyed.
         */
        std::atomic<T *> m_pointer = nullptr;
    };

    template <class T>
    RawPtr<T>::RawPtr( T *object ) : m_pointer( object )
    {
    }

    template <class T>
    RawPtr<T>::RawPtr( const RawPtr &other )
    {
        m_pointer = other.get();
    }

    template <class T>
    template <typename B>
    RawPtr<T>::RawPtr( B *other )
    {
        WP_ASSERT( dynamic_cast<T *>( other ) );
        m_pointer = static_cast<T *>( other );
    }

    template <typename T>
    template <typename B>
    RawPtr<T>::RawPtr( const RawPtr<B> &other ) : m_pointer( other.get() )
    {
#ifdef _DEBUG
        if( m_pointer )
        {
            WP_ASSERT( dynamic_cast<T *>( m_pointer ) );
        }
#endif
    }

    template <class T>
    RawPtr<T>::~RawPtr()
    {
        m_pointer = nullptr;
    }

    template <class T>
    T &RawPtr<T>::operator*() const
    {
#if WP_ENABLE_PTR_EXCEPTIONS
        if( !m_pointer )
        {
            throw Exception( "Null pointer exception." );
        }
#endif

        return *m_pointer;
    }

    template <class T>
    T *RawPtr<T>::operator->()
    {
#if WP_ENABLE_PTR_EXCEPTIONS
        if( !m_pointer )
        {
            throw Exception( "Null pointer exception." );
        }
#endif

        return m_pointer;
    }

    template <class T>
    constexpr T *RawPtr<T>::operator->() const
    {
#if WP_ENABLE_PTR_EXCEPTIONS
        if( !m_pointer )
        {
            throw Exception( "Null pointer exception." );
        }
#endif

        return m_pointer;
    }

    template <class T>
    bool RawPtr<T>::operator!() const
    {
        return m_pointer == nullptr;
    }

    template <class T>
    RawPtr<T>::operator T *() const
    {
        return m_pointer;
    }

    template <class T>
    RawPtr<T> &RawPtr<T>::operator=( T *other ) throw()
    {
        m_pointer = other;
        return *this;
    }

    template <class T>
    RawPtr<T> &RawPtr<T>::operator=( const RawPtr &pointer ) throw()
    {
        m_pointer = pointer.m_pointer.load();
        return *this;
    }

    template <class T>
    bool RawPtr<T>::operator==( T *other ) const
    {
        return m_pointer == other;
    }

    template <class T>
    bool RawPtr<T>::operator!=( T *other ) const
    {
        return m_pointer != other;
    }

    template <class T>
    bool RawPtr<T>::operator==( const RawPtr &other ) const
    {
        return m_pointer == other.m_pointer;
    }

    template <class T>
    bool RawPtr<T>::operator!=( const RawPtr &other ) const
    {
        return m_pointer != other.m_pointer;
    }

    template <class T>
    T *RawPtr<T>::get() const
    {
        return m_pointer;
    }

}  // namespace workphone

#endif  // __WP_RawPointer_h__
