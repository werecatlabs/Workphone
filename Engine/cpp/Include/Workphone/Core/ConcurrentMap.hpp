#ifndef WORKPHONE_CONCURRENT_MAP_HPP
#define WORKPHONE_CONCURRENT_MAP_HPP

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include "Map.hpp"
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <type_traits>
#include <utility>

namespace workphone
{
    /**
     * @brief Thread-safe ordered map backed by MapBase's preallocated node pool.
     *
     * ConcurrentMapBase uses RecursiveSpinMutex to permit concurrent readers and
     * exclusive writers while retaining the explicit reserve() and
     * GrowthPolicy behaviour of MapBase.
     *
     * @important Iterators and references returned by the compatibility API are
     * only protected while the member function itself executes. If another
     * thread may modify the map, use ReadLockedView or WriteLockedView to keep
     * the lock alive while an iterator/reference is being used.
     */
    template <class Key, class T, class Compare = std::less<Key>,
              class A = std::allocator<std::pair<const Key, T>>>
    class ConcurrentMapBase
    {
    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<const Key, T>;
        using compare_type = Compare;
        using allocator_type = A;

        using MapType = MapBase<Key, T, Compare, A>;
        using iterator = typename MapType::iterator;
        using const_iterator = typename MapType::const_iterator;
        using size_type = typename MapType::size_type;
        using difference_type = typename MapType::difference_type;
        using mutex_type = RecursiveSpinMutex;

        /**
         * @brief Shared/read lock that remains held for the lifetime of the view.
         *
         * Multiple ReadLockedView instances may coexist, subject to the
         * semantics of RecursiveSpinMutex. The view exposes only const access.
         */
        class ReadLockedView
        {
        public:
            explicit ReadLockedView( const ConcurrentMapBase &owner ) : m_owner( &owner )
            {
                m_owner->m_mutex.lock_shared();
            }

            ~ReadLockedView()
            {
                release();
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

            const_iterator begin() const noexcept
            {
                return m_owner->m_map.cbegin();
            }

            const_iterator end() const noexcept
            {
                return m_owner->m_map.cend();
            }

            const_iterator cbegin() const noexcept
            {
                return m_owner->m_map.cbegin();
            }

            const_iterator cend() const noexcept
            {
                return m_owner->m_map.cend();
            }

            const_iterator find( const key_type &key ) const
            {
                return m_owner->m_map.find( key );
            }

            const_iterator lower_bound( const key_type &key ) const
            {
                return m_owner->m_map.lower_bound( key );
            }

            const_iterator upper_bound( const key_type &key ) const
            {
                return m_owner->m_map.upper_bound( key );
            }

            bool contains( const key_type &key ) const
            {
                return m_owner->m_map.find( key ) != m_owner->m_map.end();
            }

            const mapped_type &at( const key_type &key ) const
            {
                return m_owner->m_map.at( key );
            }

            bool empty() const noexcept
            {
                return m_owner->m_map.empty();
            }

            size_type size() const noexcept
            {
                return m_owner->m_map.size();
            }

            size_type capacity() const noexcept
            {
                return m_owner->m_map.capacity();
            }

            size_type available() const noexcept
            {
                return m_owner->m_map.available();
            }

            size_type max_size() const noexcept
            {
                return m_owner->m_map.max_size();
            }

            GrowthPolicy getGrowthPolicy() const noexcept
            {
                return m_owner->m_map.getGrowthPolicy();
            }

            size_type getGrowthSize() const noexcept
            {
                return m_owner->m_map.getGrowthSize();
            }

            const MapType &get() const noexcept
            {
                return m_owner->m_map;
            }

        private:
            void release() noexcept
            {
                if( m_owner )
                {
                    m_owner->m_mutex.unlock_shared();
                    m_owner = nullptr;
                }
            }

            const ConcurrentMapBase *m_owner = nullptr;
        };

        /**
         * @brief Exclusive/write lock that remains held for the lifetime of the view.
         *
         * This is the safe way to keep mutable iterators/references across more
         * than one expression or to perform a compound operation atomically.
         */
        class WriteLockedView
        {
        public:
            explicit WriteLockedView( ConcurrentMapBase &owner ) : m_owner( &owner )
            {
                m_owner->m_mutex.lock();
            }

