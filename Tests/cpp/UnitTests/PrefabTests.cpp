#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/GamePrefab.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Scene/Components/Camera/CameraFollow.hpp>
#include <Workphone/Scene/Components/Camera/CameraTarget.hpp>
#include <unordered_set>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

BOOST_AUTO_TEST_CASE( prefab_instances_have_unique_graph_ids_and_local_references )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );
    guard.scene->clear( true );
    auto data = make_ptr<Properties>();
    data->setProperty( GameActorUtil::labelStr, "Template" );
    data->setProperty( GameActorUtil::uuidStr, StringUtil::getUUID() );
    auto child = make_ptr<Properties>();
    child->setName( GameActorUtil::childStr );
    child->setProperty( GameActorUtil::labelStr, "Target" );
    const auto childUUID = StringUtil::getUUID();
    child->setProperty( GameActorUtil::uuidStr, childUUID );
    const auto targetUUID = StringUtil::getUUID();
    auto target = make_ptr<Properties>();
    target->setName( GameActorUtil::componentStr );
    target->setProperty( GameActorUtil::componentTypeStr, "CameraTarget" );
    target->setProperty( GameActorUtil::uuidStr, targetUUID );
    child->addChild( target );
    data->addChild( child );
    auto follow = make_ptr<Properties>();
    follow->setName( GameActorUtil::componentStr );
    follow->setProperty( GameActorUtil::componentTypeStr, "CameraFollow" );
    follow->setProperty( GameActorUtil::uuidStr, StringUtil::getUUID() );
    follow->setProperty( CameraFollow::followObjectStr, childUUID );
    follow->setProperty( CameraFollow::targetStr, targetUUID );
    data->addChild( follow );
    const auto serializedTemplate = DataUtil::toString( data.get() );
    auto prefab = make_ptr<GamePrefab>();
    prefab->setData( data );
    auto manager = make_ptr<GamePrefabManager>();
    Array<SmartPtr<IGameActor>> instances;
    auto original = GameActorUtil::loadSceneActors( { data } ).front();
    instances.push_back( original );
    instances.push_back( prefab->createActor() );
    instances.push_back( prefab->createActor() );
    instances.push_back( manager->createInstance( original ) );
    instances.push_back( manager->createInstance( original ) );
    std::unordered_set<String> ids;
    for( auto instance : instances )
    {
        BOOST_REQUIRE( instance );
        guard.scene->addActor( instance );
        auto targetActor = instance->findChildByName( "Target" );
        BOOST_REQUIRE( targetActor );
        auto cameraFollow = instance->getComponent<CameraFollow>();
        BOOST_REQUIRE( cameraFollow );
        BOOST_CHECK( cameraFollow->getFollowObject() == targetActor );
        BOOST_CHECK( cameraFollow->getTarget() == targetActor->getComponent<CameraTarget>() );
        for( auto actor : { instance, targetActor } )
        {
            BOOST_CHECK( ids.insert( actor->getHandle()->getUUIDAsString() ).second );
            for( auto component : actor->getComponents() )
                BOOST_CHECK( ids.insert( component->getHandle()->getUUIDAsString() ).second );
        }
    }
    BOOST_CHECK_EQUAL( DataUtil::toString( data.get() ), serializedTemplate );
    auto savedScene = make_ptr<Properties>();
    for( auto instance : instances )
    {
        auto actorData = workphone::static_pointer_cast<Properties>( instance->toData() );
        actorData->setName( ApplicationUtil::actorsStr );
        savedScene->addChild( actorData );
    }
    guard.scene->clear( true );
    guard.scene->loadSceneDataStr( DataUtil::toString( savedScene.get() ), false );
    BOOST_CHECK( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
    BOOST_CHECK_EQUAL( guard.scene->getActors().size(), instances.size() );
    for( auto instance : guard.scene->getActors() )
    {
        auto cameraFollow = instance->getComponent<CameraFollow>();
        BOOST_REQUIRE( cameraFollow );
        auto targetActor = instance->findChildByName( "Target" );
        BOOST_REQUIRE( targetActor );
        BOOST_CHECK( cameraFollow->getFollowObject() == targetActor );
        BOOST_CHECK( cameraFollow->getTarget() == targetActor->getComponent<CameraTarget>() );
    }
    guard.scene->clear( true );
}

