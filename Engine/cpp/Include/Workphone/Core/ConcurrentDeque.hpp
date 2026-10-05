#ifndef ConcurrentDeque_h__
#define ConcurrentDeque_h__

#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/Core/Deque.hpp>

#include <Workphone/WorkphoneEnums.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace workphone
{
    template <class T, class A = std::allocator<T>>
    class ConcurrentDeque;

    template <class T, class A>
    class ConcurrentDequeConstIterator;

    enum class ConcurrentDequeIteratorLockMode
    {
        None,
        Exclusive,
        Shared
    };

    /**
     * @brief Random-access iterator for ConcurrentDeque.
     *
     * Iterators returned directly by ConcurrentDeque::begin() acquire an exclusive
     * lock and keep it for their lifetime. end() also retains a lock, so
     * either argument evaluation order produces one protected range.
     * Iterators returned by WriteLockedView do not acquire an additional lock,
     * because the view already owns the exclusive lock.
     */
    template <class T, class A>
    class ConcurrentDequeIterator
    {
    public:
        using owner_type = ConcurrentDeque<T, A>;
        using allocator_traits = std::allocator_traits<A>;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = typename allocator_traits::difference_type;
        using size_type = typename allocator_traits::size_type;
        using pointer = T *;
        using reference = T &;

        ConcurrentDequeIterator() noexcept = default;

        ConcurrentDequeIterator( const ConcurrentDequeIterator &other ) :
            m_owner( other.m_owner ),
            m_index( other.m_index ),
            m_lockMode( other.m_lockMode )
        {
            acquire();
        }

        ConcurrentDequeIterator( ConcurrentDequeIterator &&other ) noexcept :
            m_owner( other.m_owner ),
            m_index( other.m_index ),
            m_lockMode( other.m_lockMode )
        {
            other.m_owner = nullptr;
            other.m_index = 0;
            other.m_lockMode = ConcurrentDequeIteratorLockMode::None;
        }

        ~ConcurrentDequeIterator()
        {
            release();
        }

        ConcurrentDequeIterator &operator=( const ConcurrentDequeIterator &other )
        {
            if( this != &other )
            {
                release();
                m_owner = other.m_owner;
                m_index = other.m_index;
                m_lockMode = other.m_lockMode;
                acquire();
            }
            return *this;
        }

        ConcurrentDequeIterator &operator=( ConcurrentDequeIterator &&other ) noexcept
        {
            if( this != &other )
            {
                release();
                m_owner = other.m_owner;
                m_index = other.m_index;
                m_lockMode = other.m_lockMode;

                other.m_owner = nullptr;
                other.m_index = 0;
                other.m_lockMode = ConcurrentDequeIteratorLockMode::None;
            }
            return *this;
        }

        reference operator*() const
        {
            assertDereferenceable();
            return m_owner->elementAtLogicalIndexUnlocked( m_index )[0];
        }

        pointer operator->() const
        {
            return std::addressof( operator*() );
        }

        ConcurrentDequeIterator &operator++()
        {
            ++m_index;
            return *this;
        }

        ConcurrentDequeIterator operator++( int )
        {
            ConcurrentDequeIterator tmp( *this );
            ++( *this );
            return tmp;
        }

        ConcurrentDequeIterator &operator--()
        {
            --m_index;
            return *this;
        }

        ConcurrentDequeIterator operator--( int )
        {
            ConcurrentDequeIterator tmp( *this );
            --( *this );
            return tmp;
        }

        ConcurrentDequeIterator &operator+=( difference_type n )
        {
            m_index = static_cast<size_type>( static_cast<difference_type>( m_index ) + n );
            return *this;
        }

        ConcurrentDequeIterator &operator-=( difference_type n )
        {
            return ( *this += -n );
        }

        ConcurrentDequeIterator operator+( difference_type n ) const
        {
            ConcurrentDequeIterator tmp( *this );
            tmp += n;
            return tmp;
        }

        ConcurrentDequeIterator operator-( difference_type n ) const
        {
            ConcurrentDequeIterator tmp( *this );
            tmp -= n;
            return tmp;
        }

        difference_type operator-( const ConcurrentDequeIterator &other ) const
        {
            assert( m_owner == other.m_owner );
            return static_cast<difference_type>( m_index ) -
                   static_cast<difference_type>( other.m_index );
        }

        reference operator[]( difference_type n ) const
        {
            return *( *this + n );
        }

        bool operator==( const ConcurrentDequeIterator &other ) const noexcept
        {
            return m_owner == other.m_owner && m_index == other.m_index;
        }

        bool operator!=( const ConcurrentDequeIterator &other ) const noexcept
        {
            return !( *this == other );
        }

        bool operator<( const ConcurrentDequeIterator &other ) const
        {
            assert( m_owner == other.m_owner );
            return m_index < other.m_index;
        }

        bool operator<=( const ConcurrentDequeIterator &other ) const
        {
            return !( other < *this );
        }

        bool operator>( const ConcurrentDequeIterator &other ) const
        {
            return other < *this;
        }

        bool operator>=( const ConcurrentDequeIterator &other ) const
        {
            return !( *this < other );
        }

    private:
        friend class ConcurrentDeque<T, A>;
        friend class ConcurrentDequeConstIterator<T, A>;

        ConcurrentDequeIterator( owner_type *owner, size_type index,
                                 ConcurrentDequeIteratorLockMode lockMode ) :
            m_owner( owner ),
            m_index( index ),
            m_lockMode( lockMode )
        {
            acquire();
        }

        void acquire()
        {
            if( !m_owner )
            {
                return;
            }

            if( m_lockMode == ConcurrentDequeIteratorLockMode::Exclusive )
            {
                m_owner->lock();
            }
            else if( m_lockMode == ConcurrentDequeIteratorLockMode::Shared )
            {
                m_owner->lock_shared();
            }
        }

        void release() noexcept
        {
            if( !m_owner )
            {
                return;
            }

            if( m_lockMode == ConcurrentDequeIteratorLockMode::Exclusive )
            {
                m_owner->unlock();
            }
            else if( m_lockMode == ConcurrentDequeIteratorLockMode::Shared )
            {
                m_owner->unlock_shared();
            }

            m_owner = nullptr;
            m_lockMode = ConcurrentDequeIteratorLockMode::None;
        }

        void assertDereferenceable() const
        {
            assert( m_owner );
            assert( m_index < m_owner->m_size );
        }

        owner_type *m_owner = nullptr;
        size_type m_index = 0;
        ConcurrentDequeIteratorLockMode m_lockMode = ConcurrentDequeIteratorLockMode::None;
    };

    template <class T, class A>
    ConcurrentDequeIterator<T, A> operator+( typename ConcurrentDequeIterator<T, A>::difference_type n,
                                             const ConcurrentDequeIterator<T, A> &it )
    {
        return it + n;
    }

    /**
     * @brief Const random-access iterator for ConcurrentDeque.
     *
     * Const iterators returned directly by cbegin()/begin() hold a shared lock
     * for their lifetime. A const iterator converted from a mutable iterator
     * preserves the iterator's existing exclusive lock mode.
     */
    template <class T, class A>
    class ConcurrentDequeConstIterator
    {
    public:
        using owner_type = ConcurrentDeque<T, A>;
        using allocator_traits = std::allocator_traits<A>;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = typename allocator_traits::difference_type;
        using size_type = typename allocator_traits::size_type;
        using pointer = const T *;
        using reference = const T &;

        ConcurrentDequeConstIterator() noexcept = default;

        ConcurrentDequeConstIterator( const ConcurrentDequeIterator<T, A> &other ) :
            m_owner( other.m_owner ),
            m_index( other.m_index ),
            m_lockMode( other.m_lockMode )
        {
            acquire();
        }

        ConcurrentDequeConstIterator( const ConcurrentDequeConstIterator &other ) :
            m_owner( other.m_owner ),
            m_index( other.m_index ),
            m_lockMode( other.m_lockMode )
        {
            acquire();
        }

        ConcurrentDequeConstIterator( ConcurrentDequeConstIterator &&other ) noexcept :
            m_owner( other.m_owner ),
            m_index( other.m_index ),
            m_lockMode( other.m_lockMode )
        {
            other.m_owner = nullptr;
            other.m_index = 0;
            other.m_lockMode = ConcurrentDequeIteratorLockMode::None;
        }

        ~ConcurrentDequeConstIterator()
        {
            release();
        }

        ConcurrentDequeConstIterator &operator=( const ConcurrentDequeConstIterator &other )
        {
            if( this != &other )
            {
                release();
                m_owner = other.m_owner;
                m_index = other.m_index;
                m_lockMode = other.m_lockMode;
                acquire();
            }
            return *this;
        }

        ConcurrentDequeConstIterator &operator=( ConcurrentDequeConstIterator &&other ) noexcept
        {
            if( this != &other )
            {
                release();
                m_owner = other.m_owner;
                m_index = other.m_index;
                m_lockMode = other.m_lockMode;

                other.m_owner = nullptr;
                other.m_index = 0;
                other.m_lockMode = ConcurrentDequeIteratorLockMode::None;
            }
            return *this;
        }

        ConcurrentDequeConstIterator &operator=( const ConcurrentDequeIterator<T, A> &other )
        {
            release();
            m_owner = other.m_owner;
            m_index = other.m_index;
            m_lockMode = other.m_lockMode;
            acquire();
            return *this;
        }

        reference operator*() const
        {
            assertDereferenceable();
            return *m_owner->elementAtLogicalIndexUnlocked( m_index );
        }

        pointer operator->() const
        {
            return std::addressof( operator*() );
        }

        ConcurrentDequeConstIterator &operator++()
        {
            ++m_index;
            return *this;
        }

        ConcurrentDequeConstIterator operator++( int )
        {
            ConcurrentDequeConstIterator tmp( *this );
            ++( *this );
            return tmp;
        }

        ConcurrentDequeConstIterator &operator--()
        {
            --m_index;
            return *this;
        }

        ConcurrentDequeConstIterator operator--( int )
        {
            ConcurrentDequeConstIterator tmp( *this );
            --( *this );
            return tmp;
        }

        ConcurrentDequeConstIterator &operator+=( difference_type n )
        {
            m_index = static_cast<size_type>( static_cast<difference_type>( m_index ) + n );
            return *this;
        }

        ConcurrentDequeConstIterator &operator-=( difference_type n )
        {
            return ( *this += -n );
        }

        ConcurrentDequeConstIterator operator+( difference_type n ) const
        {
            ConcurrentDequeConstIterator tmp( *this );
            tmp += n;
            return tmp;
        }

        ConcurrentDequeConstIterator operator-( difference_type n ) const
        {
            ConcurrentDequeConstIterator tmp( *this );
            tmp -= n;
            return tmp;
        }

        difference_type operator-( const ConcurrentDequeConstIterator &other ) const
        {
            assert( m_owner == other.m_owner );
            return static_cast<difference_type>( m_index ) -
                   static_cast<difference_type>( other.m_index );
        }

        reference operator[]( difference_type n ) const
        {
            return *( *this + n );
        }

        bool operator==( const ConcurrentDequeConstIterator &other ) const noexcept
        {
            return m_owner == other.m_owner && m_index == other.m_index;
        }

        bool operator!=( const ConcurrentDequeConstIterator &other ) const noexcept
        {
            return !( *this == other );
        }

        bool operator<( const ConcurrentDequeConstIterator &other ) const
        {
            assert( m_owner == other.m_owner );
            return m_index < other.m_index;
        }

        bool operator<=( const ConcurrentDequeConstIterator &other ) const
        {
            return !( other < *this );
        }

        bool operator>( const ConcurrentDequeConstIterator &other ) const
        {
            return other < *this;
        }

        bool operator>=( const ConcurrentDequeConstIterator &other ) const
        {
            return !( *this < other );
        }

    private:
        friend class ConcurrentDeque<T, A>;

        ConcurrentDequeConstIterator( const owner_type *owner, size_type index,
                                      ConcurrentDequeIteratorLockMode lockMode ) :
            m_owner( owner ),
            m_index( index ),
            m_lockMode( lockMode )
        {
            acquire();
        }

        void acquire()
        {
            if( !m_owner )
            {
                return;
            }

            if( m_lockMode == ConcurrentDequeIteratorLockMode::Exclusive )
            {
                const_cast<owner_type *>( m_owner )->lock();
            }
            else if( m_lockMode == ConcurrentDequeIteratorLockMode::Shared )
            {
                const_cast<owner_type *>( m_owner )->lock_shared();
            }
        }

        void release() noexcept
        {
            if( !m_owner )
            {
                return;
            }

            if( m_lockMode == ConcurrentDequeIteratorLockMode::Exclusive )
            {
                const_cast<owner_type *>( m_owner )->unlock();
            }
            else if( m_lockMode == ConcurrentDequeIteratorLockMode::Shared )
            {
                const_cast<owner_type *>( m_owner )->unlock_shared();
            }

            m_owner = nullptr;
            m_lockMode = ConcurrentDequeIteratorLockMode::None;
        }

        void assertDereferenceable() const
        {
            assert( m_owner );
            assert( m_index < m_owner->m_size );
        }

        const owner_type *m_owner = nullptr;
        size_type m_index = 0;
        ConcurrentDequeIteratorLockMode m_lockMode = ConcurrentDequeIteratorLockMode::None;
    };

    template <class T, class A>
    ConcurrentDequeConstIterator<T, A> operator+(
        typename ConcurrentDequeConstIterator<T, A>::difference_type n,
        const ConcurrentDequeConstIterator<T, A> &it )
    {
        return it + n;
    }

    /**
     * @brief Thread-safe double-ended queue using a contiguous circular buffer.
     *
     * Raw storage is preallocated. T is constructed only for occupied slots.
     * push_front/push_back/pop_front/pop_back are allocation-free while spare
     * capacity exists. Explicit reserve() works even in Fixed mode.
     */
    template <class T, class A>
    class ConcurrentDeque
    {
    public:
        using allocator_type = A;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using value_type = T;
        using reference = T &;
        using const_reference = const T &;
        using difference_type = typename allocator_traits::difference_type;
        using size_type = typename allocator_traits::size_type;
        using iterator = ConcurrentDequeIterator<T, A>;
        using const_iterator = ConcurrentDequeConstIterator<T, A>;
        using mutex_type = RecursiveSpinMutex;

        static constexpr size_type DefaultGrowthSize = 64;

        class ReadLockedView
        {
        public:
            explicit ReadLockedView( const ConcurrentDeque &owner ) : m_owner( &owner )
            {
                m_owner->lock_shared();
            }

            ReadLockedView( const ReadLockedView & ) = delete;
            ReadLockedView &operator=( const ReadLockedView & ) = delete;

            ReadLockedView( ReadLockedView &&other ) noexcept : m_owner( other.m_owner )
            {
                other.m_owner = nullptr;
            }

            ReadLockedView &operator=( ReadLockedView &&other ) noexcept
            {
                if( this != &other )
                {
                    release();
                    m_owner = other.m_owner;
                    other.m_owner = nullptr;
                }
                return *this;
            }

            ~ReadLockedView()
            {
                release();
            }

            const_iterator begin() const noexcept
            {
                return const_iterator( m_owner, 0, ConcurrentDequeIteratorLockMode::None );
            }

            const_iterator end() const noexcept
            {
                return const_iterator( m_owner, m_owner->m_size, ConcurrentDequeIteratorLockMode::None );
            }

            size_type size() const noexcept
            {
                return m_owner->m_size;
            }

            bool empty() const noexcept
            {
                return m_owner->m_size == 0;
            }

            const_reference operator[]( size_type index ) const
            {
                assert( index < m_owner->m_size );
                return *m_owner->elementAtLogicalIndexUnlocked( index );
            }

            const_reference at( size_type index ) const
            {
                if( index >= m_owner->m_size )
                {
                    throw std::out_of_range( "ConcurrentDeque::ReadLockedView::at" );
                }
                return *m_owner->elementAtLogicalIndexUnlocked( index );
            }

            const_reference front() const
            {
                if( m_owner->m_size == 0 )
                {
                    throw std::out_of_range( "ConcurrentDeque::ReadLockedView::front on empty deque" );
                }
                return *m_owner->elementAtLogicalIndexUnlocked( 0 );
            }

            const_reference back() const
            {
                if( m_owner->m_size == 0 )
                {
                    throw std::out_of_range( "ConcurrentDeque::ReadLockedView::back on empty deque" );
                }
                return *m_owner->elementAtLogicalIndexUnlocked( m_owner->m_size - 1 );
            }

        private:
            void release() noexcept
            {
                if( m_owner )
                {
                    m_owner->unlock_shared();
                    m_owner = nullptr;
                }
            }

            const ConcurrentDeque *m_owner = nullptr;
        };

        class WriteLockedView
        {
        public:
            explicit WriteLockedView( ConcurrentDeque &owner ) : m_owner( &owner )
            {
                m_owner->lock();
            }

            WriteLockedView( const WriteLockedView & ) = delete;
            WriteLockedView &operator=( const WriteLockedView & ) = delete;

            WriteLockedView( WriteLockedView &&other ) noexcept : m_owner( other.m_owner )
            {
                other.m_owner = nullptr;
            }

            WriteLockedView &operator=( WriteLockedView &&other ) noexcept
            {
                if( this != &other )
                {
                    release();
                    m_owner = other.m_owner;
                    other.m_owner = nullptr;
                }
                return *this;
            }

            ~WriteLockedView()
            {
                release();
            }

            iterator begin() noexcept
            {
                return iterator( m_owner, 0, ConcurrentDequeIteratorLockMode::None );
            }

            iterator end() noexcept
            {
                return iterator( m_owner, m_owner->m_size, ConcurrentDequeIteratorLockMode::None );
            }

            size_type size() const noexcept
            {
                return m_owner->m_size;
            }

            bool empty() const noexcept
            {
                return m_owner->m_size == 0;
            }

            reference operator[]( size_type index )
            {
                assert( index < m_owner->m_size );
                return *m_owner->elementAtLogicalIndexUnlocked( index );
            }

            reference at( size_type index )
            {
                if( index >= m_owner->m_size )
                {
                    throw std::out_of_range( "ConcurrentDeque::WriteLockedView::at" );
                }
                return *m_owner->elementAtLogicalIndexUnlocked( index );
            }

            reference front()
            {
                if( m_owner->m_size == 0 )
                {
                    throw std::out_of_range( "ConcurrentDeque::WriteLockedView::front on empty deque" );
                }
                return *m_owner->elementAtLogicalIndexUnlocked( 0 );
            }

            reference back()
            {
                if( m_owner->m_size == 0 )
                {
                    throw std::out_of_range( "ConcurrentDeque::WriteLockedView::back on empty deque" );
                }
                return *m_owner->elementAtLogicalIndexUnlocked( m_owner->m_size - 1 );
            }

            template <class... Args>
            void emplace_back( Args &&...args )
            {
                m_owner->emplaceBackUnlocked( std::forward<Args>( args )... );
            }

            template <class... Args>
            void emplace_front( Args &&...args )
            {
                m_owner->emplaceFrontUnlocked( std::forward<Args>( args )... );
            }

            void pop_back()
            {
                m_owner->popBackUnlocked();
            }

            void pop_front()
            {
                m_owner->popFrontUnlocked();
            }

            void erase( size_type index )
            {
                m_owner->eraseIndexUnlocked( index );
            }

            void clear() noexcept
            {
                m_owner->destroyElementsUnlocked();
            }

        private:
            void release() noexcept
            {
                if( m_owner )
                {
                    m_owner->unlock();
                    m_owner = nullptr;
                }
            }

            ConcurrentDeque *m_owner = nullptr;
        };

        ConcurrentDeque() = default;

        explicit ConcurrentDeque( size_type initialCapacity,
                                  GrowthPolicy growthPolicy = GrowthPolicy::Double,
                                  size_type growthSize = DefaultGrowthSize,
                                  const allocator_type &allocator = allocator_type() ) :
            m_allocator( allocator ),
            m_growthPolicy( growthPolicy ),
            m_growthSize( growthSize )
        {
            validateGrowthSize( m_growthSize );
            if( initialCapacity > 0 )
            {
                reserveUnlocked( initialCapacity );
            }
        }

        ConcurrentDeque( std::initializer_list<value_type> il )
        {
            if( il.size() != 0 )
            {
                reserveUnlocked( static_cast<size_type>( il.size() ) );
                for( const auto &value : il )
                {
                    emplaceBackUnlocked( value );
                }
            }
        }

        template <class I, class = std::enable_if_t<!std::is_integral<I>::value>>
        ConcurrentDeque( I first, I last )
        {
            for( ; first != last; ++first )
            {
                emplaceBackUnlocked( *first );
            }
        }

        ConcurrentDeque( const ConcurrentDeque &other ) :
            ConcurrentDeque( other, std::unique_lock<mutex_type>( other.m_mutex ) ) {}

        ConcurrentDeque( ConcurrentDeque &&other ) :
            ConcurrentDeque( other, std::unique_lock<mutex_type>( other.m_mutex ), 0 ) {}

        ~ConcurrentDeque()
        {
            releaseStorageUnlocked();
        }

        ConcurrentDeque &operator=( const ConcurrentDeque &other )
        {
            if( this == &other )
            {
                return *this;
            }

            lockBothForCopy( other );

            try
            {
                T *newData = nullptr;
                size_type constructed = 0;

                try
                {
                    if( other.m_capacity > 0 )
                    {
                        newData = allocator_traits::allocate( m_allocator, other.m_capacity );
                        for( ; constructed < other.m_size; ++constructed )
                        {
                            allocator_traits::construct(
                                m_allocator, newData + constructed,
                                *other.elementAtLogicalIndexUnlocked( constructed ) );
                        }
                    }
                }
                catch( ... )
                {
                    destroyContiguousRange( newData, constructed );
                    if( newData )
                    {
                        allocator_traits::deallocate( m_allocator, newData, other.m_capacity );
                    }
                    throw;
                }

                releaseStorageUnlocked();
                m_data = newData;
                m_capacity = other.m_capacity;
                m_size = other.m_size;
                m_head = 0;
                m_growthPolicy = other.m_growthPolicy;
                m_growthSize = other.m_growthSize;

                unlockBothForCopy( other );
            }
            catch( ... )
            {
                unlockBothForCopy( other );
                throw;
            }

            return *this;
        }

        ConcurrentDeque &operator=( ConcurrentDeque &&other )
        {
            if( this == &other )
            {
                return *this;
            }

            lockBothExclusive( other );
            try
            {
                moveAssignUnlocked( other );
                unlockBothExclusive( other );
            }
            catch( ... )
            {
                unlockBothExclusive( other );
                throw;
            }
            return *this;
        }

        // ------------------------------------------------------------------
        // Capacity / policy
        // ------------------------------------------------------------------

        size_type size() const
        {
            lock_shared();
            const size_type value = m_size;
            unlock_shared();
            return value;
        }

        bool empty() const
        {
            return size() == 0;
        }

        size_type capacity() const
        {
            lock_shared();
            const size_type value = m_capacity;
            unlock_shared();
            return value;
        }

        size_type available() const
        {
            lock_shared();
            const size_type value = m_capacity - m_size;
            unlock_shared();
            return value;
        }

        size_type max_size() const
        {
            lock_shared();
            const size_type value = allocator_traits::max_size( m_allocator );
            unlock_shared();
            return value;
        }

        void reserve( size_type requestedCapacity )
        {
            lock();
            try
            {
                reserveUnlocked( requestedCapacity );
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        GrowthPolicy getGrowthPolicy() const
        {
            lock_shared();
            const auto value = m_growthPolicy;
            unlock_shared();
            return value;
        }

        void setGrowthPolicy( GrowthPolicy policy )
        {
            lock();
            m_growthPolicy = policy;
            unlock();
        }

        size_type getGrowthSize() const
        {
            lock_shared();
            const size_type value = m_growthSize;
            unlock_shared();
            return value;
        }

        void setGrowthSize( size_type growthSize )
        {
            validateGrowthSize( growthSize );
            lock();
            m_growthSize = growthSize;
            unlock();
        }

        // ------------------------------------------------------------------
        // Insertion
        // ------------------------------------------------------------------

        void push_back( const T &value )
        {
            emplace_back( value );
        }

        void push_back( T &&value )
        {
            emplace_back( std::move( value ) );
        }

        void push_front( const T &value )
        {
            emplace_front( value );
        }

        void push_front( T &&value )
        {
            emplace_front( std::move( value ) );
        }

        template <class... Args>
        void emplace_back( Args &&...args )
        {
            lock();
            try
            {
                emplaceBackUnlocked( std::forward<Args>( args )... );
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        template <class... Args>
        void emplace_front( Args &&...args )
        {
            lock();
            try
            {
                emplaceFrontUnlocked( std::forward<Args>( args )... );
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        bool try_push_back( const T &value )
        {
            return try_emplace_back( value );
        }

        bool try_push_back( T &&value )
        {
            return try_emplace_back( std::move( value ) );
        }

        bool try_push_front( const T &value )
        {
            return try_emplace_front( value );
        }

        bool try_push_front( T &&value )
        {
            return try_emplace_front( std::move( value ) );
        }

        template <class... Args>
        bool try_emplace_back( Args &&...args )
        {
            lock();
            if( m_size == m_capacity && m_growthPolicy == GrowthPolicy::Fixed )
            {
                unlock();
                return false;
            }

            try
            {
                emplaceBackUnlocked( std::forward<Args>( args )... );
                unlock();
                return true;
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        template <class... Args>
        bool try_emplace_front( Args &&...args )
        {
            lock();
            if( m_size == m_capacity && m_growthPolicy == GrowthPolicy::Fixed )
            {
                unlock();
                return false;
            }

            try
            {
                emplaceFrontUnlocked( std::forward<Args>( args )... );
                unlock();
                return true;
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        // ------------------------------------------------------------------
        // Removal
        // ------------------------------------------------------------------

        void pop_front()
        {
            lock();
            try
            {
                popFrontUnlocked();
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        void pop_back()
        {
            lock();
            try
            {
                popBackUnlocked();
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        bool try_pop_front()
        {
            lock();
            if( m_size == 0 )
            {
                unlock();
                return false;
            }
            popFrontUnlocked();
            unlock();
            return true;
        }

        bool try_pop_back()
        {
            lock();
            if( m_size == 0 )
            {
                unlock();
                return false;
            }
            popBackUnlocked();
            unlock();
            return true;
        }

        bool try_pop_front( T &outValue )
        {
            lock();
            if( m_size == 0 )
            {
                unlock();
                return false;
            }

            try
            {
                assignForPop( outValue, *elementAtLogicalIndexUnlocked( 0 ) );
                popFrontUnlocked();
                unlock();
                return true;
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        bool try_pop_back( T &outValue )
        {
            lock();
            if( m_size == 0 )
            {
                unlock();
                return false;
            }

            try
            {
                assignForPop( outValue, *elementAtLogicalIndexUnlocked( m_size - 1 ) );
                popBackUnlocked();
                unlock();
                return true;
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        void erase( size_type index )
        {
            lock();
            try
            {
                eraseIndexUnlocked( index );
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        void erase( const T &value )
        {
            lock();
            try
            {
                for( size_type i = 0; i < m_size; ++i )
                {
                    if( *elementAtLogicalIndexUnlocked( i ) == value )
                    {
                        eraseIndexUnlocked( i );
                        break;
                    }
                }
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        void clear()
        {
            lock();
            destroyElementsUnlocked();
            unlock();
        }

        void release()
        {
            lock();
            releaseStorageUnlocked();
            unlock();
        }

        // ------------------------------------------------------------------
        // Resize
        // ------------------------------------------------------------------

        void resize( size_type newSize )
        {
            lock();
            try
            {
                resizeUnlocked( newSize );
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        void resize( size_type newSize, const T &value )
        {
            lock();
            try
            {
                resizeUnlocked( newSize, value );
                unlock();
            }
            catch( ... )
            {
                unlock();
                throw;
            }
        }

        // ------------------------------------------------------------------
        // Access
        // ------------------------------------------------------------------

        reference at( size_type index )
        {
            lock_shared();
            if( index >= m_size )
            {
                unlock_shared();
                throw std::out_of_range( "ConcurrentDeque::at" );
            }
            reference result = *elementAtLogicalIndexUnlocked( index );
            unlock_shared();
            return result;
        }

        const_reference at( size_type index ) const
        {
            lock_shared();
            if( index >= m_size )
            {
                unlock_shared();
                throw std::out_of_range( "ConcurrentDeque::at" );
            }
            const_reference result = *elementAtLogicalIndexUnlocked( index );
            unlock_shared();
            return result;
        }

        reference operator[]( difference_type index )
        {
            lock_shared();
            assert( index >= 0 );
            assert( static_cast<size_type>( index ) < m_size );
            reference result = *elementAtLogicalIndexUnlocked( static_cast<size_type>( index ) );
            unlock_shared();
            return result;
        }

        const_reference operator[]( difference_type index ) const
        {
            lock_shared();
            assert( index >= 0 );
            assert( static_cast<size_type>( index ) < m_size );
            const_reference result = *elementAtLogicalIndexUnlocked( static_cast<size_type>( index ) );
            unlock_shared();
            return result;
        }

        reference front()
        {
            lock_shared();
            if( m_size == 0 )
            {
                unlock_shared();
                throw std::out_of_range( "ConcurrentDeque::front on empty deque" );
            }
            reference result = *elementAtLogicalIndexUnlocked( 0 );
            unlock_shared();
            return result;
        }

        const_reference front() const
        {
            lock_shared();
            if( m_size == 0 )
            {
                unlock_shared();
                throw std::out_of_range( "ConcurrentDeque::front on empty deque" );
            }
            const_reference result = *elementAtLogicalIndexUnlocked( 0 );
            unlock_shared();
            return result;
        }

        reference back()
        {
            lock_shared();
            if( m_size == 0 )
            {
                unlock_shared();
                throw std::out_of_range( "ConcurrentDeque::back on empty deque" );
            }
            reference result = *elementAtLogicalIndexUnlocked( m_size - 1 );
            unlock_shared();
            return result;
        }

        const_reference back() const
        {
            lock_shared();
            if( m_size == 0 )
            {
                unlock_shared();
                throw std::out_of_range( "ConcurrentDeque::back on empty deque" );
            }
            const_reference result = *elementAtLogicalIndexUnlocked( m_size - 1 );
            unlock_shared();
            return result;
        }

        bool try_front( T &outValue ) const
        {
            lock_shared();
            if( m_size == 0 )
            {
                unlock_shared();
                return false;
            }
            try
            {
                outValue = *elementAtLogicalIndexUnlocked( 0 );
                unlock_shared();
                return true;
            }
            catch( ... )
            {
                unlock_shared();
                throw;
            }
        }

        bool try_back( T &outValue ) const
        {
            lock_shared();
            if( m_size == 0 )
            {
                unlock_shared();
                return false;
            }
            try
            {
                outValue = *elementAtLogicalIndexUnlocked( m_size - 1 );
                unlock_shared();
                return true;
            }
            catch( ... )
            {
                unlock_shared();
                throw;
            }
        }

        // ------------------------------------------------------------------
        // Snapshot / views / iterators
        // ------------------------------------------------------------------

        Deque<T, A> snapshot() const
        {
            lock_shared();
            try
            {
                Deque<T, A> result;
                for( size_type i = 0; i < m_size; ++i )
                {
                    result.push_back( *elementAtLogicalIndexUnlocked( i ) );
                }
                unlock_shared();
                return result;
            }
            catch( ... )
            {
                unlock_shared();
                throw;
            }
        }

        ReadLockedView readLocked() const
        {
            return ReadLockedView( *this );
        }

        WriteLockedView writeLocked()
        {
            return WriteLockedView( *this );
        }

        iterator begin()
        {
            return iterator( this, 0, ConcurrentDequeIteratorLockMode::Exclusive );
        }

        iterator end()
        {
            auto result = iterator( this, 0, ConcurrentDequeIteratorLockMode::Exclusive );
            result.m_index = m_size;
            return result;
        }

        const_iterator begin() const
        {
            return const_iterator( this, 0, ConcurrentDequeIteratorLockMode::Shared );
        }

        const_iterator end() const
        {
            auto result = const_iterator( this, 0, ConcurrentDequeIteratorLockMode::Shared );
            result.m_index = m_size;
            return result;
        }

        const_iterator cbegin() const
        {
            return begin();
        }

        const_iterator cend() const
        {
            return end();
        }

        void lock() const
        {
            m_mutex.lock();
        }

        void lock_shared() const
        {
            m_mutex.lock_shared();
        }

        bool try_lock() const
        {
            return m_mutex.try_lock();
        }

        bool try_lock_shared() const
        {
            return m_mutex.try_lock_shared();
        }

        void unlock() const
        {
            m_mutex.unlock();
        }

        void unlock_shared() const
        {
            m_mutex.unlock_shared();
        }

    private:
        friend class ConcurrentDequeIterator<T, A>;
        friend class ConcurrentDequeConstIterator<T, A>;

        ConcurrentDeque( const ConcurrentDeque &other, std::unique_lock<mutex_type> ) :
            m_allocator( allocator_traits::select_on_container_copy_construction( other.m_allocator ) )
        {
            m_growthPolicy = other.m_growthPolicy;
            m_growthSize = other.m_growthSize;
            copyStorageFromUnlocked( other );
        }

        ConcurrentDeque( ConcurrentDeque &other, std::unique_lock<mutex_type>, int ) :
            m_allocator( std::move( other.m_allocator ) )
        {
            stealStorageFromUnlocked( other );
        }

        static void validateGrowthSize( size_type growthSize )
        {
            if( growthSize == 0 )
            {
                throw std::invalid_argument( "ConcurrentDeque growth size cannot be zero." );
            }
        }

        size_type physicalIndexUnlocked( size_type logicalIndex ) const noexcept
        {
            assert( m_capacity > 0 );
            const size_type distanceToEnd = m_capacity - m_head;
            if( logicalIndex >= distanceToEnd )
            {
                return logicalIndex - distanceToEnd;
            }
            return m_head + logicalIndex;
        }

        T *elementAtLogicalIndexUnlocked( size_type logicalIndex ) noexcept
        {
            return m_data + physicalIndexUnlocked( logicalIndex );
        }

        const T *elementAtLogicalIndexUnlocked( size_type logicalIndex ) const noexcept
        {
            return m_data + physicalIndexUnlocked( logicalIndex );
        }

        template <class... Args>
        void emplaceBackUnlocked( Args &&...args )
        {
            ensureCapacityForOneUnlocked();
            const size_type index = physicalIndexUnlocked( m_size );
            allocator_traits::construct( m_allocator, m_data + index, std::forward<Args>( args )... );
            ++m_size;
        }

        template <class... Args>
        void emplaceFrontUnlocked( Args &&...args )
        {
            ensureCapacityForOneUnlocked();

            const size_type newHead = ( m_head == 0 ) ? ( m_capacity - 1 ) : ( m_head - 1 );
            allocator_traits::construct( m_allocator, m_data + newHead, std::forward<Args>( args )... );
            m_head = newHead;
            ++m_size;
        }

        void popFrontUnlocked()
        {
            if( m_size == 0 )
            {
                throw std::out_of_range( "ConcurrentDeque::pop_front on empty deque" );
            }

            allocator_traits::destroy( m_allocator, m_data + m_head );
            --m_size;
            if( m_size == 0 )
            {
                m_head = 0;
            }
            else
            {
                ++m_head;
                if( m_head == m_capacity )
                {
                    m_head = 0;
                }
            }
        }

        void popBackUnlocked()
        {
            if( m_size == 0 )
            {
                throw std::out_of_range( "ConcurrentDeque::pop_back on empty deque" );
            }

            T *last = elementAtLogicalIndexUnlocked( m_size - 1 );
            allocator_traits::destroy( m_allocator, last );
            --m_size;
            if( m_size == 0 )
            {
                m_head = 0;
            }
        }

        static void assignForPop( T &destination, T &source )
        {
            if constexpr( std::is_nothrow_move_assignable<T>::value ||
                          !std::is_copy_assignable<T>::value )
            {
                destination = std::move( source );
            }
            else
            {
                destination = source;
            }
        }

        void eraseIndexUnlocked( size_type index )
        {
            if( index >= m_size )
            {
                return;
            }

            // Shift the shorter side when possible. std::deque::erase likewise
            // requires MoveAssignable T. If assignment throws the deque remains
            // valid, but some element values may have been moved/assigned: basic
            // exception guarantee, matching normal sequence-container practice.
            if( index < m_size / 2 )
            {
                for( size_type i = index; i > 0; --i )
                {
                    *elementAtLogicalIndexUnlocked( i ) =
                        std::move( *elementAtLogicalIndexUnlocked( i - 1 ) );
                }
                popFrontUnlocked();
            }
            else
            {
                for( size_type i = index; i + 1 < m_size; ++i )
                {
                    *elementAtLogicalIndexUnlocked( i ) =
                        std::move( *elementAtLogicalIndexUnlocked( i + 1 ) );
                }
                popBackUnlocked();
            }
        }

        void resizeUnlocked( size_type newSize )
        {
            if( newSize <= m_size )
            {
                while( m_size > newSize )
                {
                    popBackUnlocked();
                }
                return;
            }

            ensureCapacityForSizeUnlocked( newSize );
            const size_type oldSize = m_size;
            size_type constructed = 0;

            try
            {
                for( ; oldSize + constructed < newSize; ++constructed )
                {
                    const size_type index = physicalIndexUnlocked( oldSize + constructed );
                    allocator_traits::construct( m_allocator, m_data + index );
                }
            }
            catch( ... )
            {
                while( constructed > 0 )
                {
                    --constructed;
                    allocator_traits::destroy( m_allocator,
                                               m_data + physicalIndexUnlocked( oldSize + constructed ) );
                }
                throw;
            }

            m_size = newSize;
        }

        void resizeUnlocked( size_type newSize, const T &value )
        {
            if( newSize <= m_size )
            {
                while( m_size > newSize )
                {
                    popBackUnlocked();
                }
                return;
            }

            ensureCapacityForSizeUnlocked( newSize );
            const size_type oldSize = m_size;
            size_type constructed = 0;

            try
            {
                for( ; oldSize + constructed < newSize; ++constructed )
                {
                    const size_type index = physicalIndexUnlocked( oldSize + constructed );
                    allocator_traits::construct( m_allocator, m_data + index, value );
                }
            }
            catch( ... )
            {
                while( constructed > 0 )
                {
                    --constructed;
                    allocator_traits::destroy( m_allocator,
                                               m_data + physicalIndexUnlocked( oldSize + constructed ) );
                }
                throw;
            }

            m_size = newSize;
        }

        void ensureCapacityForSizeUnlocked( size_type requiredSize )
        {
            if( requiredSize <= m_capacity )
            {
                return;
            }

            if( m_growthPolicy == GrowthPolicy::Fixed )
            {
                throw std::length_error( "ConcurrentDeque fixed capacity exhausted." );
            }

            while( m_capacity < requiredSize )
            {
                growOnceUnlocked();
            }
        }

        void ensureCapacityForOneUnlocked()
        {
            if( m_size < m_capacity )
            {
                return;
            }
            growOnceUnlocked();
        }

        void growOnceUnlocked()
        {
            const size_type allocatorMax = allocator_traits::max_size( m_allocator );
            size_type requestedCapacity = m_capacity;

            switch( m_growthPolicy )
            {
            case GrowthPolicy::Fixed:
                throw std::length_error( "ConcurrentDeque fixed capacity exhausted." );

            case GrowthPolicy::Grow:
                if( m_growthSize > allocatorMax - m_capacity )
                {
                    throw std::length_error( "ConcurrentDeque capacity overflow." );
                }
                requestedCapacity = m_capacity + m_growthSize;
                break;

            case GrowthPolicy::Double:
                if( m_capacity == 0 )
                {
                    requestedCapacity = m_growthSize;
                }
                else if( m_capacity > allocatorMax / 2 )
                {
                    requestedCapacity = allocatorMax;
                }
                else
                {
                    requestedCapacity = m_capacity * 2;
                }
                break;

            default:
                throw std::logic_error( "Invalid ConcurrentDeque growth policy." );
            }

            if( requestedCapacity <= m_capacity )
            {
                throw std::length_error( "ConcurrentDeque cannot grow any further." );
            }
            reserveUnlocked( requestedCapacity );
        }

        void reserveUnlocked( size_type requestedCapacity )
        {
            if( requestedCapacity <= m_capacity )
            {
                return;
            }

            const size_type allocatorMax = allocator_traits::max_size( m_allocator );
            if( requestedCapacity > allocatorMax )
            {
                throw std::length_error( "ConcurrentDeque requested capacity exceeds max_size()." );
            }

            T *newData = allocator_traits::allocate( m_allocator, requestedCapacity );
            size_type constructed = 0;

            try
            {
                for( ; constructed < m_size; ++constructed )
                {
                    T *source = elementAtLogicalIndexUnlocked( constructed );
                    allocator_traits::construct( m_allocator, newData + constructed,
                                                 std::move_if_noexcept( *source ) );
                }
            }
            catch( ... )
            {
                destroyContiguousRange( newData, constructed );
                allocator_traits::deallocate( m_allocator, newData, requestedCapacity );
                throw;
            }

            destroyElementsOnlyUnlocked();
            if( m_data )
            {
                allocator_traits::deallocate( m_allocator, m_data, m_capacity );
            }

            m_data = newData;
            m_capacity = requestedCapacity;
            m_head = 0;
        }

        void destroyElementsOnlyUnlocked() noexcept
        {
            for( size_type i = 0; i < m_size; ++i )
            {
                allocator_traits::destroy( m_allocator, elementAtLogicalIndexUnlocked( i ) );
            }
        }

        void destroyElementsUnlocked() noexcept
        {
            destroyElementsOnlyUnlocked();
            m_size = 0;
            m_head = 0;
        }

        void releaseStorageUnlocked() noexcept
        {
            destroyElementsUnlocked();
            if( m_data )
            {
                allocator_traits::deallocate( m_allocator, m_data, m_capacity );
                m_data = nullptr;
            }
            m_capacity = 0;
        }

        void destroyContiguousRange( T *data, size_type count ) noexcept
        {
            if( !data )
            {
                return;
            }
            while( count > 0 )
            {
                --count;
                allocator_traits::destroy( m_allocator, data + count );
            }
        }

        void copyStorageFromUnlocked( const ConcurrentDeque &other )
        {
            if( other.m_capacity == 0 )
            {
                return;
            }

            T *newData = allocator_traits::allocate( m_allocator, other.m_capacity );
            size_type constructed = 0;
            try
            {
                for( ; constructed < other.m_size; ++constructed )
                {
                    allocator_traits::construct( m_allocator, newData + constructed,
                                                 *other.elementAtLogicalIndexUnlocked( constructed ) );
                }
            }
            catch( ... )
            {
                destroyContiguousRange( newData, constructed );
                allocator_traits::deallocate( m_allocator, newData, other.m_capacity );
                throw;
            }

            m_data = newData;
            m_capacity = other.m_capacity;
            m_size = other.m_size;
            m_head = 0;
        }

        void stealStorageFromUnlocked( ConcurrentDeque &other ) noexcept
        {
            m_data = other.m_data;
            m_capacity = other.m_capacity;
            m_size = other.m_size;
            m_head = other.m_head;
            m_growthPolicy = other.m_growthPolicy;
            m_growthSize = other.m_growthSize;

            other.m_data = nullptr;
            other.m_capacity = 0;
            other.m_size = 0;
            other.m_head = 0;
        }

        void moveAssignUnlocked( ConcurrentDeque &other )
        {
            using propagate = typename allocator_traits::propagate_on_container_move_assignment;
            using alwaysEqual = typename allocator_traits::is_always_equal;

            if constexpr( propagate::value )
            {
                releaseStorageUnlocked();
                m_allocator = std::move( other.m_allocator );
                stealStorageFromUnlocked( other );
                return;
            }
            else if constexpr( alwaysEqual::value )
            {
                releaseStorageUnlocked();
                stealStorageFromUnlocked( other );
                return;
            }
            else if( m_allocator == other.m_allocator )
            {
                releaseStorageUnlocked();
                stealStorageFromUnlocked( other );
                return;
            }

            T *newData = nullptr;
            size_type constructed = 0;
            try
            {
                if( other.m_capacity > 0 )
                {
                    newData = allocator_traits::allocate( m_allocator, other.m_capacity );
                    for( ; constructed < other.m_size; ++constructed )
                    {
                        allocator_traits::construct(
                            m_allocator, newData + constructed,
                            std::move( *other.elementAtLogicalIndexUnlocked( constructed ) ) );
                    }
                }
            }
            catch( ... )
            {
                destroyContiguousRange( newData, constructed );
                if( newData )
                {
                    allocator_traits::deallocate( m_allocator, newData, other.m_capacity );
                }
                throw;
            }

            releaseStorageUnlocked();
            m_data = newData;
            m_capacity = other.m_capacity;
            m_size = other.m_size;
            m_head = 0;
            m_growthPolicy = other.m_growthPolicy;
            m_growthSize = other.m_growthSize;
            other.releaseStorageUnlocked();
        }

        void lockBothForCopy( const ConcurrentDeque &other ) const
        {
            // Address order avoids deadlock without depending on std::lock support
            // in the custom RecursiveSpinMutex.
            if( std::less<const ConcurrentDeque *>()( this, &other ) )
            {
                lock();
                other.lock_shared();
            }
            else
            {
                other.lock_shared();
                lock();
            }
        }

        void unlockBothForCopy( const ConcurrentDeque &other ) const noexcept
        {
            // Unlock order is not semantically important here.
            other.unlock_shared();
            unlock();
        }

        void lockBothExclusive( ConcurrentDeque &other )
        {
            if( std::less<const ConcurrentDeque *>()( this, &other ) )
            {
                lock();
                other.lock();
            }
            else
            {
                other.lock();
                lock();
            }
        }

        void unlockBothExclusive( ConcurrentDeque &other ) noexcept
        {
            other.unlock();
            unlock();
        }

        mutable mutex_type m_mutex;
        allocator_type m_allocator;

        T *m_data = nullptr;
        size_type m_capacity = 0;
        size_type m_size = 0;
        size_type m_head = 0;

        GrowthPolicy m_growthPolicy = GrowthPolicy::Double;
        size_type m_growthSize = DefaultGrowthSize;
    };

    template <class T, class A>
    bool operator==( const ConcurrentDeque<T, A> &lhs, const ConcurrentDeque<T, A> &rhs )
    {
        if( &lhs == &rhs )
        {
            return true;
        }
        return lhs.snapshot() == rhs.snapshot();
    }

    template <class T, class A>
    bool operator!=( const ConcurrentDeque<T, A> &lhs, const ConcurrentDeque<T, A> &rhs )
    {
        return !( lhs == rhs );
    }

}  // namespace workphone

#endif  // ConcurrentDeque_h__
