/**
 * @file wp_physics_scene.h
 * @brief C API for a 3D physics scene.
 *
 * A physics scene is a simulation environment that holds rigid bodies,
 * applies gravity, steps the simulation, and supports raycasting and
 * line-intersection queries. Multiple scenes can exist simultaneously
 * inside a wp_physics_system.
 */

#ifndef WORKPHONE_PHYSICS_SCENE_H
#define WORKPHONE_PHYSICS_SCENE_H

#include <stdint.h>
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_physics_scene wp_physics_scene;
typedef struct wp_rigidbody wp_rigidbody;
typedef struct wp_collision_shape wp_collision_shape;
typedef struct wp_raycast_hit wp_raycast_hit;

#ifndef WP_SCENE_MAX_ACTORS
#    define WP_SCENE_MAX_ACTORS 256
#endif

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_physics_scene *wp_physics_scene_create( void );
void wp_physics_scene_destroy( wp_physics_scene *scene );
void wp_physics_scene_clear( wp_physics_scene *scene );

/* =========================================================================
 * Actor management
 * ====================================================================== */

wp_s32 wp_physics_scene_add_actor( wp_physics_scene *scene, wp_rigidbody *body );
void wp_physics_scene_remove_actor( wp_physics_scene *scene, wp_rigidbody *body );
wp_rigidbody *wp_physics_scene_get_actor( const wp_physics_scene *scene, wp_s32 index );
wp_s32 wp_physics_scene_get_actor_count( const wp_physics_scene *scene );

/* =========================================================================
 * Scene bounds
 * ====================================================================== */

wp_vec3f wp_physics_scene_get_size( const wp_physics_scene *scene );
void wp_physics_scene_set_size( wp_physics_scene *scene, wp_vec3f size );

/* =========================================================================
 * Gravity
 * ====================================================================== */

wp_vec3f wp_physics_scene_get_gravity( const wp_physics_scene *scene );
void wp_physics_scene_set_gravity( wp_physics_scene *scene, wp_vec3f gravity );

/* =========================================================================
 * Spatial Partitioning
 * ====================================================================== */

typedef enum wp_contact_update_strategy {
    WP_CONTACT_STRATEGY_ALWAYS = 0,
    WP_CONTACT_STRATEGY_FIXED,
    WP_CONTACT_STRATEGY_DISTANCE,
    WP_CONTACT_STRATEGY_COUNT
} wp_contact_update_strategy;

typedef struct wp_contact_options {
    wp_contact_update_strategy strategy;
    wp_f32 separation_threshold;
    wp_u32 fixed_update_frequency;
    wp_f32 distance_frequency_scale;
} wp_contact_options;

typedef struct wp_spatial_partitioning_options {
    wp_f32 update_rate_multiplier;
    wp_f32 sleep_threshold;
    wp_u32 update_frequency;
} wp_spatial_partitioning_options;

typedef enum wp_spatial_partitioning_method {
    WP_SPATIAL_PARTITION_NONE = 0,
    WP_SPATIAL_PARTITION_GRID,
    WP_SPATIAL_PARTITION_OCTREE,
    WP_SPATIAL_PARTITION_BVH,
    WP_SPATIAL_PARTITION_COUNT
} wp_spatial_partitioning_method;

void wp_physics_scene_set_spatial_partitioning( wp_physics_scene *scene, wp_spatial_partitioning_method method );
wp_spatial_partitioning_method wp_physics_scene_get_spatial_partitioning( const wp_physics_scene *scene );

/* =========================================================================
 * Simulation
 * ====================================================================== */

void wp_physics_scene_simulate( wp_physics_scene *scene, wp_f32 dt );
wp_s32 wp_physics_scene_fetch_results( wp_physics_scene *scene, wp_s32 block );

/* =========================================================================
 * Threading
 * ====================================================================== */

wp_u32 wp_physics_scene_get_min_threads( const wp_physics_scene *scene );
void wp_physics_scene_set_min_threads( wp_physics_scene *scene, wp_u32 min_threads );
wp_u32 wp_physics_scene_get_max_threads( const wp_physics_scene *scene );
void wp_physics_scene_set_max_threads( wp_physics_scene *scene, wp_u32 max_threads );

/* =========================================================================
 * Raycasting
 * ====================================================================== */

#define WORKPHONE_SCENE_QUERY_STATIC ( 1u << 0 )
#define WORKPHONE_SCENE_QUERY_DYNAMIC ( 1u << 1 )
#define WORKPHONE_SCENE_QUERY_KINEMATIC ( 1u << 2 )
#define WORKPHONE_SCENE_QUERY_ALL \
    ( WORKPHONE_SCENE_QUERY_STATIC | WORKPHONE_SCENE_QUERY_DYNAMIC | WORKPHONE_SCENE_QUERY_KINEMATIC )

wp_s32 wp_physics_scene_ray_test( wp_physics_scene *scene, wp_vec3f start, wp_vec3f direction,
                                  wp_vec3f *out_hit_pos, wp_vec3f *out_hit_normal, wp_u32 collision_type,
                                  wp_u32 collision_mask );

wp_s32 wp_physics_scene_intersects( wp_physics_scene *scene, wp_vec3f start, wp_vec3f end,
                                    wp_vec3f *out_hit_pos, wp_vec3f *out_hit_normal,
                                    wp_u32 collision_type, wp_u32 collision_mask );

wp_s32 wp_physics_scene_intersects_ex( wp_physics_scene *scene, wp_vec3f start, wp_vec3f end,
                                       wp_vec3f *out_hit_pos, wp_vec3f *out_hit_normal,
                                       wp_rigidbody **out_body, wp_collision_shape **out_shape,
                                       wp_u32 collision_type, wp_u32 collision_mask );

wp_s32 wp_physics_scene_intersects_actor_types_ex( wp_physics_scene *scene, wp_vec3f start, wp_vec3f end,
                                                   wp_vec3f *out_hit_pos, wp_vec3f *out_hit_normal,
                                                   wp_rigidbody **out_body,
                                                   wp_collision_shape **out_shape, wp_u32 collision_type,
                                                   wp_u32 collision_mask, wp_u32 actor_types );

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_physics_scene_get_native( const wp_physics_scene *scene );
void wp_physics_scene_set_native( wp_physics_scene *scene, void *native );
void *wp_physics_scene_get_user_data( const wp_physics_scene *scene );
void wp_physics_scene_set_user_data( wp_physics_scene *scene, void *user_data );

#ifdef __cplusplus
}
#endif

#endif



