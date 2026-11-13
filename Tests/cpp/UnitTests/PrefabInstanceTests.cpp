#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/GamePrefabManager.hpp>
#include <Workphone/Scene/PrefabInstanceService.hpp>
#include <Workphone/Scene/Components/PrefabInstance.hpp>
#include <Workphone/Scene/Components/Camera/CameraFollow.hpp>
#include <Workphone/Scene/Components/Camera/CameraTarget.hpp>
#include <Workphone/System/FactoryTemplate.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    String identity( SmartPtr<ISharedObject> object )
    {
        return object->getHandle()->getUUIDAsString();
    }

    SmartPtr<Properties> actorData( const String &name, const String &uuid,
                                    const Vector3F &position = Vector3F::zero() )
    {
        auto data = make_ptr<Properties>();
        data->setName( GameActorUtil::childStr );
        data->setProperty( GameActorUtil::labelStr, name );
        data->setProperty( GameActorUtil::uuidStr, uuid );
        data->setProperty( "enabled", true );
        data->setProperty( "visible", true );
        auto transform = make_ptr<Properties>();
        transform->setName( GameActorUtil::localTransformStr );
        transform->setProperty( "Position", position );
        transform->setProperty( "Orientation", QuaternionF::identity() );
        transform->setProperty( "Scale", Vector3F( 1, 1, 1 ) );
        data->addChild( transform );
        return data;
    }

    SmartPtr<Properties> componentData( const String &type, const String &uuid )
    {
        auto data = make_ptr<Properties>();
        data->setName( GameActorUtil::componentStr );
        data->setProperty( GameActorUtil::componentTypeStr, type );
        data->setProperty( GameActorUtil::uuidStr, uuid );
        data->setProperty( "Enabled", true );
        data->setProperty( "state", static_cast<s32>( IComponent::State::Edit ) );
        return data;
    }

    void checkPosition( const Vector3F &actual, const Vector3F &expected )
    {
        BOOST_CHECK_SMALL( actual.x - expected.x, 0.0002f );
        BOOST_CHECK_SMALL( actual.y - expected.y, 0.0002f );
        BOOST_CHECK_SMALL( actual.z - expected.z, 0.0002f );
    }

    void checkRotation( const QuaternionF &actual, const QuaternionF &expected )
    {
        // q and -q represent the same rotation.
        const auto dot = actual.w * expected.w + actual.x * expected.x +
                         actual.y * expected.y + actual.z * expected.z;
        BOOST_CHECK_SMALL( std::abs( dot ) - 1.0f, 0.0002f );
    }

    String serialized( SmartPtr<IGameActor> actor )
    {
        return DataUtil::toString( actor->toData().get() );
    }

    class PrefabCommitProbe : public CameraTarget
    {
    public:
        inline static PrefabCommitProbe *rejectTarget = nullptr;
        SmartPtr<Properties> getProperties() const override
        {
            auto properties = CameraTarget::getProperties();
            properties->setProperty( "FailOnCommit", failOnCommit );
            return properties;
        }
        void setProperties( SmartPtr<Properties> properties ) override
        {
            CameraTarget::setProperties( properties );
            bool requested = false;
            properties->getPropertyValue( "FailOnCommit", requested );
            if( this == rejectTarget && requested )
                throw std::runtime_error( "Injected retained-component setter failure" );
            failOnCommit = requested;
        }
        bool failOnCommit = false;
    };

    struct PrefabInstanceFixture : TestGuard
    {
        const String sourcePath = "Assets/Prefabs/Contracts/Linked.prefab";
        const String assetId = StringUtil::getUUID();
        const String rootId = StringUtil::getUUID();
        const String targetId = StringUtil::getUUID();
        const String targetComponentId = StringUtil::getUUID();
        const String followId = StringUtil::getUUID();
        const String removedId = StringUtil::getUUID();
        const String retypedId = StringUtil::getUUID();
        const String retypedComponentId = StringUtil::getUUID();
        const String addedId = StringUtil::getUUID();
        const QuaternionF sourceRotation = QuaternionF::eulerDegrees( 10, 40, 5 );
        const Vector3F sourceScale = Vector3F( 2, 3, 4 );
        bool previousEditor = applicationManager->isEditor();
        bool previousPlaying = applicationManager->isPlaying();

        PrefabInstanceFixture()
        {
            BOOST_REQUIRE( isAvailable );
            scene->clear( true );
            applicationManager->setEditor( true );
            applicationManager->setPlaying( false );
        }

        ~PrefabInstanceFixture()
        {
            scene->clear( true );
            applicationManager->setEditor( previousEditor );
            applicationManager->setPlaying( previousPlaying );
        }

        SmartPtr<Properties> source( bool revised = false )
        {
            auto root = actorData( revised ? "RevisedPrefab" : "OriginalPrefab", rootId,
                                   Vector3F( 9, 8, 7 ) );
            auto transform = root->getChild( GameActorUtil::localTransformStr );
            transform->setProperty( "Orientation", sourceRotation );
            transform->setProperty( "Scale", sourceScale );
            auto target = actorData( revised ? "RenamedTarget" : "Target", targetId,
                                     revised ? Vector3F( 7, 8, 9 ) : Vector3F( 1, 2, 3 ) );
            auto targetComponent = componentData( "CameraTarget", targetComponentId );
            targetComponent->setProperty( CameraTarget::offsetPositionStr,
                                          revised ? Vector3F( 6, 5, 4 ) : Vector3F( 1, 0, 0 ) );
            target->addChild( targetComponent );
            root->addChild( target );
            auto follow = componentData( "CameraFollow", followId );
            follow->setProperty( CameraFollow::followObjectStr, targetId );
            follow->setProperty( CameraFollow::targetStr, targetComponentId );
            root->addChild( follow );
            if( !revised ) root->addChild( actorData( "Removed", removedId ) );
            auto retyped = actorData( "Retyped", retypedId );
            retyped->addChild( componentData( revised ? "CameraFollow" : "CameraTarget",
                                               retypedComponentId ) );
            root->addChild( retyped );
            if( revised ) root->addChild( actorData( "Added", addedId ) );
            return root;
        }

        SmartPtr<IGameActor> instance( const String &path = "" )
        {
            auto data = PrefabInstanceService::createLinkedInstanceData(
                source(), path.empty() ? sourcePath : path, assetId );
            auto actors = GameActorUtil::loadSceneActors( { data } );
            BOOST_REQUIRE_EQUAL( actors.size(), 1u );
            auto result = actors.front();
            BOOST_REQUIRE( result );
            scene->addActor( result );
            result->updateTransform();
            BOOST_REQUIRE( result->getComponent<PrefabInstance>() );
            return result;
        }
    };
}

