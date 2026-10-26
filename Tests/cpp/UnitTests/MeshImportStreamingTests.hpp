#ifndef WP_MESH_IMPORT_STREAMING_TESTS_HPP
#define WP_MESH_IMPORT_STREAMING_TESTS_HPP

#include "TestGuard.hpp"
#include <Workphone/Scene/MeshImportStreaming.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/Scene/SceneWorldStreamer.hpp>
#include <Workphone/Scene/Components/WorldStreaming.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <thread>

namespace mesh_import_streaming_tests
{
    using namespace workphone;
    using namespace workphone::scene;
    namespace fs = std::filesystem;

    inline fs::path project( TestGuard &guard )
    {
        const auto path = fs::temp_directory_path() /
            ( "wp-mesh-import-streaming-" + std::string( StringUtil::getUUID().c_str() ) );
        guard.trackFilesystemPath( path.generic_string().c_str() ); fs::create_directories( path ); return path;
    }

    inline fs::path generation( const fs::path &root )
    {
        return root / "Cache" / "Worlds" / StringUtil::getUUID().c_str();
    }

    inline SmartPtr<Properties> pose( const String &name, const Vector3F &position,
                                     const QuaternionF &orientation = QuaternionF::identity(),
                                     const Vector3F &scale = Vector3F::unit() )
    {
        auto data = make_ptr<Properties>(); data->setName( name );
        data->setProperty( "Position", position ); data->setProperty( "Orientation", orientation );
        data->setProperty( "Scale", scale ); return data;
    }

    inline SmartPtr<Properties> leaf( const String &name, float x )
    {
        auto data = make_ptr<Properties>(); data->setName( "child" );
        data->setProperty( "uuid", StringUtil::getUUID() ); data->setProperty( "label", name );
        data->setProperty( "static", false ); data->setProperty( "enabled", true );
        data->addChild( pose( "localTransform", Vector3F( x, 0, 0 ) ) );
        auto mesh = make_ptr<Properties>(); mesh->setName( "component" );
        mesh->setProperty( "uuid", StringUtil::getUUID() );
        mesh->setProperty( "componentType", "workphone::scene::Mesh" );
        mesh->setProperty( "Mesh Path", "building.fbmeshbin" );
        auto renderer = make_ptr<Properties>(); renderer->setName( "component" );
        renderer->setProperty( "uuid", StringUtil::getUUID() );
        renderer->setProperty( "componentType", "workphone::scene::MeshRenderer" );
        renderer->setProperty( "sourceMaterial", "unchanged-material-uuid" );
        auto metadata = make_ptr<Properties>(); metadata->setName( "appearance" );
        metadata->setProperty( "variant", "city-brick" ); renderer->addChild( metadata );
        data->addChild( mesh ); data->addChild( renderer ); return data;
    }

    inline SmartPtr<Properties> source()
    {
        auto root = make_ptr<Properties>(); root->setName( "importedPrefab" );
        root->setProperty( "uuid", StringUtil::getUUID() ); root->setProperty( "label", "Imported city" );
        root->setProperty( "enabled", true ); root->setProperty( "static", false );
        root->addChild( pose( "localTransform", Vector3F::zero() ) );
        root->addChild( pose( "worldTransform", Vector3F::zero() ) );
        root->addChild( leaf( "West building", -1000 ) ); root->addChild( leaf( "East building", 1000 ) );
        return root;
    }

    inline void building( const fs::path &root )
    {
        Array<Vector3<real_Num>> positions = { {0,0,0}, {10,0,0}, {10,20,0}, {0,20,0},
                                              {0,0,10}, {10,0,10}, {10,20,10}, {0,20,10} };
        Array<Vector3<real_Num>> normals( positions.size(), Vector3<real_Num>( 0, 1, 0 ) );
        Array<Vector2<real_Num>> uv( positions.size(), Vector2<real_Num>( 0, 0 ) );
        Array<u32> indices = { 0,1,2, 0,2,3, 4,6,5, 4,7,6, 0,4,5, 0,5,1,
                              3,2,6, 3,6,7, 0,3,7, 0,7,4, 1,5,6, 1,6,2 };
        auto mesh = dynamic_pointer_cast<workphone::Mesh>( MeshUtil::createMesh( positions, normals, uv, indices ) );
        BOOST_REQUIRE( mesh ); mesh->updateAABB( true );
        MeshSerializer serializer;
        serializer.exportMesh( mesh.get(), String( ( root / "building.fbmeshbin" ).generic_string().c_str() ) );
    }

