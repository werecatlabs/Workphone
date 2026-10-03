#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( state_unload_after_owner_destruction )
{
    auto state = workphone::make_ptr<State>();
    BOOST_REQUIRE( state );

    {
        auto owner = workphone::make_ptr<ISharedObject>();
        BOOST_REQUIRE( owner );
        state->setOwner( owner );
    }

    BOOST_CHECK_NO_THROW( state->unload( nullptr ) );
    BOOST_CHECK( !state->getOwner() );
}

BOOST_AUTO_TEST_CASE( state_manager_fast )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();

        auto count = 0;
        while( count < 3 )
        {
            taskManager->update();
            count++;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( state_manager_standard )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();

        auto count = 0;
        while( count < 3 )
        {
            taskManager->update();
            count++;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( state_manager_tbb )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();

        auto count = 0;
        while( count < 3 )
        {
            taskManager->update();
            count++;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( state_context_data_by_id_accepts_derived_state_data )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        BOOST_REQUIRE( stateManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        auto stateContext = stateManager->addStateContext();
        BOOST_REQUIRE( stateContext );

        const hash_type stateId = 0x5A7E0001;

        {
            auto state = factoryManager->make_ptr<State>();
            BOOST_REQUIRE( state );
            state->setId( stateId );

            auto meshStateData = factoryManager->make_ptr<GraphicsMeshState>();
            BOOST_REQUIRE( meshStateData );
            state->setData( meshStateData );
            stateContext->addState( state );

            auto readData = stateContext->getStateDataById<GraphicsObjectData>( stateId );
            BOOST_REQUIRE( readData );
            BOOST_CHECK(
                BitUtil::getFlagValue( readData->flags, render::IGraphicsObject::visibleFlag ) );
        }

        {
            auto writeData = stateContext->invalidateStateDataById<GraphicsObjectData>( stateId );
            BOOST_REQUIRE( writeData );
            writeData->flags =
                BitUtil::setFlagValue( writeData->flags, render::IGraphicsObject::visibleFlag, false );
        }

        {
            auto updatedData = stateContext->getStateDataById<GraphicsObjectData>( stateId );
            BOOST_REQUIRE( updatedData );
            BOOST_CHECK(
                !BitUtil::getFlagValue( updatedData->flags, render::IGraphicsObject::visibleFlag ) );
        }

        stateManager->removeStateContext( stateContext );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during derived state data lookup test" );
    }
}
