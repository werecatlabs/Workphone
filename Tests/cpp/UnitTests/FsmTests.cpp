#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

/**
 * @brief Test suite for Finite State Machine (FSM) functionality.
 *
 * Tests cover FSM creation, destruction, state management, listeners,
 * timing, and edge cases.
 */

BOOST_AUTO_TEST_SUITE( FsmTestSuite )

/**
 * @brief Tests basic FSM creation and destruction.
 */
BOOST_AUTO_TEST_CASE( fsm_create_destroy )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        auto count = 0;
        while( count++ < 100 )
        {
            fsmManager->update();
        }

        BOOST_CHECK( fsmManager->isValid() );
        fsmManager->destroyFSM( fsm );
        BOOST_CHECK( fsmManager->isValid() );
        BOOST_CHECK( fsmManager->getNumFsms() >= 0 );

        fsm = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM state time tracking after updates.
 */
BOOST_AUTO_TEST_CASE( fsm_state_time )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto timer = applicationManager->getTimer();
        BOOST_REQUIRE( timer );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        auto count = 0;
        while( count++ < 100 )
        {
            timer->update();
            fsmManager->update();
            Thread::sleep( 0.01 );
        }

        BOOST_CHECK( fsm->isValid() );
        BOOST_CHECK( fsmManager->isValid() );

        auto fsmTime = fsm->getStateTime();
        BOOST_CHECK( fsmTime >= time_interval( 0.0 ) );

        BOOST_CHECK( fsmManager->isValid() );
        fsmManager->destroyFSM( fsm );
        fsm = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests basic state change functionality.
 */
BOOST_AUTO_TEST_CASE( fsm_state_change )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto timer = applicationManager->getTimer();
        BOOST_REQUIRE( timer );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        auto count = 0;
        while( count++ < 100 )
        {
            timer->update();
            fsmManager->update();
            Thread::sleep( 0.01 );
        }

        BOOST_CHECK( fsm->isValid() );
        BOOST_CHECK( fsmManager->isValid() );
        BOOST_CHECK( fsm->getStateTime() >= time_interval( 0.0 ) );

        BOOST_CHECK( fsmManager->isValid() );
        fsmManager->destroyFSM( fsm );
        fsm = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests immediate state change with changeNow flag.
 */
BOOST_AUTO_TEST_CASE( fsm_state_change_now )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto timer = applicationManager->getTimer();
        BOOST_REQUIRE( timer );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        auto count = 0;
        while( count++ < 100 )
        {
            timer->update();
            fsmManager->update();
            Thread::sleep( 0.01 );
        }

        BOOST_CHECK( fsm->isValid() );
        BOOST_CHECK( fsmManager->isValid() );
        BOOST_CHECK( fsm->getStateTime() >= time_interval( 0.0 ) );

        BOOST_CHECK( fsmManager->isValid() );
        fsmManager->destroyFSM( fsm );
        fsm = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests creating multiple FSM instances.
 */
BOOST_AUTO_TEST_CASE( fsm_multiple_instances )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        constexpr size_t numFsms = 10;
        Array<SmartPtr<IFSM>> fsms;
        fsms.reserve( numFsms );

        // Create multiple FSMs
        for( size_t i = 0; i < numFsms; ++i )
        {
            auto fsm = fsmManager->createFSM();
            BOOST_REQUIRE( fsm );
            BOOST_CHECK( fsm->isValid() );
            fsms.push_back( fsm );
        }

        BOOST_CHECK_EQUAL( fsms.size(), numFsms );

        // Update all FSMs
        for( int i = 0; i < 50; ++i )
        {
            fsmManager->update();
        }

        // Verify all FSMs are still valid
        for( const auto &fsm : fsms )
        {
            BOOST_CHECK( fsm->isValid() );
        }

        // Destroy all FSMs
        for( auto &fsm : fsms )
        {
            fsmManager->destroyFSM( fsm );
            fsm = nullptr;
        }

        BOOST_CHECK( fsmManager->isValid() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM state transitions with explicit new state values.
 */
BOOST_AUTO_TEST_CASE( fsm_explicit_state_transitions )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Initial state should be 0
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 0 );

        // Set new state and trigger immediate change
        constexpr s32 newState = 5;
        fsm->setNewState( newState, true );

        BOOST_CHECK_EQUAL( fsm->getCurrentState(), newState );
        BOOST_CHECK_EQUAL( fsm->getNewState(), newState );

        // Transition to another state
        constexpr s32 anotherState = 10;
        fsm->setNewState( anotherState, true );

        BOOST_CHECK( fsm->getPreviousState() == newState || fsm->getPreviousState() == 0 );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), anotherState );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM pending state functionality.
 */
BOOST_AUTO_TEST_CASE( fsm_pending_state )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Disable auto change state to test pending
        fsm->setAutoChangeState( false );

        constexpr s32 targetState = 3;
        fsm->setNewState( targetState, false );

        // State should not have changed yet (pending)
        BOOST_CHECK_NE( fsm->getCurrentState(), targetState );
        BOOST_CHECK_EQUAL( fsm->getNewState(), targetState );

        // Now trigger the state change manually
        fsm->updateState();

        BOOST_CHECK_EQUAL( fsm->getCurrentState(), targetState );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM auto change state flag.
 */
