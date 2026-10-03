#ifndef _WP_Array_h__
#define _WP_Array_h__

#include <Workphone/Core/Allocator.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <type_traits>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>

namespace workphone
{
    // Forward declarations
    template <class T, class A>
    class ArrayBaseConstIterator;

    /**
     * @brief Random access iterator for ArrayBase.
     *
     * Wraps a raw pointer to provide standard random access iterator semantics.
     * Unlike its concurrent counterpart, no locking is performed.
     *
     * @tparam T Value type stored in the container.
     * @tparam A Allocator type (used only for template parameter matching).
     */
    template <class T, class A>
    class ArrayBaseIterator
    {
    public:
        // Iterator traits (required for standard iterator compatibility)
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        pointer it;

        ArrayBaseIterator() : it( nullptr )
        {
        }

        explicit ArrayBaseIterator( pointer p ) : it( p )
        {
        }

        // Iterator operations
        reference operator*() const
        {
            return *it;
        }

        pointer operator->() const
        {
            return it;
        }

        // Prefix increment
        ArrayBaseIterator &operator++()
        {
            ++it;
            return *this;
        }

        // Postfix increment
        ArrayBaseIterator operator++( int )
        {
            ArrayBaseIterator tmp = *this;
            ++it;
            return tmp;
        }

        // Prefix decrement
        ArrayBaseIterator &operator--()
        {
            --it;
            return *this;
        }

        // Postfix decrement
        ArrayBaseIterator operator--( int )
        {
            ArrayBaseIterator tmp = *this;
            --it;
            return tmp;
        }

        // Random access operations
        ArrayBaseIterator &operator+=( difference_type n )
        {
            if( n != 0 )
                it += n;
            return *this;
        }

        ArrayBaseIterator &operator-=( difference_type n )
        {
            if( n != 0 )
                it -= n;
            return *this;
        }

        ArrayBaseIterator operator+( difference_type n ) const
        {
            return n == 0 ? *this : ArrayBaseIterator( it + n );
        }

        ArrayBaseIterator operator-( difference_type n ) const
        {
            return n == 0 ? *this : ArrayBaseIterator( it - n );
        }

        difference_type operator-( const ArrayBaseIterator &other ) const
        {
            if( it == nullptr && other.it == nullptr )
                return 0;
            return it - other.it;
        }

        reference operator[]( difference_type n ) const
        {
            return it[n];
        }

        // Comparison operators
        bool operator==( const ArrayBaseIterator &other ) const
        {
            return it == other.it;
        }

        bool operator!=( const ArrayBaseIterator &other ) const
        {
            return it != other.it;
        }

        bool operator<( const ArrayBaseIterator &other ) const
        {
            return std::less<pointer>()( it, other.it );
        }

        bool operator<=( const ArrayBaseIterator &other ) const
        {
            return !( other < *this );
        }

        bool operator>( const ArrayBaseIterator &other ) const
        {
            return other < *this;
        }

        bool operator>=( const ArrayBaseIterator &other ) const
        {
            return !( *this < other );
        }

        // Get the underlying pointer
        pointer base() const
        {
            return it;
        }
    };

    // Non-member operators for ArrayBaseIterator
    template <class T, class A>
    ArrayBaseIterator<T, A> operator+( typename ArrayBaseIterator<T, A>::difference_type n,
                                       const ArrayBaseIterator<T, A> &it )
    {
        return it + n;
    }

    /**
     * @brief Const random access iterator for ArrayBase.
     *
     * Wraps a const raw pointer to provide standard random access iterator
     * semantics with read-only access.
     *
     * @tparam T Value type stored in the container.
     * @tparam A Allocator type (used only for template parameter matching).
     */
    template <class T, class A>
    class ArrayBaseConstIterator
    {
    public:
        // Iterator traits (required for standard iterator compatibility)
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T *;
        using reference = const T &;

        pointer it;

        ArrayBaseConstIterator() : it( nullptr )
        {
        }

        explicit ArrayBaseConstIterator( pointer p ) : it( p )
        {
        }

        /**
         * @brief Conversion constructor from non-const iterator.
         */
        ArrayBaseConstIterator( const ArrayBaseIterator<T, A> &other ) : it( other.base() )
        {
        }

        // Iterator operations
        reference operator*() const
        {
            return *it;
        }

        pointer operator->() const
        {
            return it;
        }

        // Prefix increment
        ArrayBaseConstIterator &operator++()
        {
            ++it;
            return *this;
        }

        // Postfix increment
        ArrayBaseConstIterator operator++( int )
        {
            ArrayBaseConstIterator tmp = *this;
            ++it;
            return tmp;
        }

        // Prefix decrement
        ArrayBaseConstIterator &operator--()
        {
            --it;
            return *this;
        }

        // Postfix decrement
        ArrayBaseConstIterator operator--( int )
        {
            ArrayBaseConstIterator tmp = *this;
            --it;
            return tmp;
        }

        // Random access operations
        ArrayBaseConstIterator &operator+=( difference_type n )
        {
            if( n != 0 )
                it += n;
            return *this;
        }

        ArrayBaseConstIterator &operator-=( difference_type n )
        {
            if( n != 0 )
                it -= n;
            return *this;
        }

        ArrayBaseConstIterator operator+( difference_type n ) const
        {
            return n == 0 ? *this : ArrayBaseConstIterator( it + n );
        }

        ArrayBaseConstIterator operator-( difference_type n ) const
        {
            return n == 0 ? *this : ArrayBaseConstIterator( it - n );
        }

        difference_type operator-( const ArrayBaseConstIterator &other ) const
        {
            if( it == nullptr && other.it == nullptr )
                return 0;
            return it - other.it;
        }

        reference operator[]( difference_type n ) const
        {
            return it[n];
        }

        // Comparison operators
        bool operator==( const ArrayBaseConstIterator &other ) const
        {
            return it == other.it;
        }

        bool operator!=( const ArrayBaseConstIterator &other ) const
        {
            return it != other.it;
        }

        bool operator<( const ArrayBaseConstIterator &other ) const
        {
            return std::less<pointer>()( it, other.it );
        }

