#ifndef AtomicFloat_h__
#define AtomicFloat_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneTypes.hpp>
#include <atomic>

namespace workphone
{

    /**
     * @brief A thread-safe atomic floating-point value.
     *
     * This class provides atomic operations for a floating-point value, using the
     * standard C++ library's std::atomic template. It supports atomic load, store,
     * exchange, compare-and-swap, and various arithmetic operations.
     *
     * @tparam FLOATING_POINT The floating-point type to use (e.g. float, double).
     */
    template <typename FLOATING_POINT>
    struct AtomicFloat
    {
        /**
         * @brief Default constructor.
         *
         * Initializes the atomic float to zero.
         */
        AtomicFloat();

        /**
         * @brief Constructor that initializes the atomic float with a given value.
         *
         * @param other The initial value for the atomic float.
         */
        AtomicFloat( const AtomicFloat &other );

        /**
         * @brief Constructor that initializes the atomic float with a given value.
         *
         * @param other The initial value for the atomic float.
         */
        AtomicFloat( const FLOATING_POINT &other );

        /**
         * @brief Atomically replaces the current value with a new value.
         *
         * @param value The new value.
         * @return The old value.
         */
        FLOATING_POINT fetch_and_store( FLOATING_POINT newValue );

        /**
         * @brief Atomically compares the current value with a comparand, and if they are equal,
         * replaces the current value with a new value.
         *
         * @param value The new value.
         * @param comparand The value to compare against.
         * @return The old value.
         */
        FLOATING_POINT compare_and_swap( FLOATING_POINT newValue, FLOATING_POINT comparand );

        /**
         * @brief Atomically loads the current value.
         *
         * @return The current value.
         */
        operator FLOATING_POINT() const;

        /**
         * @brief Atomically sets the value to a new value.
         *
         * @param rhs The new value.
         * @return A reference to the modified AtomicFloat.
         */
        AtomicFloat<FLOATING_POINT> &operator=( FLOATING_POINT rhs );

        /**
         * @brief Atomically sets the value to the value of another AtomicFloat.
         *
         * @param rhs The other AtomicFloat.
         * @return A reference to the modified AtomicFloat.
         */
        AtomicFloat<FLOATING_POINT> &operator=( AtomicFloat<FLOATING_POINT> &rhs );

        /**
         * @brief Atomically adds a value to the current value.
         *
         * @param value The value to add.
         * @return The new value.
         */
        FLOATING_POINT operator+=( FLOATING_POINT operand );

        /**
         * @brief Atomically multiplies the current value by a value.
         *
         * @param value The value to multiply by.
         * @return The new value.
         */
        FLOATING_POINT operator*=( FLOATING_POINT operand );

        /**
         * @brief Atomically divides the current value by a value.
         */
        FLOATING_POINT operator/=( FLOATING_POINT operand );

        /**
         * @brief Subtracts the specified value from the atomic float.
         *
         * @param value The value to subtract.
         * @return The old value of the atomic float minus the specified value.
         */
        FLOATING_POINT operator-=( FLOATING_POINT operand );

        /**
         * @brief Prefix increment operator for the atomic float.
         *
         * Increments the value of the atomic float by 1.0 and returns the new value.
         *
         * @return The new value of the atomic float.
         */
        FLOATING_POINT operator++();

        /**
         * @brief Prefix decrement operator for the atomic float.
         *
         * Decrements the value of the atomic float by 1.0 and returns the new value.
         *
         * @return The new value of the atomic float.
         */
        FLOATING_POINT operator--();

        /**
         * @brief Postfix increment operator for the atomic float.
         *
         * Increments the value of the atomic float by 1.0 and returns the old value.
         *
         * @return The old value of the atomic float.
         */
        FLOATING_POINT operator++( int );

        /**
         * @brief Postfix decrement operator for the atomic float.
         *
         * Decrements the value of the atomic float by 1.0 and returns the old value.
         *
         * @return The old value of the atomic float.
         */
        FLOATING_POINT operator--( int );

        /**
         * @brief Atomically adds the specified value to the atomic float.
         *
         * @param value The value to add.
         * @return The new value of the atomic float.
         */
        FLOATING_POINT fetch_and_add( FLOATING_POINT operand );

