#ifndef FixedStringBase_h__
#define FixedStringBase_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <cstddef>
#include <string>

namespace workphone
{
    template <class T, class Traits, class Allocator>
    class StringBase;

    template <std::size_t N, class T = char, class Traits = std::char_traits<T>>
    class FixedStringBase
    {
    public:
        // Type definitions
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;
        using iterator = T *;
        using const_iterator = const T *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using traits_type = Traits;

        // Constants
        static constexpr size_type npos = size_type( -1 );
        static constexpr size_type max_size_value = N;

        // Constructors
        constexpr FixedStringBase() noexcept : m_size( 0 )
        {
            m_data[0] = T{};
        }

        constexpr FixedStringBase( const T *s )
        {
            assign( s );
        }

        constexpr FixedStringBase( const T *s, size_type count )
        {
            assign( s, count );
        }

        constexpr FixedStringBase( size_type count, T ch )
        {
            assign( count, ch );
        }

        template <class InputIt>
        constexpr FixedStringBase( InputIt first, InputIt last )
        {
            assign( first, last );
        }

        constexpr FixedStringBase( const std::basic_string<T, Traits> &str )
        {
            assign( str );
        }

        template <class StringTraits, class Allocator>
        constexpr FixedStringBase( const StringBase<T, StringTraits, Allocator> &str )
        {
            assign( str.data(), str.size() );
        }

        constexpr FixedStringBase( const FixedStringBase &other ) = default;
        constexpr FixedStringBase &operator=( const FixedStringBase &other ) = default;

        // Destructor
        ~FixedStringBase() = default;

        // Assignment operators
        constexpr FixedStringBase &operator=( const T *s )
        {
            return assign( s );
        }

        constexpr FixedStringBase &operator=( T ch )
        {
            clear();
            push_back( ch );
            return *this;
        }

        constexpr FixedStringBase &operator=( const std::basic_string<T, Traits> &str )
        {
            return assign( str );
        }

        template <class StringTraits, class Allocator>
        constexpr FixedStringBase &operator=( const StringBase<T, StringTraits, Allocator> &str )
        {
            return assign( str.data(), str.size() );
        }

        // Assignment methods
        constexpr FixedStringBase &assign( size_type count, T ch )
        {
            WP_ASSERT( count < N );

            if( count > N )
                throw_length_error();

            m_size = count;
            Traits::assign( m_data, count, ch );
            m_data[m_size] = T{};
            return *this;
        }

        constexpr FixedStringBase &assign( const T *s )
        {
            return assign( s, Traits::length( s ) );
        }

        constexpr FixedStringBase &assign( const T *s, size_type count )
        {
            WP_ASSERT( count < N );

            if( count > N )
                throw_length_error();

            m_size = count;
            Traits::copy( m_data, s, count );
            m_data[m_size] = T{};
            return *this;
        }

        constexpr FixedStringBase &assign( const std::basic_string<T, Traits> &str )
        {
            return assign( str.data(), str.size() );
        }

        template <class InputIt>
        constexpr FixedStringBase &assign( InputIt first, InputIt last )
        {
            clear();

            for( auto it = first; it != last; ++it )
            {
                push_back( *it );
            }

            return *this;
        }

        // Element access
        constexpr reference at( size_type pos )
        {
            if( pos >= m_size )
                throw_out_of_range();

            return m_data[pos];
        }

        constexpr const_reference at( size_type pos ) const
        {
            if( pos >= m_size )
                throw_out_of_range();
            return m_data[pos];
        }

        constexpr reference operator[]( size_type pos )
        {
            return m_data[pos];
        }

        constexpr const_reference operator[]( size_type pos ) const
        {
            return m_data[pos];
        }

        constexpr reference front()
        {
            return m_data[0];
        }

        constexpr const_reference front() const
        {
            return m_data[0];
        }

        constexpr reference back()
        {
            return m_data[m_size - 1];
        }

        constexpr const_reference back() const
        {
            return m_data[m_size - 1];
        }

        constexpr const_pointer data() const noexcept
        {
            return m_data;
        }

        constexpr pointer data() noexcept
        {
            return m_data;
        }

        constexpr const_pointer c_str() const noexcept
        {
            return m_data;
        }

