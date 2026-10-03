#ifndef WORKPHONE_PHYSICS_COLLISION_MESH_TESTS_H
#define WORKPHONE_PHYSICS_COLLISION_MESH_TESTS_H

#include "workphone_physics_triangle_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

wp_s32 wp_collision_test_mesh_ray( const wp_triangle_mesh *mesh, wp_vec3f origin, wp_vec3f direction,
                                   wp_f32 max_distance, wp_f32 *out_distance, wp_vec3f *out_normal,
                                   wp_s32 *out_triangle_index );
wp_s32 wp_collision_test_mesh_aabb_overlap( const wp_triangle_mesh *mesh, wp_vec3f aabb_min,
                                            wp_vec3f aabb_max );

#ifdef __cplusplus
}
#endif

#endif
