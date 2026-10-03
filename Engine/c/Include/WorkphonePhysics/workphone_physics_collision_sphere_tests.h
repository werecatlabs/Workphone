#ifndef WORKPHONE_PHYSICS_COLLISION_SPHERE_TESTS_H
#define WORKPHONE_PHYSICS_COLLISION_SPHERE_TESTS_H

#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

wp_s32 wp_collision_test_sphere_sphere( wp_vec3f center_a, wp_f32 radius_a, wp_vec3f center_b,
                                        wp_f32 radius_b, wp_f32 *out_penetration, wp_vec3f *out_normal );

#ifdef __cplusplus
}
#endif

#endif
