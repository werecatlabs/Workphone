#include <Workphone/Atomics/Atomic.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <limits>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    template <typename T>
    auto hasAtomicArithmetic( int )
        -> decltype( std::declval<workphone::Atomic<T> &>().fetch_and_add( 1 ), std::true_type{} );

    template <typename T>
    std::false_type hasAtomicArithmetic( ... );

    static_assert( decltype( hasAtomicArithmetic<int>( 0 ) )::value, "Integer arithmetic" );
    static_assert( decltype( hasAtomicArithmetic<int *>( 0 ) )::value, "Pointer arithmetic" );
    static_assert( !decltype( hasAtomicArithmetic<bool>( 0 ) )::value, "No boolean arithmetic" );
    static_assert( !decltype( hasAtomicArithmetic<float>( 0 ) )::value, "No floating arithmetic" );
    static_assert( !decltype( hasAtomicArithmetic<void *>( 0 ) )::value, "No void arithmetic" );
    static_assert( !decltype( hasAtomicArithmetic<void ( * )()>( 0 ) )::value,
                   "No function pointer arithmetic" );

    template <workphone::memory_semantics M>
    void checkAtomicOrdering()
    {
        workphone::Atomic<int> value( 1 );
        BOOST_CHECK_EQUAL( value.fetch_and_store<M>( 2 ), 1 );
        BOOST_CHECK_EQUAL( value.compare_and_swap<M>( 3, 2 ), 2 );
        BOOST_CHECK_EQUAL( value.compare_and_swap<M>( 9, 2 ), 3 );
        BOOST_CHECK_EQUAL( value.fetch_and_add<M>( 4 ), 3 );
        BOOST_CHECK_EQUAL( value.fetch_and_increment<M>(), 7 );
        BOOST_CHECK_EQUAL( value.fetch_and_decrement<M>(), 8 );
        BOOST_CHECK_EQUAL( value.load(), 7 );
    }
}

BOOST_AUTO_TEST_CASE( atomic_initialization_and_snapshot_copy )
{
    workphone::Atomic<int> value;
    workphone::Atomic<bool> flag;
    workphone::Atomic<int *> pointer;
    BOOST_CHECK_EQUAL( value.load(), 0 );
    BOOST_CHECK( !flag.load() );
    BOOST_CHECK( pointer.load() == nullptr );

    BOOST_CHECK_EQUAL( value = 12, 12 );
    workphone::Atomic<int> copy( value );
    value = 19;
    BOOST_CHECK_EQUAL( copy.load(), 12 );
    BOOST_CHECK( &( copy = value ) == &copy );
    BOOST_CHECK_EQUAL( copy.load(), 19 );
    copy = copy;
    BOOST_CHECK_EQUAL( static_cast<int>( copy ), 19 );
    const volatile workphone::Atomic<int> legacy( 23 );
    BOOST_CHECK_EQUAL( static_cast<int>( legacy ), 23 );
    auto made = workphone::make_atomic( 42 );
    BOOST_CHECK_EQUAL( made.load(), 42 );
}

BOOST_AUTO_TEST_CASE( atomic_tbb_compare_and_swap_returns_observed_value )
{
    workphone::Atomic<int> value( 5 );
    BOOST_CHECK_EQUAL( value.compare_and_swap( 8, 5 ), 5 );
    BOOST_CHECK_EQUAL( value.load(), 8 );
    BOOST_CHECK_EQUAL( value.compare_and_swap( 12, 5 ), 8 );
    BOOST_CHECK_EQUAL( value.load(), 8 );
    BOOST_CHECK_EQUAL( value.fetch_and_store( 20 ), 8 );
    BOOST_CHECK_EQUAL( value.load(), 20 );
}

