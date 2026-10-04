#ifndef AtomicObject_h__
#define AtomicObject_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Atomics/Atomic.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/Thread/ScopedLock.hpp>

namespace workphone
{

    /**
     * @brief A mutex-based atomic wrapper for arbitrary types.
     *
     * This class provides atomic-like semantics for types that are not
     * necessarily lock-free or trivially copyable by serialising access
     * through an internal read/write spin mutex (`SpinRWMutex`).
     *
     * The implementation intentionally mimics parts of the `std::atomic`
     * interface (exchange, compare_exchange_*, fetch_*, load, store, etc.)
     * but uses locking. Memory order parameters are accepted for API
     * compatibility but are ignored by this mutex-based implementation.
     *
     * @tparam T Type of the contained value. Methods may require `T` to be
     *           copyable/movable and for certain fetch operations to support
     *           bitwise or atomic-like member functions (see individual
     *           method documentation).
     *
     * @note Thread-safety:
     *       - Distinct instances may be accessed concurrently.
     *       - Concurrent access to the same instance is synchronised by the
     *         internal `SpinRWMutex`.
     */
    template <class T>
    class AtomicObject
    {
    public:
        using value_type = T;

        /** Default constructor. */
        AtomicObject();

        /** Constructor that copies. */
        AtomicObject( const T &other );

        /** Constructor that moves a value. */
        explicit AtomicObject( T &&other ) noexcept;

        /** Copy constructor. */
        AtomicObject( const AtomicObject &other );

        /** Move constructor. */
        AtomicObject( AtomicObject &&other ) noexcept;

        /** Assignment operator from value type. */
        AtomicObject &operator=( const T &value );

        /** Move assignment operator from value type. */
        AtomicObject &operator=( T &&value ) noexcept;

        /** Copy assignment operator. */
        AtomicObject &operator=( const AtomicObject &other );

        /** Move assignment operator. */
        AtomicObject &operator=( AtomicObject &&other ) noexcept;

        /** Comparison operator. */
        bool operator==( const T &value ) const;

        /** Comparison operator. */
        bool operator==( const AtomicObject &other ) const;

        /** Comparison operator. */
        bool operator!=( const T &value ) const;

        /** Comparison operator. */
        bool operator!=( const AtomicObject &other ) const;

        /** Gets the value of this atomic type. */
        operator T() const;

        /** Exchange the desired value.
        @param desired The desired value.
        @param order The memory order.
        @return The object's value.
        */
        T exchange( T desired, std::memory_order order = std::memory_order_seq_cst ) noexcept;

        /** Compare and swap operation.
         * @param expected Reference to expected value, updated with actual value if comparison fails
         * @param desired The new value to set if comparison succeeds
         * @param order Memory order (ignored in this mutex-based implementation)
         * @return True if the exchange was successful
         */
        bool compare_exchange_weak( T &expected, T desired,
                                    std::memory_order order = std::memory_order_seq_cst ) noexcept
        {
            ScopedLock lock( this, true );
            if( m_value == expected )
            {
                m_value = std::move( desired );
                return true;
            }
            else
            {
                expected = m_value;
                return false;
            }
        }

        /** Compare and swap operation (strong version - same as weak for mutex-based implementation).
         * @param expected Reference to expected value, updated with actual value if comparison fails
         * @param desired The new value to set if comparison succeeds
         * @param order Memory order (ignored in this mutex-based implementation)
         * @return True if the exchange was successful
         */
        bool compare_exchange_strong( T &expected, T desired,
                                      std::memory_order order = std::memory_order_seq_cst ) noexcept
        {
            return compare_exchange_weak( expected, std::move( desired ), order );
        }

        /** Fetches the value.
        @param arg The value.
        @param order The memory order.
        @return The object's value.
        */
        T fetch_or( T arg, std::memory_order order = std::memory_order_seq_cst ) noexcept;

        /** Fetches the value.
        @param arg The value.
        @param order The memory order.
        @return The object's value.
        */
        T fetch_and( T arg, std::memory_order order = std::memory_order_seq_cst ) noexcept;

        /** To know if the object is lock free.
        @return A boolean indicating whether the object is lock free.
        */
        bool is_lock_free() const;

        /** Store's a value.
        @param v The value.
        @param m The memory order.
        */
        void store( T v, std::memory_order m = std::memory_order_seq_cst );

        /** Load's a value.
        @param m The memory order.
        @return The return value.
        */
        T load( std::memory_order m = std::memory_order_seq_cst ) const;

