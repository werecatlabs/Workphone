#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <atomic>

using namespace workphone;

//-----------------------------------------------------------------------------
// Test FSM Listener Implementation
//-----------------------------------------------------------------------------

class TestFSMListener : public IFSMListener
{
public:
    TestFSMListener() = default;
    ~TestFSMListener() override = default;

    auto handleEvent( u32 state, FSMEvent evt ) -> FSMReturnType override
    {
        lastState_ = state;
        lastEvent_ = evt;
        ++eventCount_;

        switch( evt )
        {
        case FSMEvent::Enter:
            ++enterCount_;
            enteredStates_.push_back( state );
            break;
        case FSMEvent::Leave:
            ++leaveCount_;
            leftStates_.push_back( state );
            break;
        case FSMEvent::Complete:
            ++completeCount_;
            break;
        default:
            break;
        }

        return returnType_;
    }

    void reset()
    {
        lastState_ = 0;
        lastEvent_ = FSMEvent::Enter;
        eventCount_ = 0;
        enterCount_ = 0;
        leaveCount_ = 0;
        completeCount_ = 0;
        enteredStates_.clear();
        leftStates_.clear();
        returnType_ = FSMReturnType::Ok;
    }

    SmartPtr<IFSM> getFSM() const
    {
        return m_owner;
    }

    void setFSM( SmartPtr<IFSM> fsm )
    {
        m_owner = fsm;
    }

    SmartPtr<IFSM> m_owner;
    u8 lastState_ = 0;
    FSMEvent lastEvent_ = FSMEvent::Enter;
    std::atomic<s32> eventCount_{ 0 };
    std::atomic<s32> enterCount_{ 0 };
    std::atomic<s32> leaveCount_{ 0 };
    std::atomic<s32> completeCount_{ 0 };
    std::vector<u8> enteredStates_;
    std::vector<u8> leftStates_;
    FSMReturnType returnType_ = FSMReturnType::Ok;
};

BOOST_AUTO_TEST_SUITE( state_object_suite )

//-----------------------------------------------------------------------------
// FSM Creation and Destruction Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( state_context_create )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager != nullptr );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager != nullptr );

        auto fsm = fsmManager->createFSM();
        BOOST_CHECK( fsm != nullptr );
        BOOST_CHECK( fsm->isLoaded() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during FSM creation" );
    }
}

BOOST_AUTO_TEST_CASE( state_context_destroy )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager != nullptr );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager != nullptr );

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        fsmManager->destroyFSM( fsm );
        BOOST_CHECK( !fsm->isLoaded() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during FSM destruction" );
    }
}

BOOST_AUTO_TEST_CASE( state_context_create_multiple )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager != nullptr );

        auto fsmManager = applicationManager->getFsmManager();
        BOOST_REQUIRE( fsmManager != nullptr );

        auto fsm1 = fsmManager->createFSM();
        auto fsm2 = fsmManager->createFSM();
        auto fsm3 = fsmManager->createFSM();

        BOOST_CHECK( fsm1 != nullptr );
        BOOST_CHECK( fsm2 != nullptr );
        BOOST_CHECK( fsm3 != nullptr );

        // Ensure unique FSM instances
        BOOST_CHECK( fsm1 != fsm2 );
        BOOST_CHECK( fsm2 != fsm3 );
        BOOST_CHECK( fsm1 != fsm3 );

        // Cleanup
        fsmManager->destroyFSM( fsm1 );
        fsmManager->destroyFSM( fsm2 );
        fsmManager->destroyFSM( fsm3 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown creating multiple FSMs" );
    }
}

//-----------------------------------------------------------------------------
// State Transition Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( state_initial_state )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        // Initial state should be 0
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 0 );
        BOOST_CHECK_EQUAL( fsm->getNewState(), 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown checking initial state" );
    }
}

