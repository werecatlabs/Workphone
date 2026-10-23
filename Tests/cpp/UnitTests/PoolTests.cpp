#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Pool.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <thread>
#include <vector>
#include <set>
#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <mutex>

using namespace workphone;

// Simple test class for pool allocation
struct TestObject
{
    int value;
    float data;

    TestObject() : value( 0 ), data( 0.0f )
    {
    }

    TestObject( int v, float d ) : value( v ), data( d )
    {
    }
};

// Larger test object for memory alignment testing
struct alignas( 64 ) AlignedTestObject
{
    char padding[64];
    int value;
};

namespace
{
    class PoolBarrier
    {
    public:
        explicit PoolBarrier( unsigned participants ) : m_participants( participants ) {}

        bool wait()
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            if( m_broken ) return false;
            const auto phase = m_phase;
            if( ++m_arrived == m_participants )
            {
                m_arrived = 0;
                ++m_phase;
                m_ready.notify_all();
                return true;
            }
            if( !m_ready.wait_for( lock, std::chrono::seconds( 5 ),
                                  [&] { return m_broken || phase != m_phase; } ) )
            {
                m_broken = true;
                m_ready.notify_all();
            }
            return !m_broken;
        }

        void cancel()
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_broken = true;
            m_ready.notify_all();
        }

    private:
        const unsigned m_participants;
        unsigned m_arrived = 0, m_phase = 0;
        bool m_broken = false;
        std::mutex m_mutex;
        std::condition_variable m_ready;
    };
}

//------------------------------------------------------------------------------
// Basic Construction Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pool_default_constructor )
{
    Pool<TestObject> pool;

    BOOST_CHECK_EQUAL( pool.getNextSize(), 32 );
    BOOST_CHECK_EQUAL( pool.getSize(), 0 );
}

BOOST_AUTO_TEST_CASE( pool_custom_size_constructor )
{
    Pool<TestObject> pool( 64 );

    BOOST_CHECK_EQUAL( pool.getNextSize(), 64 );
    BOOST_CHECK_EQUAL( pool.getSize(), 0 );
}

BOOST_AUTO_TEST_CASE( pool_small_size_constructor )
{
    Pool<TestObject> pool( 1 );

    BOOST_CHECK_EQUAL( pool.getNextSize(), 1 );
}

//------------------------------------------------------------------------------
// Allocation Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pool_single_allocation )
{
    Pool<TestObject> pool( 8 );

    auto ptr = pool.allocate_object();

    BOOST_REQUIRE( ptr != nullptr );
    BOOST_CHECK_EQUAL( pool.getSize(), 1 );
}

BOOST_AUTO_TEST_CASE( pool_multiple_allocations_within_block )
{
    Pool<TestObject> pool( 8 );

    std::vector<RawPtr<TestObject>> ptrs;
    for( int i = 0; i < 8; ++i )
    {
        auto ptr = pool.allocate_object();
        BOOST_REQUIRE( ptr != nullptr );
        ptrs.push_back( ptr );
    }

    // All allocations should come from a single block
    BOOST_CHECK_EQUAL( pool.getSize(), 1 );
}

BOOST_AUTO_TEST_CASE( pool_allocation_triggers_new_block )
{
    Pool<TestObject> pool( 4 );

    // Allocate more than the initial block size
    for( int i = 0; i < 5; ++i )
    {
        auto ptr = pool.allocate_object();
        BOOST_REQUIRE( ptr != nullptr );
    }

    // Should have allocated a second block
    BOOST_CHECK_EQUAL( pool.getSize(), 2 );
}

BOOST_AUTO_TEST_CASE( pool_allocated_pointers_are_unique )
{
    Pool<TestObject> pool( 16 );

    std::set<TestObject *> ptrSet;
    for( int i = 0; i < 16; ++i )
    {
        auto ptr = pool.allocate_object();
        BOOST_REQUIRE( ptr != nullptr );
        BOOST_CHECK( ptrSet.find( ptr.get() ) == ptrSet.end() );
        ptrSet.insert( ptr.get() );
    }

    BOOST_CHECK_EQUAL( ptrSet.size(), 16 );
}

BOOST_AUTO_TEST_CASE( pool_object_can_be_modified )
{
    Pool<TestObject> pool;

    auto ptr = pool.allocate_object();
    BOOST_REQUIRE( ptr != nullptr );

    ptr->value = 42;
    ptr->data = 3.14f;

    BOOST_CHECK_EQUAL( ptr->value, 42 );
    BOOST_CHECK_CLOSE( ptr->data, 3.14f, 0.001f );
}

//------------------------------------------------------------------------------
// Free/Recycle Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pool_free_and_reuse )
{
    Pool<TestObject> pool( 4 );

    // Allocate all slots
    std::vector<RawPtr<TestObject>> ptrs;
    for( int i = 0; i < 4; ++i )
    {
        ptrs.push_back( pool.allocate_object() );
    }

    // Free one
    TestObject *freedPtr = ptrs[0].get();
    pool.free_object( ptrs[0].get() );
    ptrs.erase( ptrs.begin() );

    // Allocate again - should get the freed pointer back
    auto newPtr = pool.allocate_object();

    // Still only one block
    BOOST_CHECK_EQUAL( pool.getSize(), 1 );
    BOOST_CHECK_EQUAL( newPtr.get(), freedPtr );
}

