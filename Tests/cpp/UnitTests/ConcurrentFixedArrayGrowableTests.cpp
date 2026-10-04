#ifdef WP_CONCURRENT_FIXED_ARRAY_GROWABLE_STANDALONE_TESTS
#    define BOOST_TEST_MODULE ConcurrentFixedArrayGrowableTests
#    include <boost/test/included/unit_test.hpp>
#    include <cstdlib>
#    include <malloc.h>
#else
#    include <boost/test/unit_test.hpp>
#endif

#include <Workphone/Core/ConcurrentFixedArrayGrowable.hpp>
#include <Workphone/Core/FixedArrayGrowable.hpp>
#include <Workphone/Core/ConcurrentFixedArrayGrowable.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <type_traits>
#include <vector>

#ifdef WP_CONCURRENT_FIXED_ARRAY_GROWABLE_STANDALONE_TESTS
namespace
{
    bool countConcurrentAllocations = false;
    std::size_t concurrentAllocationCount = 0;
}
void *operator new( std::size_t size )
{
    if( countConcurrentAllocations )
        ++concurrentAllocationCount;
    if( auto *p = std::malloc( size == 0 ? 1 : size ) )
        return p;
    throw std::bad_alloc();
}
void *operator new[]( std::size_t size ) { return ::operator new( size ); }
void operator delete( void *p ) noexcept { std::free( p ); }
void operator delete[]( void *p ) noexcept { std::free( p ); }
void operator delete( void *p, std::size_t ) noexcept { std::free( p ); }
void operator delete[]( void *p, std::size_t ) noexcept { std::free( p ); }
void *operator new( std::size_t size, std::align_val_t alignment )
{
    if( countConcurrentAllocations )
        ++concurrentAllocationCount;
    if( auto *p = _aligned_malloc( size == 0 ? 1 : size, static_cast<std::size_t>( alignment ) ) )
        return p;
    throw std::bad_alloc();
}
void *operator new[]( std::size_t size, std::align_val_t a ) { return ::operator new( size, a ); }
void operator delete( void *p, std::align_val_t ) noexcept { _aligned_free( p ); }
void operator delete[]( void *p, std::align_val_t ) noexcept { _aligned_free( p ); }
void operator delete( void *p, std::size_t, std::align_val_t ) noexcept { _aligned_free( p ); }
void operator delete[]( void *p, std::size_t, std::align_val_t ) noexcept { _aligned_free( p ); }
#endif

namespace
{
    struct ConcurrentMoveOnly
    {
        int value;
        explicit ConcurrentMoveOnly( int v ) : value( v ) {}
        ConcurrentMoveOnly( const ConcurrentMoveOnly & ) = delete;
        ConcurrentMoveOnly &operator=( const ConcurrentMoveOnly & ) = delete;
        ConcurrentMoveOnly( ConcurrentMoveOnly &&other ) noexcept : value( other.value )
        { other.value = -1; }
        ConcurrentMoveOnly &operator=( ConcurrentMoveOnly &&other ) noexcept
        { value = other.value; other.value = -1; return *this; }
    };

    struct ConcurrentThrowingValue
    {
        static bool fail;
        static bool failCopy;
        int value;
        explicit ConcurrentThrowingValue( int v ) : value( v )
        {
            if( fail )
                throw std::runtime_error( "deliberate construction failure" );
        }
        ConcurrentThrowingValue( const ConcurrentThrowingValue &other ) : value( other.value )
        {
            if( failCopy )
                throw std::runtime_error( "deliberate copy failure" );
        }
    };
    bool ConcurrentThrowingValue::fail = false;
    bool ConcurrentThrowingValue::failCopy = false;
}

BOOST_AUTO_TEST_SUITE( concurrent_fixed_array_growable_tests )

