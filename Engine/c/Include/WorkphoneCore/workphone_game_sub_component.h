/**
 * @file workphone_game_sub_component.h
 * @brief C API for game sub-components attached to components.
 */

#ifndef WORKPHONE_GAME_SUB_COMPONENT_H
#define WORKPHONE_GAME_SUB_COMPONENT_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_string.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wp_game_component;

#ifndef WP_SUB_COMPONENT_MAX_NAME
#    define WP_SUB_COMPONENT_MAX_NAME 128
#endif

#ifndef WP_SUB_COMPONENT_MAX_CHILDREN
#    define WP_SUB_COMPONENT_MAX_CHILDREN 16
#endif

/**
 * @brief A sub-component attached to a parent component or another
 *        sub-component.
 *
 * Mirrors the C++ ISubComponent / SubComponent.  A sub-component has an
 * owning component, an optional parent sub-component, and a fixed-size
 * child array.
 */
typedef struct wp_game_sub_component
{
    wp_c8 name[WP_SUB_COMPONENT_MAX_NAME];

    wp_u32 type;

    struct wp_game_component *parent_component;
    struct wp_game_sub_component *parent;

    struct wp_game_sub_component *children[WP_SUB_COMPONENT_MAX_CHILDREN];
    wp_u32 num_children;

    void *user_data;
} wp_game_sub_component;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_game_sub_component_init( wp_game_sub_component *sub );
void wp_game_sub_component_destroy( wp_game_sub_component *sub );

/* ---- Name ------------------------------------------------------------- */

void wp_game_sub_component_set_name( wp_game_sub_component *sub, const wp_c8 *name );
const wp_c8 *wp_game_sub_component_get_name( const wp_game_sub_component *sub );

/* ---- Type ------------------------------------------------------------- */

wp_u32 wp_game_sub_component_get_type( const wp_game_sub_component *sub );
void wp_game_sub_component_set_type( wp_game_sub_component *sub, wp_u32 type );

/* ---- Parent component ------------------------------------------------- */

struct wp_game_component *wp_game_sub_component_get_parent_component( const wp_game_sub_component *sub );
void wp_game_sub_component_set_parent_component( wp_game_sub_component *sub,
                                                 struct wp_game_component *parent_component );

/* ---- Parent sub-component --------------------------------------------- */

wp_game_sub_component *wp_game_sub_component_get_parent( const wp_game_sub_component *sub );
void wp_game_sub_component_set_parent( wp_game_sub_component *sub, wp_game_sub_component *parent );

/* ---- Children --------------------------------------------------------- */

wp_s32 wp_game_sub_component_add_child( wp_game_sub_component *sub, wp_game_sub_component *child );
wp_s32 wp_game_sub_component_remove_child( wp_game_sub_component *sub, wp_game_sub_component *child );
wp_game_sub_component *wp_game_sub_component_get_child( const wp_game_sub_component *sub, wp_u32 index );
wp_u32 wp_game_sub_component_get_num_children( const wp_game_sub_component *sub );

/* ---- User data -------------------------------------------------------- */

void *wp_game_sub_component_get_user_data( const wp_game_sub_component *sub );
void wp_game_sub_component_set_user_data( wp_game_sub_component *sub, void *user_data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_SUB_COMPONENT_H */
