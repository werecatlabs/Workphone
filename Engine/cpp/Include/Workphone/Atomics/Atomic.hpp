#ifndef Atomic_h__
#define Atomic_h__

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined( _MSC_VER ) && !defined( __clang__ )
#    include <intrin.h>
#elif !defined( __GNUC__ ) && !defined( __clang__ )
#    error Atomic requires MSVC, GCC, or Clang compiler intrinsics.
#endif

namespace workphone
{
    /** Engine memory ordering. No standard library atomic types are required. */
    enum class memory_semantics
    {
        full_fence,
        acquire,
        release,
        relaxed,
        acq_rel
    };

    namespace detail
    {
        template <bool Enabled, typename T>
        struct AtomicEnableIf
        {
        };

        template <typename T>
        struct AtomicEnableIf<true, T>
        {
            using type = T;
        };

        template <typename A, typename B>
        struct AtomicSameType
        {
            static constexpr bool value = false;
        };

        template <typename T>
        struct AtomicSameType<T, T>
        {
            static constexpr bool value = true;
        };

        template <typename T>
        struct AtomicRemoveCV
        {
            using type = T;
        };

        template <typename T>
        struct AtomicRemoveCV<const T> : AtomicRemoveCV<T>
        {
        };

        template <typename T>
        struct AtomicRemoveCV<volatile T> : AtomicRemoveCV<T>
        {
        };

        template <typename T>
        struct AtomicRemoveCV<const volatile T> : AtomicRemoveCV<T>
        {
        };

        template <typename T>
        struct AtomicFunction
        {
            static constexpr bool value = false;
        };

        template <typename R, typename... Args>
        struct AtomicFunction<R( Args... )>
        {
            static constexpr bool value = true;
        };

        template <typename R, typename... Args>
        struct AtomicFunction<R( Args..., ... )> : AtomicFunction<R( Args... )>
        {
        };

#if defined( __cpp_noexcept_function_type )
        template <typename R, typename... Args>
        struct AtomicFunction<R( Args... ) noexcept> : AtomicFunction<R( Args... )>
        {
        };

        template <typename R, typename... Args>
        struct AtomicFunction<R( Args..., ... ) noexcept> : AtomicFunction<R( Args... )>
        {
        };
#endif

        template <typename T>
        struct AtomicArithmetic
        {
            using difference_type = T;
            static constexpr bool enabled = false;
            static constexpr bool integral = false;
            static constexpr bool pointer = false;
        };

#define WP_DETAIL_ATOMIC_INTEGER( Integer )             \
    template <>                                         \
    struct AtomicArithmetic<Integer>                    \
    {                                                   \
        using difference_type = Integer;                \
        static constexpr bool enabled = true;           \
        static constexpr bool integral = true;          \
        static constexpr bool pointer = false;          \
        static Integer offset( Integer value ) noexcept \
        {                                               \
            return value;                               \
        }                                               \
    };

        WP_DETAIL_ATOMIC_INTEGER( char )
        WP_DETAIL_ATOMIC_INTEGER( signed char )
        WP_DETAIL_ATOMIC_INTEGER( unsigned char )
        WP_DETAIL_ATOMIC_INTEGER( short )
        WP_DETAIL_ATOMIC_INTEGER( unsigned short )
        WP_DETAIL_ATOMIC_INTEGER( int )
        WP_DETAIL_ATOMIC_INTEGER( unsigned int )
        WP_DETAIL_ATOMIC_INTEGER( long )
        WP_DETAIL_ATOMIC_INTEGER( unsigned long )
        WP_DETAIL_ATOMIC_INTEGER( long long )
        WP_DETAIL_ATOMIC_INTEGER( unsigned long long )
#if !defined( _MSC_VER ) || defined( _NATIVE_WCHAR_T_DEFINED )
        WP_DETAIL_ATOMIC_INTEGER( wchar_t )
#endif
        WP_DETAIL_ATOMIC_INTEGER( char16_t )
        WP_DETAIL_ATOMIC_INTEGER( char32_t )
#if defined( __cpp_char8_t )
        WP_DETAIL_ATOMIC_INTEGER( char8_t )
#endif
#undef WP_DETAIL_ATOMIC_INTEGER

