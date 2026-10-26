#ifndef WP_REFLECTION_STORAGE_HPP
#define WP_REFLECTION_STORAGE_HPP

#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringBase.hpp>
#include <any>
#include <array>
#include <functional>
#include <string>
#include <tuple>
#include <typeinfo>
#include <utility>
#include <vector>

namespace workphone::reflection
{
    // Canonical storage owns every reflected buffer through SourceCpu. Raw
    // pointers in trivial values/captures are borrowed, never ownership claims.
    // Additional owning types need an audited canonical storage specialization.
    template <class T>
    struct Storage
    {
        using type = T;
        static constexpr bool supported =
            std::is_trivially_copyable_v<T> && std::is_copy_constructible_v<T>;

        static type freeze( const T &value )
        {
            return value;
        }

        static type freeze( T &&value )
        {
            return std::move( value );
        }
    };
    template <class T>
    using Stored = typename Storage<std::decay_t<T>>::type;

    template <class C, class A>
    struct Storage<std::basic_string<C, std::char_traits<C>, A>>
    {
        using input = std::basic_string<C, std::char_traits<C>, A>;
        using type = std::basic_string<C, std::char_traits<C>, SourceBudgetAllocator<C>>;
        static constexpr bool supported = true;

        static type freeze( const input &value )
        {
            type result;
            result.reserve( value.capacity() );
            result.assign( value.data(), value.size() );
            return result;
        }

        static type freeze( input &&value )
        {
            if constexpr( std::is_same_v<input, type> )
            {
                return std::move( value );
            }
            else
            {
                return freeze( static_cast<const input &>( value ) );
            }
        }
    };

    template <class C, class Traits>
    struct Storage<StringBase<C, Traits, std::allocator<C>>>
    {
        using type = StringBase<C, Traits, std::allocator<C>>;
        static constexpr bool supported = true;  // Standard CharacterPoolAllocator policy.

        static type freeze( const type &value )
        {
            return value;
        }

        static type freeze( type &&value )
        {
            return std::move( value );
        }
    };

    template <class T, class A>
    struct Storage<std::vector<T, A>>
    {
        using input = std::vector<T, A>;
        using type = std::vector<Stored<T>, SourceBudgetAllocator<Stored<T>>>;
        static constexpr bool supported = Storage<T>::supported;

        static type freeze( const input &value )
        {
            type result;
            result.reserve( value.capacity() );
            for( const auto &element : value )
            {
                result.emplace_back( Storage<T>::freeze( element ) );
            }
            return result;
        }

        static type freeze( input &&value )
        {
            if constexpr( std::is_same_v<input, type> )
            {
                return std::move( value );
            }
            else
            {
                return freeze( static_cast<const input &>( value ) );
            }
        }
    };

    template <class T, class A>
    struct Storage<ArrayBase<T, A>>
    {
        using input = ArrayBase<T, A>;
        using type = Array<Stored<T>, SourceBudgetAllocator<Stored<T>>>;
        static constexpr bool supported = Storage<T>::supported;

        static type freeze( const input &value )
        {
            type result;
            result.reserve( value.capacity() );
            for( const auto &element : value )
            {
                result.emplace_back( Storage<T>::freeze( element ) );
            }
            return result;
        }

        static type freeze( input &&value )
        {
            if constexpr( std::is_same_v<input, type> )
            {
                return std::move( value );
            }
            else
            {
                return freeze( static_cast<const input &>( value ) );
            }
        }
    };

    template <class T, size_t N>
    struct Storage<std::array<T, N>>
    {
        using type = std::array<Stored<T>, N>;
        static constexpr bool supported = Storage<T>::supported;

        template <size_t... I>
        static type freezeAt( const std::array<T, N> &value, std::index_sequence<I...> )
        {
            return { { Storage<T>::freeze( value[I] )... } };
        }

        static type freeze( const std::array<T, N> &value )
        {
            return freezeAt( value, std::make_index_sequence<N>{} );
        }
    };

