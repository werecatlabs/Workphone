// Build independently with /DWP_FIXED_ARRAY_GROWABLE_STANDALONE_TESTS to exercise
// this header without linking the engine or its allocator.
#ifdef WP_FIXED_ARRAY_GROWABLE_STANDALONE_TESTS
#    define BOOST_TEST_MODULE FixedArrayGrowableTests
#    include <boost/test/included/unit_test.hpp>
#    include <cstdlib>
#    include <malloc.h>
#else
#    include <boost/test/unit_test.hpp>
#endif

#include <Workphone/Core/FixedArrayGrowable.hpp>
#include <Workphone/Core/FixedArrayGrowable.hpp>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#if defined( WP_FIXED_ARRAY_GROWABLE_STANDALONE_TESTS ) && defined( _WP_Array_h__ )
#    error FixedArrayGrowable must not use the dynamic Array header guard
#endif

#ifdef WP_FIXED_ARRAY_GROWABLE_STANDALONE_TESTS
namespace
{
    bool countAllocations = false;
    std::size_t allocationCount = 0;
}
void *operator new( std::size_t size )
{
    if( countAllocations )
        ++allocationCount;
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
    if( countAllocations )
        ++allocationCount;
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
    struct MoveOnly
    {
        int value;
        explicit MoveOnly( int v ) : value( v ) {}
        MoveOnly( const MoveOnly & ) = delete;
        MoveOnly &operator=( const MoveOnly & ) = delete;
        MoveOnly( MoveOnly &&other ) noexcept : value( other.value ) { other.value = -1; }
        MoveOnly &operator=( MoveOnly &&other ) noexcept
        { value = other.value; other.value = -1; return *this; }
    };

    struct Tracked
    {
        static int alive;
        static int copiesBeforeThrow;
        static int movesBeforeThrow;
        static int assignmentsBeforeThrow;
        int value;
        explicit Tracked( int v = 0 ) : value( v ) { ++alive; }
        Tracked( const Tracked &other ) : value( other.value )
        { check( copiesBeforeThrow ); ++alive; }
        Tracked( Tracked &&other ) : value( other.value )
        { check( movesBeforeThrow ); other.value = -1; ++alive; }
        Tracked &operator=( const Tracked &other )
        { check( assignmentsBeforeThrow ); value = other.value; return *this; }
        Tracked &operator=( Tracked &&other )
        { check( assignmentsBeforeThrow ); value = other.value; other.value = -1; return *this; }
        ~Tracked() { --alive; }
        static void check( int &budget )
        {
            if( budget == 0 )
                throw std::runtime_error( "deliberate element failure" );
            if( budget > 0 )
                --budget;
        }
        static void reset()
        { copiesBeforeThrow = movesBeforeThrow = assignmentsBeforeThrow = -1; }
    };
    int Tracked::alive = 0;
    int Tracked::copiesBeforeThrow = -1;
    int Tracked::movesBeforeThrow = -1;
    int Tracked::assignmentsBeforeThrow = -1;

    struct alignas( 128 ) AlignedValue
    {
        const int value;
        explicit AlignedValue( int v ) : value( v ) {}
    };
}

BOOST_AUTO_TEST_SUITE( fixed_array_growable_tests )

