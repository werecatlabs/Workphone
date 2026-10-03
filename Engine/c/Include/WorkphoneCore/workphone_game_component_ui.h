/**
 * @file workphone_game_component_ui.h
 * @brief C API for a UI component attached to a game component.
 *
 * Mirrors the C++ UIComponent.  A UI component extends the base game
 * component with colour, label, canvas reference, visibility/ordering
 * helpers and input-handling flags.
 */

#ifndef WORKPHONE_GAME_COMPONENT_UI_H
#define WORKPHONE_GAME_COMPONENT_UI_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_game_component.h"
#include "workphone_color.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wp_ui_component;

/**
 * @brief Optional callbacks for UI component lifecycle / update events.
 */
typedef struct wp_ui_component_callbacks
{
    void ( *on_update_dimensions )( struct wp_ui_component *ui );
    void ( *on_update_materials )( struct wp_ui_component *ui );
    void ( *on_update_transform )( struct wp_ui_component *ui );
    void ( *on_update_visibility )( struct wp_ui_component *ui );
    void ( *on_update_order )( struct wp_ui_component *ui );
    void ( *on_update_flags )( struct wp_ui_component *ui, wp_u32 old_flags );
    void ( *on_update_colour )( struct wp_ui_component *ui );
    void ( *on_update_element_state )( struct wp_ui_component *ui );
    void ( *on_setup_canvas )( struct wp_ui_component *ui );
    void ( *on_create_ui )( struct wp_ui_component *ui );
    wp_s32 ( *on_handle_input )( struct wp_ui_component *ui, void *event );
} wp_ui_component_callbacks;

/* -------------------------------------------------------------------------
 * UI component structure
 * ---------------------------------------------------------------------- */

/**
 * @brief A UI component that extends a base game component with
 *        colour, label, canvas, input flags and ordering.
 */
typedef struct wp_ui_component
{
    wp_game_component base;

    wp_colour_f colour;
    wp_u8 ui_flags;

    wp_c8 label[WP_UI_COMPONENT_MAX_LABEL];
    wp_s32 order;

    struct wp_ui_component *canvas;

    void *element;
    void *element_listener;

    wp_ui_component_callbacks ui_callbacks;

    void *ui_user_data;
} wp_ui_component;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_ui_component_init( wp_ui_component *ui );
void wp_ui_component_destroy( wp_ui_component *ui );

/* ---- Colour ----------------------------------------------------------- */

wp_colour_f wp_ui_component_get_colour( const wp_ui_component *ui );
void wp_ui_component_set_colour( wp_ui_component *ui, wp_colour_f colour );

/* ---- UI flags --------------------------------------------------------- */

wp_u8 wp_ui_component_get_ui_flags( const wp_ui_component *ui );
void wp_ui_component_set_ui_flags( wp_ui_component *ui, wp_u8 flags );
wp_s32 wp_ui_component_get_ui_flag( const wp_ui_component *ui, wp_u8 flag );
void wp_ui_component_set_ui_flag( wp_ui_component *ui, wp_u8 flag, wp_s32 value );

/* ---- Cascade input ---------------------------------------------------- */

wp_s32 wp_ui_component_get_cascade_input( const wp_ui_component *ui );
void wp_ui_component_set_cascade_input( wp_ui_component *ui, wp_s32 value );

/* ---- Handle input events ---------------------------------------------- */

wp_s32 wp_ui_component_get_handle_input_events( const wp_ui_component *ui );
void wp_ui_component_set_handle_input_events( wp_ui_component *ui, wp_s32 value );

/* ---- Show label ------------------------------------------------------- */

wp_s32 wp_ui_component_get_show_label( const wp_ui_component *ui );
void wp_ui_component_set_show_label( wp_ui_component *ui, wp_s32 value );

/* ---- Auto calculate order --------------------------------------------- */

wp_s32 wp_ui_component_get_auto_calculate_order( const wp_ui_component *ui );
void wp_ui_component_set_auto_calculate_order( wp_ui_component *ui, wp_s32 value );

/* ---- Label ------------------------------------------------------------ */

const wp_c8 *wp_ui_component_get_label( const wp_ui_component *ui );
void wp_ui_component_set_label( wp_ui_component *ui, const wp_c8 *label );

/* ---- Order ------------------------------------------------------------ */

wp_s32 wp_ui_component_get_order( const wp_ui_component *ui );
void wp_ui_component_set_order( wp_ui_component *ui, wp_s32 order );

/* ---- Canvas ----------------------------------------------------------- */

wp_ui_component *wp_ui_component_get_canvas( const wp_ui_component *ui );
void wp_ui_component_set_canvas( wp_ui_component *ui, wp_ui_component *canvas );

/* ---- Element ---------------------------------------------------------- */

void *wp_ui_component_get_element( const wp_ui_component *ui );
void wp_ui_component_set_element( wp_ui_component *ui, void *element );

/* ---- Element listener ------------------------------------------------- */

void *wp_ui_component_get_element_listener( const wp_ui_component *ui );
void wp_ui_component_set_element_listener( wp_ui_component *ui, void *listener );

/* ---- Updates ---------------------------------------------------------- */

void wp_ui_component_update_dimensions( wp_ui_component *ui );
void wp_ui_component_update_materials( wp_ui_component *ui );
void wp_ui_component_update_transform( wp_ui_component *ui );
void wp_ui_component_update_visibility( wp_ui_component *ui );
void wp_ui_component_update_order( wp_ui_component *ui );
void wp_ui_component_update_flags( wp_ui_component *ui, wp_u32 old_flags );
void wp_ui_component_update_colour( wp_ui_component *ui );
void wp_ui_component_update_element_state( wp_ui_component *ui );
void wp_ui_component_setup_canvas( wp_ui_component *ui );
void wp_ui_component_create_ui( wp_ui_component *ui );

/* ---- Input ------------------------------------------------------------ */

wp_s32 wp_ui_component_handle_input( wp_ui_component *ui, void *event );

/* ---- Callbacks -------------------------------------------------------- */

void wp_ui_component_set_callbacks( wp_ui_component *ui, wp_ui_component_callbacks cbs );

/* ---- User data -------------------------------------------------------- */

void *wp_ui_component_get_user_data( const wp_ui_component *ui );
void wp_ui_component_set_user_data( wp_ui_component *ui, void *data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_COMPONENT_UI_H */