        template <typename T>
        struct AtomicArithmetic<T *>
        {
            using difference_type = ptrdiff_t;
            static constexpr bool enabled =
                !AtomicSameType<typename AtomicRemoveCV<T>::type, void>::value &&
                !AtomicFunction<T>::value;
            static constexpr bool integral = false;
            static constexpr bool pointer = enabled;

            static uintptr_t offset( ptrdiff_t value ) noexcept
            {
                // Unsigned arithmetic also handles negative offsets without signed overflow.
                return static_cast<uintptr_t>( value ) * sizeof( T );
            }
        };

        template <size_t Size>
        struct AtomicWord;

#define WP_DETAIL_ATOMIC_WORD( Size, Signed, Unsigned ) \
    template <>                                         \
    struct AtomicWord<Size>                             \
    {                                                   \
        using type = Signed;                            \
        using unsigned_type = Unsigned;                 \
    };
        WP_DETAIL_ATOMIC_WORD( 1, char, uint8_t )
        WP_DETAIL_ATOMIC_WORD( 2, short, uint16_t )
#if defined( _MSC_VER )
        WP_DETAIL_ATOMIC_WORD( 4, long, uint32_t )
#else
        WP_DETAIL_ATOMIC_WORD( 4, int32_t, uint32_t )
#endif
        WP_DETAIL_ATOMIC_WORD( 8, int64_t, uint64_t )
#undef WP_DETAIL_ATOMIC_WORD

        constexpr bool atomicValidOrder( memory_semantics order ) noexcept
        {
            return order == memory_semantics::full_fence || order == memory_semantics::acquire ||
                   order == memory_semantics::release || order == memory_semantics::relaxed ||
                   order == memory_semantics::acq_rel;
        }

        constexpr memory_semantics atomicFailureOrder( memory_semantics order ) noexcept
        {
            return order == memory_semantics::release   ? memory_semantics::relaxed
                   : order == memory_semantics::acq_rel ? memory_semantics::acquire
                                                        : order;
        }

        inline void atomicCheckCompareOrder( memory_semantics success,
                                             memory_semantics failure ) noexcept
        {
            assert( atomicValidOrder( success ) && atomicValidOrder( failure ) );
            assert(
                failure == memory_semantics::relaxed ||
                ( failure == memory_semantics::acquire &&
                  ( success == memory_semantics::acquire || success == memory_semantics::acq_rel ||
                    success == memory_semantics::full_fence ) ) ||
                ( failure == memory_semantics::full_fence && success == memory_semantics::full_fence ) );
            (void)success;
            (void)failure;
        }

        template <typename Word>
        struct AtomicMachine;

#if defined( _MSC_VER ) && !defined( __clang__ )
        // Interlocked operations provide full fences on MSVC. This intentionally
        // strengthens weaker requests, including relaxed, on all supported architectures.
        template <typename Word>
        struct AtomicInterlockedOperations
        {
            static constexpr bool always_lock_free = true;

            static Word load( volatile Word *location, memory_semantics order ) noexcept
            {
                return AtomicMachine<Word>::compare_and_swap( location, 0, 0, order,
                                                              memory_semantics::relaxed );
            }

            static Word exchange( volatile Word *location, Word value, memory_semantics order ) noexcept
            {
                Word expected = load( location, memory_semantics::relaxed );
                for( ;; )
                {
                    Word observed = AtomicMachine<Word>::compare_and_swap(
                        location, value, expected, order, memory_semantics::relaxed );
                    if( observed == expected )
                    {
                        return observed;
                    }
                    expected = observed;
                }
            }