BOOST_AUTO_TEST_CASE( standard_array_iterators_work_with_standard_algorithms )
{
    using Array = workphone::FixedArrayGrowable<int, 8>;
    Array values{ 1, 2, 3, 2 };
    const auto &constant = values;
    static_assert( std::is_same<decltype( values.begin() ), Array::iterator>::value, "iterator" );
    static_assert( std::is_same<decltype( constant.begin() ), Array::const_iterator>::value,
                   "const iterator" );
    static_assert( std::is_same<decltype( values.data() ), Array::pointer>::value, "data" );
    static_assert( std::is_same<decltype( constant.at( 0 ) ), Array::const_reference>::value,
                   "const reference" );
    auto found = std::find( std::begin( values ), std::end( values ), 2 );
    BOOST_CHECK( found == values.begin() + 1 );
    BOOST_CHECK( std::find( std::cbegin( constant ), std::cend( constant ), 3 ) == values.cbegin() + 2 );
    BOOST_CHECK( std::find( values.crbegin(), values.crend(), 3 ) == values.crbegin() + 1 );
    BOOST_CHECK( std::find( values.begin(), values.end(), 99 ) == values.end() );
    BOOST_TEST( std::distance( values.cbegin(), values.cend() ) == 4 );
    *found = 7;
    values.erase( std::remove( values.begin(), values.end(), 2 ), values.end() );
    BOOST_TEST( values.size() == 3u );
    values.fill( 9 );
    BOOST_TEST( values.size() == 3u );
    BOOST_CHECK( std::all_of( values.begin(), values.end(), []( int v ) { return v == 9; } ) );
    Array smaller{ 9, 9 };
    BOOST_CHECK( smaller < values );
    BOOST_CHECK( values > smaller );
    BOOST_CHECK( smaller <= values );
    BOOST_CHECK( values >= smaller );
    Array zero;
    BOOST_CHECK( std::find( zero.begin(), zero.end(), 1 ) == zero.end() );
    zero.fill( 1 );
    BOOST_TEST( zero.empty() );
    Array copy;
    copy.assign( values.cbegin(), values.cend() );
    BOOST_CHECK( copy == values );
}

BOOST_AUTO_TEST_CASE( storage_capacity_access_and_iterators )
{
    using Array = workphone::FixedArrayGrowable<int, 6>;
    Array values;
    BOOST_TEST( values.empty() );
    BOOST_TEST( values.capacity() == 6u );
    auto *storage = values.data();
    const auto start = reinterpret_cast<std::uintptr_t>( &values );
    const auto data = reinterpret_cast<std::uintptr_t>( storage );
    BOOST_TEST( data >= start );
    BOOST_TEST( data + 6 * sizeof( int ) <= start + sizeof( values ) );
    values.assign( { 3, 1, 2 } );
    values.sort();
    BOOST_TEST( values.front() == 1 );
    BOOST_TEST( values.back() == 3 );
    BOOST_TEST( values.data() == storage );
    BOOST_TEST( values.data()[1] == 2 );
    const auto &constant = values;
    BOOST_TEST( (values.end() - constant.begin()) == 3 );
    BOOST_CHECK( 1 + values.begin() == constant.begin() + 1 );
    BOOST_CHECK( values.begin() < constant.end() );
    BOOST_TEST( *constant.crbegin() == 3 );
    BOOST_TEST( values.find_if( []( int v ) { return v == 2; } ) == 1u );
    BOOST_TEST( values.find_if( []( int v ) { return v == 9; } ) == static_cast<std::size_t>( -1 ) );
    BOOST_CHECK_THROW( values.at( 3 ), std::out_of_range );
    BOOST_CHECK_THROW( values[3], std::out_of_range );
    BOOST_CHECK_THROW( values[static_cast<std::size_t>( -1 )], std::out_of_range );
    auto snapshot = values.snapshot();
    snapshot[0] = 9;
    BOOST_TEST( values[0] == 1 );
    values.clear();
    BOOST_TEST( values.capacity() == 6u );
    BOOST_TEST( values.data() == storage );
    BOOST_CHECK_THROW( values.front(), std::out_of_range );
    BOOST_CHECK_THROW( values.back(), std::out_of_range );
    values.pop_back();
    BOOST_TEST( values.empty() );
}

