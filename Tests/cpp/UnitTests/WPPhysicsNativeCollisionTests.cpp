#include "UnitTests.hpp"
#include <boost/test/unit_test.hpp>

extern "C" {
#include <workphone_physics_2d.h>
#include <workphone_physics_collisionshape.h>
#include <workphone_physics_narrowphase.h>
#include <workphone_physics_rigidbody.h>
#include <workphone_physics_scene.h>
}

namespace
{
    wp_vec3f vector3( wp_f32 x, wp_f32 y, wp_f32 z )
    {
        const wp_vec3f result = { x, y, z };
        return result;
    }

    wp_vec2f vector2( wp_f32 x, wp_f32 y )
    {
        const wp_vec2f result = { x, y };
        return result;
    }
}  // namespace

BOOST_AUTO_TEST_CASE( wp_physics_2d_box_contacts_never_resolve_on_hidden_z )
{
    auto narrowphase = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    auto bodyA = wp_rigidbody2_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    auto bodyB = wp_rigidbody2_create( WORKPHONE_RIGIDBODY_STATIC );
    auto shapeA = wp_collision_shape2_create_box( vector2( 2.0f, 2.0f ) );
    auto shapeB = wp_collision_shape2_create_box( vector2( 2.0f, 2.0f ) );
    BOOST_REQUIRE( narrowphase );
    BOOST_REQUIRE( bodyA );
    BOOST_REQUIRE( bodyB );
    BOOST_REQUIRE( shapeA );
    BOOST_REQUIRE( shapeB );

    wp_rigidbody_add_shape( bodyA, shapeA );
    wp_rigidbody_add_shape( bodyB, shapeB );
    wp_rigidbody2_set_position( bodyA, vector2( 0.5f, 0.0f ) );

    wp_contact_manifold manifold;
    BOOST_REQUIRE( wp_narrowphase_test_pair( narrowphase, bodyA, shapeA, bodyB, shapeB, &manifold ) );
    BOOST_REQUIRE_GT( manifold.contact_count, 0 );
    BOOST_CHECK_SMALL( manifold.contacts[0].normal_world_on_b.z, 1.0e-5f );

    wp_narrowphase_destroy( narrowphase );
    wp_rigidbody_destroy( bodyA );
    wp_rigidbody_destroy( bodyB );
    wp_collision_shape_destroy( shapeA );
    wp_collision_shape_destroy( shapeB );
}

BOOST_AUTO_TEST_CASE( wp_physics_track_mesh_keeps_road_and_barrier_contacts )
{
    const wp_f32 vertices[] = { -2.0f, 0.0f,  -2.0f, 2.0f, 0.0f, -2.0f, 0.0f, 0.0f, 2.0f,
                                0.0f,  -2.0f, -2.0f, 0.0f, 2.0f, -2.0f, 0.0f, 0.0f, 2.0f };
    const wp_u32 indices[] = { 0, 1, 2, 3, 4, 5 };
    const wp_collision_mesh_data meshData = { vertices, 6, indices, 2 };

    auto narrowphase = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    auto carBody = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    auto trackBody = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    auto carShape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    auto trackShape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    BOOST_REQUIRE( narrowphase );
    BOOST_REQUIRE( carBody );
    BOOST_REQUIRE( trackBody );
    BOOST_REQUIRE( carShape );
    BOOST_REQUIRE( trackShape );

    wp_collision_shape_set_box_half_extents( carShape, vector3( 0.5f, 0.5f, 0.5f ) );
    wp_collision_shape_set_mesh_data( trackShape, &meshData );
    wp_rigidbody_add_shape( carBody, carShape );
    wp_rigidbody_add_shape( trackBody, trackShape );
    wp_rigidbody_set_position( carBody, vector3( 0.25f, 0.25f, 0.0f ) );

    wp_contact_manifold manifold;
    BOOST_REQUIRE(
        wp_narrowphase_test_pair( narrowphase, carBody, carShape, trackBody, trackShape, &manifold ) );
    BOOST_CHECK_GE( manifold.contact_count, 2 );

    auto hasRoadNormal = false;
    auto hasBarrierNormal = false;
    for( wp_s32 i = 0; i < manifold.contact_count; ++i )
    {
        const auto normal = manifold.contacts[i].normal_world_on_b;
        hasRoadNormal = hasRoadNormal || normal.y < -0.9f;
        hasBarrierNormal = hasBarrierNormal || normal.x < -0.9f;
    }
    BOOST_CHECK( hasRoadNormal );
    BOOST_CHECK( hasBarrierNormal );

    wp_narrowphase_destroy( narrowphase );
    wp_rigidbody_destroy( carBody );
    wp_rigidbody_destroy( trackBody );
    wp_collision_shape_destroy( carShape );
    wp_collision_shape_destroy( trackShape );
}

