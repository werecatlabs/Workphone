#ifndef __AtomicNumber_h__
#define __AtomicNumber_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <atomic>
#include <type_traits>

namespace workphone
{

    /** Atomic number to wrap a generic value. */
    template <class T>
    class AtomicNumber
    {
    public:
        /** Default constructor. */
        AtomicNumber();

        /** Constructor that copies a value. */
        AtomicNumber( const T &other );

        /** Copy constructor. */
        AtomicNumber( const AtomicNumber &other );

        /** Destructor. */
        ~AtomicNumber();

        /** Assignment operator. */
        AtomicNumber<T> &operator=( const AtomicNumber<T> &other );

        /** Assignment operator. */
        AtomicNumber<T> &operator=( const T &other );

        /** Conversion operator. */
        operator T() const noexcept;

        /** Gets the value of this atomic type. */
        T get() const noexcept;

        /** Operator. */
        AtomicNumber<T> operator+( const T &other ) const;

        /** Operator. */
        AtomicNumber<T> &operator+=( const T &other );

        /** Operator. */
        AtomicNumber<T> operator-( const T &other ) const;

        /** Operator. */
        AtomicNumber<T> &operator-=( const T &other );

        /** Comparison operator. */
        template <class B>
        bool operator==( B other ) const;

        /** Comparison operator. */
        template <class B>
        bool operator!=( B other ) const;

        /** Prefix operator. */
        AtomicNumber<T> &operator++( void );

        /** Prefix operator. */
        AtomicNumber<T> &operator--( void );

        /** Postfix operator. */
        T operator++( s32 );

        /** Postfix operator. */
        T operator--( s32 );

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

        T fetch_sub( T arg, std::memory_order order = std::memory_order_seq_cst ) noexcept
        {
            static_assert( std::is_integral<T>::value, "fetch_sub requires an integral type" );
            return m_value.fetch_sub( arg, order );
        }

        /** Store's a value.
        @param v The value.
        @param m The memory order.
        */
        void store( T v, std::memory_order m = std::memory_order_seq_cst ) noexcept;

        /** Load's a value.
        @param m The memory order.
        @return The return value.
        */
        T load( std::memory_order m = std::memory_order_seq_cst ) const noexcept;

        /** Atomically compares the value with expected and, if equal, replaces it with desired.
        @param expected Reference to the expected value; updated with the actual value on failure.
        @param desired The value to store if the comparison succeeds.
        @param order The memory order to apply on success.
        @return True if the exchange was performed, false otherwise.
        */
        bool compareExchange( T &expected, T desired,
                              std::memory_order order = std::memory_order_seq_cst ) noexcept;

    private:
        /// The value
        std::atomic<T> m_value;
    };

    template <class T>
    AtomicNumber<T>::AtomicNumber() : m_value( T( 0 ) )
    {
    }

    template <class T>
    AtomicNumber<T>::AtomicNumber( const T &other ) : m_value( other )
    {
    }

    template <class T>
    AtomicNumber<T>::AtomicNumber( const AtomicNumber &other )
    {
        auto value = other.get();
        m_value = value;
    }

    template <class T>
    AtomicNumber<T>::~AtomicNumber() = default;

    template <class T>
    AtomicNumber<T> &AtomicNumber<T>::operator=( const AtomicNumber<T> &other )
    {
        m_value = other.get();
        return *this;
    }

    template <class T>
    AtomicNumber<T> &AtomicNumber<T>::operator=( const T &other )
    {
        m_value = other;
        return *this;
    }

    template <class T>
    AtomicNumber<T>::operator T() const noexcept
    {
        return m_value;
    }

    template <class T>
    T AtomicNumber<T>::get() const noexcept
    {
        return m_value;
    }

    template <class T>
    AtomicNumber<T> AtomicNumber<T>::operator+( const T &other ) const
    {
        return AtomicNumber<T>( m_value.load() + other );
    }

    template <class T>
    AtomicNumber<T> &AtomicNumber<T>::operator+=( const T &other )
    {
        m_value += other;
        return *this;
    }

    template <class T>
    AtomicNumber<T> AtomicNumber<T>::operator-( const T &other ) const
    {
        return AtomicNumber<T>( m_value.load() - other );
    }

    template <class T>
    AtomicNumber<T> &AtomicNumber<T>::operator-=( const T &other )
    {
        m_value -= other;
        return *this;
    }

    template <class T>
    template <class B>
    bool AtomicNumber<T>::operator==( B other ) const
    {
        return m_value == other;
    }

    template <class T>
    template <class B>
    bool AtomicNumber<T>::operator!=( B other ) const
    {
        return m_value != other;
    }

    template <class T>
    AtomicNumber<T> &AtomicNumber<T>::operator++( void )
    {
        ++m_value;
        return *this;
    }

    template <class T>
    AtomicNumber<T> &AtomicNumber<T>::operator--( void )
    {
        --m_value;
        return *this;
    }

    template <class T>
    T AtomicNumber<T>::operator++( s32 )
    {
        return m_value++;
    }

    template <class T>
    T AtomicNumber<T>::operator--( s32 )
    {
        return m_value--;
    }

    template <class T>
    T AtomicNumber<T>::fetch_or( T arg, std::memory_order order ) noexcept
    {
        static_assert( std::is_integral<T>::value, "fetch_or requires an integral type" );
        return m_value.fetch_or( arg, order );
    }

    template <class T>
    T AtomicNumber<T>::fetch_and( T arg, std::memory_order order ) noexcept
    {
        static_assert( std::is_integral<T>::value, "fetch_and requires an integral type" );
        return m_value.fetch_and( arg, order );
    }

    template <class T>
    void AtomicNumber<T>::store( T v, std::memory_order m ) noexcept
    {
        m_value.store( v, m );
    }

    template <class T>
    T AtomicNumber<T>::load( std::memory_order m ) const noexcept
    {
        return m_value.load( m );
    }

    template <class T>
    bool AtomicNumber<T>::compareExchange( T &expected, T desired, std::memory_order order ) noexcept
    {
        return m_value.compare_exchange_strong( expected, desired, order );
    }

}  // namespace workphone

#endif  // __AtomicNumber_h__