BOOST_AUTO_TEST_CASE( overflow_is_checked_before_modification )
{
    workphone::FixedArrayGrowable<int, 3> values{ 1, 2, 3 };
    BOOST_TEST( values.full() );
    BOOST_TEST( !values.try_push_back( 4 ) );
    BOOST_CHECK_THROW( values.emplace_back( 4 ), std::length_error );
    BOOST_CHECK_THROW( values.emplace( 1u, 4 ), std::length_error );
    BOOST_CHECK_THROW( values.resize( 4 ), std::length_error );
    BOOST_CHECK_THROW( values.reserve( 4 ), std::length_error );
    BOOST_CHECK_THROW( values.assign( 4, 7 ), std::length_error );
    BOOST_CHECK_THROW( values.insert( values.begin(), (std::numeric_limits<std::size_t>::max)(), 8 ),
                       std::length_error );
    BOOST_TEST( values.size() == 3u );
    BOOST_TEST( values[0] == 1 );
    values.pop_back();
    BOOST_CHECK_THROW( values.insert( values.begin(), { 8, 9 } ), std::length_error );
    BOOST_TEST( values.size() == 2u );
    BOOST_TEST( values[0] == 1 );
    BOOST_TEST( values.try_emplace_back( 4 ) );
    BOOST_TEST( values.back() == 4 );
    values.reserve( 0 );
    values.shrink_to_fit();
    BOOST_TEST( values.capacity() == 3u );
}

BOOST_AUTO_TEST_CASE( insert_erase_self_ranges_and_single_pass_ranges )
{
    workphone::FixedArrayGrowable<int, 12> values{ 1, 2, 3 };
    values.insert( values.begin() + 1, values.begin(), values.end() );
    const std::vector<int> expected{ 1, 1, 2, 3, 2, 3 };
    BOOST_CHECK_EQUAL_COLLECTIONS( values.begin(), values.end(), expected.begin(), expected.end() );
    auto next = values.erase( values.begin() + 1, values.begin() + 4 );
    BOOST_TEST( *next == 2 );
    values.emplace( 0u, values.back() );
    BOOST_TEST( values.front() == 3 );
    values.assign( values.begin() + 1, values.end() );
    BOOST_TEST( values.size() == 3u );
    BOOST_TEST( values.front() == 1 );
    values.insert( values.begin() + 1, 2u, values.back() );
    BOOST_TEST( values[1] == 3 );
    BOOST_TEST( values[2] == 3 );
    values.erase( 0u );
    BOOST_CHECK( values.erase( values.end(), values.end() ) == values.end() );
    BOOST_CHECK_THROW( values.erase( values.end() ), std::out_of_range );
    BOOST_CHECK_THROW( values.erase( values.end(), values.begin() ), std::out_of_range );
    workphone::FixedArrayGrowable<int, 12> other;
    BOOST_CHECK_THROW( values.insert( other.begin(), 9 ), std::out_of_range );
    BOOST_CHECK_THROW( values.assign( values.end(), values.begin() ), std::out_of_range );
    std::istringstream input( "7 8 9" );
    values.assign( std::istream_iterator<int>( input ), std::istream_iterator<int>() );
    BOOST_TEST( values.size() == 3u );
    BOOST_TEST( values.front() == 7 );
    values.resize( 5, values.front() );
    BOOST_TEST( values.back() == 7 );
    values.resize( 2 );
    BOOST_TEST( values.back() == 8 );
}

BOOST_AUTO_TEST_CASE( zero_capacity_and_overaligned_nondefault_elements )
{
    workphone::FixedArrayGrowable<int, 0> zero;
    BOOST_TEST( zero.full() );
    BOOST_TEST( zero.empty() );
    BOOST_CHECK( zero.data() == nullptr );
    BOOST_TEST( (zero.end() - zero.begin()) == 0 );
    zero.reserve( 0 );
    zero.resize( 0 );
    zero.sort();
    BOOST_CHECK( zero.erase( zero.begin(), zero.end() ) == zero.end() );
    BOOST_TEST( !zero.try_push_back( 1 ) );
    BOOST_CHECK_THROW( zero.push_back( 1 ), std::length_error );
    workphone::FixedArrayGrowable<AlignedValue, 3> aligned;
    aligned.emplace_back( 42 );
    BOOST_TEST( reinterpret_cast<std::uintptr_t>( aligned.data() ) % alignof( AlignedValue ) == 0u );
    BOOST_TEST( aligned[0].value == 42 );
    aligned.clear();
    aligned.emplace_back( 99 );
    BOOST_TEST( aligned[0].value == 99 );
}

