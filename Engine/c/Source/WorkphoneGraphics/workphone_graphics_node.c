/**
 * @file wp_node.c
 * @brief Implementation of the C node API.
 */

#include "workphone_graphics_node.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_node
{
    wp_vec3f position;
    wp_quatf orientation;
    wp_vec3f scale;
    wp_node *parent;
    void *native;
} wp_node;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_node *wp_node_create( void )
{
    wp_node *node = (wp_node *)malloc( sizeof( wp_node ) );
    if( !node )
    {
        return NULL;
    }

    memset( node, 0, sizeof( wp_node ) );

    node->orientation.w = 1.0f;
    node->scale.x = 1.0f;
    node->scale.y = 1.0f;
    node->scale.z = 1.0f;

    return node;
}

void wp_node_destroy( wp_node *node )
{
    if( !node )
    {
        return;
    }

    free( node );
}

/* =========================================================================
 * Parent
 * ====================================================================== */

wp_node *wp_node_get_parent( const wp_node *node )
{
    if( !node )
    {
        return NULL;
    }

    return node->parent;
}

void wp_node_set_parent( wp_node *node, wp_node *parent )
{
    if( !node )
    {
        return;
    }

    node->parent = parent;
}

/* =========================================================================
 * Position
 * ====================================================================== */

wp_vec3f wp_node_get_position( const wp_node *node )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );

    if( !node )
    {
        return zero;
    }

    return node->position;
}

void wp_node_set_position( wp_node *node, wp_vec3f position )
{
    if( !node )
    {
        return;
    }

    node->position = position;
}

/* =========================================================================
 * Orientation
 * ====================================================================== */

wp_quatf wp_node_get_orientation( const wp_node *node )
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

void wp_node_set_orientation( wp_node *node, wp_quatf orientation )
{
    if( !node )
    {
        return;
    }

    node->orientation = orientation;
}

/* =========================================================================
 * Scale
 * ====================================================================== */

wp_vec3f wp_node_get_scale( const wp_node *node )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );

    if( !node )
    {
        return zero;
    }

    return node->scale;
}

void wp_node_set_scale( wp_node *node, wp_vec3f scale )
{
    if( !node )
    {
        return;
    }

    node->scale = scale;
}

/* =========================================================================
 * Native object access
 * ====================================================================== */

void wp_node_get_native( const wp_node *node, void **pp_object )
{
    if( !pp_object )
    {
        return;
    }

    *pp_object = node ? node->native : NULL;
}

void wp_node_set_native( wp_node *node, void *native )
{
    if( !node )
    {
        return;
    }

    node->native = native;
}