BOOST_AUTO_TEST_CASE( fixed_capacity_modifiers_and_copy_reads )
{
    workphone::ConcurrentFixedArrayGrowable<int, 12> values{ 3, 1, 2 };
    static_assert( std::is_same<decltype( values.at( 0 ) ), int>::value, "reads return values" );
    static_assert( std::is_same<decltype( values.emplace_back( 4 ) ), void>::value,
                   "unlocked emplace must not leak a reference" );
    values.sort();
    BOOST_TEST( values.front() == 1 );
    BOOST_TEST( values.back() == 3 );
    auto copied = values.at( 1 );
    copied = 99;
    BOOST_TEST( copied == 99 );
    BOOST_TEST( values[1] == 2 );
    values.set( 1, 8 );
    values.insert( 1u, { 4, 5 } );
    values.insert( 0u, 2u, 9 );
    values.erase( 0u, 2u );
    values.erase( 1u );
    BOOST_TEST( values.size() == 4u );
    BOOST_TEST( values[1] == 5 );
    BOOST_TEST( values.find_if( []( int v ) { return v == 8; } ) == 2u );
    int out = 0;
    BOOST_TEST( values.try_front( out ) );
    BOOST_TEST( out == 1 );
    BOOST_TEST( values.try_back( out ) );
    BOOST_TEST( out == 3 );
    BOOST_TEST( values.try_pop_back( out ) );
    BOOST_TEST( out == 3 );
    auto snapshot = values.snapshot();
    snapshot[0] = 77;
    BOOST_TEST( values.front() == 1 );
    values.resize( 12, 6 );
    BOOST_TEST( values.full() );
    BOOST_TEST( !values.try_push_back( 9 ) );
    BOOST_CHECK_THROW( values.push_back( 9 ), std::length_error );
    BOOST_CHECK_THROW( values.reserve( 13 ), std::length_error );
    BOOST_CHECK_THROW( values.insert( 99u, 1 ), std::out_of_range );
    BOOST_CHECK_THROW( values.erase( 2u, 1u ), std::out_of_range );
    values.clear();
    values.pop_back();
    BOOST_TEST( !values.try_at( 0, out ) );
    BOOST_TEST( !values.try_back( out ) );
    BOOST_TEST( !values.try_pop_back( out ) );
    BOOST_CHECK_THROW( values.front(), std::out_of_range );
    BOOST_TEST( values.capacity() == 12u );
    values.reserve( 12 );
    values.shrink_to_fit();
    values.assign( { 2, 4 } );
    values.assign( 3, 7 );
    int range[]{ 5, 6 };
    values.assign( std::begin( range ), std::end( range ) );
    values.emplace( 1u, 8 );
    BOOST_TEST( values[1] == 8 );
}

BOOST_AUTO_TEST_CASE( locked_views_expose_references_and_the_underlying_api )
{
    using Array = workphone::ConcurrentFixedArrayGrowable<int, 8>;
    static_assert( !std::is_copy_constructible<Array::ReadLockedView>::value, "locks cannot copy" );
    static_assert( std::is_nothrow_move_constructible<Array::WriteLockedView>::value, "move guard" );
    static_assert( !std::is_copy_constructible<workphone::SpinRWMutex::ScopedLock>::value,
                   "scoped locks cannot double unlock" );
    Array values{ 3, 1, 2 };
    {
        auto write = values.writeLocked();
        write.emplace_back( 4 );
        write[0] = 9;
        write.sort();
        write->insert( write.begin(), write.begin(), write.end() );
        write.erase( write.begin(), write.begin() + 4 );
        auto moved = std::move( write );
        BOOST_CHECK( !write );
        BOOST_TEST( moved.front() == 1 );
        BOOST_TEST( moved.back() == 9 );
        BOOST_TEST( moved.data()[1] == 2 );
        moved.get().pop_back();
    }
    {
        const auto &constant = values;
        auto read = constant.readLocked();
        static_assert( std::is_same<decltype( read[0] ), const int &>::value, "const references" );
        static_assert( std::is_same<decltype( read.data() ), const int *>::value, "const data" );
        const std::vector<int> expected{ 1, 2, 4 };
        BOOST_CHECK_EQUAL_COLLECTIONS( read.begin(), read.end(), expected.begin(), expected.end() );
        BOOST_TEST( *read.crbegin() == 4 );
        BOOST_TEST( read->find_if( []( int v ) { return v == 2; } ) == 1u );
    }
    Array other;
    {
        auto first = values.writeLocked();
        auto second = other.writeLocked();
        first = std::move( second );
        BOOST_CHECK( !second );
        values.push_back( 5 );  // Move assignment released the old owner's lock.
        first.push_back( 7 );
    }
    BOOST_TEST( other.front() == 7 );
}

BOOST_AUTO_TEST_CASE( copy_move_swap_and_zero_capacity )
{
    workphone::ConcurrentFixedArrayGrowable<int, 4> a{ 1, 2 };
    workphone::ConcurrentFixedArrayGrowable<int, 4> b( a );
    BOOST_CHECK( a == b );
    b.set( 0, 9 );
    BOOST_TEST( a.front() == 1 );
    b = a;
    auto &same = b;
    b = same;
    b = std::move( same );
    b.swap( b );
    BOOST_CHECK( a == b );
    workphone::ConcurrentFixedArrayGrowable<int, 4> moved( std::move( b ) );
    BOOST_TEST( b.empty() );
    b.push_back( 7 );
    swap( b, moved );
    BOOST_TEST( b.size() == 2u );
    BOOST_TEST( moved.front() == 7 );
    moved = std::move( b );
    BOOST_TEST( b.empty() );
    BOOST_TEST( moved.back() == 2 );
    workphone::ConcurrentFixedArrayGrowable<int, 0> zero;
    BOOST_TEST( zero.full() );
    BOOST_TEST( zero.empty() );
    BOOST_TEST( !zero.try_push_back( 1 ) );
    BOOST_CHECK_THROW( zero.push_back( 1 ), std::length_error );
    zero.resize( 0 );
    zero.sort();
    BOOST_CHECK( zero.readLocked().data() == nullptr );
}