BOOST_AUTO_TEST_CASE( move_only_values_move_and_swap_unequal_sizes )
{
    workphone::FixedArrayGrowable<MoveOnly, 5> values;
    values.emplace_back( 1 );
    values.emplace_back( 3 );
    values.emplace( 1u, 2 );
    workphone::FixedArrayGrowable<MoveOnly, 5> moved( std::move( values ) );
    BOOST_TEST( values.empty() );
    BOOST_TEST( moved[1].value == 2 );
    values.emplace_back( 9 );
    swap( values, moved );
    BOOST_TEST( values.size() == 3u );
    BOOST_TEST( moved.size() == 1u );
    BOOST_TEST( moved.front().value == 9 );
    values.erase( 1u );
    BOOST_TEST( values.back().value == 3 );
    moved = std::move( values );
    BOOST_TEST( values.empty() );
    BOOST_TEST( moved.size() == 2u );
    auto &self = moved;
    moved = std::move( self );
    BOOST_TEST( moved.size() == 2u );
    moved.swap( moved );
    BOOST_TEST( moved.front().value == 1 );
}

BOOST_AUTO_TEST_CASE( bulk_insert_matches_vector_for_all_positions_and_tail_sizes )
{
    for( std::size_t size = 0; size <= 8; ++size )
    {
        for( std::size_t position = 0; position <= size; ++position )
        {
            for( std::size_t count = 0; count <= 8; ++count )
            {
                workphone::FixedArrayGrowable<int, 16> actual;
                std::vector<int> expected;
                for( std::size_t i = 0; i < size; ++i )
                {
                    actual.push_back( static_cast<int>( i ) );
                    expected.push_back( static_cast<int>( i ) );
                }
                actual.insert( actual.begin() + position, count, 99 );
                expected.insert( expected.begin() + position, count, 99 );
                BOOST_CHECK_EQUAL_COLLECTIONS( actual.begin(), actual.end(),
                                               expected.begin(), expected.end() );
            }
        }
    }
    workphone::FixedArrayGrowable<MoveOnly, 8> source;
    source.emplace_back( 7 );
    source.emplace_back( 8 );
    source.emplace_back( 9 );
    workphone::FixedArrayGrowable<MoveOnly, 8> target;
    target.emplace_back( 1 );
    target.emplace_back( 2 );
    target.insert( target.begin() + 1, std::make_move_iterator( source.begin() ),
                   std::make_move_iterator( source.end() ) );
    BOOST_TEST( target.size() == 5u );
    BOOST_TEST( target[0].value == 1 );
    BOOST_TEST( target[1].value == 7 );
    BOOST_TEST( target[2].value == 8 );
    BOOST_TEST( target[3].value == 9 );
    BOOST_TEST( target[4].value == 2 );
}

