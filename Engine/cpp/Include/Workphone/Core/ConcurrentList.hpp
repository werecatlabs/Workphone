#ifndef ConcurrentList_h__
#define ConcurrentList_h__

#include "Workphone/Core/List.hpp"
#include <cstddef>
#include <functional>
#include <mutex>
#include <type_traits>
#include <utility>

namespace workphone
{

    template <class T>
    class ConcurrentListBase
    {
    public:
        using value_type = T;
        using size_type = std::size_t;

        using ListType = ListBase<T>;

        using iterator = typename ListType::iterator;
        using const_iterator = typename ListType::const_iterator;

        using mutex_type = std::recursive_mutex;

    public:
        /**
         * Keeps the list locked for the entire lifetime
         * of this object.
         *
         * Use this for:
         *
         *     auto view = list.lock();
         *
         *     for(auto &value : view)
         *     {
         *         ...
         *     }
         */
        class LockedView
        {
        public:
            LockedView( ConcurrentListBase &owner ) : m_owner( &owner ), m_lock( owner.m_mutex )
            {
            }

            LockedView( const LockedView & ) = delete;
            LockedView &operator=( const LockedView & ) = delete;

            LockedView( LockedView && ) noexcept = default;
            LockedView &operator=( LockedView && ) noexcept = default;

            iterator begin() noexcept
            {
                return m_owner->m_list.begin();
            }

            iterator end() noexcept
            {
                return m_owner->m_list.end();
            }

            const_iterator begin() const noexcept
            {
                return m_owner->m_list.begin();
            }

            const_iterator end() const noexcept
            {
                return m_owner->m_list.end();
            }

            [[nodiscard]]
            bool empty() const noexcept
            {
                return m_owner->m_list.empty();
            }

            [[nodiscard]]
            size_type size() const noexcept
            {
                return m_owner->m_list.size();
            }

            T &front()
            {
                return m_owner->m_list.front();
            }

            T &back()
            {
                return m_owner->m_list.back();
            }

            template <class... Args>
            T &emplace_back( Args &&...args )
            {
                return m_owner->m_list.emplace_back( std::forward<Args>( args )... );
            }

            template <class... Args>
            T &emplace_front( Args &&...args )
            {
                return m_owner->m_list.emplace_front( std::forward<Args>( args )... );
            }

            iterator erase( iterator it )
            {
                return m_owner->m_list.erase( it );
            }

            void clear()
            {
                m_owner->m_list.clear();
            }

            /**
             * Escape hatch for operations not explicitly
             * forwarded by LockedView.
             *
             * This is safe because LockedView owns the mutex.
             */
            ListType &get() noexcept
            {
                return m_owner->m_list;
            }

            const ListType &get() const noexcept
            {
                return m_owner->m_list;
            }

        private:
            ConcurrentListBase *m_owner = nullptr;

            std::unique_lock<mutex_type> m_lock;
        };

        class ConstLockedView
        {
        public:
            explicit ConstLockedView( const ConcurrentListBase &owner ) :
                m_owner( &owner ),
                m_lock( owner.m_mutex )
            {
            }

            ConstLockedView( const ConstLockedView & ) = delete;
            ConstLockedView &operator=( const ConstLockedView & ) = delete;

            ConstLockedView( ConstLockedView && ) noexcept = default;

            ConstLockedView &operator=( ConstLockedView && ) noexcept = default;

            const_iterator begin() const noexcept
            {
                return m_owner->m_list.begin();
            }

            const_iterator end() const noexcept
            {
                return m_owner->m_list.end();
            }

            [[nodiscard]]
            bool empty() const noexcept
            {
                return m_owner->m_list.empty();
            }

            [[nodiscard]]
            size_type size() const noexcept
            {
                return m_owner->m_list.size();
            }

            const T &front() const
            {
                return m_owner->m_list.front();
            }

            const T &back() const
            {
                return m_owner->m_list.back();
            }

            const ListType &get() const noexcept
            {
                return m_owner->m_list;
            }

        private:
            const ConcurrentListBase *m_owner = nullptr;

            std::unique_lock<mutex_type> m_lock;
        };

    public:
        ConcurrentListBase() = default;

        explicit ConcurrentListBase( size_type initialCapacity,
                                     GrowthPolicy growthPolicy = GrowthPolicy::Double,
                                     size_type growthSize = 64 ) :
            m_list( initialCapacity, growthPolicy, growthSize )
        {
        }

        ConcurrentListBase( const ConcurrentListBase &other )
        {
            std::lock_guard<mutex_type> lock( other.m_mutex );

            m_list = other.m_list;
        }

        ConcurrentListBase( ConcurrentListBase &&other ) noexcept
        {
            std::lock_guard<mutex_type> lock( other.m_mutex );

            m_list = std::move( other.m_list );
        }

        ConcurrentListBase &operator=( const ConcurrentListBase &other )
        {
            if( this == &other )
            {
                return *this;
            }

            /*
             * std::scoped_lock can lock multiple mutexes
             * without introducing lock-order deadlocks.
             */
            std::scoped_lock lock( m_mutex, other.m_mutex );

            m_list = other.m_list;

            return *this;
        }

