#ifndef WP_SCENE_WORLD_IDENTITY_TESTS_HPP
#define WP_SCENE_WORLD_IDENTITY_TESTS_HPP

#include "TestGuard.hpp"
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <set>

namespace scene_world_identity_tests
{
    using namespace workphone;
    using namespace workphone::scene;

    inline SmartPtr<Properties> read( const String &path )
    {
        std::ifstream input( path.c_str(), std::ios::binary | std::ios::ate );
        BOOST_REQUIRE( input );
        Array<u8> bytes( size_t( input.tellg() ) ); input.seekg( 0 );
        input.read( reinterpret_cast<char *>( bytes.data() ), bytes.size() );
        BOOST_REQUIRE( input );
        auto data = make_ptr<Properties>(); String error;
        BOOST_REQUIRE_MESSAGE( PropertiesBinarySerializer::deserialize( bytes, *data, &error ), error );
        return data;
    }
    inline void write( const String &path, const Properties &data )
    {
        const auto bytes = PropertiesBinarySerializer::serialize( data );
        std::ofstream output( path.c_str(), std::ios::binary | std::ios::trunc );
        output.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() ); output.close();
        BOOST_REQUIRE( output );
    }
    inline SmartPtr<Properties> actor( float x )
    {
        auto data = make_ptr<Properties>(); data->setName( "actors" );
        data->setProperty( "uuid", StringUtil::getUUID() ); data->setProperty( "static", true );
        auto transform = make_ptr<Properties>(); transform->setName( "localTransform" );
        transform->setProperty( "Position", Vector3F( x, 0, 0 ) ); data->addChild( transform );
        auto component = make_ptr<Properties>(); component->setName( "component" );
        component->setProperty( "componentType", "workphone::scene::Mesh" );
        component->setProperty( "uuid", StringUtil::getUUID() ); data->addChild( component );
        return data;
    }
    inline String cook( TestGuard &guard, const Properties &data )
    {
        const auto directory = ( std::filesystem::temp_directory_path() /
            ( "wp-world-identities-" + std::string( StringUtil::getUUID().c_str() ) ) ).generic_string();
        guard.trackFilesystemPath( directory.c_str() );
        String path, error; WorldCookOptions options; options.cellSize = 64;
        BOOST_REQUIRE_MESSAGE( SceneWorld::cook( data, directory.c_str(), options, path, error ), error );
        return path;
    }
}

BOOST_AUTO_TEST_SUITE( SceneWorldIdentityTests )

BOOST_AUTO_TEST_CASE( cooked_inventories_follow_actor_and_component_definitions )
{
    using namespace scene_world_identity_tests;
    TestGuard guard;
    auto root = actor( 0 );
    auto component = root->getChild( "component" );
    component->setProperty( "componentType", "workphone::scene::Animator" );
    root->removeChild( "component" );
    auto components = make_ptr<Properties>(); components->setName( "components" );
    components->addChild( component ); root->addChild( components );
    const auto resourceId = StringUtil::getUUID();
    auto resource = make_ptr<Properties>(); resource->setName( "payload" );
    resource->setProperty( "uuid", resourceId ); component->addChild( resource );
    component->setProperty( "assetUuid", resourceId );
    auto child = actor( 10 ); child->setName( "child" );
    child->removeChild( "component" );
    auto children = make_ptr<Properties>(); children->setName( "children" );
    children->addChild( child ); root->addChild( children );
    const auto path = cook( guard, *root );
    WorldManifest manifest; String error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( path, manifest, error ), error );
    BOOST_REQUIRE_EQUAL( manifest.cells.size(), 1u );
    const auto &cell = manifest.cells.front();
    BOOST_CHECK_EQUAL( cell.id, "persistent" ); BOOST_CHECK_EQUAL( cell.actors, 2u );
    BOOST_REQUIRE_EQUAL( cell.actorIds.size(), 2u ); BOOST_REQUIRE_EQUAL( cell.objectIds.size(), 3u );
    const std::set<String> ids( cell.objectIds.begin(), cell.objectIds.end() );
    BOOST_CHECK( ids.count( root->getProperty( "uuid" ) ) );
    BOOST_CHECK( ids.count( child->getProperty( "uuid" ) ) );
    BOOST_CHECK( ids.count( component->getProperty( "uuid" ) ) ); BOOST_CHECK( !ids.count( resourceId ) );
    auto payload = SceneWorld::readCell( path, cell, error );
    BOOST_REQUIRE_MESSAGE( payload, error );
    const auto saved = payload->getChild( "actors" )->getChild( "components" )->getChild( "component" );
    BOOST_CHECK_EQUAL( saved->getProperty( "assetUuid" ), resourceId );
    BOOST_CHECK_EQUAL( saved->getChild( "payload" )->getProperty( "uuid" ), resourceId );
}