        void lock()
        {
            m_mutex.lock();
        }

        bool try_lock()
        {
            return m_mutex.try_lock();
        }

        void unlock()
        {
            m_mutex.unlock();
        }

        void lock_shared()
        {
            m_mutex.lock_shared();
        }

        void unlock_shared()
        {
            m_mutex.unlock_shared();
        }

    protected:
        /// The mutex
        mutable RecursiveSpinMutex m_mutex;

        /// The object
        T m_value;
    };

    template <class T>
    AtomicObject<T>::AtomicObject()
    {
        m_value = T();
    }

    template <class T>
    AtomicObject<T>::AtomicObject( const T &other ) : m_value( other )
    {
    }

    template <class T>
    AtomicObject<T>::AtomicObject( T &&other ) noexcept : m_value( std::move( other ) )
    {
    }

    template <class T>
    AtomicObject<T>::AtomicObject( const AtomicObject<T> &other )
    {
        ScopedLock lock( &other, false );
        m_value = other.m_value;
    }

    template <class T>
    AtomicObject<T>::AtomicObject( AtomicObject<T> &&other ) noexcept
    {
        ScopedLock lock( &other, true );
        m_value = std::move( other.m_value );
    }

    template <class T>
    AtomicObject<T> &AtomicObject<T>::operator=( const T &value )
    {
        store( value );
        return *this;
    }

    template <class T>
    AtomicObject<T> &AtomicObject<T>::operator=( T &&value ) noexcept
    {
        store( std::move( value ) );
        return *this;
    }

    template <class T>
    AtomicObject<T> &AtomicObject<T>::operator=( const AtomicObject<T> &other )
    {
        if( this != &other )
        {
            // Acquire locks in consistent order to prevent deadlock
            if( this < &other )
            {
                ScopedLock lock1( this, true );
                ScopedLock lock2( &other, false );
                m_value = other.m_value;
            }
            else
            {
                ScopedLock lock1( &other, false );
                ScopedLock lock2( this, true );
                m_value = other.m_value;
            }
        }

        return *this;
    }

    template <class T>
    AtomicObject<T> &AtomicObject<T>::operator=( AtomicObject<T> &&other ) noexcept
    {
        if( this != &other )
        {
            // Acquire locks in consistent order to prevent deadlock
            if( this < &other )
            {
                ScopedLock lock1( this, true );
                ScopedLock lock2( &other, true );
                m_value = std::move( other.m_value );
            }
            else
            {
                ScopedLock lock1( &other, true );
                ScopedLock lock2( this, true );
                m_value = std::move( other.m_value );
            }
        }

        return *this;
    }

    template <class T>
    bool AtomicObject<T>::operator==( const T &value ) const
    {
        ScopedLock lock( this, false );
        return m_value == value;
    }

    template <class T>
    bool AtomicObject<T>::operator==( const AtomicObject &other ) const
    {
        ScopedLock lock( this, true );
        ScopedLock lock1( &other, false );
        return m_value == other.m_value;
    }

    template <class T>
    bool AtomicObject<T>::operator!=( const T &value ) const
    {
        ScopedLock lock( this, false );
        return m_value != value;
    }

    template <class T>
    bool AtomicObject<T>::operator!=( const AtomicObject &other ) const
    {
        ScopedLock lock( this, false );
        ScopedLock lock1( &other, false );
        return m_value != other.m_value;
    }

    template <class T>
    AtomicObject<T>::operator T() const
    {
        return load();
    }

    template <class T>
    T AtomicObject<T>::exchange( T desired, std::memory_order order ) noexcept
    {
        ScopedLock lock( this, true );
        T old = std::move( m_value );
        m_value = std::move( desired );
        return old;
    }

    template <class T>
    T AtomicObject<T>::fetch_or( T arg, std::memory_order order ) noexcept
    {
        ScopedLock lock( this, false );
        return m_value;
    }

    template <class T>
    T AtomicObject<T>::fetch_and( T arg, std::memory_order order ) noexcept
    {
        ScopedLock lock( this, true );
        return m_value.fetch_and( arg );
    }

    template <class T>
    bool AtomicObject<T>::is_lock_free() const
    {
        return false;
    }

    template <class T>
    void AtomicObject<T>::store( T v, std::memory_order m )
    {
        ScopedLock lock( this, true );
        m_value = v;
    }

    template <class T>
    T AtomicObject<T>::load( std::memory_order m ) const
    {
        ScopedLock lock( this, false );
        return m_value;
    }

}  // namespace workphone

#endif  // AtomicObject_h__
