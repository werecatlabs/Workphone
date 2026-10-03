#ifndef WORKPHONE_COLLISION_AABBTREE_H
#define WORKPHONE_COLLISION_AABBTREE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WpCollisionVector3
{
    float x;
    float y;
    float z;
} WpCollisionVector3;

typedef struct WpCollisionAABB
{
    WpCollisionVector3 min;
    WpCollisionVector3 max;
} WpCollisionAABB;

typedef struct WpCollisionTriangle
{
    WpCollisionVector3 vertices[3];
    uint32_t index;
} WpCollisionTriangle;

typedef struct WpCollisionAABBTreeNode
{
    WpCollisionAABB boundingBox;
    struct WpCollisionAABBTreeNode *left;
    struct WpCollisionAABBTreeNode *right;
    WpCollisionTriangle *triangles;
    uint32_t triangleCount;
    bool isLeaf;
} WpCollisionAABBTreeNode;

typedef struct WpCollisionAABBTree
{
    WpCollisionAABBTreeNode *root;
    uint32_t totalTriangles;

    /*
     * Radius used by sphere queries. Keeping it on the tree lets callers set
     * the broad-query size once before traversing without rebuilding a query
     * AABB for every node.
     */
    float radius;
} WpCollisionAABBTree;

WpCollisionAABBTree *
wp_collision_aabbtree_create( WpCollisionTriangle *triangles, uint32_t count );
void wp_collision_aabbtree_destroy( WpCollisionAABBTree *tree );

void wp_collision_aabbtree_set_radius( WpCollisionAABBTree *tree, float radius );
float wp_collision_aabbtree_get_radius( const WpCollisionAABBTree *tree );

/*
 * Returns every triangle whose AABB overlaps the sphere centered at `center`
 * with the tree's current radius. Up to `capacity` triangle indices are
 * written to `outTriangleIndices`; the return value is the total candidate
 * count, which may be larger than the output capacity.
 */
uint32_t wp_collision_aabbtree_sphere_query(
    const WpCollisionAABBTree *tree, WpCollisionVector3 center,
    uint32_t *outTriangleIndices, uint32_t capacity );
bool wp_collision_aabbtree_sphere_intersect(
    const WpCollisionAABBTree *tree, WpCollisionVector3 center );

bool wp_collision_aabbtree_ray_intersect(
    const WpCollisionAABBTree *tree, WpCollisionVector3 origin,
    WpCollisionVector3 direction, void *outHit );
bool wp_collision_aabbtree_box_intersect(
    const WpCollisionAABBTree *tree, WpCollisionAABB box );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_COLLISION_AABBTREE_H */