    inline SmartPtr<Properties> roundTrip( const Properties &data )
    {
        auto result = make_ptr<Properties>(); String error;
        BOOST_REQUIRE_MESSAGE( PropertiesBinarySerializer::deserialize(
            PropertiesBinarySerializer::serialize( data ), *result, &error ), error );
        return result;
    }

    inline bool build( const Properties &sourceData, const MeshResourceDirector::StreamingOptions &options,
                       const fs::path &root, const fs::path &output, SmartPtr<Properties> &prefab,
                       String &manifest, String &error )
    {
        return MeshImportStreaming::buildPrefab( sourceData, options, root.generic_string().c_str(),
            output.generic_string().c_str(), prefab, manifest, error );
    }
}

BOOST_AUTO_TEST_SUITE( MeshImportStreamingTests )

BOOST_AUTO_TEST_CASE( disabled_import_is_an_independent_exact_copy_and_writes_nothing )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard; const auto root = project( guard ), output = generation( root );
    auto data = source(); const auto original = PropertiesBinarySerializer::serialize( *data );
    SmartPtr<Properties> prefab; String manifest = "previous", error;
    BOOST_REQUIRE_MESSAGE( build( *data, {}, root, output, prefab, manifest, error ), error );
    BOOST_REQUIRE( prefab ); BOOST_CHECK( prefab != data ); BOOST_CHECK( manifest.empty() );
    BOOST_CHECK( !fs::exists( output ) ); BOOST_CHECK( !fs::exists( root / "Cache" ) );
    BOOST_CHECK( PropertiesBinarySerializer::serialize( *prefab ) == original );
    prefab->getChild( "child" )->setProperty( "label", "Changed copy" );
    BOOST_CHECK( PropertiesBinarySerializer::serialize( *data ) == original );
}

BOOST_AUTO_TEST_CASE( import_director_streaming_options_are_opt_in_and_persist_all_fields )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard; MeshResourceDirector director;
    BOOST_CHECK( !director.getStreamingOptions().enabled );
    director.setProperties( make_ptr<Properties>() );
    BOOST_CHECK( !director.getStreamingOptions().enabled );
    auto options = director.getStreamingOptions(); options.enabled = true; options.cellSize = 128;
    options.maxActorsPerCell = 17; options.loadRadius = 400; options.retainRadius = 900;
    options.enableHlod = false; options.previewEditorCamera = false; director.setStreamingOptions( options );
    MeshResourceDirector loaded; loaded.setProperties( roundTrip( *director.getProperties() ) );
    const auto &result = loaded.getStreamingOptions();
    BOOST_CHECK( result.enabled ); BOOST_CHECK_EQUAL( result.cellSize, 128.0f );
    BOOST_CHECK_EQUAL( result.maxActorsPerCell, 17u ); BOOST_CHECK_EQUAL( result.loadRadius, 400.0f );
    BOOST_CHECK_EQUAL( result.retainRadius, 900.0f ); BOOST_CHECK( !result.enableHlod );
    BOOST_CHECK( !result.previewEditorCamera );
    options.cellSize = std::numeric_limits<float>::quiet_NaN(); options.maxActorsPerCell = 0;
    options.loadRadius = 20; options.retainRadius = 1; loaded.setStreamingOptions( options );
    BOOST_CHECK_EQUAL( loaded.getStreamingOptions().cellSize, 256.0f );
    BOOST_CHECK_EQUAL( loaded.getStreamingOptions().maxActorsPerCell, 1u );
    BOOST_CHECK_EQUAL( loaded.getStreamingOptions().retainRadius, 20.0f );
    options.maxActorsPerCell = ( std::numeric_limits<u32>::max )();
    loaded.setStreamingOptions( options );
    BOOST_CHECK_EQUAL( loaded.getStreamingOptions().maxActorsPerCell, u32( WP_MAX_ACTORS - 1 ) );
}

