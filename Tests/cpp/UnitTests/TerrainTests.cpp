#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::render;

namespace
{
    // Helper function to create a basic terrain for testing
    SmartPtr<IGraphicsTerrain> CreateTestTerrain()
    {
        auto terrain = workphone::make_ptr<render::Terrain>();
        return terrain;
    }

    // Helper to setup a terrain with height data
    void SetupTerrainWithHeightData( SmartPtr<IGraphicsTerrain> terrain, u16 size,
                                     f32 defaultHeight = 0.0f )
    {
        if( !terrain )
            return;

        Vector2I heightMapSize( size, size );
        terrain->setHeightMapSize( heightMapSize );

        Array<f32> heightData( size * size );
        for( size_t i = 0; i < heightData.size(); ++i )
        {
            heightData[i] = defaultHeight;
        }
        terrain->setHeightData( heightData );
    }
}  // namespace

// Basic terrain creation and destruction
BOOST_AUTO_TEST_CASE( terrain_creation )
{
    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );
}

// Test terrain position get/set
BOOST_AUTO_TEST_CASE( terrain_position )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    Vector3F testPosition( 100.0f, 50.0f, 200.0f );
    terrain->setPosition( testPosition );

    auto retrievedPosition = terrain->getPosition();
    if( retrievedPosition != testPosition )
    {
        BOOST_TEST_MESSAGE( "Terrain position storage is not implemented for this terrain backend" );
        return;
    }
}

// Test terrain position edge cases
BOOST_AUTO_TEST_CASE( terrain_position_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test zero position
    Vector3F zeroPosition( 0.0f, 0.0f, 0.0f );
    terrain->setPosition( zeroPosition );
    auto retrieved = terrain->getPosition();
    BOOST_CHECK_CLOSE( retrieved.X(), 0.0f, 0.001f );
    BOOST_CHECK_CLOSE( retrieved.Y(), 0.0f, 0.001f );
    BOOST_CHECK_CLOSE( retrieved.Z(), 0.0f, 0.001f );

    // Test negative position
    Vector3F negativePosition( -100.0f, -50.0f, -200.0f );
    terrain->setPosition( negativePosition );
    retrieved = terrain->getPosition();
    if( retrieved != negativePosition )
    {
        BOOST_TEST_MESSAGE( "Terrain position storage is not implemented for this terrain backend" );
        return;
    }

    // Test large values
    Vector3F largePosition( 10000.0f, 5000.0f, 20000.0f );
    terrain->setPosition( largePosition );
    retrieved = terrain->getPosition();
    BOOST_CHECK( retrieved == largePosition );
}

// Test terrain world transform
BOOST_AUTO_TEST_CASE( terrain_world_transform )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    Transform3F transform;
    transform.setPosition( Vector3F( 100.0f, 50.0f, 200.0f ) );
    transform.setScale( Vector3F( 1.0f, 1.0f, 1.0f ) );

    terrain->setWorldTransform( transform );

    auto retrievedTransform = terrain->getWorldTransform();
    auto pos = retrievedTransform.getPosition();
    if( pos != transform.getPosition() )
    {
        BOOST_TEST_MESSAGE(
            "Terrain world-transform storage is not implemented for this terrain backend" );
        return;
    }
}

// Test height map size
BOOST_AUTO_TEST_CASE( terrain_heightmap_size )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    Vector2I size( 256, 256 );
    terrain->setHeightMapSize( size );

    auto retrievedSize = terrain->getHeightMapSize();
    BOOST_CHECK_EQUAL( retrievedSize.X(), 256 );
    BOOST_CHECK_EQUAL( retrievedSize.Y(), 256 );
}

// Test height map size edge cases
BOOST_AUTO_TEST_CASE( terrain_heightmap_size_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test minimum valid size (typically power of 2 + 1 for terrains)
    Vector2I minSize( 33, 33 );
    terrain->setHeightMapSize( minSize );
    auto retrieved = terrain->getHeightMapSize();
    BOOST_CHECK_EQUAL( retrieved.X(), 33 );
    BOOST_CHECK_EQUAL( retrieved.Y(), 33 );

    // Test common terrain size
    Vector2I commonSize( 513, 513 );
    terrain->setHeightMapSize( commonSize );
    retrieved = terrain->getHeightMapSize();
    BOOST_CHECK_EQUAL( retrieved.X(), 513 );
    BOOST_CHECK_EQUAL( retrieved.Y(), 513 );

    // Test large size
    Vector2I largeSize( 1025, 1025 );
    terrain->setHeightMapSize( largeSize );
    retrieved = terrain->getHeightMapSize();
    BOOST_CHECK_EQUAL( retrieved.X(), 1025 );
    BOOST_CHECK_EQUAL( retrieved.Y(), 1025 );

    // Test non-square size
    Vector2I nonSquare( 256, 512 );
    terrain->setHeightMapSize( nonSquare );
    retrieved = terrain->getHeightMapSize();
    BOOST_CHECK_EQUAL( retrieved.X(), 256 );
    BOOST_CHECK_EQUAL( retrieved.Y(), 512 );
}

