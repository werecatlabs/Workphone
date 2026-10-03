/**
 * @file workphone_game_actor.c
 * @brief Implementation of the C game actor API.
 */

#include "workphone_game_actor.h"

#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_s32 wp_game_actor_next_id = 1;

static void wp_game_actor_init_defaults( wp_game_actor *actor, wp_s32 id )
{
    memset( actor, 0, sizeof( *actor ) );
    actor->id = id;
    actor->name[0] = '\0';
    actor->flags = WP_ACTOR_FLAG_VISIBLE | WP_ACTOR_FLAG_ENABLED;
    actor->previous_flags = 0;
    actor->state = WP_ACTOR_STATE_NONE;

    actor->local_transform.position = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    actor->local_transform.orientation = wp_quatf_identity();
    actor->local_transform.scale = wp_vec3f_make( 1.0f, 1.0f, 1.0f );

    actor->world_transform = actor->local_transform;

    actor->parent = NULL;
    actor->num_children = 0;
    actor->sibling_index = -1;

    actor->num_components = 0;
    actor->num_tags = 0;

    actor->layer = wp_string_make_empty();
    actor->scene = NULL;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_game_actor_init( wp_game_actor *actor )
{
    wp_game_actor_init_defaults( actor, wp_game_actor_next_id++ );
}

void wp_game_actor_init_with_id( wp_game_actor *actor, wp_s32 id )
{
    wp_game_actor_init_defaults( actor, id );
}

void wp_game_actor_destroy( wp_game_actor *actor )
{
    wp_u32 i;
    for( i = 0; i < actor->num_tags; ++i )
        wp_string_free( &actor->tags[i] );
    actor->num_tags = 0;
    wp_string_free( &actor->layer );
    actor->num_children = 0;
    actor->num_components = 0;
    actor->parent = NULL;
    actor->scene = NULL;
    actor->state = WP_ACTOR_STATE_DESTROYED;
}

/* =========================================================================
 * Name
 * ====================================================================== */

void wp_game_actor_set_name( wp_game_actor *actor, const wp_c8 *name )
{
    if( name )
    {
        strncpy( actor->name, name, WP_ACTOR_MAX_NAME - 1 );
        actor->name[WP_ACTOR_MAX_NAME - 1] = '\0';
    }
    else
    {
        actor->name[0] = '\0';
    }
}

const wp_c8 *wp_game_actor_get_name( const wp_game_actor *actor )
{
    return actor->name;
}

/* =========================================================================
 * Transform -- local
 * ====================================================================== */

wp_transform3f wp_game_actor_get_local_transform( const wp_game_actor *actor )
{
    return actor->local_transform;
}

void wp_game_actor_set_local_transform( wp_game_actor *actor, wp_transform3f t )
{
    actor->local_transform = t;
    actor->flags |= WP_ACTOR_FLAG_DIRTY;
}

wp_vec3f wp_game_actor_get_local_position( const wp_game_actor *actor )
{
    return actor->local_transform.position;
}

void wp_game_actor_set_local_position( wp_game_actor *actor, wp_vec3f position )
{
    actor->local_transform.position = position;
    actor->flags |= WP_ACTOR_FLAG_DIRTY;
}

wp_vec3f wp_game_actor_get_local_scale( const wp_game_actor *actor )
{
    return actor->local_transform.scale;
}

void wp_game_actor_set_local_scale( wp_game_actor *actor, wp_vec3f scale )
{
    actor->local_transform.scale = scale;
    actor->flags |= WP_ACTOR_FLAG_DIRTY;
}

wp_quatf wp_game_actor_get_local_orientation( const wp_game_actor *actor )
{
    return actor->local_transform.orientation;
}

void wp_game_actor_set_local_orientation( wp_game_actor *actor, wp_quatf orientation )
{
    actor->local_transform.orientation = orientation;
    actor->flags |= WP_ACTOR_FLAG_DIRTY;
}

/* =========================================================================
 * Transform -- world
 * ====================================================================== */

void wp_game_actor_update_transform( wp_game_actor *actor )
{
    wp_u32 i;
    if( actor->parent )
    {
        wp_quatf pq = actor->parent->world_transform.orientation;
        wp_vec3f ps = actor->parent->world_transform.scale;
        wp_vec3f pp = actor->parent->world_transform.position;
        wp_vec3f scaled = wp_vec3f_mul( actor->local_transform.position, ps );
        wp_vec3f rotated = wp_quatf_rotate_vec3( pq, scaled );

        actor->world_transform.position = wp_vec3f_add( pp, rotated );
        actor->world_transform.orientation = wp_quatf_mul( pq, actor->local_transform.orientation );
        actor->world_transform.scale = wp_vec3f_mul( ps, actor->local_transform.scale );
    }
    else
    {
        actor->world_transform = actor->local_transform;
    }
    actor->flags &= ~WP_ACTOR_FLAG_DIRTY;

    for( i = 0; i < actor->num_children; ++i )
    {
        if( actor->children[i] )
            wp_game_actor_update_transform( actor->children[i] );
    }
}

wp_transform3f wp_game_actor_get_world_transform( const wp_game_actor *actor )
{
    return actor->world_transform;
}

wp_vec3f wp_game_actor_get_position( const wp_game_actor *actor )
{
    return actor->world_transform.position;
}

void wp_game_actor_set_position( wp_game_actor *actor, wp_vec3f position )
{
    if( actor->parent )
    {
        wp_quatf inv_pq = wp_quatf_conjugate( actor->parent->world_transform.orientation );
        wp_vec3f diff = wp_vec3f_sub( position, actor->parent->world_transform.position );
        wp_vec3f unrotated = wp_quatf_rotate_vec3( inv_pq, diff );
        wp_vec3f ps = actor->parent->world_transform.scale;
        wp_vec3f inv_scale =
            wp_vec3f_make( ps.x != 0.0f ? 1.0f / ps.x : 0.0f, ps.y != 0.0f ? 1.0f / ps.y : 0.0f,
                           ps.z != 0.0f ? 1.0f / ps.z : 0.0f );
        actor->local_transform.position = wp_vec3f_mul( unrotated, inv_scale );
    }
    else
    {
        actor->local_transform.position = position;
    }
    actor->flags |= WP_ACTOR_FLAG_DIRTY;
}

wp_vec3f wp_game_actor_get_scale( const wp_game_actor *actor )
{
    return actor->world_transform.scale;
}

void wp_game_actor_set_scale( wp_game_actor *actor, wp_vec3f scale )
{
    if( actor->parent )
    {
        wp_vec3f ps = actor->parent->world_transform.scale;
        wp_vec3f inv_scale =
            wp_vec3f_make( ps.x != 0.0f ? 1.0f / ps.x : 0.0f, ps.y != 0.0f ? 1.0f / ps.y : 0.0f,
                           ps.z != 0.0f ? 1.0f / ps.z : 0.0f );
        actor->local_transform.scale = wp_vec3f_mul( scale, inv_scale );
    }
    else
    {
        actor->local_transform.scale = scale;
    }
    actor->flags |= WP_ACTOR_FLAG_DIRTY;
}

wp_quatf wp_game_actor_get_orientation( const wp_game_actor *actor )
{
    return actor->world_transform.orientation;
}

void wp_game_actor_set_orientation( wp_game_actor *actor, wp_quatf orientation )
{
    if( actor->parent )
    {
        wp_quatf inv_pq = wp_quatf_conjugate( actor->parent->world_transform.orientation );
        actor->local_transform.orientation = wp_quatf_mul( inv_pq, orientation );
    }
    else
    {
        actor->local_transform.orientation = orientation;
    }
    actor->flags |= WP_ACTOR_FLAG_DIRTY;
}

/* =========================================================================
 * Hierarchy
 * ====================================================================== */

wp_game_actor *wp_game_actor_get_parent( const wp_game_actor *actor )
{
    return actor->parent;
}

void wp_game_actor_set_parent( wp_game_actor *actor, wp_game_actor *parent )
{
    if( actor->parent == parent )
        return;
    if( actor->parent )
        wp_game_actor_remove_child( actor->parent, actor );
    actor->parent = parent;
    if( parent )
        wp_game_actor_add_child( parent, actor );
}

wp_s32 wp_game_actor_add_child( wp_game_actor *actor, wp_game_actor *child )
{
    wp_u32 i;
    if( !child || actor->num_children >= WP_ACTOR_MAX_CHILDREN )
        return 0;
    for( i = 0; i < actor->num_children; ++i )
    {
        if( actor->children[i] == child )
            return 0;
    }
    child->parent = actor;
    child->sibling_index = (wp_s32)actor->num_children;
    actor->children[actor->num_children++] = child;
    child->flags |= WP_ACTOR_FLAG_DIRTY;
    return 1;
}

wp_s32 wp_game_actor_remove_child( wp_game_actor *actor, wp_game_actor *child )
{
    wp_u32 i;
    if( !child )
        return 0;
    for( i = 0; i < actor->num_children; ++i )
    {
        if( actor->children[i] == child )
        {
            child->parent = NULL;
            child->sibling_index = -1;
            actor->num_children--;
            for( ; i < actor->num_children; ++i )
            {
                actor->children[i] = actor->children[i + 1];
                actor->children[i]->sibling_index = (wp_s32)i;
            }
            actor->children[actor->num_children] = NULL;
            return 1;
        }
    }
    return 0;
}

void wp_game_actor_remove_children( wp_game_actor *actor )
{
    wp_u32 i;
    for( i = 0; i < actor->num_children; ++i )
    {
        if( actor->children[i] )
        {
            actor->children[i]->parent = NULL;
            actor->children[i]->sibling_index = -1;
            actor->children[i] = NULL;
        }
    }
    actor->num_children = 0;
}

wp_game_actor *wp_game_actor_get_child( const wp_game_actor *actor, wp_u32 index )
{
    if( index >= actor->num_children )
        return NULL;
    return actor->children[index];
}

wp_u32 wp_game_actor_get_num_children( const wp_game_actor *actor )
{
    return actor->num_children;
}

wp_game_actor *wp_game_actor_find_child_by_name( const wp_game_actor *actor, const wp_c8 *name,
                                                 wp_s32 cascade )
{
    wp_u32 i;
    for( i = 0; i < actor->num_children; ++i )
    {
        wp_game_actor *c = actor->children[i];
        if( c && strcmp( c->name, name ) == 0 )
            return c;
    }
    if( cascade )
    {
        for( i = 0; i < actor->num_children; ++i )
        {
            wp_game_actor *found = wp_game_actor_find_child_by_name( actor->children[i], name, cascade );
            if( found )
                return found;
        }
    }
    return NULL;
}

wp_s32 wp_game_actor_get_sibling_index( const wp_game_actor *actor )
{
    return actor->sibling_index;
}

void wp_game_actor_set_sibling_index( wp_game_actor *actor, wp_s32 index )
{
    actor->sibling_index = index;
}

/* =========================================================================
 * Components
 * ====================================================================== */

wp_s32 wp_game_actor_add_component( wp_game_actor *actor, struct wp_game_component *component )
{
    if( !component || actor->num_components >= WP_ACTOR_MAX_COMPONENTS )
        return 0;
    actor->components[actor->num_components++] = component;
    return 1;
}

wp_s32 wp_game_actor_remove_component( wp_game_actor *actor, struct wp_game_component *component )
{
    wp_u32 i;
    if( !component )
        return 0;
    for( i = 0; i < actor->num_components; ++i )
    {
        if( actor->components[i] == component )
        {
            actor->num_components--;
            for( ; i < actor->num_components; ++i )
                actor->components[i] = actor->components[i + 1];
            actor->components[actor->num_components] = NULL;
            return 1;
        }
    }
    return 0;
}

struct wp_game_component *wp_game_actor_get_component( const wp_game_actor *actor, wp_u32 index )
{
    if( index >= actor->num_components )
        return NULL;
    return actor->components[index];
}

wp_u32 wp_game_actor_get_num_components( const wp_game_actor *actor )
{
    return actor->num_components;
}

/* =========================================================================
 * Flags
 * ====================================================================== */

wp_u32 wp_game_actor_get_flags( const wp_game_actor *actor )
{
    return actor->flags;
}

void wp_game_actor_set_flags( wp_game_actor *actor, wp_u32 flags )
{
    actor->previous_flags = actor->flags;
    actor->flags = flags;
}

wp_s32 wp_game_actor_get_flag( const wp_game_actor *actor, wp_u32 flag )
{
    return ( actor->flags & flag ) != 0;
}

void wp_game_actor_set_flag( wp_game_actor *actor, wp_u32 flag, wp_s32 value )
{
    actor->previous_flags = actor->flags;
    if( value )
        actor->flags |= flag;
    else
        actor->flags &= ~flag;
}

wp_s32 wp_game_actor_is_enabled( const wp_game_actor *actor )
{
    return ( actor->flags & WP_ACTOR_FLAG_ENABLED ) != 0;
}

void wp_game_actor_set_enabled( wp_game_actor *actor, wp_s32 enabled )
{
    wp_game_actor_set_flag( actor, WP_ACTOR_FLAG_ENABLED, enabled );
}

wp_s32 wp_game_actor_is_visible( const wp_game_actor *actor )
{
    return ( actor->flags & WP_ACTOR_FLAG_VISIBLE ) != 0;
}

void wp_game_actor_set_visible( wp_game_actor *actor, wp_s32 visible )
{
    wp_game_actor_set_flag( actor, WP_ACTOR_FLAG_VISIBLE, visible );
}

wp_s32 wp_game_actor_is_static( const wp_game_actor *actor )
{
    return ( actor->flags & WP_ACTOR_FLAG_STATIC ) != 0;
}

void wp_game_actor_set_static( wp_game_actor *actor, wp_s32 is_static )
{
    wp_game_actor_set_flag( actor, WP_ACTOR_FLAG_STATIC, is_static );
}

wp_s32 wp_game_actor_is_dirty( const wp_game_actor *actor )
{
    return ( actor->flags & WP_ACTOR_FLAG_DIRTY ) != 0;
}

void wp_game_actor_set_dirty( wp_game_actor *actor, wp_s32 dirty )
{
    wp_game_actor_set_flag( actor, WP_ACTOR_FLAG_DIRTY, dirty );
}

/* =========================================================================
 * State
 * ====================================================================== */

enum wp_actor_state wp_game_actor_get_state( const wp_game_actor *actor )
{
    return actor->state;
}

void wp_game_actor_set_state( wp_game_actor *actor, enum wp_actor_state state )
{
    actor->state = state;
}

/* =========================================================================
 * Tags
 * ====================================================================== */

wp_s32 wp_game_actor_add_tag( wp_game_actor *actor, const wp_c8 *tag )
{
    if( !tag || actor->num_tags >= WP_ACTOR_MAX_TAGS )
        return 0;
    if( wp_game_actor_has_tag( actor, tag ) )
        return 0;
    actor->tags[actor->num_tags++] = wp_string_make( tag );
    return 1;
}

wp_s32 wp_game_actor_remove_tag( wp_game_actor *actor, const wp_c8 *tag )
{
    wp_u32 i;
    if( !tag )
        return 0;
    for( i = 0; i < actor->num_tags; ++i )
    {
        if( wp_string_equals_cstr( &actor->tags[i], tag ) )
        {
            wp_string_free( &actor->tags[i] );
            actor->num_tags--;
            for( ; i < actor->num_tags; ++i )
                actor->tags[i] = actor->tags[i + 1];
            memset( &actor->tags[actor->num_tags], 0, sizeof( wp_string ) );
            return 1;
        }
    }
    return 0;
}

wp_s32 wp_game_actor_has_tag( const wp_game_actor *actor, const wp_c8 *tag )
{
    wp_u32 i;
    if( !tag )
        return 0;
    for( i = 0; i < actor->num_tags; ++i )
    {
        if( wp_string_equals_cstr( &actor->tags[i], tag ) )
            return 1;
    }
    return 0;
}

void wp_game_actor_clear_tags( wp_game_actor *actor )
{
    wp_u32 i;
    for( i = 0; i < actor->num_tags; ++i )
        wp_string_free( &actor->tags[i] );
    actor->num_tags = 0;
}

wp_u32 wp_game_actor_get_num_tags( const wp_game_actor *actor )
{
    return actor->num_tags;
}

const wp_string *wp_game_actor_get_tag( const wp_game_actor *actor, wp_u32 index )
{
    if( index >= actor->num_tags )
        return NULL;
    return &actor->tags[index];
}

/* =========================================================================
 * Layer
 * ====================================================================== */

const wp_string *wp_game_actor_get_layer( const wp_game_actor *actor )
{
    return &actor->layer;
}

void wp_game_actor_set_layer( wp_game_actor *actor, const wp_c8 *layer_name )
{
    wp_string_free( &actor->layer );
    if( layer_name )
        actor->layer = wp_string_make( layer_name );
    else
        actor->layer = wp_string_make_empty();
}

/* =========================================================================
 * Scene
 * ====================================================================== */

struct wp_game_scene *wp_game_actor_get_scene( const wp_game_actor *actor )
{
    return actor->scene;
}

void wp_game_actor_set_scene( wp_game_actor *actor, struct wp_game_scene *scene )
{
    actor->scene = scene;
    if( scene )
        actor->flags |= WP_ACTOR_FLAG_IN_SCENE;
    else
        actor->flags &= ~WP_ACTOR_FLAG_IN_SCENE;
}

/* =========================================================================
 * Perpetual
 * ====================================================================== */

wp_s32 wp_game_actor_get_perpetual( const wp_game_actor *actor )
{
    return ( actor->flags & WP_ACTOR_FLAG_PERPETUAL ) != 0;
}

void wp_game_actor_set_perpetual( wp_game_actor *actor, wp_s32 perpetual )
{
    wp_game_actor_set_flag( actor, WP_ACTOR_FLAG_PERPETUAL, perpetual );
}