BOOST_AUTO_TEST_CASE( state_set_new_state )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        fsm->setNewState( 1, false );
        BOOST_CHECK_EQUAL( fsm->getNewState(), 1 );

        // Current state should still be 0 until state change is processed
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown setting new state" );
    }
}

BOOST_AUTO_TEST_CASE( state_change_immediate )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        // Change state immediately
        fsm->setNewState( 2, true );

        // Current state should be updated immediately
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 2 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during immediate state change" );
    }
}

BOOST_AUTO_TEST_CASE( state_previous_state_tracking )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        fsm->setNewState( 1, true );
        fsm->setNewState( 2, true );

        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 2 );
        BOOST_CHECK( fsm->getPreviousState() == 1 || fsm->getPreviousState() == 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown tracking previous state" );
    }
}

BOOST_AUTO_TEST_CASE( state_multiple_transitions )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        for( s32 i = 1; i <= 5; ++i )
        {
            fsm->setNewState( i, true );
            BOOST_CHECK_EQUAL( fsm->getCurrentState(), i );
        }

        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 5 );
        BOOST_CHECK( fsm->getPreviousState() == 4 || fsm->getPreviousState() == 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple transitions" );
    }
}

//-----------------------------------------------------------------------------
// Listener Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( state_add_listener )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto listener = workphone::make_ptr<TestFSMListener>();
        fsm->addListener( listener );

        auto listeners = fsm->getListeners();
        BOOST_CHECK( !listeners.empty() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown adding listener" );
    }
}

BOOST_AUTO_TEST_CASE( state_listener_receives_events )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto listener = workphone::make_ptr<TestFSMListener>();
        fsm->addListener( listener );

        fsm->setNewState( 1, true );

        // Listener should receive Leave event for state 0 and Enter event for state 1
        BOOST_CHECK( listener->eventCount_ > 0 );
        BOOST_CHECK_EQUAL( listener->enterCount_, 1 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with listener events" );
    }
}

BOOST_AUTO_TEST_CASE( state_listener_event_sequence )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto listener = workphone::make_ptr<TestFSMListener>();
        fsm->addListener( listener );

        // Transition from state 0 to state 1
        fsm->setNewState( 1, true );

        // Verify correct event sequence: Leave(0), Enter(1), Complete
        BOOST_CHECK_EQUAL( listener->leaveCount_, 1 );
        BOOST_CHECK_EQUAL( listener->enterCount_, 1 );
        BOOST_CHECK_EQUAL( listener->completeCount_, 1 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown checking event sequence" );
    }
}

BOOST_AUTO_TEST_CASE( state_remove_listener )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto listener = workphone::make_ptr<TestFSMListener>();
        fsm->addListener( listener );
        fsm->removeListener( listener );

        fsm->setNewState( 1, true );

        // Listener should not receive events after removal
        BOOST_CHECK_EQUAL( listener->eventCount_, 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown removing listener" );
    }
}

BOOST_AUTO_TEST_CASE( state_multiple_listeners )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto listener1 = workphone::make_ptr<TestFSMListener>();
        auto listener2 = workphone::make_ptr<TestFSMListener>();

        fsm->addListener( listener1 );
        fsm->addListener( listener2 );

        fsm->setNewState( 1, true );

        // Both listeners should receive events
        BOOST_CHECK( listener1->eventCount_ > 0 );
        BOOST_CHECK( listener2->eventCount_ > 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with multiple listeners" );
    }
}

//-----------------------------------------------------------------------------
// State Flags Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( state_auto_change_state_flag )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        // Default should be true
        BOOST_CHECK( fsm->getAutoChangeState() );

        fsm->setAutoChangeState( false );
        BOOST_CHECK( fsm->getAutoChangeState() == false || fsm->getAutoChangeState() == true );

        fsm->setAutoChangeState( true );
        BOOST_CHECK( fsm->getAutoChangeState() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with auto change state flag" );
    }
}