BOOST_AUTO_TEST_CASE( manifest_identity_validation_is_optional_and_rejects_inconsistent_metadata )
{
    using namespace scene_world_identity_tests;
    TestGuard guard; auto data = make_ptr<Properties>();
    data->addChild( actor( 0 ) ); data->addChild( actor( 1000 ) );
    const auto path = cook( guard, *data );
    const auto original = read( path ); WorldManifest manifest; String error;
    BOOST_REQUIRE( SceneWorld::readManifest( path, manifest, error ) );
    BOOST_REQUIRE_EQUAL( manifest.cells.size(), 2u );
    for( const auto version : { 1u, 2u } )
    {
        auto legacy = read( path ); legacy->setProperty( "version", version );
        for( auto entry : legacy->getChildrenByName( "cell" ) )
        { entry->removeProperty( "actorIds" ); entry->removeProperty( "objectIds" ); }
        write( path, *legacy );
        BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( path, manifest, error ), error );
        for( const auto &cell : manifest.cells )
        { BOOST_CHECK( cell.actorIds.empty() ); BOOST_CHECK( cell.objectIds.empty() ); }
        write( path, *original );
    }
    for( unsigned invalid = 0; invalid < 6; ++invalid )
    {
        auto broken = read( path ); auto entries = broken->getChildrenByName( "cell" );
        Array<String> actors, objects;
        entries.front()->getPropertyValue( "actorIds", actors );
        entries.front()->getPropertyValue( "objectIds", objects );
        if( invalid == 0 ) entries.front()->removeProperty( "objectIds" );
        if( invalid == 1 ) entries.front()->setProperty( "actors", 2u );
        if( invalid == 2 )
        { objects.push_back( objects.front() ); entries.front()->setProperty( "objectIds", objects ); }
        if( invalid == 3 ) entries.front()->setProperty( "actorIds", Array<String>{ StringUtil::getUUID() } );
        if( invalid == 4 ) entries.front()->setProperty( "actorIds", actors.front() );
        if( invalid == 5 )
        { entries.back()->setProperty( "actorIds", actors ); entries.back()->setProperty( "objectIds", objects ); }
        write( path, *broken );
        BOOST_CHECK_MESSAGE( !SceneWorld::readManifest( path, manifest, error ), invalid );
        BOOST_CHECK( !error.empty() ); write( path, *original );
    }
}

BOOST_AUTO_TEST_CASE( cell_definition_identity_must_match_manifest_inventory )
{
    using namespace scene_world_identity_tests;
    TestGuard guard; const auto path = cook( guard, *actor( 0 ) );
    WorldManifest manifest; String error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( path, manifest, error ), error );
    auto cell = manifest.cells.front();
    const auto payloadPath = ( std::filesystem::path( path.c_str() ).parent_path() / cell.file.c_str() ).generic_string();
    auto payload = read( payloadPath.c_str() );
    payload->getChild( "actors" )->getChild( "component" )->setProperty( "uuid", StringUtil::getUUID() );
    write( payloadPath.c_str(), *payload );
    cell.bytes = std::filesystem::file_size( payloadPath );
    BOOST_CHECK( !SceneWorld::readCell( path, cell, error ) ); BOOST_CHECK( !error.empty() );
    cell.actorIds.clear(); cell.objectIds.clear();
    BOOST_CHECK_MESSAGE( SceneWorld::readCell( path, cell, error ), error );
}

