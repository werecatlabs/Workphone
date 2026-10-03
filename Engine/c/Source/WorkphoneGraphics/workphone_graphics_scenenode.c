/**
 * @file wp_scenenode.c
 * @brief Implementation of the C scene node API.
 */

#include "workphone_graphics_scenenode.h"
#include "workphone_graphics_object.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

#define WORKPHONE_SCENENODE_INITIAL_CAPACITY 4

typedef struct wp_scenenode
{
    wp_vec3f position;
    wp_quatf orientation;
    wp_vec3f scale;
    wp_scenenode *parent;
    wp_scenenode **children;
    wp_s32 child_count;
    wp_s32 child_capacity;
    wp_graphics_object **objects;
    wp_s32 object_count;
    wp_s32 object_capacity;
    wp_graphics_scene *creator;
    void *native;
} wp_scenenode;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_scenenode *wp_scenenode_create( void )
{
    wp_scenenode *node = (wp_scenenode *)malloc( sizeof( wp_scenenode ) );
    if( !node )
    {
        return NULL;
    }

    memset( node, 0, sizeof( wp_scenenode ) );

    node->orientation.w = 1.0f;
    node->scale.x = 1.0f;
    node->scale.y = 1.0f;
    node->scale.z = 1.0f;

    return node;
}

void wp_scenenode_destroy( wp_scenenode *node )
{
    wp_s32 i;

    if( !node )
    {
        return;
    }

    if( node->parent )
    {
        wp_scenenode_remove_child( node->parent, node );
    }
    for( i = 0; i < node->child_count; ++i )
    {
        if( node->children[i] )
        {
            node->children[i]->parent = NULL;
        }
    }
    for( i = 0; i < node->object_count; ++i )
    {
        wp_graphics_object_detach_from_parent( node->objects[i], node );
    }

    free( node->children );
    free( node->objects );
    free( node );
}

/* =========================================================================
 * Parent / child hierarchy
 * ====================================================================== */

wp_scenenode *wp_scenenode_get_parent( const wp_scenenode *node )
{
    if( !node )
    {
        return NULL;
    }

    return node->parent;
}

void wp_scenenode_set_parent( wp_scenenode *node, wp_scenenode *parent )
{
    if( !node )
    {
        return;
    }

    if( node->parent == parent )
    {
        return;
    }

    if( node->parent )
    {
        wp_scenenode_remove_child( node->parent, node );
    }

    if( parent )
    {
        wp_scenenode_add_child( parent, node );
    }
}

wp_s32 wp_scenenode_get_child_count( const wp_scenenode *node )
{
    if( !node )
    {
        return 0;
    }

    return node->child_count;
}

wp_scenenode *wp_scenenode_get_child( const wp_scenenode *node, wp_s32 index )
{
    if( !node || index < 0 || index >= node->child_count )
    {
        return NULL;
    }

    return node->children[index];
}

void wp_scenenode_add_child( wp_scenenode *node, wp_scenenode *child )
{
    wp_scenenode **new_children;
    wp_scenenode *ancestor;
    wp_s32 new_cap;

    if( !node || !child || node == child )
    {
        return;
    }

    if( child->parent == node )
    {
        return;
    }
    ancestor = node;
    while( ancestor )
    {
        if( ancestor == child )
        {
            return;
        }
        ancestor = ancestor->parent;
    }
    if( child->parent )
    {
        wp_scenenode_remove_child( child->parent, child );
    }

    if( node->child_count >= node->child_capacity )
    {
        new_cap =
            node->child_capacity == 0 ? WORKPHONE_SCENENODE_INITIAL_CAPACITY : node->child_capacity * 2;
        new_children =
            (wp_scenenode **)realloc( node->children, (wp_u32)new_cap * sizeof( wp_scenenode * ) );
        if( !new_children )
        {
            return;
        }
        node->children = new_children;
        node->child_capacity = new_cap;
    }

    node->children[node->child_count++] = child;
    child->parent = node;
}

void wp_scenenode_remove_child( wp_scenenode *node, wp_scenenode *child )
{
    wp_s32 i;

    if( !node || !child )
    {
        return;
    }

    for( i = 0; i < node->child_count; ++i )
    {
        if( node->children[i] == child )
        {
            node->children[i] = node->children[--node->child_count];
            child->parent = NULL;
            return;
        }
    }
}

/* =========================================================================
 * Transform
 * ====================================================================== */

wp_vec3f wp_scenenode_get_position( const wp_scenenode *node )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );

    if( !node )
    {
        return zero;
    }

    return node->position;
}

void wp_scenenode_set_position( wp_scenenode *node, wp_vec3f position )
{
    if( !node )
    {
        return;
    }

    node->position = position;
}

wp_quatf wp_scenenode_get_orientation( const wp_scenenode *node )
{
    wp_quatf identity;
    memset( &identity, 0, sizeof( wp_quatf ) );
    identity.w = 1.0f;

    if( !node )
    {
        return identity;
    }

    return node->orientation;
}

void wp_scenenode_set_orientation( wp_scenenode *node, wp_quatf orientation )
{
    if( !node )
    {
        return;
    }

    node->orientation = orientation;
}

wp_vec3f wp_scenenode_get_scale( const wp_scenenode *node )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );

    if( !node )
    {
        return zero;
    }

    return node->scale;
}

void wp_scenenode_set_scale( wp_scenenode *node, wp_vec3f scale )
{
    if( !node )
    {
        return;
    }

    node->scale = scale;
}

