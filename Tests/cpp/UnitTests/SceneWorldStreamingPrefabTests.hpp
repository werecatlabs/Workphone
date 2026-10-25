#pragma once
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Scene/SceneWorldStreamer.hpp>
#include <Workphone/Scene/Components/WorldStreaming.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <chrono>
#include <thread>
#include <fstream>

namespace wpStreamingPrefabTest
{
    using namespace workphone;
    using namespace workphone::scene;
    inline SmartPtr<Properties> actor( const String &name, float x )
    {
        auto data = make_ptr<Properties>(); data->setName( "actors" );
        data->setProperty( "label", name ); data->setProperty( "uuid", StringUtil::getUUID() );
        data->setProperty( "static", true );
        auto pose = make_ptr<Properties>(); pose->setName( "localTransform" );
        pose->setProperty( "Position", Vector3F( x, 0, 0 ) ); data->addChild( pose );
        auto mesh = make_ptr<Properties>(); mesh->setName( "component" );
        mesh->setProperty( "uuid", StringUtil::getUUID() );
        mesh->setProperty( "componentType", "workphone::scene::Mesh" ); data->addChild( mesh );
        return data;
    }
    inline String directory( TestGuard &guard )
    {
        auto path = ( std::filesystem::temp_directory_path() /
            ( "wp-stream-instance-" + std::string( StringUtil::getUUID().c_str() ) ) ).generic_string();
        guard.trackFilesystemPath( path.c_str() ); return path.c_str();
    }
    inline void pumpInstance( SceneWorldStreamer &stream, const Vector3F &position, u32 count )
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
        String error;
        do
        {
            BOOST_REQUIRE_MESSAGE( stream.update( { position }, error ), error );
            const auto stats = stream.statistics();
            if( stats.residentActors == count && !stats.pendingCells ) return;
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        } while( std::chrono::steady_clock::now() < deadline );
        BOOST_FAIL( "Streaming prefab residency did not converge" );
    }
    inline SmartPtr<IGameActor> owner( TestGuard &guard, float x )
    {
        auto value = guard.createBasicActor(); value->getHandle()->setUUID( StringUtil::getUUID() );
        // Local pose publication is intentionally gated by actor mode. Tests
        // construct these owners directly rather than through Editor insertion,
        // so complete that lifecycle transition before setting their placement.
        const auto task = Thread::getCurrentTask();
        const auto flags = Thread::getTaskFlags( task );
        // clear(true) cancels the previous scene load. Publishing the first
        // authored root restores Loaded, which setState requires before it can
        // make any FSM transition.
        guard.scene->addActor( value );
        BOOST_REQUIRE( guard.scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
        Thread::setTaskFlags( task, flags | Thread::Application_Flag );
        value->setState( IGameActor::State::Edit, true );
        Thread::setTaskFlags( task, flags );
        BOOST_REQUIRE( value->getState() == IGameActor::State::Edit );
        value->setName( "World instance" ); value->setLocalPosition( Vector3<real_Num>( x, 0, 0 ) );
        BOOST_REQUIRE( value->getWorldTransform().getPosition().equals( Vector3<real_Num>( x, 0, 0 ), real_Num( 0.001 ) ) );
        return value;
    }
    inline SmartPtr<IGameActor> find( const SmartPtr<IGameActor> &root, const String &name )
    {
        if( root->getName() == name ) return root;
        for( const auto &child : root->getChildren() )
            if( auto found = find( child, name ) ) return found;
        return nullptr;
    }
    class OwnerPublicationProbe : public Component
    {
    public:
        Parameter handleEvent( EventType type, hash_type value, const Array<Parameter> &arguments,
                               SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                               SmartPtr<IEvent> event ) override
        {
            if( value == IComponent::childAddedInHierarchy && !called )
            { called = true; if( callback ) callback(); }
            return Parameter();
        }
        std::function<void()> callback;
        bool called = false;
    };
    class PersistentUpdateProbe : public Component
    {
    public:
        void load( SmartPtr<ISharedObject> data ) override
        { Component::load( data ); setLoadingState( LoadingState::Loaded ); }
        void preUpdate() override { ++preUpdates; }
        void update() override { ++updates; }
        void postUpdate() override { ++postUpdates; }
        FSMReturnType handleComponentEvent( u32 state, FSMEvent event ) override
        {
            const auto result = Component::handleComponentEvent( state, event );
            if( event == FSMEvent::Enter && state == static_cast<u32>( IComponent::State::Play ) )
            {
                auto callback = std::move( onEnterPlay );
                if( callback ) callback();
            }
            return result;
        }
        void unload( SmartPtr<ISharedObject> data ) override
        {
            if( getLoadingState() == LoadingState::Unloaded ) return;
            ++unloads;
            auto callback = std::move( onUnload );
            if( callback ) callback();
            Component::unload( data ); setLoadingState( LoadingState::Unloaded );
        }
        u32 preUpdates = 0, updates = 0, postUpdates = 0, unloads = 0;
        std::function<void()> onUnload;
        std::function<void()> onEnterPlay;
    };
}

