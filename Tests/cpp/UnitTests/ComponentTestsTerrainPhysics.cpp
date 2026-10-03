#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Physics/ITerrainShape.hpp>
#include <Workphone/Scene/Components/CollisionTerrain.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/State/States/GraphicsObjectData.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/TerrainStateData.hpp>
#include <Workphone/State/States/TransformStateData.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    void checkVectorClose( const Vector3<real_Num> &actual, const Vector3<real_Num> &expected,
                           real_Num tolerance = real_Num( 0.001 ) )
    {
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual, expected, tolerance ),
                             "actual=(" << actual.X() << ", " << actual.Y() << ", " << actual.Z()
                                        << ") expected=(" << expected.X() << ", " << expected.Y() << ", "
                                        << expected.Z() << ")" );
    }

    template <class T>
    void requirePropertyValue( const SmartPtr<Properties> &properties, const String &name, T &value )
    {
        BOOST_REQUIRE( properties );
        BOOST_REQUIRE_MESSAGE( properties->getPropertyValue( name, value ),
                               "Expected property '" << name << "' to be present" );
    }

    template <class T>
    void addStateData( SmartPtr<IStateContext> stateContext, SmartPtr<IFactoryManager> factoryManager,
                       hash_type id )
    {
        auto state = factoryManager->make_ptr<State>();
        BOOST_REQUIRE( state );
        state->setId( id );

        auto data = factoryManager->make_ptr<T>();
        BOOST_REQUIRE( data );
        state->setData( data );
        stateContext->addState( state );
    }

    SmartPtr<render::Terrain> createStateBackedTerrain( TestGuard &fixture )
    {
        BOOST_REQUIRE( fixture.stateManager );
        BOOST_REQUIRE( fixture.factoryManager );

        auto terrain = workphone::make_ptr<render::Terrain>();
        BOOST_REQUIRE( terrain );

        auto stateContext = fixture.stateManager->addStateContext();
        BOOST_REQUIRE( stateContext );
        stateContext->setOwner( terrain );
        terrain->setStateContext( stateContext );

        fixture.addCleanup( [stateManager = fixture.stateManager, stateContext, terrain]() mutable {
            if( terrain )
            {
                terrain->setStateContext( nullptr );
            }

            if( stateContext )
            {
                stateContext->setOwner( nullptr );
            }

            if( stateManager && stateContext )
            {
                stateManager->removeStateContext( stateContext );
            }
        } );

        const auto id = terrain->getId();
        addStateData<TransformStateData>( stateContext, fixture.factoryManager, id );
        addStateData<TerrainStateData>( stateContext, fixture.factoryManager, id );
        addStateData<GraphicsObjectData>( stateContext, fixture.factoryManager, id );

        return terrain;
    }

    Array<f32> makeRampHeightData( s32 width, s32 depth )
    {
        Array<f32> heights;
        heights.reserve( static_cast<u32>( width * depth ) );
        for( s32 z = 0; z < depth; ++z )
        {
            for( s32 x = 0; x < width; ++x )
            {
                heights.push_back( static_cast<f32>( x + z * 10 ) );
            }
        }
        return heights;
    }

    Array<f32> makeFlatHeightData( s32 width, s32 depth, f32 height )
    {
        Array<f32> heights;
        heights.reserve( static_cast<u32>( width * depth ) );
        for( s32 i = 0; i < width * depth; ++i )
        {
            heights.push_back( height );
        }
        return heights;
    }

    SmartPtr<scene::CollisionTerrain> createLoadedTerrainCollider(
        TestGuard &fixture, SmartPtr<scene::IGameActor> &actor, SmartPtr<scene::Rigidbody> &rigidbody,
        u32 width = 16u, u32 depth = 8u,
        const Vector3<real_Num> &scale = Vector3<real_Num>( 2.0f, 3.0f, 4.0f ),
        const Vector3<real_Num> &position = Vector3<real_Num>( 1.0f, 2.0f, 3.0f ) )
    {
        actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );
        BOOST_REQUIRE( actor->isStatic() );

        auto collisionTerrain = actor->addComponent<scene::CollisionTerrain>();
        BOOST_REQUIRE( collisionTerrain );
        collisionTerrain->setTerrainWidth( width );
        collisionTerrain->setTerrainDepth( depth );
        collisionTerrain->setTerrainScale( scale );
        collisionTerrain->setPosition( position );

        rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        fixture.sceneManager->edit();
        fixture.updatePhysics( 3 );

        BOOST_REQUIRE( collisionTerrain->isLoaded() );
        BOOST_REQUIRE( collisionTerrain->getShape() );
        BOOST_REQUIRE( rigidbody->getRigidStatic() );

        return collisionTerrain;
    }

}  // namespace

