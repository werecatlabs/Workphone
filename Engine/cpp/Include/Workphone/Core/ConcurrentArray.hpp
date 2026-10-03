#ifndef ConcurrentArray_h__
#define ConcurrentArray_h__

#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/Thread/ScopedLock.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Allocator.hpp>

namespace workphone
{

    // Forward declarations
    template <class T, class A>
    class ConcurrentArrayBaseConstIterator;

    /**
     * @brief Random access iterator wrapper for ConcurrentArrayBase.
     *
     * Iterators do not retain a hidden lock. Holding such a lock until iterator
     * destruction can deadlock code that waits for a writer while an iterator is
     * still in scope. Call ConcurrentArrayBase::lock()/unlock() around iteration,
     * or iterate over snapshot(), when concurrent mutation is possible.
     *
     * @tparam T Value type stored in the container.
     * @tparam A Allocator type used by the underlying Array.
     *
     * @note Iterator acquisition is synchronized, but the returned iterator can
     *       subsequently be invalidated by a concurrent mutation.
     *
     * @warning Users must not rely on the iterator keeping the container stable
     *          beyond the iterator object's lifetime; calling mutating member
     *          functions on the container while an iterator exists may still
     *          invalidate iterators depending on the operation (e.g., insert,
     *          erase, reserve).
     */
    template <class T, class A>
    class ConcurrentArrayBaseIterator
    {
    public:
        // Iterator traits (required for standard iterator compatibility)
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = typename Array<T, A>::difference_type;
        using pointer = T *;
        using reference = T &;
        using base_iter = typename Array<T, A>::iterator;

        // Retained for source compatibility with the original iterator wrapper.
        // Iterators no longer own or retain this mutex.
        mutable RecursiveSpinMutex *mutex = nullptr;
        base_iter it;

        ConcurrentArrayBaseIterator() = default;

        /**
         * @brief Construct an iterator directly from the protected container.
         *
         * The mutex is held only while reading begin/end from the underlying Array.
         */
        ConcurrentArrayBaseIterator( Array<T, A> *data, RecursiveSpinMutex *mutex, bool atEnd ) :
            mutex( nullptr )
        {
            if( mutex )
                mutex->lock_shared();

            it = atEnd ? data->end() : data->begin();

            if( mutex )
                mutex->unlock_shared();
        }

        /**
         * @brief Construct an iterator at the given base iterator position.
         * @param i Underlying Array iterator position.
         * @param mutex Compatibility parameter; no lifetime lock is retained.
         */
        ConcurrentArrayBaseIterator( base_iter i, RecursiveSpinMutex * ) : it( i )
        {
        }

        /**
         * @brief Destructor. No mutex ownership is associated with the iterator.
         */
        ~ConcurrentArrayBaseIterator() = default;

        /**
         * @brief Copy constructor for iterator conversion.
         *
         * Copies refer to the same underlying position and do not acquire a lock.
         */
        ConcurrentArrayBaseIterator( const ConcurrentArrayBaseIterator &other ) = default;
        ConcurrentArrayBaseIterator( ConcurrentArrayBaseIterator &&other ) noexcept = default;

        /**
         * @brief Assignment operator.
         */
        ConcurrentArrayBaseIterator &operator=( const ConcurrentArrayBaseIterator &other ) = default;
        ConcurrentArrayBaseIterator &operator=( ConcurrentArrayBaseIterator &&other ) noexcept = default;

        // Iterator operations
        reference operator*() const
        {
            return *it;
        }

        pointer operator->() const
        {
            return it.operator->();
        }

        // Prefix increment
        ConcurrentArrayBaseIterator &operator++()
        {
            ++it;
            return *this;
        }

        // Postfix increment
        ConcurrentArrayBaseIterator operator++( int )
        {
            ConcurrentArrayBaseIterator tmp = *this;
            ++it;
            return tmp;
        }

        // Prefix decrement
        ConcurrentArrayBaseIterator &operator--()
        {
            --it;
            return *this;
        }

        // Postfix decrement
        ConcurrentArrayBaseIterator operator--( int )
        {
            ConcurrentArrayBaseIterator tmp = *this;
            --it;
            return tmp;
        }