BOOST_AUTO_TEST_CASE( wp_physics_track_raycast_normal_faces_suspension_ray )
{
    const wp_f32 vertices[] = { -5.0f, 0.0f, -5.0f, 5.0f, 0.0f, -5.0f, 5.0f, 0.0f, 5.0f };
    const wp_u32 indices[] = { 0, 1, 2 };  // Deliberately downward winding.
    const wp_collision_mesh_data meshData = { vertices, 3, indices, 1 };

    auto scene = wp_physics_scene_create();
    auto trackBody = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    auto trackShape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    BOOST_REQUIRE( scene );
    BOOST_REQUIRE( trackBody );
    BOOST_REQUIRE( trackShape );

    wp_collision_shape_set_mesh_data( trackShape, &meshData );
    wp_rigidbody_add_shape( trackBody, trackShape );
    BOOST_REQUIRE( wp_physics_scene_add_actor( scene, trackBody ) );

    wp_vec3f hitPosition;
    wp_vec3f hitNormal;
    BOOST_REQUIRE( wp_physics_scene_intersects( scene, vector3( 0.0f, 5.0f, 0.0f ),
                                                vector3( 0.0f, -5.0f, 0.0f ), &hitPosition, &hitNormal,
                                                0u, 0u ) );
    BOOST_CHECK_GT( hitNormal.y, 0.9f );

    wp_physics_scene_destroy( scene );
    wp_rigidbody_destroy( trackBody );
    wp_collision_shape_destroy( trackShape );
}

BOOST_AUTO_TEST_CASE( wp_physics_fast_car_does_not_tunnel_through_track_mesh )
{
    const wp_f32 vertices[] = { 0.0f, -5.0f, -5.0f, 0.0f, 5.0f, -5.0f, 0.0f, 0.0f, 5.0f };
    const wp_u32 indices[] = { 0, 1, 2 };
    const wp_collision_mesh_data meshData = { vertices, 3, indices, 1 };

    auto scene = wp_physics_scene_create();
    auto carBody = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    auto trackBody = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    auto carShape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    auto trackShape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    BOOST_REQUIRE( scene );
    BOOST_REQUIRE( carBody );
    BOOST_REQUIRE( trackBody );
    BOOST_REQUIRE( carShape );
    BOOST_REQUIRE( trackShape );

    wp_physics_scene_set_gravity( scene, vector3( 0.0f, 0.0f, 0.0f ) );
    wp_collision_shape_set_box_half_extents( carShape, vector3( 0.25f, 0.25f, 0.25f ) );
    wp_collision_shape_set_mesh_data( trackShape, &meshData );
    wp_rigidbody_add_shape( carBody, carShape );
    wp_rigidbody_add_shape( trackBody, trackShape );
    wp_rigidbody_set_position( carBody, vector3( -2.0f, 0.0f, 0.0f ) );
    wp_rigidbody_set_linear_velocity( carBody, vector3( 240.0f, 0.0f, 0.0f ) );
    BOOST_REQUIRE( wp_physics_scene_add_actor( scene, carBody ) );
    BOOST_REQUIRE( wp_physics_scene_add_actor( scene, trackBody ) );

    wp_physics_scene_simulate( scene, 1.0f / 60.0f );
    const auto position = wp_rigidbody_get_position( carBody );
    const auto velocity = wp_rigidbody_get_linear_velocity( carBody );
    BOOST_CHECK_LT( position.x, 0.0f );
    BOOST_CHECK_LE( velocity.x, 0.0f );

    wp_physics_scene_destroy( scene );
    wp_rigidbody_destroy( carBody );
    wp_rigidbody_destroy( trackBody );
    wp_collision_shape_destroy( carShape );
    wp_collision_shape_destroy( trackShape );
}
