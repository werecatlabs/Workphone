#include "workphone_physics_collision_box_tests.h"

wp_s32 wp_collision_test_aabb_aabb( wp_vec3f min_a, wp_vec3f max_a, wp_vec3f min_b, wp_vec3f max_b )
{
    if( max_a.x < min_b.x || min_a.x > max_b.x )
        return 0;
    if( max_a.y < min_b.y || min_a.y > max_b.y )
        return 0;
    if( max_a.z < min_b.z || min_a.z > max_b.z )
        return 0;
    return 1;
}

wp_s32 wp_collision_test_point_aabb( wp_vec3f point, wp_vec3f min, wp_vec3f max )
{
    return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y &&
           point.z >= min.z && point.z <= max.z;
}
