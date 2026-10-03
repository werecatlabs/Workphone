/**
 * @file workphone_vehicle_softbody.h
 * @brief C89 API for a deformable soft-body vehicle chassis.
 *
 * Ported from the Unity C# SoftBodyVehicle implementation. The chassis is
 * modelled as a graph of point masses (nodes) connected by spring-damper
 * beams. Beams support elastic deformation, plastic yielding and fracture.
 *
 * The module is physics-engine agnostic. Collision queries are performed
 * through a caller-supplied raycast callback so the existing WorkphonePhysics
 * scene can be wired in by a game component.
 */

#ifndef WORKPHONE_VEHICLE_SOFTBODY_H
#define WORKPHONE_VEHICLE_SOFTBODY_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"
#include "workphone_game_actor.h"
#include "workphone_game_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WP_SOFTBODY_VEHICLE_MAX_NAME
#    define WP_SOFTBODY_VEHICLE_MAX_NAME 64
#endif

#ifndef WP_SOFTBODY_VEHICLE_MAX_RAYCAST_HITS
#    define WP_SOFTBODY_VEHICLE_MAX_RAYCAST_HITS 16
#endif

/**
 * @brief Result of a single raycast/collision probe used by the soft-body
 *        node collision resolver.
 */
typedef struct wp_softbody_raycast_hit
{
    wp_s32 hit;          /**< Non-zero if a hit was found. */
    wp_vec3f point;      /**< World-space hit point. */
    wp_vec3f normal;     /**< World-space hit normal. */
    wp_f32 distance;     /**< Distance from the ray origin to the hit. */
    void *body;          /**< Opaque handle to the hit rigid body, or NULL. */
} wp_softbody_raycast_hit;

/**
 * @brief Callback signature for sphere/line collision probes.
 *
 * The implementation should return the closest hit within max_distance that
 * matches collision_mask. The caller is responsible for ignoring colliders
 * belonging to the vehicle itself.
 */
typedef wp_softbody_raycast_hit ( *wp_softbody_raycast_fn )( void *user_data,
                                                              wp_vec3f origin,
                                                              wp_vec3f direction,
                                                              wp_f32 max_distance,
                                                              wp_u32 collision_mask );

/**
 * @brief Optional callback for applying a collision impulse to the hit body.
 */
typedef void ( *wp_softbody_apply_impulse_fn )( void *user_data, void *body,
                                                wp_vec3f point, wp_vec3f impulse );

/**
 * @brief Collision callbacks supplied to the soft-body vehicle.
 */
typedef wp_vec3f ( *wp_softbody_get_point_velocity_fn )( void *user_data, void *body,
                                                            wp_vec3f point );

typedef void ( *wp_softbody_add_force_at_position_fn )( void *user_data, void *body,
                                                        wp_vec3f point, wp_vec3f force );

typedef struct wp_softbody_vehicle_callbacks
{
    wp_softbody_raycast_fn raycast;
    wp_softbody_get_point_velocity_fn get_point_velocity;
    wp_softbody_add_force_at_position_fn add_force_at_position;
    wp_softbody_apply_impulse_fn apply_impulse; /**< Optional legacy impulse callback. */
    void *user_data;
} wp_softbody_vehicle_callbacks;

/**
 * @brief Node definition used to build the soft-body structure.
 */
typedef struct wp_softbody_node_def
{
    wp_c8 name[WP_SOFTBODY_VEHICLE_MAX_NAME];
    wp_vec3f local_position;
    wp_f32 mass;
    wp_f32 radius;
    wp_s32 anchored;
} wp_softbody_node_def;

/**
 * @brief Beam definition used to build the soft-body structure.
 */
typedef struct wp_softbody_beam_def
{
    wp_s32 node_a;
    wp_s32 node_b;
    wp_f32 stiffness;
    wp_f32 damping;
    wp_f32 yield_strain;
    wp_f32 plasticity;
    wp_f32 break_strain;
} wp_softbody_beam_def;

/**
 * @brief Runtime state for a single soft-body node.
 */
typedef struct wp_softbody_node_state
{
    wp_vec3f position;
    wp_vec3f velocity;
    wp_vec3f force;
    wp_f32 inverse_mass;
    wp_f32 radius;
    wp_s32 anchored;
} wp_softbody_node_state;

/**
 * @brief Runtime state for a single soft-body beam.
 */
typedef struct wp_softbody_beam_state
{
    wp_s32 node_a;
    wp_s32 node_b;
    wp_f32 rest_length;
    wp_f32 stiffness;
    wp_f32 damping;
    wp_f32 yield_strain;
    wp_f32 plasticity;
    wp_f32 break_strain;
    wp_s32 broken;
} wp_softbody_beam_state;

/**
 * @brief Complete state for one soft-body vehicle instance.
 */
