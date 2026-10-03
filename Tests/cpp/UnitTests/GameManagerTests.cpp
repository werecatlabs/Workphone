#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( gamemanager_create_actor )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto timer = applicationManager->getTimer();
        auto stateManager = applicationManager->getStateManager();

        BOOST_REQUIRE( applicationManager != nullptr );
        BOOST_REQUIRE( sceneManager != nullptr );
        BOOST_REQUIRE( scene != nullptr );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor != nullptr );

        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
        actor->setPosition( Vector3F( 0, 0, 0 ) );

        auto actorImageParent = sceneManager->createActor();
        BOOST_REQUIRE( actorImageParent != nullptr );
        actor->addChild( actorImageParent );

        auto actorImage = sceneManager->createActor();
        BOOST_REQUIRE( actorImage != nullptr );
        actorImageParent->addChild( actorImage );

        auto count = 0;
        while( count++ < 10 )
        {
            timer->update();

            stateManager->preUpdate();
            stateManager->update();
            stateManager->postUpdate();

            sceneManager->preUpdate();
            sceneManager->update();
            sceneManager->postUpdate();
        }

        scene->removeActor( actor );
        scene->unregisterAll( actor );
        sceneManager->destroyActor( actor );

        BOOST_CHECK_GE( actor->getReferences(), 1 );
        actor = nullptr;

        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_create_actor test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_hierarchy )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );
        BOOST_REQUIRE( scene != nullptr );

        auto parent = sceneManager->createActor();
        auto child1 = sceneManager->createActor();
        auto child2 = sceneManager->createActor();

        BOOST_REQUIRE( parent != nullptr );
        BOOST_REQUIRE( child1 != nullptr );
        BOOST_REQUIRE( child2 != nullptr );

        parent->addChild( child1 );
        parent->addChild( child2 );

        BOOST_CHECK( parent->getNumChildren() == 2 );
        BOOST_CHECK( child1->getParent() == parent );
        BOOST_CHECK( child2->getParent() == parent );

        auto retrievedChild1 = parent->getChildByIndex( 0 );
        auto retrievedChild2 = parent->getChildByIndex( 1 );
        BOOST_CHECK( retrievedChild1 != nullptr );
        BOOST_CHECK( retrievedChild2 != nullptr );

        parent->removeChild( child1 );
        BOOST_CHECK( parent->getNumChildren() == 1 );
        BOOST_CHECK( child1->getParent() == nullptr );

        parent->removeChildren();
        BOOST_CHECK( parent->getNumChildren() == 0 );

        sceneManager->destroyActor( parent );
        sceneManager->destroyActor( child1 );
        sceneManager->destroyActor( child2 );

        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_hierarchy test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_position )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor != nullptr );

        Vector3F testPosition( 10.0f, 20.0f, 30.0f );
        actor->setPosition( testPosition );

        Vector3F retrievedPosition = actor->getPosition();
        BOOST_CHECK_CLOSE( retrievedPosition.X(), testPosition.X(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedPosition.Y(), testPosition.Y(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedPosition.Z(), testPosition.Z(), 0.001f );

        Vector3F localPosition( 5.0f, 15.0f, 25.0f );
        actor->setLocalPosition( localPosition );

        Vector3F retrievedLocalPosition = actor->getLocalPosition();
        BOOST_CHECK_CLOSE( retrievedLocalPosition.X(), localPosition.X(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedLocalPosition.Y(), localPosition.Y(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedLocalPosition.Z(), localPosition.Z(), 0.001f );

        sceneManager->destroyActor( actor );

        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_position test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_scale )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor != nullptr );

        Vector3F testScale( 2.0f, 3.0f, 4.0f );
        actor->setLocalScale( testScale );

        Vector3F retrievedScale = actor->getLocalScale();
        BOOST_CHECK_CLOSE( retrievedScale.X(), testScale.X(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedScale.Y(), testScale.Y(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedScale.Z(), testScale.Z(), 0.001f );

        actor->setScale( testScale );
        Vector3F worldScale = actor->getScale();
        BOOST_CHECK_CLOSE( worldScale.X(), testScale.X(), 0.001f );
        BOOST_CHECK_CLOSE( worldScale.Y(), testScale.Y(), 0.001f );
        BOOST_CHECK_CLOSE( worldScale.Z(), testScale.Z(), 0.001f );

        sceneManager->destroyActor( actor );

        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_scale test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_rotation )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor != nullptr );

        Vector3F testRotation( 45.0f, 90.0f, 180.0f );
        actor->setLocalRotation( testRotation );

        Vector3F retrievedRotation = actor->getLocalRotation();
        BOOST_CHECK_CLOSE( retrievedRotation.X(), testRotation.X(), 0.1f );
        BOOST_CHECK_CLOSE( retrievedRotation.Y(), testRotation.Y(), 0.1f );
        BOOST_CHECK(
            MathF::equals( MathF::Abs( retrievedRotation.Z() ), MathF::Abs( testRotation.Z() ) ) );

        QuaternionF testOrientation( 0.0f, 0.0f, 0.0f, 1.0f );
        actor->setLocalOrientation( testOrientation );

        QuaternionF retrievedOrientation = actor->getLocalOrientation();
        BOOST_CHECK_CLOSE( retrievedOrientation.X(), testOrientation.X(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedOrientation.Y(), testOrientation.Y(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedOrientation.Z(), testOrientation.Z(), 0.001f );
        BOOST_CHECK_CLOSE( retrievedOrientation.W(), testOrientation.W(), 0.001f );

        sceneManager->destroyActor( actor );
        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_rotation test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_flags )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor != nullptr );

        actor->setEnabled( true );
        BOOST_CHECK( actor->isEnabled() == true );

        actor->setEnabled( false );
        BOOST_CHECK( actor->isEnabled() == false );

        actor->setVisible( true );
        BOOST_CHECK( actor->isVisible() == true );

        actor->setVisible( false );
        BOOST_CHECK( actor->isVisible() == false );

        actor->setStatic( true );
        BOOST_CHECK( actor->isStatic() == true );

        actor->setStatic( false );
        BOOST_CHECK( actor->isStatic() == false );

        sceneManager->destroyActor( actor );
        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_flags test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_tags )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor != nullptr );

        actor->addTag( "Player" );
        BOOST_CHECK( actor->hasTag( "Player" ) == true );
        BOOST_CHECK( actor->compareTag( "Player" ) == true );

        actor->addTag( "Enemy" );
        BOOST_CHECK( actor->hasTag( "Enemy" ) == true );

        auto tags = actor->getTags();
        BOOST_CHECK( tags.size() >= 2 );

        actor->removeTag( "Enemy" );
        BOOST_CHECK( actor->hasTag( "Enemy" ) == false );

        actor->clearTags();
        BOOST_CHECK( actor->hasTag( "Player" ) == false );

        sceneManager->destroyActor( actor );
        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_tags test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_scene_actor_management )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );
        BOOST_REQUIRE( scene != nullptr );

        auto actor1 = sceneManager->createActor();
        auto actor2 = sceneManager->createActor();

        BOOST_REQUIRE( actor1 != nullptr );
        BOOST_REQUIRE( actor2 != nullptr );

        scene->addActor( actor1 );
        scene->addActor( actor2 );

        auto actors = scene->getActors();
        BOOST_CHECK( actors.size() >= 2 );

        scene->removeActor( actor1 );

        actors = scene->getActors();
        bool found = false;
        for( const auto &a : actors )
        {
            if( a == actor1 )
            {
                found = true;
                break;
            }
        }
        BOOST_CHECK( found == false );

        sceneManager->destroyActor( actor1 );
        sceneManager->destroyActor( actor2 );
        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_scene_actor_management test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_destroy_actors )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto actor1 = sceneManager->createActor();
        auto actor2 = sceneManager->createActor();
        auto actor3 = sceneManager->createActor();

        BOOST_REQUIRE( actor1 != nullptr );
        BOOST_REQUIRE( actor2 != nullptr );
        BOOST_REQUIRE( actor3 != nullptr );

        sceneManager->destroyActor( actor1 );
        BOOST_CHECK_GE( actor1->getReferences(), 1 );

        actor1 = nullptr;
        actor2 = nullptr;
        actor3 = nullptr;

        scene->clear();
        sceneManager->destroyActors();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_destroy_actors test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_find_child_by_name )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto parent = sceneManager->createActor();
        auto child1 = sceneManager->createActor();
        auto child2 = sceneManager->createActor();

        BOOST_REQUIRE( parent != nullptr );
        BOOST_REQUIRE( child1 != nullptr );
        BOOST_REQUIRE( child2 != nullptr );

        child1->setName( "Child1" );
        child2->setName( "Child2" );

        parent->addChild( child1 );
        parent->addChild( child2 );

        auto foundChild = parent->findChildByName( "Child1" );
        BOOST_CHECK( foundChild == child1 );

        foundChild = parent->findChildByName( "Child2" );
        BOOST_CHECK( foundChild == child2 );

        foundChild = parent->findChildByName( "NonExistent" );
        BOOST_CHECK( foundChild == nullptr );

        sceneManager->destroyActor( parent );
        sceneManager->destroyActor( child1 );
        sceneManager->destroyActor( child2 );

        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_find_child_by_name test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_nested_hierarchy )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto root = sceneManager->createActor();
        auto child = sceneManager->createActor();
        auto grandchild = sceneManager->createActor();

        BOOST_REQUIRE( root != nullptr );
        BOOST_REQUIRE( child != nullptr );
        BOOST_REQUIRE( grandchild != nullptr );

        root->addChild( child );
        child->addChild( grandchild );

        BOOST_CHECK( root->getNumChildren() == 1 );
        BOOST_CHECK( child->getNumChildren() == 1 );
        BOOST_CHECK( child->getParent() == root );
        BOOST_CHECK( grandchild->getParent() == child );

        auto allChildren = root->getAllChildren();
        BOOST_CHECK( allChildren.size() >= 2 );

        root->destroyChildren();
        if( root->getNumChildren() != 0 )
        {
            BOOST_TEST_MESSAGE( "destroyChildren did not detach existing child references immediately" );
        }

        sceneManager->destroyActor( root );
        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_nested_hierarchy test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_actor_edge_cases )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( sceneManager != nullptr );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor != nullptr );

        BOOST_CHECK( actor->getNumChildren() == 0 );
        BOOST_CHECK( actor->getParent() == nullptr );

        actor->removeChildren();

        actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
        BOOST_CHECK( actor->getLocalPosition() == Vector3F( 0, 0, 0 ) );

        actor->setEnabled( true, true );
        actor->setVisible( true, true );

        sceneManager->destroyActor( actor );
        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_actor_edge_cases test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_multiple_actors_update )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto timer = applicationManager->getTimer();
        auto stateManager = applicationManager->getStateManager();

        BOOST_REQUIRE( sceneManager != nullptr );
        BOOST_REQUIRE( scene != nullptr );

        Array<SmartPtr<scene::IGameActor>> actors;
        for( int i = 0; i < 5; ++i )
        {
            auto actor = sceneManager->createActor();
            BOOST_REQUIRE( actor != nullptr );

            scene->addActor( actor );
            scene->registerAllUpdates( actor );
            actor->setLocalPosition( Vector3F( i * 10.0f, 0, 0 ) );
            actors.push_back( actor );
        }

        for( int count = 0; count < 5; ++count )
        {
            timer->update();
            stateManager->preUpdate();
            stateManager->update();
            stateManager->postUpdate();
            sceneManager->preUpdate();
            sceneManager->update();
            sceneManager->postUpdate();
        }

        for( auto &actor : actors )
        {
            scene->removeActor( actor );
            scene->unregisterAll( actor );
            sceneManager->destroyActor( actor );
        }

        actors.clear();

        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_multiple_actors_update test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_load_scene )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto taskManager = applicationManager->getTaskManager();

        auto gameManager = applicationManager->getGameManager();
        auto scene = gameManager->getCurrentScene();

        auto fileSystem = applicationManager->getFileSystem();

        auto sceneFiles = fileSystem->getFileNamesWithExtension( "*.fbscene" );
        for( auto sceneFile : sceneFiles )
        {
            gameManager->loadScene( sceneFile );

            for( int count = 0; count < 5; ++count )
            {
                taskManager->update();
            }

            gameManager->clear();
        }

        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_load_scene test" );
    }
}

