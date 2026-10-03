#include "workphone_physics_collision_cylinder_tests.h"

wp_s32 wp_collision_test_point_cylinder_y( wp_vec3f point, wp_vec3f center, wp_f32 radius,
                                           wp_f32 half_height )
{
    wp_f32 dx = point.x - center.x;
    wp_f32 dz = point.z - center.z;
    wp_f32 dy = point.y - center.y;
    return dy >= -half_height && dy <= half_height && dx * dx + dz * dz <= radius * radius;
}

wp_s32 wp_collision_test_cylinder_y_sphere( wp_vec3f cylinder_center, wp_f32 radius, wp_f32 half_height,
                                            wp_vec3f sphere_center, wp_f32 sphere_radius )
{
    wp_f32 dx = sphere_center.x - cylinder_center.x;
    wp_f32 dz = sphere_center.z - cylinder_center.z;
    wp_f32 dy = sphere_center.y - cylinder_center.y;
    wp_f32 closest_y = dy;
    wp_f32 combined = radius + sphere_radius;
    if( closest_y < -half_height )
        closest_y = -half_height;
    if( closest_y > half_height )
        closest_y = half_height;
    dy -= closest_y;
    return dx * dx + dy * dy + dz * dz <= combined * combined;
}
