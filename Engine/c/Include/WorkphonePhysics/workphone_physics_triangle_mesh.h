/**
 * @file workphone_physics_triangle_mesh.h
 * @brief Triangle mesh helper used by collision and ray queries.
 */

#ifndef WORKPHONE_PHYSICS_TRIANGLE_MESH_H
#define WORKPHONE_PHYSICS_TRIANGLE_MESH_H

#include <stdint.h>
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_triangle_mesh wp_triangle_mesh;

typedef struct wp_triangle_mesh_query
{
    wp_vec3f minimum, maximum, center;
    wp_f32 radius;
    wp_s32 sphere, simd_enabled;
    /* Optional conservative face-axis OBB rejection after the local AABB test. */
    wp_s32 oriented;
    wp_vec3f axes[3], half;
} wp_triangle_mesh_query;
typedef struct wp_triangle_mesh_query_stats
{
    uint64_t nodes, candidates, full_scan_fallbacks, simd_batches;
    uint64_t oriented_tests, oriented_rejections;
} wp_triangle_mesh_query_stats;
typedef void (*wp_triangle_mesh_triangle_callback)( wp_u32 triangle, wp_vec3f a,
                                                   wp_vec3f b, wp_vec3f c, void *context );
/* Stateless, complete traversal. No fixed candidate capacity or shared query
 * radius. Missing acceleration falls back to valid original triangles. The
 * callback must not refit/destroy the mesh. stats, when supplied, accumulates. */
void wp_triangle_mesh_visit( const wp_triangle_mesh *mesh, const wp_triangle_mesh_query *query,
    wp_triangle_mesh_triangle_callback callback, void *context, wp_triangle_mesh_query_stats *stats );
wp_s32 wp_triangle_mesh_simd_available( void );
wp_u32 wp_triangle_mesh_query_aabb( const wp_triangle_mesh *mesh, wp_vec3f minimum,
    wp_vec3f maximum, wp_u32 *out_triangle_indices, wp_u32 capacity );

/* Borrows the vertex/index arrays; they must outlive the mesh. */
wp_triangle_mesh *wp_triangle_mesh_create( const wp_f32 *vertices, wp_u32 vertex_count,
                                           const wp_u32 *indices, wp_u32 triangle_count );
void wp_triangle_mesh_destroy( wp_triangle_mesh *mesh );

wp_u32 wp_triangle_mesh_get_vertex_count( const wp_triangle_mesh *mesh );
wp_u32 wp_triangle_mesh_get_triangle_count( const wp_triangle_mesh *mesh );
const wp_f32 *wp_triangle_mesh_get_vertices( const wp_triangle_mesh *mesh );
const wp_u32 *wp_triangle_mesh_get_indices( const wp_triangle_mesh *mesh );

wp_vec3f wp_triangle_mesh_get_aabb_min( const wp_triangle_mesh *mesh );
wp_vec3f wp_triangle_mesh_get_aabb_max( const wp_triangle_mesh *mesh );
/* Call after editing borrowed geometry. Refits unchanged index topology,
 * rebuilds when triangle validity/indices change, and
 * invalidates the owning shape's body bounds when attached to a shape. */
void wp_triangle_mesh_refit_aabb( wp_triangle_mesh *mesh );

/*
 * Finds triangle candidates whose local-space AABBs overlap a sphere.
 * Returns the total candidate count, which can exceed `capacity`. Returns
 * UINT32_MAX when the acceleration tree is unavailable so callers can safely
 * fall back to scanning every triangle.
 */
wp_u32 wp_triangle_mesh_query_sphere( const wp_triangle_mesh *mesh, wp_vec3f center, wp_f32 radius,
                                      wp_u32 *out_triangle_indices, wp_u32 capacity );

wp_s32 wp_triangle_mesh_raycast( const wp_triangle_mesh *mesh, wp_vec3f origin, wp_vec3f direction,
                                 wp_f32 max_distance, wp_f32 *out_distance, wp_vec3f *out_normal,
                                 wp_s32 *out_triangle_index );

void *wp_triangle_mesh_get_user_data( const wp_triangle_mesh *mesh );
void wp_triangle_mesh_set_user_data( wp_triangle_mesh *mesh, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PHYSICS_TRIANGLE_MESH_H */