BOOST_FIXTURE_TEST_SUITE( PrefabInstanceTests, PrefabInstanceFixture )

BOOST_AUTO_TEST_CASE( initial_save_links_source_actor_only_after_successful_publication )
{
    const auto folder = std::filesystem::path( applicationManager->getProjectPath().c_str() ) /
                        "Assets" / std::string( ( "prefab-contract-" + StringUtil::getUUID() ).c_str() );
    std::filesystem::create_directories( folder );
    trackFilesystemPath( String( folder.generic_u8string().c_str() ) );
    auto manager = workphone::dynamic_pointer_cast<GamePrefabManager>( applicationManager->getPrefabManager() );
    BOOST_REQUIRE( manager );
    auto actor = createBasicActor();
    actor->setName( "NewlySavedSource" );
    scene->addActor( actor );
    auto component = actor->addComponent<CameraTarget>();
    const auto actorUuid = identity( actor ), componentUuid = identity( component );
    const auto path = String( ( folder / "Saved.prefab" ).generic_u8string().c_str() );
    String error;
    BOOST_REQUIRE_MESSAGE( manager->savePrefabChecked( path, actor, error ), error.c_str() );
    auto link = actor->getComponent<PrefabInstance>();
    BOOST_REQUIRE( link );
    BOOST_CHECK_EQUAL( identity( actor ), actorUuid );
    BOOST_CHECK( actor->getComponent<CameraTarget>() == component );
    bool rootMapped = false, componentMapped = false;
    for( auto entry : link->getIdentityMap()->getChildrenByName( "identity" ) )
    {
        const auto source = entry->getProperty( "sourceUuid" );
        const auto instance = entry->getProperty( "instanceUuid" );
        if( source == actorUuid ) { rootMapped = true; BOOST_CHECK_EQUAL( instance, actorUuid ); }
        if( source == componentUuid ) { componentMapped = true; BOOST_CHECK_EQUAL( instance, componentUuid ); }
    }
    BOOST_CHECK( rootMapped && componentMapped );
    const auto bytes = applicationManager->getFileSystem()->readAllText( path );
    BOOST_REQUIRE( !bytes.empty() );
    auto document = make_ptr<Properties>();
    const auto first = bytes.find_first_not_of( " \t\r\n" );
    BOOST_REQUIRE( first != String::npos );
    DataUtil::parse( bytes, document.get(), bytes[first] == '<' ? DataFormat::XML : DataFormat::JSON );
    BOOST_CHECK_EQUAL( document->getProperty( "uuid" ), actorUuid );
    for( auto savedComponent : document->getChildrenByName( GameActorUtil::componentStr ) )
        BOOST_CHECK( savedComponent->getProperty( "componentType" ).find( "PrefabInstance" ) == String::npos );

    auto unsaved = createBasicActor();
    scene->addActor( unsaved );
    auto unsavedComponent = unsaved->addComponent<CameraTarget>();
    const auto unsavedUuid = identity( unsaved );
    const auto unsavedFlags = unsaved->getFlags();
    const auto obstruction = folder / "RegularFile";
    std::ofstream( obstruction, std::ios::binary ) << "Not a directory";
    const auto failedPath = obstruction / "MustNotExist.prefab";
    BOOST_CHECK( !manager->savePrefabChecked( String( failedPath.generic_u8string().c_str() ), unsaved, error ) );
    BOOST_CHECK( !error.empty() );
    BOOST_CHECK( !unsaved->getComponent<PrefabInstance>() );
    BOOST_CHECK( unsaved->getComponent<CameraTarget>() == unsavedComponent );
    BOOST_CHECK_EQUAL( identity( unsaved ), unsavedUuid );
    BOOST_CHECK_EQUAL( unsaved->getFlags(), unsavedFlags );
    std::error_code status;
    BOOST_CHECK( !std::filesystem::exists( failedPath, status ) );
}