BOOST_AUTO_TEST_SUITE( SceneWorldStreamingPrefabTests )

BOOST_AUTO_TEST_CASE( streaming_prefab_options_are_opt_in_and_round_trip )
{
    using namespace wpStreamingPrefabTest;
    TestGuard guard;
    auto component = make_ptr<WorldStreaming>();
    auto properties = component->getProperties();
    bool value = true;
    BOOST_REQUIRE( properties->getPropertyValue( "Instance World", value ) ); BOOST_CHECK( !value );
    value = true;
    BOOST_REQUIRE( properties->getPropertyValue( "Follow Active Camera", value ) ); BOOST_CHECK( !value );
    properties->setProperty( "Instance World", true ); properties->setProperty( "Follow Active Camera", true );
    component->setProperties( properties ); properties = component->getProperties();
    value = false; properties->getPropertyValue( "Instance World", value ); BOOST_CHECK( value );
    value = false; properties->getPropertyValue( "Follow Active Camera", value ); BOOST_CHECK( value );
}

BOOST_AUTO_TEST_CASE( duplicate_streaming_prefabs_preserve_placement_and_internal_component_references )
{
    using namespace wpStreamingPrefabTest;
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>(); auto root = actor( "Instanced root", 10 );
    auto child = actor( "Instanced child", 5 ); child->setName( "child" ); root->addChild( child );
    auto renderer = make_ptr<Properties>(); renderer->setName( "component" );
    const auto rendererId = StringUtil::getUUID(); renderer->setProperty( "uuid", rendererId );
    renderer->setProperty( "componentType", "workphone::scene::MeshRenderer" ); child->addChild( renderer );
    auto lod = make_ptr<Properties>(); lod->setName( "component" );
    lod->setProperty( "uuid", StringUtil::getUUID() ); lod->setProperty( "componentType", "workphone::scene::LODGroup" );
    auto level = make_ptr<Properties>(); level->setName( "lodLevel" );
    level->setProperty( "screenRelativeHeight", 0.1f );
    level->setProperty( "renderers", Array<String>{ rendererId } );
    lod->addChild( level ); root->addChild( lod ); data->addChild( root );
    String manifest, error; BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, directory( guard ), {}, manifest, error ), error );
    auto first = owner( guard, 1000 ), second = owner( guard, -2000 );
    first->setLocalScale( Vector3<real_Num>( 2, 3, 4 ) );
    first->setLocalOrientation( Quaternion<real_Num>::angleAxis( Math<real_Num>::pi() / real_Num( 2 ), Vector3<real_Num>::unitY() ) );
    BOOST_REQUIRE( first->getWorldTransform().getPosition().equals( Vector3<real_Num>( 1000, 0, 0 ), real_Num( 0.001 ) ) );
    BOOST_REQUIRE( first->getWorldTransform().getScale().equals( Vector3<real_Num>( 2, 3, 4 ), real_Num( 0.001 ) ) );
    auto scheduler = std::make_shared<WorldStreamingScheduler>();
    SceneWorldStreamer a( scheduler ), b( scheduler );
    BOOST_REQUIRE_MESSAGE( a.open( manifest, guard.scene, {}, first, error ), error );
    BOOST_REQUIRE_MESSAGE( b.open( manifest, guard.scene, {}, second, error ), error );
    pumpInstance( a, Vector3F( 1000, 0, 0 ), 2 ); pumpInstance( b, Vector3F( -2000, 0, 0 ), 2 );
    BOOST_REQUIRE_EQUAL( first->getChildren().size(), 1u ); BOOST_REQUIRE_EQUAL( second->getChildren().size(), 1u );
    auto ar = first->getChildren().front(), br = second->getChildren().front();
    BOOST_CHECK( ar->getParent() == first ); BOOST_CHECK( br->getParent() == second );
    BOOST_CHECK( ar->getHandle()->getUUIDAsString() != br->getHandle()->getUUIDAsString() );
    BOOST_CHECK( ar->getLocalPosition().equals( Vector3<real_Num>( 10, 0, 0 ), real_Num( 0.001 ) ) );
    const auto placement = first->getWorldTransform();
    const auto expected = placement.getPosition() + placement.getOrientation() *
        ( placement.getScale() * Vector3<real_Num>( 15, 0, 0 ) );
    auto ac = ar->getChildren().front(), bc = br->getChildren().front();
    BOOST_CHECK( ac->getWorldTransform().getPosition().equals( expected, real_Num( 0.001 ) ) );
    auto aLod = ar->getComponent<LODGroup>(), bLod = br->getComponent<LODGroup>();
    BOOST_REQUIRE( aLod ); BOOST_REQUIRE( bLod );
    BOOST_REQUIRE_EQUAL( aLod->getLevels().size(), 1u ); BOOST_REQUIRE_EQUAL( bLod->getLevels().size(), 1u );
    BOOST_REQUIRE_EQUAL( aLod->getLevels().front().renderers.size(), 1u );
    BOOST_REQUIRE_EQUAL( bLod->getLevels().front().renderers.size(), 1u );
    BOOST_CHECK( aLod->getLevels().front().renderers.front() == ac->getComponent<MeshRenderer>() );
    BOOST_CHECK( bLod->getLevels().front().renderers.front() == bc->getComponent<MeshRenderer>() );
    BOOST_CHECK( a.pinActor( ac->getHandle()->getUUIDAsString() ) );
    BOOST_CHECK( !b.pinActor( ac->getHandle()->getUUIDAsString() ) );
    auto saved = dynamic_pointer_cast<Properties>( guard.scene->toData() ); BOOST_REQUIRE( saved );
    BOOST_REQUIRE_EQUAL( saved->getChildrenByName( "actors" ).size(), 2u );
    for( const auto &savedOwner : saved->getChildrenByName( "actors" ) )
        BOOST_CHECK( savedOwner->getChildrenByName( "child" ).empty() );
    a.close(); BOOST_CHECK( first->getChildren().empty() ); BOOST_CHECK_EQUAL( second->getChildren().size(), 1u );
    b.close(); BOOST_CHECK( second->getChildren().empty() );
}

