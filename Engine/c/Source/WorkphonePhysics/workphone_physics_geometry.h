#ifndef WORKPHONE_PHYSICS_GEOMETRY_H
#define WORKPHONE_PHYSICS_GEOMETRY_H

#include "workphone_physics_collisionshape.h"
#include "workphone_physics_rigidbody.h"

/* Per-shape storage; revisions are checked at consumption, including after a
 * preceding solver contact moves the body. No scene lookup or heap allocation. */
typedef struct wp_prepared_shape
{
    const wp_rigidbody *body;
    uint64_t body_id, body_revision, shape_revision;
    wp_vec3f center, axes[3], half, minimum, maximum, segment_start, segment_end;
    wp_quatf orientation, inverse;
    wp_f32 radius, half_height, roundoff;
    wp_s32 bounds_valid, borrowed_vertices;
} wp_prepared_shape;

wp_prepared_shape *wp_collision_shape_get_prepared_storage( const wp_collision_shape *shape );
uint64_t wp_collision_shape_get_revision( const wp_collision_shape *shape );
uint64_t wp_collision_shape_get_lifetime_id( const wp_collision_shape *shape );
uint64_t wp_rigidbody_get_lifetime_id( const wp_rigidbody *body );
uint64_t wp_physics_next_lifetime_id( void );
const wp_prepared_shape *wp_shape_prepare( const wp_rigidbody *body,
                                           const wp_collision_shape *shape, wp_s32 *rebuilt );
wp_s32 wp_prepared_shapes_may_overlap( const wp_prepared_shape *a, const wp_prepared_shape *b,
                                       wp_f32 tolerance );

#endif