BOOST_AUTO_TEST_CASE( nested_provenance_source_ids_survive_outer_instance_and_duplicate_remapping )
{
    const auto nestedSourceId = StringUtil::getUUID();
    auto nestedSource = actorData( "NestedSource", nestedSourceId );
    auto nested = PrefabInstanceService::createLinkedInstanceData(
        nestedSource, "Assets/Prefabs/Contracts/Nested.prefab" );
    nested->setName( GameActorUtil::childStr );
    // A source identifier may legitimately equal another actor identifier in
    // the surrounding document. It is provenance, not an object reference.
    auto outerSource = actorData( "OuterSource", nestedSourceId );
    outerSource->addChild( nested );
    auto outer = PrefabInstanceService::createLinkedInstanceData( outerSource, sourcePath );
    auto duplicate = GameActorUtil::createInstanceData( outer );
    auto directDuplicate = GameActorUtil::createInstanceData( outerSource );
    String firstInstanceId;
    for( auto document : { outer, duplicate, directDuplicate } )
    {
        auto nestedActor = document->getChild( GameActorUtil::childStr );
        BOOST_REQUIRE( nestedActor );
        const auto nestedInstanceId = nestedActor->getProperty( GameActorUtil::uuidStr );
        BOOST_CHECK( nestedInstanceId != nestedSourceId );
        if( firstInstanceId.empty() ) firstInstanceId = nestedInstanceId;
        else BOOST_CHECK( nestedInstanceId != firstInstanceId );
        SmartPtr<Properties> nestedLink;
        for( auto component : nestedActor->getChildrenByName( GameActorUtil::componentStr ) )
            if( component->getProperty( GameActorUtil::componentTypeStr ).find( "PrefabInstance" ) != String::npos )
                nestedLink = component;
        BOOST_REQUIRE( nestedLink );
        bool found = false;
        for( auto entry : nestedLink->getChildrenByName( "identity" ) )
            if( entry->getProperty( "sourceUuid" ) == nestedSourceId )
            {
                found = true;
                BOOST_CHECK_EQUAL( entry->getProperty( "instanceUuid" ), nestedInstanceId );
            }
        BOOST_CHECK_MESSAGE( found, "Duplicating an outer prefab remapped a nested source UUID" );
    }
}