BOOST_AUTO_TEST_CASE( moving_streaming_prefab_does_not_create_an_edit_overlay_and_ids_survive_eviction )
{
    using namespace wpStreamingPrefabTest;
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>(); data->addChild( actor( "Disposable building", 10 ) );
    String manifest, error; BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, directory( guard ), {}, manifest, error ), error );
    auto instance = owner( guard, 1000 ); auto scheduler = std::make_shared<WorldStreamingScheduler>();
    SceneWorldStreamer stream( scheduler ); SceneWorldStreamOptions options;
    options.loadRadius = 1; options.retainRadius = 2; options.minimumResidencySeconds = 0;
    BOOST_REQUIRE_MESSAGE( stream.open( manifest, guard.scene, options, instance, error ), error );
    const auto manifestBytes = scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )];
    pumpInstance( stream, Vector3F( 1010, 0, 0 ), 1 );
    BOOST_REQUIRE_EQUAL( instance->getChildren().size(), 1u );
    const auto identity = instance->getChildren().front()->getHandle()->getUUIDAsString();
    instance->setLocalPosition( Vector3<real_Num>( 5000, 0, 0 ) );
    pumpInstance( stream, Vector3F( 100000, 0, 0 ), 0 );
    // Retirement acknowledgement is collected at the start of the following
    // owner update, after the cell leaves the resident inventory.
    BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( 100000, 0, 0 ) }, error ), error );
    BOOST_CHECK_EQUAL( scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )], manifestBytes );
    pumpInstance( stream, Vector3F( 5010, 0, 0 ), 1 );
    BOOST_REQUIRE_EQUAL( instance->getChildren().size(), 1u );
    auto building = instance->getChildren().front();
    BOOST_CHECK_EQUAL( building->getHandle()->getUUIDAsString(), identity );
    BOOST_CHECK_CLOSE( building->getWorldTransform().getPosition().x, real_Num( 5010 ), 0.001 );
    building->setLocalPosition( Vector3<real_Num>( 12, 0, 0 ) );
    pumpInstance( stream, Vector3F( 100000, 0, 0 ), 0 ); pumpInstance( stream, Vector3F( 5010, 0, 0 ), 1 );
    BOOST_CHECK_EQUAL( instance->getChildren().front()->getLocalPosition().x, real_Num( 12 ) );
    stream.close(); BOOST_CHECK( instance->getChildren().empty() );
}

