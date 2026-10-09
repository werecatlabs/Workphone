/**
 * @file workphone_physics_triangle_mesh.c
 * @brief Triangle mesh helper implementation.
 */

#include "workphone_physics_triangle_mesh.h"
#include "workphone_collision_aabbtree.h"
#include "workphone_physics_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WP_TRIANGLE_MESH_BVH_LEAF_SIZE 8u

typedef struct wp_triangle_mesh_bvh_node
{
    wp_vec3f aabb_min;
    wp_vec3f aabb_max;
    wp_u32 first_triangle;
    wp_u32 triangle_count;
    wp_u32 left_child;
    wp_u32 right_child;
} wp_triangle_mesh_bvh_node;

typedef struct wp_triangle_mesh
{
    const wp_f32 *vertices;
    wp_u32 vertex_count;
    const wp_u32 *indices;
    wp_u32 triangle_count;
    wp_vec3f aabb_min;
    wp_vec3f aabb_max;
    wp_u32 *bvh_triangles;
    wp_triangle_mesh_bvh_node *bvh_nodes;
    wp_u32 bvh_triangle_count;
    wp_u32 bvh_node_count;
    wp_u32 bvh_node_capacity;
    WpCollisionAABBTree *aabb_tree;
    void *user_data;
    void ( *refit_callback )( void * );
    void *refit_context;
} wp_triangle_mesh;

static wp_vec3f zero3( void )
{
    wp_vec3f v;
    memset( &v, 0, sizeof( v ) );
    return v;
}

static wp_vec3f v3( const wp_f32 *vertices, wp_u32 index )
{
    wp_vec3f v;
    v.x = vertices[index * 3u + 0u];
    v.y = vertices[index * 3u + 1u];
    v.z = vertices[index * 3u + 2u];
    return v;
}