            static void store( volatile Word *location, Word value, memory_semantics order ) noexcept
            {
                exchange( location, value, order );
            }
        };

#    define WP_DETAIL_ATOMIC_INTERLOCKED( Word, Intrinsic )                                    \
        template <>                                                                            \
        struct AtomicMachine<Word> : AtomicInterlockedOperations<Word>                         \
        {                                                                                      \
            static Word compare_and_swap( volatile Word *location, Word value, Word comparand, \
                                          memory_semantics, memory_semantics ) noexcept        \
            {                                                                                  \
                return Intrinsic( location, value, comparand );                                \
            }                                                                                  \
        };
        WP_DETAIL_ATOMIC_INTERLOCKED( char, _InterlockedCompareExchange8 )
        WP_DETAIL_ATOMIC_INTERLOCKED( short, _InterlockedCompareExchange16 )
        WP_DETAIL_ATOMIC_INTERLOCKED( long, _InterlockedCompareExchange )
        WP_DETAIL_ATOMIC_INTERLOCKED( int64_t, _InterlockedCompareExchange64 )
#    undef WP_DETAIL_ATOMIC_INTERLOCKED
#else
        constexpr int atomicBuiltinOrder( memory_semantics order ) noexcept
        {
            return order == memory_semantics::relaxed   ? __ATOMIC_RELAXED
                   : order == memory_semantics::acquire ? __ATOMIC_ACQUIRE
                   : order == memory_semantics::release ? __ATOMIC_RELEASE
                   : order == memory_semantics::acq_rel ? __ATOMIC_ACQ_REL
                                                        : __ATOMIC_SEQ_CST;
        }

        template <typename Word>
        struct AtomicMachine
        {
            // Do not silently introduce a libatomic dependency on unsupported targets.
            static_assert( __atomic_always_lock_free( sizeof( Word ), nullptr ),
                           "This target does not provide lock-free atomics of this width" );
            static constexpr bool always_lock_free = true;

            static Word load( volatile Word *location, memory_semantics order ) noexcept
            {
                return __atomic_load_n( location, atomicBuiltinOrder( order ) );
            }

            static void store( volatile Word *location, Word value, memory_semantics order ) noexcept
            {
                __atomic_store_n( location, value, atomicBuiltinOrder( order ) );
            }

            static Word exchange( volatile Word *location, Word value, memory_semantics order ) noexcept
            {
                return __atomic_exchange_n( location, value, atomicBuiltinOrder( order ) );
            }

            static Word compare_and_swap( volatile Word *location, Word value, Word comparand,
                                          memory_semantics success, memory_semantics failure ) noexcept
            {
                __atomic_compare_exchange_n( location, &comparand, value, false,
                                             atomicBuiltinOrder( success ),
                                             atomicBuiltinOrder( failure ) );
                return comparand;
            }
        };
#endif
    }  // namespace detail

    /**
     * Engine atomic implemented with compiler intrinsics, with no STL dependency.
     * Supports value-initializable, trivially copyable values of 1, 2, 4, or 8 bytes,
     * including integers, enums, pointers, floats, and small plain data structures.
     * Comparison uses the complete object representation (including padding).
     * Loads default to acquire, stores to release, and other operations to full_fence.
     * Assignment from a value and implicit conversion also use full_fence.
     * MSVC strengthens all memory orders to full fences; GCC/Clang honor the requested order.
     * Default construction initializes T{}. Copies take an atomic snapshot; assignment
     * between atomics is a load followed by a store, not a transaction on both objects.
     * Construction/destruction require external synchronization. Requires C++11 or later.
     */
    template <typename T>
    class Atomic
    {
        static_assert( sizeof( T ) == 1 || sizeof( T ) == 2 || sizeof( T ) == 4 || sizeof( T ) == 8,
                       "Atomic supports only 1-, 2-, 4-, and 8-byte values" );
        static_assert( __is_trivially_copyable( T ), "Atomic requires a trivially copyable value" );
        static_assert( detail::AtomicSameType<T, typename detail::AtomicRemoveCV<T>::type>::value,
                       "Atomic value types cannot be const or volatile" );