BOOST_AUTO_TEST_CASE( generated_streaming_component_uses_active_camera_but_explicit_observer_wins )
{
    using namespace wpStreamingPrefabTest;
    TestGuard guard; guard.scene->clear( true );
    const auto oldTask = Thread::getCurrentTask(); const auto oldEditor = guard.applicationManager->isEditor();
    guard.addCleanup( [app = guard.applicationManager, oldTask, oldEditor]() mutable {
        Thread::setCurrentTask( oldTask ); app->setEditor( oldEditor ); } );
    guard.applicationManager->setEditor( false ); Thread::setCurrentTask( guard.sceneManager->getSceneTask() );
    auto data = make_ptr<Properties>(); data->addChild( actor( "Camera district", 1000 ) );
    data->addChild( actor( "Observer district", 2000 ) );
    String manifest, error; BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, directory( guard ), {}, manifest, error ), error );
    auto instance = owner( guard, 0 ); auto cameraActor = owner( guard, 1000 );
    auto camera = cameraActor->addComponent<Camera>(); camera->setActive( true );
    auto observer = owner( guard, 2000 ); auto component = instance->addComponent<WorldStreaming>();
    auto settings = component->getProperties(); settings->setProperty( "World Manifest", manifest );
    settings->setProperty( "Instance World", true ); settings->setProperty( "Follow Active Camera", true );
    settings->setProperty( "Load Radius", 1.0f ); settings->setProperty( "Retain Radius", 2.0f );
    component->setProperties( settings );
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    while( !find( instance, "Camera district" ) && std::chrono::steady_clock::now() < deadline )
    { component->preUpdate(); std::this_thread::yield(); }
    BOOST_REQUIRE( find( instance, "Camera district" ) );
    BOOST_CHECK( !find( instance, "Observer district" ) );
    settings = component->getProperties(); settings->setPropertyAsType( "Observer", observer ); component->setProperties( settings );
    BOOST_REQUIRE_EQUAL( component->getProperties()->getPropertyObject( "Observer" ).getValue(),
                         observer->getHandle()->getUUIDAsString() );
    const auto observerDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    while( !find( instance, "Observer district" ) && std::chrono::steady_clock::now() < observerDeadline )
    { component->preUpdate(); std::this_thread::yield(); }
    BOOST_REQUIRE( find( instance, "Observer district" ) );
    settings = component->getProperties();
    settings->setPropertyAsType( "Observer", SmartPtr<IGameActor>() ); component->setProperties( settings );
    BOOST_CHECK( component->getProperties()->getPropertyObject( "Observer" ).getValue().empty() );
    const auto cameraDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    while( !find( instance, "Camera district" ) && std::chrono::steady_clock::now() < cameraDeadline )
    { component->preUpdate(); std::this_thread::yield(); }
    BOOST_REQUIRE( find( instance, "Camera district" ) );
    guard.sceneManager->destroyActor( instance );
    BOOST_CHECK( !find( instance, "Camera district" ) );
    BOOST_CHECK( !find( instance, "Observer district" ) );
}

BOOST_AUTO_TEST_CASE( streaming_prefab_radii_remain_world_units_when_the_instance_is_scaled_down )
{
    using namespace wpStreamingPrefabTest;
    TestGuard guard; guard.scene->clear( true );
    auto data = make_ptr<Properties>(); data->addChild( actor( "Scaled district", 100 ) );
    WorldCookOptions cook; cook.cellSize = 32;
    String manifest, error; BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, directory( guard ), cook, manifest, error ), error );
    auto instance = owner( guard, 1000 ); instance->setLocalScale( Vector3<real_Num>( real_Num( 0.1 ), 2, 3 ) );
    instance->setLocalOrientation( Quaternion<real_Num>::angleAxis( Math<real_Num>::pi() / real_Num( 2 ), Vector3<real_Num>::unitY() ) );
    SceneWorldStreamer stream; SceneWorldStreamOptions options;
    options.loadRadius = 11; options.retainRadius = 12; options.minimumResidencySeconds = 0;
    BOOST_REQUIRE_MESSAGE( stream.open( manifest, guard.scene, options, instance, error ), error );
    // The source point is 100 asset units away, but only 10 world units away.
    pumpInstance( stream, Vector3F( 1000, 0, 0 ), 1 );
    BOOST_REQUIRE_EQUAL( instance->getChildren().size(), 1u );
    pumpInstance( stream, Vector3F( 100000, 0, 0 ), 0 );
    stream.close();
}

