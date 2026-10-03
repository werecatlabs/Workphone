#ifndef Any_h__
#define Any_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace workphone
{

    class bad_any_cast : public std::bad_cast
    {
    public:
        const char *what() const noexcept override
        {
            return "bad any_cast";
        }
    };

    class Any
    {
    public:
        Any() noexcept = default;

        Any( const Any &other ) : m_holder( other.m_holder ? other.m_holder->clone() : nullptr )
        {
        }

        Any( Any &&other ) noexcept : m_holder( other.m_holder )
        {
            other.m_holder = nullptr;
        }

        template <typename T, typename = std::enable_if_t<!std::is_same<std::decay_t<T>, Any>::value>>
        Any( T &&value ) : m_holder( new Holder<std::decay_t<T>>( std::forward<T>( value ) ) )
        {
        }

        ~Any()
        {
            reset();
        }

        Any &operator=( const Any &other )
        {
            Any( other ).swap( *this );
            return *this;
        }

        Any &operator=( Any &&other ) noexcept
        {
            Any( std::move( other ) ).swap( *this );
            return *this;
        }

        template <typename T, typename = std::enable_if_t<!std::is_same<std::decay_t<T>, Any>::value>>
        Any &operator=( T &&value )
        {
            Any( std::forward<T>( value ) ).swap( *this );
            return *this;
        }

        template <typename T, typename... Args>
        std::decay_t<T> &emplace( Args &&...args )
        {
            reset();
            auto *holder = new Holder<std::decay_t<T>>( std::forward<Args>( args )... );
            m_holder = holder;
            return holder->m_value;
        }

        void reset() noexcept
        {
            delete m_holder;
            m_holder = nullptr;
        }

        void swap( Any &other ) noexcept
        {
            HolderBase *tmp = m_holder;
            m_holder = other.m_holder;
            other.m_holder = tmp;
        }

        bool has_value() const noexcept
        {
            return m_holder != nullptr;
        }

        const std::type_info &type() const noexcept
        {
            return m_holder ? m_holder->type() : typeid( void );
        }

    private:
        struct HolderBase
        {
            virtual ~HolderBase() = default;
            virtual HolderBase *clone() const = 0;
            virtual const std::type_info &type() const noexcept = 0;
        };

        template <typename T>
        struct Holder : HolderBase
        {
            template <typename... Args>
            explicit Holder( Args &&...args ) : m_value( std::forward<Args>( args )... )
            {
            }

            HolderBase *clone() const override
            {
                return new Holder<T>( m_value );
            }

            const std::type_info &type() const noexcept override
            {
                return typeid( T );
            }

            T m_value;
        };

        template <typename T>
        friend T *any_cast( Any *any ) noexcept;

        template <typename T>
        friend const T *any_cast( const Any *any ) noexcept;

        HolderBase *m_holder = nullptr;
    };

    template <typename T>
    T *any_cast( Any *any ) noexcept
    {
        if( any && any->m_holder && any->type() == typeid( T ) )
            return &static_cast<Any::Holder<T> *>( any->m_holder )->m_value;
        return nullptr;
    }

    template <typename T>
    const T *any_cast( const Any *any ) noexcept
    {
        if( any && any->m_holder && any->type() == typeid( T ) )
            return &static_cast<const Any::Holder<T> *>( any->m_holder )->m_value;
        return nullptr;
    }

    template <typename T>
    T any_cast( Any &any )
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        auto *ptr = any_cast<U>( &any );
        if( !ptr )
            throw bad_any_cast{};
        return static_cast<T>( *ptr );
    }

    template <typename T>
    T any_cast( const Any &any )
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        const auto *ptr = any_cast<U>( &any );
        if( !ptr )
            throw bad_any_cast{};
        return static_cast<T>( *ptr );
    }

    template <typename T>
    T any_cast( Any &&any )
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        auto *ptr = any_cast<U>( &any );
        if( !ptr )
            throw bad_any_cast{};
        return static_cast<T>( std::move( *ptr ) );
    }

}  // namespace workphone

#endif  // Any_h__