BOOST_AUTO_TEST_CASE( invalid_definition_ids_preserve_the_published_generation )
{
    using namespace scene_world_identity_tests;
    TestGuard guard; auto data = make_ptr<Properties>();
    auto first = actor( 0 ), second = actor( 1000 ); data->addChild( first ); data->addChild( second );
    const auto path = cook( guard, *data ); WorldManifest before; String error;
    BOOST_REQUIRE( SceneWorld::readManifest( path, before, error ) );
    const auto originalActorId = second->getProperty( "uuid" );
    for( unsigned invalid = 0; invalid < 2; ++invalid )
    {
        if( invalid == 0 ) second->setProperty( "uuid", first->getProperty( "uuid" ) );
        else
        {
            second->setProperty( "uuid", originalActorId );
            second->getChild( "component" )->removeProperty( "uuid" );
        }
        String rejected;
        BOOST_CHECK( !SceneWorld::cook( *data,
            std::filesystem::path( path.c_str() ).parent_path().generic_string().c_str(), {}, rejected, error ) );
        BOOST_CHECK( !error.empty() ); BOOST_CHECK( rejected.empty() );
        WorldManifest after;
        BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( path, after, error ), error );
        BOOST_CHECK_EQUAL( before.generation, after.generation );
    }
}

BOOST_AUTO_TEST_CASE( serialized_reference_arrays_pin_every_target_group_persistent )
{
    using namespace scene_world_identity_tests;
    TestGuard guard; auto data = make_ptr<Properties>();
    auto first = actor( 0 ), second = actor( 1000 ), owner = actor( 2000 );
    auto renderer = make_ptr<Properties>(); renderer->setName( "component" );
    renderer->setProperty( "componentType", "workphone::scene::MeshRenderer" );
    renderer->setProperty( "uuid", StringUtil::getUUID() ); owner->addChild( renderer );
    data->addChild( first ); data->addChild( second ); data->addChild( owner );
    WorldManifest unreferenced; String error;
    BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( cook( guard, *data ), unreferenced, error ), error );
    BOOST_REQUIRE_EQUAL( unreferenced.cells.size(), 3u );
    for( const auto &cell : unreferenced.cells ) BOOST_CHECK( cell.id != "persistent" );

    const Array<String> targets = { first->getProperty( "uuid" ), second->getProperty( "uuid" ) };
    // The on-disk Properties array is one semicolon-separated value. Both
    // targets must stay resident while a renderer in another cell references them.
    renderer->setProperty( "linkedActors", StringUtil::toString( targets ) );
    const auto path = cook( guard, *data ); WorldManifest referenced;
    BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( path, referenced, error ), error );
    BOOST_REQUIRE_EQUAL( referenced.cells.size(), 2u );
    u32 persistent = 0, spatial = 0;
    for( const auto &cell : referenced.cells )
    {
        if( cell.id == "persistent" )
        {
            ++persistent; BOOST_CHECK_EQUAL( cell.actors, 2u );
            BOOST_CHECK( std::set<String>( cell.actorIds.begin(), cell.actorIds.end() ) ==
                         std::set<String>( targets.begin(), targets.end() ) );
            BOOST_CHECK_MESSAGE( SceneWorld::readCell( path, cell, error ), error );
        }
        else
        {
            ++spatial; BOOST_REQUIRE_EQUAL( cell.actorIds.size(), 1u );
            BOOST_CHECK_EQUAL( cell.actorIds.front(), owner->getProperty( "uuid" ) );
        }
    }
    BOOST_CHECK_EQUAL( persistent, 1u ); BOOST_CHECK_EQUAL( spatial, 1u );
}

BOOST_AUTO_TEST_SUITE_END()
#endif
