#include "workphone_physics_collision_convex_tests.h"

static wp_f32 dot3( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

wp_s32 wp_collision_convex_support_point( const wp_vec3f *points, wp_s32 point_count, wp_vec3f direction,
                                          wp_vec3f *out_point )
{
    wp_s32 i;
    wp_s32 best = 0;
    wp_f32 best_dot;
    if( !points || point_count <= 0 || !out_point )
    {
        return 0;
    }
    best_dot = dot3( points[0], direction );
    for( i = 1; i < point_count; ++i )
    {
        wp_f32 d = dot3( points[i], direction );
        if( d > best_dot )
        {
            best_dot = d;
            best = i;
        }
    }
    *out_point = points[best];
    return 1;
}

wp_s32 wp_collision_test_point_convex_aabb( wp_vec3f point, const wp_vec3f *points, wp_s32 point_count )
{
    wp_s32 i;
    wp_vec3f min;
    wp_vec3f max;
    if( !points || point_count <= 0 )
    {
        return 0;
    }
    min = points[0];
    max = points[0];
    for( i = 1; i < point_count; ++i )
    {
        if( points[i].x < min.x )
            min.x = points[i].x;
        if( points[i].y < min.y )
            min.y = points[i].y;
        if( points[i].z < min.z )
            min.z = points[i].z;
        if( points[i].x > max.x )
            max.x = points[i].x;
        if( points[i].y > max.y )
            max.y = points[i].y;
        if( points[i].z > max.z )
            max.z = points[i].z;
    }
    return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y &&
           point.z >= min.z && point.z <= max.z;
}
