#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <workphone_collision_aabbtree.h>
#include <workphone_physics_2d.h>
#include <workphone_physics_collisionshape.h>
#include <workphone_physics_narrowphase.h>
#include <workphone_physics_rigidbody.h>
#include <workphone_physics_scene.h>
#include <workphone_physics_triangle_mesh.h>

#define CHECK( condition )                                                        \
    do                                                                            \
    {                                                                             \
        if( !( condition ) )                                                      \
        {                                                                         \
            fprintf( stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,    \
                     #condition );                                                \
            return 0;                                                             \
        }                                                                         \
    } while( 0 )

static wp_vec3f vector3( wp_f32 x, wp_f32 y, wp_f32 z )
{
    const wp_vec3f result = { x, y, z };
    return result;
}

static wp_vec2f vector2( wp_f32 x, wp_f32 y )
{
    const wp_vec2f result = { x, y };
    return result;
}

static int test_aabb_tree_radius_sphere_query( void )
{
    WpCollisionTriangle triangles[2] = {
        { { { -0.5f, 0.0f, -0.5f },
            { 0.5f, 0.0f, -0.5f },
            { 0.0f, 0.0f, 0.5f } },
          7u },
        { { { 9.5f, 0.0f, -0.5f },
            { 10.5f, 0.0f, -0.5f },
            { 10.0f, 0.0f, 0.5f } },
          19u }
    };
    const WpCollisionVector3 center = { 0.0f, 0.25f, 0.0f };
    WpCollisionAABBTree *tree =
        wp_collision_aabbtree_create( triangles, 2u );
    uint32_t candidates[2] = { UINT32_MAX, UINT32_MAX };
    uint32_t count;

    CHECK( tree );
    wp_collision_aabbtree_set_radius( tree, -1.0f );
    CHECK( fabsf( wp_collision_aabbtree_get_radius( tree ) - 1.0f ) <
           1.0e-6f );
    count = wp_collision_aabbtree_sphere_query(
        tree, center, candidates, 2u );
    CHECK( count == 1u );
    CHECK( candidates[0] == 7u );
    CHECK( wp_collision_aabbtree_sphere_intersect( tree, center ) );
    wp_collision_aabbtree_set_radius( tree, 20.0f );
    count = wp_collision_aabbtree_sphere_query(
        tree, center, candidates, 1u );
    CHECK( count == 2u );

    wp_collision_aabbtree_destroy( tree );
    return 1;
}

static int test_large_track_mesh_uses_local_candidates( void )
{
    const wp_u32 grid_size = 128u;
    const wp_u32 row_size = grid_size + 1u;
    const wp_u32 vertex_count = row_size * row_size;
    const wp_u32 triangle_count = grid_size * grid_size * 2u;
    wp_f32 *vertices =
        (wp_f32 *)malloc( sizeof( wp_f32 ) * vertex_count * 3u );
    wp_u32 *indices =
        (wp_u32 *)malloc( sizeof( wp_u32 ) * triangle_count * 3u );
    wp_collision_mesh_data mesh_data;
    wp_collision_shape *track_shape;
    wp_collision_shape *car_shape;
    wp_rigidbody *track_body;
    wp_rigidbody *car_body;
    wp_narrowphase *narrowphase;
    const wp_triangle_mesh *triangle_mesh;
    wp_contact_manifold manifold;
    wp_u32 candidates[256];
    wp_u32 candidate_count;
    wp_u32 x;
    wp_u32 z;
    wp_u32 triangle = 0u;
    const wp_quatf track_orientation = {
        0.0f, 0.38268343f, 0.0f, 0.92387953f
    };
    int collided;

    CHECK( vertices && indices );
    for( z = 0u; z < row_size; ++z )
    {
        for( x = 0u; x < row_size; ++x )
        {
            const wp_u32 vertex = z * row_size + x;
            vertices[vertex * 3u] =
                (wp_f32)x - (wp_f32)grid_size * 0.5f;
            vertices[vertex * 3u + 1u] = 0.0f;
            vertices[vertex * 3u + 2u] =
                (wp_f32)z - (wp_f32)grid_size * 0.5f;
        }
    }
    for( z = 0u; z < grid_size; ++z )
    {
        for( x = 0u; x < grid_size; ++x )
        {
            const wp_u32 a = z * row_size + x;
            const wp_u32 b = a + 1u;
            const wp_u32 c = a + row_size;
            const wp_u32 d = c + 1u;
            indices[triangle * 3u] = a;
            indices[triangle * 3u + 1u] = c;
            indices[triangle * 3u + 2u] = b;
            ++triangle;
            indices[triangle * 3u] = b;
            indices[triangle * 3u + 1u] = c;
            indices[triangle * 3u + 2u] = d;
            ++triangle;
        }
    }

    mesh_data.vertices = vertices;
    mesh_data.vertex_count = vertex_count;
    mesh_data.indices = indices;
    mesh_data.triangle_count = triangle_count;
    track_shape =
        wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    car_shape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    track_body = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    car_body = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    narrowphase = wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    CHECK( track_shape && car_shape && track_body && car_body && narrowphase );

    wp_collision_shape_set_mesh_data( track_shape, &mesh_data );
    triangle_mesh = wp_collision_shape_get_triangle_mesh( track_shape );
    CHECK( triangle_mesh );
    candidate_count = wp_triangle_mesh_query_sphere(
        triangle_mesh, vector3( 0.0f, 0.25f, 0.0f ), 1.0f, candidates,
        (wp_u32)( sizeof( candidates ) / sizeof( candidates[0] ) ) );
    CHECK( candidate_count > 0u );
    CHECK( candidate_count <=
           (wp_u32)( sizeof( candidates ) / sizeof( candidates[0] ) ) );
    CHECK( candidate_count < triangle_count / 100u );

    wp_collision_shape_set_box_half_extents(
        car_shape, vector3( 0.5f, 0.5f, 0.5f ) );
    CHECK( wp_rigidbody_add_shape( track_body, track_shape ) >= 0 );
    CHECK( wp_rigidbody_add_shape( car_body, car_shape ) >= 0 );
    wp_rigidbody_set_position( track_body, vector3( 10.0f, 0.0f, -3.0f ) );
    wp_rigidbody_set_orientation( track_body, track_orientation );
    wp_rigidbody_set_position( car_body, vector3( 10.0f, 0.25f, -3.0f ) );
    collided = wp_narrowphase_test_pair(
        narrowphase, car_body, car_shape, track_body, track_shape, &manifold );

    wp_narrowphase_destroy( narrowphase );
    wp_rigidbody_destroy( car_body );
    wp_rigidbody_destroy( track_body );
    wp_collision_shape_destroy( car_shape );
    wp_collision_shape_destroy( track_shape );
    free( indices );
    free( vertices );

    CHECK( collided );
    CHECK( manifold.contact_count > 0 );
    return 1;
}

static int test_2d_box_contact_stays_in_plane( void )
{
    wp_narrowphase *narrowphase =
        wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    wp_rigidbody *body_a =
        wp_rigidbody2_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    wp_rigidbody *body_b =
        wp_rigidbody2_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *shape_a =
        wp_collision_shape2_create_box( vector2( 2.0f, 2.0f ) );
    wp_collision_shape *shape_b =
        wp_collision_shape2_create_box( vector2( 2.0f, 2.0f ) );
    wp_contact_manifold manifold;
    int passed;

    CHECK( narrowphase && body_a && body_b && shape_a && shape_b );
    CHECK( wp_rigidbody_add_shape( body_a, shape_a ) >= 0 );
    CHECK( wp_rigidbody_add_shape( body_b, shape_b ) >= 0 );
    wp_rigidbody2_set_position( body_a, vector2( 0.5f, 0.0f ) );
    passed = wp_narrowphase_test_pair( narrowphase, body_a, shape_a, body_b,
                                       shape_b, &manifold ) &&
             manifold.contact_count > 0 &&
             fabsf( manifold.contacts[0].normal_world_on_b.z ) < 1.0e-5f;

    wp_narrowphase_destroy( narrowphase );
    wp_rigidbody_destroy( body_a );
    wp_rigidbody_destroy( body_b );
    wp_collision_shape_destroy( shape_a );
    wp_collision_shape_destroy( shape_b );
    CHECK( passed );
    return 1;
}

static int test_track_keeps_road_and_barrier_contacts( void )
{
    const wp_f32 vertices[] = {
        -2.0f, 0.0f, -2.0f, 2.0f, 0.0f, -2.0f, 0.0f, 0.0f, 2.0f,
        0.0f, -2.0f, -2.0f, 0.0f, 2.0f, -2.0f, 0.0f, 0.0f, 2.0f
    };
    const wp_u32 indices[] = { 0, 1, 2, 3, 4, 5 };
    const wp_collision_mesh_data mesh_data = { vertices, 6, indices, 2 };
    wp_narrowphase *narrowphase =
        wp_narrowphase_create( WORKPHONE_NARROWPHASE_HYBRID );
    wp_rigidbody *car_body =
        wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    wp_rigidbody *track_body =
        wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *car_shape =
        wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    wp_collision_shape *track_shape =
        wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    wp_contact_manifold manifold;
    int has_road_normal = 0;
    int has_barrier_normal = 0;
    wp_s32 i;

    CHECK( narrowphase && car_body && track_body && car_shape && track_shape );
    wp_collision_shape_set_box_half_extents(
        car_shape, vector3( 0.5f, 0.5f, 0.5f ) );
    wp_collision_shape_set_mesh_data( track_shape, &mesh_data );
    CHECK( wp_rigidbody_add_shape( car_body, car_shape ) >= 0 );
    CHECK( wp_rigidbody_add_shape( track_body, track_shape ) >= 0 );
    wp_rigidbody_set_position( car_body, vector3( 0.25f, 0.25f, 0.0f ) );
    CHECK( wp_narrowphase_test_pair( narrowphase, car_body, car_shape,
                                     track_body, track_shape, &manifold ) );

    for( i = 0; i < manifold.contact_count; ++i )
    {
        const wp_vec3f normal = manifold.contacts[i].normal_world_on_b;
        has_road_normal |= normal.y < -0.9f;
        has_barrier_normal |= normal.x < -0.9f;
    }

    wp_narrowphase_destroy( narrowphase );
    wp_rigidbody_destroy( car_body );
    wp_rigidbody_destroy( track_body );
    wp_collision_shape_destroy( car_shape );
    wp_collision_shape_destroy( track_shape );
    CHECK( manifold.contact_count >= 2 );
    CHECK( has_road_normal && has_barrier_normal );
    return 1;
}

static int test_track_raycast_normal_faces_suspension_ray( void )
{
    const wp_f32 vertices[] = { -5.0f, 0.0f, -5.0f, 5.0f, 0.0f, -5.0f,
                                5.0f, 0.0f, 5.0f };
    const wp_u32 indices[] = { 0, 1, 2 };
    const wp_collision_mesh_data mesh_data = { vertices, 3, indices, 1 };
    wp_physics_scene *scene = wp_physics_scene_create();
    wp_rigidbody *track_body =
        wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *track_shape =
        wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    wp_vec3f hit_position;
    wp_vec3f hit_normal;
    int passed;

    CHECK( scene && track_body && track_shape );
    wp_collision_shape_set_mesh_data( track_shape, &mesh_data );
    CHECK( wp_rigidbody_add_shape( track_body, track_shape ) >= 0 );
    CHECK( wp_physics_scene_add_actor( scene, track_body ) );
    passed = wp_physics_scene_intersects(
                 scene, vector3( 0.0f, 5.0f, 0.0f ),
                 vector3( 0.0f, -5.0f, 0.0f ), &hit_position, &hit_normal,
                 0u, 0u ) &&
             hit_normal.y > 0.9f;

    wp_physics_scene_destroy( scene );
    wp_rigidbody_destroy( track_body );
    wp_collision_shape_destroy( track_shape );
    CHECK( passed );
    return 1;
}

static int test_fast_car_does_not_tunnel_through_track_mesh( void )
{
    const wp_f32 vertices[] = { 0.0f, -5.0f, -5.0f, 0.0f, 5.0f, -5.0f,
                                0.0f, 0.0f, 5.0f };
    const wp_u32 indices[] = { 0, 1, 2 };
    const wp_collision_mesh_data mesh_data = { vertices, 3, indices, 1 };
    wp_physics_scene *scene = wp_physics_scene_create();
    wp_rigidbody *car_body =
        wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    wp_rigidbody *track_body =
        wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *car_shape =
        wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    wp_collision_shape *track_shape =
        wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_MESH );
    wp_vec3f position;
    wp_vec3f velocity;

    CHECK( scene && car_body && track_body && car_shape && track_shape );
    wp_physics_scene_set_gravity( scene, vector3( 0.0f, 0.0f, 0.0f ) );
    wp_collision_shape_set_box_half_extents(
        car_shape, vector3( 0.25f, 0.25f, 0.25f ) );
    wp_collision_shape_set_mesh_data( track_shape, &mesh_data );
    CHECK( wp_rigidbody_add_shape( car_body, car_shape ) >= 0 );
    CHECK( wp_rigidbody_add_shape( track_body, track_shape ) >= 0 );
    wp_rigidbody_set_position( car_body, vector3( -2.0f, 0.0f, 0.0f ) );
    wp_rigidbody_set_linear_velocity(
        car_body, vector3( 240.0f, 0.0f, 0.0f ) );
    CHECK( wp_physics_scene_add_actor( scene, car_body ) );
    CHECK( wp_physics_scene_add_actor( scene, track_body ) );

    wp_physics_scene_simulate( scene, 1.0f / 60.0f );
    position = wp_rigidbody_get_position( car_body );
    velocity = wp_rigidbody_get_linear_velocity( car_body );

    wp_physics_scene_destroy( scene );
    wp_rigidbody_destroy( car_body );
    wp_rigidbody_destroy( track_body );
    wp_collision_shape_destroy( car_shape );
    wp_collision_shape_destroy( track_shape );
    CHECK( position.x < 0.0f );
    CHECK( velocity.x <= 0.0f );
    return 1;
}

