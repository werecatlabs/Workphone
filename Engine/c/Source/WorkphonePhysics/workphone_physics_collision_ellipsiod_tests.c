#include "workphone_physics_collision_ellipsiod_tests.h"

wp_s32 wp_collision_test_point_ellipsoid( wp_vec3f point, wp_vec3f center, wp_vec3f radii )
{
    wp_f32 x, y, z;
    if( radii.x == 0.0f || radii.y == 0.0f || radii.z == 0.0f )
    {
        return 0;
    }
    x = ( point.x - center.x ) / radii.x;
    y = ( point.y - center.y ) / radii.y;
    z = ( point.z - center.z ) / radii.z;
    return x * x + y * y + z * z <= 1.0f;
}

wp_s32 wp_collision_test_ellipsoid_sphere( wp_vec3f ellipsoid_center, wp_vec3f radii,
                                           wp_vec3f sphere_center, wp_f32 sphere_radius )
{
    radii.x += sphere_radius;
    radii.y += sphere_radius;
    radii.z += sphere_radius;
    return wp_collision_test_point_ellipsoid( sphere_center, ellipsoid_center, radii );
}
