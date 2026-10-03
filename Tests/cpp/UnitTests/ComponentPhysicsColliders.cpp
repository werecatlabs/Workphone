#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/Interface/Physics/IPlaneShape3.hpp>
#include <Workphone/Interface/Physics/ISphereShape3.hpp>
#include <Workphone/Interface/Physics/ITerrainShape.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    constexpr auto InvalidMeshPath = "missing_collision_mesh.fbmeshbin";

    void checkVectorClose( const Vector3<real_Num> &actual, const Vector3<real_Num> &expected,
                           real_Num tolerance = real_Num( 0.001 ) )
    {
        BOOST_CHECK( MathUtil<real_Num>::equals( actual, expected, tolerance ) );
    }

    template <class T>
    void requirePropertyValue( const SmartPtr<Properties> &properties, const String &name, T &value )
    {
        BOOST_REQUIRE( properties );
        BOOST_REQUIRE_MESSAGE( properties->getPropertyValue( name, value ),
                               "Expected property '" << name << "' to be present" );
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( ComponentPhysicsCollidersTests )

BOOST_AUTO_TEST_CASE( component_physics_colliders_base_properties_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionBox source;
        const auto expectedExtents = Vector3<real_Num>( 2.0f, 3.0f, 4.0f );
        const auto expectedPosition = Vector3<real_Num>( -1.25f, 0.5f, 9.0f );
        const auto expectedRadius = 2.75f;
        const auto expectedStaticFriction = 0.15f;
        const auto expectedDynamicFriction = 0.35f;
        const auto expectedRestitution = 0.8f;

        source.setEnabled( false );
        source.setTrigger( true );
        source.setExtents( expectedExtents );
        source.setPosition( expectedPosition );
        source.setRadius( expectedRadius );
        source.setStaticFriction( expectedStaticFriction );
        source.setDynamicFriction( expectedDynamicFriction );
        source.setRestitution( expectedRestitution );

        auto properties = source.getProperties();
        BOOST_REQUIRE( properties );

        bool trigger = false;
        Vector3<real_Num> extents;
        Vector3<real_Num> position;
        f32 radius = 0.0f;
        f32 staticFriction = 0.0f;
        f32 dynamicFriction = 0.0f;
        f32 restitution = 0.0f;

        requirePropertyValue( properties, scene::Collision::isTriggerStr, trigger );
        requirePropertyValue( properties, scene::Collision::extentsStr, extents );
        requirePropertyValue( properties, scene::Collision::positionStr, position );
        requirePropertyValue( properties, scene::Collision::radiusStr, radius );
        requirePropertyValue( properties, scene::Collision::staticFrictionStr, staticFriction );
        requirePropertyValue( properties, scene::Collision::dynamicFrictionStr, dynamicFriction );
        requirePropertyValue( properties, scene::Collision::restitutionStr, restitution );

        BOOST_CHECK( trigger );
        checkVectorClose( extents, expectedExtents );
        checkVectorClose( position, expectedPosition );
        BOOST_CHECK_CLOSE( radius, expectedRadius, 0.001f );
        BOOST_CHECK_CLOSE( staticFriction, expectedStaticFriction, 0.001f );
        BOOST_CHECK_CLOSE( dynamicFriction, expectedDynamicFriction, 0.001f );
        BOOST_CHECK_CLOSE( restitution, expectedRestitution, 0.001f );

        const auto frictionProperty =
            properties->getPropertyObject( scene::Collision::staticFrictionStr );
        BOOST_CHECK_EQUAL( frictionProperty.getAttribute( "category" ), "Material" );
        BOOST_CHECK_EQUAL( frictionProperty.getAttribute( "min" ), "0" );
        BOOST_CHECK_EQUAL( frictionProperty.getAttribute( "max" ), "1" );

        scene::CollisionBox restored;
        restored.setProperties( properties );

        BOOST_CHECK( !restored.isEnabled() );
        BOOST_CHECK( restored.isTrigger() );
        checkVectorClose( restored.getExtents(), expectedExtents );
        checkVectorClose( restored.getPosition(), expectedPosition );
        BOOST_CHECK_CLOSE( restored.getRadius(), expectedRadius, 0.001f );
        BOOST_CHECK_CLOSE( restored.getStaticFriction(), expectedStaticFriction, 0.001f );
        BOOST_CHECK_CLOSE( restored.getDynamicFriction(), expectedDynamicFriction, 0.001f );
        BOOST_CHECK_CLOSE( restored.getRestitution(), expectedRestitution, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_invalid_values_are_clamped )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionBox collider;
        collider.setExtents( Vector3<real_Num>( -1.0f, 2.0f, -3.0f ) );
        collider.setRadius( -4.0f );
        collider.setStaticFriction( -0.25f );
        collider.setDynamicFriction( 1.25f );
        collider.setRestitution( 2.0f );

        checkVectorClose( collider.getExtents(), Vector3<real_Num>( 0.0f, 2.0f, 0.0f ) );
        BOOST_CHECK_SMALL( collider.getRadius(), 0.001f );
        BOOST_CHECK_SMALL( collider.getStaticFriction(), 0.001f );
        BOOST_CHECK_CLOSE( collider.getDynamicFriction(), 1.0f, 0.001f );
        BOOST_CHECK_CLOSE( collider.getRestitution(), 1.0f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_partial_properties_preserve_defaults )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionBox collider;
        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( scene::Collision::radiusStr, 5.0f );
        properties->setProperty( scene::Collision::isTriggerStr, true );

        collider.setProperties( properties );

        BOOST_CHECK( collider.isTrigger() );
        BOOST_CHECK_CLOSE( collider.getRadius(), 5.0f, 0.001f );
        checkVectorClose( collider.getExtents(), Vector3<real_Num>::unit() );
        checkVectorClose( collider.getPosition(), Vector3<real_Num>::zero() );
        BOOST_CHECK_CLOSE( collider.getStaticFriction(), 0.5f, 0.001f );
        BOOST_CHECK_CLOSE( collider.getDynamicFriction(), 0.5f, 0.001f );
        BOOST_CHECK_CLOSE( collider.getRestitution(), 0.0f, 0.001f );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_box_load_applies_cached_shape_state )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionBox collider;
        const auto expectedExtents = Vector3<real_Num>( 1.5f, 2.5f, 3.5f );
        const auto expectedPosition = Vector3<real_Num>( 0.25f, -0.5f, 1.25f );

        collider.setExtents( expectedExtents );
        collider.setPosition( expectedPosition );
        collider.setTrigger( true );
        collider.load( nullptr );

        BOOST_CHECK( collider.isLoaded() );
        auto shape = workphone::dynamic_pointer_cast<physics::IBoxShape3>( collider.getShape() );
        BOOST_REQUIRE( shape );

        checkVectorClose( shape->getExtents(), expectedExtents );
        checkVectorClose( shape->getLocalPose().getPosition(), expectedPosition );
        BOOST_CHECK( shape->isTrigger() );

        collider.unload( nullptr );
        BOOST_CHECK( !collider.getShape() );
        BOOST_CHECK( !collider.isLoaded() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_box_unload_is_idempotent )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionBox collider;
        collider.load( nullptr );
        BOOST_REQUIRE( collider.getShape() );

        collider.unload( nullptr );
        collider.unload( nullptr );

        BOOST_CHECK( !collider.getShape() );
        BOOST_CHECK( !collider.isLoaded() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_sphere_load_uses_cached_radius )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionSphere collider;
        collider.setRadius( 3.25f );
        collider.load( nullptr );

        BOOST_CHECK( collider.isLoaded() );
        auto shape = workphone::dynamic_pointer_cast<physics::ISphereShape3>( collider.getShape() );
        BOOST_REQUIRE( shape );
        BOOST_CHECK_CLOSE( shape->getRadius(), 3.25f, 0.001f );

        collider.setRadius( 1.75f );
        BOOST_CHECK_CLOSE( shape->getRadius(), 1.75f, 0.001f );

        collider.unload( nullptr );
        BOOST_CHECK( !collider.getShape() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_plane_load_creates_shape_and_material )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionPlane collider;
        collider.load( nullptr );

        BOOST_CHECK( collider.isLoaded() );
        BOOST_CHECK( collider.getMaterial() );
        BOOST_CHECK( workphone::dynamic_pointer_cast<physics::IPlaneShape3>( collider.getShape() ) );

        collider.unload( nullptr );
        BOOST_CHECK( !collider.getShape() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_mesh_properties_normalize_path_and_convex_flag )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionMesh collider;
        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( scene::CollisionMesh::meshPathStr, "sample.mesh" );
        properties->setProperty( scene::CollisionMesh::isConvexStr, true );

        collider.setProperties( properties );

        BOOST_CHECK_EQUAL( collider.getMeshPath(), "sample.fbmeshbin" );
        BOOST_CHECK( collider.isConvex() );

        auto exported = collider.getProperties();
        BOOST_REQUIRE( exported );

        String meshPath;
        bool convex = false;
        requirePropertyValue( exported, scene::CollisionMesh::meshPathStr, meshPath );
        requirePropertyValue( exported, scene::CollisionMesh::isConvexStr, convex );

        BOOST_CHECK_EQUAL( meshPath, "sample.fbmeshbin" );
        BOOST_CHECK( convex );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_mesh_load_creates_shape_without_mesh_resource )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionMesh collider;
        const auto expectedPosition = Vector3<real_Num>( -2.0f, 1.0f, 0.5f );
        collider.setMeshPath( InvalidMeshPath );
        collider.setConvex( true );
        collider.setPosition( expectedPosition );
        collider.setTrigger( true );

        collider.load( nullptr );

        BOOST_CHECK( collider.isLoaded() );
        BOOST_CHECK_EQUAL( collider.getMeshPath(), InvalidMeshPath );
        BOOST_CHECK( !collider.getMeshResource() );

        auto shape = workphone::dynamic_pointer_cast<physics::IMeshShape>( collider.getShape() );
        BOOST_REQUIRE( shape );
        BOOST_CHECK( shape->isConvex() );
        BOOST_CHECK( shape->isTrigger() );
        checkVectorClose( shape->getLocalPose().getPosition(), expectedPosition );

        collider.unload( nullptr );
        BOOST_CHECK( !collider.getShape() );
        BOOST_CHECK( !collider.getMeshResource() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_mesh_unload_is_idempotent )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionMesh collider;
        collider.setMeshPath( InvalidMeshPath );
        collider.load( nullptr );
        BOOST_REQUIRE( collider.getShape() );

        collider.unload( nullptr );
        collider.unload( nullptr );

        BOOST_CHECK( !collider.getShape() );
        BOOST_CHECK( !collider.getMeshResource() );
        BOOST_CHECK( !collider.isLoaded() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_terrain_properties_round_trip )
{
    try
    {
        TestGuard fixture;
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionTerrain source;
        const auto expectedScale = Vector3<real_Num>( 8.0f, 1.5f, 16.0f );
        source.setTerrainWidth( 64u );
        source.setTerrainDepth( 32u );
        source.setTerrainScale( expectedScale );
        source.setTrigger( true );
        source.setPosition( Vector3<real_Num>( 10.0f, 2.0f, -5.0f ) );

        auto properties = source.getProperties();
        BOOST_REQUIRE( properties );

        u32 width = 0u;
        u32 depth = 0u;
        Vector3<real_Num> scale;
        requirePropertyValue( properties, scene::CollisionTerrain::terrainWidthStr, width );
        requirePropertyValue( properties, scene::CollisionTerrain::terrainDepthStr, depth );
        requirePropertyValue( properties, scene::CollisionTerrain::terrainScaleStr, scale );

        BOOST_CHECK_EQUAL( width, 64u );
        BOOST_CHECK_EQUAL( depth, 32u );
        checkVectorClose( scale, expectedScale );

        scene::CollisionTerrain restored;
        restored.setProperties( properties );

        BOOST_CHECK_EQUAL( restored.getTerrainWidth(), 64u );
        BOOST_CHECK_EQUAL( restored.getTerrainDepth(), 32u );
        checkVectorClose( restored.getTerrainScale(), expectedScale );
        BOOST_CHECK( restored.isTrigger() );
        checkVectorClose( restored.getPosition(), Vector3<real_Num>( 10.0f, 2.0f, -5.0f ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_terrain_load_creates_shape_with_base_state )
{
    try
    {
        TestGuard fixture( true );
        if( !fixture.isAvailable )
        {
            return;
        }

        scene::CollisionTerrain collider;
        const auto expectedPosition = Vector3<real_Num>( 3.0f, -1.0f, 7.0f );
        collider.setTerrainWidth( 16u );
        collider.setTerrainDepth( 8u );
        collider.setTerrainScale( Vector3<real_Num>( 2.0f, 4.0f, 6.0f ) );
        collider.setPosition( expectedPosition );
        collider.setTrigger( true );
        collider.load( nullptr );

        BOOST_CHECK( collider.isLoaded() );
        BOOST_CHECK_EQUAL( collider.getTerrainWidth(), 16u );
        BOOST_CHECK_EQUAL( collider.getTerrainDepth(), 8u );
        checkVectorClose( collider.getTerrainScale(), Vector3<real_Num>( 2.0f, 4.0f, 6.0f ) );

        auto shape = workphone::dynamic_pointer_cast<physics::ITerrainShape>( collider.getShape() );
        BOOST_REQUIRE( shape );
        BOOST_CHECK( shape->isTrigger() );
        checkVectorClose( shape->getLocalPose().getPosition(), expectedPosition );

        collider.unload( nullptr );
        BOOST_CHECK( !collider.getShape() );
        BOOST_CHECK( !collider.isLoaded() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_CASE( component_physics_colliders_actor_attachment_loads_colliders )
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

        auto box = actor->addComponent<scene::CollisionBox>();
        auto sphere = actor->addComponent<scene::CollisionSphere>();
        auto mesh = actor->addComponent<scene::CollisionMesh>();
        auto terrain = actor->addComponent<scene::CollisionTerrain>();

        BOOST_REQUIRE( box );
        BOOST_REQUIRE( sphere );
        BOOST_REQUIRE( mesh );
        BOOST_REQUIRE( terrain );

        box->setExtents( Vector3<real_Num>( 1.0f, 2.0f, 3.0f ) );
        sphere->setRadius( 1.25f );
        mesh->setMeshPath( InvalidMeshPath );
        terrain->setTerrainWidth( 4u );
        terrain->setTerrainDepth( 4u );

        fixture.scene->registerAllUpdates( actor );
        fixture.scene->addActor( actor );

        BOOST_CHECK( box->isLoaded() );
        BOOST_CHECK( sphere->isLoaded() );
        BOOST_CHECK( mesh->isLoaded() );
        BOOST_CHECK( terrain->isLoaded() );
        BOOST_CHECK( workphone::dynamic_pointer_cast<physics::IBoxShape3>( box->getShape() ) );
        BOOST_CHECK( workphone::dynamic_pointer_cast<physics::ISphereShape3>( sphere->getShape() ) );
        BOOST_CHECK( workphone::dynamic_pointer_cast<physics::IMeshShape>( mesh->getShape() ) );
        BOOST_CHECK( workphone::dynamic_pointer_cast<physics::ITerrainShape>( terrain->getShape() ) );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