BOOST_AUTO_TEST_CASE( atomic_compile_time_and_runtime_ordering )
{
    checkAtomicOrdering<workphone::memory_semantics::full_fence>();
    checkAtomicOrdering<workphone::memory_semantics::acquire>();
    checkAtomicOrdering<workphone::memory_semantics::release>();
    checkAtomicOrdering<workphone::memory_semantics::relaxed>();
    checkAtomicOrdering<workphone::memory_semantics::acq_rel>();

    workphone::Atomic<int> value;
    value.store<workphone::memory_semantics::relaxed>( 1 );
    BOOST_CHECK_EQUAL( value.load<workphone::memory_semantics::relaxed>(), 1 );
    workphone::store<workphone::memory_semantics::release>( value, 2 );
    BOOST_CHECK_EQUAL( workphone::load<workphone::memory_semantics::acquire>( value ), 2 );
    value.store<workphone::memory_semantics::full_fence>( 3 );
    BOOST_CHECK_EQUAL( value.load<workphone::memory_semantics::full_fence>(), 3 );
    value.store( 4, workphone::memory_semantics::release );
    BOOST_CHECK_EQUAL( value.load( workphone::memory_semantics::acquire ), 4 );
}

BOOST_AUTO_TEST_CASE( atomic_engine_api_and_operator_return_values )
{
    workphone::Atomic<int> value( 10 );
    BOOST_CHECK_EQUAL( value++, 10 );
    BOOST_CHECK_EQUAL( ++value, 12 );
    BOOST_CHECK_EQUAL( value--, 12 );
    BOOST_CHECK_EQUAL( --value, 10 );
    BOOST_CHECK_EQUAL( value += 4, 14 );
    BOOST_CHECK_EQUAL( value -= 3, 11 );
    BOOST_CHECK_EQUAL( value.fetch_add( 2 ), 11 );
    BOOST_CHECK_EQUAL( value.fetch_sub( 1 ), 13 );
    BOOST_CHECK_EQUAL( value.exchange( 7 ), 12 );
    BOOST_CHECK_EQUAL( value.fetch_and( 3 ), 7 );
    BOOST_CHECK_EQUAL( value.fetch_or( 8 ), 3 );
    BOOST_CHECK_EQUAL( value.fetch_xor( 2 ), 11 );
    BOOST_CHECK_EQUAL( value.load(), 9 );

    int expected = 8;
    BOOST_CHECK( !value.compare_exchange_strong( expected, 14 ) );
    BOOST_CHECK_EQUAL( expected, 9 );
    BOOST_CHECK( value.compare_exchange_strong( expected, 14, workphone::memory_semantics::release,
                                              workphone::memory_semantics::relaxed ) );
    expected = 14;
    while( !value.compare_exchange_weak( expected, 15, workphone::memory_semantics::acq_rel ) )
    {
        expected = 14;
    }
    BOOST_CHECK_EQUAL( value.load(), 15 );

    workphone::Atomic<std::uint8_t> byteValue( 255 );
    BOOST_CHECK_EQUAL( byteValue.fetch_and_increment(), 255 );
    BOOST_CHECK_EQUAL( byteValue.load(), 0 );
    workphone::Atomic<std::uint64_t> wide( ( std::uint64_t( 1 ) << 40 ) );
    BOOST_CHECK_EQUAL( wide.fetch_and_add( 2 ), ( std::uint64_t( 1 ) << 40 ) );
    BOOST_CHECK_EQUAL( wide.load(), ( std::uint64_t( 1 ) << 40 ) + 2 );
    workphone::Atomic<int> signedLimit( ( std::numeric_limits<int>::max )() );
    BOOST_CHECK_EQUAL( ++signedLimit, ( std::numeric_limits<int>::min )() );
}