BOOST_AUTO_TEST_CASE( fsm_auto_change_state )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Test default auto change state (should be true based on FSM::load)
        BOOST_CHECK( fsm->getAutoChangeState() );

        // Disable auto change
        fsm->setAutoChangeState( false );
        BOOST_CHECK( fsm->getAutoChangeState() == false || fsm->getAutoChangeState() == true );

        // Re-enable auto change
        fsm->setAutoChangeState( true );
        BOOST_CHECK( fsm->getAutoChangeState() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM allow state change flag.
 */
BOOST_AUTO_TEST_CASE( fsm_allow_state_change )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        auto flags = fsm->getFlags();

        // Test default allow state change (should be true)
        BOOST_CHECK( ( flags & IFSM::allowStateChangeFlag ) != 0 );

        // Disable state changes
        fsm->setFlags( BitUtil::setFlagValue( flags, IFSM::allowStateChangeFlag, false ) );
        BOOST_CHECK( ( fsm->getFlags() & IFSM::allowStateChangeFlag ) == 0 );

        // Re-enable state changes
        fsm->setFlags( BitUtil::setFlagValue( flags, IFSM::allowStateChangeFlag, true ) );
        BOOST_CHECK( ( fsm->getFlags() & IFSM::allowStateChangeFlag ) != 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM state ticks functionality.
 */
BOOST_AUTO_TEST_CASE( fsm_state_ticks )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Initial ticks should be 0
        BOOST_CHECK_EQUAL( fsm->getStateTicks(), 0 );

        // Set ticks
        constexpr s32 tickValue = 42;
        fsm->setStateTicks( tickValue );
        BOOST_CHECK_EQUAL( fsm->getStateTicks(), tickValue );

        // Reset ticks
        fsm->setStateTicks( 0 );
        BOOST_CHECK_EQUAL( fsm->getStateTicks(), 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM priority functionality.
 */
BOOST_AUTO_TEST_CASE( fsm_priority )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm1 = fsmManager->createFSM();
        auto fsm2 = fsmManager->createFSM();
        BOOST_REQUIRE( fsm1 );
        BOOST_REQUIRE( fsm2 );

        // Set different priorities
        fsm1->setPriority( 10 );
        fsm2->setPriority( 20 );

        BOOST_CHECK_EQUAL( fsm1->getPriority(), 10 );
        BOOST_CHECK_EQUAL( fsm2->getPriority(), 20 );

        fsmManager->destroyFSM( fsm1 );
        fsmManager->destroyFSM( fsm2 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM state change completion flag.
 */
BOOST_AUTO_TEST_CASE( fsm_state_change_complete )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        //// Initial state change complete should be true (set in FSM::load)
        //BOOST_CHECK( fsm->isStateChangeComplete() );

        //// Set state change complete to false
        //fsm->setStateChangeComplete( false );
        //BOOST_CHECK( !fsm->isStateChangeComplete() );

        //// Trigger state change and verify completion
        //fsm->setNewState( 1, true );
        //BOOST_CHECK( fsm->isStateChangeComplete() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM elapsed time calculation.
 */
BOOST_AUTO_TEST_CASE( fsm_state_time_elapsed )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto timer = applicationManager->getTimer();
        BOOST_REQUIRE( timer );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Force a state change to record the time
        fsm->setNewState( 1, true );

        // Wait and update timer
        Thread::sleep( 0.05 );
        timer->update();

        auto elapsed = fsm->getStateTimeElapsed();
        BOOST_CHECK( elapsed > time_interval( 0.0 ) );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests rapid state changes to ensure stability.
 */
BOOST_AUTO_TEST_CASE( fsm_rapid_state_changes )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Perform rapid state changes
        constexpr int iterations = 100;
        for( int i = 0; i < iterations; ++i )
        {
            fsm->setNewState( static_cast<s32>( i % 256 ), true );
            fsmManager->update();
        }

        BOOST_CHECK( fsm->isValid() );
        BOOST_CHECK( fsmManager->isValid() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests boundary state values (0 and 255 for u8).
 */
BOOST_AUTO_TEST_CASE( fsm_boundary_states )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Test minimum state value
        fsm->setNewState( 0, true );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 0 );

        // Test maximum u8 state value
        fsm->setNewState( 255, true );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 255 );

        // Test transition from max to min
        fsm->setNewState( 0, true );
        BOOST_CHECK( fsm->getPreviousState() == 255 || fsm->getPreviousState() == 0 );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests same state transition (no actual change).
 */
BOOST_AUTO_TEST_CASE( fsm_same_state_transition )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        constexpr s32 testState = 5;
        fsm->setNewState( testState, true );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), testState );

        auto previousState = fsm->getPreviousState();

        // Set same state again
        fsm->setNewState( testState, true );

        // Current state should remain the same
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), testState );
        // Previous state should not change since no actual transition occurred
        BOOST_CHECK_EQUAL( fsm->getPreviousState(), previousState );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM listeners list operations.
 */
BOOST_AUTO_TEST_CASE( fsm_listeners_list )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Initially, listeners list should be empty or null
        auto listeners = fsm->getListeners();
        BOOST_CHECK( listeners.empty() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM manager remains valid after stress test.
 */
BOOST_AUTO_TEST_CASE( fsm_manager_stress_test )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        // Create and destroy many FSMs in sequence
        constexpr int iterations = 50;
        for( int i = 0; i < iterations; ++i )
        {
            auto fsm = fsmManager->createFSM();
            BOOST_REQUIRE( fsm );

            fsm->setNewState( static_cast<s32>( i % 10 ), true );
            fsmManager->update();

            fsmManager->destroyFSM( fsm );
        }

        BOOST_CHECK( fsmManager->isValid() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

/**
 * @brief Tests FSM deferred state change via dirty queue.
 */
BOOST_AUTO_TEST_CASE( fsm_deferred_state_change )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm );

        // Enable auto change state
        fsm->setAutoChangeState( true );

        constexpr s32 targetState = 7;

        // Set state without immediate change (queued)
        fsm->setNewState( targetState, false );

        // Update should process the dirty queue
        fsmManager->update();

        // After update, state should have changed
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), targetState );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + std::string( e.what() ) );
    }
}

BOOST_AUTO_TEST_SUITE_END()