BOOST_AUTO_TEST_CASE( streaming_prefab_unload_during_publication_cannot_reenter_or_leave_children )
{
    using namespace wpStreamingPrefabTest;
    TestGuard guard; guard.scene->clear( true );
    const auto oldTask = Thread::getCurrentTask();
    guard.addCleanup( [oldTask] { Thread::setCurrentTask( oldTask ); } );
    Thread::setCurrentTask( guard.sceneManager->getSceneTask() );
    auto data = make_ptr<Properties>(); data->addChild( actor( "Reentrant district", 0 ) );
    String manifest, error; BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, directory( guard ), {}, manifest, error ), error );
    auto instance = owner( guard, 0 ); instance->setState( IGameActor::State::Edit, true );
    auto component = instance->addComponent<WorldStreaming>();
    auto probe = make_ptr<OwnerPublicationProbe>(); instance->addComponentInstance( probe ); probe->load( nullptr );
    probe->callback = [component]() mutable { component->preUpdate(); component->unload( nullptr ); };
    auto settings = component->getProperties(); settings->setProperty( "World Manifest", manifest );
    settings->setProperty( "Instance World", true ); component->setProperties( settings );
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    while( !probe->called && std::chrono::steady_clock::now() < deadline )
    { component->preUpdate(); std::this_thread::yield(); }
    probe->callback = {};
    BOOST_REQUIRE( probe->called );
    BOOST_CHECK( component->getLoadingState() == LoadingState::Unloaded );
    BOOST_CHECK_EQUAL( component->statistics().residentActors, 0u );
    BOOST_CHECK( instance->getChildren().empty() );
    BOOST_CHECK( !find( instance, "Reentrant district" ) );
}

