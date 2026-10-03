#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <thread>
#include <future>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace workphone;
using namespace std;

namespace
{
    struct ArrayResizeThrower
    {
        static int liveCount;
        static int copyCount;
        static int throwOnCopy;

        int value;

        explicit ArrayResizeThrower( int value = 0 ) : value( value )
        {
            ++liveCount;
        }

        ArrayResizeThrower( const ArrayResizeThrower &other ) : value( other.value )
        {
            ++copyCount;
            if( throwOnCopy != 0 && copyCount == throwOnCopy )
                throw std::runtime_error( "requested copy failure" );
            ++liveCount;
        }

        ~ArrayResizeThrower()
        {
            --liveCount;
        }
    };

    int ArrayResizeThrower::liveCount = 0;
    int ArrayResizeThrower::copyCount = 0;
    int ArrayResizeThrower::throwOnCopy = 0;

    struct ArrayEraseCounter
    {
        static size_t relocations;

        int value;

        explicit ArrayEraseCounter( int value = 0 ) : value( value )
        {
        }

        ArrayEraseCounter( const ArrayEraseCounter &other ) : value( other.value )
        {
            ++relocations;
        }

        ArrayEraseCounter( ArrayEraseCounter &&other ) noexcept : value( other.value )
        {
            ++relocations;
        }

        bool operator==( const ArrayEraseCounter &other ) const
        {
            return value == other.value;
        }
    };

    size_t ArrayEraseCounter::relocations = 0;
}  // namespace

Array<int> twoSum( Array<int> &nums, int target )
{
    map<int, int> st;
    int i, n = nums.size();
    Array<int> found;
    for( i = 0; i < n; i++ )
    {
        // int pos=;
        map<int, int>::iterator it;
        it = st.find( target - nums[i] );
        if( it != st.end() )
        {
            found.push_back( it->second );

            found.push_back( i );
            break;
        }
        else
        {
            st.insert( { nums[i], i } );
        }
    }
    return found;
}

void twoSumII( Array<int> &nums, int i, Array<Array<int>> &res )
{
    int lo = i + 1, hi = nums.size() - 1;
    while( lo < hi )
    {
        int sum = nums[i] + nums[lo] + nums[hi];
        if( sum < 0 )
        {
            ++lo;
        }
        else if( sum > 0 )
        {
            --hi;
        }
        else
        {
            res.push_back( { nums[i], nums[lo++], nums[hi--] } );
            while( lo < hi && nums[lo] == nums[lo - 1] )
                ++lo;
        }
    }
}

Array<Array<int>> threeSum( Array<int> &nums )
{
    sort( begin( nums ), end( nums ) );
    Array<Array<int>> res;
    for( int i = 0; i < nums.size() && nums[i] <= 0; ++i )
        if( i == 0 || nums[i - 1] != nums[i] )
        {
            twoSumII( nums, i, res );
        }
    return res;
}

BOOST_AUTO_TEST_CASE( array_smartpointers )
{
    /*
    Array<SmartPtr<ISharedObject>> shapePtrs;
    shapePtrs.push_back( SmartPtr<ShapeFake>( new ShapeFake ) );
    shapePtrs.push_back( SmartPtr<ShapeFake>( new ShapeFake ) );
    shapePtrs.push_back( SmartPtr<ShapeFake>( new ShapeFake ) );

    BOOST_CHECK( ( shapePtrs.size() == 3 ) );

    shapePtrs.push_back( SmartPtr<ShapeFake>( new ShapeFake ) );

    BOOST_CHECK( ( shapePtrs.size() == 4 ) );

    shapePtrs.clear();

    BOOST_CHECK( ( shapePtrs.size() == 0 ) );
    BOOST_CHECK( ( shapePtrs.empty() == true ) );

#if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#endif
    */
}

BOOST_AUTO_TEST_CASE( array_concurrent_smartpointers )
{
    /*
    ConcurrentArray<SmartPtr<ShapeFake>> shapePtrs;
    shapePtrs.push_back( SmartPtr<ShapeFake>( new ShapeFake ) );
    shapePtrs.push_back( SmartPtr<ShapeFake>( new ShapeFake ) );
    shapePtrs.push_back( SmartPtr<ShapeFake>( new ShapeFake ) );

    BOOST_CHECK( ( shapePtrs.size() == 3 ) );

    shapePtrs.push_back( SmartPtr<ShapeFake>( new ShapeFake ) );

    BOOST_CHECK( ( shapePtrs.size() == 4 ) );

    shapePtrs.clear();

    BOOST_CHECK( ( shapePtrs.size() == 0 ) );
    BOOST_CHECK( ( shapePtrs.empty() == true ) );

#if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#endif
    */
}

BOOST_AUTO_TEST_CASE( concurrent_array )
{
    ConcurrentArrayBase<s32> test;
    test.push_back( 0 );
}