BOOST_AUTO_TEST_CASE( missing_source_ids_remain_stable_across_equivalent_path_spellings )
{
    auto imported = actorData( "ImportedRoot", "" );
    auto childData = actorData( "ImportedChild", "" );
    childData->addChild( componentData( "CameraTarget", "" ) );
    imported->addChild( childData );
    const auto originalDocument = DataUtil::toString( imported.get() );
    const auto absolutePath = String( ( std::filesystem::u8path( applicationManager->getProjectPath().c_str() ) /
                                       std::filesystem::u8path( sourcePath.c_str() ) ).generic_u8string().c_str() );
    const auto initial = PrefabInstanceService::createLinkedInstanceData( imported, absolutePath );
    auto actors = GameActorUtil::loadSceneActors( { initial } );
    BOOST_REQUIRE_EQUAL( actors.size(), 1u );
    auto root = actors.front();
    scene->addActor( root );
    auto child = root->findChildByName( "ImportedChild" );
    BOOST_REQUIRE( child );
    auto component = child->getComponent<CameraTarget>();
    BOOST_REQUIRE( component );
    const auto childUuid = identity( child ), componentUuid = identity( component );
    Array<String> spellings{ sourcePath, "Assets/Prefabs/../Prefabs/Contracts/Linked.prefab" };
#ifdef _WIN32
    spellings.push_back( StringUtil::make_upper( absolutePath ) );
    spellings.push_back( "Assets\\Prefabs\\Contracts\\Linked.prefab" );
#endif
    for( const auto &path : spellings )
    {
        auto transaction = PrefabInstanceService::prepareUpdates( scene, path, imported, false );
        BOOST_REQUIRE_EQUAL( transaction->changedCount(), 1u );
        transaction->commit();
        BOOST_CHECK( root->findChildByName( "ImportedChild" ) == child );
        BOOST_CHECK( child->getComponent<CameraTarget>() == component );
        BOOST_CHECK_EQUAL( identity( child ), childUuid );
        BOOST_CHECK_EQUAL( identity( component ), componentUuid );
        transaction->finish();
    }
    BOOST_CHECK_EQUAL( DataUtil::toString( imported.get() ), originalDocument );
}

BOOST_AUTO_TEST_CASE( native_json_and_xml_prefab_creation_links_authoring_instances_only )
{
    for( const auto format : { DataFormat::JSON, DataFormat::XML } )
    {
        const auto sourceData = source();
        // Imported prefabs can retain a serialized Play state. Authoring must
        // normalize every component to Edit before exposing the instance.
        const auto markImportedPlay = [&]( auto &&self, SmartPtr<Properties> node ) -> void {
            if( node->hasProperty( GameActorUtil::componentTypeStr ) )
                node->setProperty( "state", static_cast<s32>( IComponent::State::Play ) );
            for( auto child : node->getChildren() ) self( self, child );
        };
        markImportedPlay( markImportedPlay, sourceData );
        const auto bytes = DataUtil::toString( sourceData.get(), true, format );
        auto path = std::filesystem::temp_directory_path() /
                    std::string( ( "workphone-prefab-source-" + StringUtil::getUUID() + ".prefab" ).c_str() );
        const auto fullPath = String( path.generic_u8string().c_str() );
        trackFilesystemPath( fullPath );
        std::ofstream output( path, std::ios::binary );
        output.write( bytes.c_str(), static_cast<std::streamsize>( bytes.size() ) );
        output.close();
        BOOST_REQUIRE( output.good() );
        auto prefab = applicationManager->getPrefabManager()->loadPrefab( fullPath );
        BOOST_REQUIRE( prefab );
        auto first = prefab->createActor();
        auto second = prefab->createActor();
        BOOST_REQUIRE( first );BOOST_REQUIRE( second );
        scene->addActor( first );scene->addActor( second );
        BOOST_REQUIRE( first->getComponent<PrefabInstance>() );
        BOOST_REQUIRE( second->getComponent<PrefabInstance>() );
        const auto checkEditState = [&]( auto &&self, SmartPtr<IGameActor> current ) -> void {
            for( auto component : current->getComponents() )
                BOOST_CHECK( component->getState() == IComponent::State::Edit );
            for( auto child : current->getChildren() ) self( self, child );
        };
        checkEditState( checkEditState, first );
        checkEditState( checkEditState, second );
        BOOST_CHECK( identity( first ) != identity( second ) );
        BOOST_CHECK_EQUAL( first->getComponent<PrefabInstance>()->getSourcePath(), fullPath );
        auto child = first->findChildByName( "Target" );
        BOOST_REQUIRE( child );
        BOOST_CHECK( first->getComponent<CameraFollow>()->getFollowObject() == child );
        applicationManager->setEditor( false );
        auto runtime = prefab->createActor();
        applicationManager->setEditor( true );
        BOOST_REQUIRE( runtime );
        scene->addActor( runtime );
        BOOST_CHECK( !runtime->getComponent<PrefabInstance>() );
        BOOST_CHECK_EQUAL( applicationManager->getFileSystem()->readAllText( fullPath ), bytes );
    }
}