        // Random access operations
        ConcurrentArrayBaseIterator &operator+=( difference_type n )
        {
            it += n;
            return *this;
        }

        ConcurrentArrayBaseIterator &operator-=( difference_type n )
        {
            it -= n;
            return *this;
        }

        ConcurrentArrayBaseIterator operator+( difference_type n ) const
        {
            return ConcurrentArrayBaseIterator( it + n, mutex );
        }

        ConcurrentArrayBaseIterator operator-( difference_type n ) const
        {
            return ConcurrentArrayBaseIterator( it - n, mutex );
        }

        difference_type operator-( const ConcurrentArrayBaseIterator &other ) const
        {
            return it - other.it;
        }

        reference operator[]( difference_type n ) const
        {
            return it[n];
        }

        // Comparison operators
        bool operator==( const ConcurrentArrayBaseIterator &other ) const
        {
            return it == other.it;
        }

        bool operator!=( const ConcurrentArrayBaseIterator &other ) const
        {
            return it != other.it;
        }

        bool operator<( const ConcurrentArrayBaseIterator &other ) const
        {
            return it < other.it;
        }

        bool operator<=( const ConcurrentArrayBaseIterator &other ) const
        {
            return it <= other.it;
        }

        bool operator>( const ConcurrentArrayBaseIterator &other ) const
        {
            return it > other.it;
        }

        bool operator>=( const ConcurrentArrayBaseIterator &other ) const
        {
            return it >= other.it;
        }

        // Get the underlying iterator
        base_iter base() const
        {
            return it;
        }
    };

    // Non-member operators for ConcurrentArrayBaseIterator
    /**
     * @brief Random-access addition operator for iterator + n / n + iterator symmetry.
     */
    template <class T, class A>
    ConcurrentArrayBaseIterator<T, A> operator+(
        typename ConcurrentArrayBaseIterator<T, A>::difference_type n,
        const ConcurrentArrayBaseIterator<T, A> &it )
    {
        return it + n;
    }

    /**
     * @brief Const random access iterator wrapper for ConcurrentArrayBase.
     *
     * Behaves like ConcurrentArrayBaseIterator but provides const access to elements.
     *
     * @tparam T Value type stored in the container.
     * @tparam A Allocator type used by the underlying std::vector.
     */
    template <class T, class A>
    class ConcurrentArrayBaseConstIterator
    {
    public:
        // Iterator traits (required for standard iterator compatibility)
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = typename Array<T, A>::difference_type;
        using pointer = const T *;
        using reference = const T &;
        using base_iter = typename Array<T, A>::const_iterator;

        // Retained for source compatibility with the original iterator wrapper.
        // Iterators no longer own or retain this mutex.
        mutable RecursiveSpinMutex *mutex = nullptr;
        base_iter it;

        ConcurrentArrayBaseConstIterator() = default;

        /**
         * @brief Construct a const iterator directly from the protected container.
         *
         * The mutex is held only while reading cbegin/cend from the underlying Array.
         */
        ConcurrentArrayBaseConstIterator( const Array<T, A> *data, RecursiveSpinMutex *mutex,
                                          bool atEnd ) :
            mutex( nullptr )
        {
            if( mutex )
                mutex->lock_shared();

            it = atEnd ? data->cend() : data->cbegin();

            if( mutex )
                mutex->unlock_shared();
        }

        /**
         * @brief Construct a const iterator at the given base iterator position.
         * @param i Underlying Array const_iterator position.
         * @param mutex Compatibility parameter; no lifetime lock is retained.
         */
        ConcurrentArrayBaseConstIterator( base_iter i, RecursiveSpinMutex *mutex ) :
            mutex( nullptr ),
            it( i )
        {
            (void)mutex;
        }

        /**
         * @brief Conversion constructor from non-const iterator.
         *
         * Allows implicit conversion from ConcurrentArrayBaseIterator to
         * ConcurrentArrayBaseConstIterator without acquiring a lock.
         */
        ConcurrentArrayBaseConstIterator( const ConcurrentArrayBaseIterator<T, A> &other ) :
            mutex( nullptr ),
            it( other.base() )
        {
        }

