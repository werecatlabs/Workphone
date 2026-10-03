/**
 * @file workphone_game_component.h
 * @brief C API for game components attached to actors.
 */

#ifndef WORKPHONE_GAME_COMPONENT_H
#define WORKPHONE_GAME_COMPONENT_H

#include "workphone_prerequisites.h"
#include "workphone_string.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wp_game_actor;

/**
 * @brief Component lifecycle / runtime states.
 */
enum wp_component_state
{
    WP_COMPONENT_STATE_NONE = 0,
    WP_COMPONENT_STATE_CREATE,
    WP_COMPONENT_STATE_DESTROYED,
    WP_COMPONENT_STATE_EDIT,
    WP_COMPONENT_STATE_PLAY,
    WP_COMPONENT_STATE_RESET,
    WP_COMPONENT_STATE_COUNT
};

/**
 * @brief Component flag bits.
 */
enum
{
    WP_COMPONENT_FLAG_RESERVED = ( 1u << 0 ),
    WP_COMPONENT_FLAG_ENABLED = ( 1u << 1 ),
    WP_COMPONENT_FLAG_VISIBLE = ( 1u << 2 ),
    WP_COMPONENT_FLAG_DIRTY = ( 1u << 3 ),
    WP_COMPONENT_FLAG_STATIC = ( 1u << 4 )
};

/**
 * @brief Component type identifiers.
 *
 * Used to distinguish concrete component kinds without RTTI.
 */
enum wp_component_type
{
    WP_COMPONENT_TYPE_CUSTOM = 0,
    WP_COMPONENT_TYPE_RENDERER,
    WP_COMPONENT_TYPE_CAMERA,
    WP_COMPONENT_TYPE_PHYSICS,
    WP_COMPONENT_TYPE_TERRAIN,
    WP_COMPONENT_TYPE_NETWORK,
    WP_COMPONENT_TYPE_UI,
    WP_COMPONENT_TYPE_VEHICLE,
    WP_COMPONENT_TYPE_COUNT
};

#ifndef WP_COMPONENT_MAX_NAME
#    define WP_COMPONENT_MAX_NAME 128
#endif

#ifndef WP_COMPONENT_MAX_SUB_COMPONENTS
#    define WP_COMPONENT_MAX_SUB_COMPONENTS 8
#endif

/**
 * @brief Optional per-frame callbacks for a component.
 *
 * A concrete component type sets whichever callbacks it needs; unused
 * slots are left NULL.
 */
typedef struct wp_component_callbacks
{
    void ( *on_create )( struct wp_game_component *comp );
    void ( *on_destroy )( struct wp_game_component *comp );
    void ( *on_update )( struct wp_game_component *comp, wp_f64 dt );
    void ( *on_enable )( struct wp_game_component *comp );
    void ( *on_disable )( struct wp_game_component *comp );
    void ( *on_state_changed )( struct wp_game_component *comp, enum wp_component_state new_state );
    void ( *on_transform_updated )( struct wp_game_component *comp );
    void ( *on_flags_changed )( struct wp_game_component *comp, wp_u32 old_flags );
} wp_component_callbacks;

/**
 * @brief Base game component attached to a wp_game_actor.
 */
typedef struct wp_game_component
{
    wp_c8 name[WP_COMPONENT_MAX_NAME];

    enum wp_component_type type;
    enum wp_component_state state;

    wp_u32 flags;
    wp_u32 previous_flags;

    struct wp_game_actor *actor;

    struct wp_game_component *sub_components[WP_COMPONENT_MAX_SUB_COMPONENTS];
    wp_u32 num_sub_components;

    wp_component_callbacks callbacks;

    void *user_data;
} wp_game_component;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_game_component_init( wp_game_component *comp );
void wp_game_component_init_with_type( wp_game_component *comp, enum wp_component_type type );
void wp_game_component_destroy( wp_game_component *comp );

/* ---- Name ------------------------------------------------------------- */

void wp_game_component_set_name( wp_game_component *comp, const wp_c8 *name );
const wp_c8 *wp_game_component_get_name( const wp_game_component *comp );

/* ---- Type ------------------------------------------------------------- */

enum wp_component_type wp_game_component_get_type( const wp_game_component *comp );

/* ---- State ------------------------------------------------------------ */

enum wp_component_state wp_game_component_get_state( const wp_game_component *comp );
void wp_game_component_set_state( wp_game_component *comp, enum wp_component_state state );

/* ---- Flags ------------------------------------------------------------ */

wp_u32 wp_game_component_get_flags( const wp_game_component *comp );
void wp_game_component_set_flags( wp_game_component *comp, wp_u32 flags );
wp_s32 wp_game_component_get_flag( const wp_game_component *comp, wp_u32 flag );
void wp_game_component_set_flag( wp_game_component *comp, wp_u32 flag, wp_s32 value );

/* ---- Enabled / Visible ------------------------------------------------ */

wp_s32 wp_game_component_is_enabled( const wp_game_component *comp );
void wp_game_component_set_enabled( wp_game_component *comp, wp_s32 enabled );

wp_s32 wp_game_component_is_visible( const wp_game_component *comp );
void wp_game_component_set_visible( wp_game_component *comp, wp_s32 visible );

wp_s32 wp_game_component_is_dirty( const wp_game_component *comp );
void wp_game_component_set_dirty( wp_game_component *comp, wp_s32 dirty );

/* ---- Actor ------------------------------------------------------------ */

struct wp_game_actor *wp_game_component_get_actor( const wp_game_component *comp );
void wp_game_component_set_actor( wp_game_component *comp, struct wp_game_actor *actor );

/* ---- Sub-components --------------------------------------------------- */

wp_s32 wp_game_component_add_sub_component( wp_game_component *comp, wp_game_component *sub );
wp_s32 wp_game_component_remove_sub_component( wp_game_component *comp, wp_game_component *sub );
wp_s32 wp_game_component_remove_sub_component_by_index( wp_game_component *comp, wp_u32 index );
wp_game_component *wp_game_component_get_sub_component( const wp_game_component *comp, wp_u32 index );
wp_u32 wp_game_component_get_num_sub_components( const wp_game_component *comp );

/* ---- Callbacks -------------------------------------------------------- */

void wp_game_component_set_callbacks( wp_game_component *comp, wp_component_callbacks callbacks );
wp_component_callbacks wp_game_component_get_callbacks( const wp_game_component *comp );

/* ---- Update ----------------------------------------------------------- */

void wp_game_component_update( wp_game_component *comp, wp_f64 dt );
void wp_game_component_update_transform( wp_game_component *comp );
void wp_game_component_update_flags( wp_game_component *comp, wp_u32 flags );

/* ---- User data -------------------------------------------------------- */

void *wp_game_component_get_user_data( const wp_game_component *comp );
void wp_game_component_set_user_data( wp_game_component *comp, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_COMPONENT_H */