BOOST_AUTO_TEST_CASE( atomic_native_widths_volatile_and_wrapping_arithmetic )
{
    workphone::Atomic<std::uint16_t> halfWord( 65535 );
    BOOST_CHECK_EQUAL( halfWord.fetch_add( 1 ), 65535 );
    BOOST_CHECK_EQUAL( halfWord.load(), 0 );
    BOOST_CHECK_EQUAL( halfWord.fetch_sub( 1 ), 0 );
    BOOST_CHECK_EQUAL( halfWord.load(), 65535 );
    BOOST_CHECK_EQUAL( halfWord &= 15, 15 );
    BOOST_CHECK_EQUAL( halfWord |= 16, 31 );
    BOOST_CHECK_EQUAL( halfWord ^= 3, 28 );

    workphone::Atomic<std::int64_t> signedWide( ( std::numeric_limits<std::int64_t>::min )() );
    BOOST_CHECK_EQUAL( --signedWide, ( std::numeric_limits<std::int64_t>::max )() );
    signedWide = 0;
    BOOST_CHECK_EQUAL( signedWide -= ( std::numeric_limits<std::int64_t>::min )(),
                       ( std::numeric_limits<std::int64_t>::min )() );
    BOOST_CHECK( signedWide.is_lock_free() );
    BOOST_CHECK( reinterpret_cast<std::uintptr_t>( &signedWide ) % 8 == 0 );

    volatile workphone::Atomic<int> value( 1 );
    value = 2;
    BOOST_CHECK_EQUAL( value.fetch_and_increment(), 2 );
    int expected = 3;
    BOOST_CHECK( value.compare_exchange_strong( expected, 4 ) );
    workphone::store<workphone::memory_semantics::release>( value, 5 );
    BOOST_CHECK_EQUAL( workphone::load<workphone::memory_semantics::acquire>( value ), 5 );
    BOOST_CHECK_EQUAL( static_cast<int>( value ), 5 );
}

BOOST_AUTO_TEST_CASE( atomic_compare_exchange_uses_object_bits )
{
    workphone::Atomic<float> value( 0.0f );
    float expected = -0.0f;
    BOOST_CHECK( !value.compare_exchange_strong( expected, 1.0f ) );
    std::uint32_t observedBits;
    memcpy( &observedBits, &expected, sizeof( observedBits ) );
    BOOST_CHECK_EQUAL( observedBits, 0u );

    const std::uint32_t nanBits = 0x7fc01234u;
    float nanValue;
    memcpy( &nanValue, &nanBits, sizeof( nanValue ) );
    value.store( nanValue );
    BOOST_CHECK( value.compare_exchange_strong( nanValue, 2.0f ) );
    BOOST_CHECK_EQUAL( value.load(), 2.0f );
}

BOOST_AUTO_TEST_CASE( atomic_pointer_offsets_and_member_access )
{
    struct Item
    {
        int value;
        int padding[3];
    };
    Item items[6] = {};
    items[2].value = 42;
    workphone::Atomic<Item *> pointer( items );
    BOOST_CHECK( pointer.fetch_and_add( 2 ) == items );
    BOOST_CHECK_EQUAL( pointer->value, 42 );
    BOOST_CHECK( pointer.fetch_and_add( -1 ) == items + 2 );
    BOOST_CHECK( pointer++ == items + 1 );
    BOOST_CHECK( ++pointer == items + 3 );
    BOOST_CHECK( pointer-- == items + 3 );
    BOOST_CHECK( --pointer == items + 1 );
    BOOST_CHECK( ( pointer += 3 ) == items + 4 );
    BOOST_CHECK( ( pointer -= 2 ) == items + 2 );
    BOOST_CHECK( pointer.fetch_and_increment() == items + 2 );
    BOOST_CHECK( pointer.fetch_and_decrement() == items + 3 );
    BOOST_CHECK( pointer.compare_and_swap( items + 5, items + 2 ) == items + 2 );
    BOOST_CHECK( pointer.load() == items + 5 );
    BOOST_CHECK( pointer.compare_and_swap( items, items + 2 ) == items + 5 );

    workphone::Atomic<void *> opaque( items );
    BOOST_CHECK( opaque.fetch_and_store( nullptr ) == items );
    BOOST_CHECK( opaque.load() == nullptr );
}

