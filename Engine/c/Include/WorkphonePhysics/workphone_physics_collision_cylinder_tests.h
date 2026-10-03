#ifndef WORKPHONE_PHYSICS_COLLISION_CYLINDER_TESTS_H
#define WORKPHONE_PHYSICS_COLLISION_CYLINDER_TESTS_H

#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

wp_s32 wp_collision_test_point_cylinder_y( wp_vec3f point, wp_vec3f center, wp_f32 radius,
                                           wp_f32 half_height );
wp_s32 wp_collision_test_cylinder_y_sphere( wp_vec3f cylinder_center, wp_f32 radius, wp_f32 half_height,
                                            wp_vec3f sphere_center, wp_f32 sphere_radius );

#ifdef __cplusplus
}
#endif

#endif
