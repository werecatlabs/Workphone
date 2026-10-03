#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

// Create a test event listener
class TestEventListener : public workphone::IEventListener
{
public:
    bool eventReceived = false;
    Parameter handleEvent( EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
                           SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                           SmartPtr<IEvent> event ) override
    {
        eventReceived = true;
        return Parameter();
    }

    WP_CLASS_REGISTER_DECL;
};

WP_CLASS_REGISTER_DERIVED( workphone, TestEventListener, IEventListener );

BOOST_AUTO_TEST_CASE( application_manager )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        if( applicationManager )
        {
            BOOST_CHECK( applicationManager->isValid() );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( application_manager_events )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        if( applicationManager )
        {
            BOOST_CHECK( applicationManager->isValid() );

            auto application = applicationManager->getApplication();

            auto listener = workphone::make_ptr<TestEventListener>();

            // Register the listener
            applicationManager->addObjectListener( listener );

            // Trigger a test event
            Array<Parameter> arguments;
            applicationManager->triggerEvent( EventType::Application,
                                              StringUtil::getHash( "test_event" ), arguments,
                                              applicationManager, applicationManager, nullptr, true );

            // Verify the event was received
            BOOST_CHECK( listener->eventReceived );

            // Clean up
            applicationManager->removeObjectListener( listener );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( application_manager_events_mt )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        if( applicationManager )
        {
            BOOST_CHECK( applicationManager->isValid() );

            auto taskManager = applicationManager->getTaskManager();

            auto application = applicationManager->getApplication();

            auto listener = workphone::make_ptr<TestEventListener>();

            // Register the listener
            applicationManager->addObjectListener( listener );

            // Trigger a test event
            Array<Parameter> arguments;
            applicationManager->triggerEvent( EventType::Application,
                                              StringUtil::getHash( "test_event" ), arguments,
                                              applicationManager, applicationManager, nullptr, false );

            auto count = 0;
            while( !listener->eventReceived && count++ < 100 )
            {
                taskManager->update();
            }

            // Verify the event was received
            BOOST_CHECK( listener->eventReceived );

            // Clean up
            applicationManager->removeObjectListener( listener );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( application_component )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto fileSystem = applicationManager->getFileSystem();
        auto prefabManager = applicationManager->getPrefabManager();

        auto actor = sceneManager->createActor();
        //auto application = actor->addComponent<scene::Application>();
        //WP_ASSERT( application );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( application_simulator )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto fileSystem = applicationManager->getFileSystem();
        auto prefabManager = applicationManager->getPrefabManager();

        auto actor = sceneManager->createActor();
        //auto application = actor->addComponent<scene::SimulatorApplication>();
        //WP_ASSERT( application );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( application_truck_simulator )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto fileSystem = applicationManager->getFileSystem();
        auto prefabManager = applicationManager->getPrefabManager();

        auto actor = sceneManager->createActor();
        //auto application = actor->addComponent<scene::TruckSimulatorApplication>();
        //WP_ASSERT( application );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
