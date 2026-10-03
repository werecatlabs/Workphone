#ifndef ConcurrentSet_h__
#define ConcurrentSet_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/Thread/ScopedLock.hpp>

#include <Workphone/WorkphoneEnums.hpp>

#include <cstddef>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>

namespace workphone
{
    template <class T>
    struct ConcurrentSetDefaultCompare
    {
        bool operator()( const T &lhs, const T &rhs ) const
        {
            return lhs < rhs;
        }
    };

    /**
     * @brief Thread-safe sorted unique collection backed by contiguous Workphone Array storage.
     *
     * reserve() preallocates element storage. Insertions never allocate while size() < capacity().
     * GrowthPolicy::Fixed disables implicit growth while still allowing explicit reserve().
     */
    template <class T, class Cmp = ConcurrentSetDefaultCompare<T>>
    class ConcurrentSetBase
    {
    public:
        using container_type = Array<T>;
        using value_type = T;
        using size_type = typename container_type::size_type;
        using difference_type = typename container_type::difference_type;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;

    private:
        class SharedGuard
        {
        public:
            explicit SharedGuard( const ConcurrentSetBase &owner ) : m_owner( &owner )
            {
                m_owner->m_mutex.lock_shared();
            }

            ~SharedGuard()
            {
                if( m_owner )
                {
                    m_owner->m_mutex.unlock_shared();
                }
            }

            SharedGuard( const SharedGuard & ) = delete;
            SharedGuard &operator=( const SharedGuard & ) = delete;

        private:
            const ConcurrentSetBase *m_owner;
        };

        class ExclusiveGuard
        {
        public:
            explicit ExclusiveGuard( ConcurrentSetBase &owner ) : m_owner( &owner )
            {
                m_owner->m_mutex.lock();
            }

            ~ExclusiveGuard()
            {
                if( m_owner )
                {
                    m_owner->m_mutex.unlock();
                }
            }

            ExclusiveGuard( const ExclusiveGuard & ) = delete;
            ExclusiveGuard &operator=( const ExclusiveGuard & ) = delete;

        private:
            ConcurrentSetBase *m_owner;
        };

    public:
        class ReadLockedView
        {
        public:
            explicit ReadLockedView( const ConcurrentSetBase &owner ) : m_owner( &owner )
            {
                m_owner->m_mutex.lock_shared();
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
                    m_owner->m_mutex.unlock_shared();
                }
            }

            const_iterator begin() const noexcept
            {
                return m_owner->m_values.begin();
            }
            const_iterator end() const noexcept
            {
                return m_owner->m_values.end();
            }
            bool empty() const noexcept
            {
                return m_owner->m_values.empty();
            }
            size_type size() const noexcept
            {
                return m_owner->m_values.size();
            }
            size_type capacity() const noexcept
            {
                return m_owner->m_values.capacity();
            }

            bool contains( const T &value ) const
            {
                const auto index = m_owner->lowerBoundIndexUnlocked( value );
                return index < m_owner->m_values.size() &&
                       m_owner->equivalent( m_owner->m_values[index], value );
            }

        private:
            const ConcurrentSetBase *m_owner = nullptr;
        };

        class WriteLockedView
        {
        public:
            explicit WriteLockedView( ConcurrentSetBase &owner ) : m_owner( &owner )
            {
                m_owner->m_mutex.lock();
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
                    m_owner->m_mutex.unlock();
                }
            }

            iterator begin() noexcept
            {
                return m_owner->m_values.begin();
            }
            iterator end() noexcept
            {
                return m_owner->m_values.end();
            }
            bool empty() const noexcept
            {
                return m_owner->m_values.empty();
            }
            size_type size() const noexcept
            {
                return m_owner->m_values.size();
            }
            size_type capacity() const noexcept
            {
                return m_owner->m_values.capacity();
            }

            bool insert( const T &value )
            {
                return m_owner->insertUnlocked( value );
            }
            bool insert( T &&value )
            {
                return m_owner->insertUnlocked( std::move( value ) );
            }
            bool erase( const T &value )
            {
                return m_owner->eraseUnlocked( value );
            }
            void clear()
            {
                m_owner->m_values.clear();
            }
            void reserve( size_type n )
            {
                m_owner->reserveUnlocked( n );
            }