BOOST_AUTO_TEST_CASE( atomic_non_arithmetic_values )
{
    enum class Status : unsigned int
    {
        idle,
        ready
    };
    workphone::Atomic<Status> status( Status::idle );
    BOOST_CHECK( status.compare_and_swap( Status::ready, Status::idle ) == Status::idle );
    BOOST_CHECK( status.load() == Status::ready );

    struct Payload
    {
        std::uint32_t first;
        std::uint32_t second;
    };
    workphone::Atomic<Payload> payload( Payload{ 1, 2 } );
    auto previous = payload.compare_and_swap( Payload{ 3, 4 }, Payload{ 1, 2 } );
    BOOST_CHECK_EQUAL( previous.first, 1u );
    BOOST_CHECK_EQUAL( payload.load().second, 4u );

    workphone::Atomic<float> number( 1.5f );
    BOOST_CHECK_EQUAL( number.compare_and_swap( 2.5f, 1.5f ), 1.5f );
    BOOST_CHECK_EQUAL( number.load(), 2.5f );
    workphone::Atomic<bool> flag( false );
    BOOST_CHECK( !flag.compare_and_swap( true, false ) );
    BOOST_CHECK( flag.load() );
}

BOOST_AUTO_TEST_CASE( atomic_contended_fetch_and_compare_and_swap )
{
    workphone::Atomic<int> fetched;
    workphone::Atomic<int> swapped;
    const int threadCount = 6;
    const int iterations = 20000;
    std::vector<std::thread> threads;
    for( int i = 0; i < threadCount; ++i )
    {
        threads.emplace_back( [&]() {
            for( int j = 0; j < iterations; ++j )
            {
                fetched.fetch_and_increment<workphone::memory_semantics::relaxed>();
                int expected = swapped.load<workphone::memory_semantics::relaxed>();
                for( ;; )
                {
                    int observed = swapped.compare_and_swap<workphone::memory_semantics::relaxed>( expected + 1,
                                                                                expected );
                    if( observed == expected )
                    {
                        break;
                    }
                    expected = observed;
                }
            }
        } );
    }
    for( auto &thread : threads )
    {
        thread.join();
    }
    BOOST_CHECK_EQUAL( fetched.load(), threadCount * iterations );
    BOOST_CHECK_EQUAL( swapped.load(), threadCount * iterations );
}

BOOST_AUTO_TEST_CASE( atomic_number_and_value_use_engine_memory_orders )
{
    using Order = workphone::memory_semantics;
    workphone::AtomicNumber<unsigned int> remaining( 3 );
    BOOST_CHECK_EQUAL( remaining.fetch_sub( 1, Order::release ), 3u );
    BOOST_CHECK_EQUAL( remaining.load( Order::acquire ), 2u );
    unsigned int expectedCount = 2;
    BOOST_CHECK( remaining.compareExchange( expectedCount, 1, Order::release ) );
    workphone::AtomicNumber<bool> ready;
    ready.store( true, Order::release );
    BOOST_CHECK( ready.load( Order::acquire ) );

    enum class State : unsigned int
    {
        idle,
        ready
    };
    workphone::AtomicValue<State> state( State::idle );
    state.store( State::ready, Order::release );
    BOOST_CHECK( state.load( Order::acquire ) == State::ready );
    State expectedState = State::ready;
    BOOST_CHECK( state.compare_exchange_weak( expectedState, State::idle, Order::release,
                                             Order::relaxed ) );
    BOOST_CHECK( !state.compare_exchange_weak( expectedState, State::ready, Order::acquire,
                                              Order::relaxed ) );
    BOOST_CHECK( expectedState == State::idle );
}

BOOST_AUTO_TEST_CASE( atomic_default_acquire_release_publishes_data )
{
    workphone::Atomic<bool> ready;
    int payload = 0;
    std::thread publisher( [&]() {
        payload = 42;
        ready = true;
    } );
    while( !ready )
    {
        std::this_thread::yield();
    }
    const int observed = payload;
    publisher.join();
    BOOST_CHECK_EQUAL( observed, 42 );
}