            ~WriteLockedView()
            {
                release();
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

            iterator begin() noexcept
            {
                return m_owner->m_map.begin();
            }

            iterator end() noexcept
            {
                return m_owner->m_map.end();
            }

            const_iterator begin() const noexcept
            {
                return m_owner->m_map.cbegin();
            }

            const_iterator end() const noexcept
            {
                return m_owner->m_map.cend();
            }

            iterator find( const key_type &key )
            {
                return m_owner->m_map.find( key );
            }

            const_iterator find( const key_type &key ) const
            {
                return m_owner->m_map.find( key );
            }

            iterator lower_bound( const key_type &key )
            {
                return m_owner->m_map.lower_bound( key );
            }

            iterator upper_bound( const key_type &key )
            {
                return m_owner->m_map.upper_bound( key );
            }

            mapped_type &operator[]( const key_type &key )
            {
                return m_owner->m_map[key];
            }

            mapped_type &at( const key_type &key )
            {
                return m_owner->m_map.at( key );
            }

            const mapped_type &at( const key_type &key ) const
            {
                return m_owner->m_map.at( key );
            }

            std::pair<iterator, bool> insert( const value_type &value )
            {
                return m_owner->m_map.insert( value );
            }

            std::pair<iterator, bool> insert( value_type &&value )
            {
                return m_owner->m_map.insert( std::move( value ) );
            }

            template <typename... Args>
            std::pair<iterator, bool> emplace( const key_type &key, Args &&...args )
            {
                auto existing = m_owner->m_map.find( key );

                if( existing != m_owner->m_map.end() )
                {
                    return { existing, false };
                }

                return m_owner->m_map.emplace( std::piecewise_construct, std::forward_as_tuple( key ),
                                               std::forward_as_tuple( std::forward<Args>( args )... ) );
            }

            iterator erase( const_iterator pos )
            {
                return m_owner->m_map.erase( pos );
            }

            size_type erase( const key_type &key )
            {
                return m_owner->m_map.erase( key );
            }

            void clear() noexcept
            {
                m_owner->m_map.clear();
            }

            bool empty() const noexcept
            {
                return m_owner->m_map.empty();
            }

            size_type size() const noexcept
            {
                return m_owner->m_map.size();
            }

            size_type capacity() const noexcept
            {
                return m_owner->m_map.capacity();
            }

            size_type available() const noexcept
            {
                return m_owner->m_map.available();
            }

            size_type max_size() const noexcept
            {
                return m_owner->m_map.max_size();
            }

            void reserve( size_type requestedCapacity )
            {
                m_owner->m_map.reserve( requestedCapacity );
            }

            GrowthPolicy getGrowthPolicy() const noexcept
            {
                return m_owner->m_map.getGrowthPolicy();
            }

            void setGrowthPolicy( GrowthPolicy policy ) noexcept
            {
                m_owner->m_map.setGrowthPolicy( policy );
            }

            size_type getGrowthSize() const noexcept
            {
                return m_owner->m_map.getGrowthSize();
            }

            void setGrowthSize( size_type growthSize )
            {
                m_owner->m_map.setGrowthSize( growthSize );
            }

            MapType &get() noexcept
            {
                return m_owner->m_map;
            }

            const MapType &get() const noexcept
            {
                return m_owner->m_map;
            }

        private:
            void release() noexcept
            {
                if( m_owner )
                {
                    m_owner->m_mutex.unlock();
                    m_owner = nullptr;
                }
            }

            ConcurrentMapBase *m_owner = nullptr;
        };

        ConcurrentMapBase() = default;

        /**
         * @brief Construct the map and preallocate storage for initialCapacity nodes.
         */
        explicit ConcurrentMapBase( size_type initialCapacity,
                                    GrowthPolicy growthPolicy = GrowthPolicy::Double,
                                    size_type growthSize = 64, const Compare &compare = Compare(),
                                    const allocator_type &allocator = allocator_type() ) :
            m_map( initialCapacity, growthPolicy, growthSize, compare, allocator )
        {
        }

        explicit ConcurrentMapBase( const Compare &compare,
                                    const allocator_type &allocator = allocator_type() ) :
            m_map( compare, allocator )
        {
        }