BOOST_AUTO_TEST_CASE( saving_a_live_streaming_prefab_keeps_resident_cells_out_of_its_cached_document )
{
    using namespace wpStreamingPrefabTest;
    TestGuard guard; guard.scene->clear( true );
    const auto oldTask = Thread::getCurrentTask();
    const auto oldCache = guard.applicationManager->getCachePath();
    guard.addCleanup( [app = guard.applicationManager, oldTask, oldCache]() mutable {
        Thread::setCurrentTask( oldTask ); app->setCachePath( oldCache ); } );
    Thread::setCurrentTask( guard.sceneManager->getSceneTask() );
    const auto output = directory( guard );
    auto source = make_ptr<Properties>(); source->addChild( actor( "Saved streamed district", 0 ) );
    String manifest, error; BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *source, output, {}, manifest, error ), error );
    auto instance = owner( guard, 0 ); auto component = instance->addComponent<WorldStreaming>();
    auto settings = component->getProperties(); settings->setProperty( "World Manifest", manifest );
    settings->setProperty( "Instance World", true ); component->setProperties( settings );
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
    while( component->statistics().residentActors != 1 && std::chrono::steady_clock::now() < deadline )
    { component->preUpdate(); std::this_thread::yield(); }
    BOOST_REQUIRE_EQUAL( instance->getChildren().size(), 1u );
    auto resident = instance->getChildren().front();
    auto prefabs = guard.applicationManager->getPrefabManager(); BOOST_REQUIRE( prefabs );
    const auto savedPath = output + "/saved.prefab";
    prefabs->savePrefab( savedPath, instance );
    BOOST_CHECK( resident->getFlag( IGameActor::ActorFlagDontSave ) );
    auto saved = prefabs->loadPrefab( savedPath ); BOOST_REQUIRE( saved );
    auto document = dynamic_pointer_cast<Properties>( saved->getData() ); BOOST_REQUIRE( document );
    BOOST_CHECK( document->getChildrenByName( "child" ).empty() );
    bool marker = false; document->getPropertyValue( "meshImportStreaming", marker ); BOOST_CHECK( marker );
    BOOST_REQUIRE_EQUAL( document->getChildrenByName( "component" ).size(), 1u );

    // A wrapper serialized by actor->toData() has no custom import marker.
    // Source-asset loading must still recognize its configured world component.
    document->removeProperty( "meshImportStreaming" );
    guard.applicationManager->setCachePath( output + "/" );
    const auto sourcePath = output + "/markerless.obj";
    const auto cachedPath = ApplicationUtil::getPrefabPath( sourcePath );
    const auto text = DataUtil::toString( document.get(), true, DataFormat::XML );
    std::ofstream cached( cachedPath.c_str(), std::ios::binary | std::ios::trunc );
    cached.write( text.c_str(), text.size() ); cached.close(); BOOST_REQUIRE( cached );
    auto markerless = prefabs->loadPrefab( sourcePath ); BOOST_REQUIRE( markerless );
    auto thin = dynamic_pointer_cast<Properties>( markerless->getData() ); BOOST_REQUIRE( thin );
    BOOST_CHECK( thin->getChildrenByName( "child" ).empty() );
    BOOST_REQUIRE_EQUAL( thin->getChildrenByName( "component" ).size(), 1u );
    auto restored = markerless->createActor(); BOOST_REQUIRE( restored );
    BOOST_CHECK( restored->getComponent<WorldStreaming>() );
    guard.sceneManager->destroyActor( restored );

    // Saving a larger authored prefab must respect a streaming owner below it.
    auto parent = owner( guard, 0 ); parent->setName( "Ordinary prefab parent" );
    guard.scene->removeActor( instance ); parent->addChild( instance );
    auto ordinary = guard.createBasicActor(); ordinary->setName( "Ordinary hidden child" );
    ordinary->setFlag( IGameActor::ActorFlagDontSave, true ); parent->addChild( ordinary );
    const auto parentPath = output + "/parent.prefab"; prefabs->savePrefab( parentPath, parent );
    BOOST_CHECK( resident->getFlag( IGameActor::ActorFlagDontSave ) );
    BOOST_CHECK( !ordinary->getFlag( IGameActor::ActorFlagDontSave ) );
    auto savedParent = prefabs->loadPrefab( parentPath ); BOOST_REQUIRE( savedParent );
    auto parentData = dynamic_pointer_cast<Properties>( savedParent->getData() ); BOOST_REQUIRE( parentData );
    BOOST_REQUIRE_EQUAL( parentData->getChildrenByName( "child" ).size(), 2u );
    for( const auto &child : parentData->getChildrenByName( "child" ) )
        BOOST_CHECK( child->getChildrenByName( "child" ).empty() );
    guard.sceneManager->destroyActor( parent );
}

