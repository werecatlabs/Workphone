/* Private conservative body bounds. Public broadphase queries remain AABB queries. */
#ifndef WORKPHONE_PHYSICS_BOUNDS_H
#define WORKPHONE_PHYSICS_BOUNDS_H

#include "workphone_physics_rigidbody.h"

typedef struct wp_body_obb
{
    wp_vec3f center, axes[3], half;
    wp_f32 roundoff;
    wp_s32 valid, useful;
} wp_body_obb;

typedef struct wp_body_obb_cache
{
    uint64_t geometry_revision, pose_revision;
    wp_body_obb local, world;
} wp_body_obb_cache;

/* Storage lives on the opaque body, avoiding scene/body searches per pair. */
wp_body_obb_cache *wp_rigidbody_get_obb_cache( wp_rigidbody *body );
uint64_t wp_rigidbody_get_geometry_revision( const wp_rigidbody *body );
const wp_body_obb *wp_body_get_obb( wp_rigidbody *body, wp_s32 *local_rebuilt,
                                    wp_s32 *world_updated );
/* Six face axes only: conservative rejection, not the exact 15-axis box test.
 * Edge/edge separation can remain a false positive for the narrowphase. */
wp_s32 wp_body_obb_may_overlap( const wp_body_obb *a, const wp_body_obb *b,
                                wp_f32 tolerance );

#endif
