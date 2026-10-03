/**
 * @file workphone_game_component.c
 * @brief Implementation of the C game component API.
 */

#include "workphone_game_component.h"

#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void wp_game_component_init_defaults( wp_game_component *comp, enum wp_component_type type )
{
    memset( comp, 0, sizeof( *comp ) );
    comp->name[0] = '\0';
    comp->type = type;
    comp->state = WP_COMPONENT_STATE_NONE;
    comp->flags = WP_COMPONENT_FLAG_ENABLED | WP_COMPONENT_FLAG_VISIBLE;
    comp->previous_flags = 0;
    comp->actor = NULL;
    comp->num_sub_components = 0;
    comp->user_data = NULL;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_game_component_init( wp_game_component *comp )
{
    wp_game_component_init_defaults( comp, WP_COMPONENT_TYPE_CUSTOM );
}

void wp_game_component_init_with_type( wp_game_component *comp, enum wp_component_type type )
{
    wp_game_component_init_defaults( comp, type );
}

void wp_game_component_destroy( wp_game_component *comp )
{
    wp_u32 i;
    if( comp->callbacks.on_destroy )
        comp->callbacks.on_destroy( comp );

    for( i = 0; i < comp->num_sub_components; ++i )
        comp->sub_components[i] = NULL;
    comp->num_sub_components = 0;

    comp->actor = NULL;
    comp->user_data = NULL;
    comp->state = WP_COMPONENT_STATE_DESTROYED;
    comp->flags = 0;
}

/* =========================================================================
 * Name
 * ====================================================================== */

void wp_game_component_set_name( wp_game_component *comp, const wp_c8 *name )
{
    if( name )
    {
        strncpy( comp->name, name, WP_COMPONENT_MAX_NAME - 1 );
        comp->name[WP_COMPONENT_MAX_NAME - 1] = '\0';
    }
    else
    {
        comp->name[0] = '\0';
    }
}

const wp_c8 *wp_game_component_get_name( const wp_game_component *comp )
{
    return comp->name;
}

/* =========================================================================
 * Type
 * ====================================================================== */

enum wp_component_type wp_game_component_get_type( const wp_game_component *comp )
{
    return comp->type;
}

/* =========================================================================
 * State
 * ====================================================================== */

enum wp_component_state wp_game_component_get_state( const wp_game_component *comp )
{
    return comp->state;
}

void wp_game_component_set_state( wp_game_component *comp, enum wp_component_state state )
{
    comp->state = state;
    if( comp->callbacks.on_state_changed )
        comp->callbacks.on_state_changed( comp, state );
}

/* =========================================================================
 * Flags
 * ====================================================================== */

wp_u32 wp_game_component_get_flags( const wp_game_component *comp )
{
    return comp->flags;
}

void wp_game_component_set_flags( wp_game_component *comp, wp_u32 flags )
{
    comp->previous_flags = comp->flags;
    comp->flags = flags;
    if( comp->callbacks.on_flags_changed )
        comp->callbacks.on_flags_changed( comp, comp->previous_flags );
}

wp_s32 wp_game_component_get_flag( const wp_game_component *comp, wp_u32 flag )
{
    return ( comp->flags & flag ) != 0;
}

void wp_game_component_set_flag( wp_game_component *comp, wp_u32 flag, wp_s32 value )
{
    comp->previous_flags = comp->flags;
    if( value )
        comp->flags |= flag;
    else
        comp->flags &= ~flag;
    if( comp->callbacks.on_flags_changed )
        comp->callbacks.on_flags_changed( comp, comp->previous_flags );
}

/* =========================================================================
 * Enabled / Visible / Dirty
 * ====================================================================== */

wp_s32 wp_game_component_is_enabled( const wp_game_component *comp )
{
    return ( comp->flags & WP_COMPONENT_FLAG_ENABLED ) != 0;
}

void wp_game_component_set_enabled( wp_game_component *comp, wp_s32 enabled )
{
    wp_game_component_set_flag( comp, WP_COMPONENT_FLAG_ENABLED, enabled );
    if( enabled )
    {
        if( comp->callbacks.on_enable )
            comp->callbacks.on_enable( comp );
    }
    else
    {
        if( comp->callbacks.on_disable )
            comp->callbacks.on_disable( comp );
    }
}

wp_s32 wp_game_component_is_visible( const wp_game_component *comp )
{
    return ( comp->flags & WP_COMPONENT_FLAG_VISIBLE ) != 0;
}

void wp_game_component_set_visible( wp_game_component *comp, wp_s32 visible )
{
    wp_game_component_set_flag( comp, WP_COMPONENT_FLAG_VISIBLE, visible );
}

wp_s32 wp_game_component_is_dirty( const wp_game_component *comp )
{
    return ( comp->flags & WP_COMPONENT_FLAG_DIRTY ) != 0;
}

void wp_game_component_set_dirty( wp_game_component *comp, wp_s32 dirty )
{
    wp_game_component_set_flag( comp, WP_COMPONENT_FLAG_DIRTY, dirty );
}

/* =========================================================================
 * Actor
 * ====================================================================== */

struct wp_game_actor *wp_game_component_get_actor( const wp_game_component *comp )
{
    return comp->actor;
}

void wp_game_component_set_actor( wp_game_component *comp, struct wp_game_actor *actor )
{
    comp->actor = actor;
}

/* =========================================================================
 * Sub-components
 * ====================================================================== */

wp_s32 wp_game_component_add_sub_component( wp_game_component *comp, wp_game_component *sub )
{
    wp_u32 i;
    if( !sub || comp->num_sub_components >= WP_COMPONENT_MAX_SUB_COMPONENTS )
        return 0;
    for( i = 0; i < comp->num_sub_components; ++i )
    {
        if( comp->sub_components[i] == sub )
            return 0;
    }
    comp->sub_components[comp->num_sub_components++] = sub;
    return 1;
}

wp_s32 wp_game_component_remove_sub_component( wp_game_component *comp, wp_game_component *sub )
{
    wp_u32 i;
    if( !sub )
        return 0;
    for( i = 0; i < comp->num_sub_components; ++i )
    {
        if( comp->sub_components[i] == sub )
        {
            comp->num_sub_components--;
            for( ; i < comp->num_sub_components; ++i )
                comp->sub_components[i] = comp->sub_components[i + 1];
            comp->sub_components[comp->num_sub_components] = NULL;
            return 1;
        }
    }
    return 0;
}

wp_s32 wp_game_component_remove_sub_component_by_index( wp_game_component *comp, wp_u32 index )
{
    wp_u32 i;
    if( index >= comp->num_sub_components )
        return 0;
    comp->num_sub_components--;
    for( i = index; i < comp->num_sub_components; ++i )
        comp->sub_components[i] = comp->sub_components[i + 1];
    comp->sub_components[comp->num_sub_components] = NULL;
    return 1;
}

wp_game_component *wp_game_component_get_sub_component( const wp_game_component *comp, wp_u32 index )
{
    if( index >= comp->num_sub_components )
        return NULL;
    return comp->sub_components[index];
}

wp_u32 wp_game_component_get_num_sub_components( const wp_game_component *comp )
{
    return comp->num_sub_components;
}

/* =========================================================================
 * Callbacks
 * ====================================================================== */

void wp_game_component_set_callbacks( wp_game_component *comp, wp_component_callbacks callbacks )
{
    comp->callbacks = callbacks;
}

wp_component_callbacks wp_game_component_get_callbacks( const wp_game_component *comp )
{
    return comp->callbacks;
}

/* =========================================================================
 * Update
 * ====================================================================== */

void wp_game_component_update( wp_game_component *comp, wp_f64 dt )
{
    if( !( comp->flags & WP_COMPONENT_FLAG_ENABLED ) )
        return;

    if( comp->callbacks.on_update )
        comp->callbacks.on_update( comp, dt );
}

void wp_game_component_update_transform( wp_game_component *comp )
{
    if( comp->callbacks.on_transform_updated )
        comp->callbacks.on_transform_updated( comp );
}

void wp_game_component_update_flags( wp_game_component *comp, wp_u32 flags )
{
    wp_u32 old_flags = comp->flags;
    comp->previous_flags = old_flags;
    comp->flags = flags;
    if( comp->callbacks.on_flags_changed )
        comp->callbacks.on_flags_changed( comp, old_flags );
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_game_component_get_user_data( const wp_game_component *comp )
{
    return comp->user_data;
}

void wp_game_component_set_user_data( wp_game_component *comp, void *user_data )
{
    comp->user_data = user_data;
}