    namespace detail
    {
        template <class T, class... Args>
        T *create( Args &&...args )
        {
            SourceBudgetAllocator<T> allocator;
            T *value = allocator.allocate( 1 );
            try
            {
                ::new( static_cast<void *>( value ) ) T( std::forward<Args>( args )... );
            }
            catch( ... )
            {
                allocator.deallocate( value, 1 );
                throw;
            }
            return value;
        }

        template <class T>
        void destroy( void *pointer ) noexcept
        {
            auto value = static_cast<T *>( pointer );
            value->~T();
            SourceBudgetAllocator<T>{}.deallocate( value, 1 );
        }

        template <class T, class Input>
        T *freeze( Input &&input )
        {
            SourceBudgetAllocator<T> allocator;
            T *value = allocator.allocate( 1 );
            try
            {
                ::new( static_cast<void *>( value ) )
                    T( Storage<std::decay_t<Input>>::freeze( std::forward<Input>( input ) ) );
            }
            catch( ... )
            {
                allocator.deallocate( value, 1 );
                throw;
            }
            return value;
        }
    }  // namespace detail

    class Any;

    template <>
    struct Storage<Any>
    {
        using type = Any;
        static constexpr bool supported = true;
        static type freeze( const Any &value );
        static type freeze( Any &&value );
    };

    // std::any has no allocator hook. Reflection keeps its value in an admitted
    // typed allocation instead. Copies are independent; moves transfer storage.
    class Any
    {
        struct Operations
        {
            const std::type_info &( *type )() noexcept;
            void *( *copy )( const void * );
            void ( *destroy )( void * ) noexcept;
        };

        template <class T>
        static const Operations *operations()
        {
            static const Operations value{ []() noexcept -> const std::type_info & {
                                              return typeid( T );
                                          },
                                           []( const void *p ) -> void * {
                                               return detail::freeze<T>( *static_cast<const T *>( p ) );
                                           },
                                           &detail::destroy<T> };
            return &value;
        }

        void *m_value = nullptr;
        const Operations *m_operations = nullptr;

    public:
        Any() noexcept = default;

        Any( const Any &other )
        {
            if( other.m_operations )
            {
                m_value = other.m_operations->copy( other.m_value );
                m_operations = other.m_operations;
            }
        }

        Any( Any &&other ) noexcept
        {
            swap( other );
        }

        template <class T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, Any> &&
                                                Storage<std::decay_t<T>>::supported,
                                            int> = 0>
        Any( T &&input )
        {
            using TStored = Stored<T>;
            m_value = detail::freeze<TStored>( std::forward<T>( input ) );
            m_operations = operations<TStored>();
        }

        ~Any()
        {
            reset();
        }

        Any &operator=( const Any &other )
        {
            if( this != &other )
            {
                Any next( other );
                swap( next );
            }
            return *this;
        }

        Any &operator=( Any &&other ) noexcept
        {
            if( this != &other )
            {
                reset();
                swap( other );
            }
            return *this;
        }

        template <class T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, Any> &&
                                                Storage<std::decay_t<T>>::supported,
                                            int> = 0>
        Any &operator=( T &&value )
        {
            Any next( std::forward<T>( value ) );
            swap( next );
            return *this;
        }

        void swap( Any &other ) noexcept
        {
            std::swap( m_value, other.m_value );
            std::swap( m_operations, other.m_operations );
        }

        void reset() noexcept
        {
            auto pointer = std::exchange( m_value, nullptr );
            auto operations = std::exchange( m_operations, nullptr );
            if( operations )
            {
                operations->destroy( pointer );
            }
        }

        bool has_value() const noexcept
        {
            return m_operations != nullptr;
        }

        const std::type_info &type() const noexcept
        {
            return m_operations ? m_operations->type() : typeid( void );
        }

        template <class T>
        T *get() noexcept
        {
            return type() == typeid( T ) ? static_cast<T *>( m_value ) : nullptr;
        }

        template <class T>
        const T *get() const noexcept
        {
            return type() == typeid( T ) ? static_cast<const T *>( m_value ) : nullptr;
        }