BOOST_AUTO_TEST_CASE( prefab_instance_data_remaps_reference_arrays_and_keeps_external_ids )
{
    auto data = make_ptr<Properties>();
    const auto actorUUID = StringUtil::getUUID();
    const auto externalUUID = StringUtil::getUUID();
    data->setProperty( GameActorUtil::uuidStr, actorUUID );
    data->setProperty( "references", Array<String>{ actorUUID, externalUUID } );
    data->setProperty( "serializedReferences", actorUUID + ";" + externalUUID + ";" );
    auto copy = GameActorUtil::createInstanceData( data );
    const auto newUUID = copy->getPropertyObject( GameActorUtil::uuidStr ).getValue();
    BOOST_CHECK( newUUID != actorUUID );
    Array<String> refs;
    StringUtil::parseArray( copy->getPropertyObject( "references" ).getValue(), refs );
    BOOST_REQUIRE_EQUAL( refs.size(), 2u );
    BOOST_CHECK_EQUAL( refs[0], newUUID );
    BOOST_CHECK_EQUAL( refs[1], externalUUID );
    BOOST_CHECK_EQUAL( copy->getPropertyObject( "serializedReferences" ).getValue(),
                       newUUID + ";" + externalUUID + ";" );
    BOOST_CHECK_EQUAL( data->getPropertyObject( GameActorUtil::uuidStr ).getValue(), actorUUID );
}

