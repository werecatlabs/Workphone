/**
 * @file workphone_physics_broadphase.h
 * @brief C API for the physics broadphase system.
 *
 * The broadphase is responsible for quickly eliminating pairs of objects
 * that are too far apart to collide, before passing candidate pairs to
 * the narrowphase. It manages axis-aligned bounding boxes (AABBs) for
 * all objects in a scene and produces an overlapping-pair cache that
 * drives the rest of the collision pipeline.
 */

#ifndef workphone_physics_broadphase_h__
#define workphone_physics_broadphase_h__

#include <stdint.h>
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_broadphase wp_broadphase;
typedef struct wp_broadphase_pair wp_broadphase_pair;
typedef struct wp_rigidbody wp_rigidbody;

/* -------------------------------------------------------------------------
 * Broadphase type
 * ---------------------------------------------------------------------- */

typedef enum wp_broadphase_type
{
    WORKPHONE_BROADPHASE_SAP = 0,  /**< Sweep-and-prune */
    WORKPHONE_BROADPHASE_DBVT = 1, /**< Dynamic AABB tree */
    WORKPHONE_BROADPHASE_MBP = 2   /**< Multi-box pruning */
} wp_broadphase_type;

/* -------------------------------------------------------------------------
 * Overlapping pair
 * ---------------------------------------------------------------------- */

typedef struct wp_broadphase_pair
{
    wp_rigidbody *body_a;
    wp_rigidbody *body_b;
} wp_broadphase_pair;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_broadphase *wp_broadphase_create( wp_broadphase_type type );
void wp_broadphase_destroy( wp_broadphase *bp );

/* =========================================================================
 * Configuration
 * ====================================================================== */

wp_broadphase_type wp_broadphase_get_type( const wp_broadphase *bp );

wp_vec3f wp_broadphase_get_world_min( const wp_broadphase *bp );
void wp_broadphase_set_world_min( wp_broadphase *bp, wp_vec3f min );

wp_vec3f wp_broadphase_get_world_max( const wp_broadphase *bp );
void wp_broadphase_set_world_max( wp_broadphase *bp, wp_vec3f max );

/* =========================================================================
 * Proxy management
 *
 * Each rigid body is represented by a proxy (an AABB handle) inside the
 * broadphase. Proxies must be added before the broadphase will generate
 * overlap pairs for the corresponding body.
 * ====================================================================== */

wp_s32 wp_broadphase_add_proxy( wp_broadphase *bp, wp_rigidbody *body, wp_vec3f aabb_min,
                                wp_vec3f aabb_max );
void wp_broadphase_remove_proxy( wp_broadphase *bp, wp_rigidbody *body );
void wp_broadphase_update_proxy( wp_broadphase *bp, wp_rigidbody *body, wp_vec3f aabb_min,
                                 wp_vec3f aabb_max );

/* =========================================================================
 * Overlap pair cache
 * ====================================================================== */

void wp_broadphase_calculate_overlapping_pairs( wp_broadphase *bp );
const wp_broadphase_pair *wp_broadphase_get_pair_cache( const wp_broadphase *bp );
wp_s32 wp_broadphase_get_pair_count( const wp_broadphase *bp );

/* =========================================================================
 * AABB query
 * ====================================================================== */

wp_s32 wp_broadphase_query_aabb( wp_broadphase *bp, wp_vec3f aabb_min, wp_vec3f aabb_max,
                                 wp_rigidbody **out_bodies, wp_s32 max_results );

/* =========================================================================
 * Statistics
 * ====================================================================== */

wp_s32 wp_broadphase_get_proxy_count( const wp_broadphase *bp );

/* =========================================================================
 * Native / user data
 * ====================================================================== */

void *wp_broadphase_get_native( const wp_broadphase *bp );
void wp_broadphase_set_native( wp_broadphase *bp, void *native );
void *wp_broadphase_get_user_data( const wp_broadphase *bp );
void wp_broadphase_set_user_data( wp_broadphase *bp, void *user_data );

#ifdef __cplusplus
}
#endif

#endif  // workphone_physics_broadphase_h__
