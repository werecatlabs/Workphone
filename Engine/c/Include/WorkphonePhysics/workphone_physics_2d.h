/**
 * @file workphone_physics_2d.h
 * @brief 2D convenience API over the Workphone C physics storage.
 */

#ifndef WORKPHONE_PHYSICS_2D_H
#define WORKPHONE_PHYSICS_2D_H

#include "workphone_vector.h"
#include "workphone_physics_collisionshape.h"
#include "workphone_physics_rigidbody.h"
#include "workphone_physics_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

wp_collision_shape *wp_collision_shape2_create_box( wp_vec2f half_extents );
wp_collision_shape *wp_collision_shape2_create_sphere( wp_f32 radius );
void wp_collision_shape2_set_box_half_extents( wp_collision_shape *shape, wp_vec2f half_extents );
wp_vec2f wp_collision_shape2_get_box_half_extents( const wp_collision_shape *shape );
void wp_collision_shape2_set_sphere_radius( wp_collision_shape *shape, wp_f32 radius );
wp_f32 wp_collision_shape2_get_sphere_radius( const wp_collision_shape *shape );

wp_rigidbody *wp_rigidbody2_create( wp_rigidbody_type type );
wp_vec2f wp_rigidbody2_get_position( const wp_rigidbody *body );
void wp_rigidbody2_set_position( wp_rigidbody *body, wp_vec2f position );
wp_f32 wp_rigidbody2_get_orientation( const wp_rigidbody *body );
void wp_rigidbody2_set_orientation( wp_rigidbody *body, wp_f32 radians );
wp_vec2f wp_rigidbody2_get_linear_velocity( const wp_rigidbody *body );
void wp_rigidbody2_set_linear_velocity( wp_rigidbody *body, wp_vec2f velocity );
wp_f32 wp_rigidbody2_get_angular_velocity( const wp_rigidbody *body );
void wp_rigidbody2_set_angular_velocity( wp_rigidbody *body, wp_f32 velocity );
wp_vec2f wp_rigidbody2_get_force( const wp_rigidbody *body );
void wp_rigidbody2_set_force( wp_rigidbody *body, wp_vec2f force );
void wp_rigidbody2_add_force( wp_rigidbody *body, wp_vec2f force );
wp_f32 wp_rigidbody2_get_torque( const wp_rigidbody *body );
void wp_rigidbody2_set_torque( wp_rigidbody *body, wp_f32 torque );
void wp_rigidbody2_add_torque( wp_rigidbody *body, wp_f32 torque );

wp_vec2f wp_physics_scene2_get_size( const wp_physics_scene *scene );
void wp_physics_scene2_set_size( wp_physics_scene *scene, wp_vec2f size );
wp_vec2f wp_physics_scene2_get_gravity( const wp_physics_scene *scene );
void wp_physics_scene2_set_gravity( wp_physics_scene *scene, wp_vec2f gravity );
void wp_physics_scene2_simulate( wp_physics_scene *scene, wp_f32 dt );

#ifdef __cplusplus
}
#endif

#endif