        private:
            ConcurrentSetBase *m_owner = nullptr;
        };

        ConcurrentSetBase() = default;

        explicit ConcurrentSetBase( const Cmp &compare ) : m_compare( compare )
        {
        }

        explicit ConcurrentSetBase( size_type initialCapacity,
                                    GrowthPolicy growthPolicy = GrowthPolicy::Double,
                                    size_type growthSize = 64, const Cmp &compare = Cmp() ) :
            m_compare( compare ),
            m_growthPolicy( growthPolicy ),
            m_growthSize( growthSize )
        {
            validateGrowthSize();
            if( initialCapacity > 0 )
            {
                m_values.reserve( initialCapacity );
            }
        }

        ~ConcurrentSetBase() = default;

        ConcurrentSetBase( const ConcurrentSetBase & ) = delete;
        ConcurrentSetBase &operator=( const ConcurrentSetBase & ) = delete;

        ConcurrentSetBase( ConcurrentSetBase &&other ) noexcept
        {
            ExclusiveGuard lock( other );
            moveFromUnlocked( other );
        }

        ConcurrentSetBase &operator=( ConcurrentSetBase &&other ) noexcept
        {
            if( this == &other )
            {
                return *this;
            }

            // Avoid lock-order deadlock without relying on object-address relational comparison.
            ConcurrentSetBase *first = this;
            ConcurrentSetBase *second = &other;
            if( std::less<ConcurrentSetBase *>()( second, first ) )
            {
                std::swap( first, second );
            }

            first->m_mutex.lock();
            second->m_mutex.lock();
            moveFromUnlocked( other );
            second->m_mutex.unlock();
            first->m_mutex.unlock();
            return *this;
        }

        bool empty() const
        {
            SharedGuard lock( *this );
            return m_values.empty();
        }

        size_type size() const
        {
            SharedGuard lock( *this );
            return m_values.size();
        }

        size_type capacity() const
        {
            SharedGuard lock( *this );
            return m_values.capacity();
        }

        size_type available() const
        {
            SharedGuard lock( *this );
            return m_values.capacity() - m_values.size();
        }

        void reserve( size_type requestedCapacity )
        {
            ExclusiveGuard lock( *this );
            reserveUnlocked( requestedCapacity );
        }

        bool insert( const T &value )
        {
            ExclusiveGuard lock( *this );
            return insertUnlocked( value );
        }

        bool insert( T &&value )
        {
            ExclusiveGuard lock( *this );
            return insertUnlocked( std::move( value ) );
        }

        bool try_insert( const T &value )
        {
            ExclusiveGuard lock( *this );
            const auto index = lowerBoundIndexUnlocked( value );
            if( index < m_values.size() && equivalent( m_values[index], value ) )
            {
                return false;
            }
            if( m_growthPolicy == GrowthPolicy::Fixed && m_values.size() >= m_values.capacity() )
            {
                return false;
            }
            ensureCapacityForOneUnlocked();
            m_values.insert( m_values.begin() + static_cast<difference_type>( index ), value );
            return true;
        }

        bool try_insert( T &&value )
        {
            ExclusiveGuard lock( *this );
            const auto index = lowerBoundIndexUnlocked( value );
            if( index < m_values.size() && equivalent( m_values[index], value ) )
            {
                return false;
            }
            if( m_growthPolicy == GrowthPolicy::Fixed && m_values.size() >= m_values.capacity() )
            {
                return false;
            }
            ensureCapacityForOneUnlocked();
            m_values.insert( m_values.begin() + static_cast<difference_type>( index ),
                             std::move( value ) );
            return true;
        }

        bool erase( const T &value )
        {
            ExclusiveGuard lock( *this );
            return eraseUnlocked( value );
        }

        void clear()
        {
            ExclusiveGuard lock( *this );
            m_values.clear();
        }

        void release()
        {
            ExclusiveGuard lock( *this );
            container_type emptyValues;
            m_values = std::move( emptyValues );
        }

        bool contains( const T &value ) const
        {
            SharedGuard lock( *this );
            const auto index = lowerBoundIndexUnlocked( value );
            return index < m_values.size() && equivalent( m_values[index], value );
        }

        container_type snapshot() const
        {
            SharedGuard lock( *this );
            return m_values;
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
                throw std::invalid_argument( "ConcurrentSetBase growth size cannot be zero" );
            }
            ExclusiveGuard lock( *this );
            m_growthSize = growthSize;
        }

        ReadLockedView readLocked() const
        {
            return ReadLockedView( *this );
        }
        WriteLockedView writeLocked()
        {
            return WriteLockedView( *this );
        }

        // Compatibility API. Iterators are non-locking; externally lock the set or use a locked view.
        iterator begin() noexcept
        {
            return m_values.begin();
        }
        const_iterator begin() const noexcept
        {
            return m_values.begin();
        }
        const_iterator cbegin() const noexcept
        {
            return m_values.begin();
        }
        iterator end() noexcept
        {
            return m_values.end();
        }
        const_iterator end() const noexcept
        {
            return m_values.end();
        }
        const_iterator cend() const noexcept
        {
            return m_values.end();
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
                throw std::invalid_argument( "ConcurrentSetBase growth size cannot be zero" );
            }
        }

        bool equivalent( const T &lhs, const T &rhs ) const
        {
            return !m_compare( lhs, rhs ) && !m_compare( rhs, lhs );
        }

        size_type lowerBoundIndexUnlocked( const T &value ) const
        {
            size_type first = 0;
            size_type count = m_values.size();

            while( count > 0 )
            {
                const auto step = count / 2;
                const auto middle = first + step;

                if( m_compare( m_values[middle], value ) )
                {
                    first = middle + 1;
                    count -= step + 1;
                }
                else
                {
                    count = step;
                }
            }
            return first;
        }

        void ensureCapacityForOneUnlocked()
        {
            if( m_values.size() < m_values.capacity() )
            {
                return;
            }

            const auto current = m_values.capacity();
            size_type requested = 0;

            switch( m_growthPolicy )
            {
            case GrowthPolicy::Fixed:
                throw std::length_error( "ConcurrentSetBase fixed capacity exhausted" );

            case GrowthPolicy::Grow:
                if( current > std::numeric_limits<size_type>::max() - m_growthSize )
                {
                    throw std::length_error( "ConcurrentSetBase capacity overflow" );
                }
                requested = current + m_growthSize;
                break;

            case GrowthPolicy::Double:
                if( current == 0 )
                {
                    requested = m_growthSize;
                }
                else
                {
                    if( current > std::numeric_limits<size_type>::max() / 2 )
                    {
                        throw std::length_error( "ConcurrentSetBase capacity overflow" );
                    }
                    requested = current * 2;
                }
                break;
            }

            reserveUnlocked( requested );
        }

        void reserveUnlocked( size_type requestedCapacity )
        {
            if( requestedCapacity > m_values.capacity() )
            {
                m_values.reserve( requestedCapacity );
            }
        }

        bool insertUnlocked( const T &value )
        {
            const auto index = lowerBoundIndexUnlocked( value );
            if( index < m_values.size() && equivalent( m_values[index], value ) )
            {
                return false;
            }

            ensureCapacityForOneUnlocked();
            m_values.insert( m_values.begin() + static_cast<difference_type>( index ), value );
            return true;
        }

        bool insertUnlocked( T &&value )
        {
            const auto index = lowerBoundIndexUnlocked( value );
            if( index < m_values.size() && equivalent( m_values[index], value ) )
            {
                return false;
            }

            ensureCapacityForOneUnlocked();
            m_values.insert( m_values.begin() + static_cast<difference_type>( index ),
                             std::move( value ) );
            return true;
        }

        bool eraseUnlocked( const T &value )
        {
            const auto index = lowerBoundIndexUnlocked( value );
            if( index >= m_values.size() || !equivalent( m_values[index], value ) )
            {
                return false;
            }

            m_values.erase( m_values.begin() + static_cast<difference_type>( index ) );
            return true;
        }

        void moveFromUnlocked( ConcurrentSetBase &other )
        {
            m_values = std::move( other.m_values );
            m_compare = std::move( other.m_compare );
            m_growthPolicy = other.m_growthPolicy;
            m_growthSize = other.m_growthSize;
        }

        mutable RecursiveSpinMutex m_mutex;
        container_type m_values;
        Cmp m_compare;
        GrowthPolicy m_growthPolicy = GrowthPolicy::Double;
        size_type m_growthSize = 64;
    };

    template <class T, class Cmp = ConcurrentSetDefaultCompare<T>>
    using ConcurrentSet = ConcurrentSetBase<T, Cmp>;

}  // namespace workphone

#endif  // ConcurrentSet_h__