BOOST_AUTO_TEST_CASE( move_only_values_and_exceptions_release_locks )
{
    workphone::ConcurrentFixedArrayGrowable<ConcurrentMoveOnly, 4> values;
    values.emplace_back( 1 );
    values.emplace( 0u, 2 );
    {
        auto view = values.readLocked();
        BOOST_TEST( view.front().value == 2 );
    }
    ConcurrentMoveOnly output( 0 );
    BOOST_TEST( values.try_pop_back( output ) );
    BOOST_TEST( output.value == 1 );
    workphone::ConcurrentFixedArrayGrowable<ConcurrentMoveOnly, 4> moved( std::move( values ) );
    BOOST_TEST( values.empty() );
    BOOST_TEST( moved.readLocked().front().value == 2 );
    workphone::ConcurrentFixedArrayGrowable<ConcurrentThrowingValue, 2> throwing;
    ConcurrentThrowingValue::fail = true;
    BOOST_CHECK_THROW( throwing.emplace_back( 1 ), std::runtime_error );
    ConcurrentThrowingValue::fail = false;
    BOOST_TEST( throwing.empty() );
    throwing.emplace_back( 9 );
    BOOST_CHECK_THROW( throwing.find_if( []( const auto & ) -> bool {
        throw std::runtime_error( "deliberate predicate failure" );
    } ), std::runtime_error );
    throwing.clear();
    BOOST_TEST( throwing.empty() );
    try
    {
        auto view = throwing.writeLocked();
        throw std::runtime_error( "deliberate view scope failure" );
    }
    catch( const std::runtime_error & )
    {
    }
    throwing.emplace_back( 7 );
    BOOST_TEST( throwing.front().value == 7 );
    workphone::ConcurrentFixedArrayGrowable<ConcurrentThrowingValue, 2> target;
    ConcurrentThrowingValue::failCopy = true;
    BOOST_CHECK_THROW( target = throwing, std::runtime_error );
    BOOST_CHECK_THROW( throwing.snapshot(), std::runtime_error );
    ConcurrentThrowingValue::failCopy = false;
    target = throwing;
    throwing = target;
    BOOST_TEST( target.front().value == 7 );
}

BOOST_AUTO_TEST_CASE( spin_mutex_excludes_a_second_writer )
{
    workphone::SpinRWMutex mutex;
    mutex.lock();
    std::promise<void> attempting;
    std::promise<void> acquired;
    auto attemptingFuture = attempting.get_future();
    auto acquiredFuture = acquired.get_future();
    std::thread writer( [&] {
        attempting.set_value();
        workphone::SpinRWMutex::ScopedLock lock( mutex );
        acquired.set_value();
    } );
    attemptingFuture.wait();
    const bool overlapped = acquiredFuture.wait_for( std::chrono::milliseconds( 100 ) ) ==
                            std::future_status::ready;
    mutex.unlock();
    writer.join();
    BOOST_TEST( !overlapped );
}

BOOST_AUTO_TEST_CASE( read_views_share_the_lock_and_block_writers )
{
    workphone::ConcurrentFixedArrayGrowable<int, 4> values{ 1 };
    std::promise<void> readerAcquired;
    std::promise<void> writerAttempting;
    std::promise<void> writerAcquired;
    auto readerFuture = readerAcquired.get_future();
    auto attemptingFuture = writerAttempting.get_future();
    auto writerFuture = writerAcquired.get_future();
    std::thread reader;
    std::thread writer;
    bool readerShared = false;
    bool writerOverlapped = false;
    {
        auto view = values.readLocked();
        reader = std::thread( [&] {
            auto otherRead = values.readLocked();
            readerAcquired.set_value();
        } );
        readerShared = readerFuture.wait_for( std::chrono::seconds( 2 ) ) == std::future_status::ready;
        writer = std::thread( [&] {
            writerAttempting.set_value();
            values.push_back( 2 );
            writerAcquired.set_value();
        } );
        attemptingFuture.wait();
        writerOverlapped = writerFuture.wait_for( std::chrono::milliseconds( 100 ) ) ==
                           std::future_status::ready;
    }
    reader.join();
    writer.join();
    BOOST_TEST( readerShared );
    BOOST_TEST( !writerOverlapped );
    BOOST_TEST( values.back() == 2 );
}