// New comprehensive unit tests for ConcurrentArrayBase
BOOST_AUTO_TEST_CASE( concurrent_array_constructors )
{
    // Default constructor
    ConcurrentArrayBase<int> arr1;
    BOOST_TEST( arr1.empty() );
    BOOST_TEST( arr1.size() == 0 );

    // Initializer list constructor
    ConcurrentArrayBase<int> arr2{ 1, 2, 3, 4, 5 };
    BOOST_TEST( arr2.size() == 5 );
    BOOST_TEST( arr2[0] == 1 );
    BOOST_TEST( arr2[4] == 5 );

    // Copy constructor
    ConcurrentArrayBase<int> arr3( arr2 );
    BOOST_TEST( arr3.size() == 5 );
    BOOST_TEST( arr3[0] == 1 );
    BOOST_TEST( arr3[4] == 5 );

    // Move constructor
    ConcurrentArrayBase<int> arr4( std::move( arr3 ) );
    BOOST_TEST( arr4.size() == 5 );
    BOOST_TEST( arr4[0] == 1 );
    BOOST_TEST( arr4[4] == 5 );

    // Iterator constructor
    Array<int> vec{ 10, 20, 30 };
    ConcurrentArrayBase<int> arr5( vec.begin(), vec.end() );
    BOOST_TEST( arr5.size() == 3 );
    BOOST_TEST( arr5[0] == 10 );
    BOOST_TEST( arr5[2] == 30 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_assignment_operators )
{
    ConcurrentArrayBase<int> arr1{ 1, 2, 3 };
    ConcurrentArrayBase<int> arr2;

    // Copy assignment
    arr2 = arr1;
    BOOST_TEST( arr2.size() == 3 );
    BOOST_TEST( arr2[0] == 1 );
    BOOST_TEST( arr2[2] == 3 );

    // Move assignment
    ConcurrentArrayBase<int> arr3;
    arr3 = std::move( arr1 );
    BOOST_TEST( arr3.size() == 3 );
    BOOST_TEST( arr3[0] == 1 );
    BOOST_TEST( arr3[2] == 3 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_element_access )
{
    ConcurrentArrayBase<int> arr{ 10, 20, 30, 40, 50 };

    // Test operator[] for read access
    BOOST_TEST( arr[0] == 10 );
    BOOST_TEST( arr[4] == 50 );

    // Test at() method for bounds checking
    BOOST_TEST( arr.at( 0 ) == 10 );
    BOOST_TEST( arr.at( 4 ) == 50 );

    // Test out of bounds exception
    bool exception_thrown = false;
    try
    {
        int x = arr.at( 10 );
    }
    catch( const std::out_of_range &e )
    {
        exception_thrown = true;
    }
    BOOST_TEST( exception_thrown );

    // Test front() and back()
    BOOST_TEST( arr.front() == 10 );
    BOOST_TEST( arr.back() == 50 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_modifiers )
{
    ConcurrentArrayBase<int> arr;

    // Test push_back
    arr.push_back( 1 );
    arr.push_back( 2 );
    arr.push_back( 3 );
    BOOST_TEST( arr.size() == 3 );
    BOOST_TEST( arr[0] == 1 );
    BOOST_TEST( arr[2] == 3 );

    // Test emplace_back
    arr.emplace_back( 4 );
    BOOST_TEST( arr.size() == 4 );
    BOOST_TEST( arr[3] == 4 );

    // Test insert at specific index
    arr.insert( 1, 99 );
    BOOST_TEST( arr.size() == 5 );
    BOOST_TEST( arr[1] == 99 );
    BOOST_TEST( arr[2] == 2 );

    // Test emplace at specific index
    arr.emplace( 0, 88 );
    BOOST_TEST( arr.size() == 6 );
    BOOST_TEST( arr[0] == 88 );
    BOOST_TEST( arr[1] == 1 );

    // Test pop_back
    arr.pop_back();
    BOOST_TEST( arr.size() == 5 );

    // Test erase by index
    arr.erase( 1 );
    BOOST_TEST( arr.size() == 4 );
    BOOST_TEST( arr[1] == 99 );

    // Test clear
    arr.clear();
    BOOST_TEST( arr.empty() );
    BOOST_TEST( arr.size() == 0 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_capacity )
{
    ConcurrentArrayBase<int> arr;

    // Test initial capacity
    BOOST_TEST( arr.capacity() == 0 );
    BOOST_TEST( arr.empty() );

    // Test reserve
    arr.reserve( 100 );
    BOOST_TEST( arr.capacity() >= 100 );
    BOOST_TEST( arr.empty() );

    // Test resize
    arr.resize( 50 );
    BOOST_TEST( arr.size() == 50 );
    BOOST_TEST( !arr.empty() );

    // Test resize with default value
    arr.resize( 75, 42 );
    BOOST_TEST( arr.size() == 75 );
    for( int i = 50; i < 75; ++i )
    {
        BOOST_TEST( arr[i] == 42 );
    }

    // Test shrinking resize
    arr.resize( 25 );
    BOOST_TEST( arr.size() == 25 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_algorithms )
{
    ConcurrentArrayBase<int> arr{ 5, 2, 8, 1, 9, 3 };

    // Test find_if
    auto index = arr.find_if( []( int x ) { return x > 7; } );
    BOOST_TEST( index == 2 );  // First element > 7 is at index 2 (value 8)

    auto not_found = arr.find_if( []( int x ) { return x > 100; } );
    BOOST_TEST( not_found == static_cast<size_t>( -1 ) );

    // Test sort
    arr.sort();
    BOOST_TEST( arr[0] == 1 );
    BOOST_TEST( arr[1] == 2 );
    BOOST_TEST( arr[2] == 3 );
    BOOST_TEST( arr[3] == 5 );
    BOOST_TEST( arr[4] == 8 );
    BOOST_TEST( arr[5] == 9 );

    // Test sort with custom comparator
    arr.sort( std::greater<int>() );
    BOOST_TEST( arr[0] == 9 );
    BOOST_TEST( arr[1] == 8 );
    BOOST_TEST( arr[2] == 5 );
    BOOST_TEST( arr[3] == 3 );
    BOOST_TEST( arr[4] == 2 );
    BOOST_TEST( arr[5] == 1 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_snapshot )
{
    ConcurrentArrayBase<int> arr{ 1, 2, 3, 4, 5 };

    // Test snapshot functionality
    auto snapshot = arr.snapshot();
    BOOST_TEST( snapshot.size() == 5 );
    BOOST_TEST( snapshot[0] == 1 );
    BOOST_TEST( snapshot[4] == 5 );

    // Modify original array
    arr.push_back( 6 );
    arr[0] = 100;

    // Snapshot should remain unchanged
    BOOST_TEST( snapshot.size() == 5 );
    BOOST_TEST( snapshot[0] == 1 );

    // New snapshot should reflect changes
    auto new_snapshot = arr.snapshot();
    BOOST_TEST( new_snapshot.size() == 6 );
    BOOST_TEST( new_snapshot[0] == 100 );
    BOOST_TEST( new_snapshot[5] == 6 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_thread_safety_basic )
{
    ConcurrentArray<int> arr;
    const int num_threads = 4;
    const int items_per_thread = 100;

    // Test concurrent push_back operations
    Array<std::thread> threads;

    arr.reserve( num_threads * items_per_thread );
    threads.reserve( num_threads );

    for( int t = 0; t < num_threads; ++t )
    {
        threads.emplace_back( [&arr, t, items_per_thread]() {
            for( int i = 0; i < items_per_thread; ++i )
            {
                arr.push_back( t * items_per_thread + i );
            }
        } );
    }

    for( auto &thread : threads )
    {
        if( thread.joinable() )
        {
            thread.join();
        }
    }

    // Verify all elements were added
    BOOST_TEST( arr.size() == num_threads * items_per_thread );

    // Verify no data corruption by checking all values are in expected range
    auto snapshot = arr.snapshot();
    for( const auto &value : snapshot )
    {
        BOOST_TEST( value >= 0 );
        BOOST_TEST( value < num_threads * items_per_thread );
    }
}

BOOST_AUTO_TEST_CASE( concurrent_array_thread_safety_mixed_operations )
{
    ConcurrentArrayBase<int> arr;
    std::atomic<bool> should_stop{ false };

    // Pre-populate with some data
    for( int i = 0; i < 100; ++i )
    {
        arr.push_back( i );
    }

    // Writer thread
    std::thread writer( [&arr, &should_stop]() {
        int counter = 1000;
        while( !should_stop.load() )
        {
            arr.push_back( counter++ );
            std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );
        }
    } );

    // Reader threads
    Array<std::thread> readers;
    std::atomic<int> read_count{ 0 };

    for( int i = 0; i < 3; ++i )
    {
        readers.emplace_back( [&arr, &should_stop, &read_count]() {
            while( !should_stop.load() )
            {
                auto size = arr.size();
                if( size > 0 )
                {
                    auto value = arr[size - 1];  // Read last element
                    read_count.fetch_add( 1 );
                }
                std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );
            }
        } );
    }

    // Let threads run for a short time
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    should_stop.store( true );

    writer.join();
    for( auto &reader : readers )
    {
        reader.join();
    }

    // Verify array integrity
    BOOST_TEST( arr.size() >= 100 );
    BOOST_TEST( read_count.load() > 0 );

    // Check that the array is still in a valid state
    auto snapshot = arr.snapshot();
    BOOST_TEST( snapshot.size() == arr.size() );
}

BOOST_AUTO_TEST_CASE( concurrent_array_iterator_operations )
{
    ConcurrentArrayBase<int> arr{ 1, 2, 3, 4, 5 };

    // Test begin/end iterators (note: these are not thread-safe)
    int sum = 0;
    for( auto it = arr.begin(); it != arr.end(); ++it )
    {
        sum += *it;
    }
    BOOST_TEST( sum == 15 );

    // Test const iterators
    const auto &const_arr = arr;
    sum = 0;
    for( auto it = const_arr.cbegin(); it != const_arr.cend(); ++it )
    {
        sum += *it;
    }
    BOOST_TEST( sum == 15 );

    // Test range-based for loop using snapshot for thread safety
    auto snapshot = arr.snapshot();
    sum = 0;
    for( const auto &value : snapshot )
    {
        sum += value;
    }
    BOOST_TEST( sum == 15 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_stress_test )
{
    /*
    ConcurrentArrayBase<int> arr;
    const auto num_operations = 1000;
    const auto num_threads = 8;

    ConcurrentArrayBase<std::future<void>> futures;

    RecursiveMutex mutex;

    // Create multiple threads performing different operations
    for( int t = 0; t < num_threads; ++t )
    {
        futures.push_back( std::async( std::launch::async, [&arr, t, num_operations, &mutex]() {
            for( int i = 0; i < num_operations; ++i )
            {
                auto operation = ( t + i ) % 4;

                switch( operation )
                {
                case 0:  // push_back
                {
                    RecursiveMutex::ScopedLock lock( mutex );
                    arr.push_back( t * num_operations + i );
                }
                break;
                case 1:  // size check
                {
                    RecursiveMutex::ScopedLock lock( mutex );
                    auto size = arr.size();
                    (void)size;  // Suppress unused variable warning
                }
                break;
                case 2:  // read access (if not empty)
                {
                    RecursiveMutex::ScopedLock lock( mutex );
                    if( !arr.empty() )
                    {
                        auto value = arr[0];
                        (void)value;  // Suppress unused variable warning
                    }
                }
                break;
                case 3:  // snapshot
                {
                    RecursiveMutex::ScopedLock lock( mutex );
                    auto snapshot = arr.snapshot();
                    (void)snapshot;  // Suppress unused variable warning
                }
                break;
                }
            }
        } ) );
    }

    // Wait for all threads to complete
    for( auto &future : futures )
    {
        future.wait();
    }

    // Verify array is in a consistent state
    auto final_size = arr.size();
    auto snapshot = arr.snapshot();
    BOOST_TEST( snapshot.size() == final_size );
    BOOST_TEST( final_size > 0 );
    */
}

BOOST_AUTO_TEST_CASE( concurrent_array_pop_back_edge_cases )
{
    ConcurrentArrayBase<int> arr;

    // Test pop_back on empty array (should not crash)
    arr.pop_back();
    BOOST_TEST( arr.empty() );

    // Add elements and test pop_back
    arr.push_back( 1 );
    arr.push_back( 2 );
    arr.push_back( 3 );
    BOOST_TEST( arr.size() == 3 );

    arr.pop_back();
    BOOST_TEST( arr.size() == 2 );
    BOOST_TEST( arr.back() == 2 );

    arr.pop_back();
    arr.pop_back();
    BOOST_TEST( arr.empty() );

    // Additional pop_back on empty array
    arr.pop_back();
    BOOST_TEST( arr.empty() );
}

BOOST_AUTO_TEST_CASE( concurrent_array_erase_edge_cases )
{
    ConcurrentArrayBase<int> arr{ 10, 20, 30, 40, 50 };

    // Test erase at valid indices
    arr.erase( (size_t)2 );  // Remove 30
    BOOST_TEST( arr.size() == 4 );
    BOOST_TEST( arr[2] == 40 );

    // Test erase at first index
    arr.erase( size_t( 0 ) );  // Remove 10
    BOOST_TEST( arr.size() == 3 );
    BOOST_TEST( arr[0] == 20 );

    // Test erase at last index
    arr.erase( size_t( arr.size() - 1 ) );  // Remove last element
    BOOST_TEST( arr.size() == 2 );
    BOOST_TEST( arr[1] == 40 );

    // Test erase with out-of-bounds index (should not crash)
    size_t original_size = arr.size();
    arr.erase( size_t( 100 ) );
    BOOST_TEST( arr.size() == original_size );
}

BOOST_AUTO_TEST_CASE( concurrent_array_insert_edge_cases )
{
    ConcurrentArrayBase<int> arr{ 10, 20, 30 };

    // Test insert at beginning
    arr.insert( 0, 5 );
    BOOST_TEST( arr.size() == 4 );
    BOOST_TEST( arr[0] == 5 );
    BOOST_TEST( arr[1] == 10 );

    // Test insert at end
    arr.insert( arr.size(), 40 );
    BOOST_TEST( arr.size() == 5 );
    BOOST_TEST( arr[4] == 40 );

    // Test insert in middle
    arr.insert( 2, 15 );
    BOOST_TEST( arr.size() == 6 );
    BOOST_TEST( arr[2] == 15 );
    BOOST_TEST( arr[3] == 20 );

    // Test insert with out-of-bounds index (should not insert)
    size_t original_size = arr.size();
    arr.insert( 100, 999 );
    BOOST_TEST( arr.size() == original_size );
}

BOOST_AUTO_TEST_CASE( test_edge_cases )
{
    // Test empty array
    ConcurrentArrayBase<s32> arr;
    BOOST_TEST( arr.empty() );
    BOOST_TEST( arr.size() == 0 );

    // Test reserve and capacity
    arr.reserve( 100 );
    BOOST_TEST( arr.capacity() == 100 );
    arr.reserve( 50 );
    BOOST_TEST( arr.capacity() == 100 );

    // Test resize
    arr.resize( 50 );
    BOOST_TEST( arr.size() == 50 );
    arr.resize( 100, 1 );
    BOOST_TEST( arr.size() == 100 );
    for( int i = 50; i < 100; ++i )
    {
        BOOST_TEST( arr[i] == 1 );
    }
    arr.resize( 50 );
    BOOST_TEST( arr.size() == 50 );

    // Test out-of-range access
    bool exception_thrown = false;
    try
    {
        int x = arr.at( 100 );
    }
    catch( const std::out_of_range &e )
    {
        exception_thrown = true;
    }
    BOOST_TEST( exception_thrown );

    // Test front and back
    arr.push_back( 2 );
    BOOST_TEST( arr.front() == 0 );
    BOOST_TEST( arr.back() == 2 );

    // Test iterators
    int i = 0;
    for( auto it = arr.begin(); it != arr.end(); ++it )
    {
        BOOST_TEST( *it == arr[i] );
        ++i;
    }

    // Test const iterators
    const ConcurrentArrayBase<int> &arr_const = arr;
    i = 0;
    for( auto it = arr_const.cbegin(); it != arr_const.cend(); ++it )
    {
        BOOST_TEST( *it == arr[i] );
        ++i;
    }
}

vector<int> twoSum2( vector<int> &nums, int target )
{
    auto indicies = vector<int>();

    auto sum = 0;

    for( size_t i = 0; i < nums.size(); ++i )
    {
        auto n = nums[i];

        if( n < target )
        {
            sum += n;

            indicies.push_back( i );

            if( indicies.size() > 2 )
            {
                indicies.erase( indicies.begin(), indicies.begin() + 1 );
            }
        }
    }

    return indicies;
}

BOOST_AUTO_TEST_CASE( array_2sum )
{
    Array<int> nums( { 3, 2, 3 } );
    nums.resize( 3, 0 );

    auto indices = twoSum( nums, 6 );
    BOOST_CHECK( indices[0] == 0 && indices[1] == 2 );
}

BOOST_AUTO_TEST_CASE( array_2sum_fbarray )
{
    Array<int> nums( { 3, 2, 3 } );
    auto indices = twoSum( nums, 6 );
    BOOST_CHECK( indices[0] == 0 && indices[1] == 2 );
}

BOOST_AUTO_TEST_CASE( array_3sum )
{
    // Test case 1: Standard case with triplets that sum to zero
    {
        Array<int> nums = { -1, 0, 1, 2, -1, -4 };
        auto result = threeSum( nums );

        // Expected result should contain [[-1,-1,2],[-1,0,1]]
        BOOST_CHECK( result.size() == 2 );

        // Sort the result for consistent checking
        //std::sort( result.begin(), result.end() );

        BOOST_CHECK( result[0].size() == 3 );
        BOOST_CHECK( result[1].size() == 3 );

        // Check first triplet: [-1, -1, 2]
        BOOST_CHECK( result[0][0] == -1 );
        BOOST_CHECK( result[0][1] == -1 );
        BOOST_CHECK( result[0][2] == 2 );

        // Check second triplet: [-1, 0, 1]
        BOOST_CHECK( result[1][0] == -1 );
        BOOST_CHECK( result[1][1] == 0 );
        BOOST_CHECK( result[1][2] == 1 );

        // Verify the sums are zero
        BOOST_CHECK( result[0][0] + result[0][1] + result[0][2] == 0 );
        BOOST_CHECK( result[1][0] + result[1][1] + result[1][2] == 0 );
    }

    // Test case 2: No triplets sum to zero
    {
        Array<int> nums = { 0, 1, 1 };
        auto result = threeSum( nums );
        BOOST_CHECK( result.empty() );
    }

    // Test case 3: All zeros
    {
        Array<int> nums = { 0, 0, 0 };
        auto result = threeSum( nums );
        BOOST_CHECK( result.size() == 1 );
        BOOST_CHECK( result[0][0] == 0 );
        BOOST_CHECK( result[0][1] == 0 );
        BOOST_CHECK( result[0][2] == 0 );
    }

    // Test case 4: Array with less than 3 elements
    {
        Array<int> nums = { 1, 2 };
        auto result = threeSum( nums );
        BOOST_CHECK( result.empty() );
    }

    // Test case 5: Empty array
    {
        Array<int> nums = {};
        auto result = threeSum( nums );
        BOOST_CHECK( result.empty() );
    }

    // Test case 6: Single element array
    {
        Array<int> nums = { 1 };
        auto result = threeSum( nums );
        BOOST_CHECK( result.empty() );
    }

    // Test case 7: Array with duplicates
    {
        Array<int> nums = { -2, 0, 1, 1, 2 };
        auto result = threeSum( nums );

        // Should find [-2, 0, 2] and [-2, 1, 1]
        BOOST_CHECK( result.size() == 2 );

        // Verify each triplet sums to zero
        for( const auto &triplet : result )
        {
            BOOST_CHECK( triplet.size() == 3 );
            BOOST_CHECK( triplet[0] + triplet[1] + triplet[2] == 0 );
        }
    }

    // Test case 8: All positive numbers
    {
        Array<int> nums = { 1, 2, 3, 4, 5 };
        auto result = threeSum( nums );
        BOOST_CHECK( result.empty() );
    }

    // Test case 9: All negative numbers
    {
        Array<int> nums = { -5, -4, -3, -2, -1 };
        auto result = threeSum( nums );
        BOOST_CHECK( result.empty() );
    }

    // Test case 10: Large array with multiple solutions
    {
        Array<int> nums = { -4, -2, -2, -2, 0, 1, 2, 2, 2, 3, 3, 4, 4, 6, 6 };
        auto result = threeSum( nums );

        // Verify all results sum to zero and are unique
        Set<Array<int>> unique_triplets;
        for( const auto &triplet : result )
        {
            BOOST_CHECK( triplet.size() == 3 );
            BOOST_CHECK( triplet[0] + triplet[1] + triplet[2] == 0 );

            // Ensure triplets are sorted (which they should be due to the algorithm)
            BOOST_CHECK( triplet[0] <= triplet[1] );
            BOOST_CHECK( triplet[1] <= triplet[2] );

            unique_triplets.insert( triplet );
        }

        // Verify no duplicate triplets
        BOOST_CHECK( unique_triplets.size() == result.size() );
    }
}

BOOST_AUTO_TEST_CASE( DefaultConstructorTest )
{
    Array<int> array;
    BOOST_TEST( array.empty() );
    BOOST_TEST( array.size() == 0 );
    BOOST_TEST( array.capacity() == 0 );
}

BOOST_AUTO_TEST_CASE( SizeConstructorTest )
{
    Array<int> array( 5 );
    BOOST_TEST( !array.empty() );
    BOOST_TEST( array.size() == 5 );
    BOOST_TEST( array.capacity() >= 5 );
}

BOOST_AUTO_TEST_CASE( SizeValueConstructorTest )
{
    Array<int> array( 3, 7 );
    BOOST_TEST( !array.empty() );
    BOOST_TEST( array.size() == 3 );
    BOOST_TEST( array.capacity() >= 3 );
    //BOOST_TEST( array[0] == 7 );
    //BOOST_TEST( array[1] == 7 );
    //BOOST_TEST( array[2] == 7 );
}

BOOST_AUTO_TEST_CASE( CopyConstructorTest )
{
    Array<int> array1( 3, 5 );
    Array<int> array2( array1 );
    BOOST_TEST( array1 == array2 );
}

BOOST_AUTO_TEST_CASE( AssignmentOperatorTest )
{
    Array<int> array1( 3, 5 );
    Array<int> array2 = array1;
    BOOST_TEST( array1 == array2 );
}

BOOST_AUTO_TEST_CASE( IteratorTest )
{
    Array<int> array = { 1, 2, 3 };
    int sum = 0;
    for( auto it = array.begin(); it != array.end(); ++it )
    {
        sum += *it;
    }
    BOOST_TEST( sum == 6 );
}

BOOST_AUTO_TEST_CASE( ConstIteratorTest )
{
    const Array<int> array = { 1, 2, 3 };
    int sum = 0;
    for( auto it = array.cbegin(); it != array.cend(); ++it )
    {
        sum += *it;
    }
    BOOST_TEST( sum == 6 );
}

BOOST_AUTO_TEST_CASE( ReverseIteratorTest )
{
    Array<int> array = { 1, 2, 3 };
    int sum = 0;
    for( auto it = array.rbegin(); it != array.rend(); ++it )
    {
        sum += *it;
    }
    BOOST_TEST( sum == 6 );
}

BOOST_AUTO_TEST_CASE( ConstReverseIteratorTest )
{
    const Array<int> array = { 1, 2, 3 };
    int sum = 0;
    for( auto it = array.crbegin(); it != array.crend(); ++it )
    {
        sum += *it;
    }

    BOOST_TEST( sum == 6 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_reverse_iterator_operations )
{
    ConcurrentArrayBase<int> arr{ 1, 2, 3, 4, 5 };

    // Test reverse iterators
    int sum = 0;
    for( auto it = arr.rbegin(); it != arr.rend(); ++it )
    {
        sum += *it;
    }
    BOOST_TEST( sum == 15 );

    // Test const reverse iterators
    const auto &const_arr = arr;
    sum = 0;
    for( auto it = const_arr.crbegin(); it != const_arr.crend(); ++it )
    {
        sum += *it;
    }
    BOOST_TEST( sum == 15 );

    // Test range-based for loop using snapshot for thread safety
    auto snapshot = arr.snapshot();
    sum = 0;
    for( const auto &value : snapshot )
    {
        sum += value;
    }
    BOOST_TEST( sum == 15 );
}

BOOST_AUTO_TEST_CASE( array_range_construction_copies_lvalues_and_rejects_reversed_ranges )
{
    std::vector<std::string> source{ "one", "two", "three" };
    Array<std::string> copy( source.begin(), source.end() );

    BOOST_TEST( copy.size() == 3 );
    BOOST_TEST( copy[1] == "two" );
    BOOST_TEST( source[0] == "one" );
    BOOST_TEST( source[1] == "two" );

    Array<int> ordered{ 1, 2, 3 };
    BOOST_CHECK_THROW( Array<int>( ordered.end(), ordered.begin() ), std::invalid_argument );
    BOOST_CHECK_THROW( ordered.insert( ordered.begin(), ordered.end(), ordered.begin() ),
                       std::invalid_argument );
    BOOST_CHECK_THROW( ordered.erase( ordered.end(), ordered.begin() ), std::out_of_range );
}

BOOST_AUTO_TEST_CASE( array_self_referential_modifiers_preserve_values )
{
    Array<std::string> assigned{ "zero", "one", "two" };
    assigned.assign( assigned.begin() + 1, assigned.end() );
    BOOST_TEST( assigned.size() == 2 );
    BOOST_TEST( assigned[0] == "one" );
    BOOST_TEST( assigned[1] == "two" );

    assigned.assign( 3, assigned.front() );
    BOOST_TEST( assigned.size() == 3 );
    BOOST_TEST( assigned[0] == "one" );
    BOOST_TEST( assigned[2] == "one" );

    Array<std::string> inserted{ "first", "second" };
    inserted.insert( inserted.end(), inserted.front() );
    BOOST_TEST( inserted.size() == 3 );
    BOOST_TEST( inserted.back() == "first" );

    Array<std::string> emplaced{ "first", "second" };
    emplaced.emplace( emplaced.end(), emplaced.front() );
    BOOST_TEST( emplaced.size() == 3 );
    BOOST_TEST( emplaced.back() == "first" );

    Array<std::string> resized{ "repeat" };
    resized.resize( 3, resized.front() );
    BOOST_TEST( resized.size() == 3 );
    BOOST_TEST( resized[0] == "repeat" );
    BOOST_TEST( resized[1] == "repeat" );
    BOOST_TEST( resized[2] == "repeat" );

    Array<std::string> moved{ "move me" };
    moved.push_back( std::move( moved.front() ) );
    BOOST_TEST( moved.size() == 2 );
    BOOST_TEST( moved.back() == "move me" );
}

BOOST_AUTO_TEST_CASE( array_value_erase_handles_aliases_and_duplicates_in_one_pass )
{
    Array<std::string> values{ "remove", "keep", "remove", "also keep", "remove" };
    const std::string &valueFromArray = values.front();

    values.erase( valueFromArray );

    BOOST_TEST( values.size() == 2 );
    BOOST_TEST( values[0] == "keep" );
    BOOST_TEST( values[1] == "also keep" );

    Array<ArrayEraseCounter> manyValues;
    manyValues.reserve( 200 );
    for( int i = 0; i < 200; ++i )
        manyValues.emplace_back( i % 2 );

    ArrayEraseCounter needle( 1 );
    ArrayEraseCounter::relocations = 0;
    manyValues.erase( needle );
    BOOST_TEST( manyValues.size() == 100 );
    BOOST_TEST( ArrayEraseCounter::relocations == 100 );
}

BOOST_AUTO_TEST_CASE( array_move_iterator_range_supports_move_only_values )
{
    std::vector<std::unique_ptr<int>> source;
    source.emplace_back( new int( 7 ) );
    source.emplace_back( new int( 11 ) );

    Array<std::unique_ptr<int>> values;
    values.insert( values.end(), std::make_move_iterator( source.begin() ),
                   std::make_move_iterator( source.end() ) );

    BOOST_TEST( values.size() == 2 );
    BOOST_TEST( *values[0] == 7 );
    BOOST_TEST( *values[1] == 11 );
    BOOST_TEST( source[0] == nullptr );
    BOOST_TEST( source[1] == nullptr );
}

BOOST_AUTO_TEST_CASE( array_resize_cleans_up_partially_constructed_elements )
{
    BOOST_TEST( ArrayResizeThrower::liveCount == 0 );
    {
        Array<ArrayResizeThrower> values;
        values.reserve( 4 );
        values.emplace_back( 1 );
        ArrayResizeThrower fill( 9 );

        ArrayResizeThrower::copyCount = 0;
        ArrayResizeThrower::throwOnCopy = 3;
        BOOST_CHECK_THROW( values.resize( 4, fill ), std::runtime_error );
        ArrayResizeThrower::throwOnCopy = 0;

        BOOST_TEST( values.size() == 1 );
        BOOST_TEST( values[0].value == 1 );
        BOOST_TEST( ArrayResizeThrower::liveCount == 2 );
    }
    BOOST_TEST( ArrayResizeThrower::liveCount == 0 );
}

BOOST_AUTO_TEST_CASE( array_range_constructor_cleans_up_after_copy_failure )
{
    BOOST_TEST( ArrayResizeThrower::liveCount == 0 );
    {
        std::vector<ArrayResizeThrower> source;
        source.reserve( 3 );
        source.emplace_back( 1 );
        source.emplace_back( 2 );
        source.emplace_back( 3 );

        ArrayResizeThrower::copyCount = 0;
        ArrayResizeThrower::throwOnCopy = 2;
        BOOST_CHECK_THROW( Array<ArrayResizeThrower>( source.begin(), source.end() ),
                           std::runtime_error );
        ArrayResizeThrower::throwOnCopy = 0;

        BOOST_TEST( ArrayResizeThrower::liveCount == 3 );
    }
    BOOST_TEST( ArrayResizeThrower::liveCount == 0 );
}

BOOST_AUTO_TEST_CASE( array_growth_policy_controls_automatic_capacity )
{
    Array<int> fixed( GrowthPolicy::Fixed, 2 );
    BOOST_CHECK_THROW( fixed.push_back( 1 ), std::length_error );
    fixed.reserve( 2 );
    fixed.push_back( 1 );
    fixed.push_back( 2 );
    BOOST_CHECK_THROW( fixed.push_back( 3 ), std::length_error );
    BOOST_TEST( fixed.size() == 2 );

    Array<int> grow( GrowthPolicy::Grow, 3 );
    grow.push_back( 1 );
    BOOST_TEST( grow.capacity() == 3 );
    grow.resize( 4 );
    BOOST_TEST( grow.capacity() == 6 );
    grow.reserve( 10 );
    BOOST_TEST( grow.capacity() == 10 );

    Array<int> doubled( GrowthPolicy::Double, 4 );
    doubled.push_back( 1 );
    BOOST_TEST( doubled.capacity() == 4 );
    doubled.resize( 5 );
    BOOST_TEST( doubled.capacity() == 8 );

    BOOST_CHECK_THROW( doubled.setGrowthSize( 0 ), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE( concurrent_array_growth_policy_and_move_only_support )
{
    ConcurrentArray<int> values( GrowthPolicy::Grow, 3 );
    values.emplace_back( 1 );
    BOOST_TEST( values.capacity() == 3 );
    BOOST_CHECK( values.getGrowthPolicy() == GrowthPolicy::Grow );
    BOOST_TEST( values.getGrowthSize() == 3 );

    values.setGrowthPolicy( GrowthPolicy::Fixed );
    values.reserve( 4 );
    values.push_back( 2 );
    values.push_back( 3 );
    values.push_back( 4 );
    BOOST_CHECK_THROW( values.push_back( 5 ), std::length_error );

    ConcurrentArray<std::unique_ptr<int>> moveOnly;
    moveOnly.emplace_back( new int( 7 ) );
    moveOnly.push_back( std::unique_ptr<int>( new int( 11 ) ) );
    BOOST_TEST( *moveOnly[0] == 7 );
    BOOST_TEST( *moveOnly[1] == 11 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_iterators_do_not_block_a_writer )
{
    ConcurrentArray<int> values{ 1, 2, 3 };
    std::future<void> writer;
    {
        auto iteratorKeptInScope = values.begin();
        writer = std::async( std::launch::async, [&values]() { values.push_back( 4 ); } );
        BOOST_CHECK( writer.wait_for( std::chrono::seconds( 2 ) ) == std::future_status::ready );

        // The successful push_back may invalidate this iterator, so do not dereference it.
        (void)iteratorKeptInScope;
    }

    // If the regression recurs, destroying the iterator above releases the old hidden lock and
    // prevents this test from hanging indefinitely after reporting the failed timeout check.
    writer.get();
    BOOST_TEST( values.size() == 4 );
}

BOOST_AUTO_TEST_CASE( concurrent_array_value_erase_handles_aliased_duplicates )
{
    ConcurrentArray<std::string> values{ "remove", "keep", "remove", "also keep", "remove" };
    const std::string &aliasedValue = values.front();

    values.erase( aliasedValue );

    auto remaining = values.snapshot();
    BOOST_TEST( remaining.size() == 2 );
    BOOST_TEST( remaining[0] == "keep" );
    BOOST_TEST( remaining[1] == "also keep" );
}