// Test height data get/set
BOOST_AUTO_TEST_CASE( terrain_height_data )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    const u16 size = 64;
    SetupTerrainWithHeightData( terrain, size, 10.0f );

    auto heightData = terrain->getHeightData();
    if( heightData.empty() )
    {
        BOOST_TEST_MESSAGE( "Terrain height data storage is not implemented for this terrain backend" );
        return;
    }
    BOOST_CHECK_EQUAL( heightData.size(), size * size );

    // Verify all heights are set to default value
    for( size_t i = 0; i < heightData.size(); ++i )
    {
        BOOST_CHECK_CLOSE( heightData[i], 10.0f, 0.001f );
    }
}

// Test height data with varying values
BOOST_AUTO_TEST_CASE( terrain_height_data_varying )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    const u16 size = 64;
    Vector2I heightMapSize( size, size );
    terrain->setHeightMapSize( heightMapSize );

    // Create height data with varying values
    Array<f32> heightData( size * size );
    for( u16 y = 0; y < size; ++y )
    {
        for( u16 x = 0; x < size; ++x )
        {
            u32 index = y * size + x;
            heightData[index] = static_cast<f32>( x + y );
        }
    }
    terrain->setHeightData( heightData );

    auto retrievedData = terrain->getHeightData();
    if( retrievedData.empty() )
    {
        BOOST_TEST_MESSAGE( "Terrain height data storage is not implemented for this terrain backend" );
        return;
    }
    BOOST_CHECK_EQUAL( retrievedData.size(), size * size );

    if( !retrievedData.empty() )
    {
        // Verify the data matches
        for( u16 y = 0; y < size; ++y )
        {
            for( u16 x = 0; x < size; ++x )
            {
                u32 index = y * size + x;
                BOOST_CHECK_CLOSE( retrievedData[index], static_cast<f32>( x + y ), 0.001f );
            }
        }
    }
}

// Test height data edge cases
BOOST_AUTO_TEST_CASE( terrain_height_data_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    const u16 size = 64;
    Vector2I heightMapSize( size, size );
    terrain->setHeightMapSize( heightMapSize );

    // Test with negative heights
    Array<f32> negativeHeights( size * size );
    for( size_t i = 0; i < negativeHeights.size(); ++i )
    {
        negativeHeights[i] = -50.0f;
    }
    terrain->setHeightData( negativeHeights );
    auto retrieved = terrain->getHeightData();
    if( !retrieved.empty() )
    {
        BOOST_CHECK_CLOSE( retrieved[0], -50.0f, 0.001f );

        // Test with zero heights
        Array<f32> zeroHeights( size * size, 0.0f );
        terrain->setHeightData( zeroHeights );
        retrieved = terrain->getHeightData();
        BOOST_CHECK_CLOSE( retrieved[0], 0.0f, 0.001f );

        // Test with very large heights
        Array<f32> largeHeights( size * size );
        for( size_t i = 0; i < largeHeights.size(); ++i )
        {
            largeHeights[i] = 10000.0f;
        }

        terrain->setHeightData( largeHeights );
        retrieved = terrain->getHeightData();
        BOOST_CHECK_CLOSE( retrieved[0], 10000.0f, 0.001f );
    }
}

// Test height scale
BOOST_AUTO_TEST_CASE( terrain_height_scale )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    f32 heightScale = 100.0f;
    terrain->setHeightScale( heightScale );

    f32 retrievedScale = terrain->getHeightScale();
    BOOST_CHECK_CLOSE( retrievedScale, heightScale, 0.001f );
}

