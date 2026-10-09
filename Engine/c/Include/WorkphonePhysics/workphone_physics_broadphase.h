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
    WORKPHONE_BROADPHASE_MBP = 2,  /**< Legacy selection; currently uses the shared AABB tree. */
    WORKPHONE_BROADPHASE_ABP = 3   /**< Legacy automatic-pruning selection. */
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
void wp_broadphase_clear( wp_broadphase *bp );
/* Scalar traversal is the default. Select batched SSE2 traversal when compiled
 * for an SSE2 target. Returns the
 * actual enabled state; unsupported targets retain the scalar implementation. */
wp_s32 wp_broadphase_set_simd_enabled( wp_broadphase *bp, wp_s32 enabled );
wp_s32 wp_broadphase_get_simd_enabled( const wp_broadphase *bp );

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

/* Stable index handles avoid body searches during scene synchronization. Handles
 * become invalid on removal/clear. All selections currently use a dynamic AABB
 * tree. Bounds must be finite, ordered, and conservative for the whole body. */
#define WP_BROADPHASE_INVALID_PROXY ( -1 )
wp_s32 wp_broadphase_create_proxy( wp_broadphase *bp, wp_rigidbody *body,
                                   wp_vec3f aabb_min, wp_vec3f aabb_max );
void wp_broadphase_destroy_proxy( wp_broadphase *bp, wp_s32 proxy );
void wp_broadphase_move_proxy( wp_broadphase *bp, wp_s32 proxy,
                               wp_vec3f aabb_min, wp_vec3f aabb_max );
/* Default enabled=1, movable=1. Disabled proxies are omitted from pairs/queries.
 * Pairs with two immovable proxies are omitted. The order
 * key controls canonical pair order; scene users set it to the actor index. */
void wp_broadphase_configure_proxy( wp_broadphase *bp, wp_s32 proxy,
                                    wp_s32 enabled, wp_s32 movable, wp_u32 order );

typedef void ( *wp_broadphase_pair_callback )( const wp_broadphase_pair *pair, void *context );
/* Visits exact AABB overlaps in order-key order without allocating pair storage.
 * Callbacks must not modify the broadphase. Body transforms may change: bounds
 * are a snapshot and must be synchronized before the next traversal. */
void wp_broadphase_visit_pairs( wp_broadphase *bp, wp_broadphase_pair_callback callback,
                                void *context );

/* =========================================================================
 * Overlap pair cache
 * ====================================================================== */

void wp_broadphase_calculate_overlapping_pairs( wp_broadphase *bp );
/* On allocation failure returns 0 and exposes no partial pair cache. */
wp_s32 wp_broadphase_calculate_overlapping_pairs_checked( wp_broadphase *bp );
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