        using Word = typename detail::AtomicWord<sizeof( T )>::type;
        using Unsigned = typename detail::AtomicWord<sizeof( T )>::unsigned_type;
        using Machine = detail::AtomicMachine<Word>;
        static_assert( Machine::always_lock_free, "Atomic requires lock-free compiler operations" );

        template <typename U>
        using ArithmeticResult = typename detail::AtomicEnableIf<
            detail::AtomicSameType<T, U>::value && detail::AtomicArithmetic<U>::enabled, T>::type;
        template <typename U>
        using IntegralResult = typename detail::AtomicEnableIf<
            detail::AtomicSameType<T, U>::value && detail::AtomicArithmetic<U>::integral, T>::type;

    public:
        using value_type = T;
        using difference_type = typename detail::AtomicArithmetic<T>::difference_type;

        Atomic() noexcept : Atomic( T{} )
        {
        }

        Atomic( T value ) noexcept : m_value( toWord( value ) )
        {
        }

        Atomic( const Atomic &other ) noexcept : Atomic( other.load() )
        {
        }

        Atomic &operator=( const Atomic &other ) noexcept
        {
            store( other.load() );
            return *this;
        }

        T operator=( T value ) volatile noexcept
        {
            store( value, memory_semantics::full_fence );
            return value;
        }

        T operator=( T value ) noexcept
        {
            store( value, memory_semantics::full_fence );
            return value;
        }

        operator T() const volatile noexcept
        {
            return load( memory_semantics::full_fence );
        }

        bool is_lock_free() const volatile noexcept
        {
            return true;
        }

        template <memory_semantics M = memory_semantics::acquire>
        T load() const volatile noexcept
        {
            static_assert( detail::atomicValidOrder( M ) && M != memory_semantics::release &&
                               M != memory_semantics::acq_rel,
                           "Invalid atomic load ordering" );
            return load( M );
        }

        T load( memory_semantics order ) const volatile noexcept
        {
            assert( detail::atomicValidOrder( order ) && order != memory_semantics::release &&
                    order != memory_semantics::acq_rel );
            return fromWord( Machine::load( &m_value, order ) );
        }

        template <memory_semantics M = memory_semantics::release>
        void store( T value ) volatile noexcept
        {
            static_assert( detail::atomicValidOrder( M ) && M != memory_semantics::acquire &&
                               M != memory_semantics::acq_rel,
                           "Invalid atomic store ordering" );
            store( value, M );
        }

        void store( T value, memory_semantics order ) volatile noexcept
        {
            assert( detail::atomicValidOrder( order ) && order != memory_semantics::acquire &&
                    order != memory_semantics::acq_rel );
            Machine::store( &m_value, toWord( value ), order );
        }

        T exchange( T value, memory_semantics order = memory_semantics::full_fence ) volatile noexcept
        {
            assert( detail::atomicValidOrder( order ) );
            return fromWord( Machine::exchange( &m_value, toWord( value ), order ) );
        }

        bool compare_exchange_strong( T &expected, T desired, memory_semantics success,
                                      memory_semantics failure ) volatile noexcept
        {
            detail::atomicCheckCompareOrder( success, failure );
            Word comparand = toWord( expected );
            Word observed =
                Machine::compare_and_swap( &m_value, toWord( desired ), comparand, success, failure );
            if( observed == comparand )
            {
                return true;
            }
            expected = fromWord( observed );
            return false;
        }

        bool compare_exchange_strong(
            T &expected, T desired,
            memory_semantics order = memory_semantics::full_fence ) volatile noexcept
        {
            return compare_exchange_strong( expected, desired, order,
                                            detail::atomicFailureOrder( order ) );
        }

        // A strong implementation also satisfies the weak compare-exchange contract.
        bool compare_exchange_weak( T &expected, T desired, memory_semantics success,
                                    memory_semantics failure ) volatile noexcept
        {
            return compare_exchange_strong( expected, desired, success, failure );
        }