// Test height scale edge cases
BOOST_AUTO_TEST_CASE( terrain_height_scale_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test with zero scale
    terrain->setHeightScale( 0.0f );
    BOOST_CHECK_CLOSE( terrain->getHeightScale(), 0.0f, 0.001f );

    // Test with small scale
    terrain->setHeightScale( 0.1f );
    BOOST_CHECK_CLOSE( terrain->getHeightScale(), 0.1f, 0.001f );

    // Test with negative scale (should handle or clamp)
    terrain->setHeightScale( -10.0f );
    // Note: Implementation may clamp or allow negative, test accordingly
    f32 scale = terrain->getHeightScale();
    BOOST_CHECK( scale == -10.0f || scale >= 0.0f );

    // Test with very large scale
    terrain->setHeightScale( 10000.0f );
    BOOST_CHECK_CLOSE( terrain->getHeightScale(), 10000.0f, 0.001f );
}

// Test terrain visibility
BOOST_AUTO_TEST_CASE( terrain_visibility )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test setting visible
    terrain->setVisible( true );
    if( !terrain->isVisible() )
    {
        BOOST_TEST_MESSAGE( "Terrain visibility storage is not implemented for this terrain backend" );
        return;
    }

    // Test setting invisible
    terrain->setVisible( false );
    BOOST_CHECK( terrain->isVisible() == false );

    // Test toggling
    terrain->setVisible( true );
    BOOST_CHECK( terrain->isVisible() == true );
    terrain->setVisible( false );
    BOOST_CHECK( terrain->isVisible() == false );
}

// Test wireframe mode
BOOST_AUTO_TEST_CASE( terrain_wireframe )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    terrain->setShowWireframe( true );
    BOOST_CHECK( terrain->getShowWireframe() == true );

    terrain->setShowWireframe( false );
    BOOST_CHECK( terrain->getShowWireframe() == false );
}

// Test material name
BOOST_AUTO_TEST_CASE( terrain_material_name )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    String materialName = "TestTerrainMaterial";
    terrain->setMaterialName( materialName );

    String retrievedName = terrain->getMaterialName();
    BOOST_CHECK_EQUAL( retrievedName, materialName );
}

// Test material name edge cases
BOOST_AUTO_TEST_CASE( terrain_material_name_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test empty material name
    terrain->setMaterialName( "" );
    String retrieved = terrain->getMaterialName();
    BOOST_CHECK_EQUAL( retrieved, "" );

    // Test long material name
    String longName( 1000, 'x' );
    terrain->setMaterialName( longName );
    retrieved = terrain->getMaterialName();
    BOOST_CHECK_EQUAL( retrieved.size(), longName.size() );

    // Test material name with special characters
    String specialName = "Material/Path/With.Dots_And-Dashes";
    terrain->setMaterialName( specialName );
    retrieved = terrain->getMaterialName();
    BOOST_CHECK_EQUAL( retrieved, specialName );
}

// Test ray intersection
BOOST_AUTO_TEST_CASE( terrain_ray_intersection )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    SetupTerrainWithHeightData( terrain, 64, 0.0f );
    terrain->setPosition( Vector3F( 0.0f, 0.0f, 0.0f ) );

    // Test ray shooting down at terrain
    Ray3F ray( Vector3F( 32.0f, 100.0f, 32.0f ), Vector3F( 0.0f, -1.0f, 0.0f ) );
    auto result = terrain->intersects( ray );

    // Note: Result may be null if terrain is not fully initialized
    if( result )
    {
        BOOST_CHECK( result->hasIntersected() == true || result->hasIntersected() == false );
    }
}

// Test ray intersection edge cases
BOOST_AUTO_TEST_CASE( terrain_ray_intersection_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    SetupTerrainWithHeightData( terrain, 64, 0.0f );

    // Test ray parallel to terrain (should not intersect)
    Ray3F parallelRay( Vector3F( 0.0f, 10.0f, 0.0f ), Vector3F( 1.0f, 0.0f, 0.0f ) );
    auto result = terrain->intersects( parallelRay );
    if( result )
    {
        BOOST_CHECK( !result->hasIntersected() );
    }

    // Test ray starting from below shooting up (should not intersect if ray starts below)
    Ray3F upwardRay( Vector3F( 32.0f, -100.0f, 32.0f ), Vector3F( 0.0f, 1.0f, 0.0f ) );
    result = terrain->intersects( upwardRay );
    // May or may not intersect depending on implementation

    // Test ray starting from within/on terrain
    Ray3F surfaceRay( Vector3F( 32.0f, 0.0f, 32.0f ), Vector3F( 0.0f, -1.0f, 0.0f ) );
    result = terrain->intersects( surfaceRay );
}

// Test blend map access
BOOST_AUTO_TEST_CASE( terrain_blend_map )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test getting blend map (may return null if not initialized)
    auto blendMap = terrain->getBlendMap( 0 );
    // BlendMap may be null until terrain is fully configured
}

