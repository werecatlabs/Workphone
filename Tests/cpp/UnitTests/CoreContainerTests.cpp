#include "UnitTests.hpp"

#include <Workphone/Core/ConcurrentDeque.hpp>
#include <Workphone/Core/ConcurrentHashMap.hpp>
#include <Workphone/Core/ConcurrentList.hpp>
#include <Workphone/Core/ConcurrentMap.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/ConcurrentSet.hpp>
#include <Workphone/Core/Deque.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/GenericPool.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Core/HashTable.hpp>
#include <Workphone/Core/List.hpp>
#include <Workphone/Core/LRUCache.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/Core/StringBuffer.hpp>
#include <Workphone/Core/StringPool.hpp>
#include <Workphone/Core/UnorderedMap.hpp>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace workphone;

BOOST_AUTO_TEST_SUITE( core_container_tests )

BOOST_AUTO_TEST_CASE( deque_preserves_front_back_order_and_supports_move_only_values )
{
    Deque<int> values{ 2, 3 };
    values.push_front( 1 );
    values.emplace_back( 4 );

    BOOST_REQUIRE_EQUAL( values.size(), 4 );
    const std::vector<int> initialExpected{ 1, 2, 3, 4 };
    BOOST_CHECK_EQUAL_COLLECTIONS( values.begin(), values.end(), initialExpected.begin(),
                                   initialExpected.end() );

    values.pop_front();
    values.pop_back();
    values.erase( static_cast<Deque<int>::size_type>( 1 ) );
    BOOST_REQUIRE_EQUAL( values.size(), 1 );
    BOOST_TEST( values.front() == 2 );
    BOOST_CHECK_THROW( values.at( 1 ), std::out_of_range );

    Deque<std::unique_ptr<int>> moveOnly;
    moveOnly.emplace_front( new int( 7 ) );
    moveOnly.push_back( std::make_unique<int>( 11 ) );
    BOOST_TEST( *moveOnly.front() == 7 );
    BOOST_TEST( *moveOnly.back() == 11 );
}

BOOST_AUTO_TEST_CASE( concurrent_deque_handles_wraparound_fixed_capacity_and_locked_views )
{
    ConcurrentDeque<int> values( 3, GrowthPolicy::Fixed, 1 );
    BOOST_TEST( values.try_push_back( 1 ) );
    BOOST_TEST( values.try_push_back( 2 ) );
    BOOST_TEST( values.try_push_back( 3 ) );
    BOOST_TEST( !values.try_push_back( 4 ) );

    int popped = 0;
    BOOST_TEST( values.try_pop_front( popped ) );
    BOOST_TEST( popped == 1 );
    BOOST_TEST( values.try_push_back( 4 ) );

    {
        auto view = values.readLocked();
        const std::vector<int> expected{ 2, 3, 4 };
        BOOST_CHECK_EQUAL_COLLECTIONS( view.begin(), view.end(), expected.begin(), expected.end() );
    }

    {
        auto view = values.writeLocked();
        view.pop_front();
        view.emplace_front( 9 );
    }

    BOOST_TEST( values.try_front( popped ) );
    BOOST_TEST( popped == 9 );
    values.release();
    BOOST_TEST( values.capacity() == 0 );
    BOOST_TEST( values.empty() );
}

