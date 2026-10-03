#ifndef StringBase_h__
#define StringBase_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include "CharacterPoolAllocator.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <ostream>
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace workphone
{
    template <std::size_t N, class T, class Traits>
    class FixedStringBase;

    /**
     * @brief Cache-friendly dynamic string with SSO, controlled growth and a shared character pool.
     *
     * Small strings stay entirely inside the StringBase object. Dynamic buffers are obtained
     * from CharacterPoolAllocator, which allocates character slabs lazily and reuses returned
     * blocks. GrowthPolicy controls how each string grows when its current capacity is
     * exhausted. Explicit reserve() is always permitted, including in Fixed mode.
     *
     * @tparam T         Character type.
     * @tparam Traits    Character traits.
     * @tparam Allocator Upstream allocator used by the shared character pool.
     */
    template <class T, class Traits = std::char_traits<T>, class Allocator = std::allocator<T>>
    class StringBase
    {
    public:
        using value_type = T;
        using traits_type = Traits;
        using allocator_type = Allocator;
        using storage_allocator_type = CharacterPoolAllocator<T, Allocator>;
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

        static constexpr size_type npos = static_cast<size_type>( -1 );

        // 32 bytes of inline character storage, including the terminator.
        static constexpr size_type InlineStorageBytes = 32;
        static constexpr size_type InlineCapacity =
            ( InlineStorageBytes / sizeof( T ) > 1 ) ? ( ( InlineStorageBytes / sizeof( T ) ) - 1 ) : 1;

        // Deliberately larger than std::string-like tiny growth increments. SSO absorbs
        // small strings, while the first dynamic allocation normally reserves 256 chars.
        static constexpr size_type DefaultGrowthSize = 256;

        // ------------------------------------------------------------------ //
        // Constructors
        // ------------------------------------------------------------------ //

        StringBase() noexcept
        {
            initialise_inline();
        }

        /**
         * @brief Construct an empty string and explicitly reserve initialCapacity.
         */
        explicit StringBase( size_type initialCapacity, GrowthPolicy growthPolicy,
                             size_type growthSize = DefaultGrowthSize ) :
            m_growthPolicy( growthPolicy ),
            m_growthSize( validate_growth_size( growthSize ) )
        {
            initialise_inline();
            reserve( initialCapacity );
        }

        StringBase( const StringBase &other ) :
            m_growthPolicy( other.m_growthPolicy ),
            m_growthSize( other.m_growthSize )
        {
            initialise_inline();
            if( other.m_capacity > InlineCapacity )
            {
                reserve( other.m_capacity );
            }
            assign( other.m_data, other.m_size );
        }

        StringBase( StringBase &&other ) noexcept :
            m_growthPolicy( other.m_growthPolicy ),
            m_growthSize( other.m_growthSize )
        {
            initialise_inline();
            move_from( other );
        }

        StringBase( const T *s )
        {
            initialise_inline();
            if( s )
            {
                assign( s, Traits::length( s ) );
            }
        }

        StringBase( const T *s, size_type count )
        {
            initialise_inline();
            assign( s, count );
        }

        StringBase( size_type count, T ch )
        {
            initialise_inline();
            assign( count, ch );
        }

        template <class InputIt,
                  typename std::enable_if<!std::is_integral<InputIt>::value, int>::type = 0>
        StringBase( InputIt first, InputIt last )
        {
            initialise_inline();
            assign( first, last );
        }

        template <class OtherAllocator>
        StringBase( const std::basic_string<T, Traits, OtherAllocator> &str )
        {
            initialise_inline();
            assign( str.data(), str.size() );
        }

        template <std::size_t N, class FixedTraits>
        StringBase( const FixedStringBase<N, T, FixedTraits> &str )
        {
            initialise_inline();
            assign( str.data(), str.size() );
        }

        ~StringBase()
        {
            release_dynamic_storage();
        }

        // ------------------------------------------------------------------ //
        // Assignment
        // ------------------------------------------------------------------ //

        StringBase &operator=( const StringBase &other )
        {
            if( this != &other )
            {
                m_growthPolicy = other.m_growthPolicy;
                m_growthSize = other.m_growthSize;

                if( other.m_capacity > m_capacity )
                {
                    reserve( other.m_capacity );
                }

                assign( other.m_data, other.m_size );
            }
            return *this;
        }

        StringBase &operator=( StringBase &&other ) noexcept
        {
            if( this != &other )
            {
                release_dynamic_storage();
                initialise_inline();
                m_growthPolicy = other.m_growthPolicy;
                m_growthSize = other.m_growthSize;
                move_from( other );
            }
            return *this;
        }

        StringBase &operator=( const T *s )
        {
            if( !s )
            {
                clear();
                return *this;
            }
            return assign( s, Traits::length( s ) );
        }

        StringBase &operator=( T ch )
        {
            return assign( 1, ch );
        }

        template <class OtherAllocator>
        StringBase &operator=( const std::basic_string<T, Traits, OtherAllocator> &str )
        {
            return assign( str.data(), str.size() );
        }

        template <std::size_t N, class FixedTraits>
        StringBase &operator=( const FixedStringBase<N, T, FixedTraits> &str )
        {
            return assign( str.data(), str.size() );
        }

        StringBase &assign( size_type count, T ch )
        {
            ensure_capacity_for_write( count );
            std::fill_n( m_data, count, ch );
            m_size = count;
            set_terminator();
            return *this;
        }

        StringBase &assign( const T *s )
        {
            if( !s )
            {
                clear();
                return *this;
            }
            return assign( s, Traits::length( s ) );
        }

        StringBase &assign( const T *s, size_type count )
        {
            if( count == 0 )
            {
                clear();
                return *this;
            }
            require_pointer( s, "StringBase::assign" );

            size_type sourceOffset = 0;
            const bool aliases = source_offset( s, count, sourceOffset );

            ensure_capacity_for_write( count );
            const T *source = aliases ? ( m_data + sourceOffset ) : s;
            Traits::move( m_data, source, count );
            m_size = count;
            set_terminator();
            return *this;
        }

        StringBase &assign( const StringBase &str )
        {
            if( this == &str )
            {
                return *this;
            }
            return assign( str.m_data, str.m_size );
        }

        template <class InputIt>
        StringBase &assign( InputIt first, InputIt last )
        {
            using Category = typename std::iterator_traits<InputIt>::iterator_category;

            if constexpr( std::is_pointer<InputIt>::value &&
                          std::is_convertible<InputIt, const T *>::value )
            {
                const auto distance = checked_distance( first, last );
                return assign( static_cast<const T *>( first ), distance );
            }
            else if constexpr( std::is_base_of<std::forward_iterator_tag, Category>::value )
            {
                const size_type count = checked_distance( first, last );
                ensure_capacity_for_write( count );
                std::copy( first, last, m_data );
                m_size = count;
                set_terminator();
                return *this;
            }
            else
            {
                clear();
                for( ; first != last; ++first )
                {
                    push_back( *first );
                }
                return *this;
            }
        }

        // ------------------------------------------------------------------ //
        // Element access
        // ------------------------------------------------------------------ //

        reference at( size_type pos )
        {
            if( pos >= m_size )
            {
                throw std::out_of_range( "StringBase::at" );
            }
            return m_data[pos];
        }

        const_reference at( size_type pos ) const
        {
            if( pos >= m_size )
            {
                throw std::out_of_range( "StringBase::at" );
            }
            return m_data[pos];
        }

        reference operator[]( size_type pos ) noexcept
        {
            return m_data[pos];
        }

        const_reference operator[]( size_type pos ) const noexcept
        {
            return m_data[pos];
        }

        reference front()
        {
            if( empty() )
            {
                throw std::out_of_range( "StringBase::front on empty string" );
            }
            return m_data[0];
        }

        const_reference front() const
        {
            if( empty() )
            {
                throw std::out_of_range( "StringBase::front on empty string" );
            }
            return m_data[0];
        }

        reference back()
        {
            if( empty() )
            {
                throw std::out_of_range( "StringBase::back on empty string" );
            }
            return m_data[m_size - 1];
        }

        const_reference back() const
        {
            if( empty() )
            {
                throw std::out_of_range( "StringBase::back on empty string" );
            }
            return m_data[m_size - 1];
        }

        pointer data() noexcept
        {
            return m_data;
        }
        const_pointer data() const noexcept
        {
            return m_data;
        }
        const_pointer c_str() const noexcept
        {
            return m_data;
        }

        std::basic_string<T, Traits> str() const
        {
            return std::basic_string<T, Traits>( m_data, m_size );
        }

        operator std::basic_string<T, Traits>() const
        {
            return str();
        }

        iterator begin() noexcept
        {
            return m_data;
        }
        const_iterator begin() const noexcept
        {
            return m_data;
        }
        const_iterator cbegin() const noexcept
        {
            return m_data;
        }
        iterator end() noexcept
        {
            return m_data + m_size;
        }
        const_iterator end() const noexcept
        {
            return m_data + m_size;
        }
        const_iterator cend() const noexcept
        {
            return m_data + m_size;
        }
        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator( end() );
        }
        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator( end() );
        }
        const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator( end() );
        }
        reverse_iterator rend() noexcept
        {
            return reverse_iterator( begin() );
        }
        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator( begin() );
        }
        const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator( begin() );
        }

        // ------------------------------------------------------------------ //
        // Capacity / growth policy / pool control
        // ------------------------------------------------------------------ //

        bool empty() const noexcept
        {
            return m_size == 0;
        }
        size_type size() const noexcept
        {
            return m_size;
        }
        size_type length() const noexcept
        {
            return m_size;
        }
        size_type capacity() const noexcept
        {
            return m_capacity;
        }
        size_type available() const noexcept
        {
            return m_capacity - m_size;
        }

        size_type max_size() const noexcept
        {
            const size_type allocatorMax = m_storageAllocator.max_size();
            return allocatorMax > 0 ? allocatorMax - 1 : 0;
        }

        static constexpr size_type inline_capacity() noexcept
        {
            return InlineCapacity;
        }

        bool using_inline_storage() const noexcept
        {
            return is_inline();
        }

        GrowthPolicy getGrowthPolicy() const noexcept
        {
            return m_growthPolicy;
        }

        void setGrowthPolicy( GrowthPolicy policy ) noexcept
        {
            m_growthPolicy = policy;
        }

        size_type getGrowthSize() const noexcept
        {
            return m_growthSize;
        }

        void setGrowthSize( size_type growthSize )
        {
            m_growthSize = validate_growth_size( growthSize );
        }

        /** Explicit growth; allowed even when the policy is Fixed. */
        void reserve( size_type newCapacity )
        {
            if( newCapacity > max_size() )
            {
                throw std::length_error( "StringBase::reserve exceeds max_size" );
            }

            if( newCapacity > m_capacity )
            {
                reallocate( newCapacity );
            }
        }

        void shrink_to_fit()
        {
            if( m_size <= InlineCapacity )
            {
                if( !is_inline() )
                {
                    const size_type oldSize = m_size;
                    T temp[InlineCapacity + 1];
                    if( oldSize > 0 )
                    {
                        Traits::copy( temp, m_data, oldSize );
                    }
                    temp[oldSize] = T();

                    release_dynamic_storage();
                    initialise_inline();
                    if( oldSize > 0 )
                    {
                        // Preserve embedded NULs by restoring the explicit size.
                        Traits::copy( m_inline, temp, oldSize );
                    }
                    m_size = oldSize;
                    set_terminator();
                }
                return;
            }

            if( m_capacity > m_size )
            {
                reallocate( m_size );
            }
        }

        /** Return dynamic storage to the shared pool and become an empty SSO string. */
        void release() noexcept
        {
            release_dynamic_storage();
            initialise_inline();
        }

        /**
         * @brief Warm the shared pool for strings of approximately stringCapacity.
         *
         * For example preallocate_character_pool(256, 512) prepares at least 512
         * reusable dynamic buffers for ~256-character strings before gameplay.
         */
        static void preallocate_character_pool( size_type stringCapacity = DefaultGrowthSize,
                                                size_type stringCount = 256 )
        {
            if( stringCapacity == ( std::numeric_limits<size_type>::max )() )
            {
                throw std::length_error( "StringBase character pool capacity overflow" );
            }
            storage_allocator_type::reserve_blocks( stringCapacity + 1, stringCount );
        }

        static void release_unused_character_pool() noexcept
        {
            storage_allocator_type::release_unused();
        }

        static CharacterPoolStats character_pool_stats() noexcept
        {
            return storage_allocator_type::stats();
        }

        // ------------------------------------------------------------------ //
        // Modifiers
        // ------------------------------------------------------------------ //

        void clear() noexcept
        {
            m_size = 0;
            set_terminator();
        }

        StringBase &append( size_type count, T ch )
        {
            if( count == 0 )
            {
                return *this;
            }

            const size_type newSize = checked_add( m_size, count, "StringBase::append" );
            ensure_capacity_for_write( newSize );
            std::fill_n( m_data + m_size, count, ch );
            m_size = newSize;
            set_terminator();
            return *this;
        }

        StringBase &append( const T *s )
        {
            return append( s, checked_length( s, "StringBase::append" ) );
        }

        StringBase &append( const T *s, size_type count )
        {
            if( count == 0 )
            {
                return *this;
            }
            require_pointer( s, "StringBase::append" );

            size_type sourceOffset = 0;
            const bool aliases = source_offset( s, count, sourceOffset );
            const size_type oldSize = m_size;
            const size_type newSize = checked_add( oldSize, count, "StringBase::append" );

            ensure_capacity_for_write( newSize );
            const T *source = aliases ? ( m_data + sourceOffset ) : s;
            Traits::move( m_data + oldSize, source, count );
            m_size = newSize;
            set_terminator();
            return *this;
        }

        StringBase &append( const StringBase &str )
        {
            return append( str.m_data, str.m_size );
        }

        template <class InputIt>
        StringBase &append( InputIt first, InputIt last )
        {
            using Category = typename std::iterator_traits<InputIt>::iterator_category;

            if constexpr( std::is_pointer<InputIt>::value &&
                          std::is_convertible<InputIt, const T *>::value )
            {
                const size_type count = checked_distance( first, last );
                return append( static_cast<const T *>( first ), count );
            }
            else if constexpr( std::is_base_of<std::forward_iterator_tag, Category>::value )
            {
                const size_type count = checked_distance( first, last );
                if( count == 0 )
                {
                    return *this;
                }

                const size_type oldSize = m_size;
                const size_type newSize = checked_add( oldSize, count, "StringBase::append range" );
                ensure_capacity_for_write( newSize );
                std::copy( first, last, m_data + oldSize );
                m_size = newSize;
                set_terminator();
                return *this;
            }
            else
            {
                for( ; first != last; ++first )
                {
                    push_back( *first );
                }
                return *this;
            }
        }

        StringBase &operator+=( T ch )
        {
            return append( 1, ch );
        }
        StringBase &operator+=( const T *s )
        {
            return append( s );
        }
        StringBase &operator+=( const StringBase &str )
        {
            return append( str );
        }

        void push_back( T ch )
        {
            append( 1, ch );
        }

        bool try_push_back( T ch )
        {
            if( m_growthPolicy == GrowthPolicy::Fixed && m_size == m_capacity )
            {
                return false;
            }
            push_back( ch );
            return true;
        }

        void pop_back() noexcept
        {
            if( m_size > 0 )
            {
                --m_size;
                set_terminator();
            }
        }

        void resize( size_type count )
        {
            resize( count, T() );
        }

        void resize( size_type count, T ch )
        {
            if( count > m_size )
            {
                ensure_capacity_for_write( count );
                std::fill( m_data + m_size, m_data + count, ch );
            }
            m_size = count;
            set_terminator();
        }

        StringBase &insert( size_type pos, size_type count, T ch )
        {
            validate_position( pos, "StringBase::insert" );
            if( count == 0 )
            {
                return *this;
            }

            const size_type newSize = checked_add( m_size, count, "StringBase::insert" );
            ensure_capacity_for_write( newSize );
            Traits::move( m_data + pos + count, m_data + pos, m_size - pos );
            std::fill_n( m_data + pos, count, ch );
            m_size = newSize;
            set_terminator();
            return *this;
        }

        StringBase &insert( size_type pos, const T *s )
        {
            return insert( pos, s, checked_length( s, "StringBase::insert" ) );
        }

        StringBase &insert( size_type pos, const T *s, size_type count )
        {
            validate_position( pos, "StringBase::insert" );
            if( count == 0 )
            {
                return *this;
            }
            require_pointer( s, "StringBase::insert" );

            size_type sourceOffset = 0;
            const bool aliases = source_offset( s, count, sourceOffset );
            const size_type oldSize = m_size;
            const size_type newSize = checked_add( oldSize, count, "StringBase::insert" );

            ensure_capacity_for_write( newSize );
            Traits::move( m_data + pos + count, m_data + pos, oldSize - pos );

            if( !aliases )
            {
                Traits::copy( m_data + pos, s, count );
            }
            else
            {
                const size_type sourceEnd = sourceOffset + count;
                if( sourceEnd <= pos )
                {
                    Traits::move( m_data + pos, m_data + sourceOffset, count );
                }
                else if( sourceOffset >= pos )
                {
                    Traits::move( m_data + pos, m_data + sourceOffset + count, count );
                }
                else
                {
                    // Source straddles the insertion point. The left part stayed in
                    // place; the right part moved right with the tail.
                    const size_type leftCount = pos - sourceOffset;
                    const size_type rightCount = count - leftCount;
                    if( leftCount > 0 )
                    {
                        Traits::move( m_data + pos, m_data + sourceOffset, leftCount );
                    }
                    if( rightCount > 0 )
                    {
                        Traits::move( m_data + pos + leftCount, m_data + pos + count, rightCount );
                    }
                }
            }

            m_size = newSize;
            set_terminator();
            return *this;
        }

        StringBase &insert( size_type pos, const StringBase &str )
        {
            return insert( pos, str.m_data, str.m_size );
        }

        StringBase &erase( size_type pos = 0, size_type count = npos )
        {
            validate_position( pos, "StringBase::erase" );
            const size_type availableCount = m_size - pos;
            if( count == npos || count > availableCount )
            {
                count = availableCount;
            }

            if( count > 0 )
            {
                Traits::move( m_data + pos, m_data + pos + count, m_size - pos - count );
                m_size -= count;
                set_terminator();
            }
            return *this;
        }

        iterator erase( const_iterator pos )
        {
            const size_type index = iterator_index( pos, false, "StringBase::erase iterator" );
            erase( index, 1 );
            return m_data + index;
        }

        iterator erase( const_iterator first, const_iterator last )
        {
            const size_type firstIndex = iterator_index( first, true, "StringBase::erase range" );
            const size_type lastIndex = iterator_index( last, true, "StringBase::erase range" );
            if( lastIndex < firstIndex )
            {
                throw std::out_of_range( "StringBase::erase invalid iterator range" );
            }
            erase( firstIndex, lastIndex - firstIndex );
            return m_data + firstIndex;
        }

        StringBase &replace( size_type pos, size_type count, const StringBase &str )
        {
            if( this == &str )
            {
                StringBase temp( str );
                return replace_range( pos, count, temp.m_data, temp.m_size );
            }
            return replace_range( pos, count, str.m_data, str.m_size );
        }

        StringBase &replace( size_type pos, size_type count, const T *s )
        {
            const size_type replacementCount = checked_length( s, "StringBase::replace" );

            size_type sourceOffset = 0;
            if( source_offset( s, replacementCount, sourceOffset ) )
            {
                StringBase temp( s, replacementCount );
                return replace_range( pos, count, temp.m_data, temp.m_size );
            }
            return replace_range( pos, count, s, replacementCount );
        }

        StringBase &replace( size_type pos, size_type count1, size_type count2, T ch )
        {
            validate_position( pos, "StringBase::replace" );
            const size_type eraseCount = (std::min)( count1, m_size - pos );
            const size_type baseSize = m_size - eraseCount;
            const size_type finalSize = checked_add( baseSize, count2, "StringBase::replace" );
            ensure_capacity_for_write( finalSize );

            const size_type tailCount = m_size - pos - eraseCount;
            if( count2 != eraseCount )
            {
                Traits::move( m_data + pos + count2, m_data + pos + eraseCount, tailCount );
            }
            std::fill_n( m_data + pos, count2, ch );
            m_size = finalSize;
            set_terminator();
            return *this;
        }

        void swap( StringBase &other ) noexcept
        {
            if( this == &other )
            {
                return;
            }

            StringBase temp( std::move( *this ) );
            *this = std::move( other );
            other = std::move( temp );
        }

        // ------------------------------------------------------------------ //
        // Search
        // ------------------------------------------------------------------ //

        size_type find( const StringBase &str, size_type pos = 0 ) const noexcept
        {
            return find( str.m_data, pos, str.m_size );
        }

        size_type find( const T *s, size_type pos, size_type count ) const
        {
            if( pos > m_size )
            {
                return npos;
            }
            if( count == 0 )
            {
                return pos;
            }
            require_pointer( s, "StringBase::find" );
            if( count > m_size - pos )
            {
                return npos;
            }

            const size_type last = m_size - count;
            for( size_type i = pos; i <= last; ++i )
            {
                if( Traits::compare( m_data + i, s, count ) == 0 )
                {
                    return i;
                }
            }
            return npos;
        }

        size_type find( const T *s, size_type pos = 0 ) const
        {
            return find( s, pos, checked_length( s, "StringBase::find" ) );
        }

        size_type find( T ch, size_type pos = 0 ) const noexcept
        {
            for( size_type i = pos; i < m_size; ++i )
            {
                if( Traits::eq( m_data[i], ch ) )
                {
                    return i;
                }
            }
            return npos;
        }

        size_type rfind( const StringBase &str, size_type pos = npos ) const noexcept
        {
            return rfind( str.m_data, pos, str.m_size );
        }

        size_type rfind( const T *s, size_type pos, size_type count ) const
        {
            if( count == 0 )
            {
                return (std::min)( pos, m_size );
            }
            require_pointer( s, "StringBase::rfind" );
            if( count > m_size )
            {
                return npos;
            }

            size_type i = (std::min)( pos, m_size - count );
            for( ;; )
            {
                if( Traits::compare( m_data + i, s, count ) == 0 )
                {
                    return i;
                }
                if( i == 0 )
                {
                    break;
                }
                --i;
            }
            return npos;
        }

        size_type rfind( const T *s, size_type pos = npos ) const
        {
            return rfind( s, pos, checked_length( s, "StringBase::rfind" ) );
        }

        size_type rfind( T ch, size_type pos = npos ) const noexcept
        {
            if( m_size == 0 )
            {
                return npos;
            }
            size_type i = (std::min)( pos, m_size - 1 );
            for( ;; )
            {
                if( Traits::eq( m_data[i], ch ) )
                {
                    return i;
                }
                if( i == 0 )
                {
                    break;
                }
                --i;
            }
            return npos;
        }

        size_type find_first_of( const StringBase &str, size_type pos = 0 ) const noexcept
        {
            return find_first_of( str.m_data, pos, str.m_size );
        }

        size_type find_first_of( const T *s, size_type pos, size_type count ) const
        {
            if( count == 0 || pos >= m_size )
            {
                return npos;
            }
            require_pointer( s, "StringBase::find_first_of" );
            for( size_type i = pos; i < m_size; ++i )
            {
                for( size_type j = 0; j < count; ++j )
                {
                    if( Traits::eq( m_data[i], s[j] ) )
                    {
                        return i;
                    }
                }
            }
            return npos;
        }

        size_type find_first_of( const T *s, size_type pos = 0 ) const
        {
            return find_first_of( s, pos, checked_length( s, "StringBase::find_first_of" ) );
        }

        size_type find_first_of( T ch, size_type pos = 0 ) const noexcept
        {
            return find( ch, pos );
        }

        size_type find_first_not_of( const StringBase &str, size_type pos = 0 ) const noexcept
        {
            return find_first_not_of( str.m_data, pos, str.m_size );
        }

        size_type find_first_not_of( const T *s, size_type pos, size_type count ) const
        {
            if( pos >= m_size )
            {
                return npos;
            }
            if( count == 0 )
            {
                return pos;
            }
            require_pointer( s, "StringBase::find_first_not_of" );

            for( size_type i = pos; i < m_size; ++i )
            {
                bool matched = false;
                for( size_type j = 0; j < count; ++j )
                {
                    if( Traits::eq( m_data[i], s[j] ) )
                    {
                        matched = true;
                        break;
                    }
                }
                if( !matched )
                {
                    return i;
                }
            }
            return npos;
        }

        size_type find_first_not_of( const T *s, size_type pos = 0 ) const
        {
            return find_first_not_of( s, pos, checked_length( s, "StringBase::find_first_not_of" ) );
        }

        size_type find_first_not_of( T ch, size_type pos = 0 ) const noexcept
        {
            for( size_type i = pos; i < m_size; ++i )
            {
                if( !Traits::eq( m_data[i], ch ) )
                {
                    return i;
                }
            }
            return npos;
        }

        size_type find_last_of( const StringBase &str, size_type pos = npos ) const noexcept
        {
            return find_last_of( str.m_data, pos, str.m_size );
        }

        size_type find_last_of( const T *s, size_type pos, size_type count ) const
        {
            if( m_size == 0 || count == 0 )
            {
                return npos;
            }
            require_pointer( s, "StringBase::find_last_of" );

            size_type i = (std::min)( pos, m_size - 1 );
            for( ;; )
            {
                for( size_type j = 0; j < count; ++j )
                {
                    if( Traits::eq( m_data[i], s[j] ) )
                    {
                        return i;
                    }
                }
                if( i == 0 )
                {
                    break;
                }
                --i;
            }
            return npos;
        }

        size_type find_last_of( const T *s, size_type pos = npos ) const
        {
            return find_last_of( s, pos, checked_length( s, "StringBase::find_last_of" ) );
        }

        size_type find_last_of( T ch, size_type pos = npos ) const noexcept
        {
            return rfind( ch, pos );
        }

        size_type find_last_not_of( const StringBase &str, size_type pos = npos ) const noexcept
        {
            return find_last_not_of( str.m_data, pos, str.m_size );
        }

        size_type find_last_not_of( const T *s, size_type pos, size_type count ) const
        {
            if( m_size == 0 )
            {
                return npos;
            }

            size_type i = (std::min)( pos, m_size - 1 );
            if( count == 0 )
            {
                return i;
            }
            require_pointer( s, "StringBase::find_last_not_of" );

            for( ;; )
            {
                bool matched = false;
                for( size_type j = 0; j < count; ++j )
                {
                    if( Traits::eq( m_data[i], s[j] ) )
                    {
                        matched = true;
                        break;
                    }
                }
                if( !matched )
                {
                    return i;
                }
                if( i == 0 )
                {
                    break;
                }
                --i;
            }
            return npos;
        }

        size_type find_last_not_of( const T *s, size_type pos = npos ) const
        {
            return find_last_not_of( s, pos, checked_length( s, "StringBase::find_last_not_of" ) );
        }

        size_type find_last_not_of( T ch, size_type pos = npos ) const noexcept
        {
            if( m_size == 0 )
            {
                return npos;
            }
            size_type i = (std::min)( pos, m_size - 1 );
            for( ;; )
            {
                if( !Traits::eq( m_data[i], ch ) )
                {
                    return i;
                }
                if( i == 0 )
                {
                    break;
                }
                --i;
            }
            return npos;
        }

        // ------------------------------------------------------------------ //
        // Substring / compare
        // ------------------------------------------------------------------ //

        StringBase substr( size_type pos = 0, size_type count = npos ) const
        {
            validate_position( pos, "StringBase::substr" );
            const size_type availableCount = m_size - pos;
            if( count == npos || count > availableCount )
            {
                count = availableCount;
            }
            return StringBase( m_data + pos, count );
        }

        int compare( const StringBase &str ) const noexcept
        {
            const size_type minLength = (std::min)( m_size, str.m_size );
            if( minLength > 0 )
            {
                const int cmp = Traits::compare( m_data, str.m_data, minLength );
                if( cmp != 0 )
                {
                    return cmp;
                }
            }
            if( m_size < str.m_size )
                return -1;
            if( m_size > str.m_size )
                return 1;
            return 0;
        }

        int compare( size_type pos1, size_type count1, const StringBase &str ) const
        {
            validate_position( pos1, "StringBase::compare" );
            count1 = (std::min)( count1, m_size - pos1 );
            return compare_ranges( m_data + pos1, count1, str.m_data, str.m_size );
        }

        int compare( size_type pos1, size_type count1, const StringBase &str, size_type pos2,
                     size_type count2 ) const
        {
            validate_position( pos1, "StringBase::compare" );
            str.validate_position( pos2, "StringBase::compare" );
            count1 = (std::min)( count1, m_size - pos1 );
            count2 = (std::min)( count2, str.m_size - pos2 );
            return compare_ranges( m_data + pos1, count1, str.m_data + pos2, count2 );
        }

        int compare( const T *s ) const
        {
            return compare_ranges( m_data, m_size, s, checked_length( s, "StringBase::compare" ) );
        }

        int compare( size_type pos1, size_type count1, const T *s ) const
        {
            const size_type rhsCount = checked_length( s, "StringBase::compare" );
            validate_position( pos1, "StringBase::compare" );
            count1 = (std::min)( count1, m_size - pos1 );
            return compare_ranges( m_data + pos1, count1, s, rhsCount );
        }

        int compare( size_type pos1, size_type count1, const T *s, size_type count2 ) const
        {
            if( count2 > 0 )
            {
                require_pointer( s, "StringBase::compare" );
            }
            validate_position( pos1, "StringBase::compare" );
            count1 = (std::min)( count1, m_size - pos1 );
            return compare_ranges( m_data + pos1, count1, s, count2 );
        }

    private:
        static size_type validate_growth_size( size_type growthSize )
        {
            if( growthSize == 0 )
            {
                throw std::invalid_argument( "StringBase growth size cannot be zero" );
            }
            return growthSize;
        }

        void initialise_inline() noexcept
        {
            m_data = m_inline;
            m_size = 0;
            m_capacity = InlineCapacity;
            m_inline[0] = T();
        }

        bool is_inline() const noexcept
        {
            return m_data == m_inline;
        }

        void move_from( StringBase &other ) noexcept
        {
            if( other.is_inline() )
            {
                if( other.m_size > 0 )
                {
                    Traits::copy( m_inline, other.m_inline, other.m_size );
                }
                m_size = other.m_size;
                m_capacity = InlineCapacity;
                m_data = m_inline;
                set_terminator();
                other.clear();
            }
            else
            {
                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;
                other.initialise_inline();
            }
        }

        static void require_pointer( const T *ptr, const char *message )
        {
            if( !ptr )
            {
                throw std::invalid_argument( message );
            }
        }

        static size_type checked_length( const T *ptr, const char *message )
        {
            if( !ptr )
            {
                throw std::invalid_argument( message );
            }
            return Traits::length( ptr );
        }

        void validate_position( size_type pos, const char *message ) const
        {
            if( pos > m_size )
            {
                throw std::out_of_range( message );
            }
        }

        static size_type checked_add( size_type a, size_type b, const char *message )
        {
            if( b > ( std::numeric_limits<size_type>::max )() - a )
            {
                throw std::length_error( message );
            }
            return a + b;
        }

        template <class InputIt>
        static size_type checked_distance( InputIt first, InputIt last )
        {
            const auto distance = std::distance( first, last );
            if constexpr( std::is_signed<decltype( distance )>::value )
            {
                if( distance < 0 )
                {
                    throw std::length_error( "StringBase invalid iterator range" );
                }
            }
            return static_cast<size_type>( distance );
        }

        size_type growth_target( size_type requested ) const
        {
            if( requested <= m_capacity )
            {
                return m_capacity;
            }
            if( requested > max_size() )
            {
                throw std::length_error( "StringBase capacity exceeds max_size" );
            }

            switch( m_growthPolicy )
            {
            case GrowthPolicy::Fixed:
                throw std::length_error( "StringBase fixed capacity exhausted" );

            case GrowthPolicy::Grow:
            {
                const size_type g = m_growthSize;
                const size_type remainder = requested % g;
                if( remainder == 0 )
                {
                    return requested;
                }
                const size_type extra = g - remainder;
                if( requested > max_size() - extra )
                {
                    return requested;
                }
                return requested + extra;
            }

            case GrowthPolicy::Double:
            {
                size_type doubled = m_capacity;
                if( doubled < m_growthSize )
                {
                    doubled = m_growthSize;
                }
                else if( doubled <= max_size() / 2 )
                {
                    doubled *= 2;
                }
                else
                {
                    doubled = max_size();
                }
                return (std::max)( requested, doubled );
            }
            }

            throw std::logic_error( "StringBase invalid GrowthPolicy" );
        }

        void ensure_capacity_for_write( size_type requested )
        {
            if( requested <= m_capacity )
            {
                return;
            }
            reallocate( growth_target( requested ) );
        }

        void reallocate( size_type newCapacity )
        {
            if( newCapacity <= InlineCapacity )
            {
                return;
            }
            if( newCapacity > max_size() )
            {
                throw std::length_error( "StringBase allocation exceeds max_size" );
            }

            pointer newData = m_storageAllocator.allocate( newCapacity + 1 );
            try
            {
                if( m_size > 0 )
                {
                    Traits::copy( newData, m_data, m_size );
                }
                newData[m_size] = T();
            }
            catch( ... )
            {
                m_storageAllocator.deallocate( newData, newCapacity + 1 );
                throw;
            }

            if( !is_inline() )
            {
                m_storageAllocator.deallocate( m_data, m_capacity + 1 );
            }

            m_data = newData;
            m_capacity = newCapacity;
        }

        void release_dynamic_storage() noexcept
        {
            if( m_data && !is_inline() )
            {
                m_storageAllocator.deallocate( m_data, m_capacity + 1 );
            }
        }

        void set_terminator() noexcept
        {
            m_data[m_size] = T();
        }

        /**
         * Determine whether [source, source+count) is a range inside this string's
         * currently initialized characters. Integer addresses avoid undefined pointer
         * ordering/subtraction between unrelated arrays.
         */
        bool source_offset( const T *source, size_type count, size_type &offset ) const
        {
            if( !source || count == 0 )
            {
                return false;
            }

            const std::uintptr_t beginAddress = reinterpret_cast<std::uintptr_t>( m_data );
            const std::uintptr_t sourceAddress = reinterpret_cast<std::uintptr_t>( source );
            const std::uintptr_t initializedEnd = beginAddress + ( m_size * sizeof( T ) );

            if( sourceAddress < beginAddress || sourceAddress >= initializedEnd )
            {
                return false;
            }

            const std::uintptr_t delta = sourceAddress - beginAddress;
            if( delta % sizeof( T ) != 0 )
            {
                return false;
            }

            offset = static_cast<size_type>( delta / sizeof( T ) );
            if( count > m_size - offset )
            {
                throw std::out_of_range( "StringBase source range exceeds string data" );
            }
            return true;
        }

        size_type iterator_index( const_iterator it, bool allowEnd, const char *message ) const
        {
            if( !it )
            {
                throw std::out_of_range( message );
            }

            const std::uintptr_t beginAddress = reinterpret_cast<std::uintptr_t>( m_data );
            const std::uintptr_t endAddress = beginAddress + m_size * sizeof( T );
            const std::uintptr_t address = reinterpret_cast<std::uintptr_t>( it );

            if( address < beginAddress || address > endAddress ||
                ( !allowEnd && address == endAddress ) )
            {
                throw std::out_of_range( message );
            }

            const std::uintptr_t delta = address - beginAddress;
            if( delta % sizeof( T ) != 0 )
            {
                throw std::out_of_range( message );
            }
            return static_cast<size_type>( delta / sizeof( T ) );
        }

        StringBase &replace_range( size_type pos, size_type count, const T *replacement,
                                   size_type replacementCount )
        {
            validate_position( pos, "StringBase::replace" );
            if( replacementCount > 0 )
            {
                require_pointer( replacement, "StringBase::replace" );
            }

            const size_type eraseCount = (std::min)( count, m_size - pos );
            const size_type baseSize = m_size - eraseCount;
            const size_type finalSize = checked_add( baseSize, replacementCount, "StringBase::replace" );
            ensure_capacity_for_write( finalSize );

            const size_type tailCount = m_size - pos - eraseCount;
            if( replacementCount != eraseCount )
            {
                Traits::move( m_data + pos + replacementCount, m_data + pos + eraseCount, tailCount );
            }
            if( replacementCount > 0 )
            {
                Traits::copy( m_data + pos, replacement, replacementCount );
            }
            m_size = finalSize;
            set_terminator();
            return *this;
        }

        static int compare_ranges( const T *lhs, size_type lhsCount, const T *rhs,
                                   size_type rhsCount ) noexcept
        {
            const size_type minLength = (std::min)( lhsCount, rhsCount );
            if( minLength > 0 )
            {
                const int cmp = Traits::compare( lhs, rhs, minLength );
                if( cmp != 0 )
                {
                    return cmp;
                }
            }
            if( lhsCount < rhsCount )
                return -1;
            if( lhsCount > rhsCount )
                return 1;
            return 0;
        }

        T m_inline[InlineCapacity + 1]{};
        pointer m_data = nullptr;
        size_type m_size = 0;
        size_type m_capacity = InlineCapacity;
        GrowthPolicy m_growthPolicy = GrowthPolicy::Grow;
        size_type m_growthSize = DefaultGrowthSize;
        storage_allocator_type m_storageAllocator{};
    };

    // ---------------------------------------------------------------------- //
    // Comparison operators
    // ---------------------------------------------------------------------- //

    template <class T, class Traits, class Allocator>
    bool operator==( const StringBase<T, Traits, Allocator> &lhs,
                     const StringBase<T, Traits, Allocator> &rhs ) noexcept
    {
        return lhs.compare( rhs ) == 0;
    }

    template <class T, class Traits, class Allocator>
    bool operator!=( const StringBase<T, Traits, Allocator> &lhs,
                     const StringBase<T, Traits, Allocator> &rhs ) noexcept
    {
        return !( lhs == rhs );
    }

    template <class T, class Traits, class Allocator>
    bool operator<( const StringBase<T, Traits, Allocator> &lhs,
                    const StringBase<T, Traits, Allocator> &rhs ) noexcept
    {
        return lhs.compare( rhs ) < 0;
    }

    template <class T, class Traits, class Allocator>
    bool operator<=( const StringBase<T, Traits, Allocator> &lhs,
                     const StringBase<T, Traits, Allocator> &rhs ) noexcept
    {
        return lhs.compare( rhs ) <= 0;
    }

    template <class T, class Traits, class Allocator>
    bool operator>( const StringBase<T, Traits, Allocator> &lhs,
                    const StringBase<T, Traits, Allocator> &rhs ) noexcept
    {
        return lhs.compare( rhs ) > 0;
    }

    template <class T, class Traits, class Allocator>
    bool operator>=( const StringBase<T, Traits, Allocator> &lhs,
                     const StringBase<T, Traits, Allocator> &rhs ) noexcept
    {
        return lhs.compare( rhs ) >= 0;
    }

    template <class T, class Traits, class Allocator>
    bool operator==( const StringBase<T, Traits, Allocator> &lhs, const T *rhs )
    {
        return lhs.compare( rhs ) == 0;
    }

    template <class T, class Traits, class Allocator>
    bool operator==( const T *lhs, const StringBase<T, Traits, Allocator> &rhs )
    {
        return rhs == lhs;
    }

    template <class T, class Traits, class Allocator>
    bool operator!=( const StringBase<T, Traits, Allocator> &lhs, const T *rhs )
    {
        return !( lhs == rhs );
    }

    template <class T, class Traits, class Allocator>
    bool operator!=( const T *lhs, const StringBase<T, Traits, Allocator> &rhs )
    {
        return !( rhs == lhs );
    }

    template <class T, class Traits, class Allocator>
    bool operator<( const StringBase<T, Traits, Allocator> &lhs, const T *rhs )
    {
        return lhs.compare( rhs ) < 0;
    }

    template <class T, class Traits, class Allocator>
    bool operator>( const StringBase<T, Traits, Allocator> &lhs, const T *rhs )
    {
        return lhs.compare( rhs ) > 0;
    }

    template <class T, class Traits, class Allocator, class OtherAllocator>
    bool operator==( const StringBase<T, Traits, Allocator> &lhs,
                     const std::basic_string<T, Traits, OtherAllocator> &rhs )
    {
        if( lhs.size() != rhs.size() )
        {
            return false;
        }
        return lhs.size() == 0 || Traits::compare( lhs.data(), rhs.data(), lhs.size() ) == 0;
    }

    template <class T, class Traits, class Allocator, class OtherAllocator>
    bool operator==( const std::basic_string<T, Traits, OtherAllocator> &lhs,
                     const StringBase<T, Traits, Allocator> &rhs )
    {
        return rhs == lhs;
    }

    template <class T, class Traits, class Allocator, class OtherAllocator>
    bool operator!=( const StringBase<T, Traits, Allocator> &lhs,
                     const std::basic_string<T, Traits, OtherAllocator> &rhs )
    {
        return !( lhs == rhs );
    }

    template <class T, class Traits, class Allocator, class OtherAllocator>
    bool operator!=( const std::basic_string<T, Traits, OtherAllocator> &lhs,
                     const StringBase<T, Traits, Allocator> &rhs )
    {
        return !( rhs == lhs );
    }

    // ---------------------------------------------------------------------- //
    // Concatenation operators
    // ---------------------------------------------------------------------- //

    template <class T, class Traits, class Allocator>
    StringBase<T, Traits, Allocator> operator+( StringBase<T, Traits, Allocator> lhs,
                                                const StringBase<T, Traits, Allocator> &rhs )
    {
        lhs += rhs;
        return lhs;
    }

    template <class T, class Traits, class Allocator>
    StringBase<T, Traits, Allocator> operator+( StringBase<T, Traits, Allocator> lhs, const T *rhs )
    {
        lhs += rhs;
        return lhs;
    }

    template <class T, class Traits, class Allocator>
    StringBase<T, Traits, Allocator> operator+( const T *lhs,
                                                const StringBase<T, Traits, Allocator> &rhs )
    {
        return StringBase<T, Traits, Allocator>( lhs ) += rhs;
    }

    template <class T, class Traits, class Allocator>
    StringBase<T, Traits, Allocator> operator+( StringBase<T, Traits, Allocator> lhs, T rhs )
    {
        lhs += rhs;
        return lhs;
    }

    template <class T, class Traits, class Allocator>
    StringBase<T, Traits, Allocator> operator+( T lhs, const StringBase<T, Traits, Allocator> &rhs )
    {
        return StringBase<T, Traits, Allocator>( 1, lhs ) + rhs;
    }

    template <class T, class Traits, class Allocator, class OtherAllocator>
    StringBase<T, Traits, Allocator> operator+( StringBase<T, Traits, Allocator> lhs,
                                                const std::basic_string<T, Traits, OtherAllocator> &rhs )
    {
        lhs.append( rhs.data(), rhs.size() );
        return lhs;
    }

    template <class T, class Traits, class Allocator, class OtherAllocator>
    StringBase<T, Traits, Allocator> operator+( const std::basic_string<T, Traits, OtherAllocator> &lhs,
                                                const StringBase<T, Traits, Allocator> &rhs )
    {
        return StringBase<T, Traits, Allocator>( lhs ) + rhs;
    }

    // ---------------------------------------------------------------------- //
    // Stream operators
    // ---------------------------------------------------------------------- //

    template <class T, class Traits, class Allocator>
    std::basic_ostream<T, Traits> &operator<<( std::basic_ostream<T, Traits> &os,
                                               const StringBase<T, Traits, Allocator> &str )
    {
        // write() preserves embedded NUL characters; operator<<(c_str()) does not.
        return os.write( str.data(), static_cast<std::streamsize>( str.size() ) );
    }

    template <class T, class Traits, class Allocator>
    std::basic_istream<T, Traits> &operator>>( std::basic_istream<T, Traits> &is,
                                               StringBase<T, Traits, Allocator> &str )
    {
        std::basic_string<T, Traits> temp;
        is >> temp;
        if( is )
        {
            str.assign( temp.data(), temp.size() );
        }
        return is;
    }

    template <class T, class Traits, class Allocator>
    std::basic_istream<T, Traits> &getline( std::basic_istream<T, Traits> &is,
                                            StringBase<T, Traits, Allocator> &str, T delimiter )
    {
        std::basic_string<T, Traits> temp;
        std::getline( is, temp, delimiter );
        if( is || !temp.empty() )
        {
            str.assign( temp.data(), temp.size() );
        }
        return is;
    }

    template <class T, class Traits, class Allocator>
    std::basic_istream<T, Traits> &getline( std::basic_istream<T, Traits> &is,
                                            StringBase<T, Traits, Allocator> &str )
    {
        return getline( is, str, is.widen( '\n' ) );
    }

    template <class T, class Traits, class Allocator>
    void swap( StringBase<T, Traits, Allocator> &lhs, StringBase<T, Traits, Allocator> &rhs ) noexcept
    {
        lhs.swap( rhs );
    }

}  // namespace workphone

namespace std
{
    template <class T, class Traits, class Allocator>
    struct hash<workphone::StringBase<T, Traits, Allocator>>
    {
        std::size_t operator()( const workphone::StringBase<T, Traits, Allocator> &s ) const noexcept
        {
            return std::hash<std::basic_string_view<T, Traits>>{}(
                std::basic_string_view<T, Traits>( s.data(), s.size() ) );
        }
    };
}  // namespace std

#endif  // StringBase_h__