BOOST_AUTO_TEST_CASE( retained_objects_and_internal_external_references_keep_identity )
{
    auto root = instance();
    auto child = root->findChildByName( "Target" );
    auto target = child->getComponent<CameraTarget>();
    auto follow = root->getComponent<CameraFollow>();
    auto observer = createBasicActor();
    scene->addActor( observer );
    auto external = observer->addComponent<CameraFollow>();
    external->setFollowObject( child );
    external->setTarget( target );
    const auto rootUuid = identity( root ), childUuid = identity( child ), componentUuid = identity( target );
    const auto newTemplate = source( true );
    const auto templateBefore = DataUtil::toString( newTemplate.get() );
    auto transaction = PrefabInstanceService::prepareUpdate( root, newTemplate );
    BOOST_REQUIRE( transaction );
    transaction->commit();
    BOOST_CHECK_EQUAL( root->getName(), "RevisedPrefab" );
    BOOST_CHECK( root->findChildByName( "RenamedTarget" ) == child );
    BOOST_CHECK( child->getComponent<CameraTarget>() == target );
    BOOST_CHECK( root->getComponent<CameraFollow>() == follow );
    BOOST_CHECK( follow->getFollowObject() == child );
    BOOST_CHECK( follow->getTarget() == target );
    BOOST_CHECK( external->getFollowObject() == child );
    BOOST_CHECK( external->getTarget() == target );
    BOOST_CHECK_EQUAL( identity( root ), rootUuid );
    BOOST_CHECK_EQUAL( identity( child ), childUuid );
    BOOST_CHECK_EQUAL( identity( target ), componentUuid );
    checkPosition( target->getOffsetPosition(), Vector3F( 6, 5, 4 ) );
    transaction->finish();
    BOOST_CHECK_EQUAL( DataUtil::toString( newTemplate.get() ), templateBefore );
}

BOOST_AUTO_TEST_CASE( partial_reference_properties_preserve_missing_fields_and_clear_explicit_nulls )
{
    auto root = instance();
    auto child = root->findChildByName( "Target" );
    auto target = child->getComponent<CameraTarget>();
    auto follow = root->getComponent<CameraFollow>();
    follow->setProperties( make_ptr<Properties>() );
    BOOST_CHECK( follow->getFollowObject() == child );
    BOOST_CHECK( follow->getTarget() == target );
    auto cleared = make_ptr<Properties>();
    cleared->setProperty( CameraFollow::followObjectStr, "" );
    cleared->setProperty( CameraFollow::targetStr, "" );
    follow->setProperties( cleared );
    BOOST_CHECK( !follow->getFollowObject() );
    BOOST_CHECK( !follow->getTarget() );
}

