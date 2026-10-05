#ifndef NOMINMAX
#    define NOMINMAX
#endif
#ifdef WP_THREADING_STANDALONE_TESTS
#    define BOOST_TEST_MODULE ThreadingRegressionTests
#    include <boost/test/included/unit_test.hpp>
#else
#    include <boost/test/unit_test.hpp>
#endif

#include <Workphone/Core/ConcurrentHashMap.hpp>
#include <Workphone/Core/ConcurrentSet.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/ConcurrentDeque.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <Workphone/Thread/RecursiveSpinRWMutex.hpp>
#include <Workphone/Thread/SharedMutex.hpp>
#include <Workphone/Thread/SpinMutex.hpp>
#include <atomic>
#include <future>
#include <limits>
#include <string>
#include <type_traits>

#if defined( WP_THREADING_STANDALONE_TESTS ) && WP_FINAL
#    include <malloc.h>
namespace
{
    bool countAllocations = false;
    std::size_t allocationCount = 0;
}  // namespace
void *operator new( std::size_t size )
{
    if( countAllocations )
        ++allocationCount;
    if( auto p = std::malloc( size ? size : 1 ) )
        return p;
    throw std::bad_alloc();
}
void *operator new[]( std::size_t size )
{
    return ::operator new( size );
}
void operator delete( void *p ) noexcept
{
    std::free( p );
}
void operator delete[]( void *p ) noexcept
{
    std::free( p );
}
void operator delete( void *p, std::size_t ) noexcept
{
    std::free( p );
}
void operator delete[]( void *p, std::size_t ) noexcept
{
    std::free( p );
}
void *operator new( std::size_t size, std::align_val_t alignment )
{
    if( countAllocations )
        ++allocationCount;
    if( auto p = _aligned_malloc( size ? size : 1, static_cast<std::size_t>( alignment ) ) )
        return p;
    throw std::bad_alloc();
}
void *operator new[]( std::size_t size, std::align_val_t a )
{
    return ::operator new( size, a );
}
void operator delete( void *p, std::align_val_t ) noexcept
{
    _aligned_free( p );
}
void operator delete[]( void *p, std::align_val_t ) noexcept
{
    _aligned_free( p );
}
void operator delete( void *p, std::size_t, std::align_val_t ) noexcept
{
    _aligned_free( p );
}
void operator delete[]( void *p, std::size_t, std::align_val_t ) noexcept
{
    _aligned_free( p );
}
#endif

using namespace workphone;

static_assert( !std::is_copy_constructible<SpinMutex::ScopedLock>::value, "lock guards own one unlock" );
static_assert( !std::is_copy_constructible<SharedMutex::ScopedLock>::value,
               "lock guards own one unlock" );
static_assert( !std::is_copy_constructible<RecursiveMutex::ScopedLock>::value,
               "lock guards own one unlock" );
static_assert( !std::is_copy_constructible<RecursiveMutex::ScopedSharedLock>::value,
               "lock guards own one unlock" );
static_assert( std::is_const<typename std::remove_reference<
                   decltype( *std::declval<ConcurrentSet<int>::iterator>() )>::type>::value,
               "set keys must not be mutable" );

namespace
{
    // Exercise shutdown after the diagnostics maps were first touched by a global object.
    SpinMutex shutdownMutex;
    struct ShutdownProbe
    {
        ShutdownProbe()
        {
            shutdownMutex.lock();
            shutdownMutex.unlock();
        }
    } shutdownProbe;
    SharedMutex shutdownSharedMutex;
    struct ShutdownReadProbe
    {
        SharedMutex::ScopedLock guard;
        ShutdownReadProbe() : guard( shutdownSharedMutex, false )
        {
        }
    } shutdownReadProbe;
    std::atomic<bool> gateAllocation{ false }, allocationEntered{ false }, releaseAllocation{ false },
        allocatorMoved{ false };
    template <class Mutex>
    void checkDowngrade()
    {
        Mutex mutex;
        mutex.lock();
        mutex.lock_shared();
        mutex.lock_shared();
        mutex.unlock();
        auto canWrite = [&] {
            return std::async( std::launch::async,
                               [&] {
                                   const bool acquired = mutex.try_lock();
                                   if( acquired )
                                       mutex.unlock();
                                   return acquired;
                               } )
                .get();
        };
        BOOST_CHECK( !canWrite() );
        mutex.unlock_shared();
        BOOST_CHECK( !canWrite() );
        mutex.unlock_shared();
        BOOST_CHECK( canWrite() );
    }