        bool operator<=( const ArrayBaseConstIterator &other ) const
        {
            return !( other < *this );
        }

        bool operator>( const ArrayBaseConstIterator &other ) const
        {
            return other < *this;
        }

        bool operator>=( const ArrayBaseConstIterator &other ) const
        {
            return !( *this < other );
        }

        // Get the underlying pointer
        pointer base() const
        {
            return it;
        }
    };

    // Non-member operators for ArrayBaseConstIterator
    template <class T, class A>
    ArrayBaseConstIterator<T, A> operator+( typename ArrayBaseConstIterator<T, A>::difference_type n,
                                            const ArrayBaseConstIterator<T, A> &it )
    {
        return it + n;
    }

    template <class T, class A>
    bool operator==( const ArrayBaseIterator<T, A> &lhs, const ArrayBaseConstIterator<T, A> &rhs )
    {
        return lhs.base() == rhs.base();
    }

    template <class T, class A>
    bool operator==( const ArrayBaseConstIterator<T, A> &lhs, const ArrayBaseIterator<T, A> &rhs )
    {
        return rhs == lhs;
    }

    template <class T, class A>
    bool operator!=( const ArrayBaseIterator<T, A> &lhs, const ArrayBaseConstIterator<T, A> &rhs )
    {
        return !( lhs == rhs );
    }

    template <class T, class A>
    bool operator!=( const ArrayBaseConstIterator<T, A> &lhs, const ArrayBaseIterator<T, A> &rhs )
    {
        return !( lhs == rhs );
    }

    template <class T, class A>
    bool operator<( const ArrayBaseIterator<T, A> &lhs, const ArrayBaseConstIterator<T, A> &rhs )
    {
        return std::less<const T *>()( lhs.base(), rhs.base() );
    }

    template <class T, class A>
    bool operator<( const ArrayBaseConstIterator<T, A> &lhs, const ArrayBaseIterator<T, A> &rhs )
    {
        return std::less<const T *>()( lhs.base(), rhs.base() );
    }

    template <class T, class A>
    bool operator<=( const ArrayBaseIterator<T, A> &lhs, const ArrayBaseConstIterator<T, A> &rhs )
    {
        return !( rhs < lhs );
    }

    template <class T, class A>
    bool operator<=( const ArrayBaseConstIterator<T, A> &lhs, const ArrayBaseIterator<T, A> &rhs )
    {
        return !( rhs < lhs );
    }

    template <class T, class A>
    bool operator>( const ArrayBaseIterator<T, A> &lhs, const ArrayBaseConstIterator<T, A> &rhs )
    {
        return rhs < lhs;
    }

    template <class T, class A>
    bool operator>( const ArrayBaseConstIterator<T, A> &lhs, const ArrayBaseIterator<T, A> &rhs )
    {
        return rhs < lhs;
    }

    template <class T, class A>
    bool operator>=( const ArrayBaseIterator<T, A> &lhs, const ArrayBaseConstIterator<T, A> &rhs )
    {
        return !( lhs < rhs );
    }

    template <class T, class A>
    bool operator>=( const ArrayBaseConstIterator<T, A> &lhs, const ArrayBaseIterator<T, A> &rhs )
    {
        return !( lhs < rhs );
    }

    template <class T, class A>
    typename ArrayBaseIterator<T, A>::difference_type operator-(
        const ArrayBaseIterator<T, A> &lhs, const ArrayBaseConstIterator<T, A> &rhs )
    {
        if( lhs.base() == nullptr && rhs.base() == nullptr )
            return 0;
        return lhs.base() - rhs.base();
    }

    template <class T, class A>
    typename ArrayBaseConstIterator<T, A>::difference_type operator-(
        const ArrayBaseConstIterator<T, A> &lhs, const ArrayBaseIterator<T, A> &rhs )
    {
        if( lhs.base() == nullptr && rhs.base() == nullptr )
            return 0;
        return lhs.base() - rhs.base();
    }

    /**
     * @brief Dynamic array container with custom memory management.
     *
     * Provides a subset of std::vector's API with additional convenience methods
     * such as find_if, sort, snapshot, and index-based insert/erase. This is the
     * non-thread-safe counterpart of ConcurrentArrayBase.
     *
     * Manages its own raw storage (m_data, m_size, m_capacity) via the allocator
     * rather than delegating to std::vector.
     *
     * @tparam T Value type stored in the container.
     * @tparam A Allocator type (defaults to std::allocator<T>).
     */
    template <class T, class A = Allocator<T>>
    class ArrayBase
    {
    public:
        typedef A allocator_type;
        typedef T value_type;
        typedef T &reference;
        typedef const T &const_reference;
        typedef typename A::difference_type difference_type;
        typedef typename A::size_type size_type;
        typedef ArrayBaseIterator<T, A> iterator;
        typedef ArrayBaseConstIterator<T, A> const_iterator;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /** @name Constructors, destructor, and assignment
         * @{
         */

        /** @brief Default constructor. */
        ArrayBase();

        /** @brief Configure automatic capacity growth. */
        explicit ArrayBase( GrowthPolicy growthPolicy, size_type growthSize = 1,
                            const allocator_type &a = allocator_type() ) :
            m_alloc( a ),
            m_growthPolicy( growthPolicy ),
            m_growthSize( growthSize )
        {
            validateGrowthSize( growthSize );
        }

        /** @brief Destructor. Destroys elements and frees storage. */
        ~ArrayBase();

        /** @brief Copy constructor. */
        ArrayBase( const ArrayBase &other );

        /** @brief Initialize from initializer list. */
        ArrayBase( std::initializer_list<value_type> il );

        /** @brief Construct from iterator range. */
        template <class I, typename = typename std::enable_if<!std::is_integral<I>::value>::type>
        ArrayBase( I first, I last, const allocator_type &a = allocator_type() ) : m_alloc( a )
        {
            validateRangeOrder( first, last, typename std::iterator_traits<I>::iterator_category() );
            try
            {
                for( ; first != last; ++first )
                    emplace_back( *first );
            }
            catch( ... )
            {
                releaseStorage();
                throw;
            }
        }