static void wp_scenenode_local_matrix( const wp_scenenode *node, wp_mat4f *matrix )
{
    wp_f32 x;
    wp_f32 y;
    wp_f32 z;
    wp_f32 w;
    wp_f32 length_sq;
    wp_f32 inverse_length;

    wp_mat4f_identity( matrix );
    if( !node )
    {
        return;
    }

    x = node->orientation.x;
    y = node->orientation.y;
    z = node->orientation.z;
    w = node->orientation.w;
    length_sq = x * x + y * y + z * z + w * w;
    if( length_sq > 0.0000001f )
    {
        inverse_length = 1.0f / (wp_f32)sqrt( (double)length_sq );
        x *= inverse_length;
        y *= inverse_length;
        z *= inverse_length;
        w *= inverse_length;
    }
    else
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        w = 1.0f;
    }

    /* T * R * S, using the column-vector convention used by wp_mat4f. */
    matrix->m[0][0] = ( 1.0f - 2.0f * ( y * y + z * z ) ) * node->scale.x;
    matrix->m[0][1] = ( 2.0f * ( x * y - z * w ) ) * node->scale.y;
    matrix->m[0][2] = ( 2.0f * ( x * z + y * w ) ) * node->scale.z;
    matrix->m[0][3] = node->position.x;
    matrix->m[1][0] = ( 2.0f * ( x * y + z * w ) ) * node->scale.x;
    matrix->m[1][1] = ( 1.0f - 2.0f * ( x * x + z * z ) ) * node->scale.y;
    matrix->m[1][2] = ( 2.0f * ( y * z - x * w ) ) * node->scale.z;
    matrix->m[1][3] = node->position.y;
    matrix->m[2][0] = ( 2.0f * ( x * z - y * w ) ) * node->scale.x;
    matrix->m[2][1] = ( 2.0f * ( y * z + x * w ) ) * node->scale.y;
    matrix->m[2][2] = ( 1.0f - 2.0f * ( x * x + y * y ) ) * node->scale.z;
    matrix->m[2][3] = node->position.z;
}

void wp_scenenode_get_local_matrix( const wp_scenenode *node, wp_mat4f *matrix )
{
    if( !matrix )
    {
        return;
    }
    wp_scenenode_local_matrix( node, matrix );
}

static void wp_scenenode_world_matrix( const wp_scenenode *node, wp_mat4f *matrix, wp_s32 depth )
{
    wp_mat4f local;
    wp_mat4f parent;

    wp_scenenode_local_matrix( node, &local );
    if( !node || !node->parent || depth >= 256 )
    {
        *matrix = local;
        return;
    }

    wp_scenenode_world_matrix( node->parent, &parent, depth + 1 );
    wp_mat4f_mul( matrix, &parent, &local );
}

void wp_scenenode_get_world_matrix( const wp_scenenode *node, wp_mat4f *matrix )
{
    if( !matrix )
    {
        return;
    }
    wp_scenenode_world_matrix( node, matrix, 0 );
}

/* =========================================================================
 * Graphics object attachment
 * ====================================================================== */

void wp_scenenode_attach_object( wp_scenenode *node, wp_graphics_object *obj )
{
    wp_graphics_object **new_objects;
    wp_scenenode *owner;
    wp_s32 new_cap;

    if( !node || !obj )
    {
        return;
    }

    owner = wp_graphics_object_get_owner( obj );
    if( owner == node )
    {
        return;
    }

    if( owner )
    {
        wp_scenenode_detach_object( owner, obj );
    }

    if( node->object_count >= node->object_capacity )
    {
        new_cap = node->object_capacity == 0 ? WORKPHONE_SCENENODE_INITIAL_CAPACITY
                                             : node->object_capacity * 2;
        new_objects = (wp_graphics_object **)realloc( node->objects,
                                                      (wp_u32)new_cap * sizeof( wp_graphics_object * ) );
        if( !new_objects )
        {
            return;
        }

        node->objects = new_objects;
        node->object_capacity = new_cap;
    }

    node->objects[node->object_count++] = obj;
    wp_graphics_object_attach_to_parent( obj, node );
}

void wp_scenenode_detach_object( wp_scenenode *node, wp_graphics_object *obj )
{
    wp_s32 i;

    if( !node || !obj )
    {
        return;
    }

    for( i = 0; i < node->object_count; ++i )
    {
        if( node->objects[i] == obj )
        {
            node->objects[i] = node->objects[--node->object_count];
            wp_graphics_object_detach_from_parent( obj, node );
            return;
        }
    }
}

wp_s32 wp_scenenode_get_object_count( const wp_scenenode *node )
{
    if( !node )
    {
        return 0;
    }

    return node->object_count;
}

wp_graphics_object *wp_scenenode_get_object( const wp_scenenode *node, wp_s32 index )
{
    if( !node || index < 0 || index >= node->object_count )
    {
        return NULL;
    }

    return node->objects[index];
}

/* =========================================================================
 * Creator scene
 * ====================================================================== */

wp_graphics_scene *wp_scenenode_get_creator( const wp_scenenode *node )
{
    if( !node )
    {
        return NULL;
    }

    return node->creator;
}

void wp_scenenode_set_creator( wp_scenenode *node, wp_graphics_scene *scene )
{
    if( !node )
    {
        return;
    }

    node->creator = scene;
}

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_scenenode_get_native( const wp_scenenode *node, void **pp_object )
{
    if( !pp_object )
    {
        return;
    }

    *pp_object = node ? node->native : NULL;
}

void wp_scenenode_set_native( wp_scenenode *node, void *native )
{
    if( !node )
    {
        return;
    }

    node->native = native;
}