BOOST_AUTO_TEST_CASE( enabled_import_cooks_cells_and_round_trips_a_thin_portable_prefab )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard; const auto root = project( guard ), output = generation( root ); building( root );
    auto data = source(); const auto original = PropertiesBinarySerializer::serialize( *data );
    MeshResourceDirector::StreamingOptions options; options.enabled = true; options.maxActorsPerCell = 1;
    options.retainRadius = 5000;
    SmartPtr<Properties> prefab; String manifestPath, error;
    BOOST_REQUIRE_MESSAGE( build( *data, options, root, output, prefab, manifestPath, error ), error );
    BOOST_REQUIRE( prefab ); BOOST_CHECK( PropertiesBinarySerializer::serialize( *data ) == original );
    BOOST_CHECK_EQUAL( prefab->getName(), data->getName() ); BOOST_CHECK( !prefab->hasChild( "child" ) );
    BOOST_CHECK( !prefab->hasChild( "actors" ) );
    bool value = false; BOOST_REQUIRE( prefab->getPropertyValue( "meshImportStreaming", value ) ); BOOST_CHECK( value );
    prefab = roundTrip( *prefab ); auto component = prefab->getChild( "component" ); BOOST_REQUIRE( component );
    BOOST_CHECK_EQUAL( component->getProperty( "componentType" ), "workphone::scene::WorldStreaming" );
    const auto relative = component->getProperty( "World Manifest" );
    BOOST_CHECK( !fs::path( relative.c_str() ).is_absolute() );
    BOOST_CHECK( fs::equivalent( root / relative.c_str(), fs::path( manifestPath.c_str() ) ) );
    for( const auto &name : { "Instance World", "Follow Active Camera", "Preview Editor Camera", "Enable HLOD" } )
    { value = false; BOOST_REQUIRE( component->getPropertyValue( name, value ) ); BOOST_CHECK( value ); }
    float proxyLoad = 0, proxyRetain = 0;
    component->getPropertyValue( "Proxy Load Radius", proxyLoad );
    component->getPropertyValue( "Proxy Retain Radius", proxyRetain );
    BOOST_CHECK( proxyLoad > options.retainRadius ); BOOST_CHECK( proxyRetain > proxyLoad );
    u32 actors = 0; component->getPropertyValue( "Maximum Resident Actors", actors );
    BOOST_CHECK_GE( actors, options.maxActorsPerCell );
    WorldManifest manifest; BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( manifestPath, manifest, error ), error );
    u32 details = 0, proxies = 0;
    std::map<String, SmartPtr<Properties>> expected;
    for( const auto &child : data->getChildrenByName( "child" ) ) expected[child->getProperty( "uuid" )] = child;
    for( const auto &cell : manifest.cells )
    {
        BOOST_CHECK_LE( cell.actors, 1u );
        if( cell.detailKey ) { ++proxies; continue; }
        ++details; BOOST_CHECK( cell.id != "persistent" );
        auto payload = SceneWorld::readCell( manifestPath, cell, error ); BOOST_REQUIRE_MESSAGE( payload, error );
        auto actor = payload->getChild( "actors" ); BOOST_REQUIRE( actor );
        const auto found = expected.find( actor->getProperty( "uuid" ) ); BOOST_REQUIRE( found != expected.end() );
        const auto before = found->second->getChildrenByName( "component" );
        const auto after = actor->getChildrenByName( "component" ); BOOST_REQUIRE_EQUAL( before.size(), after.size() );
        for( size_t i = 0; i < before.size(); ++i )
            BOOST_CHECK( PropertiesBinarySerializer::serialize( *before[i] ) == PropertiesBinarySerializer::serialize( *after[i] ) );
    }
    BOOST_CHECK_EQUAL( details, 2u ); BOOST_CHECK_EQUAL( proxies, 2u );
}

BOOST_AUTO_TEST_CASE( import_status_is_read_only_transient_and_ignores_older_completions )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard;
    auto director = make_ptr<MeshResourceDirector>();
    const auto first = director->beginImport();
    BOOST_CHECK_EQUAL( director->getImportStatus(), "Import queued." );
    const auto current = director->beginImport();
    director->setImportStatus( first, "Old request failed" );
    BOOST_CHECK_EQUAL( director->getImportStatus(), "Import queued." );
    std::thread completion( [director, current]() mutable {
        director->setImportStatus( current, "Import completed on worker" );
    } );
    completion.join();
    auto properties = director->getProperties();
    BOOST_CHECK_EQUAL( properties->getProperty( ResourceDirector::importStatusStr ), "Import completed on worker" );
    BOOST_CHECK( properties->getPropertyObject( ResourceDirector::importStatusStr ).isReadOnly() );
    BOOST_CHECK_EQUAL( properties->getPropertyObject( ResourceDirector::importStatusStr ).getAttribute( "description" ),
                       "Hover to read the complete import status." );
    properties->setProperty( ResourceDirector::importStatusStr, "Forged completion" );
    director->setProperties( properties );
    BOOST_CHECK_EQUAL( director->getImportStatus(), "Import completed on worker" );
    auto saved = dynamic_pointer_cast<Properties>( director->toData() );
    BOOST_REQUIRE( saved );
    BOOST_CHECK( !saved->hasProperty( ResourceDirector::importStatusStr ) );
    const auto directory = project( guard );
    const auto savedPath = directory / "import.resourcedata";
    director->setFilePath( savedPath.generic_string().c_str() );
    director->save();
    std::ifstream input( savedPath, std::ios::binary );
    BOOST_REQUIRE( input );
    const std::string savedText( ( std::istreambuf_iterator<char>( input ) ), std::istreambuf_iterator<char>() );
    BOOST_CHECK( savedText.find( ResourceDirector::importStatusStr.c_str() ) == std::string::npos );
    BOOST_CHECK_EQUAL( director->getImportStatus(), "Import completed on worker" );
    MeshResourceDirector restored;
    restored.setProperties( properties );
    BOOST_CHECK( restored.getImportStatus().empty() );
    director->setResourcePath( "another-source.obj" );
    director->setImportStatus( current, "Late completion for previous source" );
    BOOST_CHECK( director->getImportStatus().empty() );
    const auto afterPathChange = director->beginImport();
    director->unload( nullptr );
    director->setImportStatus( afterPathChange, "Late completion after unload" );
    BOOST_CHECK( director->getImportStatus().empty() );
}