        ArrayBase( size_type count )
        {
            if( count > 0 )
            {
                if( count > max_size() )
                    throw std::length_error( "ArrayBase count exceeds allocator max_size" );

                m_capacity = count;
                m_data = alloc_traits::allocate( m_alloc, m_capacity );
                try
                {
                    for( m_size = 0; m_size < count; ++m_size )
                        alloc_traits::construct( m_alloc, m_data + m_size );
                }
                catch( ... )
                {
                    releaseStorage();
                    throw;
                }
            }
        }

        /** @brief Construct with count copies of value. */
        ArrayBase( size_type count, const value_type &value,
                   const allocator_type &a = allocator_type() ) :
            m_alloc( a )
        {
            if( count > 0 )
            {
                if( count > max_size() )
                    throw std::length_error( "ArrayBase count exceeds allocator max_size" );

                m_capacity = count;
                m_data = alloc_traits::allocate( m_alloc, m_capacity );
                try
                {
                    for( m_size = 0; m_size < count; ++m_size )
                        alloc_traits::construct( m_alloc, m_data + m_size, value );
                }
                catch( ... )
                {
                    releaseStorage();
                    throw;
                }
            }
        }

        /** @brief Move constructor. */
        ArrayBase( ArrayBase &&other ) noexcept( std::is_nothrow_move_constructible<A>::value );

        /** @brief Move assignment. */
        ArrayBase &operator=( ArrayBase &&other ) noexcept(
            alloc_traits::is_always_equal::value ||
            ( alloc_traits::propagate_on_container_move_assignment::value &&
              std::is_nothrow_move_assignable<A>::value ) );

        /** @brief Copy assignment. */
        ArrayBase &operator=( const ArrayBase &other );

        /** @brief Assigns new size and fills with value. */
        void assign( const size_type _Newsize, const T &_Val )
        {
            ArrayBase replacement( m_growthPolicy, m_growthSize, m_alloc );
            replacement.reserve( m_capacity );
            replacement.resize( _Newsize, _Val );
            swap( replacement );
        }

        void assign( iterator _First, iterator _Last )
        {
            ArrayBase replacement( m_growthPolicy, m_growthSize, m_alloc );
            replacement.reserve( m_capacity );
            replacement.insert( replacement.begin(), _First, _Last );
            swap( replacement );
        }

        void assign( const_iterator _First, const_iterator _Last )
        {
            ArrayBase replacement( m_growthPolicy, m_growthSize, m_alloc );
            replacement.reserve( m_capacity );
            replacement.insert( replacement.begin(), _First, _Last );
            swap( replacement );
        }

        void assign( const std::initializer_list<T> _Ilist )
        {
            ArrayBase replacement( m_growthPolicy, m_growthSize, m_alloc );
            replacement.reserve( m_capacity );
            replacement.insert( replacement.begin(), _Ilist.begin(), _Ilist.end() );
            swap( replacement );
        }

        void swap( ArrayBase &_Right ) noexcept( alloc_traits::propagate_on_container_swap::value )
        {
            using std::swap;
            if constexpr( alloc_traits::propagate_on_container_swap::value )
            {
                static_assert( std::is_swappable<A>::value,
                               "A propagating allocator must be swappable" );
                swap( m_alloc, _Right.m_alloc );
            }
            else if( m_alloc != _Right.m_alloc )
            {
                throw std::logic_error( "ArrayBase cannot swap unequal allocators" );
            }

            swap( m_data, _Right.m_data );
            swap( m_size, _Right.m_size );
            swap( m_capacity, _Right.m_capacity );
            swap( m_growthPolicy, _Right.m_growthPolicy );
            swap( m_growthSize, _Right.m_growthSize );
        }

        /** @} */

        /** @name Element insertion and construction
         * @{
         */

        /**
         * @brief Construct an element in-place at the end of the container.
         * @tparam Args Argument types forwarded to the element constructor.
         * @return Reference to the newly emplaced element.
         */
        template <typename... Args>
        reference emplace_back( Args &&...args );

        /**
         * @brief Construct an element in-place at the specified index.
         * @param index Position at which to emplace. If index == size() the element is appended.
         * @tparam Args Argument types forwarded to the element constructor.
         */
        template <class... Args>
        void emplace( size_t index, Args &&...args );

        template <class... _Valty>
        iterator emplace( const_iterator _Where, _Valty &&..._Val )
        {
            size_t pos = iteratorIndex( _Where, true, "ArrayBase::emplace iterator out of range" );
            size_t newSize = checkedAdd( m_size, 1, "ArrayBase::emplace capacity overflow" );
            size_t newCapacity = newSize > m_capacity ? growthCapacity( newSize ) : m_capacity;

            rebuildWithInsert( pos, 1, newCapacity, [&]( T *dst, size_t ) {
                alloc_traits::construct( m_alloc, dst, std::forward<_Valty>( _Val )... );
            } );

            return iterator( m_data + pos );
        }

        /** @} */

        /** @name Search and ordering
         * @{
         */

        /**
         * @brief Find the index of the first element satisfying the predicate.
         * @param pred Unary predicate called for elements.
         * @return Index of the found element, or static_cast<size_t>(-1) if not found.
         */
        template <class Pred>
        size_t find_if( Pred pred ) const;

        /**
         * @brief Sort the container using the provided comparison function.
         * @tparam Compare Comparison functor (defaults to std::less<T>).
         */
        template <class Compare = std::less<T>>
        void sort( Compare comp = Compare() );

        /** @} */

        /** @name Element access
         * @{
         */

        const_reference at( size_type index ) const;
        reference at( size_type index );

        reference operator[]( const difference_type index );
        const_reference operator[]( const difference_type index ) const;

        /** @} */

        /** @name Capacity and modifiers
         * @{
         */

        void resize( size_t newSize );
        void resize( size_t newSize, const T &value );
        void reserve( size_t newSize );

        size_t size() const;
        size_t capacity() const;
        bool empty() const;
        size_t max_size() const;

        GrowthPolicy getGrowthPolicy() const noexcept
        {
            return m_growthPolicy;
        }

        void setGrowthPolicy( GrowthPolicy growthPolicy ) noexcept
        {
            m_growthPolicy = growthPolicy;
        }

