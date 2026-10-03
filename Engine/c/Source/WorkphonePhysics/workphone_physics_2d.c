#include "workphone_physics_2d.h"
#include <math.h>

/*
 * The 2D API reuses the 3D primitive narrow phase. Give 2D boxes a very
 * large out-of-plane extent so the artificial Z slab can never become the
 * minimum-penetration axis. A finite 0.5 extent allowed deep X/Y overlaps to
 * resolve along hidden Z, leaving the bodies visibly intersecting.
 */
#define WP_PHYSICS2_HALF_DEPTH 1000000.0f

static wp_vec3f wp_physics2_to_vec3( wp_vec2f v )
{
    wp_vec3f r;
    r.x = v.x;
    r.y = v.y;
    r.z = 0.0f;
    return r;
}

static wp_vec2f wp_physics2_from_vec3( wp_vec3f v )
{
    wp_vec2f r;
    r.x = v.x;
    r.y = v.y;
    return r;
}

static wp_quatf wp_physics2_quat_from_angle( wp_f32 radians )
{
    wp_quatf q;
    wp_f32 half_angle;
    half_angle = radians * 0.5f;
    q.x = 0.0f;
    q.y = 0.0f;
    q.z = sinf( half_angle );
    q.w = cosf( half_angle );
    return q;
}

static wp_f32 wp_physics2_angle_from_quat( wp_quatf q )
{
    return atan2f( 2.0f * ( q.w * q.z ), 1.0f - 2.0f * ( q.z * q.z ) );
}

wp_collision_shape *wp_collision_shape2_create_box( wp_vec2f half_extents )
{
    wp_collision_shape *shape;
    shape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_BOX );
    if( shape )
    {
        wp_collision_shape2_set_box_half_extents( shape, half_extents );
    }
    return shape;
}

wp_collision_shape *wp_collision_shape2_create_sphere( wp_f32 radius )
{
    wp_collision_shape *shape;
    shape = wp_collision_shape_create( WORKPHONE_COLLISION_SHAPE_SPHERE );
    if( shape )
    {
        wp_collision_shape2_set_sphere_radius( shape, radius );
    }
    return shape;
}

void wp_collision_shape2_set_box_half_extents( wp_collision_shape *shape, wp_vec2f half_extents )
{
    wp_vec3f extents;
    extents = wp_physics2_to_vec3( half_extents );
    extents.z = WP_PHYSICS2_HALF_DEPTH;
    wp_collision_shape_set_box_half_extents( shape, extents );
}

wp_vec2f wp_collision_shape2_get_box_half_extents( const wp_collision_shape *shape )
{
    return wp_physics2_from_vec3( wp_collision_shape_get_box_half_extents( shape ) );
}

void wp_collision_shape2_set_sphere_radius( wp_collision_shape *shape, wp_f32 radius )
{
    wp_collision_shape_set_sphere_radius( shape, radius );
}

wp_f32 wp_collision_shape2_get_sphere_radius( const wp_collision_shape *shape )
{
    return wp_collision_shape_get_sphere_radius( shape );
}

wp_rigidbody *wp_rigidbody2_create( wp_rigidbody_type type )
{
    return wp_rigidbody_create( type );
}

wp_vec2f wp_rigidbody2_get_position( const wp_rigidbody *body )
{
    return wp_physics2_from_vec3( wp_rigidbody_get_position( body ) );
}

void wp_rigidbody2_set_position( wp_rigidbody *body, wp_vec2f position )
{
    wp_rigidbody_set_position( body, wp_physics2_to_vec3( position ) );
}

wp_f32 wp_rigidbody2_get_orientation( const wp_rigidbody *body )
{
    return wp_physics2_angle_from_quat( wp_rigidbody_get_orientation( body ) );
}

void wp_rigidbody2_set_orientation( wp_rigidbody *body, wp_f32 radians )
{
    wp_rigidbody_set_orientation( body, wp_physics2_quat_from_angle( radians ) );
}

wp_vec2f wp_rigidbody2_get_linear_velocity( const wp_rigidbody *body )
{
    return wp_physics2_from_vec3( wp_rigidbody_get_linear_velocity( body ) );
}

void wp_rigidbody2_set_linear_velocity( wp_rigidbody *body, wp_vec2f velocity )
{
    wp_rigidbody_set_linear_velocity( body, wp_physics2_to_vec3( velocity ) );
}

wp_f32 wp_rigidbody2_get_angular_velocity( const wp_rigidbody *body )
{
    return wp_rigidbody_get_angular_velocity( body ).z;
}

void wp_rigidbody2_set_angular_velocity( wp_rigidbody *body, wp_f32 velocity )
{
    wp_vec3f angular_velocity;
    angular_velocity.x = 0.0f;
    angular_velocity.y = 0.0f;
    angular_velocity.z = velocity;
    wp_rigidbody_set_angular_velocity( body, angular_velocity );
}

wp_vec2f wp_rigidbody2_get_force( const wp_rigidbody *body )
{
    return wp_physics2_from_vec3( wp_rigidbody_get_accumulated_force( body ) );
}

void wp_rigidbody2_set_force( wp_rigidbody *body, wp_vec2f force )
{
    wp_rigidbody_clear_force( body );
    wp_rigidbody_add_force( body, wp_physics2_to_vec3( force ), WORKPHONE_FORCE_MODE_FORCE );
}

void wp_rigidbody2_add_force( wp_rigidbody *body, wp_vec2f force )
{
    wp_rigidbody_add_force( body, wp_physics2_to_vec3( force ), WORKPHONE_FORCE_MODE_FORCE );
}

wp_f32 wp_rigidbody2_get_torque( const wp_rigidbody *body )
{
    return wp_rigidbody_get_accumulated_torque( body ).z;
}

void wp_rigidbody2_set_torque( wp_rigidbody *body, wp_f32 torque )
{
    wp_vec3f torque3;
    torque3.x = 0.0f;
    torque3.y = 0.0f;
    torque3.z = torque;
    wp_rigidbody_clear_torque( body );
    wp_rigidbody_add_torque( body, torque3, WORKPHONE_FORCE_MODE_FORCE );
}

void wp_rigidbody2_add_torque( wp_rigidbody *body, wp_f32 torque )
{
    wp_vec3f torque3;
    torque3.x = 0.0f;
    torque3.y = 0.0f;
    torque3.z = torque;
    wp_rigidbody_add_torque( body, torque3, WORKPHONE_FORCE_MODE_FORCE );
}

wp_vec2f wp_physics_scene2_get_size( const wp_physics_scene *scene )
{
    return wp_physics2_from_vec3( wp_physics_scene_get_size( scene ) );
}

void wp_physics_scene2_set_size( wp_physics_scene *scene, wp_vec2f size )
{
    wp_physics_scene_set_size( scene, wp_physics2_to_vec3( size ) );
}

wp_vec2f wp_physics_scene2_get_gravity( const wp_physics_scene *scene )
{
    return wp_physics2_from_vec3( wp_physics_scene_get_gravity( scene ) );
}

void wp_physics_scene2_set_gravity( wp_physics_scene *scene, wp_vec2f gravity )
{
    wp_physics_scene_set_gravity( scene, wp_physics2_to_vec3( gravity ) );
}

void wp_physics_scene2_simulate( wp_physics_scene *scene, wp_f32 dt )
{
    wp_physics_scene_simulate( scene, dt );
}