BOOST_AUTO_TEST_CASE( retained_import_status_admits_capacity_and_rolls_back_denied_replacement )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard;
    auto director = make_ptr<MeshResourceDirector>();
    const auto request = director->beginImport();
    const String oldStatus( 4096, 'a' ), nextStatus( 8192, 'b' );
    auto scheduler = WorldStreamingScheduler::shared();
    const auto index = size_t( StreamMemoryClass::SourceCpu );
    const auto before = scheduler->stats();
    director->setImportStatus( request, oldStatus );
    const auto oldCharge = scheduler->stats().used[index] - before.used[index];
    BOOST_REQUIRE_GT( oldCharge, 0u );
    u64 nextCharge = 0;
    {
        const auto used = scheduler->stats().used[index];
        SourceBudgetString probe( nextStatus );
        nextCharge = scheduler->stats().used[index] - used;
    }
    BOOST_REQUIRE_GT( nextCharge, 0u );
    struct RestoreStatusBudget
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
        ~RestoreStatusBudget() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
    } restore{ scheduler, before.limits[index] };
    String error;
    BOOST_REQUIRE( scheduler->setBudget( StreamMemoryClass::SourceCpu,
        before.used[index] + ( std::max )( oldCharge, nextCharge ), error ) );
    bool denied = false;
    try { director->setImportStatus( request, nextStatus ); }
    catch( const std::bad_alloc & ) { denied = true; }
    BOOST_CHECK( denied );
    BOOST_CHECK_EQUAL( director->getImportStatus(), oldStatus );
    BOOST_CHECK_EQUAL( scheduler->stats().used[index], before.used[index] + oldCharge );
    BOOST_REQUIRE( scheduler->setBudget( StreamMemoryClass::SourceCpu, before.limits[index], error ) );
    director->setImportStatus( request, nextStatus );
    BOOST_CHECK_EQUAL( scheduler->stats().used[index], before.used[index] + nextCharge );
    BOOST_CHECK_EQUAL( director->getImportStatus(), nextStatus );
    BOOST_REQUIRE( scheduler->setBudget( StreamMemoryClass::SourceCpu, scheduler->stats().used[index], error ) );
    director->unload( nullptr );
    BOOST_CHECK_EQUAL( scheduler->stats().used[index], before.used[index] );
}

BOOST_AUTO_TEST_CASE( long_import_status_keeps_full_value_without_overflowing_property_metadata )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard;
    auto director = make_ptr<MeshResourceDirector>();
    const auto request = director->beginImport();
    const String status( 1024, 'x' );
    director->setImportStatus( request, status );
    const auto properties = director->getProperties();
    BOOST_REQUIRE( properties );
    BOOST_CHECK_EQUAL( properties->getProperty( ResourceDirector::importStatusStr ), status );
    const auto description = properties->getPropertyObject( ResourceDirector::importStatusStr ).getAttribute( "description" );
    BOOST_CHECK_LE( description.size(), 255u );
    BOOST_CHECK_EQUAL( description, "Hover to read the complete import status." );
    BOOST_CHECK_EQUAL( properties->getPropertyObject( ResourceDirector::importStatusStr ).getAttribute( "valueTooltip" ), "true" );
}

