#ifndef WORKPHONE_PHYSICS_COLLISION_CONVEX_TESTS_H
#define WORKPHONE_PHYSICS_COLLISION_CONVEX_TESTS_H

#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

wp_s32 wp_collision_convex_support_point( const wp_vec3f *points, wp_s32 point_count, wp_vec3f direction,
                                          wp_vec3f *out_point );
wp_s32 wp_collision_test_point_convex_aabb( wp_vec3f point, const wp_vec3f *points, wp_s32 point_count );

#ifdef __cplusplus
}
#endif

#endif
