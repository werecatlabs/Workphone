/**
 * @file workphone_game_sub_component.c
 * @brief Implementation of the C game sub-component API.
 */

#include "workphone_game_sub_component.h"

#include <string.h>

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_game_sub_component_init( wp_game_sub_component *sub )
{
    memset( sub, 0, sizeof( *sub ) );
    sub->name[0] = '\0';
    sub->type = 0;
    sub->parent_component = NULL;
    sub->parent = NULL;
    sub->num_children = 0;
    sub->user_data = NULL;
}

void wp_game_sub_component_destroy( wp_game_sub_component *sub )
{
    wp_u32 i;
    for( i = 0; i < sub->num_children; ++i )
        sub->children[i] = NULL;
    sub->num_children = 0;

    sub->parent_component = NULL;
    sub->parent = NULL;
    sub->user_data = NULL;
}

/* =========================================================================
 * Name
 * ====================================================================== */

void wp_game_sub_component_set_name( wp_game_sub_component *sub, const wp_c8 *name )
{
    if( name )
    {
        strncpy( sub->name, name, WP_SUB_COMPONENT_MAX_NAME - 1 );
        sub->name[WP_SUB_COMPONENT_MAX_NAME - 1] = '\0';
    }
    else
    {
        sub->name[0] = '\0';
    }
}

const wp_c8 *wp_game_sub_component_get_name( const wp_game_sub_component *sub )
{
    return sub->name;
}

/* =========================================================================
 * Type
 * ====================================================================== */

wp_u32 wp_game_sub_component_get_type( const wp_game_sub_component *sub )
{
    return sub->type;
}

void wp_game_sub_component_set_type( wp_game_sub_component *sub, wp_u32 type )
{
    sub->type = type;
}

/* =========================================================================
 * Parent component
 * ====================================================================== */

struct wp_game_component *wp_game_sub_component_get_parent_component( const wp_game_sub_component *sub )
{
    return sub->parent_component;
}

void wp_game_sub_component_set_parent_component( wp_game_sub_component *sub,
                                                 struct wp_game_component *parent_component )
{
    sub->parent_component = parent_component;
}

/* =========================================================================
 * Parent sub-component
 * ====================================================================== */

wp_game_sub_component *wp_game_sub_component_get_parent( const wp_game_sub_component *sub )
{
    return sub->parent;
}

void wp_game_sub_component_set_parent( wp_game_sub_component *sub, wp_game_sub_component *parent )
{
    sub->parent = parent;
}

/* =========================================================================
 * Children
 * ====================================================================== */

wp_s32 wp_game_sub_component_add_child( wp_game_sub_component *sub, wp_game_sub_component *child )
{
    wp_u32 i;
    if( !child || sub->num_children >= WP_SUB_COMPONENT_MAX_CHILDREN )
        return 0;

    for( i = 0; i < sub->num_children; ++i )
    {
        if( sub->children[i] == child )
            return 0;
    }

    child->parent = sub;
    sub->children[sub->num_children++] = child;
    return 1;
}

wp_s32 wp_game_sub_component_remove_child( wp_game_sub_component *sub, wp_game_sub_component *child )
{
    wp_u32 i;
    if( !child )
        return 0;

    for( i = 0; i < sub->num_children; ++i )
    {
        if( sub->children[i] == child )
        {
            child->parent = NULL;
            sub->num_children--;
            for( ; i < sub->num_children; ++i )
                sub->children[i] = sub->children[i + 1];
            sub->children[sub->num_children] = NULL;
            return 1;
        }
    }
    return 0;
}

wp_game_sub_component *wp_game_sub_component_get_child( const wp_game_sub_component *sub, wp_u32 index )
{
    if( index >= sub->num_children )
        return NULL;
    return sub->children[index];
}

wp_u32 wp_game_sub_component_get_num_children( const wp_game_sub_component *sub )
{
    return sub->num_children;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_game_sub_component_get_user_data( const wp_game_sub_component *sub )
{
    return sub->user_data;
}

void wp_game_sub_component_set_user_data( wp_game_sub_component *sub, void *user_data )
{
    sub->user_data = user_data;
}
