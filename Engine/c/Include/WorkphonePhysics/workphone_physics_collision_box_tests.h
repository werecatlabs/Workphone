#ifndef WORKPHONE_PHYSICS_COLLISION_BOX_TESTS_H
#define WORKPHONE_PHYSICS_COLLISION_BOX_TESTS_H

#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

wp_s32 wp_collision_test_aabb_aabb( wp_vec3f min_a, wp_vec3f max_a, wp_vec3f min_b, wp_vec3f max_b );
wp_s32 wp_collision_test_point_aabb( wp_vec3f point, wp_vec3f min, wp_vec3f max );

#ifdef __cplusplus
}
#endif

#endif