    template <class T>
    struct UnequalAllocator
    {
        using value_type = T;
        using is_always_equal = std::false_type;
        using propagate_on_container_move_assignment = std::false_type;
        int id = 0;
        UnequalAllocator() = default;
        explicit UnequalAllocator( int value ) : id( value )
        {
        }
        UnequalAllocator( const UnequalAllocator & ) = default;
        UnequalAllocator &operator=( const UnequalAllocator & ) = default;
        UnequalAllocator &operator=( UnequalAllocator && ) = default;
        UnequalAllocator( UnequalAllocator &&other ) noexcept : id( other.id )
        {
            allocatorMoved.store( true );
            other.id = -1;
        }
        template <class U>
        UnequalAllocator( const UnequalAllocator<U> &other ) : id( other.id )
        {
        }
        T *allocate( std::size_t count )
        {
            if( gateAllocation.load() )
            {
                allocationEntered.store( true );
                while( !releaseAllocation.load() )
                    std::this_thread::yield();
            }
            return std::allocator<T>{}.allocate( count );
        }
        void deallocate( T *data, std::size_t count )
        {
            std::allocator<T>{}.deallocate( data, count );
        }
        bool operator==( const UnequalAllocator &other ) const
        {
            return id == other.id;
        }
        bool operator!=( const UnequalAllocator &other ) const
        {
            return !( *this == other );
        }
    };
    struct ThrowMove
    {
        ThrowMove() = default;
        ThrowMove( const ThrowMove & ) = delete;
        ThrowMove( ThrowMove && )
        {
            throw std::runtime_error( "move failed" );
        }
    };
}  // namespace

BOOST_AUTO_TEST_SUITE( ThreadingRegressionTests )

BOOST_AUTO_TEST_CASE( recursive_mutex_preserves_writer_reads_on_downgrade )
{
    checkDowngrade<RecursiveMutex>();
    checkDowngrade<RecursiveSpinRWMutex>();
}

BOOST_AUTO_TEST_CASE( spin_rw_reports_read_ownership_and_supports_atomic_upgrade )
{
    RecursiveSpinRWMutex mutex;
    mutex.lock_shared();
    BOOST_CHECK( mutex.is_read_locked_by_current_thread() );
    BOOST_REQUIRE( mutex.try_lock() );
    BOOST_CHECK( mutex.is_write_locked_by_current_thread() );
    mutex.unlock();
    BOOST_CHECK( mutex.is_read_locked_by_current_thread() );
    mutex.unlock_shared();
    BOOST_CHECK( !mutex.is_read_locked_by_current_thread() );
}

BOOST_AUTO_TEST_CASE( recursive_mutex_own_upgrade_reservation_succeeds )
{
    RecursiveMutex mutex;
    mutex.lock_shared_write();
    BOOST_REQUIRE( mutex.try_unlock_shared_and_lock() );
    mutex.unlock();
}

BOOST_AUTO_TEST_CASE( hash_map_growth_preserves_aliased_values )
{
    ConcurrentHashMap<int, std::string> map( 1, GrowthPolicy::Double, 1 );
    map.emplace( 1, std::string( 200, 'x' ) );
    auto view = map.writeLocked();
    view.emplace( 2, view.at( 1 ) );
    BOOST_CHECK_EQUAL( view.at( 2 ), std::string( 200, 'x' ) );
}

BOOST_AUTO_TEST_CASE( hash_map_rejects_overflowing_growth )
{
    ConcurrentHashMap<int, int> map( 1, GrowthPolicy::Grow, 1 );
    map.emplace( 1, 42 );
    map.setGrowthSize( std::numeric_limits<std::size_t>::max() );
    BOOST_CHECK_THROW( map.emplace( 2, 43 ), std::length_error );
    BOOST_CHECK_EQUAL( map.size(), 1u );
    BOOST_CHECK_EQUAL( map.capacity(), 1u );
    BOOST_CHECK_EQUAL( map.at( 1 ), 42 );
}

