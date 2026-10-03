#include "workphone_physics_collision_sphere_tests.h"
#include <math.h>
#include <string.h>

wp_s32 wp_collision_test_sphere_sphere( wp_vec3f center_a, wp_f32 radius_a, wp_vec3f center_b,
                                        wp_f32 radius_b, wp_f32 *out_penetration, wp_vec3f *out_normal )
{
    wp_vec3f d;
    wp_f32 dist_sq;
    wp_f32 sum = radius_a + radius_b;
    d.x = center_b.x - center_a.x;
    d.y = center_b.y - center_a.y;
    d.z = center_b.z - center_a.z;
    dist_sq = d.x * d.x + d.y * d.y + d.z * d.z;
    if( dist_sq > sum * sum )
    {
        return 0;
    }
    if( out_penetration || out_normal )
    {
        wp_f32 dist = sqrtf( dist_sq );
        if( out_penetration )
        {
            *out_penetration = sum - dist;
        }
        if( out_normal )
        {
            memset( out_normal, 0, sizeof( *out_normal ) );
            if( dist > 1.0e-7f )
            {
                out_normal->x = d.x / dist;
                out_normal->y = d.y / dist;
                out_normal->z = d.z / dist;
            }
            else
            {
                out_normal->y = 1.0f;
            }
        }
    }
    return 1;
}