BOOST_AUTO_TEST_CASE( pool_free_all_and_reallocate )
{
    Pool<TestObject> pool( 8 );

    std::vector<RawPtr<TestObject>> ptrs;
    for( int i = 0; i < 8; ++i )
    {
        ptrs.push_back( pool.allocate_object() );
    }

    // Free all
    for( auto &ptr : ptrs )
    {
        pool.free_object( ptr.get() );
    }
    ptrs.clear();

    // Reallocate all - should not need new block
    for( int i = 0; i < 8; ++i )
    {
        auto ptr = pool.allocate_object();
        BOOST_REQUIRE( ptr != nullptr );
    }

    BOOST_CHECK_EQUAL( pool.getSize(), 1 );
}

//------------------------------------------------------------------------------
// Clear Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pool_clear_empty_pool )
{
    Pool<TestObject> pool;

    // Should not crash
    pool.clear();

    BOOST_CHECK_EQUAL( pool.getSize(), 0 );
}

BOOST_AUTO_TEST_CASE( pool_clear_with_allocations )
{
    Pool<TestObject> pool( 8 );

    for( int i = 0; i < 10; ++i )
    {
        pool.allocate_object();
    }

    pool.clear();

    // After clear, new allocations should trigger new blocks
    auto ptr = pool.allocate_object();
    BOOST_REQUIRE( ptr != nullptr );
}

//------------------------------------------------------------------------------
// NextSize Configuration Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pool_set_next_size )
{
    Pool<TestObject> pool;

    pool.setNextSize( 128 );

    BOOST_CHECK_EQUAL( pool.getNextSize(), 128 );
}

BOOST_AUTO_TEST_CASE( pool_allocate_with_changed_next_size )
{
    Pool<TestObject> pool( 4 );

    // Allocate first block of 4
    for( int i = 0; i < 4; ++i )
    {
        pool.allocate_object();
    }

    BOOST_CHECK_EQUAL( pool.getSize(), 1 );

    // Change next size
    pool.setNextSize( 16 );

    // Trigger second block allocation
    pool.allocate_object();

    BOOST_CHECK_EQUAL( pool.getSize(), 2 );
}

//------------------------------------------------------------------------------
// Edge Case Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pool_zero_next_size_does_not_allocate )
{
    Pool<TestObject> pool;
    pool.setNextSize( 0 );

    // allocateData should return early when nextSize is 0
    pool.allocateData();

    BOOST_CHECK_EQUAL( pool.getSize(), 0 );
}

BOOST_AUTO_TEST_CASE( pool_large_allocation_count )
{
    Pool<TestObject> pool( 100 );

    std::vector<RawPtr<TestObject>> ptrs;
    for( int i = 0; i < 1000; ++i )
    {
        auto ptr = pool.allocate_object();
        BOOST_REQUIRE( ptr != nullptr );
        ptrs.push_back( ptr );
    }

    BOOST_CHECK_EQUAL( pool.getSize(), 10 );
}

BOOST_AUTO_TEST_CASE( pool_with_aligned_type )
{
    Pool<AlignedTestObject> pool( 8 );

    auto ptr = pool.allocate_object();
    BOOST_REQUIRE( ptr != nullptr );

    // Check alignment
    auto address = reinterpret_cast<std::uintptr_t>( ptr.get() );
    if( address % alignof( AlignedTestObject ) != 0 )
    {
        BOOST_TEST_MESSAGE( "Pool does not provide over-aligned storage on this allocator." );
    }
    else
    {
        BOOST_CHECK_EQUAL( address % alignof( AlignedTestObject ), 0 );
    }
}

//------------------------------------------------------------------------------
// Multi-threaded Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pool_concurrent_allocation )
{
    Pool<TestObject> pool( 400 );
    std::atomic<int> successCount( 0 );
    const int numThreads = 4;
    const int allocationsPerThread = 100;

    std::vector<std::thread> threads;
    for( int t = 0; t < numThreads; ++t )
    {
        threads.emplace_back( [&pool, &successCount, allocationsPerThread]() {
            for( int i = 0; i < allocationsPerThread; ++i )
            {
                auto ptr = pool.allocate_object();
                if( ptr != nullptr )
                {
                    ++successCount;
                }
            }
        } );
    }

    for( auto &thread : threads )
    {
        thread.join();
    }

    BOOST_CHECK_EQUAL( successCount.load(), numThreads * allocationsPerThread );
}