        bool compare_exchange_weak(
            T &expected, T desired,
            memory_semantics order = memory_semantics::full_fence ) volatile noexcept
        {
            return compare_exchange_strong( expected, desired, order );
        }

        template <memory_semantics M = memory_semantics::full_fence>
        T fetch_and_store( T value ) volatile noexcept
        {
            static_assert( detail::atomicValidOrder( M ), "Invalid atomic memory ordering" );
            return exchange( value, M );
        }

        /** Stores value if equal to comparand; returns the old value in either case. */
        template <memory_semantics M = memory_semantics::full_fence>
        T compare_and_swap( T value, T comparand ) volatile noexcept
        {
            static_assert( detail::atomicValidOrder( M ), "Invalid atomic memory ordering" );
            compare_exchange_strong( comparand, value, M );
            return comparand;
        }

        template <typename U = T>
        ArithmeticResult<U> fetch_add(
            difference_type addend,
            memory_semantics order = memory_semantics::full_fence ) volatile noexcept
        {
            return fetchUpdate( offset( addend ), Update::Add, order );
        }

        template <typename U = T>
        ArithmeticResult<U> fetch_sub(
            difference_type subtrahend,
            memory_semantics order = memory_semantics::full_fence ) volatile noexcept
        {
            return fetchUpdate( offset( subtrahend ), Update::Subtract, order );
        }

        template <typename U = T>
        IntegralResult<U> fetch_and(
            T bits, memory_semantics order = memory_semantics::full_fence ) volatile noexcept
        {
            return fetchUpdate( static_cast<Unsigned>( toWord( bits ) ), Update::And, order );
        }

        template <typename U = T>
        IntegralResult<U> fetch_or(
            T bits, memory_semantics order = memory_semantics::full_fence ) volatile noexcept
        {
            return fetchUpdate( static_cast<Unsigned>( toWord( bits ) ), Update::Or, order );
        }

        template <typename U = T>
        IntegralResult<U> fetch_xor(
            T bits, memory_semantics order = memory_semantics::full_fence ) volatile noexcept
        {
            return fetchUpdate( static_cast<Unsigned>( toWord( bits ) ), Update::Xor, order );
        }

        template <memory_semantics M = memory_semantics::full_fence, typename U = T>
        ArithmeticResult<U> fetch_and_add( difference_type addend ) volatile noexcept
        {
            static_assert( detail::atomicValidOrder( M ), "Invalid atomic memory ordering" );
            return fetch_add( addend, M );
        }

        template <memory_semantics M = memory_semantics::full_fence, typename U = T>
        ArithmeticResult<U> fetch_and_increment() volatile noexcept
        {
            static_assert( detail::atomicValidOrder( M ), "Invalid atomic memory ordering" );
            return fetch_add( 1, M );
        }

        template <memory_semantics M = memory_semantics::full_fence, typename U = T>
        ArithmeticResult<U> fetch_and_decrement() volatile noexcept
        {
            static_assert( detail::atomicValidOrder( M ), "Invalid atomic memory ordering" );
            return fetch_sub( 1, M );
        }

        template <typename U = T>
        ArithmeticResult<U> operator+=( difference_type value ) volatile noexcept
        {
            return adjusted( fetch_add( value ), offset( value ), false );
        }

        template <typename U = T>
        ArithmeticResult<U> operator-=( difference_type value ) volatile noexcept
        {
            return adjusted( fetch_sub( value ), offset( value ), true );
        }

        template <typename U = T>
        ArithmeticResult<U> operator++() volatile noexcept
        {
            return operator+=( 1 );
        }

        template <typename U = T>
        ArithmeticResult<U> operator--() volatile noexcept
        {
            return operator-=( 1 );
        }

        template <typename U = T>
        ArithmeticResult<U> operator++( int ) volatile noexcept
        {
            return fetch_add( 1 );
        }

        template <typename U = T>
        ArithmeticResult<U> operator--( int ) volatile noexcept
        {
            return fetch_sub( 1 );
        }