BOOST_AUTO_TEST_CASE( added_deleted_and_retyped_nodes_follow_the_new_source_graph )
{
    auto root = instance();
    auto retained = root->findChildByName( "Retyped" );
    auto oldComponent = retained->getComponent<CameraTarget>();
    const auto componentUuid = identity( oldComponent );
    auto removed = root->findChildByName( "Removed" );
    auto observer = createBasicActor();
    scene->addActor( observer );
    auto external = observer->addComponent<CameraFollow>();
    external->setFollowObject( removed );
    external->setTarget( oldComponent );
    auto transaction = PrefabInstanceService::prepareUpdate( root, source( true ) );
    transaction->commit();
    BOOST_CHECK( !root->findChildByName( "Removed" ) );
    BOOST_REQUIRE( root->findChildByName( "Added" ) );
    BOOST_CHECK( root->findChildByName( "Retyped" ) == retained );
    BOOST_CHECK( !retained->getComponent<CameraTarget>() );
    auto replacement = retained->getComponent<CameraFollow>();
    BOOST_REQUIRE( replacement );
    BOOST_CHECK_EQUAL( identity( replacement ), componentUuid );
    BOOST_CHECK( !external->getFollowObject() );
    BOOST_CHECK( !external->getTarget() );
    transaction->finish();
}

BOOST_AUTO_TEST_CASE( parented_instance_preserves_only_world_position_and_parent_attachment )
{
    auto root = instance();
    auto parent = createBasicActor();
    scene->addActor( parent );
    parent->setLocalPosition( Vector3F( 40, 20, -30 ) );
    parent->setLocalOrientation( QuaternionF::eulerDegrees( 0, 65, 0 ) );
    parent->setLocalScale( Vector3F( 1.5f, 2.0f, 0.75f ) );
    scene->removeActor( root );
    parent->addChild( root );
    root->setLocalPosition( Vector3F( 3, 4, 5 ) );
    root->setLocalOrientation( QuaternionF::eulerDegrees( 75, 12, 21 ) );
    root->setLocalScale( Vector3F( 8, 6, 9 ) );
    root->setName( "InstanceNameOverride" );
    root->setVisible( false, false );
    parent->updateTransform();
    root->updateTransform();
    const auto position = root->getPosition();
    auto transaction = PrefabInstanceService::prepareUpdate( root, source( true ) );
    transaction->commit();
    parent->updateTransform();
    root->updateTransform();
    BOOST_CHECK( root->getParent() == parent );
    checkPosition( root->getPosition(), position );
    checkRotation( root->getLocalOrientation(), sourceRotation );
    checkPosition( root->getLocalScale(), sourceScale );
    BOOST_CHECK_EQUAL( root->getName(), "RevisedPrefab" );
    BOOST_CHECK( root->isVisible() );
    checkPosition( root->findChildByName( "RenamedTarget" )->getLocalPosition(), Vector3F( 7, 8, 9 ) );
    transaction->finish();
}

BOOST_AUTO_TEST_CASE( campaign_reconcile_touches_only_instances_of_the_selected_source )
{
    auto first = instance();
    auto second = instance();
    auto other = instance( "Assets/Prefabs/Contracts/Other.prefab" );
    other->setName( "OtherOverride" );
    const auto otherBefore = serialized( other );
    auto transaction = PrefabInstanceService::prepareUpdates( scene, sourcePath, source( true ), false );
    BOOST_REQUIRE( transaction );
    BOOST_CHECK_EQUAL( transaction->changedCount(), 2u );
    transaction->commit();
    BOOST_CHECK_EQUAL( first->getName(), "RevisedPrefab" );
    BOOST_CHECK_EQUAL( second->getName(), "RevisedPrefab" );
    BOOST_CHECK_EQUAL( serialized( other ), otherBefore );
    transaction->finish();
}

BOOST_AUTO_TEST_CASE( matching_descendant_is_covered_by_ancestor_reset_without_overlapping_plans )
{
    auto parent = instance();
    auto nested = instance();
    scene->removeActor( nested );
    nested->setName( "NestedInstanceOverride" );
    parent->addChild( nested );
    parent->updateTransform();
    auto transaction = PrefabInstanceService::prepareUpdates( scene, sourcePath, source( true ), false );
    BOOST_REQUIRE( transaction );
    BOOST_CHECK_EQUAL( transaction->changedCount(), 1u );
    transaction->commit();
    BOOST_CHECK_EQUAL( parent->getName(), "RevisedPrefab" );
    BOOST_CHECK( !parent->findChildByName( "NestedInstanceOverride" ) );
    BOOST_REQUIRE( parent->findChildByName( "RenamedTarget" ) );
    transaction->finish();
}