        template <class T, class... Args>
        T &emplace( Args &&...args )
        {
            static_assert(
                Storage<T>::supported && std::is_same_v<Stored<T>, T>,
                "Emplace a budget-native type; standard containers are imported through Any(value)." );
            Any next;
            next.m_value = detail::create<T>( std::forward<Args>( args )... );
            next.m_operations = operations<T>();
            swap( next );
            return *static_cast<T *>( m_value );
        }

        template <class T>
        static Any fromStdAny( const std::any &value )
        {
            static_assert( Storage<T>::supported,
                           "Unsupported owning std::any payload needs a canonical storage adapter." );
            return Any( std::any_cast<const T &>( value ) );
        }

        template <class T>
        static Any fromStdAny( std::any &&value )
        {
            Any next = fromStdAny<T>( static_cast<const std::any &>( value ) );
            value.reset();  // Consume only after complete admission/copy.
            return next;
        }
    };

    inline Any Storage<Any>::freeze( const Any &value )
    {
        return value;
    }

    inline Any Storage<Any>::freeze( Any &&value )
    {
        return std::move( value );
    }

    template <class T>
    T *any_cast( Any *value ) noexcept
    {
        return value ? value->template get<T>() : nullptr;
    }

    template <class T>
    const T *any_cast( const Any *value ) noexcept
    {
        return value ? value->template get<T>() : nullptr;
    }