        /**
         * @brief Destructor. No mutex ownership is associated with the iterator.
         */
        ~ConcurrentArrayBaseConstIterator() = default;

        /**
         * @brief Copy constructor for iterator conversion.
         *
         * Copies refer to the same underlying position and do not acquire a lock.
         */
        ConcurrentArrayBaseConstIterator( const ConcurrentArrayBaseConstIterator &other ) = default;

        /**
         * @brief Move constructor.
         */
        ConcurrentArrayBaseConstIterator( ConcurrentArrayBaseConstIterator &&other ) noexcept = default;

        /**
         * @brief Assignment operator.
         */
        ConcurrentArrayBaseConstIterator &operator=( const ConcurrentArrayBaseConstIterator &other ) =
            default;

        /**
         * @brief Move assignment operator.
         */
        ConcurrentArrayBaseConstIterator &operator=(
            ConcurrentArrayBaseConstIterator &&other ) noexcept = default;

        /**
         * @brief Assignment from non-const iterator.
         */
        ConcurrentArrayBaseConstIterator &operator=( const ConcurrentArrayBaseIterator<T, A> &other )
        {
            it = other.base();
            mutex = nullptr;
            return *this;
        }

        // Iterator operations
        reference operator*() const
        {
            return *it;
        }

        pointer operator->() const
        {
            return it.operator->();
        }

        // Prefix increment
        ConcurrentArrayBaseConstIterator &operator++()
        {
            ++it;
            return *this;
        }

        // Postfix increment
        ConcurrentArrayBaseConstIterator operator++( int )
        {
            ConcurrentArrayBaseConstIterator tmp = *this;
            ++it;
            return tmp;
        }

        // Prefix decrement
        ConcurrentArrayBaseConstIterator &operator--()
        {
            --it;
            return *this;
        }

        // Postfix decrement
        ConcurrentArrayBaseConstIterator operator--( int )
        {
            ConcurrentArrayBaseConstIterator tmp = *this;
            --it;
            return tmp;
        }

        // Random access operations
        ConcurrentArrayBaseConstIterator &operator+=( difference_type n )
        {
            it += n;
            return *this;
        }

        ConcurrentArrayBaseConstIterator &operator-=( difference_type n )
        {
            it -= n;
            return *this;
        }

        ConcurrentArrayBaseConstIterator operator+( difference_type n ) const
        {
            return ConcurrentArrayBaseConstIterator( it + n, mutex );
        }

        ConcurrentArrayBaseConstIterator operator-( difference_type n ) const
        {
            return ConcurrentArrayBaseConstIterator( it - n, mutex );
        }

        difference_type operator-( const ConcurrentArrayBaseConstIterator &other ) const
        {
            return it - other.it;
        }

        reference operator[]( difference_type n ) const
        {
            return it[n];
        }

        // Comparison operators
        bool operator==( const ConcurrentArrayBaseConstIterator &other ) const
        {
            return it == other.it;
        }

        bool operator!=( const ConcurrentArrayBaseConstIterator &other ) const
        {
            return it != other.it;
        }

        bool operator<( const ConcurrentArrayBaseConstIterator &other ) const
        {
            return it < other.it;
        }

        bool operator<=( const ConcurrentArrayBaseConstIterator &other ) const
        {
            return it <= other.it;
        }

        bool operator>( const ConcurrentArrayBaseConstIterator &other ) const
        {
            return it > other.it;
        }

        bool operator>=( const ConcurrentArrayBaseConstIterator &other ) const
        {
            return it >= other.it;
        }

        // Get the underlying iterator
        base_iter base() const
        {
            return it;
        }
    };

    // Non-member operators for ConcurrentArrayBaseConstIterator
    /**
     * @brief Random-access addition operator for const iterator symmetry (n + it).
     */
    template <class T, class A>
    ConcurrentArrayBaseConstIterator<T, A> operator+(
        typename ConcurrentArrayBaseConstIterator<T, A>::difference_type n,
        const ConcurrentArrayBaseConstIterator<T, A> &it )
    {
        return it + n;
    }