BOOST_AUTO_TEST_CASE( committed_graph_can_roll_back_without_replacing_original_objects )
{
    auto root = instance();
    auto removed = root->findChildByName( "Removed" );
    auto retyped = root->findChildByName( "Retyped" );
    auto component = retyped->getComponent<CameraTarget>();
    auto child = root->findChildByName( "Target" );
    auto follower = root->getComponent<CameraFollow>();
    auto observer = createBasicActor();
    scene->addActor( observer );
    auto external = observer->addComponent<CameraFollow>();
    external->setFollowObject( removed );
    external->setTarget( component );
    root->setLocalPosition( Vector3F( 51, 12, -13 ) );
    root->updateTransform();
    const auto before = serialized( root );
    auto transaction = PrefabInstanceService::prepareUpdate( root, source( true ) );
    transaction->commit();
    BOOST_REQUIRE( root->findChildByName( "Added" ) );
    BOOST_CHECK( !external->getFollowObject() );
    BOOST_CHECK( !external->getTarget() );
    transaction->rollback();
    BOOST_CHECK( root->findChildByName( "Removed" ) == removed );
    BOOST_CHECK( root->findChildByName( "Retyped" ) == retyped );
    BOOST_CHECK( retyped->getComponent<CameraTarget>() == component );
    BOOST_CHECK( !retyped->getComponent<CameraFollow>() );
    BOOST_CHECK( root->findChildByName( "Target" ) == child );
    BOOST_CHECK( root->getComponent<CameraFollow>() == follower );
    BOOST_CHECK( follower->getFollowObject() == child );
    BOOST_CHECK( follower->getTarget() == child->getComponent<CameraTarget>() );
    BOOST_CHECK( external->getFollowObject() == removed );
    BOOST_CHECK( external->getTarget() == component );
    BOOST_CHECK( !root->findChildByName( "Added" ) );
    BOOST_CHECK_EQUAL( serialized( root ), before );
}

BOOST_AUTO_TEST_CASE( invalid_source_preflight_preserves_instances_and_source )
{
    auto root = instance();
    const auto before = serialized( root );
    auto invalid = source( true );
    invalid->addChild( componentData( "MissingPrefabComponentType", StringUtil::getUUID() ) );
    const auto invalidBefore = DataUtil::toString( invalid.get() );
    BOOST_CHECK_THROW( PrefabInstanceService::prepareUpdate( root, invalid ), std::exception );
    BOOST_CHECK_EQUAL( serialized( root ), before );
    BOOST_CHECK_EQUAL( DataUtil::toString( invalid.get() ), invalidBefore );
}

BOOST_AUTO_TEST_CASE( retained_component_setter_failure_automatically_rolls_back_partial_commit )
{
    auto producer = make_ptr<FactoryTemplate<PrefabCommitProbe>>();
    producer->setObjectTypeName( "PrefabCommitProbe" );
    producer->setObjectTypeHash( StringUtil::getHash64( "PrefabCommitProbe" ) );
    factoryManager->addFactory( producer );
    addCleanup( [manager = factoryManager, producer]() mutable {
        PrefabCommitProbe::rejectTarget = nullptr;
        manager->removeFactory( producer );
        producer->unload( nullptr );
    } );
    const auto probeSource = [&]( bool revised ) {
        auto data = source( revised );
        auto target = data->getChild( GameActorUtil::childStr );
        BOOST_REQUIRE( target );
        auto component = target->getChild( GameActorUtil::componentStr );
        BOOST_REQUIRE( component );
        component->setProperty( GameActorUtil::componentTypeStr, "PrefabCommitProbe" );
        component->setProperty( "FailOnCommit", revised );
        return data;
    };
    auto data = PrefabInstanceService::createLinkedInstanceData( probeSource( false ), sourcePath );
    auto actors = GameActorUtil::loadSceneActors( { data } );
    BOOST_REQUIRE_EQUAL( actors.size(), 1u );
    auto root = actors.front();
    scene->addActor( root );
    auto child = root->findChildByName( "Target" );
    BOOST_REQUIRE( child );
    auto probe = workphone::dynamic_pointer_cast<PrefabCommitProbe>( child->getComponent<CameraTarget>() );
    BOOST_REQUIRE( probe );
    root->updateTransform();
    const auto before = serialized( root );
    auto transaction = PrefabInstanceService::prepareUpdate( root, probeSource( true ) );
    PrefabCommitProbe::rejectTarget = probe.get();
    BOOST_CHECK_THROW( transaction->commit(), std::exception );
    PrefabCommitProbe::rejectTarget = nullptr;
    BOOST_CHECK_EQUAL( root->getName(), "OriginalPrefab" );
    BOOST_CHECK( root->findChildByName( "Target" ) == child );
    BOOST_CHECK( child->getComponent<CameraTarget>() == probe );
    BOOST_CHECK( !root->findChildByName( "Added" ) );
    checkPosition( probe->getOffsetPosition(), Vector3F( 1, 0, 0 ) );
    BOOST_CHECK_EQUAL( serialized( root ), before );
}