        /**
         * @brief Atomically increments the atomic float by 1.0 and returns the new value.
         *
         * @return The new value of the atomic float.
         */
        FLOATING_POINT fetch_and_increment();

        /**
         * @brief Atomically decrements the atomic float by 1.0 and returns the new value.
         *
         * @return The new value of the atomic float.
         */
        FLOATING_POINT fetch_and_decrement();

    private:
        std::atomic<FLOATING_POINT> value;
    };

    template <typename FLOATING_POINT>
    AtomicFloat<FLOATING_POINT>::AtomicFloat() : value( 0.0 )
    {
    }

    template <typename FLOATING_POINT>
    AtomicFloat<FLOATING_POINT>::AtomicFloat( const AtomicFloat &other )
    {
        value = other.value.load();
    }

    template <typename FLOATING_POINT>
    AtomicFloat<FLOATING_POINT>::AtomicFloat( const FLOATING_POINT &other )
    {
        value = other;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::fetch_and_store( FLOATING_POINT newValue )
    {
        return std::atomic_exchange( &value, newValue );
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::compare_and_swap( FLOATING_POINT newValue,
                                                                  FLOATING_POINT comparand )
    {
        std::atomic_compare_exchange_strong( &value, &comparand, newValue );
        return comparand;
    }

    template <typename FLOATING_POINT>
    AtomicFloat<FLOATING_POINT>::operator FLOATING_POINT() const
    {
        return value.load();
    }

    template <typename FLOATING_POINT>
    AtomicFloat<FLOATING_POINT> &AtomicFloat<FLOATING_POINT>::operator=( FLOATING_POINT rhs )
    {
        value.store( rhs );
        return *this;
    }

    template <typename FLOATING_POINT>
    AtomicFloat<FLOATING_POINT> &AtomicFloat<FLOATING_POINT>::operator=(
        AtomicFloat<FLOATING_POINT> &rhs )
    {
        value.store( rhs.value.load() );
        return *this;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::operator+=( FLOATING_POINT operand )
    {
        FLOATING_POINT old_val = value.load();
        while( !value.compare_exchange_weak( old_val, old_val + operand ) )
            ;
        return old_val + operand;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::operator*=( FLOATING_POINT operand )
    {
        FLOATING_POINT old_val = value.load();
        while( !value.compare_exchange_weak( old_val, old_val * operand ) )
            ;
        return old_val * operand;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::operator/=( FLOATING_POINT operand )
    {
        FLOATING_POINT old_val = value.load();
        while( !value.compare_exchange_weak( old_val, old_val / operand ) )
            ;
        return old_val / operand;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::operator-=( FLOATING_POINT operand )
    {
        FLOATING_POINT old_val = value.load();
        while( !value.compare_exchange_weak( old_val, old_val - operand ) )
            ;
        return old_val - operand;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::operator++()
    {
        return operator+=( 1.0 );
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::operator--()
    {
        return operator-=( 1.0 );
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::operator++( int )
    {
        FLOATING_POINT old_val = value.load();
        while( !value.compare_exchange_weak( old_val, old_val + 1.0 ) )
            ;
        return old_val;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::operator--( int )
    {
        FLOATING_POINT old_val = value.load();
        while( !value.compare_exchange_weak( old_val, old_val - 1.0 ) )
            ;
        return old_val;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::fetch_and_add( FLOATING_POINT operand )
    {
        FLOATING_POINT oldValue = value.load();
        while( !value.compare_exchange_weak( oldValue, oldValue + operand ) )
            ;
        return oldValue;
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::fetch_and_increment()
    {
        return fetch_and_add( 1.0 );
    }

    template <typename FLOATING_POINT>
    FLOATING_POINT AtomicFloat<FLOATING_POINT>::fetch_and_decrement()
    {
        return fetch_and_add( -1.0 );
    }

    using atomic_f32 = AtomicFloat<float>;
    using atomic_f64 = AtomicFloat<double>;

}  // namespace workphone

#endif  // AtomicFloat_h__