        size_type getGrowthSize() const noexcept
        {
            return m_growthSize;
        }

        void setGrowthSize( size_type growthSize )
        {
            validateGrowthSize( growthSize );
            m_growthSize = growthSize;
        }

        /**
         * @brief Adds an element to the end by moving.
         *
         * Time complexity: O(1) amortized.
         *
         * @param value Element to move to the end.
         */
        void push_back( T &&value );

        /**
         * @brief Adds an element to the end by copying.
         *
         * Disabled when T is not copy-constructible to prevent compilation errors
         * with move-only types like std::unique_ptr.
         *
         * Time complexity: O(1) amortized.
         *
         * @param value Element to copy to the end.
         */
        template <typename U = T,
                  typename std::enable_if<std::is_copy_constructible<U>::value, int>::type = 0>
        void push_back( const T &value );

        void pop_back();
        void clear();
        void erase( size_t index );

        void erase( const T &p );

        iterator erase( const_iterator pos );
        iterator erase( const_iterator first, const_iterator last );

        void insert( size_t index, const T &value );

        /**
         * @brief Template overload to allow inserting values convertible to T.
         * @tparam U Type convertible to T.
         */
        template <typename U>
        typename std::enable_if<std::is_convertible<U, T>::value>::type insert( size_t index,
                                                                                const U &value );

        typename ArrayBase<T, A>::iterator insert( typename ArrayBase<T, A>::const_iterator _Where,
                                                   const T &_Val );
        typename ArrayBase<T, A>::iterator insert( typename ArrayBase<T, A>::const_iterator _Where,
                                                   T &&_Val );
        typename ArrayBase<T, A>::iterator insert( typename ArrayBase<T, A>::const_iterator _Where,
                                                   const size_type _Count, const T &_Val );

        template <class InputIt,
                  typename = typename std::enable_if<!std::is_integral<InputIt>::value>::type>
        typename ArrayBase<T, A>::iterator insert( typename ArrayBase<T, A>::const_iterator _Where,
                                                   InputIt first, InputIt last );

        /**
         * @brief Return a snapshot copy as a std::vector.
         */
        std::vector<T, A> snapshot() const;

        /** @} */

        /** @name Iterators
         * @{
         */

        const_iterator cbegin() const;
        const_iterator cend() const;
        iterator begin();
        const_iterator begin() const;
        iterator end();
        const_iterator end() const;

        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator( end() );
        }

