/**
 * @file workphone_game_component_rigidbody.h
 * @brief C API for the rigidbody game component.
 *
 * Bridges the game actor system with the physics module.  A rigidbody
 * component owns a wp_rigidbody, syncs transforms between the actor and the
 * physics body, and exposes convenience wrappers for forces, velocities,
 * damping and collision filtering.
 */

#ifndef WORKPHONE_GAME_COMPONENT_RIGIDBODY_H
#define WORKPHONE_GAME_COMPONENT_RIGIDBODY_H

#include <stdint.h>
#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"
#include "workphone_game_component.h"
#include "workphone_game_actor.h"
#include "workphone_physics_rigidbody.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declaration
 * ---------------------------------------------------------------------- */

struct wp_rigidbody_component;

/* -------------------------------------------------------------------------
 * Component-level physics callbacks
 * ---------------------------------------------------------------------- */

typedef struct wp_rigidbody_component_callbacks
{
    void ( *on_collision_enter )( struct wp_rigidbody_component *self,
                                  struct wp_rigidbody_component *other );
    void ( *on_collision_stay )( struct wp_rigidbody_component *self,
                                 struct wp_rigidbody_component *other );
    void ( *on_collision_exit )( struct wp_rigidbody_component *self,
                                 struct wp_rigidbody_component *other );
    void ( *on_trigger_enter )( struct wp_rigidbody_component *self,
                                struct wp_rigidbody_component *other );
    void ( *on_trigger_exit )( struct wp_rigidbody_component *self,
                               struct wp_rigidbody_component *other );
    void ( *on_sleep )( struct wp_rigidbody_component *self );
    void ( *on_wake )( struct wp_rigidbody_component *self );
} wp_rigidbody_component_callbacks;

/* -------------------------------------------------------------------------
 * Rigidbody component
 *
 * The base wp_game_component is the first member so a
 * wp_rigidbody_component * can be safely cast to wp_game_component * and back.
 * ---------------------------------------------------------------------- */

typedef struct wp_rigidbody_component
{
    wp_game_component base;

    wp_rigidbody *body;
    wp_rigidbody_type body_type;

    wp_f32 mass;
    wp_f32 linear_damping;
    wp_f32 angular_damping;

    wp_s32 use_gravity;
    wp_s32 is_kinematic;

    wp_u32 collision_type;
    wp_u32 collision_mask;

    wp_rigidbody_component_callbacks rb_callbacks;
    void *rb_user_data;
} wp_rigidbody_component;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_rigidbody_component_init( wp_rigidbody_component *rc );
void wp_rigidbody_component_init_with_type( wp_rigidbody_component *rc, wp_rigidbody_type body_type );
void wp_rigidbody_component_destroy( wp_rigidbody_component *rc );

/* ---- Update ----------------------------------------------------------- */

/**
 * @brief Called each frame; for dynamic bodies, reads back the simulated
 *        transform from the physics body and writes it to the actor.
 */
void wp_rigidbody_component_update( wp_rigidbody_component *rc, wp_f64 dt );

/**
 * @brief Pushes the actor's current world transform into the physics body.
 *
 * Use for teleporting or resetting a body; always applied to kinematic
 * and static bodies.
 */
void wp_rigidbody_component_sync_to_physics( wp_rigidbody_component *rc );

/**
 * @brief Reads the physics body's simulated transform back into the actor.
 *
 * Typically called after the physics step for dynamic bodies.
 */
void wp_rigidbody_component_sync_from_physics( wp_rigidbody_component *rc );

/* ---- Physics body access --------------------------------------------- */

wp_rigidbody *wp_rigidbody_component_get_body( const wp_rigidbody_component *rc );
wp_rigidbody_type wp_rigidbody_component_get_body_type( const wp_rigidbody_component *rc );
void wp_rigidbody_component_set_body_type( wp_rigidbody_component *rc, wp_rigidbody_type type );

/* ---- Mass ------------------------------------------------------------- */

void wp_rigidbody_component_set_mass( wp_rigidbody_component *rc, wp_f32 mass );
wp_f32 wp_rigidbody_component_get_mass( const wp_rigidbody_component *rc );

/* ---- Damping ---------------------------------------------------------- */

void wp_rigidbody_component_set_linear_damping( wp_rigidbody_component *rc, wp_f32 damping );
wp_f32 wp_rigidbody_component_get_linear_damping( const wp_rigidbody_component *rc );

void wp_rigidbody_component_set_angular_damping( wp_rigidbody_component *rc, wp_f32 damping );
wp_f32 wp_rigidbody_component_get_angular_damping( const wp_rigidbody_component *rc );

/* ---- Gravity / kinematic --------------------------------------------- */

void wp_rigidbody_component_set_use_gravity( wp_rigidbody_component *rc, wp_s32 enabled );
wp_s32 wp_rigidbody_component_get_use_gravity( const wp_rigidbody_component *rc );

void wp_rigidbody_component_set_kinematic( wp_rigidbody_component *rc, wp_s32 kinematic );
wp_s32 wp_rigidbody_component_is_kinematic( const wp_rigidbody_component *rc );

/* ---- Velocity --------------------------------------------------------- */

void wp_rigidbody_component_set_linear_velocity( wp_rigidbody_component *rc, wp_vec3f v );
wp_vec3f wp_rigidbody_component_get_linear_velocity( const wp_rigidbody_component *rc );

void wp_rigidbody_component_set_angular_velocity( wp_rigidbody_component *rc, wp_vec3f v );
wp_vec3f wp_rigidbody_component_get_angular_velocity( const wp_rigidbody_component *rc );

/* ---- Forces and torques ----------------------------------------------- */

void wp_rigidbody_component_add_force( wp_rigidbody_component *rc, wp_vec3f force, wp_force_mode mode );
void wp_rigidbody_component_add_torque( wp_rigidbody_component *rc, wp_vec3f torque,
                                        wp_force_mode mode );
void wp_rigidbody_component_clear_forces( wp_rigidbody_component *rc );

/* ---- Sleep ------------------------------------------------------------ */

wp_s32 wp_rigidbody_component_is_sleeping( const wp_rigidbody_component *rc );
void wp_rigidbody_component_wake_up( wp_rigidbody_component *rc );
void wp_rigidbody_component_put_to_sleep( wp_rigidbody_component *rc );

/* ---- Collision filtering ---------------------------------------------- */

void wp_rigidbody_component_set_collision_type( wp_rigidbody_component *rc, wp_u32 type );
wp_u32 wp_rigidbody_component_get_collision_type( const wp_rigidbody_component *rc );

void wp_rigidbody_component_set_collision_mask( wp_rigidbody_component *rc, wp_u32 mask );
wp_u32 wp_rigidbody_component_get_collision_mask( const wp_rigidbody_component *rc );

/* ---- Collision callbacks ---------------------------------------------- */

void wp_rigidbody_component_set_callbacks( wp_rigidbody_component *rc,
                                           wp_rigidbody_component_callbacks cbs );

/* ---- User data -------------------------------------------------------- */

void *wp_rigidbody_component_get_user_data( const wp_rigidbody_component *rc );
void wp_rigidbody_component_set_user_data( wp_rigidbody_component *rc, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_COMPONENT_RIGIDBODY_H */