BOOST_AUTO_TEST_CASE( persistent_actor_limit_reports_actual_count_and_keeps_previous_generation )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard; const auto root = project( guard ); building( root );
    auto data = source();
    while( data->hasChild( "child" ) ) data->removeChild( "child" );
    for( unsigned i = 0; i < 31; ++i )
        data->addChild( leaf( "Persistent building " + StringUtil::toString( i ), float( i * 1000 ) ) );
    // A non-render component keeps this 32-actor hierarchy indivisible.
    auto script = make_ptr<Properties>(); script->setName( "component" );
    script->setProperty( "uuid", StringUtil::getUUID() );
    script->setProperty( "componentType", "workphone::scene::Script" );
    data->addChild( script );
    MeshResourceDirector::StreamingOptions options; options.enabled = true;
    options.enableHlod = false; options.maxActorsPerCell = 64;
    SmartPtr<Properties> prefab; String manifest, error;
    BOOST_REQUIRE_MESSAGE( build( *data, options, root, generation( root ), prefab, manifest, error ), error );
    const auto previous = prefab;
    const auto previousManifest = manifest;
    const auto previousPrefabBytes = PropertiesBinarySerializer::serialize( *prefab );
    WorldManifest before; BOOST_REQUIRE( SceneWorld::readManifest( manifest, before, error ) );
    BOOST_REQUIRE_EQUAL( before.cells.size(), 1u );
    BOOST_CHECK_EQUAL( before.cells.front().actors, 32u );
    options.maxActorsPerCell = 20;
    BOOST_CHECK( !build( *data, options, root, generation( root ), prefab, manifest, error ) );
    for( const auto &part : { "persistent", "32 actors", "limit of 20", "at least 32" } )
        BOOST_CHECK_MESSAGE( error.find( part ) != String::npos, error );
    BOOST_CHECK( prefab == previous );
    BOOST_CHECK_EQUAL( manifest, previousManifest );
    BOOST_CHECK( PropertiesBinarySerializer::serialize( *prefab ) == previousPrefabBytes );
    WorldManifest retained; BOOST_REQUIRE( SceneWorld::readManifest( manifest, retained, error ) );
    BOOST_CHECK_EQUAL( retained.generation, before.generation );
    for( const auto &cell : retained.cells )
        BOOST_CHECK_MESSAGE( SceneWorld::readCell( manifest, cell, error ), error );
    options.maxActorsPerCell = 32;
    BOOST_REQUIRE_MESSAGE( build( *data, options, root, generation( root ), prefab, manifest, error ), error );
    BOOST_CHECK( manifest != previousManifest );
}

BOOST_AUTO_TEST_CASE( transformed_root_is_kept_only_on_the_streaming_prefab_owner )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard; const auto root = project( guard ), output = generation( root ); building( root );
    auto data = source();
    for( const auto &name : { "localTransform", "worldTransform" } )
    {
        data->removeChild( name );
        data->addChild( pose( name, Vector3F( 5000, 100, -2000 ),
            QuaternionF::angleAxis( 0.7f, Vector3F::unitY() ), Vector3F( 2, 3, 4 ) ) );
    }
    // Keep an indivisible persistent subtree, so descendants' stored world
    // transforms are exercised as well as the cooker's flattened leaf roots.
    auto animator = make_ptr<Properties>(); animator->setName( "component" );
    animator->setProperty( "uuid", StringUtil::getUUID() );
    animator->setProperty( "componentType", "workphone::scene::Animator" ); data->addChild( animator );
    for( auto child : data->getChildrenByName( "child" ) )
        child->addChild( pose( "worldTransform", Vector3F( 99999, 88888, 77777 ),
            QuaternionF::angleAxis( 0.7f, Vector3F::unitY() ), Vector3F( 2, 3, 4 ) ) );
    const auto original = PropertiesBinarySerializer::serialize( *data );
    MeshResourceDirector::StreamingOptions options; options.enabled = true; options.enableHlod = false;
    SmartPtr<Properties> prefab; String manifestPath, error;
    BOOST_REQUIRE_MESSAGE( build( *data, options, root, output, prefab, manifestPath, error ), error );
    for( const auto &name : { "localTransform", "worldTransform" } )
        BOOST_CHECK( PropertiesBinarySerializer::serialize( *prefab->getChild( name ) ) ==
                     PropertiesBinarySerializer::serialize( *data->getChild( name ) ) );
    WorldManifest manifest; BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( manifestPath, manifest, error ), error );
    BOOST_REQUIRE_EQUAL( manifest.cells.size(), 1u ); BOOST_CHECK_EQUAL( manifest.cells.front().id, "persistent" );
    for( const auto &cell : manifest.cells )
    {
        auto payload = SceneWorld::readCell( manifestPath, cell, error ); BOOST_REQUIRE_MESSAGE( payload, error );
        auto cookedRoot = payload->getChild( "actors" ); BOOST_REQUIRE( cookedRoot );
        Vector3F rootPosition; cookedRoot->getChild( "localTransform" )->getPropertyValue( "Position", rootPosition );
        BOOST_CHECK( rootPosition == Vector3F::zero() );
        for( const auto &actor : cookedRoot->getChildrenByName( "child" ) )
        {
            Vector3F position; actor->getChild( "localTransform" )->getPropertyValue( "Position", position );
            BOOST_CHECK_EQUAL( position.y, 0 ); BOOST_CHECK_EQUAL( position.z, 0 );
            BOOST_CHECK( position.x == -1000 || position.x == 1000 );
            Vector3F scale; actor->getChild( "localTransform" )->getPropertyValue( "Scale", scale );
            BOOST_CHECK( scale == Vector3F::unit() );
            Vector3F worldPosition; actor->getChild( "worldTransform" )->getPropertyValue( "Position", worldPosition );
            BOOST_CHECK( worldPosition == position );
            Vector3F worldScale; actor->getChild( "worldTransform" )->getPropertyValue( "Scale", worldScale );
            BOOST_CHECK( worldScale == Vector3F::unit() );
        }
    }
    BOOST_CHECK( PropertiesBinarySerializer::serialize( *data ) == original );
}

