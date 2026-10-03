#ifndef AtomicValue_h__
#define AtomicValue_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <atomic>

namespace workphone
{
    /** Atomic value to wrap a generic value. */
    template <class T>
    class AtomicValue
    {
    public:
        /** Default constructor. */
        AtomicValue();

        /** Constructor that copies a value. */
        AtomicValue( const T &other );

        /** Copy constructor. */
        AtomicValue( const AtomicValue &other );

        /** Destructor. */
        ~AtomicValue();

        /** Assignment operator. */
        T operator=( const AtomicValue<T> &other );

        /** Assignment operator. */
        T operator=( const T &other );

        /** Value operator. */
        operator T() const;

        /** Returns a value. */
        T get() const;

        /** Operator. */
        T operator+( const T &other ) const;

        /** Operator. */
        T operator+=( const T &other );

        /** Operator. */
        T operator-( const T &other ) const;

        /** Operator. */
        T operator-=( const T &other );

        /** Assignment operator. */
        bool operator==( const T &other ) const;

        /** Assignment operator. */
        bool operator!=( const T &other ) const;

        /** Prefix operator. */
        T operator++( void );

        /** Postfix operator. */
        T operator--( void );

        /** Prefix operator. */
        T operator++( int );

        /** Postfix operator. */
        T operator--( int );

        /** Fetches the value.
        @param arg The value.
        @param order The memory order.
        @return The object's value.
        */
        T fetch_and( T arg, std::memory_order order = std::memory_order_seq_cst ) noexcept;

        /** Atomically adds arg to the value and returns the previous value.
        @param arg The value to add.
        @param order The memory order.
        @return The value immediately preceding the effects of this function.
        */
        T fetch_add( T arg, std::memory_order order = std::memory_order_seq_cst ) noexcept;

        /** Atomically subtracts arg from the value and returns the previous value.
        @param arg The value to subtract.
        @param order The memory order.
        @return The value immediately preceding the effects of this function.
        */
        T fetch_sub( T arg, std::memory_order order = std::memory_order_seq_cst ) noexcept;

        /** Atomically performs bitwise XOR and returns the previous value.
        @param arg The value to XOR with.
        @param order The memory order.
        @return The value immediately preceding the effects of this function.
        */
        T fetch_xor( T arg, std::memory_order order = std::memory_order_seq_cst ) noexcept;

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

        /** Atomically compares the value with expected and exchanges it with desired if equal.
        @param expected Reference to the value expected to be found in the atomic object.
        @param desired The value to store in the atomic object if it is as expected.
        @param success The memory synchronization ordering for the read-modify-write operation if
        successful.
        @param failure The memory synchronization ordering for the load operation if unsuccessful.
        @return True if the comparison succeeded and the exchange was performed, false otherwise.
        */
        bool compare_exchange_weak( T &expected, T desired,
                                    std::memory_order success = std::memory_order_seq_cst,
                                    std::memory_order failure = std::memory_order_seq_cst ) noexcept;

    private:
        /// The atomic value
        std::atomic<T> m_value;
    };

    template <class T>
    AtomicValue<T>::AtomicValue() : m_value{}
    {
    }

    template <class T>
    AtomicValue<T>::AtomicValue( const T &other ) : m_value{ other }
    {
    }

    template <class T>
    AtomicValue<T>::AtomicValue( const AtomicValue &other ) : m_value{ other.load() }
    {
    }

    template <class T>
    AtomicValue<T>::~AtomicValue()
    {
    }

    template <class T>
    T AtomicValue<T>::operator=( const AtomicValue<T> &value )
    {
        T current = value.load();
        m_value.store( current );
        return current;
    }

    template <class T>
    T AtomicValue<T>::operator=( const T &value )
    {
        m_value.store( value );
        return value;
    }

    template <class T>
    AtomicValue<T>::operator T() const
    {
        return m_value.load();
    }

    template <class T>
    T AtomicValue<T>::get() const
    {
        return m_value.load();
    }

    template <class T>
    T AtomicValue<T>::operator+( const T &other ) const
    {
        return m_value.load() + other;
    }

    template <class T>
    T AtomicValue<T>::operator+=( const T &other )
    {
        return m_value.fetch_add( other ) + other;
    }

    template <class T>
    T AtomicValue<T>::operator-( const T &other ) const
    {
        return m_value.load() - other;
    }

    template <class T>
    T AtomicValue<T>::operator-=( const T &other )
    {
        return m_value.fetch_sub( other ) - other;
    }

    template <class T>
    bool AtomicValue<T>::operator==( const T &other ) const
    {
        return m_value.load() == other;
    }

    template <class T>
    bool AtomicValue<T>::operator!=( const T &other ) const
    {
        return m_value.load() != other;
    }

    template <class T>
    T AtomicValue<T>::operator++( void )
    {
        return ++m_value;
    }

    template <class T>
    T AtomicValue<T>::operator--( void )
    {
        return --m_value;
    }

    template <class T>
    T AtomicValue<T>::operator++( int )
    {
        return m_value++;
    }

    template <class T>
    T AtomicValue<T>::operator--( int )
    {
        return m_value--;
    }

    template <class T>
    T AtomicValue<T>::fetch_and( T arg, std::memory_order order ) noexcept
    {
        return m_value.fetch_and( arg, order );
    }

    template <class T>
    T AtomicValue<T>::fetch_add( T arg, std::memory_order order ) noexcept
    {
        return m_value.fetch_add( arg, order );
    }

    template <class T>
    T AtomicValue<T>::fetch_sub( T arg, std::memory_order order ) noexcept
    {
        return m_value.fetch_sub( arg, order );
    }

    template <class T>
    T AtomicValue<T>::fetch_xor( T arg, std::memory_order order ) noexcept
    {
        return m_value.fetch_xor( arg, order );
    }

    template <class T>
    void AtomicValue<T>::store( T v, std::memory_order m )
    {
        m_value.store( v, m );
    }

    template <class T>
    T AtomicValue<T>::load( std::memory_order m ) const
    {
        return m_value.load( m );
    }

    template <class T>
    bool AtomicValue<T>::compare_exchange_weak( T &expected, T desired, std::memory_order success,
                                                std::memory_order failure ) noexcept
    {
        return m_value.compare_exchange_weak( expected, desired, success, failure );
    }

}  // namespace workphone

#endif  // AtomicValue_h__