        ConcurrentMapBase( std::initializer_list<value_type> il,
                           GrowthPolicy growthPolicy = GrowthPolicy::Double, size_type growthSize = 64,
                           const Compare &compare = Compare(),
                           const allocator_type &allocator = allocator_type() ) :
            m_map( static_cast<size_type>( il.size() ), growthPolicy, growthSize, compare, allocator )
        {
            for( const auto &entry : il )
            {
                m_map.insert( entry );
            }
        }

        ConcurrentMapBase( const ConcurrentMapBase &other )
        {
            std::shared_lock<mutex_type> lock( other.m_mutex );
            m_map = other.m_map;
        }

        ConcurrentMapBase( ConcurrentMapBase &&other ) noexcept
        {
            std::unique_lock<mutex_type> lock( other.m_mutex );
            m_map = std::move( other.m_map );
        }

        ~ConcurrentMapBase() = default;

        ConcurrentMapBase &operator=( const ConcurrentMapBase &other )
        {
            if( this == &other )
            {
                return *this;
            }

            std::unique_lock<mutex_type> lhs( m_mutex, std::defer_lock );
            std::shared_lock<mutex_type> rhs( other.m_mutex, std::defer_lock );
            std::lock( lhs, rhs );

            m_map = other.m_map;
            return *this;
        }

        ConcurrentMapBase &operator=( ConcurrentMapBase &&other ) noexcept
        {
            if( this == &other )
            {
                return *this;
            }

            std::unique_lock<mutex_type> lhs( m_mutex, std::defer_lock );
            std::unique_lock<mutex_type> rhs( other.m_mutex, std::defer_lock );
            std::lock( lhs, rhs );

            m_map = std::move( other.m_map );
            return *this;
        }

        // ------------------------------------------------------------------
        // Element access
        // ------------------------------------------------------------------

        /**
         * @warning The returned reference is no longer protected once this
         * function returns. Prefer writeLocked() if concurrent erasure is possible.
         */
        mapped_type &operator[]( const key_type &key )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            return m_map[key];
        }

        /**
         * @warning The returned reference is no longer protected once this
         * function returns. Prefer writeLocked() if concurrent erasure is possible.
         */
        mapped_type &operator[]( key_type &&key )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            return m_map[std::move( key )];
        }

        /**
         * @warning The returned reference is no longer protected once this
         * function returns. Prefer readLocked()/writeLocked() for safe reference use.
         */
        mapped_type &at( const key_type &key )
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.at( key );
        }

