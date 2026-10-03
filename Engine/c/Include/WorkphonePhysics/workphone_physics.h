/**
 * @file wp_physics.h
 * @brief C API for the top-level physics system (manager).
 *
 * The physics system owns scenes, rigid bodies, collision shapes,
 * constraints, materials, and raycast-hit results. It acts as a
 * factory and registry for all physics objects and provides
 * convenience functions for raycasting and stepping the simulation.
 */

#ifndef WORKPHONE_PHYSICS_H
#define WORKPHONE_PHYSICS_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_quat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_physics_system wp_physics_system;
typedef struct wp_physics_scene wp_physics_scene;
typedef struct wp_physics_material wp_physics_material;
typedef struct wp_rigidbody wp_rigidbody;
typedef struct wp_collision_shape wp_collision_shape;
typedef struct wp_constraint wp_constraint;
typedef struct wp_raycast_hit wp_raycast_hit;

/* -------------------------------------------------------------------------
 * Material
 * ---------------------------------------------------------------------- */

wp_physics_material *wp_physics_material_create( void );
void wp_physics_material_destroy( wp_physics_material *mat );

wp_f32 wp_physics_material_get_static_friction( const wp_physics_material *mat );
void wp_physics_material_set_static_friction( wp_physics_material *mat, wp_f32 friction );

wp_f32 wp_physics_material_get_dynamic_friction( const wp_physics_material *mat );
void wp_physics_material_set_dynamic_friction( wp_physics_material *mat, wp_f32 friction );

wp_f32 wp_physics_material_get_restitution( const wp_physics_material *mat );
void wp_physics_material_set_restitution( wp_physics_material *mat, wp_f32 restitution );

/* -------------------------------------------------------------------------
 * Data-driven material registry (owned by the system)
 * ---------------------------------------------------------------------- */

struct wp_physics_material_desc;
struct wp_physics_material_database;
typedef struct wp_physics_material_registry wp_physics_material_registry;

wp_s32 wp_physics_system_register_material( wp_physics_system *sys, const struct wp_physics_material_desc *desc );
wp_u32 wp_physics_system_register_material_database( wp_physics_system *sys, const struct wp_physics_material_database *db );
wp_s32 wp_physics_system_unregister_material( wp_physics_system *sys, const wp_c8 *name );
void wp_physics_system_clear_materials( wp_physics_system *sys );
wp_physics_material *wp_physics_system_get_material_by_name( wp_physics_system *sys, const wp_c8 *name );
wp_physics_material *wp_physics_system_get_default_material( wp_physics_system *sys );
wp_physics_material_registry *wp_physics_system_get_material_registry( wp_physics_system *sys );
wp_u32 wp_physics_system_get_material_count( wp_physics_system *sys );
wp_physics_material *wp_physics_system_get_material_by_index( wp_physics_system *sys, wp_u32 index, wp_c8 *out_name );

/* -------------------------------------------------------------------------
 * System lifecycle
 * ---------------------------------------------------------------------- */

wp_physics_system *wp_physics_system_create( void );
void wp_physics_system_destroy( wp_physics_system *sys );

/* -------------------------------------------------------------------------
 * Scene management
 * ---------------------------------------------------------------------- */

wp_physics_scene *wp_physics_system_add_scene( wp_physics_system *sys );
void wp_physics_system_remove_scene( wp_physics_system *sys, wp_physics_scene *scene );
wp_physics_scene *wp_physics_system_get_scene( const wp_physics_system *sys, wp_s32 index );
wp_s32 wp_physics_system_get_scene_count( const wp_physics_system *sys );

/* -------------------------------------------------------------------------
 * Gravity
 * ---------------------------------------------------------------------- */

wp_vec3f wp_physics_system_get_gravity( const wp_physics_system *sys );
void wp_physics_system_set_gravity( wp_physics_system *sys, wp_vec3f gravity );

/* -------------------------------------------------------------------------
 * Simulation step
 * ---------------------------------------------------------------------- */

void wp_physics_system_step( wp_physics_system *sys, wp_f32 dt );

/* -------------------------------------------------------------------------
 * Debug draw
 * ---------------------------------------------------------------------- */

wp_s32 wp_physics_system_get_debug_draw( const wp_physics_system *sys );
void wp_physics_system_set_debug_draw( wp_physics_system *sys, wp_s32 enabled );

/* -------------------------------------------------------------------------
 * Raycasting (convenience wrappers over scene-level queries)
 * ---------------------------------------------------------------------- */

wp_s32 wp_physics_system_raycast( const wp_physics_system *sys, wp_vec3f origin, wp_vec3f direction,
                                  wp_f32 max_distance, wp_u32 collision_mask, wp_raycast_hit *out_hit );

/* -------------------------------------------------------------------------
 * Native / user data
 * ---------------------------------------------------------------------- */

void *wp_physics_system_get_native( const wp_physics_system *sys );
void wp_physics_system_set_native( wp_physics_system *sys, void *native );
void *wp_physics_system_get_user_data( const wp_physics_system *sys );
void wp_physics_system_set_user_data( wp_physics_system *sys, void *user_data );

#ifdef __cplusplus
}
#endif

#endif
