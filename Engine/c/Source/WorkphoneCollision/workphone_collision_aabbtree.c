#include "workphone_collision_aabbtree.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define WP_COLLISION_AABBTREE_LEAF_SIZE 4u

static WpCollisionVector3 vec3_min( WpCollisionVector3 a,
                                    WpCollisionVector3 b )
{
    const WpCollisionVector3 result = {
        a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y,
        a.z < b.z ? a.z : b.z
    };
    return result;
}

static WpCollisionVector3 vec3_max( WpCollisionVector3 a,
                                    WpCollisionVector3 b )
{
    const WpCollisionVector3 result = {
        a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y,
        a.z > b.z ? a.z : b.z
    };
    return result;
}

static WpCollisionAABB aabb_union( WpCollisionAABB a, WpCollisionAABB b )
{
    const WpCollisionAABB result = { vec3_min( a.min, b.min ),
                                     vec3_max( a.max, b.max ) };
    return result;
}

static bool aabb_intersect( WpCollisionAABB a, WpCollisionAABB b )
{
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

static WpCollisionAABB triangle_aabb( WpCollisionTriangle triangle )
{
    WpCollisionVector3 minimum = triangle.vertices[0];
    WpCollisionVector3 maximum = triangle.vertices[0];
    int vertex;
    for( vertex = 1; vertex < 3; ++vertex )
    {
        minimum = vec3_min( minimum, triangle.vertices[vertex] );
        maximum = vec3_max( maximum, triangle.vertices[vertex] );
    }
    {
        const WpCollisionAABB result = { minimum, maximum };
        return result;
    }
}

static WpCollisionVector3 triangle_centroid(
    const WpCollisionTriangle *triangle )
{
    const WpCollisionVector3 result = {
        ( triangle->vertices[0].x + triangle->vertices[1].x +
          triangle->vertices[2].x ) /
            3.0f,
        ( triangle->vertices[0].y + triangle->vertices[1].y +
          triangle->vertices[2].y ) /
            3.0f,
        ( triangle->vertices[0].z + triangle->vertices[1].z +
          triangle->vertices[2].z ) /
            3.0f
    };
    return result;
}

static float vector_component( WpCollisionVector3 vector, int axis )
{
    return axis == 0 ? vector.x : ( axis == 1 ? vector.y : vector.z );
}

static bool make_leaf( WpCollisionAABBTreeNode *node,
                       const WpCollisionTriangle *triangles, uint32_t count )
{
    node->triangles =
        (WpCollisionTriangle *)malloc( sizeof( WpCollisionTriangle ) * count );
    if( !node->triangles )
    {
        return false;
    }

    memcpy( node->triangles, triangles,
            sizeof( WpCollisionTriangle ) * count );
    node->triangleCount = count;
    node->isLeaf = true;
    node->left = NULL;
    node->right = NULL;
    return true;
}

static void destroy_node_recursive( WpCollisionAABBTreeNode *node )
{
    if( !node )
    {
        return;
    }
    destroy_node_recursive( node->left );
    destroy_node_recursive( node->right );
    free( node->triangles );
    free( node );
}

static WpCollisionAABBTreeNode *build_tree_recursive(
    WpCollisionTriangle *triangles, uint32_t count )
{
    WpCollisionAABBTreeNode *node;
    WpCollisionAABB bounds;
    WpCollisionTriangle *left_triangles;
    WpCollisionTriangle *right_triangles;
    uint32_t left_count = 0u;
    uint32_t right_count = 0u;
    uint32_t triangle;
    float dx;
    float dy;
    float dz;
    float split_position = 0.0f;
    int axis = 0;

    if( !triangles || count == 0u )
    {
        return NULL;
    }

    node =
        (WpCollisionAABBTreeNode *)calloc( 1u, sizeof( WpCollisionAABBTreeNode ) );
    if( !node )
    {
        return NULL;
    }

    bounds = triangle_aabb( triangles[0] );
    for( triangle = 1u; triangle < count; ++triangle )
    {
        bounds = aabb_union( bounds, triangle_aabb( triangles[triangle] ) );
    }
    node->boundingBox = bounds;

    if( count <= WP_COLLISION_AABBTREE_LEAF_SIZE )
    {
        if( !make_leaf( node, triangles, count ) )
        {
            free( node );
            return NULL;
        }
        return node;
    }

    dx = bounds.max.x - bounds.min.x;
    dy = bounds.max.y - bounds.min.y;
    dz = bounds.max.z - bounds.min.z;
    if( dy > dx && dy > dz )
    {
        axis = 1;
    }
    else if( dz > dx && dz > dy )
    {
        axis = 2;
    }

    for( triangle = 0u; triangle < count; ++triangle )
    {
        split_position +=
            vector_component( triangle_centroid( &triangles[triangle] ), axis );
    }
    split_position /= (float)count;

    left_triangles = (WpCollisionTriangle *)malloc(
        sizeof( WpCollisionTriangle ) * count );
    right_triangles = (WpCollisionTriangle *)malloc(
        sizeof( WpCollisionTriangle ) * count );
    if( !left_triangles || !right_triangles )
    {
        free( left_triangles );
        free( right_triangles );
        if( !make_leaf( node, triangles, count ) )
        {
            free( node );
            return NULL;
        }
        return node;
    }

    for( triangle = 0u; triangle < count; ++triangle )
    {
        const float centroid = vector_component(
            triangle_centroid( &triangles[triangle] ), axis );
        if( centroid < split_position )
        {
            left_triangles[left_count++] = triangles[triangle];
        }
        else
        {
            right_triangles[right_count++] = triangles[triangle];
        }
    }

    if( left_count == 0u || right_count == 0u )
    {
        free( left_triangles );
        free( right_triangles );
        if( !make_leaf( node, triangles, count ) )
        {
            free( node );
            return NULL;
        }
        return node;
    }

    node->left = build_tree_recursive( left_triangles, left_count );
    node->right = build_tree_recursive( right_triangles, right_count );
    free( left_triangles );
    free( right_triangles );

    if( !node->left || !node->right )
    {
        destroy_node_recursive( node->left );
        destroy_node_recursive( node->right );
        node->left = NULL;
        node->right = NULL;
        if( !make_leaf( node, triangles, count ) )
        {
            free( node );
            return NULL;
        }
        return node;
    }

    node->isLeaf = false;
    return node;
}

WpCollisionAABBTree *wp_collision_aabbtree_create(
    WpCollisionTriangle *triangles, uint32_t count )
{
    WpCollisionAABBTree *tree = (WpCollisionAABBTree *)calloc(
        1u, sizeof( WpCollisionAABBTree ) );
    if( !tree )
    {
        return NULL;
    }

    tree->totalTriangles = count;
    tree->radius = 0.0f;
    tree->root = build_tree_recursive( triangles, count );
    if( count > 0u && !tree->root )
    {
        free( tree );
        return NULL;
    }
    return tree;
}

void wp_collision_aabbtree_destroy( WpCollisionAABBTree *tree )
{
    if( !tree )
    {
        return;
    }
    destroy_node_recursive( tree->root );
    free( tree );
}

void wp_collision_aabbtree_set_radius( WpCollisionAABBTree *tree,
                                       float radius )
{
    if( tree )
    {
        if( radius != radius )
        {
            radius = 0.0f;
        }
        tree->radius = fabsf( radius );
    }
}

float wp_collision_aabbtree_get_radius( const WpCollisionAABBTree *tree )
{
    return tree ? tree->radius : 0.0f;
}

static bool sphere_aabb_intersect( WpCollisionVector3 center, float radius,
                                   WpCollisionAABB bounds )
{
    float distance_squared = 0.0f;
    float delta;

#define WP_COLLISION_SPHERE_AABB_AXIS( axis )                         \
    if( center.axis < bounds.min.axis )                               \
    {                                                                 \
        delta = bounds.min.axis - center.axis;                        \
        distance_squared += delta * delta;                            \
    }                                                                 \
    else if( center.axis > bounds.max.axis )                          \
    {                                                                 \
        delta = center.axis - bounds.max.axis;                        \
        distance_squared += delta * delta;                            \
    }

    WP_COLLISION_SPHERE_AABB_AXIS( x )
    WP_COLLISION_SPHERE_AABB_AXIS( y )
    WP_COLLISION_SPHERE_AABB_AXIS( z )

#undef WP_COLLISION_SPHERE_AABB_AXIS

    return distance_squared <= radius * radius;
}

static uint32_t sphere_query_node( const WpCollisionAABBTreeNode *node,
                                   WpCollisionVector3 center, float radius,
                                   uint32_t *out_indices, uint32_t capacity,
                                   uint32_t written_before )
{
    uint32_t count = 0u;
    if( !node ||
        !sphere_aabb_intersect( center, radius, node->boundingBox ) )
    {
        return 0u;
    }

    if( node->isLeaf )
    {
        uint32_t triangle;
        for( triangle = 0u; triangle < node->triangleCount; ++triangle )
        {
            if( sphere_aabb_intersect(
                    center, radius,
                    triangle_aabb( node->triangles[triangle] ) ) )
            {
                if( out_indices && written_before + count < capacity )
                {
                    out_indices[written_before + count] =
                        node->triangles[triangle].index;
                }
                ++count;
            }
        }
        return count;
    }

    count += sphere_query_node( node->left, center, radius, out_indices,
                                capacity, written_before + count );
    count += sphere_query_node( node->right, center, radius, out_indices,
                                capacity, written_before + count );
    return count;
}

uint32_t wp_collision_aabbtree_sphere_query(
    const WpCollisionAABBTree *tree, WpCollisionVector3 center,
    uint32_t *out_triangle_indices, uint32_t capacity )
{
    if( !tree || !tree->root )
    {
        return 0u;
    }
    return sphere_query_node( tree->root, center, tree->radius,
                              out_triangle_indices, capacity, 0u );
}

bool wp_collision_aabbtree_sphere_intersect(
    const WpCollisionAABBTree *tree, WpCollisionVector3 center )
{
    return wp_collision_aabbtree_sphere_query( tree, center, NULL, 0u ) >
           0u;
}

static bool box_intersect_node( const WpCollisionAABBTreeNode *node,
                                WpCollisionAABB box )
{
    if( !node || !aabb_intersect( node->boundingBox, box ) )
    {
        return false;
    }
    if( node->isLeaf )
    {
        return true;
    }
    return box_intersect_node( node->left, box ) ||
           box_intersect_node( node->right, box );
}

bool wp_collision_aabbtree_box_intersect(
    const WpCollisionAABBTree *tree, WpCollisionAABB box )
{
    return tree && box_intersect_node( tree->root, box );
}

static bool ray_aabb_intersect( WpCollisionVector3 origin,
                                WpCollisionVector3 direction,
                                WpCollisionAABB bounds )
{
    float minimum = -FLT_MAX;
    float maximum = FLT_MAX;

#define WP_COLLISION_RAY_AABB_AXIS( axis )                                \
    if( fabsf( direction.axis ) <= 1.0e-8f )                              \
    {                                                                      \
        if( origin.axis < bounds.min.axis ||                              \
            origin.axis > bounds.max.axis )                               \
        {                                                                  \
            return false;                                                  \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        const float inverse = 1.0f / direction.axis;                      \
        float first = ( bounds.min.axis - origin.axis ) * inverse;        \
        float second = ( bounds.max.axis - origin.axis ) * inverse;       \
        if( first > second )                                               \
        {                                                                  \
            const float temporary = first;                                \
            first = second;                                                \
            second = temporary;                                            \
        }                                                                  \
        minimum = fmaxf( minimum, first );                                 \
        maximum = fminf( maximum, second );                                \
        if( minimum > maximum )                                            \
        {                                                                  \
            return false;                                                  \
        }                                                                  \
    }

    WP_COLLISION_RAY_AABB_AXIS( x )
    WP_COLLISION_RAY_AABB_AXIS( y )
    WP_COLLISION_RAY_AABB_AXIS( z )

#undef WP_COLLISION_RAY_AABB_AXIS

    return maximum >= 0.0f;
}

static bool ray_intersect_node( const WpCollisionAABBTreeNode *node,
                                WpCollisionVector3 origin,
                                WpCollisionVector3 direction )
{
    if( !node ||
        !ray_aabb_intersect( origin, direction, node->boundingBox ) )
    {
        return false;
    }
    if( node->isLeaf )
    {
        return true;
    }
    return ray_intersect_node( node->left, origin, direction ) ||
           ray_intersect_node( node->right, origin, direction );
}

bool wp_collision_aabbtree_ray_intersect(
    const WpCollisionAABBTree *tree, WpCollisionVector3 origin,
    WpCollisionVector3 direction, void *out_hit )
{
    (void)out_hit;
    return tree &&
           ray_intersect_node( tree->root, origin, direction );
}
