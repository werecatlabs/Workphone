/**
 * @file workphone_physics_narrowphase.h
 * @brief C API for the physics narrowphase system.
 *
 * The narrowphase receives candidate overlapping pairs from the broadphase
 * and performs exact shape-vs-shape collision tests to produce contact
 * manifolds. Each manifold describes the contact points, penetration depth,
 * and contact normal for a single colliding pair and is consumed by the
 * constraint solver to generate response impulses.
 */

#ifndef workphone_physics_narrowphase_h__
#define workphone_physics_narrowphase_h__

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_physics_broadphase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_narrowphase wp_narrowphase;
typedef struct wp_contact_manifold wp_contact_manifold;
typedef struct wp_contact_point wp_contact_point;
typedef struct wp_collision_shape wp_collision_shape;
typedef struct wp_rigidbody wp_rigidbody;

/* -------------------------------------------------------------------------
 * Limits
 * ---------------------------------------------------------------------- */

/** Maximum contact points stored per manifold (matches typical physics engines). */
#ifndef WP_MANIFOLD_MAX_CONTACTS
#    define WP_MANIFOLD_MAX_CONTACTS 4
#endif

/* -------------------------------------------------------------------------
 * Contact point
 *
 * A single point of contact between two shapes, in world space.
 * ---------------------------------------------------------------------- */

typedef struct wp_contact_point
{
    wp_vec3f position_world_on_a; /**< Contact point on shape A in world space.   */
    wp_vec3f position_world_on_b; /**< Contact point on shape B in world space.   */
    wp_vec3f normal_world_on_b;   /**< Contact normal pointing from A toward B.   */
    wp_f32 penetration_depth;     /**< Signed penetration depth (positive = overlap). */
    wp_f32 combined_friction;     /**< Pre-combined friction coefficient.          */
    wp_f32 combined_restitution;  /**< Pre-combined restitution coefficient.       */
} wp_contact_point;

/* -------------------------------------------------------------------------
 * Contact manifold
 *
 * Describes all contact points between one pair of shapes for a single
 * simulation step.
 * ---------------------------------------------------------------------- */

typedef struct wp_contact_manifold
{
    wp_rigidbody *body_a;
    wp_rigidbody *body_b;
    wp_collision_shape *shape_a;
    wp_collision_shape *shape_b;
    wp_contact_point contacts[WP_MANIFOLD_MAX_CONTACTS];
    wp_s32 contact_count;
    wp_s32 is_touching; /**< Non-zero when at least one contact exists. */
} wp_contact_manifold;

/* -------------------------------------------------------------------------
 * Narrowphase algorithm
 * ---------------------------------------------------------------------- */

typedef enum wp_narrowphase_algorithm
{
    /* Compatibility selectors: supported pairs currently use dedicated kernels. */
    WORKPHONE_NARROWPHASE_GJK_EPA = 0,
    WORKPHONE_NARROWPHASE_SAT = 1,
    WORKPHONE_NARROWPHASE_HYBRID = 2
} wp_narrowphase_algorithm;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_narrowphase *wp_narrowphase_create( wp_narrowphase_algorithm algorithm );
void wp_narrowphase_destroy( wp_narrowphase *np );

/* =========================================================================
 * Configuration
 * ====================================================================== */

wp_narrowphase_algorithm wp_narrowphase_get_algorithm( const wp_narrowphase *np );

/** Compatibility setting; dedicated primitive kernels do not use this limit. */
wp_s32 wp_narrowphase_get_max_iterations( const wp_narrowphase *np );
void wp_narrowphase_set_max_iterations( wp_narrowphase *np, wp_s32 iterations );

/** Distance tolerance below which two shapes are considered touching. */
wp_f32 wp_narrowphase_get_contact_tolerance( const wp_narrowphase *np );
void wp_narrowphase_set_contact_tolerance( wp_narrowphase *np, wp_f32 tolerance );

/* =========================================================================
 * Dispatch
 *
 * Process all candidate pairs produced by the broadphase and populate the
 * internal manifold cache. Call wp_narrowphase_get_manifold_cache /
 * wp_narrowphase_get_manifold_count afterwards to read the results.
 * ====================================================================== */

void wp_narrowphase_process_pairs( wp_narrowphase *np, const wp_broadphase_pair *pairs,
                                   wp_s32 pair_count );
/* Complete checked batch; allocation failure clears the batch and returns 0.
 * Empty input resets counts. The legacy void wrapper calls this implementation. */
wp_s32 wp_narrowphase_process_pairs_checked( wp_narrowphase *np, const wp_broadphase_pair *pairs,
                                            wp_s32 pair_count );

/* =========================================================================
 * Single-pair test
 *
 * Test one explicit pair and write the result into *out_manifold.
 * Returns non-zero if the shapes are touching.
 * ====================================================================== */

wp_s32 wp_narrowphase_test_pair( wp_narrowphase *np, wp_rigidbody *body_a, wp_collision_shape *shape_a,
                                 wp_rigidbody *body_b, wp_collision_shape *shape_b,
                                 wp_contact_manifold *out_manifold );

/* =========================================================================
 * Manifold cache
 *
 * Valid after wp_narrowphase_process_pairs. The pointer is owned by the
 * narrowphase and remains valid until the next call to process_pairs.
 * ====================================================================== */

const wp_contact_manifold *wp_narrowphase_get_manifold_cache( const wp_narrowphase *np );
wp_s32 wp_narrowphase_get_manifold_count( const wp_narrowphase *np );

/* =========================================================================
 * Statistics
 * ====================================================================== */

wp_s32 wp_narrowphase_get_touching_pair_count( const wp_narrowphase *np );
typedef struct wp_narrowphase_stats
{
    uint64_t pair_calls[5][5], pair_nanoseconds[5][5];
    uint64_t prepared_rebuilds, prepared_reuses;
    uint64_t mesh_nodes, mesh_candidates, tested_triangles, full_scan_fallbacks;
    uint64_t simd_batches, oriented_tests, oriented_rejections;
    uint64_t contacts_generated, contacts_retained;
} wp_narrowphase_stats;
wp_narrowphase_stats wp_narrowphase_get_stats( const wp_narrowphase *np );
void wp_narrowphase_reset_stats( wp_narrowphase *np );
/* Timing and SIMD are optional and disabled by default. */
void wp_narrowphase_set_timing_enabled( wp_narrowphase *np, wp_s32 enabled );
wp_s32 wp_narrowphase_set_simd_enabled( wp_narrowphase *np, wp_s32 enabled );
/* Reference/profiling controls: exact contact kernels are shared. */
void wp_narrowphase_set_mesh_acceleration_enabled( wp_narrowphase *np, wp_s32 enabled );
/* Enabled by default; applies face-axis rejection only to boxes whose local
 * AABB volume exceeds their OBB volume by 1.5x. */
void wp_narrowphase_set_mesh_obb_enabled( wp_narrowphase *np, wp_s32 enabled );

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_narrowphase_get_native( const wp_narrowphase *np );
void wp_narrowphase_set_native( wp_narrowphase *np, void *native );
void *wp_narrowphase_get_user_data( const wp_narrowphase *np );
void wp_narrowphase_set_user_data( wp_narrowphase *np, void *user_data );

#ifdef __cplusplus
}
#endif

#endif  // workphone_physics_narrowphase_h__