        /**
         * @warning The returned reference is no longer protected once this
         * function returns. Prefer readLocked() for safe reference use.
         */
        const mapped_type &at( const key_type &key ) const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.at( key );
        }

        /**
         * @brief Copy the mapped value while holding a shared lock.
         */
        bool tryGet( const key_type &key, mapped_type &value ) const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            auto it = m_map.find( key );

            if( it == m_map.end() )
            {
                return false;
            }

            value = it->second;
            return true;
        }

        // ------------------------------------------------------------------
        // Modifiers
        // ------------------------------------------------------------------

        /**
         * @brief Insert key/value or assign value when key already exists.
         */
        void insert( const key_type &key, const mapped_type &value )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            auto it = m_map.find( key );

            if( it != m_map.end() )
            {
                it->second = value;
                return;
            }

            m_map.emplace( std::piecewise_construct, std::forward_as_tuple( key ),
                           std::forward_as_tuple( value ) );
        }

        void insert( const key_type &key, mapped_type &&value )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            auto it = m_map.find( key );

            if( it != m_map.end() )
            {
                it->second = std::move( value );
                return;
            }

            m_map.emplace( std::piecewise_construct, std::forward_as_tuple( key ),
                           std::forward_as_tuple( std::move( value ) ) );
        }

        std::pair<iterator, bool> insert( const value_type &value )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            return m_map.insert( value );
        }

        std::pair<iterator, bool> insert( value_type &&value )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            return m_map.insert( std::move( value ) );
        }

        /**
         * @brief Construct mapped value in-place if key does not already exist.
         *
         * Duplicate lookup is intentionally performed before MapBase::emplace().
         * This means a duplicate insertion does not require a spare pool node,
         * which is especially important for GrowthPolicy::Fixed.
         *
         * @warning The returned iterator is not protected after this function
         * returns. Prefer writeLocked() when it must remain valid against erasure.
         */
        template <typename... Args>
        std::pair<iterator, bool> emplace( const key_type &key, Args &&...args )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            auto existing = m_map.find( key );

            if( existing != m_map.end() )
            {
                return { existing, false };
            }

            return m_map.emplace( std::piecewise_construct, std::forward_as_tuple( key ),
                                  std::forward_as_tuple( std::forward<Args>( args )... ) );
        }

        bool erase( const key_type &key )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            return m_map.erase( key ) > 0;
        }

        void clear() noexcept
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            m_map.clear();
        }

        // ------------------------------------------------------------------
        // Lookup
        // ------------------------------------------------------------------

        bool contains( const key_type &key ) const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.find( key ) != m_map.end();
        }

        /**
         * @warning The returned iterator is not protected once this function
         * returns. Prefer readLocked()/writeLocked() during concurrent mutation.
         */
        iterator find( const key_type &key )
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.find( key );
        }

        const_iterator find( const key_type &key ) const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.find( key );
        }

        // ------------------------------------------------------------------
        // Capacity / pool control
        // ------------------------------------------------------------------

        bool empty() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.empty();
        }

        size_type size() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.size();
        }

        size_type capacity() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.capacity();
        }

        size_type available() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.available();
        }

        size_type max_size() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.max_size();
        }

        /**
         * @brief Explicitly preallocate enough tree nodes for requestedCapacity.
         *
         * This remains legal with GrowthPolicy::Fixed. Existing nodes and
         * their iterators/references are not moved by reserve().
         */
        void reserve( size_type requestedCapacity )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            m_map.reserve( requestedCapacity );
        }

        GrowthPolicy getGrowthPolicy() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.getGrowthPolicy();
        }

        void setGrowthPolicy( GrowthPolicy policy )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            m_map.setGrowthPolicy( policy );
        }

        size_type getGrowthSize() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.getGrowthSize();
        }

        void setGrowthSize( size_type growthSize )
        {
            std::unique_lock<mutex_type> lock( m_mutex );
            m_map.setGrowthSize( growthSize );
        }

        // ------------------------------------------------------------------
        // Compatibility iterators
        // ------------------------------------------------------------------

        /**
         * @warning Lock is released before returned iterator can be used.
         * Prefer readLocked()/writeLocked() for concurrent iteration.
         */
        iterator begin()
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.begin();
        }

        iterator end()
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.end();
        }

        const_iterator begin() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.begin();
        }

        const_iterator end() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.end();
        }

        const_iterator cbegin() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.cbegin();
        }

        const_iterator cend() const
        {
            std::shared_lock<mutex_type> lock( m_mutex );
            return m_map.cend();
        }

        // ------------------------------------------------------------------
        // Locked views
        // ------------------------------------------------------------------

        [[nodiscard]]
        ReadLockedView readLocked() const
        {
            return ReadLockedView( *this );
        }

        [[nodiscard]]
        WriteLockedView writeLocked()
        {
            return WriteLockedView( *this );
        }

        // ------------------------------------------------------------------
        // Manual locking compatibility
        // ------------------------------------------------------------------

        void lock()
        {
            m_mutex.lock();
        }

        void lock_shared()
        {
            m_mutex.lock_shared();
        }

        bool try_lock()
        {
            return m_mutex.try_lock();
        }

        bool try_lock_shared()
        {
            return m_mutex.try_lock_shared();
        }

        void unlock()
        {
            m_mutex.unlock();
        }

        void unlock_shared()
        {
            m_mutex.unlock_shared();
        }

        // Const overloads are useful with generic RAII wrappers and because the
        // mutex itself is mutable; locking does not change the logical map value.
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
        mutable mutex_type m_mutex;
        MapType m_map;
    };

    template <class Key, class T, class Compare = std::less<Key>,
              class A = std::allocator<std::pair<const Key, T>>>
    using ConcurrentMap = ConcurrentMapBase<Key, T, Compare, A>;

}  // namespace workphone

#endif  // WORKPHONE_CONCURRENT_MAP_HPP