BOOST_AUTO_TEST_CASE( failed_cooks_and_invalid_paths_preserve_previous_output_arguments )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard; const auto root = project( guard ); auto data = source();
    MeshResourceDirector::StreamingOptions options; options.enabled = true;
    auto previous = make_ptr<Properties>(); previous->setProperty( "published", "unchanged" );
    SmartPtr<Properties> prefab = previous; String manifest = "published/world.wpworld", error;
    const auto outside = root.parent_path() / ( "wp-rejected-" + std::string( StringUtil::getUUID().c_str() ) );
    BOOST_CHECK( !build( *data, options, root, outside, prefab, manifest, error ) );
    BOOST_CHECK( !error.empty() ); BOOST_CHECK( !fs::exists( outside ) );
    BOOST_CHECK( prefab == previous ); BOOST_CHECK_EQUAL( manifest, "published/world.wpworld" );
    const auto invalid = generation( root ); data->removeProperty( "uuid" );
    BOOST_CHECK( !build( *data, options, root, invalid, prefab, manifest, error ) );
    BOOST_CHECK( !error.empty() ); BOOST_CHECK( !fs::exists( invalid ) );
    BOOST_CHECK( prefab == previous ); BOOST_CHECK_EQUAL( manifest, "published/world.wpworld" );
    data = source(); auto children = data->getChildrenByName( "child" );
    children.back()->setProperty( "uuid", children.front()->getProperty( "uuid" ) );
    BOOST_CHECK( !build( *data, options, root, generation( root ), prefab, manifest, error ) );
    BOOST_CHECK( !error.empty() ); BOOST_CHECK( prefab == previous );
    BOOST_CHECK_EQUAL( manifest, "published/world.wpworld" );
}