typedef struct wp_softbody_vehicle
{
    /* Tunable solver parameters. */
    wp_s32 substeps;
    wp_f32 global_velocity_damping;
    wp_f32 maximum_beam_force;
    wp_f32 maximum_node_speed;

    /* Collision parameters. */
    wp_u32 collision_mask;
    wp_f32 collision_skin;
    wp_f32 restitution;
    wp_f32 collision_friction;

    /* World-space gravity vector applied to dynamic nodes. */
    wp_vec3f gravity;

    /* Runtime state. */
    wp_s32 is_initialized;
    wp_s32 node_count;
    wp_s32 beam_count;

    /* Definition arrays (owned by the caller or copied during set). */
    wp_softbody_node_def *node_definitions;
    wp_softbody_beam_def *beam_definitions;

    /* Allocated runtime arrays. */
    wp_softbody_node_state *nodes;
    wp_softbody_beam_state *beams;
    wp_vec3f *external_forces;

    /* Local-to-world transform used for node initialisation and queries. */
    wp_transform3f transform;

    /* Caller-supplied collision callbacks. */
    wp_softbody_vehicle_callbacks callbacks;
} wp_softbody_vehicle;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Initialise a soft-body vehicle with default parameters.
 */
void wp_softbody_vehicle_init( wp_softbody_vehicle *vehicle );

/**
 * @brief Destroy a soft-body vehicle, freeing all internally allocated
 *        runtime arrays.
 */
void wp_softbody_vehicle_destroy( wp_softbody_vehicle *vehicle );

/* -------------------------------------------------------------------------
 * Configuration
 * ---------------------------------------------------------------------- */

/**
 * @brief Replace the node and beam definitions and rebuild runtime state.
 *
 * The definition arrays are copied internally. The original arrays may be
 * freed by the caller after this call returns.
 */
void wp_softbody_vehicle_set_definitions( wp_softbody_vehicle *vehicle,
                                           const wp_softbody_node_def *nodes, wp_s32 node_count,
                                           const wp_softbody_beam_def *beams, wp_s32 beam_count );

/**
 * @brief Install collision callbacks.
 */
void wp_softbody_vehicle_set_callbacks( wp_softbody_vehicle *vehicle,
                                        wp_softbody_vehicle_callbacks callbacks );

/**
 * @brief Update the world-space transform used for node initialisation and
 *        frame queries.
 */
void wp_softbody_vehicle_set_transform( wp_softbody_vehicle *vehicle, wp_transform3f transform );

/* -------------------------------------------------------------------------
 * Simulation
 * ---------------------------------------------------------------------- */

/**
 * @brief Advance the soft-body simulation by dt seconds.
 *
 * Internally subdivides the time step into vehicle->substeps substeps and
 * integrates the node/beam system. External forces are cleared after the
 * step, so callers should add wheel and environmental forces each tick.
 */
void wp_softbody_vehicle_step( wp_softbody_vehicle *vehicle, wp_f32 dt );

/* -------------------------------------------------------------------------
 * Forces
 * ---------------------------------------------------------------------- */

/**
 * @brief Accumulate a world-space force on a node.
 */
void wp_softbody_vehicle_add_force( wp_softbody_vehicle *vehicle, wp_s32 node_index, wp_vec3f force );

/**
 * @brief Apply a world-space impulse to a node, changing its velocity
 *        directly.
 */
void wp_softbody_vehicle_add_impulse( wp_softbody_vehicle *vehicle, wp_s32 node_index,
                                      wp_vec3f impulse );

/**
 * @brief Clear all externally accumulated forces.
 */
void wp_softbody_vehicle_clear_external_forces( wp_softbody_vehicle *vehicle );

/* -------------------------------------------------------------------------
 * Queries
 * ---------------------------------------------------------------------- */

/**
 * @brief Returns non-zero if the vehicle has been initialised with at least
 *        one node.
 */
wp_s32 wp_softbody_vehicle_is_initialized( const wp_softbody_vehicle *vehicle );

/**
 * @brief Returns the number of nodes, or zero if not initialised.
 */
wp_s32 wp_softbody_vehicle_get_node_count( const wp_softbody_vehicle *vehicle );

/**
 * @brief Returns the number of beams, or zero if not initialised.
 */
wp_s32 wp_softbody_vehicle_get_beam_count( const wp_softbody_vehicle *vehicle );

/**
 * @brief Returns the current world-space position of a node.
 */
wp_vec3f wp_softbody_vehicle_get_node_position( const wp_softbody_vehicle *vehicle,
                                                wp_s32 node_index );

/**
 * @brief Returns the current world-space velocity of a node.
 */
wp_vec3f wp_softbody_vehicle_get_node_velocity( const wp_softbody_vehicle *vehicle,
                                               wp_s32 node_index );

/**
 * @brief Returns the rest local-space position of a node.
 */
wp_vec3f wp_softbody_vehicle_get_rest_local_position( const wp_softbody_vehicle *vehicle,
                                                      wp_s32 node_index );

/**
 * @brief Returns non-zero if the given beam is broken.
 */
wp_s32 wp_softbody_vehicle_is_beam_broken( const wp_softbody_vehicle *vehicle, wp_s32 beam_index );

/**
 * @brief Returns the world-space centre of mass of all dynamic nodes.
 */
wp_vec3f wp_softbody_vehicle_get_center_of_mass( const wp_softbody_vehicle *vehicle );

/**
 * @brief Derive a right/up/forward frame from four corner nodes.
 */
void wp_softbody_vehicle_get_frame( const wp_softbody_vehicle *vehicle,
                                     wp_s32 front_left_node, wp_s32 front_right_node,
                                     wp_s32 rear_left_node, wp_s32 rear_right_node,
                                     wp_vec3f *out_centre, wp_vec3f *out_right,
                                     wp_vec3f *out_up, wp_vec3f *out_forward );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_SOFTBODY_H */