BOOST_AUTO_TEST_SUITE( GraphicsTerrainData )

BOOST_AUTO_TEST_CASE( state_backed_terrain_defaults )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );

        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().X(), 256 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().Y(), 256 );
        BOOST_CHECK_EQUAL( terrain->getSize(), 256u );
        BOOST_CHECK_CLOSE( terrain->getHeightScale(), 50.0f, 0.001f );
        BOOST_CHECK( terrain->isVisible() );
        BOOST_CHECK( !terrain->getShowWireframe() );
        BOOST_CHECK( terrain->getMaterialName().empty() );
        BOOST_CHECK_EQUAL( terrain->getHeightData().size(), 256u * 256u );
        BOOST_CHECK_EQUAL( terrain->getTextures().size(), 24u );
        BOOST_CHECK( !terrain->getHeightMap() );
        BOOST_CHECK( !terrain->getTexture( 0u ) );
        BOOST_CHECK( !terrain->getMesh() );
        // getLayerBlendMapSize returns 0 when no blend maps have been configured.
        BOOST_CHECK_EQUAL( terrain->getLayerBlendMapSize(), 0u );
        // Without graphics system, getBlendMap returns null. With graphics system,
        // a CPU-side blend map is lazily created and returned (non-null).
        auto applicationManager = core::IApplicationManager::instancePtr();
        const bool graphicsAvailable = applicationManager && applicationManager->getGraphicsSystemPtr();
        if( !graphicsAvailable )
        {
            BOOST_CHECK( !terrain->getBlendMap( 0u ) );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( heightmap_size_clamps_and_reports_min_axis )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );

        terrain->setHeightMapSize( Vector2I( 1, -25 ) );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().X(), 2 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().Y(), 2 );
        BOOST_CHECK_EQUAL( terrain->getSize(), 2u );

        terrain->setHeightMapSize( Vector2I( 0, 0 ) );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().X(), 2 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().Y(), 2 );
        BOOST_CHECK_EQUAL( terrain->getSize(), 2u );

        terrain->setHeightMapSize( Vector2I( 512, 128 ) );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().X(), 512 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().Y(), 128 );
        BOOST_CHECK_EQUAL( terrain->getSize(), 128u );

        terrain->setHeightMapSize( Vector2I( 64, 64 ) );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().X(), 64 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().Y(), 64 );
        BOOST_CHECK_EQUAL( terrain->getSize(), 64u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( height_data_round_trip_and_bilinear_sampling )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 4, 4 ) );
        terrain->setPosition( Vector3<real_Num>( 10.0f, 0.0f, 20.0f ) );

        auto heights = makeRampHeightData( 4, 4 );
        terrain->setHeightData( heights );

        auto storedHeights = terrain->getHeightData();
        BOOST_REQUIRE_EQUAL( storedHeights.size(), heights.size() );
        for( size_t i = 0; i < heights.size(); ++i )
        {
            BOOST_TEST_CONTEXT( "index " << i )
            {
                BOOST_CHECK_CLOSE( storedHeights[i], heights[i], 0.001f );
            }
        }

        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 8.0f, 0.0f, 18.0f ) ),
                           0.0f, 0.001f );
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 11.0f, 0.0f, 21.0f ) ),
                           33.0f, 0.001f );
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 8.5f, 0.0f, 18.5f ) ),
                           5.5f, 0.001f );
        BOOST_CHECK_CLOSE(
            terrain->getHeightAtWorldPosition( Vector3<real_Num>( -100.0f, 0.0f, -100.0f ) ), 0.0f,
            0.001f );
        BOOST_CHECK_CLOSE(
            terrain->getHeightAtWorldPosition( Vector3<real_Num>( 100.0f, 0.0f, 100.0f ) ), 33.0f,
            0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( height_sampling_rejects_invalid_buffers )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 4, 4 ) );

        terrain->setHeightData( {} );
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 0.0f, 0.0f, 0.0f ) ),
                           0.0f, 0.001f );

        Array<f32> wrongSize( 3u, 99.0f );
        terrain->setHeightData( wrongSize );
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 0.0f, 0.0f, 0.0f ) ),
                           0.0f, 0.001f );

        terrain->setHeightData( makeFlatHeightData( 4, 4, 7.0f ) );
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 0.0f, 0.0f, 0.0f ) ),
                           7.0f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( height_scale_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 4, 4 ) );
        terrain->setHeightData( makeFlatHeightData( 4, 4, 1.0f ) );

        terrain->setHeightScale( 10.0f );
        BOOST_CHECK_CLOSE( terrain->getHeightScale(), 10.0f, 0.001f );

        // The stored height data is not multiplied by height scale; sampling returns raw values.
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 0.0f, 0.0f, 0.0f ) ),
                           1.0f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( optional_graphics_queries_return_null_when_unconfigured )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 8, 8 ) );
        terrain->setHeightData( makeFlatHeightData( 8, 8, 5.0f ) );

        // When the graphics system is unavailable, intersects() returns nullptr.
        // When the graphics system is available, the result is a non-null ray
        // query whose getIntersected() flag indicates whether a hit was found.
        auto applicationManager = core::IApplicationManager::instancePtr();
        const bool graphicsAvailable = applicationManager && applicationManager->getGraphicsSystemPtr();
        Ray3F ray( Vector3<real_Num>( 0.0f, 10.0f, 0.0f ), Vector3<real_Num>( 0.0f, -1.0f, 0.0f ) );
        if( !graphicsAvailable )
        {
            BOOST_CHECK( !terrain->intersects( ray ) );
        }
        else
        {
            auto hitResult = terrain->intersects( ray );
            BOOST_REQUIRE( hitResult );
            BOOST_CHECK( hitResult->hasIntersected() );
        }
        Ray3F missRay( Vector3<real_Num>( 0.0f, 300.0f, 0.0f ), Vector3<real_Num>( 0.0f, 0.0f, 1.0f ) );
        if( !graphicsAvailable )
        {
            BOOST_CHECK( !terrain->intersects( missRay ) );
        }
        else
        {
            auto missResult = terrain->intersects( missRay );
            BOOST_REQUIRE( missResult );
            BOOST_CHECK( !missResult->hasIntersected() );
        }
        BOOST_CHECK( !terrain->getMesh() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE( GraphicsTerrainTransform )

BOOST_AUTO_TEST_CASE( position_and_world_transform_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );

        const auto position = Vector3<real_Num>( 12.0f, 34.0f, 56.0f );
        terrain->setPosition( position );
        checkVectorClose( terrain->getPosition(), position );
        checkVectorClose( terrain->getWorldTransform().getPosition(), position );

        Transform3<real_Num> transform;
        transform.setPosition( Vector3<real_Num>( -1.0f, -2.0f, -3.0f ) );
        terrain->setWorldTransform( transform );
        checkVectorClose( terrain->getWorldTransform().getPosition(),
                          Vector3<real_Num>( -1.0f, -2.0f, -3.0f ) );
        checkVectorClose( terrain->getPosition(), Vector3<real_Num>( -1.0f, -2.0f, -3.0f ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( terrain_space_conversion_clamps_to_heightmap )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 8, 6 ) );

        Transform3<real_Num> transform;
        transform.setPosition( Vector3<real_Num>( 10.0f, 5.0f, 20.0f ) );
        terrain->setWorldTransform( transform );

        checkVectorClose( terrain->getTerrainSpacePosition( Vector3<real_Num>( 10.0f, 7.0f, 20.0f ) ),
                          Vector3<real_Num>( 4.0f, 2.0f, 3.0f ) );
        checkVectorClose(
            terrain->getTerrainSpacePosition( Vector3<real_Num>( -100.0f, 7.0f, -100.0f ) ),
            Vector3<real_Num>( 0.0f, 2.0f, 0.0f ) );
        checkVectorClose( terrain->getTerrainSpacePosition( Vector3<real_Num>( 100.0f, 7.0f, 100.0f ) ),
                          Vector3<real_Num>( 7.0f, 2.0f, 5.0f ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( terrain_space_conversion_with_arbitrary_transform )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 4, 4 ) );

        Transform3<real_Num> transform;
        transform.setPosition( Vector3<real_Num>( 5.0f, 0.0f, 5.0f ) );
        terrain->setWorldTransform( transform );

        const auto centre = terrain->getTerrainSpacePosition( Vector3<real_Num>( 5.0f, 0.0f, 5.0f ) );
        BOOST_CHECK_CLOSE( centre.X(), 2.0f, 0.001f );
        BOOST_CHECK_CLOSE( centre.Z(), 2.0f, 0.001f );

        const auto southWest = terrain->getTerrainSpacePosition( Vector3<real_Num>( 3.0f, 0.0f, 3.0f ) );
        BOOST_CHECK_CLOSE( southWest.X(), 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( southWest.Z(), 0.0f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( terrain_origin_centered_sampling )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 2, 2 ) );
        terrain->setPosition( Vector3<real_Num>( 100.0f, 0.0f, -50.0f ) );
        terrain->setHeightData( makeFlatHeightData( 2, 2, 9.0f ) );

        BOOST_CHECK_CLOSE(
            terrain->getHeightAtWorldPosition( Vector3<real_Num>( 100.0f, 0.0f, -50.0f ) ), 9.0f,
            0.001f );
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 99.0f, 0.0f, -51.0f ) ),
                           9.0f, 0.001f );
        BOOST_CHECK_CLOSE(
            terrain->getHeightAtWorldPosition( Vector3<real_Num>( 101.0f, 0.0f, -49.0f ) ), 9.0f,
            0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( GraphicsTerrainMaterialVisibility )

BOOST_AUTO_TEST_CASE( visibility_flag_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );

        BOOST_CHECK( terrain->isVisible() );
        terrain->setVisible( false );
        BOOST_CHECK( !terrain->isVisible() );
        terrain->setVisible( true );
        BOOST_CHECK( terrain->isVisible() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( wireframe_flag_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );

        BOOST_CHECK( !terrain->getShowWireframe() );
        terrain->setShowWireframe( true );
        BOOST_CHECK( terrain->getShowWireframe() );
        terrain->setShowWireframe( false );
        BOOST_CHECK( !terrain->getShowWireframe() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( material_name_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );

        BOOST_CHECK( terrain->getMaterialName().empty() );
        terrain->setMaterialName( "GrassTerrain" );
        BOOST_CHECK_EQUAL( terrain->getMaterialName(), "GrassTerrain" );
        terrain->setMaterialName( "" );
        BOOST_CHECK( terrain->getMaterialName().empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( properties_export_visibility_and_material )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 32, 32 ) );
        terrain->setHeightScale( 75.0f );
        terrain->setShowWireframe( true );
        terrain->setMaterialName( "RockTerrain" );

        auto properties = terrain->getProperties();
        BOOST_REQUIRE( properties );

        Vector2I heightMapSize;
        requirePropertyValue( properties, render::Terrain::HeightMapSizeStr, heightMapSize );
        BOOST_CHECK_EQUAL( heightMapSize.X(), 32 );
        BOOST_CHECK_EQUAL( heightMapSize.Y(), 32 );

        f32 heightScale = 0.0f;
        requirePropertyValue( properties, render::Terrain::HeightScaleStr, heightScale );
        BOOST_CHECK_CLOSE( heightScale, 75.0f, 0.001f );

        bool showWireframe = false;
        requirePropertyValue( properties, render::Terrain::ShowWireframeStr, showWireframe );
        BOOST_CHECK( showWireframe );

        String materialName;
        requirePropertyValue( properties, render::Terrain::MaterialNameStr, materialName );
        BOOST_CHECK_EQUAL( materialName, "RockTerrain" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( set_properties_restores_rendering_state )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 16, 16 ) );
        terrain->setHeightScale( 25.0f );
        terrain->setShowWireframe( false );
        terrain->setMaterialName( "Default" );

        auto properties = terrain->getProperties();
        BOOST_REQUIRE( properties );

        properties->setProperty( render::Terrain::HeightMapSizeStr, Vector2I( 64, 64 ) );
        properties->setProperty( render::Terrain::HeightScaleStr, 100.0f );
        properties->setProperty( render::Terrain::ShowWireframeStr, true );
        properties->setProperty( render::Terrain::MaterialNameStr, String( "Restored" ) );

        terrain->setProperties( properties );

        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().X(), 64 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().Y(), 64 );
        BOOST_CHECK_CLOSE( terrain->getHeightScale(), 100.0f, 0.001f );
        BOOST_CHECK( terrain->getShowWireframe() );
        BOOST_CHECK_EQUAL( terrain->getMaterialName(), "Restored" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE( CollisionTerrainComponent )

BOOST_AUTO_TEST_CASE( properties_round_trip )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionTerrain = actor->addComponent<scene::CollisionTerrain>();
        BOOST_REQUIRE( collisionTerrain );

        collisionTerrain->setTerrainWidth( 16u );
        collisionTerrain->setTerrainDepth( 8u );
        collisionTerrain->setTerrainScale( Vector3<real_Num>( 2.0f, 3.0f, 4.0f ) );
        collisionTerrain->setPosition( Vector3<real_Num>( 1.0f, 2.0f, 3.0f ) );
        collisionTerrain->setTrigger( true );
        collisionTerrain->setStaticFriction( 0.1f );
        collisionTerrain->setDynamicFriction( 0.2f );
        collisionTerrain->setRestitution( 0.3f );

        auto properties = collisionTerrain->getProperties();
        BOOST_REQUIRE( properties );

        u32 width = 0u;
        u32 depth = 0u;
        Vector3<real_Num> scale;
        Vector3<real_Num> position;
        bool trigger = false;
        f32 staticFriction = 0.0f;
        f32 dynamicFriction = 0.0f;
        f32 restitution = 0.0f;

        requirePropertyValue( properties, CollisionTerrain::terrainWidthStr, width );
        requirePropertyValue( properties, CollisionTerrain::terrainDepthStr, depth );
        requirePropertyValue( properties, CollisionTerrain::terrainScaleStr, scale );
        requirePropertyValue( properties, Collision::positionStr, position );
        requirePropertyValue( properties, Collision::isTriggerStr, trigger );
        requirePropertyValue( properties, Collision::staticFrictionStr, staticFriction );
        requirePropertyValue( properties, Collision::dynamicFrictionStr, dynamicFriction );
        requirePropertyValue( properties, Collision::restitutionStr, restitution );

        BOOST_CHECK_EQUAL( width, 16u );
        BOOST_CHECK_EQUAL( depth, 8u );
        checkVectorClose( scale, Vector3<real_Num>( 2.0f, 3.0f, 4.0f ) );
        checkVectorClose( position, Vector3<real_Num>( 1.0f, 2.0f, 3.0f ) );
        BOOST_CHECK( trigger );
        BOOST_CHECK_CLOSE( staticFriction, 0.1f, 0.001f );
        BOOST_CHECK_CLOSE( dynamicFriction, 0.2f, 0.001f );
        BOOST_CHECK_CLOSE( restitution, 0.3f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( load_creates_terrain_shape )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        SmartPtr<scene::IGameActor> actor;
        SmartPtr<scene::Rigidbody> rigidbody;
        auto collisionTerrain = createLoadedTerrainCollider( fixture, actor, rigidbody );

        auto shape = collisionTerrain->getShape();
        BOOST_REQUIRE( shape );
        BOOST_CHECK( workphone::dynamic_pointer_cast<physics::ITerrainShape>( shape ) );
        BOOST_CHECK( shape->isAttached() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( load_unload_load_recreates_shape )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        SmartPtr<scene::IGameActor> actor;
        SmartPtr<scene::Rigidbody> rigidbody;
        auto collisionTerrain = createLoadedTerrainCollider( fixture, actor, rigidbody );

        BOOST_REQUIRE( collisionTerrain->getShape() );

        collisionTerrain->unload( nullptr );
        BOOST_CHECK( !collisionTerrain->getShape() );

        collisionTerrain->load( nullptr );
        BOOST_REQUIRE( collisionTerrain->getShape() );
        BOOST_CHECK(
            workphone::dynamic_pointer_cast<physics::ITerrainShape>( collisionTerrain->getShape() ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( shape_pose_and_scale )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        SmartPtr<scene::IGameActor> actor;
        SmartPtr<scene::Rigidbody> rigidbody;
        const auto offset = Vector3<real_Num>( 1.0f, 2.0f, 3.0f );
        auto collisionTerrain = createLoadedTerrainCollider(
            fixture, actor, rigidbody, 16u, 8u, Vector3<real_Num>( 2.0f, 3.0f, 4.0f ), offset );

        checkVectorClose( collisionTerrain->getPosition(), offset );

        auto shape = collisionTerrain->getShape();
        BOOST_REQUIRE( shape );
        BOOST_CHECK( shape->getLocalPose().isValid() );
        BOOST_CHECK( shape->isAttached() );

        // Scale is stored on the component; it can be recovered through properties.
        auto properties = collisionTerrain->getProperties();
        Vector3<real_Num> scale;
        requirePropertyValue( properties, CollisionTerrain::terrainScaleStr, scale );
        checkVectorClose( scale, Vector3<real_Num>( 2.0f, 3.0f, 4.0f ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( trigger_and_enabled_inheritance )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionTerrain = actor->addComponent<scene::CollisionTerrain>();
        BOOST_REQUIRE( collisionTerrain );
        collisionTerrain->setTerrainWidth( 8u );
        collisionTerrain->setTerrainDepth( 8u );
        collisionTerrain->setTrigger( true );
        collisionTerrain->setEnabled( false );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        fixture.sceneManager->edit();
        fixture.updatePhysics( 3 );

        BOOST_REQUIRE( collisionTerrain->getShape() );
        BOOST_CHECK( collisionTerrain->getShape()->isTrigger() );
        BOOST_CHECK( !collisionTerrain->getShape()->isEnabled() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( isValid_states )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        // A standalone collider with no actor is not valid.
        auto standalone = workphone::make_ptr<scene::CollisionTerrain>();
        BOOST_CHECK( !standalone->isValid() );

        SmartPtr<scene::IGameActor> actor;
        SmartPtr<scene::Rigidbody> rigidbody;
        auto collisionTerrain = createLoadedTerrainCollider( fixture, actor, rigidbody );
        BOOST_CHECK( collisionTerrain->isValid() );

        collisionTerrain->unload( nullptr );
        BOOST_CHECK( !collisionTerrain->isValid() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( load_without_physics_manager_is_safe )
{
    try
    {
        TestGuard fixture( false );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionTerrain = actor->addComponent<scene::CollisionTerrain>();
        BOOST_REQUIRE( collisionTerrain );
        collisionTerrain->setTerrainWidth( 8u );
        collisionTerrain->setTerrainDepth( 8u );

        // Even when a physics manager exists, a public load() call must not crash.
        BOOST_CHECK_NO_THROW( collisionTerrain->load( nullptr ) );
        BOOST_CHECK( collisionTerrain->isLoaded() );

        if( !fixture.physicsManager )
        {
            // Without a physics back-end the component loads safely but produces no shape.
            BOOST_CHECK( !collisionTerrain->getShape() );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE( TerrainActorIntegration )

BOOST_AUTO_TEST_CASE( default_actor_components )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = ApplicationUtil::createDefaultTerrain( false );
        BOOST_REQUIRE( actor );
        BOOST_CHECK( actor->isStatic() );
        BOOST_CHECK( actor->hasComponent<scene::TerrainSystem>() );
        BOOST_CHECK( actor->hasComponent<scene::CollisionTerrain>() );
        BOOST_CHECK( actor->hasComponent<scene::Rigidbody>() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( default_terrain_actor_enters_scene_as_static_physics )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = ApplicationUtil::createDefaultTerrain( true );
        BOOST_REQUIRE( actor );
        BOOST_CHECK( actor->isStatic() );

        fixture.sceneManager->play();
        fixture.resetTimer();
        fixture.updatePhysics( 5 );

        auto rigidbodies = actor->getComponentsByType<scene::Rigidbody>();
        BOOST_REQUIRE( !rigidbodies.empty() );
        BOOST_CHECK( rigidbodies.front()->hasRigidStatic() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( default_terrain_actor_addToScene_creates_static_body )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = ApplicationUtil::createDefaultTerrain( true );
        BOOST_REQUIRE( actor );
        BOOST_CHECK( actor->isStatic() );

        fixture.sceneManager->play();
        fixture.resetTimer();
        fixture.updatePhysics( 5 );

        auto rigidbodies = actor->getComponentsByType<scene::Rigidbody>();
        BOOST_REQUIRE_EQUAL( rigidbodies.size(), 1u );
        BOOST_CHECK( rigidbodies.front()->hasRigidStatic() );
        BOOST_CHECK( rigidbodies.front()->getRigidStatic() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( collider_attaches_to_static_body )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = ApplicationUtil::createDefaultTerrain( true );
        BOOST_REQUIRE( actor );

        fixture.sceneManager->play();
        fixture.resetTimer();
        fixture.updatePhysics( 5 );

        auto rigidbodies = actor->getComponentsByType<scene::Rigidbody>();
        BOOST_REQUIRE( !rigidbodies.empty() );
        auto rigidbody = rigidbodies.front();

        BOOST_TEST_MESSAGE( "before numShapes GE check: " << rigidbody->getNumShapes() );
        BOOST_CHECK_GE( rigidbody->getNumShapes(), 1u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( shape_removed_on_actor_destroy )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping terrain test" );
        return;
    }

    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = ApplicationUtil::createDefaultTerrain( true );
        BOOST_REQUIRE( actor );

        fixture.sceneManager->play();
        fixture.resetTimer();
        fixture.updatePhysics( 5 );

        auto rigidbodies = actor->getComponentsByType<scene::Rigidbody>();
        BOOST_REQUIRE( !rigidbodies.empty() );
        auto rigidbody = rigidbodies.front();

        BOOST_CHECK_GE( rigidbody->getNumShapes(), 1u );

        fixture.sceneManager->destroyActor( actor );
        fixture.updatePhysics( 5 );
        BOOST_CHECK_EQUAL( rigidbody->getNumShapes(), 0u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( transform_update_propagates )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = ApplicationUtil::createDefaultTerrain( true );
        BOOST_REQUIRE( actor );

        fixture.sceneManager->play();
        fixture.resetTimer();
        fixture.updatePhysics( 5 );

        auto rigidbodies = actor->getComponentsByType<scene::Rigidbody>();
        BOOST_REQUIRE( !rigidbodies.empty() );
        auto rigidbody = rigidbodies.front();
        auto staticBody = rigidbody->getRigidStatic();
        BOOST_REQUIRE( staticBody );

        const auto newPosition = Vector3<real_Num>( 100.0f, 20.0f, -50.0f );
        actor->setPosition( newPosition );
        fixture.updatePhysics( 5 );

        checkVectorClose( staticBody->getTransform().getPosition(), newPosition, 0.1f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( TerrainErrorAndEdgeCases )

BOOST_AUTO_TEST_CASE( zero_size_terrain_collider_does_not_crash )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionTerrain = actor->addComponent<scene::CollisionTerrain>();
        BOOST_REQUIRE( collisionTerrain );
        collisionTerrain->setTerrainWidth( 0u );
        collisionTerrain->setTerrainDepth( 0u );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        BOOST_REQUIRE( rigidbody );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );
        fixture.sceneManager->edit();
        BOOST_CHECK_NO_THROW( fixture.updatePhysics( 3 ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( mismatched_height_data )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto terrain = createStateBackedTerrain( fixture );
        terrain->setHeightMapSize( Vector2I( 4, 4 ) );

        // A buffer of the wrong size is rejected and sampling falls back to zero.
        terrain->setHeightData( makeFlatHeightData( 8, 8, 5.0f ) );
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 0.0f, 0.0f, 0.0f ) ),
                           0.0f, 0.001f );

        // Supplying the correct size restores normal sampling.
        terrain->setHeightData( makeFlatHeightData( 4, 4, 7.0f ) );
        BOOST_CHECK_CLOSE( terrain->getHeightAtWorldPosition( Vector3<real_Num>( 0.0f, 0.0f, 0.0f ) ),
                           7.0f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( standalone_collider_lifecycle )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping terrain test" );
        return;
    }

    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        auto collisionTerrain = workphone::make_ptr<scene::CollisionTerrain>();
        BOOST_REQUIRE( collisionTerrain );
        BOOST_CHECK( !collisionTerrain->getActor() );

        BOOST_CHECK_NO_THROW( collisionTerrain->unload( nullptr ) );
        BOOST_CHECK_NO_THROW( collisionTerrain->load( nullptr ) );
        BOOST_CHECK( collisionTerrain->getShape() );
        BOOST_CHECK_NO_THROW( collisionTerrain->unload( nullptr ) );
        BOOST_CHECK( !collisionTerrain->getShape() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( base_collision_properties_round_trip )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        auto actor = fixture.createBasicActor( true );
        BOOST_REQUIRE( actor );

        auto collisionTerrain = actor->addComponent<scene::CollisionTerrain>();
        BOOST_REQUIRE( collisionTerrain );

        collisionTerrain->setTerrainWidth( 4u );
        collisionTerrain->setTerrainDepth( 4u );
        collisionTerrain->setPosition( Vector3<real_Num>( 5.0f, 6.0f, 7.0f ) );
        collisionTerrain->setTrigger( true );
        collisionTerrain->setStaticFriction( 0.4f );
        collisionTerrain->setDynamicFriction( 0.5f );
        collisionTerrain->setRestitution( 0.6f );

        auto properties = collisionTerrain->getProperties();
        BOOST_REQUIRE( properties );

        Vector3<real_Num> position;
        bool trigger = false;
        f32 staticFriction = 0.0f;
        f32 dynamicFriction = 0.0f;
        f32 restitution = 0.0f;

        requirePropertyValue( properties, Collision::positionStr, position );
        requirePropertyValue( properties, Collision::isTriggerStr, trigger );
        requirePropertyValue( properties, Collision::staticFrictionStr, staticFriction );
        requirePropertyValue( properties, Collision::dynamicFrictionStr, dynamicFriction );
        requirePropertyValue( properties, Collision::restitutionStr, restitution );

        checkVectorClose( position, Vector3<real_Num>( 5.0f, 6.0f, 7.0f ) );
        BOOST_CHECK( trigger );
        BOOST_CHECK_CLOSE( staticFriction, 0.4f, 0.001f );
        BOOST_CHECK_CLOSE( dynamicFriction, 0.5f, 0.001f );
        BOOST_CHECK_CLOSE( restitution, 0.6f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( property_changes_after_load )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        SmartPtr<scene::IGameActor> actor;
        SmartPtr<scene::Rigidbody> rigidbody;
        auto collisionTerrain = createLoadedTerrainCollider( fixture, actor, rigidbody );

        BOOST_REQUIRE( collisionTerrain->getShape() );
        auto shape = collisionTerrain->getShape();

        collisionTerrain->setTrigger( true );
        BOOST_CHECK( shape->isTrigger() );

        collisionTerrain->setEnabled( false );
        BOOST_CHECK( !shape->isEnabled() );

        collisionTerrain->setEnabled( true );
        BOOST_CHECK( shape->isEnabled() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