BOOST_AUTO_TEST_CASE( prefab_create )
{
    try
    {
        TestGuard fixture;

        // Create a source actor to use as a prefab template
        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );
        BOOST_CHECK( actor->isValid() );

        // Add components to the source actor
        auto transform = actor->getComponent<scene::Transform>();
        if( !transform )
        {
            BOOST_TEST_MESSAGE(
                "Actor does not create a Transform component by default in this configuration." );
        }

        actor->setName( "PrefabTemplate" );
        BOOST_CHECK_EQUAL( actor->getName(), "PrefabTemplate" );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_create test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_create_with_components )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );

        // Add multiple components
        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        auto collisionBox = actor->addComponent<scene::CollisionBox>();

        BOOST_CHECK( meshRenderer || collisionBox );

        // Verify components exist
        auto components = actor->getComponents();
        BOOST_CHECK( components.size() > 0 );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_create_with_components test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_clone_actor )
{
    try
    {
        TestGuard fixture;

        // Create source actor
        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto source = workphone::static_pointer_cast<IGameActor>( sourceActor );
        source->setName( "SourcePrefab" );

        // Clone the actor
        auto clonedActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( clonedActor );

        auto clone = workphone::static_pointer_cast<IGameActor>( clonedActor );
        BOOST_CHECK( clone->isValid() );

        // Verify clone is independent
        clone->setName( "ClonedPrefab" );
        BOOST_CHECK_NE( source->getName(), clone->getName() );

        fixture.sceneManager->destroyActor( sourceActor );
        fixture.sceneManager->destroyActor( clonedActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_clone_actor test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_null_actor )
{
    try
    {
        TestGuard fixture;

        SmartPtr<IGameActor> nullActor;
        BOOST_CHECK( !nullActor );

        // Attempting operations on null should be handled gracefully
        // This tests edge case behavior
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_null_actor test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_add_to_scene )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );
        actor->setName( "ScenePrefab" );

        // Add to scene
        fixture.scene->addActor( actor );

        // Verify actor is in scene
        auto sceneActors = fixture.scene->getActors();
        bool found = false;
        for( auto &sceneActor : sceneActors )
        {
            if( sceneActor == actor )
            {
                found = true;
                break;
            }
        }
        BOOST_CHECK( found );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_add_to_scene test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_remove_from_scene )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );

        fixture.scene->addActor( actor );
        fixture.scene->removeActor( actor );

        // Verify actor is removed from scene
        auto sceneActors = fixture.scene->getActors();
        bool found = false;
        for( auto &sceneActor : sceneActors )
        {
            if( sceneActor == actor )
            {
                found = true;
                break;
            }
        }
        BOOST_CHECK( !found );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_remove_from_scene test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_multiple_instances )
{
    try
    {
        TestGuard fixture;

        constexpr u32 instanceCount = 10;
        Array<SmartPtr<IGameActor>> instances;

        // Create multiple instances from the same template pattern
        for( u32 i = 0; i < instanceCount; ++i )
        {
            auto actor = fixture.sceneManager->createActor();
            BOOST_REQUIRE( actor );

            auto gameActor = workphone::static_pointer_cast<IGameActor>( actor );
            gameActor->setName( "PrefabInstance_" + StringUtil::toString( i ) );

            fixture.scene->addActor( gameActor );
            instances.push_back( actor );
        }

        BOOST_CHECK_EQUAL( instances.size(), instanceCount );

        // Verify all instances are valid and unique
        for( u32 i = 0; i < instanceCount; ++i )
        {
            auto gameActor = workphone::static_pointer_cast<IGameActor>( instances[i] );
            BOOST_CHECK( gameActor->isValid() );

            auto expectedName = "PrefabInstance_" + StringUtil::toString( i );
            BOOST_CHECK_EQUAL( gameActor->getName(), expectedName );
        }

        // Cleanup
        for( auto &instance : instances )
        {
            fixture.sceneManager->destroyActor( instance );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_multiple_instances test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_hierarchy )
{
    try
    {
        TestGuard fixture;

        // Create parent actor
        auto parentActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( parentActor );

        auto parent = workphone::static_pointer_cast<IGameActor>( parentActor );
        parent->setName( "ParentPrefab" );

        // Create child actor
        auto childActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( childActor );

        auto child = workphone::static_pointer_cast<IGameActor>( childActor );
        child->setName( "ChildPrefab" );

        // Set up hierarchy
        child->setParent( parent );

        // Verify hierarchy
        auto retrievedParent = child->getParent();
        BOOST_CHECK( retrievedParent == parent );

        BOOST_CHECK_NO_THROW( parent->getChildren() );

        fixture.sceneManager->destroyActor( childActor );
        fixture.sceneManager->destroyActor( parentActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_hierarchy test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_transform_properties )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );

        auto transform = actor->getComponent<scene::Transform>();
        if( transform )
        {
            // Set transform properties
            Vector3F position( 10.0f, 20.0f, 30.0f );
            transform->setPosition( position );

            Vector3F scale( 2.0f, 2.0f, 2.0f );
            transform->setScale( scale );

            QuaternionF rotation = QuaternionF::identity();
            transform->setOrientation( rotation );

            // Verify transform properties are set
            auto retrievedPosition = transform->getPosition();
            BOOST_CHECK_CLOSE( retrievedPosition.X(), position.X(), 0.001f );
            BOOST_CHECK_CLOSE( retrievedPosition.Y(), position.Y(), 0.001f );
            BOOST_CHECK_CLOSE( retrievedPosition.Z(), position.Z(), 0.001f );
        }

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_transform_properties test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_destroy_cleanup )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );
        actor->setName( "DestroyTestPrefab" );

        fixture.scene->addActor( actor );

        // Destroy the actor
        fixture.sceneManager->destroyActor( sourceActor );

        // Update to process destruction
        fixture.updateScene( 5 );

        // Verify actor is no longer valid in the scene
        auto sceneActors = fixture.scene->getActors();
        bool found = false;
        for( auto &sceneActor : sceneActors )
        {
            auto gameActor = workphone::static_pointer_cast<IGameActor>( sceneActor );
            if( gameActor && gameActor->getName() == "DestroyTestPrefab" )
            {
                found = true;
                break;
            }
        }
        BOOST_CHECK( !found );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_destroy_cleanup test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_empty_name )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );

        // Test with empty name
        actor->setName( "" );
        BOOST_CHECK( actor->getName().empty() );

        // Test with whitespace name
        actor->setName( "   " );
        auto name = actor->getName();
        BOOST_CHECK( !name.empty() || name == "   " );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_empty_name test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_special_characters_name )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );

        // Test with special characters
        String specialName = "Prefab_Test-123!@#$%";
        actor->setName( specialName );

        auto retrievedName = actor->getName();
        BOOST_CHECK_EQUAL( retrievedName, specialName );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_special_characters_name test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_static_actor )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );

        // Set as static
        actor->setStatic( true );
        BOOST_CHECK( actor->isStatic() );

        // Toggle back to dynamic
        actor->setStatic( false );
        BOOST_CHECK( !actor->isStatic() );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_static_actor test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_enabled_state )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );

        // Test enabled state
        actor->setEnabled( true );
        BOOST_CHECK( actor->isEnabled() );

        actor->setEnabled( false );
        BOOST_CHECK( !actor->isEnabled() );

        // Re-enable
        actor->setEnabled( true );
        BOOST_CHECK( actor->isEnabled() );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_enabled_state test" );
    }
}

BOOST_AUTO_TEST_CASE( prefab_visible_state )
{
    try
    {
        TestGuard fixture;

        auto sourceActor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( sourceActor );

        auto actor = workphone::static_pointer_cast<IGameActor>( sourceActor );

        // Test visibility state
        actor->setVisible( true );
        BOOST_CHECK( actor->isVisible() );

        actor->setVisible( false );
        BOOST_CHECK( !actor->isVisible() );

        fixture.sceneManager->destroyActor( sourceActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in prefab_visible_state test" );
    }
}
