#ifndef ConcurrentHashMap_h__
#define ConcurrentHashMap_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Allocator.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/Thread/ScopedLock.hpp>

#include <Workphone/WorkphoneEnums.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace workphone
{
    template <class Key, class T, class Hash, class KeyEqual, class A>
    class ConcurrentHashmapBase;

    /**
     * @brief Non-owning iterator over ConcurrentHashmapBase's physical hash-table order.
     *
     * The iterator itself does not lock the container. Use readLocked()/writeLocked()
     * when an iterator must remain valid while other threads may modify the map.
     */
    template <typename Key, typename T, bool IsConst = false, class Hash = std::hash<Key>,
              class KeyEqual = std::equal_to<Key>, class A = Allocator<std::pair<const Key, T>>>
    class ConcurrentHashmapIterator
    {
    private:
        using owner_type = ConcurrentHashmapBase<Key, T, Hash, KeyEqual, A>;
        using owner_pointer = typename std::conditional<IsConst, const owner_type *, owner_type *>::type;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const Key, T>;
        using difference_type = std::ptrdiff_t;
        using pointer = typename std::conditional<IsConst, const value_type *, value_type *>::type;
        using reference = typename std::conditional<IsConst, const value_type &, value_type &>::type;

        ConcurrentHashmapIterator() noexcept = default;

        template <bool B = IsConst, typename std::enable_if<B, int>::type = 0>
        ConcurrentHashmapIterator(
            const ConcurrentHashmapIterator<Key, T, false, Hash, KeyEqual, A> &other ) noexcept :
            m_owner( other.m_owner ),
            m_index( other.m_index )
        {
        }

        reference operator*() const
        {
            assert( m_owner );
            return m_owner->valueAtIndexUnlocked( m_index );
        }

        pointer operator->() const
        {
            return std::addressof( operator*() );
        }

        ConcurrentHashmapIterator &operator++()
        {
            assert( m_owner );
            m_index = m_owner->nextOccupiedIndexUnlocked( m_index + 1 );
            return *this;
        }

        ConcurrentHashmapIterator operator++( int )
        {
            ConcurrentHashmapIterator tmp( *this );
            ++( *this );
            return tmp;
        }

        template <bool OtherIsConst>
        bool operator==( const ConcurrentHashmapIterator<Key, T, OtherIsConst, Hash, KeyEqual, A>
                             &other ) const noexcept
        {
            return m_owner == other.m_owner && m_index == other.m_index;
        }

        template <bool OtherIsConst>
        bool operator!=( const ConcurrentHashmapIterator<Key, T, OtherIsConst, Hash, KeyEqual, A>
                             &other ) const noexcept
        {
            return !( *this == other );
        }

    private:
        ConcurrentHashmapIterator( owner_pointer owner, std::size_t index ) noexcept :
            m_owner( owner ),
            m_index( index )
        {
        }

        owner_pointer m_owner = nullptr;
        std::size_t m_index = 0;

        template <class, class, class, class, class>
        friend class ConcurrentHashmapBase;

        template <typename, typename, bool, class, class, class>
        friend class ConcurrentHashmapIterator;
    };

    /**
     * @brief Thread-safe, cache-friendly open-addressed hash map with explicit capacity control.
     *
     * reserve(n) preallocates the slot array required to hold at least n elements. Unlike
     * std::unordered_map, insertion does not allocate one node per element while spare
     * capacity exists.
     *
     * Growth invalidates iterators/references because elements live directly in the slot
     * array. GrowthPolicy::Fixed is therefore particularly useful when stable storage
     * during a real-time phase is desired.
     */
    template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
              class A = Allocator<std::pair<const Key, T>>>
    class ConcurrentHashmapBase
    {
    private:
        enum class SlotState : std::uint8_t
        {
            Empty,
            Occupied,
            Deleted
        };

    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<const Key, T>;
        using hasher = Hash;
        using key_equal = KeyEqual;
        using allocator_type = A;
        using allocator_traits = std::allocator_traits<A>;
        using size_type = typename allocator_traits::size_type;
        using difference_type = typename allocator_traits::difference_type;
        using iterator = ConcurrentHashmapIterator<Key, T, false, Hash, KeyEqual, A>;
        using const_iterator = ConcurrentHashmapIterator<Key, T, true, Hash, KeyEqual, A>;

    private:
        struct Slot
        {
            SlotState state = SlotState::Empty;
            typename std::aligned_storage<sizeof( value_type ), alignof( value_type )>::type storage;

            value_type *value() noexcept
            {
                return std::launder( reinterpret_cast<value_type *>( &storage ) );
            }

            const value_type *value() const noexcept
            {
                return std::launder( reinterpret_cast<const value_type *>( &storage ) );
            }
        };

        using SlotAllocator = typename allocator_traits::template rebind_alloc<Slot>;
        using SlotAllocTraits = std::allocator_traits<SlotAllocator>;

        static constexpr size_type npos = std::numeric_limits<size_type>::max();

        struct ProbeResult
        {
            size_type index = npos;
            bool found = false;
        };

        class SharedGuard
        {
        public:
            explicit SharedGuard( const ConcurrentHashmapBase &owner ) : m_owner( &owner )
            {
                m_owner->lock_shared();
            }

            ~SharedGuard()
            {
                if( m_owner )
                {
                    m_owner->unlock_shared();
                }
            }

            SharedGuard( const SharedGuard & ) = delete;
            SharedGuard &operator=( const SharedGuard & ) = delete;

        private:
            const ConcurrentHashmapBase *m_owner;
        };

        class ExclusiveGuard
        {
        public:
            explicit ExclusiveGuard( ConcurrentHashmapBase &owner ) : m_owner( &owner )
            {
                m_owner->lock();
            }

            ~ExclusiveGuard()
            {
                if( m_owner )
                {
                    m_owner->unlock();
                }
            }

            ExclusiveGuard( const ExclusiveGuard & ) = delete;
            ExclusiveGuard &operator=( const ExclusiveGuard & ) = delete;

        private:
            ConcurrentHashmapBase *m_owner;
        };

    public:
        class ReadLockedView
        {
        public:
            explicit ReadLockedView( const ConcurrentHashmapBase &owner ) : m_owner( &owner )
            {
                m_owner->lock_shared();
            }

            ReadLockedView( const ReadLockedView & ) = delete;
            ReadLockedView &operator=( const ReadLockedView & ) = delete;

            ReadLockedView( ReadLockedView &&other ) noexcept : m_owner( other.m_owner )
            {
                other.m_owner = nullptr;
            }

            ~ReadLockedView()
            {
                if( m_owner )
                {
                    m_owner->unlock_shared();
                }
            }

            const_iterator begin() const noexcept
            {
                return m_owner->cbeginUnlocked();
            }
            const_iterator end() const noexcept
            {
                return m_owner->cendUnlocked();
            }
            const_iterator find( const Key &key ) const
            {
                return m_owner->findUnlocked( key );
            }
            bool contains( const Key &key ) const
            {
                return m_owner->findIndexUnlocked( key ) != npos;
            }
            bool empty() const noexcept
            {
                return m_owner->m_size == 0;
            }
            size_type size() const noexcept
            {
                return m_owner->m_size;
            }
            size_type capacity() const noexcept
            {
                return m_owner->m_capacity;
            }

            const T &at( const Key &key ) const
            {
                const auto index = m_owner->findIndexUnlocked( key );
                if( index == npos )
                {
                    throw std::out_of_range(
                        "ConcurrentHashmapBase::ReadLockedView::at: key not found" );
                }
                return m_owner->m_slots[index].value()->second;
            }

        private:
            const ConcurrentHashmapBase *m_owner = nullptr;
        };

        class WriteLockedView
        {
        public:
            explicit WriteLockedView( ConcurrentHashmapBase &owner ) : m_owner( &owner )
            {
                m_owner->lock();
            }

            WriteLockedView( const WriteLockedView & ) = delete;
            WriteLockedView &operator=( const WriteLockedView & ) = delete;

            WriteLockedView( WriteLockedView &&other ) noexcept : m_owner( other.m_owner )
            {
                other.m_owner = nullptr;
            }

            ~WriteLockedView()
            {
                if( m_owner )
                {
                    m_owner->unlock();
                }
            }

            iterator begin() noexcept
            {
                return m_owner->beginUnlocked();
            }
            iterator end() noexcept
            {
                return m_owner->endUnlocked();
            }
            iterator find( const Key &key )
            {
                return m_owner->findUnlocked( key );
            }
            bool contains( const Key &key ) const
            {
                return m_owner->findIndexUnlocked( key ) != npos;
            }
            bool empty() const noexcept
            {
                return m_owner->m_size == 0;
            }
            size_type size() const noexcept
            {
                return m_owner->m_size;
            }
            size_type capacity() const noexcept
            {
                return m_owner->m_capacity;
            }

            template <typename... Args>
            std::pair<iterator, bool> emplace( const Key &key, Args &&...args )
            {
                return m_owner->emplaceUnlocked( key, std::forward<Args>( args )... );
            }

            bool erase( const Key &key )
            {
                return m_owner->eraseUnlocked( key );
            }
            iterator erase( const_iterator pos )
            {
                return m_owner->eraseUnlocked( pos );
            }
            void clear() noexcept
            {
                m_owner->clearUnlocked();
            }
            void reserve( size_type n )
            {
                m_owner->reserveUnlocked( n );
            }

            T &at( const Key &key )
            {
                const auto index = m_owner->findIndexUnlocked( key );
                if( index == npos )
                {
                    throw std::out_of_range(
                        "ConcurrentHashmapBase::WriteLockedView::at: key not found" );
                }
                return m_owner->m_slots[index].value()->second;
            }

        private:
            ConcurrentHashmapBase *m_owner = nullptr;
        };

        ConcurrentHashmapBase() = default;

        explicit ConcurrentHashmapBase( size_type initialCapacity,
                                        GrowthPolicy growthPolicy = GrowthPolicy::Double,
                                        size_type growthSize = 64, const Hash &hash = Hash(),
                                        const KeyEqual &equal = KeyEqual(), const A &allocator = A() ) :
            m_slotAllocator( allocator ),
            m_hash( hash ),
            m_equal( equal ),
            m_growthPolicy( growthPolicy ),
            m_growthSize( growthSize )
        {
            validateGrowthSize();
            if( initialCapacity > 0 )
            {
                reserveUnlocked( initialCapacity );
            }
        }

        ~ConcurrentHashmapBase()
        {
            releaseUnlocked();
        }

        ConcurrentHashmapBase( const ConcurrentHashmapBase & ) = delete;
        ConcurrentHashmapBase &operator=( const ConcurrentHashmapBase & ) = delete;
        ConcurrentHashmapBase( ConcurrentHashmapBase && ) = delete;
        ConcurrentHashmapBase &operator=( ConcurrentHashmapBase && ) = delete;

        T &operator[]( const Key &key )
        {
            ExclusiveGuard lock( *this );
            const auto existing = findIndexUnlocked( key );
            if( existing != npos )
            {
                return m_slots[existing].value()->second;
            }

            ensureCapacityForOneUnlocked();
            const auto probe = probeForInsertUnlocked( key );
            if( probe.index == npos )
            {
                throw std::length_error( "ConcurrentHashmapBase: no insertion slot available" );
            }

            constructAtUnlocked( probe.index, key );
            return m_slots[probe.index].value()->second;
        }

        T &at( const Key &key )
        {
            SharedGuard lock( *this );
            const auto index = findIndexUnlocked( key );
            if( index == npos )
            {
                throw std::out_of_range( "ConcurrentHashmapBase::at: key not found" );
            }
            return m_slots[index].value()->second;
        }

        const T &at( const Key &key ) const
        {
            SharedGuard lock( *this );
            const auto index = findIndexUnlocked( key );
            if( index == npos )
            {
                throw std::out_of_range( "ConcurrentHashmapBase::at: key not found" );
            }
            return m_slots[index].value()->second;
        }

        void insert( const Key &key, const T &value )
        {
            ExclusiveGuard lock( *this );
            const auto existing = findIndexUnlocked( key );
            if( existing != npos )
            {
                m_slots[existing].value()->second = value;
                return;
            }

            ensureCapacityForOneUnlocked();
            const auto probe = probeForInsertUnlocked( key );
            constructAtUnlocked( probe.index, key, value );
        }

        void insert( const Key &key, T &&value )
        {
            ExclusiveGuard lock( *this );
            const auto existing = findIndexUnlocked( key );
            if( existing != npos )
            {
                m_slots[existing].value()->second = std::move( value );
                return;
            }

            ensureCapacityForOneUnlocked();
            const auto probe = probeForInsertUnlocked( key );
            constructAtUnlocked( probe.index, key, std::move( value ) );
        }

        template <typename... Args>
        std::pair<iterator, bool> emplace( const Key &key, Args &&...args )
        {
            ExclusiveGuard lock( *this );
            return emplaceUnlocked( key, std::forward<Args>( args )... );
        }

        template <typename... Args>
        std::pair<iterator, bool> try_emplace( const Key &key, Args &&...args )
        {
            ExclusiveGuard lock( *this );

            const auto existing = findIndexUnlocked( key );
            if( existing != npos )
            {
                return { iterator( this, existing ), false };
            }

            if( m_growthPolicy == GrowthPolicy::Fixed && m_size >= m_capacity )
            {
                return { endUnlocked(), false };
            }

            return emplaceUnlocked( key, std::forward<Args>( args )... );
        }

        iterator erase( const_iterator pos )
        {
            ExclusiveGuard lock( *this );
            return eraseUnlocked( pos );
        }

        iterator erase( const_iterator first, const_iterator last )
        {
            ExclusiveGuard lock( *this );
            while( first != last )
            {
                first = const_iterator( eraseUnlocked( first ) );
            }
            return iterator( this, first.m_index );
        }

        bool erase( const Key &key )
        {
            ExclusiveGuard lock( *this );
            return eraseUnlocked( key );
        }

        void clear()
        {
            ExclusiveGuard lock( *this );
            clearUnlocked();
        }

        void release()
        {
            ExclusiveGuard lock( *this );
            releaseUnlocked();
        }

        bool contains( const Key &key ) const
        {
            SharedGuard lock( *this );
            return findIndexUnlocked( key ) != npos;
        }

        bool tryGet( const Key &key, T &out ) const
        {
            SharedGuard lock( *this );
            const auto index = findIndexUnlocked( key );
            if( index == npos )
            {
                return false;
            }
            out = m_slots[index].value()->second;
            return true;
        }

        iterator find( const Key &key )
        {
            SharedGuard lock( *this );
            return findUnlocked( key );
        }

        const_iterator find( const Key &key ) const
        {
            SharedGuard lock( *this );
            return findUnlocked( key );
        }

        bool empty() const
        {
            SharedGuard lock( *this );
            return m_size == 0;
        }

        size_type size() const
        {
            SharedGuard lock( *this );
            return m_size;
        }

        size_type capacity() const
        {
            SharedGuard lock( *this );
            return m_capacity;
        }

        size_type available() const
        {
            SharedGuard lock( *this );
            return m_capacity - m_size;
        }

        size_type bucket_count() const
        {
            SharedGuard lock( *this );
            return m_bucketCount;
        }

        float load_factor() const
        {
            SharedGuard lock( *this );
            return m_bucketCount == 0
                       ? 0.0f
                       : static_cast<float>( m_size ) / static_cast<float>( m_bucketCount );
        }

        size_type max_size() const noexcept
        {
            return static_cast<size_type>( std::numeric_limits<size_type>::max() / 2 );
        }

        void reserve( size_type requestedCapacity )
        {
            ExclusiveGuard lock( *this );
            reserveUnlocked( requestedCapacity );
        }

        GrowthPolicy getGrowthPolicy() const
        {
            SharedGuard lock( *this );
            return m_growthPolicy;
        }

        void setGrowthPolicy( GrowthPolicy policy )
        {
            ExclusiveGuard lock( *this );
            m_growthPolicy = policy;
        }

        size_type getGrowthSize() const
        {
            SharedGuard lock( *this );
            return m_growthSize;
        }

        void setGrowthSize( size_type growthSize )
        {
            if( growthSize == 0 )
            {
                throw std::invalid_argument( "ConcurrentHashmapBase growth size cannot be zero" );
            }
            ExclusiveGuard lock( *this );
            m_growthSize = growthSize;
        }

        iterator begin() noexcept
        {
            return beginUnlocked();
        }
        iterator end() noexcept
        {
            return endUnlocked();
        }
        const_iterator begin() const noexcept
        {
            return cbeginUnlocked();
        }
        const_iterator end() const noexcept
        {
            return cendUnlocked();
        }
        const_iterator cbegin() const noexcept
        {
            return cbeginUnlocked();
        }
        const_iterator cend() const noexcept
        {
            return cendUnlocked();
        }

        ReadLockedView readLocked() const
        {
            return ReadLockedView( *this );
        }
        WriteLockedView writeLocked()
        {
            return WriteLockedView( *this );
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
        void validateGrowthSize() const
        {
            if( m_growthSize == 0 )
            {
                throw std::invalid_argument( "ConcurrentHashmapBase growth size cannot be zero" );
            }
        }

        static size_type nextPowerOfTwo( size_type value )
        {
            if( value <= 1 )
            {
                return 1;
            }

            const auto max = std::numeric_limits<size_type>::max();
            --value;
            for( size_type shift = 1; shift < sizeof( size_type ) * 8; shift <<= 1 )
            {
                value |= value >> shift;
            }
            if( value == max )
            {
                throw std::length_error( "ConcurrentHashmapBase capacity overflow" );
            }
            return value + 1;
        }

        static size_type bucketCountForCapacity( size_type capacity )
        {
            if( capacity == 0 )
            {
                return 0;
            }

            // Keep the table at or below 70% load. Compute ceil(capacity * 10 / 7)
            // without overflowing capacity * 10.
            const auto max = std::numeric_limits<size_type>::max();
            const size_type quotient = capacity / 7;
            const size_type remainder = capacity % 7;
            if( quotient > max / 10 )
            {
                throw std::length_error( "ConcurrentHashmapBase capacity too large" );
            }
            const size_type base = quotient * 10;
            const size_type extra = ( remainder * 10 + 6 ) / 7;
            if( base > max - extra )
            {
                throw std::length_error( "ConcurrentHashmapBase capacity too large" );
            }
            const size_type needed = base + extra;
            const size_type buckets = nextPowerOfTwo( needed < 8 ? 8 : needed );
            return buckets;
        }

        size_type initialBucket( const Key &key, size_type bucketCount ) const
        {
            return static_cast<size_type>( m_hash( key ) ) & ( bucketCount - 1 );
        }

        size_type findIndexUnlocked( const Key &key ) const
        {
            if( m_bucketCount == 0 )
            {
                return npos;
            }

            const size_type start = initialBucket( key, m_bucketCount );
            const size_type mask = m_bucketCount - 1;

            for( size_type probe = 0; probe < m_bucketCount; ++probe )
            {
                const size_type index = ( start + probe ) & mask;
                const Slot &slot = m_slots[index];

                if( slot.state == SlotState::Empty )
                {
                    return npos;
                }

                if( slot.state == SlotState::Occupied && m_equal( slot.value()->first, key ) )
                {
                    return index;
                }
            }

            return npos;
        }

        ProbeResult probeForInsertUnlocked( const Key &key ) const
        {
            if( m_bucketCount == 0 )
            {
                return {};
            }

            const size_type start = initialBucket( key, m_bucketCount );
            const size_type mask = m_bucketCount - 1;
            size_type firstDeleted = npos;

            for( size_type probe = 0; probe < m_bucketCount; ++probe )
            {
                const size_type index = ( start + probe ) & mask;
                const Slot &slot = m_slots[index];

                if( slot.state == SlotState::Occupied )
                {
                    if( m_equal( slot.value()->first, key ) )
                    {
                        return { index, true };
                    }
                }
                else if( slot.state == SlotState::Deleted )
                {
                    if( firstDeleted == npos )
                    {
                        firstDeleted = index;
                    }
                }
                else
                {
                    return { firstDeleted != npos ? firstDeleted : index, false };
                }
            }

            return { firstDeleted, false };
        }

        template <typename... Args>
        void constructAtUnlocked( size_type index, const Key &key, Args &&...args )
        {
            if( index == npos )
            {
                throw std::length_error( "ConcurrentHashmapBase: no insertion slot available" );
            }

            Slot &slot = m_slots[index];
            const bool reusedDeleted = slot.state == SlotState::Deleted;

            ::new( static_cast<void *>( &slot.storage ) )
                value_type( std::piecewise_construct, std::forward_as_tuple( key ),
                            std::forward_as_tuple( std::forward<Args>( args )... ) );

            slot.state = SlotState::Occupied;
            ++m_size;
            if( reusedDeleted )
            {
                --m_deletedCount;
            }
        }

        template <typename... Args>
        std::pair<iterator, bool> emplaceUnlocked( const Key &key, Args &&...args )
        {
            const auto existing = findIndexUnlocked( key );
            if( existing != npos )
            {
                return { iterator( this, existing ), false };
            }

            ensureCapacityForOneUnlocked();
            const auto probe = probeForInsertUnlocked( key );
            constructAtUnlocked( probe.index, key, std::forward<Args>( args )... );
            return { iterator( this, probe.index ), true };
        }

        bool eraseUnlocked( const Key &key )
        {
            const auto index = findIndexUnlocked( key );
            if( index == npos )
            {
                return false;
            }
            eraseIndexUnlocked( index );
            return true;
        }

        iterator eraseUnlocked( const_iterator pos )
        {
            if( pos.m_owner != this || pos.m_index >= m_bucketCount ||
                m_slots[pos.m_index].state != SlotState::Occupied )
            {
                throw std::out_of_range( "ConcurrentHashmapBase::erase: invalid iterator" );
            }

            const auto next = nextOccupiedIndexUnlocked( pos.m_index + 1 );
            eraseIndexUnlocked( pos.m_index );
            return iterator( this, next );
        }

        void eraseIndexUnlocked( size_type index ) noexcept
        {
            Slot &slot = m_slots[index];
            slot.value()->~value_type();
            slot.state = SlotState::Deleted;
            --m_size;
            ++m_deletedCount;
        }

        void clearUnlocked() noexcept
        {
            for( size_type i = 0; i < m_bucketCount; ++i )
            {
                Slot &slot = m_slots[i];
                if( slot.state == SlotState::Occupied )
                {
                    slot.value()->~value_type();
                }
                slot.state = SlotState::Empty;
            }
            m_size = 0;
            m_deletedCount = 0;
        }

        void releaseUnlocked() noexcept
        {
            if( !m_slots )
            {
                m_size = 0;
                m_capacity = 0;
                m_bucketCount = 0;
                m_deletedCount = 0;
                return;
            }

            clearUnlocked();
            for( size_type i = 0; i < m_bucketCount; ++i )
            {
                SlotAllocTraits::destroy( m_slotAllocator, m_slots + i );
            }
            SlotAllocTraits::deallocate( m_slotAllocator, m_slots, m_bucketCount );
            m_slots = nullptr;
            m_capacity = 0;
            m_bucketCount = 0;
        }

        void ensureCapacityForOneUnlocked()
        {
            if( m_size < m_capacity )
            {
                return;
            }

            size_type newCapacity = 0;
            switch( m_growthPolicy )
            {
            case GrowthPolicy::Fixed:
                throw std::length_error( "ConcurrentHashmapBase fixed capacity exhausted" );

            case GrowthPolicy::Grow:
                if( m_capacity > max_size() - m_growthSize )
                {
                    throw std::length_error( "ConcurrentHashmapBase capacity overflow" );
                }
                newCapacity = m_capacity + m_growthSize;
                break;

            case GrowthPolicy::Double:
                if( m_capacity == 0 )
                {
                    newCapacity = m_growthSize;
                }
                else
                {
                    if( m_capacity > max_size() / 2 )
                    {
                        throw std::length_error( "ConcurrentHashmapBase capacity overflow" );
                    }
                    newCapacity = m_capacity * 2;
                }
                break;
            }

            reserveUnlocked( newCapacity );
        }

        void reserveUnlocked( size_type requestedCapacity )
        {
            if( requestedCapacity <= m_capacity )
            {
                return;
            }
            rehashToCapacityUnlocked( requestedCapacity );
        }

        static void destroySlotArray( SlotAllocator &allocator, Slot *slots, size_type count ) noexcept
        {
            if( !slots )
            {
                return;
            }
            for( size_type i = 0; i < count; ++i )
            {
                if( slots[i].state == SlotState::Occupied )
                {
                    slots[i].value()->~value_type();
                }
                SlotAllocTraits::destroy( allocator, slots + i );
            }
            SlotAllocTraits::deallocate( allocator, slots, count );
        }

        size_type findInsertionIndexInArray( Slot *slots, size_type bucketCount, const Key &key ) const
        {
            const size_type start = initialBucket( key, bucketCount );
            const size_type mask = bucketCount - 1;
            for( size_type probe = 0; probe < bucketCount; ++probe )
            {
                const size_type index = ( start + probe ) & mask;
                if( slots[index].state != SlotState::Occupied )
                {
                    return index;
                }
            }
            return npos;
        }

        void rehashToCapacityUnlocked( size_type requestedCapacity )
        {
            if( requestedCapacity < m_size )
            {
                requestedCapacity = m_size;
            }

            const size_type newBucketCount = bucketCountForCapacity( requestedCapacity );
            Slot *newSlots = SlotAllocTraits::allocate( m_slotAllocator, newBucketCount );
            size_type slotsConstructed = 0;

            try
            {
                for( ; slotsConstructed < newBucketCount; ++slotsConstructed )
                {
                    SlotAllocTraits::construct( m_slotAllocator, newSlots + slotsConstructed );
                }

                for( size_type i = 0; i < m_bucketCount; ++i )
                {
                    Slot &oldSlot = m_slots[i];
                    if( oldSlot.state != SlotState::Occupied )
                    {
                        continue;
                    }

                    const size_type destination =
                        findInsertionIndexInArray( newSlots, newBucketCount, oldSlot.value()->first );
                    if( destination == npos )
                    {
                        throw std::length_error( "ConcurrentHashmapBase rehash failed: table full" );
                    }

                    ::new( static_cast<void *>( &newSlots[destination].storage ) )
                        value_type( std::move_if_noexcept( *oldSlot.value() ) );
                    newSlots[destination].state = SlotState::Occupied;
                }
            }
            catch( ... )
            {
                for( size_type i = 0; i < slotsConstructed; ++i )
                {
                    if( newSlots[i].state == SlotState::Occupied )
                    {
                        newSlots[i].value()->~value_type();
                    }
                    SlotAllocTraits::destroy( m_slotAllocator, newSlots + i );
                }
                SlotAllocTraits::deallocate( m_slotAllocator, newSlots, newBucketCount );
                throw;
            }

            if( m_slots )
            {
                for( size_type i = 0; i < m_bucketCount; ++i )
                {
                    Slot &oldSlot = m_slots[i];
                    if( oldSlot.state == SlotState::Occupied )
                    {
                        oldSlot.value()->~value_type();
                    }
                    SlotAllocTraits::destroy( m_slotAllocator, m_slots + i );
                }
                SlotAllocTraits::deallocate( m_slotAllocator, m_slots, m_bucketCount );
            }

            m_slots = newSlots;
            m_bucketCount = newBucketCount;
            m_capacity = requestedCapacity;
            m_deletedCount = 0;
        }

        size_type nextOccupiedIndexUnlocked( size_type start ) const noexcept
        {
            for( size_type i = start; i < m_bucketCount; ++i )
            {
                if( m_slots[i].state == SlotState::Occupied )
                {
                    return i;
                }
            }
            return m_bucketCount;
        }

        value_type &valueAtIndexUnlocked( size_type index )
        {
            assert( index < m_bucketCount && m_slots[index].state == SlotState::Occupied );
            return *m_slots[index].value();
        }

        const value_type &valueAtIndexUnlocked( size_type index ) const
        {
            assert( index < m_bucketCount && m_slots[index].state == SlotState::Occupied );
            return *m_slots[index].value();
        }

        iterator beginUnlocked() noexcept
        {
            return iterator( this, nextOccupiedIndexUnlocked( 0 ) );
        }
        iterator endUnlocked() noexcept
        {
            return iterator( this, m_bucketCount );
        }
        const_iterator cbeginUnlocked() const noexcept
        {
            return const_iterator( this, nextOccupiedIndexUnlocked( 0 ) );
        }
        const_iterator cendUnlocked() const noexcept
        {
            return const_iterator( this, m_bucketCount );
        }

        iterator findUnlocked( const Key &key )
        {
            const auto index = findIndexUnlocked( key );
            return iterator( this, index == npos ? m_bucketCount : index );
        }

        const_iterator findUnlocked( const Key &key ) const
        {
            const auto index = findIndexUnlocked( key );
            return const_iterator( this, index == npos ? m_bucketCount : index );
        }

        mutable RecursiveSpinMutex m_mutex;
        SlotAllocator m_slotAllocator;
        Slot *m_slots = nullptr;
        size_type m_size = 0;
        size_type m_capacity = 0;
        size_type m_bucketCount = 0;
        size_type m_deletedCount = 0;
        Hash m_hash{};
        KeyEqual m_equal{};
        GrowthPolicy m_growthPolicy = GrowthPolicy::Double;
        size_type m_growthSize = 64;

        template <typename, typename, bool, class, class, class>
        friend class ConcurrentHashmapIterator;
    };

    template <typename Key, typename T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
              class A = Allocator<std::pair<const Key, T>>>
    using ConcurrentHashMap = ConcurrentHashmapBase<Key, T, Hash, KeyEqual, A>;

}  // namespace workphone

#endif  // ConcurrentHashMap_h__
