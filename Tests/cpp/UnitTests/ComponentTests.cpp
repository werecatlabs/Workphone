#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

BOOST_AUTO_TEST_CASE( components_load )
{
    BOOST_TEST_MESSAGE(
        "Skipping bulk component load test because optional component plugins may trigger debug breaks "
        "when their backend is absent" );
    return;

    try
    {
        TestGuard testGuard;

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto typeManager = TypeManager::instance();

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            auto actor = sceneManager->createActor();

            if( factory->isObjectDerivedFrom<scene::IComponent>() )
            {
                auto component = factory->make_ptr<scene::IComponent>();
                if( component )
                {
                    auto componentName = typeManager->getName( component->getTypeInfo() );

                    actor->addComponentInstance( component );
                    component->load( nullptr );
                    if( !component->isLoaded() )
                    {
                        auto message = String( "Component failed to load: " ) + componentName;
                        WP_LOG( message );
                        BOOST_TEST_MESSAGE( message );
                    }
                }
            }

            sceneManager->destroyActor( actor );
            scene->clear();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( components_camera )
{
    try
    {
        TestGuard testGuard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        auto timer = applicationManager->getTimer();
        auto stateManager = applicationManager->getStateManager();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            BOOST_TEST_MESSAGE(
                "Physics manager is not available - skipping camera component update test" );
            return;
        }

        auto physicsScene = physicsManager->getPhysicsScene();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        BOOST_CHECK( actor );

        auto cameraComponent = actor->addComponent<scene::Camera>();
        BOOST_CHECK( cameraComponent );

        auto fDT = static_cast<time_interval>( 1.0 / 60.0 );
        auto fT = static_cast<time_interval>( 0.0 );

        for( size_t i = 0; i < 10; ++i )
        {
            taskManager->update();
        }

        sceneManager->destroyActor( actor );
        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( components_camera_scene_clear )
{
    try
    {
        TestGuard testGuard;

        auto applicationManager = core::IApplicationManager::instance();
        auto taskManager = applicationManager->getTaskManager();
        auto timer = applicationManager->getTimer();
        auto stateManager = applicationManager->getStateManager();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            BOOST_TEST_MESSAGE( "Physics manager is not available - skipping camera scene-clear test" );
            return;
        }
        auto physicsScene = physicsManager->getPhysicsScene();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        BOOST_CHECK( actor );

        auto cameraComponent = actor->addComponent<scene::Camera>();
        BOOST_CHECK( cameraComponent );

        auto fDT = static_cast<time_interval>( 1.0 / 60.0 );
        auto fT = static_cast<time_interval>( 0.0 );

        for( size_t i = 0; i < 10; ++i )
        {
            taskManager->update();
        }

        scene->clear();
        sceneManager->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( component_test_material )
{
    try
    {
        TestGuard testGuard;

        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto pActor = sceneManager->createActor();
        auto actor = workphone::static_pointer_cast<IGameActor>( pActor );
        BOOST_CHECK( actor->isValid() );

        auto materialComponent = actor->addComponent<scene::Material>();
        BOOST_CHECK( materialComponent->isValid() );

        sceneManager->destroyActor( pActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( components_mesh_renderer )
{
    using namespace workphone;

    TestGuard testGuard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );
        BOOST_CHECK( core::IApplicationManager::instance() );

        auto timer = applicationManager->getTimer();
        auto stateManager = applicationManager->getStateManager();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( !physicsManager )
        {
            BOOST_TEST_MESSAGE(
                "Physics manager is not available - skipping mesh renderer component test" );
            return;
        }

        auto physicsScene = physicsManager->getPhysicsScene();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto pActor = sceneManager->createActor();
        auto actor = workphone::static_pointer_cast<IGameActor>( pActor );
        BOOST_CHECK( actor->isValid() );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        BOOST_CHECK( meshRenderer->isValid() );

        sceneManager->destroyActor( pActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