        template <typename U = T>
        IntegralResult<U> operator&=( T value ) volatile noexcept
        {
            return fromUnsigned( static_cast<Unsigned>( toWord( fetch_and( value ) ) ) &
                                 static_cast<Unsigned>( toWord( value ) ) );
        }

        template <typename U = T>
        IntegralResult<U> operator|=( T value ) volatile noexcept
        {
            return fromUnsigned( static_cast<Unsigned>( toWord( fetch_or( value ) ) ) |
                                 static_cast<Unsigned>( toWord( value ) ) );
        }

        template <typename U = T>
        IntegralResult<U> operator^=( T value ) volatile noexcept
        {
            return fromUnsigned( static_cast<Unsigned>( toWord( fetch_xor( value ) ) ) ^
                                 static_cast<Unsigned>( toWord( value ) ) );
        }

        template <typename U = T>
        typename detail::AtomicEnableIf<
            detail::AtomicSameType<T, U>::value && detail::AtomicArithmetic<U>::pointer, T>::type
        operator->() const volatile noexcept
        {
            return load();
        }

    private:
        enum class Update
        {
            Add,
            Subtract,
            And,
            Or,
            Xor
        };

        static Word toWord( T value ) noexcept
        {
            Word word;
            memcpy( &word, &value, sizeof( word ) );
            return word;
        }

        static T fromWord( Word word ) noexcept
        {
            T value{};
            memcpy( &value, &word, sizeof( value ) );
            return value;
        }

        static T fromUnsigned( Unsigned bits ) noexcept
        {
            Word word;
            memcpy( &word, &bits, sizeof( word ) );
            return fromWord( word );
        }

        static Unsigned offset( difference_type value ) noexcept
        {
            return static_cast<Unsigned>( detail::AtomicArithmetic<T>::offset( value ) );
        }

        static T adjusted( T oldValue, Unsigned delta, bool subtract ) noexcept
        {
            Unsigned oldBits = static_cast<Unsigned>( toWord( oldValue ) );
            return fromUnsigned( static_cast<Unsigned>( subtract ? oldBits - delta : oldBits + delta ) );
        }

        T fetchUpdate( Unsigned operand, Update operation, memory_semantics order ) volatile noexcept
        {
            assert( detail::atomicValidOrder( order ) );
            Word expected = Machine::load( &m_value, memory_semantics::relaxed );
            for( ;; )
            {
                Unsigned bits = static_cast<Unsigned>( expected );
                Unsigned desired = bits;
                switch( operation )
                {
                case Update::Add:
                    desired = static_cast<Unsigned>( bits + operand );
                    break;
                case Update::Subtract:
                    desired = static_cast<Unsigned>( bits - operand );
                    break;
                case Update::And:
                    desired = static_cast<Unsigned>( bits & operand );
                    break;
                case Update::Or:
                    desired = static_cast<Unsigned>( bits | operand );
                    break;
                case Update::Xor:
                    desired = static_cast<Unsigned>( bits ^ operand );
                    break;
                }
                Word desiredWord;
                memcpy( &desiredWord, &desired, sizeof( desiredWord ) );
                Word observed = Machine::compare_and_swap( &m_value, desiredWord, expected, order,
                                                           memory_semantics::relaxed );
                if( observed == expected )
                {
                    return fromWord( observed );
                }
                expected = observed;
            }
        }

        // Mutable permits a CAS-based MSVC load even through a const Atomic object.
        alignas( sizeof( T ) ) mutable Word m_value;
    };

    template <memory_semantics M, typename T>
    T load( const volatile Atomic<T> &value ) noexcept
    {
        return value.template load<M>();
    }

    template <memory_semantics M, typename T>
    void store( volatile Atomic<T> &target, T value ) noexcept
    {
        target.template store<M>( value );
    }

    template <typename T>
    Atomic<T> make_atomic( T value ) noexcept
    {
        return Atomic<T>( value );
    }
}  // namespace workphone

#endif  // Atomic_h__
