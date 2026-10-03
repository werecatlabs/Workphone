#ifndef WORKPHONE_PHYSICS_COLLISION_ELLIPSIOD_TESTS_H
#define WORKPHONE_PHYSICS_COLLISION_ELLIPSIOD_TESTS_H

#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

wp_s32 wp_collision_test_point_ellipsoid( wp_vec3f point, wp_vec3f center, wp_vec3f radii );
wp_s32 wp_collision_test_ellipsoid_sphere( wp_vec3f ellipsoid_center, wp_vec3f radii,
                                           wp_vec3f sphere_center, wp_f32 sphere_radius );

#ifdef __cplusplus
}
#endif

#endif
