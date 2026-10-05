#ifndef ConcurrentQueue_h__
#define ConcurrentQueue_h__

#include <Workphone/Core/Allocator.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace workphone
{
    /**
     * @brief Thread-safe FIFO queue backed by a contiguous circular buffer.
     *
     * The queue preallocates raw storage and constructs T only for live elements.
     * Unlike the old linked-node implementation, push/pop do not allocate while
     * capacity is available and there is no lock-free memory-reclamation hazard.
     *
     * @tparam T     Element type.
     * @tparam A     Allocator used for T storage.
     * @tparam Mutex Mutex type. std::mutex is the default; std::recursive_mutex
     *               (or an engine mutex satisfying BasicLockable) can be supplied.
     */
    template <class T, class A = Allocator<T>, class Mutex = std::mutex>
    class ConcurrentQueueBase
    {
    public:
        using allocator_type = A;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using value_type = T;
        using reference = T &;
        using const_reference = const T &;
        using difference_type = typename allocator_traits::difference_type;
        using size_type = typename allocator_traits::size_type;
        using mutex_type = Mutex;

        static constexpr size_type DefaultGrowthSize = 64;

        ConcurrentQueueBase() = default;

        explicit ConcurrentQueueBase( size_type initialCapacity,
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

        ConcurrentQueueBase( const ConcurrentQueueBase &other ) :
            ConcurrentQueueBase( other, std::unique_lock<mutex_type>( other.m_mutex ) ) {}

        ConcurrentQueueBase( ConcurrentQueueBase &&other ) :
            ConcurrentQueueBase( other, std::unique_lock<mutex_type>( other.m_mutex ), 0 ) {}

        ~ConcurrentQueueBase()
        {
            // Concurrent use while an object is being destroyed is invalid for
            // essentially all C++ containers; no lock is required here.
            releaseStorageUnlocked();
        }

        ConcurrentQueueBase &operator=( const ConcurrentQueueBase &other )
        {
            if( this == &other )
            {
                return *this;
            }

            std::scoped_lock<mutex_type, mutex_type> lock( m_mutex, other.m_mutex );

            // Allocate and copy first so a failed copy leaves *this unchanged.
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

            return *this;
        }

        ConcurrentQueueBase &operator=( ConcurrentQueueBase &&other )
        {
            if( this == &other )
            {
                return *this;
            }

            std::scoped_lock<mutex_type, mutex_type> lock( m_mutex, other.m_mutex );

            moveAssignUnlocked( other );
            return *this;
        }

        // ------------------------------------------------------------------
        // Capacity / growth policy
        // ------------------------------------------------------------------

        [[nodiscard]] bool empty() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            return m_size == 0;
        }

        [[nodiscard]] size_type size() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            return m_size;
        }

        [[nodiscard]] size_type capacity() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            return m_capacity;
        }

        [[nodiscard]] size_type available() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            return m_capacity - m_size;
        }

        [[nodiscard]] size_type max_size() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            return allocator_traits::max_size( m_allocator );
        }

        /**
         * @brief Explicitly preallocate capacity for at least requestedCapacity items.
         *
         * Explicit reserve() is allowed even when GrowthPolicy::Fixed is used.
         * Existing queue order is preserved.
         */
        void reserve( size_type requestedCapacity )
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            reserveUnlocked( requestedCapacity );
        }

        [[nodiscard]] GrowthPolicy getGrowthPolicy() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            return m_growthPolicy;
        }

        void setGrowthPolicy( GrowthPolicy policy )
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            m_growthPolicy = policy;
        }

        [[nodiscard]] size_type getGrowthSize() const
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            return m_growthSize;
        }

        void setGrowthSize( size_type growthSize )
        {
            validateGrowthSize( growthSize );

            std::lock_guard<mutex_type> lock( m_mutex );
            m_growthSize = growthSize;
        }

        // ------------------------------------------------------------------
        // Insertion
        // ------------------------------------------------------------------

        void push( const T &value )
        {
            emplace( value );
        }

        void push( T &&value )
        {
            emplace( std::move( value ) );
        }

        template <class... Args>
        void emplace( Args &&...args )
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            emplaceUnlocked( std::forward<Args>( args )... );
        }

        /**
         * @brief Non-throwing-full variant for Fixed pools.
         *
         * Returns false only when a Fixed queue has exhausted its capacity.
         * Allocation failures and T constructor exceptions still propagate for
         * Grow/Double queues, because hiding those failures would be misleading.
         */
        bool try_push( const T &value )
        {
            return try_emplace( value );
        }

        bool try_push( T &&value )
        {
            return try_emplace( std::move( value ) );
        }

        template <class... Args>
        bool try_emplace( Args &&...args )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_size == m_capacity && m_growthPolicy == GrowthPolicy::Fixed )
            {
                return false;
            }

            emplaceUnlocked( std::forward<Args>( args )... );
            return true;
        }

        // ------------------------------------------------------------------
        // Removal / inspection
        // ------------------------------------------------------------------

        /**
         * @brief Pop the oldest item into outValue.
         *
         * The output assignment is performed before the queue state changes. If
         * copy/move assignment of T throws, the logical queue remains unchanged.
         * For a move-only type whose throwing move assignment modifies its source,
         * the front value may of course be left moved-from; that limitation comes
         * from T itself.
         */
        bool try_pop( T &outValue )
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_size == 0 )
            {
                return false;
            }

            T *front = m_data + m_head;
            assignForPop( outValue, *front );

            allocator_traits::destroy( m_allocator, front );
            advanceHeadAfterPopUnlocked();

            return true;
        }

        /**
         * @brief Copy the front item without removing it.
         */
        bool try_front( T &outValue ) const
        {
            std::lock_guard<mutex_type> lock( m_mutex );

            if( m_size == 0 )
            {
                return false;
            }

            outValue = m_data[m_head];
            return true;
        }

        /**
         * @brief Destroy all queued items while retaining preallocated capacity.
         */
        void clear()
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            destroyElementsUnlocked();
        }

        /**
         * @brief Destroy all elements and release all reserved storage.
         */
        void release()
        {
            std::lock_guard<mutex_type> lock( m_mutex );
            releaseStorageUnlocked();
        }

    private:
        static void validateGrowthSize( size_type growthSize )
        {
            if( growthSize == 0 )
            {
                throw std::invalid_argument( "ConcurrentQueue growth size cannot be zero." );
            }
        }

        size_type physicalIndexUnlocked( size_type logicalIndex ) const noexcept
        {
            // Both m_head and logicalIndex are < m_capacity. Avoid m_head +
            // logicalIndex overflowing size_type before applying modulo.
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

        static void assignForPop( T &destination, T &source )
        {
            // Prefer a noexcept move. If moving can throw and copying is available,
            // copy instead so a failed assignment does not disturb the queued value.
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

        template <class... Args>
        void emplaceUnlocked( Args &&...args )
        {
            ensureCapacityForOneUnlocked();

            const size_type tailIndex = physicalIndexUnlocked( m_size );

            // Increment size only after construction succeeds.
            allocator_traits::construct( m_allocator, m_data + tailIndex,
                                         std::forward<Args>( args )... );

            ++m_size;
        }

        void ensureCapacityForOneUnlocked()
        {
            if( m_size < m_capacity )
            {
                return;
            }

            const size_type allocatorMax = allocator_traits::max_size( m_allocator );
            size_type requestedCapacity = m_capacity;

            switch( m_growthPolicy )
            {
            case GrowthPolicy::Fixed:
                throw std::length_error( "ConcurrentQueue fixed capacity exhausted." );

            case GrowthPolicy::Grow:
                if( m_growthSize > allocatorMax - m_capacity )
                {
                    throw std::length_error( "ConcurrentQueue capacity overflow." );
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
                throw std::logic_error( "Invalid ConcurrentQueue growth policy." );
            }

            if( requestedCapacity <= m_capacity || requestedCapacity <= m_size )
            {
                throw std::length_error( "ConcurrentQueue cannot grow any further." );
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
                throw std::length_error( "ConcurrentQueue requested capacity exceeds max_size()." );
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

            // Only destroy the old ring after every relocated element exists.
            destroyElementsOnlyUnlocked();

            if( m_data )
            {
                allocator_traits::deallocate( m_allocator, m_data, m_capacity );
            }

            m_data = newData;
            m_capacity = requestedCapacity;
            m_head = 0;
            // m_size deliberately unchanged.
        }

        void advanceHeadAfterPopUnlocked() noexcept
        {
            --m_size;

            if( m_size == 0 )
            {
                m_head = 0;
                return;
            }

            ++m_head;
            if( m_head == m_capacity )
            {
                m_head = 0;
            }
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

        void copyStorageFromUnlocked( const ConcurrentQueueBase &other )
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

        // The lock parameter remains alive through allocator initialization
        // and storage transfer; neither may precede locking the source.
        ConcurrentQueueBase( const ConcurrentQueueBase &other, std::unique_lock<mutex_type> ) :
            m_allocator( allocator_traits::select_on_container_copy_construction( other.m_allocator ) )
        {
            m_growthPolicy = other.m_growthPolicy;
            m_growthSize = other.m_growthSize;
            copyStorageFromUnlocked( other );
        }

        ConcurrentQueueBase( ConcurrentQueueBase &other, std::unique_lock<mutex_type>, int ) :
            m_allocator( std::move( other.m_allocator ) )
        {
            stealStorageFromUnlocked( other );
        }

        void stealStorageFromUnlocked( ConcurrentQueueBase &other ) noexcept
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

        void moveAssignUnlocked( ConcurrentQueueBase &other )
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
            else
            {
                if( m_allocator == other.m_allocator )
                {
                    releaseStorageUnlocked();
                    stealStorageFromUnlocked( other );
                    return;
                }
            }

            // Unequal, non-propagating allocators: move elements into storage owned
            // by *this instead of stealing memory owned by other's allocator.
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

        mutable mutex_type m_mutex;

        allocator_type m_allocator;

        T *m_data = nullptr;
        size_type m_capacity = 0;
        size_type m_size = 0;
        size_type m_head = 0;

        GrowthPolicy m_growthPolicy = GrowthPolicy::Double;
        size_type m_growthSize = DefaultGrowthSize;
    };

    template <class T, class A = Allocator<T>>
    using ConcurrentQueue = ConcurrentQueueBase<T, A>;

}  // namespace workphone

#endif  // ConcurrentQueue_h__