BOOST_AUTO_TEST_CASE( reader_writer_stress_preserves_compound_updates )
{
    workphone::ConcurrentFixedArrayGrowable<int, 2> values{ 0, 0 };
    std::atomic<int> violations{ 0 };
    std::atomic<bool> start{ false };
    std::vector<std::thread> threads;
    for( int i = 0; i < 4; ++i )
        threads.emplace_back( [&] {
            while( !start.load() )
                std::this_thread::yield();
            for( int n = 0; n < 4000; ++n )
            {
                auto view = values.writeLocked();
                ++view[0];
                std::this_thread::yield();
                view[1] = view[0];
            }
        } );
    for( int i = 0; i < 4; ++i )
        threads.emplace_back( [&] {
            while( !start.load() )
                std::this_thread::yield();
            for( int n = 0; n < 4000; ++n )
            {
                auto view = values.readLocked();
                if( view[0] != view[1] )
                    ++violations;
            }
        } );
    start.store( true );
    for( auto &thread : threads )
        thread.join();
    BOOST_TEST( violations.load() == 0 );
    BOOST_TEST( values.front() == 16000 );
    BOOST_TEST( values.back() == 16000 );
}

BOOST_AUTO_TEST_CASE( concurrent_push_respects_capacity_and_pop_removes_each_value_once )
{
    workphone::ConcurrentFixedArrayGrowable<int, 128> values;
    std::atomic<int> accepted{ 0 };
    std::vector<std::thread> threads;
    for( int t = 0; t < 8; ++t )
        threads.emplace_back( [&, t] {
            for( int i = 0; i < 32; ++i )
                if( values.try_push_back( t * 32 + i ) )
                    ++accepted;
        } );
    for( auto &thread : threads )
        thread.join();
    BOOST_TEST( accepted.load() == 128 );
    BOOST_TEST( values.size() == 128u );
    std::atomic<int> popped{ 0 };
    std::atomic<int> seen[256]{};
    threads.clear();
    for( int t = 0; t < 8; ++t )
        threads.emplace_back( [&] {
            int value;
            while( values.try_pop_back( value ) )
            {
                ++seen[value];
                ++popped;
            }
        } );
    for( auto &thread : threads )
        thread.join();
    BOOST_TEST( popped.load() == 128 );
    for( auto &count : seen )
        BOOST_TEST( count.load() <= 1 );
    BOOST_TEST( values.empty() );
}

BOOST_AUTO_TEST_CASE( opposing_assignments_and_swaps_use_consistent_lock_order )
{
    workphone::ConcurrentFixedArrayGrowable<int, 4> a{ 1, 2 };
    workphone::ConcurrentFixedArrayGrowable<int, 4> b{ 3, 4 };
    std::thread first( [&] {
        for( int i = 0; i < 1000; ++i )
        {
            a = b;
            a.swap( b );
        }
    } );
    std::thread second( [&] {
        for( int i = 0; i < 1000; ++i )
        {
            b = a;
            b.swap( a );
        }
    } );
    first.join();
    second.join();
    BOOST_TEST( a.size() == 2u );
    BOOST_TEST( b.size() == 2u );
    BOOST_CHECK( a == b );
}

#ifdef WP_CONCURRENT_FIXED_ARRAY_GROWABLE_STANDALONE_TESTS
BOOST_AUTO_TEST_CASE( successful_operations_and_views_do_not_allocate )
{
    concurrentAllocationCount = 0;
    countConcurrentAllocations = true;
    {
        workphone::ConcurrentFixedArrayGrowable<int, 16> a{ 1, 2, 3 };
        a.push_back( 4 );
        a.set( 0, 9 );
        a.insert( 1u, { 5, 6 } );
        a.erase( 0u );
        a.sort();
        auto snapshot = a.snapshot();
        workphone::ConcurrentFixedArrayGrowable<int, 16> b( a );
        b = a;
        workphone::ConcurrentFixedArrayGrowable<int, 16> c( std::move( b ) );
        b = std::move( c );
        a.swap( b );
        a.resize( 16, 0 );
        a.try_push_back( 1 );
        {
            auto view = a.writeLocked();
            view[0] = 1;
            view.pop_back();
        }
        {
            auto view = a.readLocked();
            snapshot[0] = view.front();
        }
        a.clear();
    }
    countConcurrentAllocations = false;
    BOOST_TEST( concurrentAllocationCount == 0u );
}
#endif

BOOST_AUTO_TEST_SUITE_END()