static wp_vec3f sub3( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

static wp_vec3f cross3( wp_vec3f a, wp_vec3f b )
{
    wp_vec3f r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

static wp_f32 dot3( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static wp_f32 component3( wp_vec3f v, wp_u32 axis )
{
    return axis == 0u ? v.x : ( axis == 1u ? v.y : v.z );
}

static wp_vec3f normalize3( wp_vec3f v )
{
    wp_f32 len = sqrtf( dot3( v, v ) );
    if( len <= 1.0e-7f )
    {
        return zero3();
    }
    v.x /= len;
    v.y /= len;
    v.z /= len;
    return v;
}

static wp_s32 triangle_vertices( const wp_triangle_mesh *mesh, wp_u32 triangle, wp_vec3f *a, wp_vec3f *b,
                                 wp_vec3f *c )
{
    wp_u32 ia;
    wp_u32 ib;
    wp_u32 ic;

    if( !mesh || !mesh->vertices || !mesh->indices || triangle >= mesh->triangle_count || !a || !b ||
        !c )
    {
        return 0;
    }

    ia = mesh->indices[triangle * 3u];
    ib = mesh->indices[triangle * 3u + 1u];
    ic = mesh->indices[triangle * 3u + 2u];
    if( ia >= mesh->vertex_count || ib >= mesh->vertex_count || ic >= mesh->vertex_count )
    {
        return 0;
    }

    *a = v3( mesh->vertices, ia );
    *b = v3( mesh->vertices, ib );
    *c = v3( mesh->vertices, ic );
    return 1;
}

static wp_vec3f triangle_centroid( const wp_triangle_mesh *mesh, wp_u32 triangle )
{
    wp_vec3f a;
    wp_vec3f b;
    wp_vec3f c;
    wp_vec3f centroid = zero3();
    if( triangle_vertices( mesh, triangle, &a, &b, &c ) )
    {
        centroid.x = ( a.x + b.x + c.x ) / 3.0f;
        centroid.y = ( a.y + b.y + c.y ) / 3.0f;
        centroid.z = ( a.z + b.z + c.z ) / 3.0f;
    }
    return centroid;
}

static void bounds_include_point( wp_vec3f *minimum, wp_vec3f *maximum, wp_vec3f point )
{
    if( point.x < minimum->x )
        minimum->x = point.x;
    if( point.y < minimum->y )
        minimum->y = point.y;
    if( point.z < minimum->z )
        minimum->z = point.z;
    if( point.x > maximum->x )
        maximum->x = point.x;
    if( point.y > maximum->y )
        maximum->y = point.y;
    if( point.z > maximum->z )
        maximum->z = point.z;
}

static void swap_triangles( wp_u32 *a, wp_u32 *b )
{
    wp_u32 temporary = *a;
    *a = *b;
    *b = temporary;
}

static wp_u32 partition_triangles( wp_triangle_mesh *mesh, wp_u32 first, wp_u32 count, wp_u32 axis,
                                   wp_f32 split )
{
    wp_u32 read;
    wp_u32 write = first;
    for( read = first; read < first + count; ++read )
    {
        const wp_u32 triangle = mesh->bvh_triangles[read];
        if( component3( triangle_centroid( mesh, triangle ), axis ) < split )
        {
            swap_triangles( &mesh->bvh_triangles[write], &mesh->bvh_triangles[read] );
            ++write;
        }
    }
    return write - first;
}

static wp_u32 build_bvh_node( wp_triangle_mesh *mesh, wp_u32 first, wp_u32 count )
{
    wp_u32 node_index;
    wp_triangle_mesh_bvh_node *node;
    wp_vec3f centroid_min;
    wp_vec3f centroid_max;
    wp_u32 i;
    wp_u32 axis;
    wp_u32 left_count;
    wp_f32 split;

    if( !mesh || count == 0u || mesh->bvh_node_count >= mesh->bvh_node_capacity )
    {
        return UINT32_MAX;
    }

    node_index = mesh->bvh_node_count++;
    node = &mesh->bvh_nodes[node_index];
    memset( node, 0, sizeof( *node ) );
    node->first_triangle = first;
    node->triangle_count = count;
    node->left_child = UINT32_MAX;
    node->right_child = UINT32_MAX;

    {
        wp_vec3f a;
        wp_vec3f b;
        wp_vec3f c;
        const wp_u32 triangle = mesh->bvh_triangles[first];
        triangle_vertices( mesh, triangle, &a, &b, &c );
        node->aabb_min = node->aabb_max = a;
        bounds_include_point( &node->aabb_min, &node->aabb_max, b );
        bounds_include_point( &node->aabb_min, &node->aabb_max, c );
        centroid_min = centroid_max = triangle_centroid( mesh, triangle );
    }

    for( i = 1u; i < count; ++i )
    {
        wp_vec3f a;
        wp_vec3f b;
        wp_vec3f c;
        wp_vec3f centroid;
        const wp_u32 triangle = mesh->bvh_triangles[first + i];
        triangle_vertices( mesh, triangle, &a, &b, &c );
        bounds_include_point( &node->aabb_min, &node->aabb_max, a );
        bounds_include_point( &node->aabb_min, &node->aabb_max, b );
        bounds_include_point( &node->aabb_min, &node->aabb_max, c );
        centroid = triangle_centroid( mesh, triangle );
        bounds_include_point( &centroid_min, &centroid_max, centroid );
    }

    if( count <= WP_TRIANGLE_MESH_BVH_LEAF_SIZE )
    {
        return node_index;
    }

    axis = 0u;
    if( centroid_max.y - centroid_min.y > centroid_max.x - centroid_min.x )
    {
        axis = 1u;
    }
    if( component3( centroid_max, 2u ) - component3( centroid_min, 2u ) >
        component3( centroid_max, axis ) - component3( centroid_min, axis ) )
    {
        axis = 2u;
    }

    split = ( component3( centroid_min, axis ) + component3( centroid_max, axis ) ) * 0.5f;
    left_count = partition_triangles( mesh, first, count, axis, split );
    if( left_count < count / 8u || left_count > count - count / 8u )
    {
        left_count = count / 2u;
    }

    node->triangle_count = 0u;
    node->left_child = build_bvh_node( mesh, first, left_count );
    node->right_child = build_bvh_node( mesh, first + left_count, count - left_count );
    return node_index;
}

static void destroy_bvh( wp_triangle_mesh *mesh )
{
    if( !mesh )
    {
        return;
    }
    free( mesh->bvh_triangles );
    free( mesh->bvh_nodes );
    mesh->bvh_triangles = NULL;
    mesh->bvh_nodes = NULL;
    mesh->bvh_triangle_count = 0u;
    mesh->bvh_node_count = 0u;
    mesh->bvh_node_capacity = 0u;
}

static void rebuild_bvh( wp_triangle_mesh *mesh )
{
    wp_u32 triangle;

    destroy_bvh( mesh );
    if( !mesh || !mesh->vertices || !mesh->indices || mesh->triangle_count == 0u ||
        mesh->triangle_count > UINT32_MAX / 2u )
    {
        return;
    }

    mesh->bvh_triangles = (wp_u32 *)malloc( sizeof( wp_u32 ) * mesh->triangle_count );
    mesh->bvh_node_capacity = mesh->triangle_count * 2u;
    mesh->bvh_nodes = (wp_triangle_mesh_bvh_node *)malloc( sizeof( wp_triangle_mesh_bvh_node ) *
                                                           mesh->bvh_node_capacity );
    if( !mesh->bvh_triangles || !mesh->bvh_nodes )
    {
        destroy_bvh( mesh );
        return;
    }

    for( triangle = 0u; triangle < mesh->triangle_count; ++triangle )
    {
        wp_vec3f a;
        wp_vec3f b;
        wp_vec3f c;
        if( triangle_vertices( mesh, triangle, &a, &b, &c ) )
        {
            mesh->bvh_triangles[mesh->bvh_triangle_count++] = triangle;
        }
    }

    if( mesh->bvh_triangle_count == 0u ||
        build_bvh_node( mesh, 0u, mesh->bvh_triangle_count ) == UINT32_MAX )
    {
        destroy_bvh( mesh );
    }
}

static void destroy_aabb_tree( wp_triangle_mesh *mesh )
{
    if( !mesh )
    {
        return;
    }
    wp_collision_aabbtree_destroy( mesh->aabb_tree );
    mesh->aabb_tree = NULL;
}

static WpCollisionVector3 collision_vector( wp_vec3f vector )
{
    WpCollisionVector3 result;
    result.x = vector.x;
    result.y = vector.y;
    result.z = vector.z;
    return result;
}

static void rebuild_aabb_tree( wp_triangle_mesh *mesh )
{
    WpCollisionTriangle *triangles;
    wp_u32 valid_count = 0u;
    wp_u32 triangle;

    destroy_aabb_tree( mesh );
    if( !mesh || !mesh->vertices || !mesh->indices || mesh->triangle_count == 0u )
    {
        return;
    }

    triangles = (WpCollisionTriangle *)malloc( sizeof( WpCollisionTriangle ) * mesh->triangle_count );
    if( !triangles )
    {
        return;
    }

    for( triangle = 0u; triangle < mesh->triangle_count; ++triangle )
    {
        wp_vec3f a;
        wp_vec3f b;
        wp_vec3f c;
        if( !triangle_vertices( mesh, triangle, &a, &b, &c ) )
        {
            continue;
        }

        triangles[valid_count].vertices[0] = collision_vector( a );
        triangles[valid_count].vertices[1] = collision_vector( b );
        triangles[valid_count].vertices[2] = collision_vector( c );
        triangles[valid_count].index = triangle;
        ++valid_count;
    }

    if( valid_count > 0u )
    {
        mesh->aabb_tree = wp_collision_aabbtree_create( triangles, valid_count );
    }
    free( triangles );
}

wp_triangle_mesh *wp_triangle_mesh_create( const wp_f32 *vertices, wp_u32 vertex_count,
                                           const wp_u32 *indices, wp_u32 triangle_count )
{
    wp_triangle_mesh *mesh = (wp_triangle_mesh *)malloc( sizeof( wp_triangle_mesh ) );
    if( !mesh )
    {
        return NULL;
    }
    memset( mesh, 0, sizeof( wp_triangle_mesh ) );
    mesh->vertices = vertices;
    mesh->vertex_count = vertex_count;
    mesh->indices = indices;
    mesh->triangle_count = triangle_count;
    wp_triangle_mesh_refit_aabb( mesh );
    return mesh;
}

void wp_triangle_mesh_destroy( wp_triangle_mesh *mesh )
{
    destroy_bvh( mesh );
    destroy_aabb_tree( mesh );
    free( mesh );
}

wp_u32 wp_triangle_mesh_get_vertex_count( const wp_triangle_mesh *mesh )
{
    return mesh ? mesh->vertex_count : 0u;
}

wp_u32 wp_triangle_mesh_get_triangle_count( const wp_triangle_mesh *mesh )
{
    return mesh ? mesh->triangle_count : 0u;
}

const wp_f32 *wp_triangle_mesh_get_vertices( const wp_triangle_mesh *mesh )
{
    return mesh ? mesh->vertices : NULL;
}

const wp_u32 *wp_triangle_mesh_get_indices( const wp_triangle_mesh *mesh )
{
    return mesh ? mesh->indices : NULL;
}

wp_vec3f wp_triangle_mesh_get_aabb_min( const wp_triangle_mesh *mesh )
{
    return mesh ? mesh->aabb_min : zero3();
}

wp_vec3f wp_triangle_mesh_get_aabb_max( const wp_triangle_mesh *mesh )
{
    return mesh ? mesh->aabb_max : zero3();
}

void wp_triangle_mesh_refit_aabb( wp_triangle_mesh *mesh )
{
    wp_u32 i;
    if( mesh && mesh->refit_callback )
    {
        mesh->refit_callback( mesh->refit_context );
    }
    if( !mesh || !mesh->vertices || mesh->vertex_count == 0u )
    {
        destroy_bvh( mesh );
        destroy_aabb_tree( mesh );
        return;
    }
    mesh->aabb_min = v3( mesh->vertices, 0u );
    mesh->aabb_max = mesh->aabb_min;
    for( i = 1u; i < mesh->vertex_count; ++i )
    {
        wp_vec3f p = v3( mesh->vertices, i );
        if( p.x < mesh->aabb_min.x )
            mesh->aabb_min.x = p.x;
        if( p.y < mesh->aabb_min.y )
            mesh->aabb_min.y = p.y;
        if( p.z < mesh->aabb_min.z )
            mesh->aabb_min.z = p.z;
        if( p.x > mesh->aabb_max.x )
            mesh->aabb_max.x = p.x;
        if( p.y > mesh->aabb_max.y )
            mesh->aabb_max.y = p.y;
        if( p.z > mesh->aabb_max.z )
            mesh->aabb_max.z = p.z;
    }
    rebuild_bvh( mesh );
    rebuild_aabb_tree( mesh );
}

void wp_triangle_mesh_set_refit_callback( wp_triangle_mesh *mesh,
                                         void ( *callback )( void * ), void *context )
{
    if( !mesh )
    {
        return;
    }
    mesh->refit_callback = callback;
    mesh->refit_context = context;
}

wp_u32 wp_triangle_mesh_query_sphere( const wp_triangle_mesh *mesh, wp_vec3f center, wp_f32 radius,
                                      wp_u32 *out_triangle_indices, wp_u32 capacity )
{
    if( !mesh || !mesh->aabb_tree )
    {
        return UINT32_MAX;
    }

    wp_collision_aabbtree_set_radius( mesh->aabb_tree, radius );
    return wp_collision_aabbtree_sphere_query( mesh->aabb_tree, collision_vector( center ),
                                               (uint32_t *)out_triangle_indices, (uint32_t)capacity );
}

static wp_s32 ray_aabb( wp_vec3f origin, wp_vec3f direction, wp_f32 max_distance, wp_vec3f minimum,
                        wp_vec3f maximum, wp_f32 *out_entry )
{
    wp_f32 entry = 0.0f;
    wp_f32 exit = max_distance;

#define WP_MESH_RAY_AABB_AXIS( axis ) \
    if( fabsf( direction.axis ) <= 1.0e-8f ) \
    { \
        if( origin.axis < minimum.axis || origin.axis > maximum.axis ) \
            return 0; \
    } \
    else \
    { \
        wp_f32 inverse = 1.0f / direction.axis; \
        wp_f32 first = ( minimum.axis - origin.axis ) * inverse; \
        wp_f32 second = ( maximum.axis - origin.axis ) * inverse; \
        if( first > second ) \
        { \
            wp_f32 temporary = first; \
            first = second; \
            second = temporary; \
        } \
        if( first > entry ) \
            entry = first; \
        if( second < exit ) \
            exit = second; \
        if( entry > exit ) \
            return 0; \
    }

    WP_MESH_RAY_AABB_AXIS( x )
    WP_MESH_RAY_AABB_AXIS( y )
    WP_MESH_RAY_AABB_AXIS( z )

#undef WP_MESH_RAY_AABB_AXIS

    if( out_entry )
    {
        *out_entry = entry;
    }
    return exit >= 0.0f;
}

static wp_s32 ray_triangle_distance( wp_vec3f origin, wp_vec3f direction, wp_f32 max_distance,
                                     wp_vec3f a, wp_vec3f b, wp_vec3f c, wp_f32 *out_distance )
{
    const wp_f32 epsilon = 1.0e-7f;
    const wp_vec3f edge1 = sub3( b, a );
    const wp_vec3f edge2 = sub3( c, a );
    const wp_vec3f p = cross3( direction, edge2 );
    const wp_f32 determinant = dot3( edge1, p );
    wp_vec3f from_a;
    wp_vec3f q;
    wp_f32 inverse;
    wp_f32 u;
    wp_f32 v;
    wp_f32 distance;

    if( determinant > -epsilon && determinant < epsilon )
    {
        return 0;
    }

    inverse = 1.0f / determinant;
    from_a = sub3( origin, a );
    u = dot3( from_a, p ) * inverse;
    if( u < 0.0f || u > 1.0f )
    {
        return 0;
    }

    q = cross3( from_a, edge1 );
    v = dot3( direction, q ) * inverse;
    if( v < 0.0f || u + v > 1.0f )
    {
        return 0;
    }

    distance = dot3( edge2, q ) * inverse;
    if( distance < 0.0f || distance > max_distance )
    {
        return 0;
    }

    if( out_distance )
    {
        *out_distance = distance;
    }
    return 1;
}

static void raycast_bvh_node( const wp_triangle_mesh *mesh, wp_u32 node_index, wp_vec3f origin,
                              wp_vec3f direction, wp_f32 *best_distance, wp_s32 *best_triangle )
{
    const wp_triangle_mesh_bvh_node *node;
    wp_f32 node_entry;

    if( !mesh || !best_distance || !best_triangle || node_index >= mesh->bvh_node_count )
    {
        return;
    }

    node = &mesh->bvh_nodes[node_index];
    if( !ray_aabb( origin, direction, *best_distance, node->aabb_min, node->aabb_max, &node_entry ) )
    {
        return;
    }

    if( node->triangle_count > 0u )
    {
        wp_u32 i;
        for( i = 0u; i < node->triangle_count; ++i )
        {
            const wp_u32 triangle = mesh->bvh_triangles[node->first_triangle + i];
            wp_vec3f a;
            wp_vec3f b;
            wp_vec3f c;
            wp_f32 distance;
            if( triangle_vertices( mesh, triangle, &a, &b, &c ) &&
                ray_triangle_distance( origin, direction, *best_distance, a, b, c, &distance ) )
            {
                *best_distance = distance;
                *best_triangle = (wp_s32)triangle;
            }
        }
        return;
    }

    {
        const wp_triangle_mesh_bvh_node *left =
            node->left_child < mesh->bvh_node_count ? &mesh->bvh_nodes[node->left_child] : NULL;
        const wp_triangle_mesh_bvh_node *right =
            node->right_child < mesh->bvh_node_count ? &mesh->bvh_nodes[node->right_child] : NULL;
        wp_f32 left_entry = 0.0f;
        wp_f32 right_entry = 0.0f;
        const wp_s32 hit_left = left && ray_aabb( origin, direction, *best_distance, left->aabb_min,
                                                  left->aabb_max, &left_entry );
        const wp_s32 hit_right = right && ray_aabb( origin, direction, *best_distance, right->aabb_min,
                                                    right->aabb_max, &right_entry );

        if( hit_left && hit_right )
        {
            if( left_entry <= right_entry )
            {
                raycast_bvh_node( mesh, node->left_child, origin, direction, best_distance,
                                  best_triangle );
                raycast_bvh_node( mesh, node->right_child, origin, direction, best_distance,
                                  best_triangle );
            }
            else
            {
                raycast_bvh_node( mesh, node->right_child, origin, direction, best_distance,
                                  best_triangle );
                raycast_bvh_node( mesh, node->left_child, origin, direction, best_distance,
                                  best_triangle );
            }
        }
        else if( hit_left )
        {
            raycast_bvh_node( mesh, node->left_child, origin, direction, best_distance, best_triangle );
        }
        else if( hit_right )
        {
            raycast_bvh_node( mesh, node->right_child, origin, direction, best_distance, best_triangle );
        }
    }
}

wp_s32 wp_triangle_mesh_raycast( const wp_triangle_mesh *mesh, wp_vec3f origin, wp_vec3f direction,
                                 wp_f32 max_distance, wp_f32 *out_distance, wp_vec3f *out_normal,
                                 wp_s32 *out_triangle_index )
{
    wp_f32 best = max_distance;
    wp_s32 best_index = -1;

    if( !mesh || !mesh->vertices || !mesh->indices || max_distance < 0.0f )
    {
        return 0;
    }

    if( mesh->bvh_nodes && mesh->bvh_node_count > 0u )
    {
        raycast_bvh_node( mesh, 0u, origin, direction, &best, &best_index );
    }
    else
    {
        wp_u32 triangle;
        for( triangle = 0u; triangle < mesh->triangle_count; ++triangle )
        {
            wp_vec3f a;
            wp_vec3f b;
            wp_vec3f c;
            wp_f32 distance;
            if( triangle_vertices( mesh, triangle, &a, &b, &c ) &&
                ray_triangle_distance( origin, direction, best, a, b, c, &distance ) )
            {
                best = distance;
                best_index = (wp_s32)triangle;
            }
        }
    }

    if( best_index >= 0 )
    {
        wp_vec3f a;
        wp_vec3f b;
        wp_vec3f c;
        wp_vec3f normal = zero3();
        if( triangle_vertices( mesh, (wp_u32)best_index, &a, &b, &c ) )
        {
            normal = normalize3( cross3( sub3( b, a ), sub3( c, a ) ) );
        }
        if( out_distance )
            *out_distance = best;
        if( out_normal )
            *out_normal = normal;
        if( out_triangle_index )
            *out_triangle_index = best_index;
        return 1;
    }
    return 0;
}

void *wp_triangle_mesh_get_user_data( const wp_triangle_mesh *mesh )
{
    return mesh ? mesh->user_data : NULL;
}

void wp_triangle_mesh_set_user_data( wp_triangle_mesh *mesh, void *user_data )
{
    if( mesh )
    {
        mesh->user_data = user_data;
    }
}