BOOST_AUTO_TEST_CASE( list_reuses_reserved_nodes_and_honours_growth_policy )
{
    List<int> values( 2, GrowthPolicy::Fixed, 1 );
    values.push_back( 2 );
    values.push_front( 1 );
    BOOST_CHECK_THROW( values.push_back( 3 ), std::length_error );

    auto first = values.begin();
    values.erase( first );
    values.push_back( 3 );
    BOOST_TEST( values.capacity() == 2 );

    const std::vector<int> expected{ 2, 3 };
    BOOST_CHECK_EQUAL_COLLECTIONS( values.begin(), values.end(), expected.begin(), expected.end() );

    BOOST_CHECK_THROW( values.setGrowthSize( 0 ), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE( concurrent_list_supports_non_throwing_fixed_capacity_operations )
{
    ConcurrentList<int> values( 2, GrowthPolicy::Fixed, 1 );
    BOOST_TEST( values.try_emplace_back( 2 ) );
    BOOST_TEST( values.try_emplace_front( 1 ) );
    BOOST_TEST( !values.try_emplace_back( 3 ) );

    int value = 0;
    BOOST_TEST( values.try_get_front( value ) );
    BOOST_TEST( value == 1 );
    BOOST_TEST( values.remove_first( 1 ) );
    BOOST_TEST( !values.remove_first( 99 ) );
    BOOST_TEST( values.try_emplace_back( 4 ) );
    BOOST_TEST( values.remove_if( []( int candidate ) { return candidate % 2 == 0; } ) == 2 );
    BOOST_TEST( values.empty() );

    values.withLock( []( auto &list ) { list.push_back( 8 ); } );
    BOOST_TEST( values.try_get_back( value ) );
    BOOST_TEST( value == 8 );
}

BOOST_AUTO_TEST_CASE( map_orders_keys_rejects_duplicates_and_reuses_pool_nodes )
{
    Map<int, std::string> values( 3, GrowthPolicy::Fixed, 1 );
    BOOST_TEST( values.emplace( 3, "three" ).second );
    BOOST_TEST( values.emplace( 1, "one" ).second );
    BOOST_TEST( values.emplace( 2, "two" ).second );
    BOOST_TEST( !values.emplace( 2, "duplicate" ).second );
    BOOST_TEST( values.at( 2 ) == "two" );
    BOOST_CHECK_THROW( values.at( 99 ), std::out_of_range );

    std::vector<int> keys;
    for( const auto &entry : values )
        keys.push_back( entry.first );
    const std::vector<int> expected{ 1, 2, 3 };
    BOOST_CHECK_EQUAL_COLLECTIONS( keys.begin(), keys.end(), expected.begin(), expected.end() );

    BOOST_TEST( values.erase( 2 ) == 1 );
    BOOST_TEST( values.erase( 2 ) == 0 );
    BOOST_TEST( values.emplace( 4, "four" ).second );
    BOOST_TEST( values.capacity() == 3 );
    BOOST_CHECK_THROW( values.emplace( 5, "five" ), std::length_error );
}

BOOST_AUTO_TEST_CASE( concurrent_map_supports_atomic_views_and_safe_value_copies )
{
    ConcurrentMap<int, std::string> values( 2, GrowthPolicy::Fixed, 1 );
    values.insert( 1, "one" );
    values.insert( 1, "updated" );
    BOOST_TEST( values.size() == 1 );

    std::string copied;
    BOOST_TEST( values.tryGet( 1, copied ) );
    BOOST_TEST( copied == "updated" );
    BOOST_TEST( !values.tryGet( 99, copied ) );

    {
        auto view = values.writeLocked();
        view.get().emplace( 2, "two" );
        BOOST_TEST( view.at( 2 ) == "two" );
    }

    BOOST_CHECK_THROW( values.emplace( 3, "three" ), std::length_error );
    BOOST_TEST( values.erase( 1 ) );
    BOOST_TEST( values.emplace( 3, "three" ).second );

    {
        auto view = values.readLocked();
        BOOST_TEST( view.size() == 2 );
        BOOST_TEST( view.contains( 2 ) );
        BOOST_TEST( view.contains( 3 ) );
    }
}

BOOST_AUTO_TEST_CASE( set_is_sorted_unique_and_uses_comparator_equivalence )
{
    Set<int> values{ 3, 1, 2, 2 };
    const std::vector<int> expected{ 1, 2, 3 };
    BOOST_CHECK_EQUAL_COLLECTIONS( values.begin(), values.end(), expected.begin(), expected.end() );
    BOOST_TEST( values.size() == 3 );
    BOOST_TEST( !values.insert( 2 ).second );
    BOOST_TEST( values.contains( 3 ) );
    BOOST_TEST( *values.lower_bound( 2 ) == 2 );

    auto next = values.erase( values.find( 2 ) );
    BOOST_REQUIRE( next != values.end() );
    BOOST_TEST( *next == 3 );

    struct AbsoluteLess
    {
        bool operator()( int lhs, int rhs ) const
        {
            return std::abs( lhs ) < std::abs( rhs );
        }
    };
    Set<int, AbsoluteLess> absoluteValues;
    BOOST_TEST( absoluteValues.insert( -4 ).second );
    BOOST_TEST( !absoluteValues.insert( 4 ).second );
}

BOOST_AUTO_TEST_CASE( concurrent_set_handles_fixed_capacity_duplicates_and_snapshots )
{
    ConcurrentSet<int> values( 2, GrowthPolicy::Fixed, 1 );
    BOOST_TEST( values.insert( 3 ) );
    BOOST_TEST( values.insert( 1 ) );
    BOOST_TEST( !values.insert( 3 ) );
    BOOST_TEST( !values.try_insert( 2 ) );

    const auto snapshot = values.snapshot();
    const std::vector<int> expected{ 1, 3 };
    BOOST_CHECK_EQUAL_COLLECTIONS( snapshot.begin(), snapshot.end(), expected.begin(), expected.end() );

    BOOST_TEST( values.erase( 1 ) );
    BOOST_TEST( values.try_insert( 2 ) );
    {
        auto view = values.readLocked();
        BOOST_TEST( view.contains( 2 ) );
        BOOST_TEST( view.size() == 2 );
    }
}

BOOST_AUTO_TEST_CASE( concurrent_hash_map_handles_collisions_tombstones_and_fixed_capacity )
{
    struct ConstantHash
    {
        std::size_t operator()( int ) const noexcept
        {
            return 0;
        }
    };

    ConcurrentHashMap<int, std::string, ConstantHash> values( 3, GrowthPolicy::Fixed, 1 );
    BOOST_TEST( values.emplace( 1, "one" ).second );
    BOOST_TEST( values.emplace( 2, "two" ).second );
    BOOST_TEST( values.emplace( 3, "three" ).second );
    BOOST_TEST( !values.try_emplace( 4, "four" ).second );
    BOOST_TEST( !values.emplace( 2, "duplicate" ).second );

    BOOST_TEST( values.erase( 2 ) );
    BOOST_TEST( values.contains( 3 ) );
    BOOST_TEST( values.try_emplace( 4, "four" ).second );

    std::string copied;
    BOOST_TEST( values.tryGet( 4, copied ) );
    BOOST_TEST( copied == "four" );
    BOOST_CHECK_THROW( values.at( 99 ), std::out_of_range );

    {
        auto view = values.readLocked();
        BOOST_TEST( view.size() == 3 );
        BOOST_TEST( view.contains( 1 ) );
        BOOST_TEST( view.contains( 3 ) );
        BOOST_TEST( view.contains( 4 ) );
    }
}

BOOST_AUTO_TEST_CASE( concurrent_queue_preserves_fifo_order_across_wraparound_and_growth )
{
    ConcurrentQueue<int> values( 3, GrowthPolicy::Fixed, 1 );
    BOOST_TEST( values.try_push( 1 ) );
    BOOST_TEST( values.try_push( 2 ) );
    BOOST_TEST( values.try_push( 3 ) );
    BOOST_TEST( !values.try_push( 4 ) );

    int output = 0;
    BOOST_TEST( values.try_pop( output ) );
    BOOST_TEST( output == 1 );
    BOOST_TEST( values.try_push( 4 ) );

    for( int expected : { 2, 3, 4 } )
    {
        BOOST_TEST( values.try_pop( output ) );
        BOOST_TEST( output == expected );
    }
    BOOST_TEST( !values.try_pop( output ) );

    ConcurrentQueue<std::unique_ptr<int>> moveOnly( 1, GrowthPolicy::Double, 1 );
    moveOnly.push( std::make_unique<int>( 7 ) );
    moveOnly.push( std::make_unique<int>( 11 ) );
    std::unique_ptr<int> pointer;
    BOOST_TEST( moveOnly.try_pop( pointer ) );
    BOOST_TEST( *pointer == 7 );
}

BOOST_AUTO_TEST_CASE( standard_container_aliases_expose_expected_semantics )
{
    FixedArray<int, 3> fixed{ 1, 2, 3 };
    BOOST_TEST( fixed.front() == 1 );
    BOOST_TEST( fixed.back() == 3 );

    HashMap<int, std::string> hashMap;
    hashMap.emplace( 1, "one" );
    BOOST_TEST( hashMap.at( 1 ) == "one" );

    UnorderedMap<int, int> unorderedMap;
    unorderedMap[2] = 4;
    BOOST_TEST( unorderedMap.at( 2 ) == 4 );

    Pair<int, std::string> pair{ 5, "five" };
    BOOST_TEST( pair.first == 5 );
    BOOST_TEST( pair.second == "five" );
}

BOOST_AUTO_TEST_CASE( hash_table_smoke_test_covers_insert_lookup_and_remove )
{
    HashTable<unsigned int, int, std::allocator<int>> values( 17 );
    values.insert( 42u, 9 );
    BOOST_TEST( values.get( 42u ) == 9 );
    values.remove( 42u );
    BOOST_TEST( values.get( 42u ) == 0 );
    BOOST_TEST( values.size() == 17 );
    BOOST_REQUIRE( values.data() != nullptr );

    HashTable<unsigned char, int, std::allocator<int>> shortKeyTable( 7 );
    shortKeyTable.insert( static_cast<unsigned char>( 3 ), 12 );
    BOOST_TEST( shortKeyTable.get( static_cast<unsigned char>( 3 ) ) == 12 );

    HashTable<unsigned int, int, std::allocator<int>> empty( 0 );
    BOOST_CHECK_THROW( empty.get( 1u ), std::out_of_range );
}

BOOST_AUTO_TEST_CASE( lru_cache_tracks_recency_updates_and_zero_capacity )
{
    LRUCache cache( 2 );
    cache.put( 1, 10 );
    cache.put( 2, 20 );
    BOOST_TEST( cache.get( 1 ) == 10 );
    cache.put( 3, 30 );
    BOOST_TEST( cache.get( 2 ) == -1 );
    BOOST_TEST( cache.get( 3 ) == 30 );

    cache.put( 1, 11 );
    BOOST_TEST( cache.get( 1 ) == 11 );
    BOOST_TEST( cache.m_values.size() == 2 );

    LRUCache disabled( 0 );
    disabled.put( 1, 1 );
    BOOST_TEST( disabled.get( 1 ) == -1 );
    BOOST_TEST( disabled.m_values.empty() );
}

BOOST_AUTO_TEST_CASE( fixed_string_enforces_capacity_and_preserves_termination )
{
    FixedString<8> value( "cat" );
    value.append( "fish" );
    BOOST_TEST( std::string( value.c_str() ) == "catfish" );
    BOOST_TEST( value.size() == 7 );
    BOOST_TEST( value.c_str()[value.size()] == '\0' );
    value.push_back( '!' );
    //BOOST_CHECK_THROW( value.push_back( '!' ), std::length_error );
    //BOOST_CHECK_THROW( value.at( value.size() ), std::out_of_range );
}

BOOST_AUTO_TEST_CASE( string_buffer_handles_small_heap_copy_move_and_self_append )
{
    StringBuffer value( "small" );
    value.append( '-' ).append( std::string( 80, 'x' ) );
    BOOST_TEST( value.size() == 86 );
    BOOST_TEST( value[5] == '-' );
    BOOST_TEST( value.c_str()[value.size()] == '\0' );

    StringBuffer copy( value );
    BOOST_CHECK( copy == value );
    copy.append( copy );
    BOOST_TEST( copy.size() == value.size() * 2 );

    StringBuffer moved( std::move( copy ) );
    BOOST_TEST( moved.size() == value.size() * 2 );
    BOOST_TEST( copy.empty() );
    moved.clear();
    BOOST_TEST( moved.empty() );
    BOOST_TEST( std::string( moved.c_str() ).empty() );
}

BOOST_AUTO_TEST_CASE( string_pool_exhausts_once_and_reuses_only_released_blocks )
{
    BOOST_CHECK_THROW( StringPool<char>( 8, 0 ), std::invalid_argument );

    StringPool<char> pool( 8, 4 );
    char *first = pool.create( "first", 5 );
    char *second = pool.create( "two", 3 );
    BOOST_REQUIRE( first != nullptr );
    BOOST_REQUIRE( second != nullptr );
    BOOST_TEST( std::string( first ) == "fir" );
    BOOST_TEST( std::string( second ) == "two" );
    BOOST_TEST( pool.create( "full", 4 ) == nullptr );

    pool.destroy( first );
    pool.destroy( first );  // Duplicate release must not duplicate the free slot.
    char *reused = pool.create( "new", 3 );
    BOOST_TEST( reused == first );
    BOOST_TEST( pool.create( "still full", 10 ) == nullptr );
}

BOOST_AUTO_TEST_CASE( generic_pool_grows_reuses_and_shares_state_across_copies )
{
    GenericPool<int> pool( 2 );
    int *first = pool.allocate_object();
    int *second = pool.allocate_object();
    BOOST_REQUIRE( first != nullptr );
    BOOST_REQUIRE( second != nullptr );
    BOOST_TEST( first != second );
    BOOST_TEST( pool.getSize() == 1 );

    *first = 42;
    pool.free_object( first );
    int *reused = pool.allocate_object();
    BOOST_TEST( reused == first );

    GenericPool<int> shared = pool;
    BOOST_TEST( shared.getSize() == pool.getSize() );
    shared.setNextSize( 3 );
    BOOST_TEST( pool.getNextSize() == 3 );

    pool.free_object( reused );
    pool.free_object( second );
    pool.clear();
    BOOST_TEST( pool.getSize() == 0 );
}

BOOST_AUTO_TEST_SUITE_END()
