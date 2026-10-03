/**
 * @file workphone_game_actor.h
 * @brief C API for game actors -- the fundamental scene entity type.
 */

#ifndef WORKPHONE_GAME_ACTOR_H
#define WORKPHONE_GAME_ACTOR_H

#include "workphone_quat.h"
#include "workphone_string.h"
#include "workphone_types.h"
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wp_game_component;
struct wp_game_scene;

enum
{
    WP_ACTOR_FLAG_RESERVED = ( 1u << 0 ),
    WP_ACTOR_FLAG_STATIC = ( 1u << 1 ),
    WP_ACTOR_FLAG_VISIBLE = ( 1u << 2 ),
    WP_ACTOR_FLAG_ENABLED = ( 1u << 3 ),
    WP_ACTOR_FLAG_MINE = ( 1u << 4 ),
    WP_ACTOR_FLAG_PERPETUAL = ( 1u << 5 ),
    WP_ACTOR_FLAG_DIRTY = ( 1u << 6 ),
    WP_ACTOR_FLAG_AWAKE = ( 1u << 7 ),
    WP_ACTOR_FLAG_STARTED = ( 1u << 8 ),
    WP_ACTOR_FLAG_DUMMY = ( 1u << 9 ),
    WP_ACTOR_FLAG_IN_SCENE = ( 1u << 10 ),
    WP_ACTOR_FLAG_ENABLED_IN_SCENE = ( 1u << 11 ),
    WP_ACTOR_FLAG_IS_EDITOR = ( 1u << 12 ),
    WP_ACTOR_FLAG_SMOOTH_MOTION = ( 1u << 13 ),
    WP_ACTOR_FLAG_HIDDEN = ( 1u << 14 ),
    WP_ACTOR_FLAG_DONT_SAVE = ( 1u << 15 ),
    WP_ACTOR_FLAG_PREFAB = ( 1u << 16 )
};

enum wp_actor_state
{
    WP_ACTOR_STATE_NONE = 0,
    WP_ACTOR_STATE_CREATE,
    WP_ACTOR_STATE_DESTROYED,
    WP_ACTOR_STATE_EDIT,
    WP_ACTOR_STATE_PLAY,
    WP_ACTOR_STATE_COUNT
};

#ifndef WP_ACTOR_MAX_CHILDREN
#    define WP_ACTOR_MAX_CHILDREN 64
#endif

#ifndef WP_ACTOR_MAX_COMPONENTS
#    define WP_ACTOR_MAX_COMPONENTS 16
#endif

#ifndef WP_ACTOR_MAX_TAGS
#    define WP_ACTOR_MAX_TAGS 8
#endif

#ifndef WP_ACTOR_MAX_NAME
#    define WP_ACTOR_MAX_NAME 128
#endif

typedef struct
{
    wp_vec3f position;
    wp_quatf orientation;
    wp_vec3f scale;
} wp_transform3f;

typedef struct wp_game_actor
{
    wp_s32 id;
    wp_c8 name[WP_ACTOR_MAX_NAME];

    wp_u32 flags;
    wp_u32 previous_flags;
    enum wp_actor_state state;

    wp_transform3f local_transform;
    wp_transform3f world_transform;

    struct wp_game_actor *parent;
    struct wp_game_actor *children[WP_ACTOR_MAX_CHILDREN];
    wp_u32 num_children;
    wp_s32 sibling_index;

    struct wp_game_component *components[WP_ACTOR_MAX_COMPONENTS];
    wp_u32 num_components;

    wp_string tags[WP_ACTOR_MAX_TAGS];
    wp_u32 num_tags;

    wp_string layer;

    struct wp_game_scene *scene;
} wp_game_actor;

void wp_game_actor_init( wp_game_actor *actor );
void wp_game_actor_init_with_id( wp_game_actor *actor, wp_s32 id );
void wp_game_actor_destroy( wp_game_actor *actor );

void wp_game_actor_set_name( wp_game_actor *actor, const wp_c8 *name );
const wp_c8 *wp_game_actor_get_name( const wp_game_actor *actor );

wp_transform3f wp_game_actor_get_local_transform( const wp_game_actor *actor );
void wp_game_actor_set_local_transform( wp_game_actor *actor, wp_transform3f t );

wp_vec3f wp_game_actor_get_local_position( const wp_game_actor *actor );
void wp_game_actor_set_local_position( wp_game_actor *actor, wp_vec3f position );

wp_vec3f wp_game_actor_get_local_scale( const wp_game_actor *actor );
void wp_game_actor_set_local_scale( wp_game_actor *actor, wp_vec3f scale );