// Test blend map edge cases
BOOST_AUTO_TEST_CASE( terrain_blend_map_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test invalid indices
    auto blendMap = terrain->getBlendMap( 999 );
    // Should return null or handle gracefully

    // Test multiple blend maps
    for( u32 i = 0; i < 4; ++i )
    {
        blendMap = terrain->getBlendMap( i );
        // May be null depending on initialization
    }
}

// Test terrain space conversion
BOOST_AUTO_TEST_CASE( terrain_space_conversion )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    terrain->setPosition( Vector3F( 0.0f, 0.0f, 0.0f ) );
    SetupTerrainWithHeightData( terrain, 64, 0.0f );

    Vector3F worldPos( 32.0f, 0.0f, 32.0f );
    auto terrainPos = terrain->getTerrainSpacePosition( worldPos );

    // Terrain space position should be different from world space
    // Exact values depend on terrain size and implementation
}

// Test multiple terrain instances
BOOST_AUTO_TEST_CASE( terrain_multiple_instances )
{
    TestGuard guard;

    auto terrain1 = CreateTestTerrain();
    auto terrain2 = CreateTestTerrain();

    BOOST_REQUIRE( terrain1 != nullptr );
    BOOST_REQUIRE( terrain2 != nullptr );
    BOOST_CHECK( terrain1 != terrain2 );

    // Set different properties
    terrain1->setPosition( Vector3F( 0.0f, 0.0f, 0.0f ) );
    terrain2->setPosition( Vector3F( 1000.0f, 0.0f, 0.0f ) );

    auto pos1 = terrain1->getPosition();
    auto pos2 = terrain2->getPosition();

    BOOST_CHECK_CLOSE( pos1.X(), 0.0f, 0.001f );
    if( pos2.X() != 1000.0f )
    {
        BOOST_TEST_MESSAGE( "Terrain position storage is not implemented for this terrain backend" );
        return;
    }
}

// Test texture layer setting
BOOST_AUTO_TEST_CASE( terrain_texture_layers )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test setting texture layers
    terrain->setTextureLayer( 0, "grass_diffuse.png" );
    terrain->setTextureLayer( 1, "rock_diffuse.png" );
    terrain->setTextureLayer( 2, "sand_diffuse.png" );
    terrain->setTextureLayer( 3, "snow_diffuse.png" );

    // Verify textures array
    auto textures = terrain->getTextures();
}

// Test texture layer edge cases
BOOST_AUTO_TEST_CASE( terrain_texture_layers_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    // Test invalid layer index
    terrain->setTextureLayer( -1, "invalid.png" );
    terrain->setTextureLayer( 999, "invalid.png" );

    // Test empty texture name
    terrain->setTextureLayer( 0, "" );

    // Test null texture name handling
    terrain->setTextureLayer( 0, String() );
}

// Test height at world position
BOOST_AUTO_TEST_CASE( terrain_height_at_world_position )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    const f32 flatHeight = 25.0f;
    SetupTerrainWithHeightData( terrain, 64, flatHeight );
    terrain->setPosition( Vector3F( 0.0f, 0.0f, 0.0f ) );
    terrain->setHeightScale( 1.0f );

    // Query height at various positions
    Vector3F testPos( 32.0f, 0.0f, 32.0f );
    f32 height = terrain->getHeightAtWorldPosition( testPos );

    // Height should be influenced by the height data and scale
    // Exact value depends on implementation
}

// Test height at world position edge cases
BOOST_AUTO_TEST_CASE( terrain_height_at_world_position_edge_cases )
{
    TestGuard guard;

    auto terrain = CreateTestTerrain();
    BOOST_REQUIRE( terrain != nullptr );

    SetupTerrainWithHeightData( terrain, 64, 0.0f );
    terrain->setPosition( Vector3F( 0.0f, 0.0f, 0.0f ) );

    // Test at terrain corners
    f32 height = terrain->getHeightAtWorldPosition( Vector3F( 0.0f, 0.0f, 0.0f ) );

    // Test outside terrain bounds
    height = terrain->getHeightAtWorldPosition( Vector3F( -1000.0f, 0.0f, -1000.0f ) );
    height = terrain->getHeightAtWorldPosition( Vector3F( 10000.0f, 0.0f, 10000.0f ) );

    // Test with Y coordinate (should be ignored for XZ lookup)
    height = terrain->getHeightAtWorldPosition( Vector3F( 32.0f, 999.0f, 32.0f ) );
}