    template <class T>
    T any_cast( Any &value )
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        auto pointer = value.template get<U>();
        if( !pointer )
        {
            throw std::bad_any_cast();
        }
        return static_cast<T>( *pointer );
    }

    template <class T>
    T any_cast( const Any &value )
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        auto pointer = value.template get<U>();
        if( !pointer )
        {
            throw std::bad_any_cast();
        }
        return static_cast<T>( *pointer );
    }

    template <class T>
    T any_cast( Any &&value )
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        auto pointer = value.template get<U>();
        if( !pointer )
        {
            throw std::bad_any_cast();
        }
        return static_cast<T>( std::move( *pointer ) );
    }

    using Arguments = std::vector<Any, SourceBudgetAllocator<Any>>;

    // Explicit export to a caller-owned field. Its allocation policy belongs to
    // that object; reflected values and callback captures stay budget-native.
    namespace detail
    {
        template <class T>
        struct External
        {
            static T copy( const Stored<T> &value )
            {
                return value;
            }
        };

        template <class C, class A>
        struct External<std::basic_string<C, std::char_traits<C>, A>>
        {
            using T = std::basic_string<C, std::char_traits<C>, A>;

            static T copy( const Stored<T> &value )
            {
                return T( value.data(), value.size() );
            }
        };

        template <class T, class A>
        struct External<std::vector<T, A>>
        {
            using V = std::vector<T, A>;

            static V copy( const Stored<V> &value )
            {
                V result;
                result.reserve( value.size() );
                for( const auto &element : value )
                {
                    result.emplace_back( External<T>::copy( element ) );
                }
                return result;
            }
        };

        template <class T, class A>
        struct External<ArrayBase<T, A>>
        {
            using V = ArrayBase<T, A>;

            static V copy( const Stored<V> &value )
            {
                V result;
                result.reserve( value.size() );
                for( const auto &element : value )
                {
                    result.emplace_back( External<T>::copy( element ) );
                }
                return result;
            }
        };

        template <class T, size_t N>
        struct External<std::array<T, N>>
        {
            template <size_t... I>
            static std::array<T, N> copyAt( const Stored<std::array<T, N>> &value,
                                            std::index_sequence<I...> )
            {
                return { { External<T>::copy( value[I] )... } };
            }

            static std::array<T, N> copy( const Stored<std::array<T, N>> &value )
            {
                return copyAt( value, std::make_index_sequence<N>{} );
            }
        };
    }  // namespace detail

    template <class T>
    T copyToExternal( const Any &value )
    {
        return detail::External<T>::copy( any_cast<const Stored<T> &>( value ) );
    }

    template <class Signature>
    class Callback;

    template <class R, class... Args>
    class Callback<R( Args... )>
    {
        struct Operations
        {
            R ( *invoke )( void *, Args &&... );
            void *( *copy )( const void * );
            void ( *destroy )( void * ) noexcept;
        };

        template <class F>
        static const Operations *operations()
        {
            static const Operations value{ []( void *p, Args &&...args ) -> R {
                                              return std::invoke( *static_cast<F *>( p ),
                                                                  std::forward<Args>( args )... );
                                          },
                                           []( const void *p ) -> void * {
                                               return detail::create<F>( *static_cast<const F *>( p ) );
                                           },
                                           &detail::destroy<F> };
            return &value;
        }

        template <class F, class... Captures>
        struct Bound
        {
            F function;
            std::tuple<Stored<Captures>...> captures;

            Bound( F fn, const Captures &...values ) :
                function( std::move( fn ) ),
                captures( Storage<Captures>::freeze( values )... )
            {
            }

            R operator()( Args... args )
            {
                return std::apply(
                    [&]( auto &...values ) -> R {
                        return std::invoke( function, values..., std::forward<Args>( args )... );
                    },
                    captures );
            }
        };

        void *m_function = nullptr;
        const Operations *m_operations = nullptr;

    public:
        Callback() noexcept = default;

        Callback( std::nullptr_t ) noexcept
        {
        }

        Callback( const Callback &other )
        {
            if( other.m_operations )
            {
                m_function = other.m_operations->copy( other.m_function );
                m_operations = other.m_operations;
            }
        }

        Callback( Callback &&other ) noexcept
        {
            swap( other );
        }

        template <class F, std::enable_if_t<std::is_trivially_copyable_v<std::decay_t<F>> &&
                                                std::is_copy_constructible_v<std::decay_t<F>> &&
                                                std::is_invocable_r_v<R, std::decay_t<F> &, Args...>,
                                            int> = 0>
        Callback( F &&function )
        {
            using Fn = std::decay_t<F>;
            if constexpr( std::is_pointer_v<Fn> || std::is_member_pointer_v<Fn> )
            {
                if( !function )
                {
                    return;
                }
            }
            m_function = detail::create<Fn>( std::forward<F>( function ) );
            m_operations = operations<Fn>();
        }

        ~Callback()
        {
            reset();
        }

        Callback &operator=( const Callback &other )
        {
            if( this != &other )
            {
                Callback next( other );
                swap( next );
            }
            return *this;
        }

        Callback &operator=( Callback &&other ) noexcept
        {
            if( this != &other )
            {
                reset();
                swap( other );
            }
            return *this;
        }

        Callback &operator=( std::nullptr_t ) noexcept
        {
            reset();
            return *this;
        }

        template <class F, std::enable_if_t<std::is_trivially_copyable_v<std::decay_t<F>> &&
                                                std::is_invocable_r_v<R, std::decay_t<F> &, Args...>,
                                            int> = 0>
        Callback &operator=( F &&function )
        {
            Callback next( std::forward<F>( function ) );
            swap( next );
            return *this;
        }

        template <class F, class... Captures>
        static Callback bind( F function, const Captures &...captures )
        {
            static_assert( std::is_trivially_copyable_v<F> && std::is_copy_constructible_v<F>,
                           "Bind a trivial callable and pass owning captures as separate arguments." );
            static_assert( ( Storage<Captures>::supported && ... ),
                           "Unsupported owning capture needs a canonical storage adapter." );
            using State = Bound<F, Captures...>;
            Callback result;
            result.m_function = detail::create<State>( std::move( function ), captures... );
            result.m_operations = operations<State>();
            return result;
        }

        explicit operator bool() const noexcept
        {
            return m_operations != nullptr;
        }

        R operator()( Args... args ) const
        {
            if( !m_operations )
            {
                throw std::bad_function_call();
            }
            return m_operations->invoke( m_function, std::forward<Args>( args )... );
        }

        void reset() noexcept
        {
            auto function = std::exchange( m_function, nullptr );
            auto operations = std::exchange( m_operations, nullptr );
            if( operations )
            {
                operations->destroy( function );
            }
        }

        void swap( Callback &other ) noexcept
        {
            std::swap( m_function, other.m_function );
            std::swap( m_operations, other.m_operations );
        }
    };
}  // namespace workphone::reflection
#endif
