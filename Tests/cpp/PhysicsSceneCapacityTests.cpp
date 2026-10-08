#include <workphone_physics_scene.h>
#include <workphone_physics_rigidbody.h>
#include <workphone_physics_collisionshape.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace
{
    void check( bool passed, const char *message )
    {
        if( !passed )
        {
            std::fprintf( stderr, "%s\n", message );
            std::exit( 1 );
        }
    }
}  // namespace

int main()
{
    auto scene = wp_physics_scene_create();
    check( scene != nullptr, "Scene creation failed" );
    wp_physics_scene_set_gravity( scene, { 0, 0, 0 } );
    std::vector<wp_rigidbody *> bodies;
    std::vector<wp_collision_shape *> shapes;
    const int count = WP_SCENE_MAX_ACTORS * 2 + 17;
    for( int i = 0; i < count; ++i )
    {
        auto body = wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC );
        auto shape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
        check( body && shape, "Body/shape creation failed" );
        wp_collision_shape_set_box_half_extents( shape, { .5f, .5f, .5f } );
        wp_rigidbody_add_shape( body, shape );
        wp_rigidbody_set_position( body, { float( i * 2 ), 0, 0 } );
        check( wp_physics_scene_add_actor( scene, body ) != 0, "Scene silently truncated actors" );
        bodies.push_back( body );
        shapes.push_back( shape );
    }
    check( wp_physics_scene_get_actor_count( scene ) == count, "Wrong actor count after growth" );
    check( wp_physics_scene_add_actor( scene, bodies.back() ) != 0 &&
               wp_physics_scene_get_actor_count( scene ) == count,
           "Duplicate registration changed count" );
    wp_vec3f point, normal;
    const float x = float( ( count - 1 ) * 2 );
    wp_rigidbody *hit = nullptr;
    check( wp_physics_scene_intersects_ex( scene, { x, 2, 0 }, { x, -2, 0 }, &point, &normal, &hit,
                                           nullptr, 0, 0 ) != 0 &&
               hit == bodies.back() && std::abs( point.y - .5f ) < .001f,
           "Ray query cannot reach actors beyond the original limit" );

    auto moving = wp_rigidbody_create( WORKPHONE_RIGIDBODY_DYNAMIC );
    auto movingShape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    check( moving && movingShape, "Dynamic body/shape creation failed" );
    wp_collision_shape_set_box_half_extents( movingShape, { .5f, .5f, .5f } );
    wp_rigidbody_add_shape( moving, movingShape );
    wp_rigidbody_set_position( moving, { x, 0, 3 } );
    wp_rigidbody_set_linear_velocity( moving, { 0, 0, -10 } );
    check( wp_physics_scene_add_actor( scene, moving ) != 0,
           "Dynamic registration after growth failed" );
    wp_physics_scene_simulate( scene, .25f );
    check( wp_rigidbody_get_position( moving ).z > .9f,
           "Contact solver misses actors beyond its old scratch-array limit" );
    wp_physics_scene_remove_actor( scene, moving );
    wp_rigidbody_destroy( moving );
    wp_collision_shape_destroy( movingShape );

    wp_physics_scene_remove_actor( scene, bodies[count / 2] );
    check( wp_physics_scene_get_actor_count( scene ) == count - 1, "Removal after growth failed" );
    wp_physics_scene_clear( scene );
    check( wp_physics_scene_get_actor_count( scene ) == 0, "Clear after growth failed" );
    for( auto body : bodies )
        check( wp_physics_scene_add_actor( scene, body ) != 0, "Scene reuse after clear failed" );
    wp_physics_scene_destroy( scene );
    for( auto body : bodies )
        wp_rigidbody_destroy( body );
    for( auto shape : shapes )
        wp_collision_shape_destroy( shape );
    std::puts( "Scene growth, queries, contacts, removal and reuse: passed." );
}