        // Iterators
        constexpr iterator begin() noexcept
        {
            return m_data;
        }

        constexpr const_iterator begin() const noexcept
        {
            return m_data;
        }

        constexpr const_iterator cbegin() const noexcept
        {
            return m_data;
        }

        constexpr iterator end() noexcept
        {
            return m_data + m_size;
        }

        constexpr const_iterator end() const noexcept
        {
            return m_data + m_size;
        }

        constexpr const_iterator cend() const noexcept
        {
            return m_data + m_size;
        }

        constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator( end() );
        }

        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator( end() );
        }

        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator( end() );
        }

        constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator( begin() );
        }

        constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator( begin() );
        }

        constexpr const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator( begin() );
        }

        // Capacity
        constexpr bool empty() const noexcept
        {
            return m_size == 0;
        }

        constexpr size_type size() const noexcept
        {
            return m_size;
        }

        constexpr size_type length() const noexcept
        {
            return m_size;
        }

        constexpr size_type max_size() const noexcept
        {
            return N;
        }

        constexpr size_type capacity() const noexcept
        {
            return N;
        }

        // Modifiers
        constexpr void clear() noexcept
        {
            m_size = 0;
            m_data[0] = T{};
        }

        constexpr FixedStringBase &append( size_type count, T ch )
        {
            WP_ASSERT( count < N );

            if( m_size + count > N )
                throw_length_error();

            Traits::assign( m_data + m_size, count, ch );
            m_size += count;
            m_data[m_size] = T{};
            return *this;
        }

        constexpr FixedStringBase &append( const T *s )
        {
            return append( s, Traits::length( s ) );
        }

        constexpr FixedStringBase &append( const T *s, size_type count )
        {
            WP_ASSERT( count < N );

            if( m_size + count > N )
                throw_length_error();

            Traits::copy( m_data + m_size, s, count );
            m_size += count;
            m_data[m_size] = T{};
            return *this;
        }

        constexpr FixedStringBase &append( const std::basic_string<T, Traits> &str )
        {
            return append( str.data(), str.size() );
        }

        constexpr FixedStringBase &append( const FixedStringBase &str )
        {
            return append( str.data(), str.size() );
        }

        constexpr FixedStringBase &operator+=( T ch )
        {
            push_back( ch );
            return *this;
        }

        constexpr FixedStringBase &operator+=( const T *s )
        {
            return append( s );
        }

        constexpr FixedStringBase &operator+=( const std::basic_string<T, Traits> &str )
        {
            return append( str );
        }

        constexpr FixedStringBase &operator+=( const FixedStringBase &str )
        {
            return append( str );
        }

        constexpr void push_back( T ch )
        {
            WP_ASSERT( m_size < N );

            if( m_size >= N )
                throw_length_error();

            m_data[m_size] = ch;
            ++m_size;
            m_data[m_size] = T{};
        }

        constexpr void pop_back()
        {
            if( m_size > 0 )
            {
                --m_size;
                m_data[m_size] = T{};
            }
        }

        constexpr void resize( size_type count )
        {
            resize( count, T{} );
        }

        constexpr void resize( size_type count, T ch )
        {
            WP_ASSERT( count < N );

            if( count > N )
                throw_length_error();

            if( count > m_size )
            {
                Traits::assign( m_data + m_size, count - m_size, ch );
            }

            m_size = count;
            m_data[m_size] = T{};
        }

        // String operations
        constexpr int compare( const FixedStringBase &str ) const noexcept
        {
            const size_type len = std::min( m_size, str.m_size );
            int result = Traits::compare( m_data, str.m_data, len );
            if( result == 0 )
            {
                if( m_size < str.m_size )
                    return -1;
                if( m_size > str.m_size )
                    return 1;
            }

            return result;
        }

        constexpr int compare( const T *s ) const
        {
            return compare( FixedStringBase( s ) );
        }

        constexpr int compare( const std::basic_string<T, Traits> &str ) const
        {
            return compare( FixedStringBase( str ) );
        }

        // Conversion
        constexpr operator std::basic_string<T, Traits>() const
        {
            return std::basic_string<T, Traits>( m_data, m_size );
        }

        std::basic_string<T, Traits> str() const
        {
            return std::basic_string<T, Traits>( m_data, m_size );
        }

        // Find functions
        constexpr size_type find( const FixedStringBase &str, size_type pos = 0 ) const noexcept
        {
            return find( str.data(), pos, str.size() );
        }

        constexpr size_type find( const T *s, size_type pos, size_type count ) const noexcept
        {
            if( pos > m_size || count == 0 )
                return npos;

            if( count > m_size - pos )
                return npos;

            for( size_type i = pos; i <= m_size - count; ++i )
            {
                if( Traits::compare( m_data + i, s, count ) == 0 )
                    return i;
            }

            return npos;
        }

        constexpr size_type find( const T *s, size_type pos = 0 ) const noexcept
        {
            return find( s, pos, Traits::length( s ) );
        }

        constexpr size_type find( T ch, size_type pos = 0 ) const noexcept
        {
            if( pos >= m_size )
                return npos;

            const T *p = Traits::find( m_data + pos, m_size - pos, ch );
            return p ? static_cast<size_type>( p - m_data ) : npos;
        }

        constexpr size_type find( const std::basic_string<T, Traits> &str,
                                  size_type pos = 0 ) const noexcept
        {
            return find( str.data(), pos, str.size() );
        }

        // Reverse find functions
        constexpr size_type rfind( const FixedStringBase &str, size_type pos = npos ) const noexcept
        {
            return rfind( str.data(), pos, str.size() );
        }

        constexpr size_type rfind( const T *s, size_type pos, size_type count ) const noexcept
        {
            if( count == 0 )
                return pos > m_size ? m_size : pos;

            if( count > m_size )
                return npos;

            size_type start_pos = std::min( pos, m_size - count );
            for( size_type i = start_pos + 1; i > 0; --i )
            {
                if( Traits::compare( m_data + i - 1, s, count ) == 0 )
                    return i - 1;
            }
            return npos;
        }

        constexpr size_type rfind( const T *s, size_type pos = npos ) const noexcept
        {
            return rfind( s, pos, Traits::length( s ) );
        }

        constexpr size_type rfind( T ch, size_type pos = npos ) const noexcept
        {
            if( m_size == 0 )
                return npos;

            size_type start_pos = std::min( pos, m_size - 1 );
            for( size_type i = start_pos + 1; i > 0; --i )
            {
                if( Traits::eq( m_data[i - 1], ch ) )
                    return i - 1;
            }
            return npos;
        }

        constexpr size_type rfind( const std::basic_string<T, Traits> &str,
                                   size_type pos = npos ) const noexcept
        {
            return rfind( str.data(), pos, str.size() );
        }

        // Find first of functions
        constexpr size_type find_first_of( const FixedStringBase &str, size_type pos = 0 ) const noexcept
        {
            return find_first_of( str.data(), pos, str.size() );
        }

        constexpr size_type find_first_of( const T *s, size_type pos, size_type count ) const noexcept
        {
            for( size_type i = pos; i < m_size; ++i )
            {
                for( size_type j = 0; j < count; ++j )
                {
                    if( Traits::eq( m_data[i], s[j] ) )
                        return i;
                }
            }
            return npos;
        }

        constexpr size_type find_first_of( const T *s, size_type pos = 0 ) const noexcept
        {
            return find_first_of( s, pos, Traits::length( s ) );
        }

        constexpr size_type find_first_of( T ch, size_type pos = 0 ) const noexcept
        {
            return find( ch, pos );
        }

        constexpr size_type find_first_of( const std::basic_string<T, Traits> &str,
                                           size_type pos = 0 ) const noexcept
        {
            return find_first_of( str.data(), pos, str.size() );
        }

        // Find first not of functions
        constexpr size_type find_first_not_of( const FixedStringBase &str,
                                               size_type pos = 0 ) const noexcept
        {
            return find_first_not_of( str.data(), pos, str.size() );
        }

        constexpr size_type find_first_not_of( const T *s, size_type pos,
                                               size_type count ) const noexcept
        {
            for( size_type i = pos; i < m_size; ++i )
            {
                bool found = false;
                for( size_type j = 0; j < count; ++j )
                {
                    if( Traits::eq( m_data[i], s[j] ) )
                    {
                        found = true;
                        break;
                    }
                }
                if( !found )
                    return i;
            }
            return npos;
        }

        constexpr size_type find_first_not_of( const T *s, size_type pos = 0 ) const noexcept
        {
            return find_first_not_of( s, pos, Traits::length( s ) );
        }

        constexpr size_type find_first_not_of( T ch, size_type pos = 0 ) const noexcept
        {
            for( size_type i = pos; i < m_size; ++i )
            {
                if( !Traits::eq( m_data[i], ch ) )
                    return i;
            }
            return npos;
        }

        constexpr size_type find_first_not_of( const std::basic_string<T, Traits> &str,
                                               size_type pos = 0 ) const noexcept
        {
            return find_first_not_of( str.data(), pos, str.size() );
        }

        // Find last of functions
        constexpr size_type find_last_of( const FixedStringBase &str,
                                          size_type pos = npos ) const noexcept
        {
            return find_last_of( str.data(), pos, str.size() );
        }

        constexpr size_type find_last_of( const T *s, size_type pos, size_type count ) const noexcept
        {
            if( m_size == 0 || count == 0 )
                return npos;

            size_type start_pos = std::min( pos, m_size - 1 );
            for( size_type i = start_pos + 1; i > 0; --i )
            {
                for( size_type j = 0; j < count; ++j )
                {
                    if( Traits::eq( m_data[i - 1], s[j] ) )
                        return i - 1;
                }
            }
            return npos;
        }

        constexpr size_type find_last_of( const T *s, size_type pos = npos ) const noexcept
        {
            return find_last_of( s, pos, Traits::length( s ) );
        }

        constexpr size_type find_last_of( T ch, size_type pos = npos ) const noexcept
        {
            return rfind( ch, pos );
        }

        constexpr size_type find_last_of( const std::basic_string<T, Traits> &str,
                                          size_type pos = npos ) const noexcept
        {
            return find_last_of( str.data(), pos, str.size() );
        }

        // Find last not of functions
        constexpr size_type find_last_not_of( const FixedStringBase &str,
                                              size_type pos = npos ) const noexcept
        {
            return find_last_not_of( str.data(), pos, str.size() );
        }

        constexpr size_type find_last_not_of( const T *s, size_type pos, size_type count ) const noexcept
        {
            if( m_size == 0 )
                return npos;

            size_type start_pos = std::min( pos, m_size - 1 );
            for( size_type i = start_pos + 1; i > 0; --i )
            {
                bool found = false;
                for( size_type j = 0; j < count; ++j )
                {
                    if( Traits::eq( m_data[i - 1], s[j] ) )
                    {
                        found = true;
                        break;
                    }
                }
                if( !found )
                    return i - 1;
            }
            return npos;
        }

        constexpr size_type find_last_not_of( const T *s, size_type pos = npos ) const noexcept
        {
            return find_last_not_of( s, pos, Traits::length( s ) );
        }

        constexpr size_type find_last_not_of( T ch, size_type pos = npos ) const noexcept
        {
            if( m_size == 0 )
                return npos;

            size_type start_pos = std::min( pos, m_size - 1 );
            for( size_type i = start_pos + 1; i > 0; --i )
            {
                if( !Traits::eq( m_data[i - 1], ch ) )
                    return i - 1;
            }
            return npos;
        }

        constexpr size_type find_last_not_of( const std::basic_string<T, Traits> &str,
                                              size_type pos = npos ) const noexcept
        {
            return find_last_not_of( str.data(), pos, str.size() );
        }

        constexpr FixedStringBase substr( size_type pos = 0, size_type count = npos ) const
        {
            if( pos > m_size )
                throw_out_of_range();

            size_type actual_count = std::min( count, m_size - pos );
            return FixedStringBase( m_data + pos, actual_count );
        }

    private:
        T m_data[N + 1];  // +1 for null terminator
        size_type m_size;

        constexpr void throw_length_error() const
        {
            throw std::length_error( "FixedStringBase: length exceeds maximum size" );
        }

        constexpr void throw_out_of_range() const
        {
            throw std::out_of_range( "FixedStringBase: index out of range" );
        }
    };

    // Comparison operators
    template <std::size_t N, class T, class Traits>
    constexpr bool operator==( const FixedStringBase<N, T, Traits> &lhs,
                               const FixedStringBase<N, T, Traits> &rhs ) noexcept
    {
        return lhs.compare( rhs ) == 0;
    }

    template <std::size_t N, class T, class Traits>
    constexpr bool operator!=( const FixedStringBase<N, T, Traits> &lhs,
                               const FixedStringBase<N, T, Traits> &rhs ) noexcept
    {
        return !( lhs == rhs );
    }

    template <std::size_t N, class T, class Traits>
    constexpr bool operator<( const FixedStringBase<N, T, Traits> &lhs,
                              const FixedStringBase<N, T, Traits> &rhs ) noexcept
    {
        return lhs.compare( rhs ) < 0;
    }

    template <std::size_t N, class T, class Traits>
    constexpr bool operator<=( const FixedStringBase<N, T, Traits> &lhs,
                               const FixedStringBase<N, T, Traits> &rhs ) noexcept
    {
        return lhs.compare( rhs ) <= 0;
    }

    template <std::size_t N, class T, class Traits>
    constexpr bool operator>( const FixedStringBase<N, T, Traits> &lhs,
                              const FixedStringBase<N, T, Traits> &rhs ) noexcept
    {
        return lhs.compare( rhs ) > 0;
    }

    template <std::size_t N, class T, class Traits>
    constexpr bool operator>=( const FixedStringBase<N, T, Traits> &lhs,
                               const FixedStringBase<N, T, Traits> &rhs ) noexcept
    {
        return lhs.compare( rhs ) >= 0;
    }

    // Comparison with std::basic_string
    template <std::size_t N, class T, class Traits>
    constexpr bool operator==( const FixedStringBase<N, T, Traits> &lhs,
                               const std::basic_string<T, Traits> &rhs ) noexcept
    {
        return lhs.compare( rhs ) == 0;
    }

    template <std::size_t N, class T, class Traits>
    constexpr bool operator==( const std::basic_string<T, Traits> &lhs,
                               const FixedStringBase<N, T, Traits> &rhs ) noexcept
    {
        return rhs == lhs;
    }

    template <std::size_t N, class T, class FixedTraits, class StringTraits, class Allocator>
    constexpr bool operator==( const FixedStringBase<N, T, FixedTraits> &lhs,
                               const StringBase<T, StringTraits, Allocator> &rhs ) noexcept
    {
        return lhs.size() == rhs.size() &&
               FixedTraits::compare( lhs.data(), rhs.data(), lhs.size() ) == 0;
    }

    template <std::size_t N, class T, class FixedTraits, class StringTraits, class Allocator>
    constexpr bool operator==( const StringBase<T, StringTraits, Allocator> &lhs,
                               const FixedStringBase<N, T, FixedTraits> &rhs ) noexcept
    {
        return rhs == lhs;
    }

    template <std::size_t N, class T, class FixedTraits, class StringTraits, class Allocator>
    constexpr bool operator!=( const FixedStringBase<N, T, FixedTraits> &lhs,
                               const StringBase<T, StringTraits, Allocator> &rhs ) noexcept
    {
        return !( lhs == rhs );
    }

    template <std::size_t N, class T, class FixedTraits, class StringTraits, class Allocator>
    constexpr bool operator!=( const StringBase<T, StringTraits, Allocator> &lhs,
                               const FixedStringBase<N, T, FixedTraits> &rhs ) noexcept
    {
        return !( lhs == rhs );
    }

    // Comparison with C-string
    template <std::size_t N, class T, class Traits>
    constexpr bool operator==( const FixedStringBase<N, T, Traits> &lhs, const T *rhs ) noexcept
    {
        return lhs.compare( rhs ) == 0;
    }

    template <std::size_t N, class T, class Traits>
    constexpr bool operator==( const T *lhs, const FixedStringBase<N, T, Traits> &rhs ) noexcept
    {
        return rhs == lhs;
    }

    // Type aliases for convenience
    template <std::size_t N>
    using FixedString = FixedStringBase<N, char>;

    template <std::size_t N>
    using FixedWString = FixedStringBase<N, wchar_t>;

    template <std::size_t N>
    using FixedU16String = FixedStringBase<N, char16_t>;

    template <std::size_t N>
    using FixedU32String = FixedStringBase<N, char32_t>;

}  // namespace workphone

#endif  // FixedStringBase_h__