        ConcurrentListBase &operator=( ConcurrentListBase &&other ) noexcept
        {
            if( this == &other )
            {
                return *this;
            }

            std::scoped_lock lock( m_mutex, other.m_mutex );

            m_list = std::move( other.m_list );

            return *this;
        }

    public:
        // ---------------------------------------------------------
        // Capacity
        // ---------------------------------------------------------

        [[nodiscard]]
        bool empty() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            return m_list.empty();
        }

        [[nodiscard]]
        size_type size() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            return m_list.size();
        }

        [[nodiscard]]
        size_type capacity() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            return m_list.capacity();
        }

        [[nodiscard]]
        size_type available() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            return m_list.available();
        }

        void reserve( size_type capacity )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.reserve( capacity );
        }

    public:
        // ---------------------------------------------------------
        // Growth policy
        // ---------------------------------------------------------

        [[nodiscard]]
        GrowthPolicy getGrowthPolicy() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            return m_list.getGrowthPolicy();
        }

        void setGrowthPolicy( GrowthPolicy policy )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.setGrowthPolicy( policy );
        }

        [[nodiscard]]
        size_type getGrowthSize() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            return m_list.getGrowthSize();
        }

        void setGrowthSize( size_type growthSize )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.setGrowthSize( growthSize );
        }

    public:
        // ---------------------------------------------------------
        // Insertion
        // ---------------------------------------------------------

        template <class... Args>
        void emplace_back( Args &&...args )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.emplace_back( std::forward<Args>( args )... );
        }

        template <class... Args>
        void emplace_front( Args &&...args )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.emplace_front( std::forward<Args>( args )... );
        }

        void push_back( const T &value )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.push_back( value );
        }

        void push_back( T &&value )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.push_back( std::move( value ) );
        }

        void push_front( const T &value )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.push_front( value );
        }

        void push_front( T &&value )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.push_front( std::move( value ) );
        }

        /**
         * Particularly useful with GrowthPolicy::Fixed.
         *
         * Returns false if the fixed pool is full.
         *
         * Constructor/allocation exceptions are not swallowed.
         */
        template <class... Args>
        bool try_emplace_back( Args &&...args )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_list.getGrowthPolicy() == GrowthPolicy::Fixed && m_list.available() == 0 )
            {
                return false;
            }

            m_list.emplace_back( std::forward<Args>( args )... );

            return true;
        }

        template <class... Args>
        bool try_emplace_front( Args &&...args )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_list.getGrowthPolicy() == GrowthPolicy::Fixed && m_list.available() == 0 )
            {
                return false;
            }

            m_list.emplace_front( std::forward<Args>( args )... );

            return true;
        }

    public:
        // ---------------------------------------------------------
        // Removal
        // ---------------------------------------------------------

        bool try_pop_front()
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_list.empty() )
            {
                return false;
            }

            m_list.pop_front();

            return true;
        }

        bool try_pop_back()
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_list.empty() )
            {
                return false;
            }

            m_list.pop_back();

            return true;
        }

        /**
         * Remove the first matching value.
         *
         * Requires T::operator==.
         */
        bool remove_first( const T &value )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            for( auto it = m_list.begin(); it != m_list.end(); ++it )
            {
                if( *it == value )
                {
                    m_list.erase( it );

                    return true;
                }
            }

            return false;
        }

        template <class Predicate>
        size_type remove_if( Predicate predicate )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            size_type removed = 0;

            auto it = m_list.begin();

            while( it != m_list.end() )
            {
                if( predicate( *it ) )
                {
                    it = m_list.erase( it );
                    ++removed;
                }
                else
                {
                    ++it;
                }
            }

            return removed;
        }

        void clear()
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            m_list.clear();
        }

    public:
        // ---------------------------------------------------------
        // Value retrieval
        // ---------------------------------------------------------

        /**
         * Copies the front element while holding the lock.
         *
         * Returning T& from an individually locked function would
         * not be safe because the mutex would be released before
         * the caller used the reference.
         */
        bool try_get_front( T &value ) const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_list.empty() )
            {
                return false;
            }

            value = m_list.front();

            return true;
        }

        bool try_get_back( T &value ) const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_list.empty() )
            {
                return false;
            }

            value = m_list.back();

            return true;
        }

    public:
        // ---------------------------------------------------------
        // Safe iteration
        // ---------------------------------------------------------

        [[nodiscard]]
        LockedView lock()
        {
            return LockedView( *this );
        }

        [[nodiscard]]
        ConstLockedView lock() const
        {
            return ConstLockedView( *this );
        }

        /**
         * Run arbitrary code while the container remains locked.
         *
         * Recursive mutex means the callback can safely call
         * other ConcurrentListBase member functions too.
         */
        template <class Function>
        decltype( auto ) withLock( Function &&function )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            return std::invoke( std::forward<Function>( function ), m_list );
        }

        template <class Function>
        decltype( auto ) withLock( Function &&function ) const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            return std::invoke( std::forward<Function>( function ), std::as_const( m_list ) );
        }

    private:
        mutable mutex_type m_mutex;

        ListType m_list;
    };

    template <class _Ty>
    using ConcurrentList = ConcurrentListBase<_Ty>;

}  // namespace workphone

#endif  // ConcurrentList_h__