wp_quatf wp_game_actor_get_local_orientation( const wp_game_actor *actor );
void wp_game_actor_set_local_orientation( wp_game_actor *actor, wp_quatf orientation );

wp_transform3f wp_game_actor_get_world_transform( const wp_game_actor *actor );

wp_vec3f wp_game_actor_get_position( const wp_game_actor *actor );
void wp_game_actor_set_position( wp_game_actor *actor, wp_vec3f position );

wp_vec3f wp_game_actor_get_scale( const wp_game_actor *actor );
void wp_game_actor_set_scale( wp_game_actor *actor, wp_vec3f scale );

wp_quatf wp_game_actor_get_orientation( const wp_game_actor *actor );
void wp_game_actor_set_orientation( wp_game_actor *actor, wp_quatf orientation );

void wp_game_actor_update_transform( wp_game_actor *actor );

wp_game_actor *wp_game_actor_get_parent( const wp_game_actor *actor );
void wp_game_actor_set_parent( wp_game_actor *actor, wp_game_actor *parent );

wp_s32 wp_game_actor_add_child( wp_game_actor *actor, wp_game_actor *child );
wp_s32 wp_game_actor_remove_child( wp_game_actor *actor, wp_game_actor *child );
void wp_game_actor_remove_children( wp_game_actor *actor );

wp_game_actor *wp_game_actor_get_child( const wp_game_actor *actor, wp_u32 index );
wp_u32 wp_game_actor_get_num_children( const wp_game_actor *actor );

wp_game_actor *wp_game_actor_find_child_by_name( const wp_game_actor *actor, const wp_c8 *name,
                                                 wp_s32 cascade );

wp_s32 wp_game_actor_get_sibling_index( const wp_game_actor *actor );
void wp_game_actor_set_sibling_index( wp_game_actor *actor, wp_s32 index );

wp_s32 wp_game_actor_add_component( wp_game_actor *actor, struct wp_game_component *component );
wp_s32 wp_game_actor_remove_component( wp_game_actor *actor, struct wp_game_component *component );
struct wp_game_component *wp_game_actor_get_component( const wp_game_actor *actor, wp_u32 index );
wp_u32 wp_game_actor_get_num_components( const wp_game_actor *actor );

wp_u32 wp_game_actor_get_flags( const wp_game_actor *actor );
void wp_game_actor_set_flags( wp_game_actor *actor, wp_u32 flags );
wp_s32 wp_game_actor_get_flag( const wp_game_actor *actor, wp_u32 flag );
void wp_game_actor_set_flag( wp_game_actor *actor, wp_u32 flag, wp_s32 value );

wp_s32 wp_game_actor_is_enabled( const wp_game_actor *actor );
void wp_game_actor_set_enabled( wp_game_actor *actor, wp_s32 enabled );

wp_s32 wp_game_actor_is_visible( const wp_game_actor *actor );
void wp_game_actor_set_visible( wp_game_actor *actor, wp_s32 visible );

wp_s32 wp_game_actor_is_static( const wp_game_actor *actor );
void wp_game_actor_set_static( wp_game_actor *actor, wp_s32 is_static );

wp_s32 wp_game_actor_is_dirty( const wp_game_actor *actor );
void wp_game_actor_set_dirty( wp_game_actor *actor, wp_s32 dirty );

enum wp_actor_state wp_game_actor_get_state( const wp_game_actor *actor );
void wp_game_actor_set_state( wp_game_actor *actor, enum wp_actor_state state );

wp_s32 wp_game_actor_add_tag( wp_game_actor *actor, const wp_c8 *tag );
wp_s32 wp_game_actor_remove_tag( wp_game_actor *actor, const wp_c8 *tag );
wp_s32 wp_game_actor_has_tag( const wp_game_actor *actor, const wp_c8 *tag );
void wp_game_actor_clear_tags( wp_game_actor *actor );
wp_u32 wp_game_actor_get_num_tags( const wp_game_actor *actor );
const wp_string *wp_game_actor_get_tag( const wp_game_actor *actor, wp_u32 index );

const wp_string *wp_game_actor_get_layer( const wp_game_actor *actor );
void wp_game_actor_set_layer( wp_game_actor *actor, const wp_c8 *layer_name );

struct wp_game_scene *wp_game_actor_get_scene( const wp_game_actor *actor );
void wp_game_actor_set_scene( wp_game_actor *actor, struct wp_game_scene *scene );

wp_s32 wp_game_actor_get_perpetual( const wp_game_actor *actor );
void wp_game_actor_set_perpetual( wp_game_actor *actor, wp_s32 perpetual );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_ACTOR_H */