        reverse_iterator rend() noexcept
        {
            return reverse_iterator( begin() );
        }

        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator( end() );
        }

        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator( begin() );
        }

        const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator( end() );
        }

        const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator( begin() );
        }

        /** @} */

        /** @name Front / Back access
         * @{
         */

        reference front();
        const_reference front() const;
        reference back();
        const_reference back() const;

        /** @} */

        /** @name Raw data access
         * @{
         */

        T *data();
        const T *data() const;

        /** @} */

    private:
        using alloc_traits = std::allocator_traits<A>;

        T *m_data = nullptr;
        size_t m_size = 0;
        size_t m_capacity = 0;
        A m_alloc;
        GrowthPolicy m_growthPolicy = GrowthPolicy::Default;
        size_type m_growthSize = 1;

        static void validateGrowthSize( size_type growthSize )
        {
            if( growthSize == 0 )
                throw std::invalid_argument( "ArrayBase growth size cannot be zero" );
        }

        void assertValid() const
        {
            assert( m_size <= m_capacity );
            assert( ( m_capacity == 0 ) == ( m_data == nullptr ) );
        }

        size_t checkedAdd( size_t lhs, size_t rhs, const char *message ) const
        {
            if( lhs > std::numeric_limits<size_t>::max() - rhs )
                throw std::length_error( message );

            size_t result = lhs + rhs;
            if( result > max_size() )
                throw std::length_error( message );

            return result;
        }

        void checkIndex( size_t index, const char *message ) const
        {
            if( index >= m_size )
                throw std::out_of_range( message );
        }

        void checkInsertIndex( size_t index, const char *message ) const
        {
            if( index > m_size )
                throw std::out_of_range( message );
        }

        template <class I>
        static void validateRangeOrder( const I &, const I &, std::input_iterator_tag )
        {
        }

        template <class I>
        static void validateRangeOrder( const I &first, const I &last, std::random_access_iterator_tag )
        {
            if( last < first )
                throw std::invalid_argument( "ArrayBase iterator range is reversed" );
        }

        T *endPtr()
        {
            return m_data ? m_data + m_size : nullptr;
        }

        const T *endPtr() const
        {
            return m_data ? m_data + m_size : nullptr;
        }

        size_t iteratorIndex( const_iterator it, bool allowEnd, const char *message ) const
        {
            const T *ptr = it.base();

            if( m_size == 0 )
            {
                if( allowEnd && ptr == m_data )
                    return 0;

                throw std::out_of_range( message );
            }

            const T *last = m_data + m_size;
            const auto beginAddress = reinterpret_cast<std::uintptr_t>( m_data );
            const auto endAddress = reinterpret_cast<std::uintptr_t>( last );
            const auto ptrAddress = reinterpret_cast<std::uintptr_t>( ptr );

            if( ptrAddress < beginAddress || ptrAddress > endAddress ||
                ( ptrAddress - beginAddress ) % sizeof( T ) != 0 || ( !allowEnd && ptr == last ) )
            {
                throw std::out_of_range( message );
            }

            return static_cast<size_t>( ( ptrAddress - beginAddress ) / sizeof( T ) );
        }

        void releaseStorage()
        {
            destroyRange( m_data, endPtr() );
            if( m_data )
                alloc_traits::deallocate( m_alloc, m_data, m_capacity );

            m_data = nullptr;
            m_size = 0;
            m_capacity = 0;
        }

        size_t growthCapacity( size_t minCapacity ) const
        {
            const size_t limit = max_size();
            if( minCapacity > limit )
                throw std::length_error( "ArrayBase capacity exceeds allocator max_size" );

            validateGrowthSize( m_growthSize );

            switch( m_growthPolicy )
            {
            case GrowthPolicy::Fixed:
                throw std::length_error( "ArrayBase fixed capacity exhausted" );

            case GrowthPolicy::Grow:
            {
                const size_t required = minCapacity - m_capacity;
                const size_t chunks =
                    required / m_growthSize + static_cast<size_t>( required % m_growthSize != 0 );
                if( chunks > ( limit - m_capacity ) / m_growthSize )
                    return minCapacity;
                return m_capacity + chunks * m_growthSize;
            }

            case GrowthPolicy::Double:
            {
                size_t newCapacity = m_capacity == 0 ? m_growthSize : m_capacity;
                while( newCapacity < minCapacity )
                {
                    if( newCapacity > limit / 2 )
                        return minCapacity;
                    newCapacity *= 2;
                }
                return newCapacity;
            }

            default:
                throw std::logic_error( "Invalid ArrayBase growth policy" );
            }
        }

        /** @brief Destroy elements in range [first, last). */
        void destroyRange( T *first, T *last )
        {
            if( first == nullptr || last == nullptr || first == last )
                return;

            for( ; first != last; ++first )
                alloc_traits::destroy( m_alloc, first );
        }

        void reallocateStorage( size_t newCapacity )
        {
            assertValid();
            if( newCapacity <= m_capacity )
                return;

            if( newCapacity > max_size() )
                throw std::length_error( "ArrayBase capacity exceeds allocator max_size" );

            T *newData = alloc_traits::allocate( m_alloc, newCapacity );
            size_t constructed = 0;
            try
            {
                for( ; constructed < m_size; ++constructed )
                    alloc_traits::construct( m_alloc, newData + constructed,
                                             std::move_if_noexcept( m_data[constructed] ) );
            }
            catch( ... )
            {
                destroyRange( newData, newData + constructed );
                alloc_traits::deallocate( m_alloc, newData, newCapacity );
                throw;
            }

            destroyRange( m_data, endPtr() );

            if( m_data )
                alloc_traits::deallocate( m_alloc, m_data, m_capacity );

            m_data = newData;
            m_capacity = newCapacity;
            assertValid();
        }

        /** @brief Grow storage so that at least minCapacity elements can be held. */
        void ensureCapacity( size_t minCapacity )
        {
            if( minCapacity > m_capacity )
                reallocateStorage( growthCapacity( minCapacity ) );
        }

        template <class Source>
        void rebuildWithInsert( size_t pos, size_t count, size_t newCapacity, Source source )
        {
            T *newData = newCapacity > 0 ? alloc_traits::allocate( m_alloc, newCapacity ) : nullptr;
            size_t insertedConstructed = 0;
            size_t prefixConstructed = 0;
            size_t suffixConstructed = 0;
            try
            {
                // Construct inserted values before relocating existing elements. This keeps
                // references into this array valid while Source consumes them.
                for( ; insertedConstructed < count; ++insertedConstructed )
                    source( newData + pos + insertedConstructed, insertedConstructed );

                for( ; prefixConstructed < pos; ++prefixConstructed )
                    alloc_traits::construct( m_alloc, newData + prefixConstructed,
                                             std::move_if_noexcept( m_data[prefixConstructed] ) );

                for( size_t i = pos; i < m_size; ++i, ++suffixConstructed )
                    alloc_traits::construct( m_alloc, newData + count + i,
                                             std::move_if_noexcept( m_data[i] ) );
            }
            catch( ... )
            {
                destroyRange( newData, newData + prefixConstructed );
                destroyRange( newData + pos, newData + pos + insertedConstructed );
                destroyRange( newData + pos + count, newData + pos + count + suffixConstructed );
                if( newData )
                    alloc_traits::deallocate( m_alloc, newData, newCapacity );
                throw;
            }

            destroyRange( m_data, endPtr() );
            if( m_data )
                alloc_traits::deallocate( m_alloc, m_data, m_capacity );

            m_data = newData;
            m_size += count;
            m_capacity = newCapacity;
            assertValid();
        }

        void rebuildErase( size_t first, size_t last )
        {
            assert( first <= last );
            assert( last <= m_size );
            if( first >= last )
                return;

            if( last == m_size )
            {
                destroyRange( m_data + first, endPtr() );
                m_size = first;
                assertValid();
                return;
            }

            size_t count = last - first;
            size_t newSize = m_size - count;
            T *newData = m_capacity > 0 ? alloc_traits::allocate( m_alloc, m_capacity ) : nullptr;
            size_t constructed = 0;

            try
            {
                for( size_t i = 0; i < first; ++i, ++constructed )
                    alloc_traits::construct( m_alloc, newData + constructed,
                                             std::move_if_noexcept( m_data[i] ) );

                for( size_t i = last; i < m_size; ++i, ++constructed )
                    alloc_traits::construct( m_alloc, newData + constructed,
                                             std::move_if_noexcept( m_data[i] ) );
            }
            catch( ... )
            {
                destroyRange( newData, newData + constructed );
                if( newData )
                    alloc_traits::deallocate( m_alloc, newData, m_capacity );
                throw;
            }

            destroyRange( m_data, endPtr() );
            if( m_data )
                alloc_traits::deallocate( m_alloc, m_data, m_capacity );

            m_data = newData;
            m_size = newSize;
            assertValid();
        }
    };

    /**
     * @brief Equality comparison for two ArrayBase instances.
     *
     * Two arrays are equal if they have the same size and all elements compare equal.
     */
    template <class T, class A>
    bool operator==( const ArrayBase<T, A> &lhs, const ArrayBase<T, A> &rhs )
    {
        if( lhs.size() != rhs.size() )
            return false;

        for( size_t i = 0; i < lhs.size(); ++i )
        {
            if( !( lhs[i] == rhs[i] ) )
                return false;
        }
        return true;
    }

    /**
     * @brief Inequality comparison for two ArrayBase instances.
     */
    template <class T, class A>
    bool operator!=( const ArrayBase<T, A> &lhs, const ArrayBase<T, A> &rhs )
    {
        return !( lhs == rhs );
    }

    /**
     * @brief Lexicographic less-than comparison for two ArrayBase instances.
     */
    template <class T, class A>
    bool operator<( const ArrayBase<T, A> &lhs, const ArrayBase<T, A> &rhs )
    {
        const size_t minSize = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
        for( size_t i = 0; i < minSize; ++i )
        {
            if( lhs[i] < rhs[i] )
                return true;
            if( rhs[i] < lhs[i] )
                return false;
        }
        return lhs.size() < rhs.size();
    }

    /**
     * @brief Lexicographic greater-than comparison for two ArrayBase instances.
     */
    template <class T, class A>
    bool operator>( const ArrayBase<T, A> &lhs, const ArrayBase<T, A> &rhs )
    {
        return rhs < lhs;
    }

    /**
     * @brief Lexicographic less-than-or-equal comparison for two ArrayBase instances.
     */
    template <class T, class A>
    bool operator<=( const ArrayBase<T, A> &lhs, const ArrayBase<T, A> &rhs )
    {
        return !( rhs < lhs );
    }

    /**
     * @brief Lexicographic greater-than-or-equal comparison for two ArrayBase instances.
     */
    template <class T, class A>
    bool operator>=( const ArrayBase<T, A> &lhs, const ArrayBase<T, A> &rhs )
    {
        return !( lhs < rhs );
    }

    template <class T, class A>
    ArrayBase<T, A>::ArrayBase() = default;

    template <class T, class A>
    ArrayBase<T, A>::~ArrayBase()
    {
        releaseStorage();
    }

    template <class T, class A>
    ArrayBase<T, A>::ArrayBase( const ArrayBase &other ) :
        m_alloc( alloc_traits::select_on_container_copy_construction( other.m_alloc ) ),
        m_growthPolicy( other.m_growthPolicy ),
        m_growthSize( other.m_growthSize )
    {
        static_assert( std::is_copy_constructible<T>::value,
                       "ArrayBase<T> copy constructor requires T to be copy constructible" );
        if( other.m_capacity > 0 )
        {
            if( other.m_capacity > max_size() )
                throw std::length_error( "ArrayBase copy exceeds allocator max_size" );

            m_capacity = other.m_capacity;
            m_data = alloc_traits::allocate( m_alloc, m_capacity );
            try
            {
                for( m_size = 0; m_size < other.m_size; ++m_size )
                    alloc_traits::construct( m_alloc, m_data + m_size, other.m_data[m_size] );
            }
            catch( ... )
            {
                releaseStorage();
                throw;
            }
        }
    }

    template <class T, class A>
    ArrayBase<T, A>::ArrayBase( std::initializer_list<value_type> il )
    {
        if( il.size() > 0 )
        {
            if( il.size() > max_size() )
                throw std::length_error( "ArrayBase initializer_list exceeds allocator max_size" );

            m_capacity = il.size();
            m_data = alloc_traits::allocate( m_alloc, m_capacity );
            try
            {
                for( const auto &elem : il )
                {
                    alloc_traits::construct( m_alloc, m_data + m_size, elem );
                    ++m_size;
                }
            }
            catch( ... )
            {
                releaseStorage();
                throw;
            }
        }
    }

    template <class T, class A>
    ArrayBase<T, A>::ArrayBase( ArrayBase &&other ) noexcept(
        std::is_nothrow_move_constructible<A>::value ) :
        m_data( other.m_data ),
        m_size( other.m_size ),
        m_capacity( other.m_capacity ),
        m_alloc( std::move( other.m_alloc ) ),
        m_growthPolicy( other.m_growthPolicy ),
        m_growthSize( other.m_growthSize )
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    template <class T, class A>
    ArrayBase<T, A> &ArrayBase<T, A>::operator=( ArrayBase &&other ) noexcept(
        alloc_traits::is_always_equal::value ||
        ( alloc_traits::propagate_on_container_move_assignment::value &&
          std::is_nothrow_move_assignable<A>::value ) )
    {
        if( this != &other )
        {
            if constexpr( !alloc_traits::is_always_equal::value &&
                          alloc_traits::propagate_on_container_move_assignment::value &&
                          std::is_move_assignable<A>::value )
            {
                releaseStorage();
                m_alloc = std::move( other.m_alloc );
            }
            else if( m_alloc == other.m_alloc )
            {
                releaseStorage();
            }
            else
            {
                ArrayBase replacement( other.m_growthPolicy, other.m_growthSize, m_alloc );
                replacement.reserve( other.m_size );
                for( size_t i = 0; i < other.m_size; ++i )
                    replacement.emplace_back( std::move( other.m_data[i] ) );
                swap( replacement );
                other.clear();
                return *this;
            }

            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            m_growthPolicy = other.m_growthPolicy;
            m_growthSize = other.m_growthSize;

            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    template <class T, class A>
    ArrayBase<T, A> &ArrayBase<T, A>::operator=( const ArrayBase &other )
    {
        static_assert( std::is_copy_constructible<T>::value,
                       "ArrayBase<T> copy assignment requires T to be copy constructible" );
        if( this != &other )
        {
            ArrayBase temp( other );
            *this = std::move( temp );
        }
        return *this;
    }

    template <class T, class A>
    template <typename... Args>
    typename ArrayBase<T, A>::reference ArrayBase<T, A>::emplace_back( Args &&...args )
    {
        size_t newSize = checkedAdd( m_size, 1, "ArrayBase::emplace_back capacity overflow" );
        if( newSize > m_capacity )
        {
            const size_t pos = m_size;
            rebuildWithInsert( pos, 1, growthCapacity( newSize ), [&]( T *dst, size_t ) {
                alloc_traits::construct( m_alloc, dst, std::forward<Args>( args )... );
            } );
            return m_data[pos];
        }

        alloc_traits::construct( m_alloc, m_data + m_size, std::forward<Args>( args )... );
        ++m_size;
        return m_data[m_size - 1];
    }

    template <class T, class A>
    template <class... Args>
    void ArrayBase<T, A>::emplace( size_t index, Args &&...args )
    {
        checkInsertIndex( index, "ArrayBase::emplace index out of range" );
        size_t newSize = checkedAdd( m_size, 1, "ArrayBase::emplace capacity overflow" );
        size_t newCapacity = newSize > m_capacity ? growthCapacity( newSize ) : m_capacity;

        rebuildWithInsert( index, 1, newCapacity, [&]( T *dst, size_t ) {
            alloc_traits::construct( m_alloc, dst, std::forward<Args>( args )... );
        } );
    }

    template <class T, class A>
    template <class Pred>
    size_t ArrayBase<T, A>::find_if( Pred pred ) const
    {
        if( m_size == 0 )
            return static_cast<size_t>( -1 );

        auto it = std::find_if( m_data, m_data + m_size, pred );
        return it != m_data + m_size ? static_cast<size_t>( it - m_data ) : static_cast<size_t>( -1 );
    }

    template <class T, class A>
    template <class Compare>
    void ArrayBase<T, A>::sort( Compare comp )
    {
        if( m_size < 2 )
            return;

        std::sort( m_data, m_data + m_size, comp );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::const_reference ArrayBase<T, A>::at( size_type index ) const
    {
        checkIndex( index, "ArrayBase::at index out of range" );
        return m_data[index];
    }

    template <class T, class A>
    typename ArrayBase<T, A>::reference ArrayBase<T, A>::at( size_type index )
    {
        checkIndex( index, "ArrayBase::at index out of range" );
        return m_data[index];
    }

    template <class T, class A>
    typename ArrayBase<T, A>::reference ArrayBase<T, A>::operator[]( const difference_type index )
    {
        if( index < 0 )
            throw std::out_of_range( "ArrayBase::operator[] negative index" );

        return at( static_cast<size_type>( index ) );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::const_reference ArrayBase<T, A>::operator[](
        const difference_type index ) const
    {
        if( index < 0 )
            throw std::out_of_range( "ArrayBase::operator[] negative index" );

        return at( static_cast<size_type>( index ) );
    }

    template <class T, class A>
    void ArrayBase<T, A>::resize( size_t newSize )
    {
        if( newSize > max_size() )
            throw std::length_error( "ArrayBase::resize exceeds allocator max_size" );

        if( newSize > m_size )
        {
            ensureCapacity( newSize );
            const size_t oldSize = m_size;
            size_t constructed = oldSize;
            try
            {
                for( ; constructed < newSize; ++constructed )
                    alloc_traits::construct( m_alloc, m_data + constructed );
            }
            catch( ... )
            {
                destroyRange( m_data + oldSize, m_data + constructed );
                throw;
            }
        }
        else if( newSize < m_size )
        {
            destroyRange( m_data ? m_data + newSize : nullptr, endPtr() );
        }
        m_size = newSize;
    }

    template <class T, class A>
    void ArrayBase<T, A>::resize( size_t newSize, const T &value )
    {
        if( newSize > max_size() )
            throw std::length_error( "ArrayBase::resize exceeds allocator max_size" );

        if( newSize > m_size )
        {
            T valueCopy( value );
            ensureCapacity( newSize );
            const size_t oldSize = m_size;
            size_t constructed = oldSize;
            try
            {
                for( ; constructed < newSize; ++constructed )
                    alloc_traits::construct( m_alloc, m_data + constructed, valueCopy );
            }
            catch( ... )
            {
                destroyRange( m_data + oldSize, m_data + constructed );
                throw;
            }
        }
        else if( newSize < m_size )
        {
            destroyRange( m_data ? m_data + newSize : nullptr, endPtr() );
        }
        m_size = newSize;
    }

    template <class T, class A>
    void ArrayBase<T, A>::reserve( size_t newSize )
    {
        reallocateStorage( newSize );
    }

    template <class T, class A>
    size_t ArrayBase<T, A>::size() const
    {
        return m_size;
    }

    template <class T, class A>
    size_t ArrayBase<T, A>::capacity() const
    {
        return m_capacity;
    }

    template <class T, class A>
    bool ArrayBase<T, A>::empty() const
    {
        return m_size == 0;
    }

    template <class T, class A>
    size_t ArrayBase<T, A>::max_size() const
    {
        return alloc_traits::max_size( m_alloc );
    }

    template <class T, class A>
    void ArrayBase<T, A>::push_back( T &&value )
    {
        emplace_back( std::move( value ) );
    }

    // Copy version - enabled only for copy-constructible types to prevent
    // issues with move-only types like std::unique_ptr
    template <class T, class A>
    template <typename U, typename std::enable_if<std::is_copy_constructible<U>::value, int>::type>
    void ArrayBase<T, A>::push_back( const T &value )
    {
        emplace_back( value );
    }

    template <class T, class A>
    void ArrayBase<T, A>::pop_back()
    {
        if( m_size > 0 )
        {
            --m_size;
            alloc_traits::destroy( m_alloc, m_data + m_size );
        }
    }

    template <class T, class A>
    void ArrayBase<T, A>::clear()
    {
        destroyRange( m_data, endPtr() );
        m_size = 0;
    }

    template <class T, class A>
    void ArrayBase<T, A>::erase( const T &p )
    {
        std::vector<unsigned char> eraseMask( m_size, 0 );
        size_t eraseCount = 0;
        for( size_t i = 0; i < m_size; ++i )
        {
            if( m_data[i] == p )
            {
                eraseMask[i] = 1;
                ++eraseCount;
            }
        }

        if( eraseCount == 0 )
            return;

        if( eraseCount == m_size )
        {
            clear();
            return;
        }

        T *newData = alloc_traits::allocate( m_alloc, m_capacity );
        size_t constructed = 0;
        try
        {
            for( size_t i = 0; i < m_size; ++i )
            {
                if( !eraseMask[i] )
                {
                    alloc_traits::construct( m_alloc, newData + constructed,
                                             std::move_if_noexcept( m_data[i] ) );
                    ++constructed;
                }
            }
        }
        catch( ... )
        {
            destroyRange( newData, newData + constructed );
            alloc_traits::deallocate( m_alloc, newData, m_capacity );
            throw;
        }

        destroyRange( m_data, endPtr() );
        alloc_traits::deallocate( m_alloc, m_data, m_capacity );
        m_data = newData;
        m_size -= eraseCount;
        assertValid();
    }

    template <class T, class A>
    void ArrayBase<T, A>::erase( size_t index )
    {
        if( index >= m_size )
            return;

        rebuildErase( index, index + 1 );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::iterator ArrayBase<T, A>::erase( const_iterator pos )
    {
        size_t index = iteratorIndex( pos, false, "ArrayBase::erase iterator out of range" );
        rebuildErase( index, index + 1 );
        return iterator( m_data + index );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::iterator ArrayBase<T, A>::erase( const_iterator first,
                                                               const_iterator last )
    {
        size_t firstIdx = iteratorIndex( first, true, "ArrayBase::erase first iterator out of range" );
        size_t lastIdx = iteratorIndex( last, true, "ArrayBase::erase last iterator out of range" );

        if( firstIdx > lastIdx )
            throw std::out_of_range( "ArrayBase::erase iterator range is reversed" );
        if( firstIdx == lastIdx )
            return iterator( m_data ? m_data + firstIdx : nullptr );

        rebuildErase( firstIdx, lastIdx );
        return iterator( m_data + firstIdx );
    }

    template <class T, class A>
    void ArrayBase<T, A>::insert( size_t index, const T &value )
    {
        emplace( index, value );
    }

    template <class T, class A>
    template <typename U>
    typename std::enable_if<std::is_convertible<U, T>::value>::type ArrayBase<T, A>::insert(
        size_t index, const U &value )
    {
        emplace( index, value );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::iterator ArrayBase<T, A>::insert(
        typename ArrayBase<T, A>::const_iterator _Where, const T &_Val )
    {
        size_t pos = iteratorIndex( _Where, true, "ArrayBase::insert iterator out of range" );
        insert( pos, _Val );
        return iterator( m_data + pos );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::iterator ArrayBase<T, A>::insert(
        typename ArrayBase<T, A>::const_iterator _Where, T &&_Val )
    {
        size_t pos = iteratorIndex( _Where, true, "ArrayBase::insert iterator out of range" );
        size_t newSize = checkedAdd( m_size, 1, "ArrayBase::insert capacity overflow" );
        size_t newCapacity = newSize > m_capacity ? growthCapacity( newSize ) : m_capacity;

        rebuildWithInsert( pos, 1, newCapacity, [&]( T *dst, size_t ) {
            alloc_traits::construct( m_alloc, dst, std::move( _Val ) );
        } );

        return iterator( m_data + pos );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::iterator ArrayBase<T, A>::insert(
        typename ArrayBase<T, A>::const_iterator _Where, const size_type _Count, const T &_Val )
    {
        size_t pos = iteratorIndex( _Where, true, "ArrayBase::insert iterator out of range" );
        if( _Count == 0 )
            return iterator( m_data ? m_data + pos : nullptr );

        size_t newSize = checkedAdd( m_size, _Count, "ArrayBase::insert capacity overflow" );
        size_t newCapacity = newSize > m_capacity ? growthCapacity( newSize ) : m_capacity;

        rebuildWithInsert( pos, _Count, newCapacity,
                           [&]( T *dst, size_t ) { alloc_traits::construct( m_alloc, dst, _Val ); } );

        return iterator( m_data + pos );
    }

    template <class T, class A>
    template <class InputIt, typename>
    typename ArrayBase<T, A>::iterator ArrayBase<T, A>::insert(
        typename ArrayBase<T, A>::const_iterator _Where, InputIt first, InputIt last )
    {
        size_t pos = iteratorIndex( _Where, true, "ArrayBase::insert iterator out of range" );
        ArrayBase<T, A> values( first, last, m_alloc );
        size_t count = values.size();

        if( count == 0 )
            return iterator( m_data ? m_data + pos : nullptr );

        size_t newSize = checkedAdd( m_size, count, "ArrayBase::insert capacity overflow" );
        size_t newCapacity = newSize > m_capacity ? growthCapacity( newSize ) : m_capacity;

        rebuildWithInsert( pos, count, newCapacity, [&]( T *dst, size_t i ) {
            alloc_traits::construct( m_alloc, dst, std::move_if_noexcept( values.m_data[i] ) );
        } );

        return iterator( m_data + pos );
    }

    template <class T, class A>
    std::vector<T, A> ArrayBase<T, A>::snapshot() const
    {
        if( m_size == 0 )
            return std::vector<T, A>( m_alloc );

        return std::vector<T, A>( m_data, m_data + m_size, m_alloc );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::const_iterator ArrayBase<T, A>::cbegin() const
    {
        return const_iterator( m_data );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::const_iterator ArrayBase<T, A>::cend() const
    {
        return const_iterator( endPtr() );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::iterator ArrayBase<T, A>::begin()
    {
        return iterator( m_data );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::const_iterator ArrayBase<T, A>::begin() const
    {
        return const_iterator( m_data );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::iterator ArrayBase<T, A>::end()
    {
        return iterator( endPtr() );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::const_iterator ArrayBase<T, A>::end() const
    {
        return const_iterator( endPtr() );
    }

    template <class T, class A>
    typename ArrayBase<T, A>::reference ArrayBase<T, A>::front()
    {
        checkIndex( 0, "ArrayBase::front on empty array" );
        return m_data[0];
    }

    template <class T, class A>
    typename ArrayBase<T, A>::const_reference ArrayBase<T, A>::front() const
    {
        checkIndex( 0, "ArrayBase::front on empty array" );
        return m_data[0];
    }

    template <class T, class A>
    typename ArrayBase<T, A>::reference ArrayBase<T, A>::back()
    {
        checkIndex( m_size == 0 ? 0 : m_size - 1, "ArrayBase::back on empty array" );
        return m_data[m_size - 1];
    }

    template <class T, class A>
    typename ArrayBase<T, A>::const_reference ArrayBase<T, A>::back() const
    {
        checkIndex( m_size == 0 ? 0 : m_size - 1, "ArrayBase::back on empty array" );
        return m_data[m_size - 1];
    }

    template <class T, class A>
    T *ArrayBase<T, A>::data()
    {
        return m_data;
    }

    template <class T, class A>
    const T *ArrayBase<T, A>::data() const
    {
        return m_data;
    }

    template <class _Ty, class _Alloc = Allocator<_Ty>>
    using Array = ArrayBase<_Ty, _Alloc>;

}  // namespace workphone

#endif  // Array_h__