    /**
     * @brief Thread-safe dynamic array wrapper around std::vector.
     *
     * Provides a subset of std::vector's API with internal synchronization via
     * a RecursiveSpinMutex. Several operations acquire either an exclusive lock
     * (for mutating operations) or a shared/non-exclusive lock for read-only
     * operations. Iterators do not retain a lock; use manual locking or snapshot()
     * when iteration must be isolated from concurrent mutation.
     *
     * @tparam T Value type stored in the container.
     * @tparam A Allocator type used by the underlying Array (defaults to std::allocator<T>).
     *
     * @note The class aims to provide convenient thread-safety for common
     *       patterns, but users must still respect iterator invalidation rules
     *       of a dynamic array: operations that reallocate or erase elements may
     *       invalidate existing iterators.
     *
     * @warning References returned by element-access and emplacement functions
     *          are no longer protected after the function returns. Use the manual
     *          locking API while consuming such references if another thread may
     *          mutate the container.
     */
    template <class T, class A = std::allocator<T>>
    class ConcurrentArrayBase
    {
    public:
        typedef A allocator_type;
        typedef T value_type;
        typedef T &reference;
        typedef const T &const_reference;
        typedef typename A::difference_type difference_type;
        typedef typename A::size_type size_type;
        typedef ConcurrentArrayBaseIterator<T, A> iterator;
        typedef ConcurrentArrayBaseConstIterator<T, A> const_iterator;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /** @name Constructors and assignment
         * @{
         */

        /** @brief Default constructor. */
        ConcurrentArrayBase();

        /** @brief Configure automatic capacity growth. */
        explicit ConcurrentArrayBase( GrowthPolicy growthPolicy, size_type growthSize = 1,
                                      const allocator_type &a = allocator_type() ) :
            m_data( growthPolicy, growthSize, a )
        {
        }

        /** @brief Copy constructor. Performs a thread-safe copy of the data. */
        ConcurrentArrayBase( const ConcurrentArrayBase &other );

        /** @brief Initialize from initializer list. */
        ConcurrentArrayBase( std::initializer_list<value_type> il );

        /** @brief Construct from iterator range. */
        template <class I, typename = typename std::enable_if<!std::is_integral<I>::value>::type>
        ConcurrentArrayBase( I first, I last, const allocator_type &a = allocator_type() )
        {
            ScopedLock lock( this, true );  // Write lock for mutation
            m_data = Array<T, A>( first, last, a );
        }

        /** @brief Construct with count copies of value. */
        ConcurrentArrayBase( size_type count, const value_type &value,
                             const allocator_type &a = allocator_type() )
        {
            ScopedLock lock( this, true );
            m_data = Array<T, A>( count, value, a );
        }

        /** @brief Move constructor (thread-safe). */
        ConcurrentArrayBase( ConcurrentArrayBase &&other ) noexcept;

        /** @brief Move assignment (thread-safe). */
        ConcurrentArrayBase &operator=( ConcurrentArrayBase &&other ) noexcept;

        /** @brief Copy assignment (thread-safe). */
        ConcurrentArrayBase &operator=( const ConcurrentArrayBase &other );

        /** @} */

        /** @name Element insertion and construction
         * @{
         */

        /**
         * @brief Construct an element in-place at the end of the container.
         * @tparam Args Argument types forwarded to the element constructor.
         * @return Reference to the newly emplaced element.
         *
         * Thread-safety: exclusive lock is taken.
         */
        template <typename... Args>
        typename Array<T, A>::reference emplace_back( Args &&...args );

        /**
         * @brief Construct an element in-place at the specified index.
         * @param index Position at which to emplace. If index == size() the element is appended.
         * @tparam Args Argument types forwarded to the element constructor.
         *
         * Thread-safety: exclusive lock is taken.
         */
        template <class... Args>
        void emplace( size_t index, Args &&...args );

        /** @} */

        /** @name Search and ordering
         * @{
         */