BOOST_AUTO_TEST_CASE( persistent_streamed_hierarchies_receive_scene_updates_and_unregister_on_reentrant_close )
{
    using namespace wpStreamingPrefabTest;
    TestGuard guard; guard.scene->clear( true );
    const auto oldTask = Thread::getCurrentTask();
    const auto oldFlags = Thread::getTaskFlags( TaskId::Application );
    guard.addCleanup( [oldTask, oldFlags] {
        Thread::setTaskFlags( TaskId::Application, oldFlags ); Thread::setCurrentTask( oldTask ); } );
    auto data = make_ptr<Properties>(); auto rootData = actor( "Persistent update root", 0 );
    auto childData = actor( "Persistent update child", 5 ); childData->setName( "child" ); rootData->addChild( childData );
    // An authored LOD component makes this group persistent, using the same
    // conservative preservation path as animation, scripts and physics.
    auto lod = make_ptr<Properties>(); lod->setName( "component" );
    lod->setProperty( "componentType", "workphone::scene::LODGroup" );
    lod->setProperty( "uuid", StringUtil::getUUID() ); rootData->addChild( lod ); data->addChild( rootData );
    String manifest, error; BOOST_REQUIRE_MESSAGE( SceneWorld::cook( *data, directory( guard ), {}, manifest, error ), error );
    auto instance = owner( guard, 100 ); SceneWorldStreamer stream;
    BOOST_REQUIRE_MESSAGE( stream.open( manifest, guard.scene, {}, instance, error ), error );
    pumpInstance( stream, Vector3F( 100, 0, 0 ), 2 );
    auto root = find( instance, "Persistent update root" ), child = find( instance, "Persistent update child" );
    BOOST_REQUIRE( root ); BOOST_REQUIRE( child );
    auto rootProbe = make_ptr<PersistentUpdateProbe>(), childProbe = make_ptr<PersistentUpdateProbe>();
    root->addComponentInstance( rootProbe ); rootProbe->load( nullptr );
    child->addComponentInstance( childProbe ); childProbe->load( nullptr );
    Thread::setTaskFlags( TaskId::Application, oldFlags | Thread::Application_Flag );
    Thread::setCurrentTask( TaskId::Application );
    const auto tick = [&] { guard.scene->preUpdate(); guard.scene->update(); guard.scene->postUpdate(); };
    tick();
    for( const auto &probe : { rootProbe, childProbe } )
    {
        BOOST_CHECK_EQUAL( probe->preUpdates, 1u ); BOOST_CHECK_EQUAL( probe->updates, 1u );
        BOOST_CHECK_EQUAL( probe->postUpdates, 1u );
    }
    guard.scene->setState( IGameScene::State::Play );
    BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( 100, 0, 0 ) }, error ), error );
    BOOST_CHECK( root->getState() == IGameActor::State::Play );
    BOOST_CHECK( child->getState() == IGameActor::State::Play );
    tick();
    child->setState( IGameActor::State::None, true );
    BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( 100, 0, 0 ) }, error ), error );
    BOOST_CHECK( child->getState() == IGameActor::State::None );
    guard.scene->setState( IGameScene::State::Edit );
    BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( 100, 0, 0 ) }, error ), error );
    BOOST_CHECK( root->getState() == IGameActor::State::Edit );
    BOOST_CHECK( child->getState() == IGameActor::State::Edit );
    tick();
    for( const auto &probe : { rootProbe, childProbe } )
    {
        BOOST_CHECK_EQUAL( probe->preUpdates, 3u ); BOOST_CHECK_EQUAL( probe->updates, 3u );
        BOOST_CHECK_EQUAL( probe->postUpdates, 3u );
    }
    // Unload callbacks can reenter all public coordinator entry points while
    // retire() temporarily owns the resident source lease elsewhere.
    for( auto probe : { rootProbe, childProbe } ) probe->onUnload = [&] {
        stream.close();
        String rejected;
        BOOST_CHECK( !stream.update( { Vector3F( 100, 0, 0 ) }, rejected ) );
        BOOST_CHECK( !stream.open( manifest, guard.scene, {}, instance, rejected ) );
    };
    guard.scene->setState( IGameScene::State::Play );
    // Force the callback to occur inside the streamer's transition sync even
    // when this fixture's scene applies root transitions synchronously.
    root->setState( IGameActor::State::Edit, true );
    rootProbe->onEnterPlay = [&] { stream.close(); };
    BOOST_CHECK( !stream.update( { Vector3F( 100, 0, 0 ) }, error ) );
    BOOST_CHECK( !error.empty() );
    BOOST_CHECK( instance->getChildren().empty() ); BOOST_CHECK_EQUAL( stream.statistics().residentActors, 0u );
    BOOST_CHECK_EQUAL( stream.statistics().residentCells, 0u );
    for( u32 phase = 0; phase < static_cast<u32>( Thread::UpdateState::Count ); ++phase )
        for( u32 task = 0; task < static_cast<u32>( TaskId::Count ); ++task )
        {
            const auto entries = guard.scene->getRegisteredObjects(
                static_cast<Thread::UpdateState>( phase ), static_cast<TaskId>( task ) ).snapshot();
            BOOST_CHECK( std::find( entries.begin(), entries.end(), root ) == entries.end() );
            BOOST_CHECK( std::find( entries.begin(), entries.end(), child ) == entries.end() );
        }
    tick();
    for( const auto &probe : { rootProbe, childProbe } )
    {
        BOOST_CHECK_EQUAL( probe->preUpdates, 3u ); BOOST_CHECK_EQUAL( probe->updates, 3u );
        BOOST_CHECK_EQUAL( probe->postUpdates, 3u ); BOOST_CHECK_EQUAL( probe->unloads, 1u );
    }
}

BOOST_AUTO_TEST_SUITE_END()