BOOST_AUTO_TEST_CASE( linked_identity_and_references_survive_native_json_and_xml_scene_reload )
{
    for( const auto format : { DataFormat::JSON, DataFormat::XML } )
    {
        scene->clear( true );
        auto root = instance();
        auto child = root->findChildByName( "Target" );
        const auto rootUuid = identity( root ), childUuid = identity( child );
        const auto componentUuid = identity( child->getComponent<CameraTarget>() );
        const auto linkPath = root->getComponent<PrefabInstance>()->getSourcePath();
        auto data = make_ptr<Properties>();
        auto savedActor = workphone::static_pointer_cast<Properties>( root->toData() );
        savedActor->setName( ApplicationUtil::actorsStr );
        data->addChild( savedActor );
        const auto bytes = DataUtil::toString( data.get(), true, format );
        const auto extension = format == DataFormat::XML ? ApplicationUtil::builtinXmlSceneExt
                                                        : ApplicationUtil::builtinSceneExt;
        auto path = std::filesystem::temp_directory_path() /
                    std::string( ( "workphone-prefab-link-" + StringUtil::getUUID() + extension ).c_str() );
        trackFilesystemPath( String( path.generic_u8string().c_str() ) );
        std::ofstream output( path, std::ios::binary );
        output.write( bytes.c_str(), static_cast<std::streamsize>( bytes.size() ) );
        output.close();
        BOOST_REQUIRE( output.good() );
        scene->clear( true );
        scene->loadScene( String( path.generic_u8string().c_str() ), false );
        BOOST_REQUIRE( scene->getSceneLoadingState() == IGameScene::SceneLoadingState::Loaded );
        auto restored = scene->findActorByName( "OriginalPrefab" );
        BOOST_REQUIRE( restored );
        auto restoredChild = restored->findChildByName( "Target" );
        BOOST_REQUIRE( restoredChild );
        auto link = restored->getComponent<PrefabInstance>();
        BOOST_REQUIRE( link );
        BOOST_CHECK_EQUAL( link->getSourcePath(), linkPath );
        BOOST_CHECK_EQUAL( link->getSourceAssetUuid(), assetId );
        BOOST_CHECK_EQUAL( identity( restored ), rootUuid );
        BOOST_CHECK_EQUAL( identity( restoredChild ), childUuid );
        BOOST_CHECK_EQUAL( identity( restoredChild->getComponent<CameraTarget>() ), componentUuid );
        auto follow = restored->getComponent<CameraFollow>();
        BOOST_REQUIRE( follow );
        BOOST_CHECK( follow->getFollowObject() == restoredChild );
        BOOST_CHECK( follow->getTarget() == restoredChild->getComponent<CameraTarget>() );
        auto templateData = PrefabInstanceService::makeTemplateData( restored );
        BOOST_REQUIRE( templateData );
        BOOST_CHECK_EQUAL( templateData->getProperty( GameActorUtil::uuidStr ), rootId );
        for( auto component : templateData->getChildrenByName( GameActorUtil::componentStr ) )
            BOOST_CHECK( component->getProperty( GameActorUtil::componentTypeStr ).find( "PrefabInstance" ) == String::npos );
        auto transaction = PrefabInstanceService::prepareUpdate( restored, source( true ) );
        transaction->commit();
        BOOST_CHECK( restored->findChildByName( "RenamedTarget" ) == restoredChild );
        transaction->finish();
    }
}

BOOST_AUTO_TEST_SUITE_END()