        /**
         * @brief Find the index of the first element satisfying the predicate.
         * @param pred Unary predicate called for elements.
         * @return Index of the found element, or static_cast<size_t>(-1) if not found.
         *
         * Thread-safety: a non-exclusive/shared lock is used for the search.
         */
        template <class Pred>
        size_t find_if( Pred pred ) const;

        /**
         * @brief Sort the container using the provided comparison function.
         * @tparam Compare Comparison functor (defaults to std::less<T>).
         *
         * Thread-safety: exclusive lock is taken for the duration of the sort.
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

        GrowthPolicy getGrowthPolicy() const;
        void setGrowthPolicy( GrowthPolicy growthPolicy );
        size_type getGrowthSize() const;
        void setGrowthSize( size_type growthSize );

        void push_back( const T &value );
        void push_back( T &&value );

        /**
         * @brief Template overload to allow pushing values convertible to T.
         * @tparam U Type convertible to T.
         *
         * Thread-safety: exclusive lock is taken.
         */
        template <typename U>
        typename std::enable_if<std::is_convertible<U, T>::value>::type push_back( const U &value );

        void pop_back();
        void clear();
        void erase( size_t index );

        void erase( const T &p );

        void erase( typename ConcurrentArrayBase<T, A>::const_iterator it );
        void erase( typename ConcurrentArrayBase<T, A>::const_iterator first,
                    typename ConcurrentArrayBase<T, A>::const_iterator last );

        void insert( size_t index, const T &value );

        /**
         * @brief Template overload to allow inserting values convertible to T.
         * @tparam U Type convertible to T.
         */
        template <typename U>
        typename std::enable_if<std::is_convertible<U, T>::value>::type insert( size_t index,
                                                                                const U &value );

        typename ConcurrentArrayBase<T, A>::iterator insert(
            typename ConcurrentArrayBase<T, A>::const_iterator _Where, const T &_Val );
        typename ConcurrentArrayBase<T, A>::iterator insert(
            typename ConcurrentArrayBase<T, A>::const_iterator _Where, T &&_Val );
        typename ConcurrentArrayBase<T, A>::iterator insert(
            typename ConcurrentArrayBase<T, A>::const_iterator _Where, const size_type _Count,
            const T &_Val );

        // Additional insert overloads for iterator compatibility
        template <class InputIt>
        typename ConcurrentArrayBase<T, A>::iterator insert(
            typename ConcurrentArrayBase<T, A>::const_iterator _Where, InputIt first, InputIt last );

        /**
         * @brief Return a snapshot copy of the underlying Array.
         *
         * Thread-safety: a non-exclusive/shared lock is taken while copying.
         */
        Array<T, A> snapshot() const;

        /** @} */

        /** @name Iterators
         * @{
         */

        /**
         * @brief Get a const begin iterator. Iterator acquisition is synchronized,
         *        but the iterator does not retain the lock.
         */
        const_iterator cbegin() const;

        /**
         * @brief Get a const end iterator. Acquisition is synchronized only.
         */
        const_iterator cend() const;
        iterator begin();
        const_iterator begin() const;
        iterator end();
        const_iterator end() const;

        reverse_iterator rbegin()
        {
            return reverse_iterator( end() );
        }

        reverse_iterator rend()
        {
            return reverse_iterator( begin() );
        }

        const_reverse_iterator rbegin() const
        {
            return const_reverse_iterator( end() );
        }

        const_reverse_iterator rend() const
        {
            return const_reverse_iterator( begin() );
        }

        const_reverse_iterator crbegin() const
        {
            return const_reverse_iterator( end() );
        }

        const_reverse_iterator crend() const
        {
            return const_reverse_iterator( begin() );
        }

        /** @} */

        /** @name Front / Back access
         * @{
         */

        typename Array<T, A>::reference front();
        typename Array<T, A>::const_reference front() const;
        typename Array<T, A>::reference back();
        typename Array<T, A>::const_reference back() const;

        /** @} */

        /** @name Manual locking
         * @{
         */

        /**
         * @brief Acquire an exclusive lock on the container's mutex.
         *
         * Use when multiple operations must be performed atomically. Prefer RAII
         * via ScopedLock where possible.
         */
        void lock();