BOOST_AUTO_TEST_CASE( binary_city_properties_preserve_many_strings_across_buffer_growth )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard;
    auto source = make_ptr<Properties>(); source->setName( u8"City-\u03a9" );
    String value( 1024, 'x' ); value[31] = '\0'; value[123] = char( 0xc3 ); value[124] = char( 0xa9 );
    const String attribute( value.data() + 24, 104 );
    // Thousands of independent strings cross many append-capacity boundaries;
    // embedded NUL and UTF-8 bytes must remain length-delimited format-v1 data.
    for( u32 district = 0; district < 256; ++district )
    {
        auto child = make_ptr<Properties>(); child->setName( "District-" + StringUtil::toString( district ) );
        for( u32 field = 0; field < 12; ++field )
        {
            value[0] = char( 'A' + district % 26 ); value[1] = char( '0' + field % 10 );
            Property property( "field-" + StringUtil::toString( field ), value, "string" );
            property.setReadOnly( field % 2 != 0 ); property.setAttribute( "metadata", attribute );
            child->addProperty( property );
        }
        source->addChild( child );
    }
    const auto bytes = PropertiesBinarySerializer::serialize( *source );
    BOOST_CHECK_GT( bytes.size(), size_t( 3 * 1024 * 1024 ) );
    BOOST_CHECK( PropertiesBinarySerializer::hasBinaryHeader( bytes.data(), bytes.size() ) );
    auto decoded = make_ptr<Properties>(); String error;
    BOOST_REQUIRE_MESSAGE( PropertiesBinarySerializer::deserialize( bytes, *decoded, &error ), error );
    BOOST_CHECK_EQUAL( decoded->getName(), source->getName() );
    const auto originalChildren = source->getChildren(), decodedChildren = decoded->getChildren();
    BOOST_REQUIRE_EQUAL( decodedChildren.size(), originalChildren.size() );
    for( size_t district = 0; district < originalChildren.size(); ++district )
    {
        BOOST_CHECK_EQUAL( decodedChildren[district]->getName(), originalChildren[district]->getName() );
        const auto before = originalChildren[district]->getPropertiesAsArray();
        const auto after = decodedChildren[district]->getPropertiesAsArray();
        BOOST_REQUIRE_EQUAL( after.size(), before.size() );
        for( size_t field = 0; field < before.size(); ++field )
        {
            BOOST_CHECK_EQUAL( after[field].getName(), before[field].getName() );
            BOOST_CHECK( after[field].getValue() == before[field].getValue() );
            BOOST_CHECK_EQUAL( after[field].getTypeName(), before[field].getTypeName() );
            BOOST_CHECK_EQUAL( after[field].isReadOnly(), before[field].isReadOnly() );
            const auto beforeAttributes = before[field].getAttributes();
            const auto afterAttributes = after[field].getAttributes();
            BOOST_REQUIRE_EQUAL( afterAttributes.size(), beforeAttributes.size() );
            for( size_t attributeIndex = 0; attributeIndex < beforeAttributes.size(); ++attributeIndex )
            {
                BOOST_CHECK( afterAttributes[attributeIndex].first == beforeAttributes[attributeIndex].first );
                BOOST_CHECK( afterAttributes[attributeIndex].second == beforeAttributes[attributeIndex].second );
            }
        }
    }
    BOOST_CHECK( PropertiesBinarySerializer::serialize( *decoded ) == bytes );
}