BOOST_AUTO_TEST_CASE( set_middle_insertion_preserves_order_and_capacity )
{
    ConcurrentSet<int> values( 4, GrowthPolicy::Fixed, 1 );
    values.insert( 4 );
    values.insert( 1 );
    values.insert( 3 );
    values.insert( 2 );
    auto view = values.readLocked();
    int expected = 1;
    for( auto value : view )
        BOOST_CHECK_EQUAL( value, expected++ );
    BOOST_CHECK_EQUAL( values.capacity(), 4u );
}

BOOST_AUTO_TEST_CASE( queue_move_assignment_propagates_element_failure )
{
    using Queue = ConcurrentQueueBase<ThrowMove, UnequalAllocator<ThrowMove>>;
    Queue source( 1, GrowthPolicy::Double, 1, UnequalAllocator<ThrowMove>( 1 ) );
    Queue target( 0, GrowthPolicy::Double, 1, UnequalAllocator<ThrowMove>( 2 ) );
    source.emplace();
    BOOST_CHECK_THROW( target = std::move( source ), std::runtime_error );
}

BOOST_AUTO_TEST_CASE( deque_move_assignment_propagates_element_failure )
{
    using Deque = ConcurrentDeque<ThrowMove, UnequalAllocator<ThrowMove>>;
    Deque source( 1, GrowthPolicy::Double, 1, UnequalAllocator<ThrowMove>( 1 ) );
    Deque target( 0, GrowthPolicy::Double, 1, UnequalAllocator<ThrowMove>( 2 ) );
    source.emplace_back();
    BOOST_CHECK_THROW( target = std::move( source ), std::runtime_error );
}

BOOST_AUTO_TEST_CASE( deque_end_alone_keeps_range_stable )
{
    ConcurrentDeque<int> deque( 4, GrowthPolicy::Fixed, 1 );
    deque.push_back( 1 );
    std::future<bool> acquired;
    {
        auto end = deque.end();
        acquired = std::async( std::launch::async, [&] {
            bool result = deque.try_lock();
            if( result )
                deque.unlock();
            return result;
        } );
        BOOST_CHECK( !acquired.get() );
        BOOST_CHECK_EQUAL( end - deque.begin(), 1 );
    }
    BOOST_CHECK( deque.try_lock() );
    deque.unlock();
}

BOOST_AUTO_TEST_CASE( shared_guard_releases_the_selected_mode )
{
    SharedMutex mutex;
    {
        SharedMutex::ScopedLock guard( mutex, false );
    }
    mutex.lock();
    mutex.unlock();
}

BOOST_AUTO_TEST_CASE( queue_move_does_not_touch_allocator_before_source_lock )
{
    using Queue = ConcurrentQueueBase<int, UnequalAllocator<int>>;
    Queue source( 1, GrowthPolicy::Double, 1, UnequalAllocator<int>( 1 ) );
    source.push( 9 );
    allocatorMoved = false;
    allocationEntered = false;
    releaseAllocation = false;
    gateAllocation = true;
    std::thread reserving( [&] { source.reserve( 2 ); } );
    while( !allocationEntered.load() )
        std::this_thread::yield();
    auto moving = std::async( std::launch::async, [&] {
        Queue target( std::move( source ) );
        int value = 0;
        target.try_pop( value );
        return value;
    } );
    const auto status = moving.wait_for( std::chrono::milliseconds( 50 ) );
    const bool movedEarly = allocatorMoved.load();
    releaseAllocation = true;
    reserving.join();
    gateAllocation = false;
    BOOST_CHECK( status == std::future_status::timeout );
    BOOST_CHECK( !movedEarly );
    BOOST_CHECK_EQUAL( moving.get(), 9 );
}

#if defined( WP_THREADING_STANDALONE_TESTS ) && WP_FINAL
BOOST_AUTO_TEST_CASE( set_reserved_middle_insertion_does_not_allocate )
{
    ConcurrentSet<int> values( 4, GrowthPolicy::Fixed, 1 );
    values.insert( 4 );
    allocationCount = 0;
    countAllocations = true;
    try
    {
        values.insert( 1 );
        values.insert( 3 );
        values.insert( 2 );
    }
    catch( ... )
    {
        countAllocations = false;
        throw;
    }
    countAllocations = false;
    BOOST_CHECK_EQUAL( allocationCount, 0u );
}
#endif

BOOST_AUTO_TEST_SUITE_END()