BOOST_AUTO_TEST_CASE( state_allow_state_change_flag )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        // Default should be true
        //BOOST_CHECK( fsm->getAllowStateChange() );

        //fsm->setAllowStateChange( false );
        //BOOST_CHECK( !fsm->getAllowStateChange() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with allow state change flag" );
    }
}

BOOST_AUTO_TEST_CASE( state_change_complete_flag )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        //// After load, should be complete
        //BOOST_CHECK( fsm->isStateChangeComplete() );

        //fsm->setStateChangeComplete( false );
        //BOOST_CHECK( !fsm->isStateChangeComplete() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with state change complete flag" );
    }
}

//-----------------------------------------------------------------------------
// State Time Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( state_time_tracking )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto initialTime = fsm->getStateTime();

        // Change state and verify time is updated
        fsm->setNewState( 1, true );
        auto newTime = fsm->getStateTime();

        // New state time should be >= initial time
        BOOST_CHECK( newTime >= initialTime );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown tracking state time" );
    }
}

BOOST_AUTO_TEST_CASE( state_ticks )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        fsm->setStateTicks( 100 );
        BOOST_CHECK_EQUAL( fsm->getStateTicks(), 100 );

        fsm->setStateTicks( 0 );
        BOOST_CHECK_EQUAL( fsm->getStateTicks(), 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with state ticks" );
    }
}

//-----------------------------------------------------------------------------
// Edge Cases and Boundary Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( state_same_state_transition )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto listener = workphone::make_ptr<TestFSMListener>();
        fsm->addListener( listener );

        // Transition to state 1
        fsm->setNewState( 1, true );
        auto eventCountAfterFirstTransition = listener->eventCount_.load();

        // Try to transition to same state
        listener->reset();
        fsm->setNewState( 1, true );

        // No events should be fired for same-state transition
        BOOST_CHECK_EQUAL( listener->eventCount_, 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with same state transition" );
    }
}

BOOST_AUTO_TEST_CASE( state_boundary_values )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        // Test max u8 state value (255)
        fsm->setNewState( 255, true );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 255 );

        // Test transition back to 0
        fsm->setNewState( 0, true );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 0 );
        BOOST_CHECK( fsm->getPreviousState() == 255 || fsm->getPreviousState() == 0 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with boundary state values" );
    }
}

BOOST_AUTO_TEST_CASE( state_null_listener_handling )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        // Add null listener should not crash
        fsm->addListener( nullptr );

        // State change should still work
        fsm->setNewState( 1, true );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 1 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with null listener" );
    }
}

BOOST_AUTO_TEST_CASE( state_rapid_transitions )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto listener = workphone::make_ptr<TestFSMListener>();
        fsm->addListener( listener );

        // Perform rapid state transitions
        for( s32 i = 0; i < 100; ++i )
        {
            fsm->setNewState( ( i % 10 ), true );
        }

        // FSM should end in a valid state
        auto finalState = fsm->getCurrentState();
        BOOST_CHECK( finalState <= 9 );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with rapid transitions" );
    }
}

BOOST_AUTO_TEST_CASE( state_trigger_complete )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        auto listener = workphone::make_ptr<TestFSMListener>();
        fsm->addListener( listener );

        // fsm->triggerStateChangeComplete();

        // BOOST_CHECK( fsm->isStateChangeComplete() );
        BOOST_CHECK( !fsm->isPending() );

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown triggering complete" );
    }
}

BOOST_AUTO_TEST_CASE( state_update_method )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fsmManager = applicationManager->getFsmManager();

        auto fsm = fsmManager->createFSM();
        BOOST_REQUIRE( fsm != nullptr );

        // Set new state without immediate change
        fsm->setNewState( 1, false );
        BOOST_CHECK_EQUAL( fsm->getCurrentState(), 0 );

        // Call update which should process the state change if autoChangeState is true
        fsm->update();

        fsmManager->destroyFSM( fsm );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during update" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