        /**
         * @brief Acquire a shared/non-exclusive lock on the container's mutex.
         */
        void lock_shared();

        /**
         * @brief Try to acquire an exclusive lock without blocking.
         * @return true if the exclusive lock was acquired.
         */
        bool try_lock();

        /**
         * @brief Try to acquire a shared/non-exclusive lock without blocking.
         * @return true if the shared lock was acquired.
         */
        bool try_lock_shared();

        /**
         * @brief Release an exclusive lock previously acquired.
         */
        void unlock();

        /**
         * @brief Release a shared/non-exclusive lock previously acquired.
         */
        void unlock_shared();

        /** @} */

    private:
        /** Underlying container storing the elements. Access protected by m_mutex. */
        Array<T, A> m_data;

        /** Recursive mutex used to synchronize access. */
        mutable RecursiveSpinMutex m_mutex;
    };

    template <class T, class A>
    ConcurrentArrayBase<T, A>::ConcurrentArrayBase() = default;

    template <class T, class A>
    ConcurrentArrayBase<T, A>::ConcurrentArrayBase( const ConcurrentArrayBase &other )
    {
        ScopedLock otherLock( &other, false );  // Read lock on source
        m_data = other.m_data;
    }

    template <class T, class A>
    ConcurrentArrayBase<T, A>::ConcurrentArrayBase( std::initializer_list<value_type> il )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data = il;
    }

    template <class T, class A>
    ConcurrentArrayBase<T, A>::ConcurrentArrayBase( ConcurrentArrayBase &&other ) noexcept
    {
        ScopedLock otherLock( &other, true );  // Write lock for move (source is modified)
        m_data = std::move( other.m_data );
    }

    template <class T, class A>
    ConcurrentArrayBase<T, A> &ConcurrentArrayBase<T, A>::operator=(
        ConcurrentArrayBase &&other ) noexcept
    {
        if( this != &other )
        {
            // Lock both mutexes in consistent order to avoid deadlock
            // Use address comparison to determine lock order
            if( std::less<const ConcurrentArrayBase *>()( this, &other ) )
            {
                ScopedLock lock( this, true );
                ScopedLock otherLock( &other, true );  // Write lock - source is modified
                m_data = std::move( other.m_data );
            }
            else
            {
                ScopedLock otherLock( &other, true );
                ScopedLock lock( this, true );
                m_data = std::move( other.m_data );
            }
        }
        return *this;
    }

    template <class T, class A>
    ConcurrentArrayBase<T, A> &ConcurrentArrayBase<T, A>::operator=( const ConcurrentArrayBase &other )
    {
        if( this != &other )
        {
            // Lock both mutexes in consistent order to avoid deadlock
            if( std::less<const ConcurrentArrayBase *>()( this, &other ) )
            {
                ScopedLock lock( this, true );          // Write lock for this
                ScopedLock otherLock( &other, false );  // Read lock for source
                m_data = other.m_data;
            }
            else
            {
                ScopedLock otherLock( &other, false );  // Read lock for source
                ScopedLock lock( this, true );          // Write lock for this
                m_data = other.m_data;
            }
        }

        return *this;
    }

    template <class T, class A>
    template <typename... Args>
    typename Array<T, A>::reference ConcurrentArrayBase<T, A>::emplace_back( Args &&...args )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        return m_data.emplace_back( std::forward<Args>( args )... );
    }

    template <class T, class A>
    template <class... Args>
    void ConcurrentArrayBase<T, A>::emplace( size_t index, Args &&...args )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        if( index <= m_data.size() )
            m_data.emplace( m_data.begin() + index, std::forward<Args>( args )... );
    }

    template <class T, class A>
    template <class Pred>
    size_t ConcurrentArrayBase<T, A>::find_if( Pred pred ) const
    {
        ScopedLock lock( this, false );  // Read lock for search
        auto it = std::find_if( m_data.begin(), m_data.end(), pred );
        return it != m_data.end() ? std::distance( m_data.begin(), it ) : static_cast<size_t>( -1 );
    }

    template <class T, class A>
    template <class Compare>
    void ConcurrentArrayBase<T, A>::sort( Compare comp )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        std::sort( m_data.begin(), m_data.end(), comp );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::const_reference ConcurrentArrayBase<T, A>::at(
        size_type index ) const
    {
        ScopedLock lock( this, false );  // Read lock for const access
        if( index >= m_data.size() )
        {
            throw std::out_of_range( "Index out of range" );
        }
        return m_data[index];
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::reference ConcurrentArrayBase<T, A>::at( size_type index )
    {
        ScopedLock lock( this, true );  // Write lock for non-const access
        if( index >= m_data.size() )
        {
            throw std::out_of_range( "Index out of range" );
        }

        return m_data[index];
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::reference ConcurrentArrayBase<T, A>::operator[](
        const difference_type index )
    {
        ScopedLock lock( this, true );  // Write lock for non-const access
        return m_data[index];
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::const_reference ConcurrentArrayBase<T, A>::operator[](
        const difference_type index ) const
    {
        ScopedLock lock( this, false );  // Read lock for const access
        return m_data[index];
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::resize( size_t newSize )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data.resize( newSize );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::resize( size_t newSize, const T &value )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data.resize( newSize, value );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::reserve( size_t newSize )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data.reserve( newSize );
    }

    template <class T, class A>
    size_t ConcurrentArrayBase<T, A>::size() const
    {
        ScopedLock lock( this, false );  // Read lock for const access
        return m_data.size();
    }

    template <class T, class A>
    size_t ConcurrentArrayBase<T, A>::capacity() const
    {
        ScopedLock lock( this, false );  // Read lock for const access
        return m_data.capacity();
    }

    template <class T, class A>
    bool ConcurrentArrayBase<T, A>::empty() const
    {
        ScopedLock lock( this, false );  // Read lock for const access
        return m_data.empty();
    }

    template <class T, class A>
    GrowthPolicy ConcurrentArrayBase<T, A>::getGrowthPolicy() const
    {
        ScopedLock lock( this, false );
        return m_data.getGrowthPolicy();
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::setGrowthPolicy( GrowthPolicy growthPolicy )
    {
        ScopedLock lock( this, true );
        m_data.setGrowthPolicy( growthPolicy );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::size_type ConcurrentArrayBase<T, A>::getGrowthSize() const
    {
        ScopedLock lock( this, false );
        return m_data.getGrowthSize();
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::setGrowthSize( size_type growthSize )
    {
        ScopedLock lock( this, true );
        m_data.setGrowthSize( growthSize );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::push_back( const T &value )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data.push_back( value );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::push_back( T &&value )
    {
        ScopedLock lock( this, true );
        m_data.push_back( std::move( value ) );
    }

    template <class T, class A>
    template <typename U>
    typename std::enable_if<std::is_convertible<U, T>::value>::type ConcurrentArrayBase<T, A>::push_back(
        const U &value )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data.push_back( static_cast<T>( value ) );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::pop_back()
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        if( !m_data.empty() )
            m_data.pop_back();
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::clear()
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data.clear();
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::erase( const T &p )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data.erase( p );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::erase( size_t index )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        if( index < m_data.size() )
            m_data.erase( m_data.begin() + index );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::erase( typename ConcurrentArrayBase<T, A>::const_iterator it )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        auto baseIt = it.base();
        if( baseIt != m_data.end() )
            m_data.erase( baseIt );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::erase( typename ConcurrentArrayBase<T, A>::const_iterator first,
                                           typename ConcurrentArrayBase<T, A>::const_iterator last )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        m_data.erase( first.base(), last.base() );
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::insert( size_t index, const T &value )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        if( index <= m_data.size() )
            m_data.insert( m_data.begin() + index, value );
    }

    template <class T, class A>
    template <typename U>
    typename std::enable_if<std::is_convertible<U, T>::value>::type ConcurrentArrayBase<T, A>::insert(
        size_t index, const U &value )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        if( index <= m_data.size() )
            m_data.insert( m_data.begin() + index, static_cast<T>( value ) );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::iterator ConcurrentArrayBase<T, A>::insert(
        typename ConcurrentArrayBase<T, A>::const_iterator _Where, const T &_Val )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        return ConcurrentArrayBase<T, A>::iterator( m_data.insert( _Where.base(), _Val ), nullptr );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::iterator ConcurrentArrayBase<T, A>::insert(
        typename ConcurrentArrayBase<T, A>::const_iterator _Where, T &&_Val )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        return ConcurrentArrayBase<T, A>::iterator( m_data.insert( _Where.base(), std::move( _Val ) ),
                                                    nullptr );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::iterator ConcurrentArrayBase<T, A>::insert(
        typename ConcurrentArrayBase<T, A>::const_iterator _Where, const size_type _Count,
        const T &_Val )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        return ConcurrentArrayBase<T, A>::iterator( m_data.insert( _Where.base(), _Count, _Val ),
                                                    nullptr );
    }

    template <class T, class A>
    template <class InputIt>
    typename ConcurrentArrayBase<T, A>::iterator ConcurrentArrayBase<T, A>::insert(
        typename ConcurrentArrayBase<T, A>::const_iterator _Where, InputIt first, InputIt last )
    {
        ScopedLock lock( this, true );  // Write lock for mutation
        return ConcurrentArrayBase<T, A>::iterator( m_data.insert( _Where.base(), first, last ),
                                                    nullptr );
    }

    template <class T, class A>
    Array<T, A> ConcurrentArrayBase<T, A>::snapshot() const
    {
        ScopedLock lock( this, false );  // Read lock for copying
        return m_data;
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::const_iterator ConcurrentArrayBase<T, A>::cbegin() const
    {
        return const_iterator( &m_data, &m_mutex, false );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::const_iterator ConcurrentArrayBase<T, A>::cend() const
    {
        return const_iterator( &m_data, &m_mutex, true );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::iterator ConcurrentArrayBase<T, A>::begin()
    {
        return iterator( &m_data, &m_mutex, false );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::const_iterator ConcurrentArrayBase<T, A>::begin() const
    {
        return const_iterator( &m_data, &m_mutex, false );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::iterator ConcurrentArrayBase<T, A>::end()
    {
        return iterator( &m_data, &m_mutex, true );
    }

    template <class T, class A>
    typename ConcurrentArrayBase<T, A>::const_iterator ConcurrentArrayBase<T, A>::end() const
    {
        return const_iterator( &m_data, &m_mutex, true );
    }

    template <class T, class A>
    typename Array<T, A>::reference ConcurrentArrayBase<T, A>::front()
    {
        ScopedLock lock( this, true );  // Write lock for non-const access
        return m_data.front();
    }

    template <class T, class A>
    typename Array<T, A>::const_reference ConcurrentArrayBase<T, A>::front() const
    {
        ScopedLock lock( this, false );  // Read lock for const access
        return m_data.front();
    }

    template <class T, class A>
    typename Array<T, A>::reference ConcurrentArrayBase<T, A>::back()
    {
        ScopedLock lock( this, true );  // Write lock for non-const access
        return m_data.back();
    }

    template <class T, class A>
    typename Array<T, A>::const_reference ConcurrentArrayBase<T, A>::back() const
    {
        ScopedLock lock( this, false );  // Read lock for const access
        return m_data.back();
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::lock()
    {
        m_mutex.lock();
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::lock_shared()
    {
        m_mutex.lock_shared();
    }

    template <class T, class A>
    bool ConcurrentArrayBase<T, A>::try_lock()
    {
        return m_mutex.try_lock();
    }

    template <class T, class A>
    bool ConcurrentArrayBase<T, A>::try_lock_shared()
    {
        return m_mutex.try_lock_shared();
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::unlock()
    {
        m_mutex.unlock();
    }

    template <class T, class A>
    void ConcurrentArrayBase<T, A>::unlock_shared()
    {
        m_mutex.unlock_shared();
    }

    template <class _Ty, class _Alloc = Allocator<_Ty>>
    using ConcurrentArray = ConcurrentArrayBase<_Ty, _Alloc>;

}  // namespace workphone

#endif  // ConcurrentArray_h__