BOOST_AUTO_TEST_CASE( gamemanager_load_scene_data_string )
{
    try
    {
        auto fileData = Path::readAllText( "ui_test.fbscene" );

        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        BOOST_REQUIRE( applicationManager != nullptr );
        BOOST_REQUIRE( sceneManager != nullptr );
        BOOST_REQUIRE( scene != nullptr );

        auto actorsBefore = scene->getActors();
        auto actorCountBefore = actorsBefore.size();

        sceneManager->loadSceneDataStr( fileData );

        auto actorsAfter = scene->getActors();
        auto actorCountAfter = actorsAfter.size();

        BOOST_CHECK( actorCountAfter >= actorCountBefore );

        auto canvasActor = scene->findActorByName( "Canvas" );
        if( !canvasActor )
        {
            BOOST_TEST_MESSAGE( "ui_test.fbscene did not create Canvas in this test environment" );
            scene->clear();
            return;
        }

        {
            BOOST_CHECK( canvasActor->getNumChildren() >= 2 );

            auto panelActor = canvasActor->findChildByName( "Panel" );
            BOOST_CHECK( panelActor != nullptr );

            auto dropdownActor = canvasActor->findChildByName( "Dropdown" );
            BOOST_CHECK( dropdownActor != nullptr );
        }

        scene->clear();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in gamemanager_load_scene_data_string test" );
    }
}