BOOST_AUTO_TEST_CASE( lifetime_and_exception_cleanup )
{
    Tracked::reset();
    BOOST_TEST( Tracked::alive == 0 );
    {
        workphone::FixedArrayGrowable<Tracked, 5> values;
        BOOST_TEST( Tracked::alive == 0 );
        values.emplace_back( 1 );
        values.emplace_back( 2 );
        values.emplace_back( 3 );
        BOOST_TEST( Tracked::alive == 3 );
        Tracked::copiesBeforeThrow = 1;
        BOOST_CHECK_THROW( (workphone::FixedArrayGrowable<Tracked, 5>( values )), std::runtime_error );
        BOOST_TEST( Tracked::alive == 3 );
        Tracked::reset();
        Tracked::copiesBeforeThrow = 1;
        BOOST_CHECK_THROW( values.resize( 5, values.front() ), std::runtime_error );
        BOOST_TEST( values.size() == 3u );
        BOOST_TEST( Tracked::alive == 3 );
        Tracked::reset();
        Tracked::assignmentsBeforeThrow = 0;
        BOOST_CHECK_THROW( values.emplace( 0u, 8 ), std::runtime_error );
        BOOST_TEST( values.size() == 3u );
        BOOST_TEST( Tracked::alive == 3 );
        BOOST_CHECK_THROW( values.erase( 0u ), std::runtime_error );
        BOOST_TEST( values.size() == 3u );
        Tracked::reset();
        Tracked::movesBeforeThrow = 1;
        BOOST_CHECK_THROW( (workphone::FixedArrayGrowable<Tracked, 5>( std::move( values ) )),
                           std::runtime_error );
        BOOST_TEST( Tracked::alive == 3 );
        Tracked::reset();
        values.resize( 1 );
        BOOST_TEST( Tracked::alive == 1 );
        values.clear();
        BOOST_TEST( Tracked::alive == 0 );
    }
    BOOST_TEST( Tracked::alive == 0 );
}

BOOST_AUTO_TEST_CASE( throwing_bulk_insert_swap_and_move_assignment_track_all_elements )
{
    Tracked::reset();
    {
        workphone::FixedArrayGrowable<Tracked, 8> values;
        values.emplace_back( 1 );
        values.emplace_back( 2 );
        values.emplace_back( 3 );
        Tracked::assignmentsBeforeThrow = 0;
        BOOST_CHECK_THROW( values.insert( values.begin(), 2u, values.front() ), std::runtime_error );
        BOOST_TEST( values.size() == 3u );
        BOOST_TEST( Tracked::alive == 3 );
        Tracked::reset();
        workphone::FixedArrayGrowable<Tracked, 8> smaller;
        smaller.emplace_back( 9 );
        Tracked::copiesBeforeThrow = 1;
        BOOST_CHECK_THROW( values.swap( smaller ), std::runtime_error );
        BOOST_TEST( values.size() == 3u );
        BOOST_TEST( smaller.size() == 1u );
        BOOST_TEST( Tracked::alive == 4 );
        Tracked::reset();
        Tracked::movesBeforeThrow = 1;
        BOOST_CHECK_THROW( smaller = std::move( values ), std::runtime_error );
        BOOST_TEST( values.size() == 3u );
        BOOST_TEST( smaller.size() == 1u );
        BOOST_TEST( Tracked::alive == 4 );
        Tracked::reset();
    }
    BOOST_TEST( Tracked::alive == 0 );
}

#ifdef WP_FIXED_ARRAY_GROWABLE_STANDALONE_TESTS
BOOST_AUTO_TEST_CASE( successful_container_operations_do_not_allocate )
{
    allocationCount = 0;
    countAllocations = true;
    {
        workphone::FixedArrayGrowable<int, 16> values{ 3, 1, 2 };
        values.reserve( 16 );
        values.push_back( 4 );
        values.emplace( 1u, 5 );
        values.insert( values.begin(), values.begin(), values.end() );
        values.erase( values.begin() + 1, values.begin() + 3 );
        values.sort();
        values.fill( 3 );
        (void)std::find( values.cbegin(), values.cend(), 3 );
        (void)std::find( values.rbegin(), values.rend(), 3 );
        auto copied = values.snapshot();
        auto moved = std::move( copied );
        copied.assign( values.begin(), values.end() );
        copied.swap( moved );
        copied = moved;
        copied = std::move( moved );
        copied.resize( 16, 7 );
        copied.try_push_back( 8 );
        copied.clear();
        copied.shrink_to_fit();
    }
    countAllocations = false;
    BOOST_TEST( allocationCount == 0u );
}
#endif

BOOST_AUTO_TEST_SUITE_END()