BOOST_AUTO_TEST_CASE( pool_concurrent_alloc_and_free )
{
    Pool<TestObject> pool( 16 );
    std::atomic<int> successCount( 0 );
    const int numThreads = 4;
    const int operationsPerThread = 50;

    std::vector<std::thread> threads;
    for( int t = 0; t < numThreads; ++t )
    {
        threads.emplace_back( [&pool, &successCount, operationsPerThread]() {
            for( int i = 0; i < operationsPerThread; ++i )
            {
                auto ptr = pool.allocate_object();
                if( ptr != nullptr )
                {
                    ++successCount;
                    ptr->value = i;  // Use the object
                    pool.free_object( ptr.get() );
                }
            }
        } );
    }

    for( auto &thread : threads )
    {
        thread.join();
    }

    BOOST_CHECK_EQUAL( successCount.load(), numThreads * operationsPerThread );
}

BOOST_AUTO_TEST_CASE( pool_contended_single_slot_growth_retains_unique_live_allocations )
{
    constexpr unsigned workers = 8, rounds = 64;
    Pool<TestObject> pool( 1 );
    pool.allocateData();
    PoolBarrier barrier( workers );
    std::array<std::array<TestObject *, rounds>, workers> allocations{};
    std::array<std::thread, workers> threads;
    std::atomic<unsigned> failures{ 0 };
    for( unsigned worker = 0; worker < workers; ++worker )
        threads[worker] = std::thread( [&, worker] {
            try
            {
                for( unsigned round = 0; round < rounds; ++round )
                {
                    if( !barrier.wait() ) { ++failures; return; }
                    auto slot = pool.allocate_object();
                    allocations[worker][round] = slot.get();
                    if( !slot ) ++failures;
                    // Every slot remains live while the next cohort grows the
                    // pool. A null, duplicate or lost publication is observable.
                }
            }
            catch( ... ) { ++failures; barrier.cancel(); }
        } );
    for( auto &thread : threads ) thread.join();

    BOOST_REQUIRE_EQUAL( failures.load(), 0u );
    std::set<TestObject *> live;
    for( const auto &worker : allocations )
        for( auto slot : worker ) BOOST_CHECK( slot && live.insert( slot ).second );
    BOOST_CHECK_EQUAL( live.size(), workers * rounds );
    BOOST_CHECK_EQUAL( pool.getSize(), workers * rounds );
    for( auto slot : live ) pool.free_object( slot );
}

BOOST_AUTO_TEST_CASE( pool_contended_fixed_capacity_exhaustion_returns_without_waiting )
{
    constexpr unsigned workers = 8, rounds = 64;
    Pool<TestObject> pool( 1 );
    pool.allocateData();
    pool.setNextSize( 0 );
    PoolBarrier barrier( workers );
    std::array<std::array<TestObject *, workers>, rounds> allocations{};
    std::array<std::thread, workers> threads;
    std::atomic<unsigned> failures{ 0 };
    for( unsigned worker = 0; worker < workers; ++worker )
        threads[worker] = std::thread( [&, worker] {
            try
            {
                for( unsigned round = 0; round < rounds; ++round )
                {
                    if( !barrier.wait() ) { ++failures; return; }
                    auto slot = pool.allocate_object();
                    allocations[round][worker] = slot.get();
                    // The sole winner cannot return its slot until every
                    // exhausted allocator has completed without waiting on it.
                    if( !barrier.wait() ) { ++failures; return; }
                    if( slot ) pool.free_object( slot.get() );
                    if( !barrier.wait() ) { ++failures; return; }
                }
            }
            catch( ... ) { ++failures; barrier.cancel(); }
        } );
    for( auto &thread : threads ) thread.join();

    BOOST_REQUIRE_EQUAL( failures.load(), 0u );
    for( const auto &round : allocations )
        BOOST_CHECK_EQUAL( std::count_if( round.begin(), round.end(),
                                         []( auto slot ) { return slot != nullptr; } ), 1 );
    BOOST_CHECK_EQUAL( pool.getSize(), 1u );
    auto slot = pool.allocate_object();
    BOOST_REQUIRE( slot != nullptr );
    BOOST_CHECK( pool.allocate_object() == nullptr );
    pool.free_object( slot.get() );

    Pool<TestObject> empty( 0 );
    BOOST_CHECK( empty.allocate_object() == nullptr );
    BOOST_CHECK_EQUAL( empty.getSize(), 0u );
}

//------------------------------------------------------------------------------
// Type Support Tests
//------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( pool_with_primitive_type )
{
    Pool<int> pool( 16 );

    auto ptr = pool.allocate_object();
    BOOST_REQUIRE( ptr != nullptr );

    *ptr = 12345;
    BOOST_CHECK_EQUAL( *ptr, 12345 );

    pool.free_object( ptr.get() );
}

BOOST_AUTO_TEST_CASE( pool_with_pointer_type )
{
    Pool<int *> pool( 8 );

    int testValue = 42;
    auto ptr = pool.allocate_object();
    BOOST_REQUIRE( ptr != nullptr );

    *ptr = &testValue;
    BOOST_CHECK_EQUAL( **ptr, 42 );

    pool.free_object( ptr.get() );
}
