#ifndef _WP_FixedArrayGrowable_h__
#define _WP_FixedArrayGrowable_h__

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace workphone
{
    /**
     * A variable-size array with capacity N and storage embedded in the container.
     * Only the size() elements are constructed; T need not be default constructible.
     * The container never allocates. T's operations and user callbacks may allocate,
     * and throwing an exception may allocate exception storage.
     *
     * Requests exceeding N throw std::length_error. Invalid positions throw
     * std::out_of_range. clear() retains the fixed capacity. Moves transfer elements
     * individually and empty the source on success. Insertion/erasure invalidate
     * iterators at and after the position; assignment and swap invalidate all.
     * If element movement/assignment throws, affected values may be changed, but
     * all live elements remain tracked and will be destroyed.
     */
    template <class T, std::size_t N>
    class FixedArrayGrowable
    {
        static_assert( N <= static_cast<std::size_t>( (std::numeric_limits<std::ptrdiff_t>::max)() ),
                       "FixedArrayGrowable capacity exceeds iterator range" );
        static_assert( std::is_object<T>::value && !std::is_const<T>::value &&
                           !std::is_volatile<T>::value,
                       "FixedArrayGrowable requires a non-cv object type" );

    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;

        // Index-based iterators avoid pointer arithmetic on unconstructed storage.
        template <bool IsConst>
        class BasicIterator
        {
            friend class FixedArrayGrowable;
            template <bool>
            friend class BasicIterator;
            using owner_type = typename std::conditional<IsConst, const FixedArrayGrowable,
                                                          FixedArrayGrowable>::type;
            owner_type *m_owner = nullptr;
            std::ptrdiff_t m_index = 0;

            BasicIterator( owner_type *owner, std::ptrdiff_t index ) noexcept :
                m_owner( owner ), m_index( index )
            {
            }

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = typename std::conditional<IsConst, const T *, T *>::type;
            using reference = typename std::conditional<IsConst, const T &, T &>::type;

            BasicIterator() noexcept = default;
            template <bool Other, typename std::enable_if<IsConst && !Other, int>::type = 0>
            BasicIterator( const BasicIterator<Other> &other ) noexcept :
                m_owner( other.m_owner ), m_index( other.m_index )
            {
            }
            reference operator*() const { return (*m_owner)[static_cast<size_type>( m_index )]; }
            pointer operator->() const { return std::addressof( **this ); }
            reference operator[]( difference_type n ) const { return *(*this + n); }
            BasicIterator &operator++() { ++m_index; return *this; }
            BasicIterator operator++( int ) { auto result = *this; ++*this; return result; }
            BasicIterator &operator--() { --m_index; return *this; }
            BasicIterator operator--( int ) { auto result = *this; --*this; return result; }
            BasicIterator &operator+=( difference_type n ) { m_index += n; return *this; }
            BasicIterator &operator-=( difference_type n ) { m_index -= n; return *this; }
            BasicIterator operator+( difference_type n ) const { auto r = *this; return r += n; }
            BasicIterator operator-( difference_type n ) const { auto r = *this; return r -= n; }
            friend BasicIterator operator+( difference_type n, BasicIterator it ) { return it += n; }
            template <bool Other>
            difference_type operator-( const BasicIterator<Other> &other ) const
            {
                assert( m_owner == other.m_owner );
                return m_index - other.m_index;
            }
            template <bool Other>
            bool operator==( const BasicIterator<Other> &other ) const
            { return m_owner == other.m_owner && m_index == other.m_index; }
            template <bool Other>
            bool operator!=( const BasicIterator<Other> &other ) const { return !(*this == other); }
            template <bool Other>
            bool operator<( const BasicIterator<Other> &other ) const
            { assert( m_owner == other.m_owner ); return m_index < other.m_index; }
            template <bool Other>
            bool operator>( const BasicIterator<Other> &other ) const { return other < *this; }
            template <bool Other>
            bool operator<=( const BasicIterator<Other> &other ) const { return !(other < *this); }
            template <bool Other>
            bool operator>=( const BasicIterator<Other> &other ) const { return !(*this < other); }
        };

        using iterator = BasicIterator<false>;
        using const_iterator = BasicIterator<true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        FixedArrayGrowable() noexcept {}
        explicit FixedArrayGrowable( size_type count ) : FixedArrayGrowable() { resize( count ); }
        FixedArrayGrowable( size_type count, const T &value ) : FixedArrayGrowable()
        { resize( count, value ); }
        FixedArrayGrowable( std::initializer_list<T> values ) :
            FixedArrayGrowable( values.begin(), values.end() )
        {
        }
        template <class InputIt,
                  typename std::enable_if<!std::is_integral<InputIt>::value, int>::type = 0>
        FixedArrayGrowable( InputIt first, InputIt last ) : FixedArrayGrowable()
        {
            validateRange( first, last );
            for( ; first != last; ++first )
                emplace_back( *first );
        }
        FixedArrayGrowable( const FixedArrayGrowable &other ) :
            FixedArrayGrowable( other.begin(), other.end() )
        {
        }
        FixedArrayGrowable( FixedArrayGrowable &&other ) noexcept(
            std::is_nothrow_move_constructible<T>::value ) : FixedArrayGrowable()
        {
            for( auto &value : other )
                emplace_back( std::move( value ) );
            other.clear();
        }
        ~FixedArrayGrowable() { clear(); }

        FixedArrayGrowable &operator=( const FixedArrayGrowable &other )
        {
            if( this != &other )
            {
                FixedArrayGrowable replacement( other );
                replaceFrom( replacement );
            }
            return *this;
        }
        FixedArrayGrowable &operator=( FixedArrayGrowable &&other ) noexcept(
            std::is_nothrow_move_constructible<T>::value )
        {
            if( this != &other )
            {
                clear();
                for( auto &value : other )
                    emplace_back( std::move( value ) );
                other.clear();
            }
            return *this;
        }
        FixedArrayGrowable &operator=( std::initializer_list<T> values )
        { assign( values ); return *this; }

        void assign( size_type count, const T &value )
        {
            FixedArrayGrowable replacement( count, value );
            replaceFrom( replacement );
        }
        template <class InputIt,
                  typename std::enable_if<!std::is_integral<InputIt>::value, int>::type = 0>
        void assign( InputIt first, InputIt last )
        {
            FixedArrayGrowable replacement( first, last );
            replaceFrom( replacement );
        }
        void assign( std::initializer_list<T> values ) { assign( values.begin(), values.end() ); }
        // Like std::array::fill, assign the value to each existing element.
        void fill( const T &value ) { std::fill( begin(), end(), value ); }

        size_type size() const noexcept { return m_size; }
        static constexpr size_type capacity() noexcept { return N; }
        static constexpr size_type max_size() noexcept { return N; }
        bool empty() const noexcept { return m_size == 0; }
        bool full() const noexcept { return m_size == N; }
        void reserve( size_type count ) const { checkCapacity( count ); }
        void shrink_to_fit() noexcept {}

        reference at( size_type index ) { checkIndex( index ); return *element( index ); }
        const_reference at( size_type index ) const { checkIndex( index ); return *element( index ); }
        reference operator[]( size_type index ) { return at( index ); }
        const_reference operator[]( size_type index ) const { return at( index ); }
        reference front() { return at( 0 ); }
        const_reference front() const { return at( 0 ); }
        reference back() { return at( m_size == 0 ? 0 : m_size - 1 ); }
        const_reference back() const { return at( m_size == 0 ? 0 : m_size - 1 ); }
        pointer data() noexcept { return N == 0 ? nullptr : rawElement( 0 ); }
        const_pointer data() const noexcept { return N == 0 ? nullptr : rawElement( 0 ); }

        iterator begin() noexcept { return iterator( this, 0 ); }
        const_iterator begin() const noexcept { return const_iterator( this, 0 ); }
        iterator end() noexcept { return iterator( this, static_cast<difference_type>( m_size ) ); }
        const_iterator end() const noexcept
        { return const_iterator( this, static_cast<difference_type>( m_size ) ); }
        const_iterator cbegin() const noexcept { return begin(); }
        const_iterator cend() const noexcept { return end(); }
        reverse_iterator rbegin() noexcept { return reverse_iterator( end() ); }
        reverse_iterator rend() noexcept { return reverse_iterator( begin() ); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator( end() ); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator( begin() ); }
        const_reverse_iterator crbegin() const noexcept { return rbegin(); }
        const_reverse_iterator crend() const noexcept { return rend(); }

        template <class... Args>
        reference emplace_back( Args &&...args )
        {
            checkAdditional( 1 );
            T *value = ::new( static_cast<void *>( rawElement( m_size ) ) )
                T( std::forward<Args>( args )... );
            ++m_size;
            return *value;
        }
        void push_back( const T &value ) { emplace_back( value ); }
        void push_back( T &&value ) { emplace_back( std::move( value ) ); }
        template <class... Args>
        bool try_emplace_back( Args &&...args )
        {
            if( full() )
                return false;
            emplace_back( std::forward<Args>( args )... );
            return true;
        }
        bool try_push_back( const T &value ) { return try_emplace_back( value ); }
        bool try_push_back( T &&value ) { return try_emplace_back( std::move( value ) ); }
        void pop_back()
        {
            if( m_size != 0 )
                truncate( m_size - 1 );
        }
        void clear() noexcept
        {
            while( m_size != 0 )
            {
                element( m_size - 1 )->~T();
                --m_size;
            }
        }
        void resize( size_type count )
        {
            checkCapacity( count );
            const auto oldSize = m_size;
            try
            {
                while( m_size < count )
                    emplace_back();
            }
            catch( ... )
            {
                truncate( oldSize );
                throw;
            }
            truncate( count );
        }
        void resize( size_type count, const T &value )
        {
            checkCapacity( count );
            const auto oldSize = m_size;
            try
            {
                while( m_size < count )
                    emplace_back( value );
            }
            catch( ... )
            {
                truncate( oldSize );
                throw;
            }
            truncate( count );
        }

        template <class... Args>
        iterator emplace( const_iterator where, Args &&...args )
        {
            const auto index = iteratorIndex( where, true );
            checkAdditional( 1 );
            if( index == m_size )
                emplace_back( std::forward<Args>( args )... );
            else
            {
                // Stage before shifting, so arguments may refer to our own elements.
                T value( std::forward<Args>( args )... );
                emplace_back( std::move_if_noexcept( back() ) );
                try
                {
                    for( auto i = m_size - 2; i > index; --i )
                        (*this)[i] = std::move_if_noexcept( (*this)[i - 1] );
                    (*this)[index] = std::move_if_noexcept( value );
                }
                catch( ... )
                {
                    pop_back();
                    throw;
                }
            }
            return begin() + static_cast<difference_type>( index );
        }
        template <class... Args>
        void emplace( size_type index, Args &&...args )
        {
            checkPosition( index );
            emplace( cbegin() + static_cast<difference_type>( index ),
                     std::forward<Args>( args )... );
        }
        iterator insert( const_iterator where, const T &value ) { return emplace( where, value ); }
        iterator insert( const_iterator where, T &&value )
        { return emplace( where, std::move( value ) ); }
        void insert( size_type index, const T &value ) { emplace( index, value ); }
        void insert( size_type index, T &&value ) { emplace( index, std::move( value ) ); }
        iterator insert( const_iterator where, size_type count, const T &value )
        {
            const auto index = iteratorIndex( where, true );
            checkAdditional( count );
            FixedArrayGrowable staged( count, value );
            return insertStaged( index, staged );
        }
        template <class InputIt,
                  typename std::enable_if<!std::is_integral<InputIt>::value, int>::type = 0>
        iterator insert( const_iterator where, InputIt first, InputIt last )
        {
            const auto index = iteratorIndex( where, true );
            validateRange( first, last );
            FixedArrayGrowable staged;
            for( ; first != last; ++first )
            {
                checkAdditional( staged.size() + 1 );
                staged.emplace_back( *first );
            }
            return insertStaged( index, staged );
        }
        iterator insert( const_iterator where, std::initializer_list<T> values )
        { return insert( where, values.begin(), values.end() ); }

        void erase( size_type index )
        {
            checkIndex( index );
            erase( cbegin() + static_cast<difference_type>( index ) );
        }
        iterator erase( const_iterator where )
        {
            const auto index = iteratorIndex( where, false );
            return erase( where, cbegin() + static_cast<difference_type>( index + 1 ) );
        }
        iterator erase( const_iterator first, const_iterator last )
        {
            const auto start = iteratorIndex( first, true );
            const auto finish = iteratorIndex( last, true );
            if( finish < start )
                throw std::out_of_range( "FixedArrayGrowable reversed erase range" );
            const auto count = finish - start;
            if( count != 0 )
            {
                for( auto i = start; i < m_size - count; ++i )
                    (*this)[i] = std::move_if_noexcept( (*this)[i + count] );
                truncate( m_size - count );
            }
            return begin() + static_cast<difference_type>( start );
        }
        template <class Pred>
        size_type find_if( Pred pred ) const
        {
            for( size_type i = 0; i < m_size; ++i )
                if( pred( (*this)[i] ) )
                    return i;
            return static_cast<size_type>( -1 );
        }
        template <class Compare = std::less<T>>
        void sort( Compare comp = Compare() ) { std::sort( begin(), end(), comp ); }
        // A snapshot keeps the same inline-storage guarantee.
        FixedArrayGrowable snapshot() const { return *this; }

        void swap( FixedArrayGrowable &other ) noexcept(
            std::is_nothrow_swappable<T>::value && std::is_nothrow_move_constructible<T>::value )
        {
            if( this == &other )
                return;
            const auto common = (std::min)( m_size, other.m_size );
            using std::swap;
            for( size_type i = 0; i < common; ++i )
                swap( (*this)[i], other[i] );
            auto &larger = m_size > other.m_size ? *this : other;
            auto &smaller = m_size > other.m_size ? other : *this;
            smaller.appendSwapTail( larger, common );
            larger.truncate( common );
        }
        friend void swap( FixedArrayGrowable &lhs, FixedArrayGrowable &rhs ) noexcept(
            noexcept( lhs.swap( rhs ) ) ) { lhs.swap( rhs ); }
        friend bool operator==( const FixedArrayGrowable &lhs, const FixedArrayGrowable &rhs )
        { return lhs.size() == rhs.size() && std::equal( lhs.begin(), lhs.end(), rhs.begin() ); }
        friend bool operator!=( const FixedArrayGrowable &lhs, const FixedArrayGrowable &rhs )
        { return !(lhs == rhs); }
        friend bool operator<( const FixedArrayGrowable &lhs, const FixedArrayGrowable &rhs )
        { return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() ); }
        friend bool operator>( const FixedArrayGrowable &lhs, const FixedArrayGrowable &rhs )
        { return rhs < lhs; }
        friend bool operator<=( const FixedArrayGrowable &lhs, const FixedArrayGrowable &rhs )
        { return !(rhs < lhs); }
        friend bool operator>=( const FixedArrayGrowable &lhs, const FixedArrayGrowable &rhs )
        { return !(lhs < rhs); }

    private:
        alignas( T ) unsigned char m_storage[sizeof( T ) * (N == 0 ? 1 : N)];
        size_type m_size = 0;

        pointer rawElement( size_type index ) noexcept
        { return reinterpret_cast<pointer>( m_storage + sizeof( T ) * index ); }
        const_pointer rawElement( size_type index ) const noexcept
        { return reinterpret_cast<const_pointer>( m_storage + sizeof( T ) * index ); }
        pointer element( size_type index ) noexcept { return std::launder( rawElement( index ) ); }
        const_pointer element( size_type index ) const noexcept
        { return std::launder( rawElement( index ) ); }
        static void checkCapacity( size_type count )
        {
            if( count > N )
                throw std::length_error( "FixedArrayGrowable capacity exceeded" );
        }
        void checkAdditional( size_type count ) const
        {
            if( count > N - m_size )
                throw std::length_error( "FixedArrayGrowable capacity exceeded" );
        }
        void checkIndex( size_type index ) const
        {
            if( index >= m_size )
                throw std::out_of_range( "FixedArrayGrowable index out of range" );
        }
        void checkPosition( size_type index ) const
        {
            if( index > m_size )
                throw std::out_of_range( "FixedArrayGrowable position out of range" );
        }
        size_type iteratorIndex( const_iterator it, bool allowEnd ) const
        {
            if( it.m_owner != this || it.m_index < 0 ||
                static_cast<size_type>( it.m_index ) > m_size ||
                (!allowEnd && static_cast<size_type>( it.m_index ) == m_size) )
                throw std::out_of_range( "FixedArrayGrowable iterator out of range" );
            return static_cast<size_type>( it.m_index );
        }
        template <class InputIt>
        static void validateRange( InputIt first, InputIt last )
        {
            using category = typename std::iterator_traits<InputIt>::iterator_category;
            if constexpr( std::is_base_of<std::random_access_iterator_tag, category>::value )
            {
                if( first != last && last < first )
                    throw std::out_of_range( "FixedArrayGrowable reversed iterator range" );
            }
        }
        void truncate( size_type count ) noexcept
        {
            while( m_size > count )
            {
                element( m_size - 1 )->~T();
                --m_size;
            }
        }
        void replaceFrom( FixedArrayGrowable &replacement )
        {
            clear();
            for( auto &value : replacement )
                emplace_back( std::move_if_noexcept( value ) );
        }
        iterator insertStaged( size_type index, FixedArrayGrowable &staged )
        {
            const auto count = staged.size();
            if( count == 0 )
                return begin() + static_cast<difference_type>( index );
            const auto oldSize = m_size;
            try
            {
                // Construct the new tail before assigning any existing slots.
                for( auto i = oldSize; i < oldSize + count; ++i )
                {
                    if( i < index + count )
                        emplace_back( std::move_if_noexcept( staged[i - index] ) );
                    else
                        emplace_back( std::move_if_noexcept( (*this)[i - count] ) );
                }
                for( auto i = oldSize; i > index; --i )
                {
                    const auto destination = i - 1 + count;
                    if( destination < oldSize )
                        (*this)[destination] = std::move_if_noexcept( (*this)[i - 1] );
                }
                for( size_type i = 0; i < count && index + i < oldSize; ++i )
                    (*this)[index + i] = std::move_if_noexcept( staged[i] );
            }
            catch( ... )
            {
                truncate( oldSize );
                throw;
            }
            return begin() + static_cast<difference_type>( index );
        }
        void appendSwapTail( FixedArrayGrowable &other, size_type first )
        {
            const auto oldSize = m_size;
            try
            {
                for( auto i = first; i < other.m_size; ++i )
                    emplace_back( std::move_if_noexcept( other[i] ) );
            }
            catch( ... )
            {
                truncate( oldSize );
                throw;
            }
        }
    };
}  // namespace workphone

#endif  // _WP_FixedArrayGrowable_h__
