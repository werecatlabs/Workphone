#include "workphone_physics_collision_mesh_tests .h"

wp_s32 wp_collision_test_mesh_ray( const wp_triangle_mesh *mesh, wp_vec3f origin, wp_vec3f direction,
                                   wp_f32 max_distance, wp_f32 *out_distance, wp_vec3f *out_normal,
                                   wp_s32 *out_triangle_index )
{
    return wp_triangle_mesh_raycast( mesh, origin, direction, max_distance, out_distance, out_normal,
                                     out_triangle_index );
}

wp_s32 wp_collision_test_mesh_aabb_overlap( const wp_triangle_mesh *mesh, wp_vec3f aabb_min,
                                            wp_vec3f aabb_max )
{
    wp_vec3f min;
    wp_vec3f max;
    if( !mesh )
    {
        return 0;
    }
    min = wp_triangle_mesh_get_aabb_min( mesh );
    max = wp_triangle_mesh_get_aabb_max( mesh );
    if( max.x < aabb_min.x || min.x > aabb_max.x )
        return 0;
    if( max.y < aabb_min.y || min.y > aabb_max.y )
        return 0;
    if( max.z < aabb_min.z || min.z > aabb_max.z )
        return 0;
    return 1;
}