static int test_moved_static_wakes_sleeping_contact_pair( void )
{
    wp_physics_scene *scene = wp_physics_scene_create();
    wp_rigidbody *dynamic_body =
        wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    wp_rigidbody *static_body =
        wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
    wp_collision_shape *dynamic_shape =
        wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    wp_collision_shape *static_shape =
        wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    wp_vec3f position;

    CHECK( scene && dynamic_body && static_body && dynamic_shape &&
           static_shape );
    wp_physics_scene_set_gravity( scene, vector3( 0.0f, 0.0f, 0.0f ) );
    CHECK( wp_rigidbody_add_shape( dynamic_body, dynamic_shape ) >= 0 );
    CHECK( wp_rigidbody_add_shape( static_body, static_shape ) >= 0 );
    wp_rigidbody_set_position( static_body, vector3( 10.0f, 0.0f, 0.0f ) );
    wp_rigidbody_put_to_sleep( dynamic_body );
    CHECK( wp_physics_scene_add_actor( scene, dynamic_body ) );
    CHECK( wp_physics_scene_add_actor( scene, static_body ) );

    wp_rigidbody_set_position( static_body, vector3( 0.5f, 0.0f, 0.0f ) );
    wp_physics_scene_simulate( scene, 1.0f / 60.0f );
    position = wp_rigidbody_get_position( dynamic_body );

    wp_physics_scene_destroy( scene );
    wp_rigidbody_destroy( dynamic_body );
    wp_rigidbody_destroy( static_body );
    wp_collision_shape_destroy( dynamic_shape );
    wp_collision_shape_destroy( static_shape );
    CHECK( position.x < -0.1f );
    return 1;
}

int main( void )
{
    int passed = 1;
    passed &= test_aabb_tree_radius_sphere_query();
    passed &= test_large_track_mesh_uses_local_candidates();
    passed &= test_2d_box_contact_stays_in_plane();
    passed &= test_track_keeps_road_and_barrier_contacts();
    passed &= test_track_raycast_normal_faces_suspension_ray();
    passed &= test_fast_car_does_not_tunnel_through_track_mesh();
    passed &= test_moved_static_wakes_sleeping_contact_pair();
    if( passed )
    {
        puts( "All WPPhysics collision tests passed." );
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