BOOST_AUTO_TEST_CASE( unicode_project_and_generation_paths_cook_and_reload_hlod )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard; const auto base = project( guard ); building( base );
    const auto root = base / fs::u8path( u8"city-\u57ce-\u00e9" ); fs::create_directories( root );
    const String meshName = u8"building-\u57ce-\u00e9.fbmeshbin";
    fs::copy_file( base / "building.fbmeshbin", root / fs::u8path( meshName.c_str() ) );
    auto data = source();
    for( const auto &child : data->getChildrenByName( "child" ) )
        child->getChild( "component" )->setProperty( "Mesh Path", meshName );
    const auto original = PropertiesBinarySerializer::serialize( *data );
    const auto output = root / "Cache" / fs::u8path( u8"generation-\u8857-\u00f6" );
    MeshResourceDirector::StreamingOptions options; options.enabled = true; options.maxActorsPerCell = 1;
    auto previous = make_ptr<Properties>(); previous->setProperty( "previous", "published" );
    SmartPtr<Properties> prefab = previous; String manifest = "previous/world.wpworld", error;
    BOOST_REQUIRE_MESSAGE( MeshImportStreaming::buildPrefab( *data, options,
        String( root.generic_u8string().c_str() ), String( output.generic_u8string().c_str() ),
        prefab, manifest, error ), error );
    BOOST_CHECK( prefab != previous ); BOOST_CHECK( fs::is_regular_file( fs::u8path( manifest.c_str() ) ) );
    const auto expectedManifest = fs::canonical( output / "world.wpworld" ).generic_u8string();
    BOOST_CHECK_EQUAL( manifest, expectedManifest.c_str() );
    auto component = prefab->getChild( "component" ); BOOST_REQUIRE( component );
    const auto relative = component->getProperty( "World Manifest" );
    BOOST_CHECK( relative.find( u8"generation-\u8857-\u00f6" ) != String::npos );
    BOOST_CHECK( fs::equivalent( root / fs::u8path( relative.c_str() ), fs::u8path( manifest.c_str() ) ) );
    WorldManifest cooked; BOOST_REQUIRE_MESSAGE( SceneWorld::readManifest( manifest, cooked, error ), error );
    u32 proxies = 0;
    for( const auto &cell : cooked.cells )
    {
        auto payload = SceneWorld::readCell( manifest, cell, error ); BOOST_REQUIRE_MESSAGE( payload, error );
        if( !cell.detailKey ) continue;
        ++proxies; auto proxy = payload->getChild( "actors" ); BOOST_REQUIRE( proxy );
        const auto proxyPath = proxy->getChild( "component" )->getProperty( "Mesh Path" );
        const auto file = root / fs::u8path( proxyPath.c_str() );
        BOOST_CHECK( proxyPath.find( u8"generation-\u8857-\u00f6" ) != String::npos );
        BOOST_REQUIRE( fs::is_regular_file( file ) ); BOOST_CHECK_GT( fs::file_size( file ), 0u );
    }
    BOOST_CHECK_EQUAL( proxies, 2u );
    BOOST_CHECK( PropertiesBinarySerializer::serialize( *data ) == original );

    auto owner = guard.createBasicActor(); BOOST_REQUIRE( owner );
    owner->getHandle()->setUUID( StringUtil::getUUID() );
    owner->setLocalPosition( Vector3<real_Num>( 100000000, 0, 100000000 ) );
    guard.scene->addActor( owner );
    auto scheduler = std::make_shared<WorldStreamingScheduler>(); SceneWorldStreamer stream( scheduler );
    BOOST_REQUIRE_MESSAGE( stream.open( manifest, guard.scene, {}, owner, error ), error );
    BOOST_CHECK_GT( stream.statistics().manifestBytes, 0u );
    BOOST_REQUIRE_MESSAGE( stream.update( { Vector3F( 100000000, 0, 100000000 ) }, error ), error );
    stream.close();

    // Exercise the generated relative reference through the component's actual
    // project-path resolver as well as direct streamer and cell reads above.
    const auto previousProject = guard.applicationManager->getProjectPath();
    const auto previousTask = Thread::getCurrentTask();
    guard.addCleanup( [app = guard.applicationManager, previousProject, previousTask]() mutable {
        app->setProjectPath( previousProject ); Thread::setCurrentTask( previousTask );
    } );
    guard.applicationManager->setProjectPath( root.generic_u8string().c_str() );
    Thread::setCurrentTask( guard.sceneManager->getSceneTask() );
    auto runtime = owner->addComponent<WorldStreaming>(); BOOST_REQUIRE( runtime );
    auto settings = runtime->getProperties(); settings->setProperty( "World Manifest", relative );
    settings->setProperty( "Instance World", true ); settings->setProperty( "Enable HLOD", false );
    settings->setProperty( "Follow Active Camera", false ); settings->setProperty( "Preview Editor Camera", false );
    runtime->setProperties( settings ); runtime->preUpdate();
    BOOST_CHECK_GT( runtime->statistics().manifestBytes, 0u );
    runtime->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( allocation_denial_preserves_both_previous_prefab_outputs )
{
    using namespace mesh_import_streaming_tests;
    TestGuard guard; const auto root = project( guard ), output = generation( root );
    auto data = source(); auto previous = make_ptr<Properties>(); previous->setProperty( "published", "old" );
    SmartPtr<Properties> prefab = previous; String manifest = "previous/world.wpworld", error;
    error.reserve( 1024 );
    // Prewarm diagnostic storage before denying new SourceCpu admission.
    { String diagnostic = "Mesh import streaming: bad allocation"; }
    const String rootText( root.generic_u8string().c_str() ), outputText( output.generic_u8string().c_str() );
    MeshResourceDirector::StreamingOptions options; options.enabled = true;
    auto scheduler = WorldStreamingScheduler::shared(); const auto stats = scheduler->stats();
    const auto previousLimit = stats.limits[size_t( StreamMemoryClass::SourceCpu )];
    struct RestoreLimit
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler; u64 limit;
        ~RestoreLimit() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
    } restore{ scheduler, previousLimit };
    BOOST_REQUIRE_MESSAGE( scheduler->setBudget( StreamMemoryClass::SourceCpu,
        stats.used[size_t( StreamMemoryClass::SourceCpu )], error ), error );
    const auto deniedBefore = sourceBudgetDenialCount();
    const auto succeeded = MeshImportStreaming::buildPrefab( *data, options, rootText, outputText,
                                                            prefab, manifest, error );
    const auto deniedAfter = sourceBudgetDenialCount();
    String restoreError; BOOST_REQUIRE( scheduler->setBudget( StreamMemoryClass::SourceCpu, previousLimit, restoreError ) );
    BOOST_CHECK( !succeeded ); BOOST_CHECK_GT( deniedAfter, deniedBefore ); BOOST_CHECK( !error.empty() );
    BOOST_CHECK( prefab == previous ); BOOST_CHECK_EQUAL( manifest, "previous/world.wpworld" );
    BOOST_CHECK( !fs::exists( output ) );
}

BOOST_AUTO_TEST_SUITE_END()
#endif
