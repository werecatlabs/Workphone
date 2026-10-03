/**
 * @file workphone_game_component_ui.c
 * @brief Implementation of the C UI component API.
 */

#include "workphone_game_component_ui.h"

#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_colour_f wp_colour_f_white( void )
{
    wp_colour_f c;
    c.r = 1.0f;
    c.g = 1.0f;
    c.b = 1.0f;
    c.a = 1.0f;
    return c;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_ui_component_init( wp_ui_component *ui )
{
    memset( ui, 0, sizeof( *ui ) );
    wp_game_component_init_with_type( &ui->base, WP_COMPONENT_TYPE_UI );

    ui->colour = wp_colour_f_white();
    ui->ui_flags = WP_UI_FLAG_CASCADE_INPUT | WP_UI_FLAG_AUTO_CALCULATE_ORDER;
    ui->label[0] = '\0';
    ui->order = 0;
    ui->canvas = NULL;
    ui->element = NULL;
    ui->element_listener = NULL;
    memset( &ui->ui_callbacks, 0, sizeof( ui->ui_callbacks ) );
    ui->ui_user_data = NULL;
}

void wp_ui_component_destroy( wp_ui_component *ui )
{
    ui->canvas = NULL;
    ui->element = NULL;
    ui->element_listener = NULL;
    ui->ui_user_data = NULL;
    ui->ui_flags = 0;
    memset( &ui->ui_callbacks, 0, sizeof( ui->ui_callbacks ) );

    wp_game_component_destroy( &ui->base );
}

/* =========================================================================
 * Colour
 * ====================================================================== */

wp_colour_f wp_ui_component_get_colour( const wp_ui_component *ui )
{
    return ui->colour;
}

void wp_ui_component_set_colour( wp_ui_component *ui, wp_colour_f colour )
{
    ui->colour = colour;
}

/* =========================================================================
 * UI flags
 * ====================================================================== */

wp_u8 wp_ui_component_get_ui_flags( const wp_ui_component *ui )
{
    return ui->ui_flags;
}

void wp_ui_component_set_ui_flags( wp_ui_component *ui, wp_u8 flags )
{
    ui->ui_flags = flags;
}

wp_s32 wp_ui_component_get_ui_flag( const wp_ui_component *ui, wp_u8 flag )
{
    return ( ui->ui_flags & flag ) != 0;
}

void wp_ui_component_set_ui_flag( wp_ui_component *ui, wp_u8 flag, wp_s32 value )
{
    if( value )
        ui->ui_flags |= flag;
    else
        ui->ui_flags &= (wp_u8)~flag;
}

/* =========================================================================
 * Cascade input
 * ====================================================================== */

wp_s32 wp_ui_component_get_cascade_input( const wp_ui_component *ui )
{
    return wp_ui_component_get_ui_flag( ui, WP_UI_FLAG_CASCADE_INPUT );
}

void wp_ui_component_set_cascade_input( wp_ui_component *ui, wp_s32 value )
{
    wp_ui_component_set_ui_flag( ui, WP_UI_FLAG_CASCADE_INPUT, value );
}

/* =========================================================================
 * Handle input events
 * ====================================================================== */

wp_s32 wp_ui_component_get_handle_input_events( const wp_ui_component *ui )
{
    return wp_ui_component_get_ui_flag( ui, WP_UI_FLAG_HANDLE_INPUT_EVENTS );
}

void wp_ui_component_set_handle_input_events( wp_ui_component *ui, wp_s32 value )
{
    wp_ui_component_set_ui_flag( ui, WP_UI_FLAG_HANDLE_INPUT_EVENTS, value );
}

/* =========================================================================
 * Show label
 * ====================================================================== */

wp_s32 wp_ui_component_get_show_label( const wp_ui_component *ui )
{
    return wp_ui_component_get_ui_flag( ui, WP_UI_FLAG_SHOW_LABEL );
}

void wp_ui_component_set_show_label( wp_ui_component *ui, wp_s32 value )
{
    wp_ui_component_set_ui_flag( ui, WP_UI_FLAG_SHOW_LABEL, value );
}

/* =========================================================================
 * Auto calculate order
 * ====================================================================== */

wp_s32 wp_ui_component_get_auto_calculate_order( const wp_ui_component *ui )
{
    return wp_ui_component_get_ui_flag( ui, WP_UI_FLAG_AUTO_CALCULATE_ORDER );
}

void wp_ui_component_set_auto_calculate_order( wp_ui_component *ui, wp_s32 value )
{
    wp_ui_component_set_ui_flag( ui, WP_UI_FLAG_AUTO_CALCULATE_ORDER, value );
}

/* =========================================================================
 * Label
 * ====================================================================== */

const wp_c8 *wp_ui_component_get_label( const wp_ui_component *ui )
{
    return ui->label;
}

void wp_ui_component_set_label( wp_ui_component *ui, const wp_c8 *label )
{
    if( label )
    {
        strncpy( ui->label, label, WP_UI_COMPONENT_MAX_LABEL - 1 );
        ui->label[WP_UI_COMPONENT_MAX_LABEL - 1] = '\0';
    }
    else
    {
        ui->label[0] = '\0';
    }
}

/* =========================================================================
 * Order
 * ====================================================================== */

wp_s32 wp_ui_component_get_order( const wp_ui_component *ui )
{
    return ui->order;
}

void wp_ui_component_set_order( wp_ui_component *ui, wp_s32 order )
{
    ui->order = order;
}

/* =========================================================================
 * Canvas
 * ====================================================================== */

wp_ui_component *wp_ui_component_get_canvas( const wp_ui_component *ui )
{
    return ui->canvas;
}

void wp_ui_component_set_canvas( wp_ui_component *ui, wp_ui_component *canvas )
{
    ui->canvas = canvas;
}

/* =========================================================================
 * Element
 * ====================================================================== */

void *wp_ui_component_get_element( const wp_ui_component *ui )
{
    return ui->element;
}

void wp_ui_component_set_element( wp_ui_component *ui, void *element )
{
    ui->element = element;
}

/* =========================================================================
 * Element listener
 * ====================================================================== */

void *wp_ui_component_get_element_listener( const wp_ui_component *ui )
{
    return ui->element_listener;
}

void wp_ui_component_set_element_listener( wp_ui_component *ui, void *listener )
{
    ui->element_listener = listener;
}

/* =========================================================================
 * Updates
 * ====================================================================== */

void wp_ui_component_update_dimensions( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_update_dimensions )
        ui->ui_callbacks.on_update_dimensions( ui );
}

void wp_ui_component_update_materials( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_update_materials )
        ui->ui_callbacks.on_update_materials( ui );
}

void wp_ui_component_update_transform( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_update_transform )
        ui->ui_callbacks.on_update_transform( ui );
}

void wp_ui_component_update_visibility( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_update_visibility )
        ui->ui_callbacks.on_update_visibility( ui );
}

void wp_ui_component_update_order( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_update_order )
        ui->ui_callbacks.on_update_order( ui );
}

void wp_ui_component_update_flags( wp_ui_component *ui, wp_u32 old_flags )
{
    if( ui->ui_callbacks.on_update_flags )
        ui->ui_callbacks.on_update_flags( ui, old_flags );
}

void wp_ui_component_update_colour( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_update_colour )
        ui->ui_callbacks.on_update_colour( ui );
}

void wp_ui_component_update_element_state( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_update_element_state )
        ui->ui_callbacks.on_update_element_state( ui );
}

void wp_ui_component_setup_canvas( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_setup_canvas )
        ui->ui_callbacks.on_setup_canvas( ui );
}

void wp_ui_component_create_ui( wp_ui_component *ui )
{
    if( ui->ui_callbacks.on_create_ui )
        ui->ui_callbacks.on_create_ui( ui );
}

/* =========================================================================
 * Input
 * ====================================================================== */

wp_s32 wp_ui_component_handle_input( wp_ui_component *ui, void *event )
{
    if( !wp_ui_component_get_handle_input_events( ui ) )
        return 0;

    if( ui->ui_callbacks.on_handle_input )
        return ui->ui_callbacks.on_handle_input( ui, event );

    return 0;
}

/* =========================================================================
 * Callbacks
 * ====================================================================== */

void wp_ui_component_set_callbacks( wp_ui_component *ui, wp_ui_component_callbacks cbs )
{
    ui->ui_callbacks = cbs;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_ui_component_get_user_data( const wp_ui_component *ui )
{
    return ui->ui_user_data;
}

void wp_ui_component_set_user_data( wp_ui_component *ui, void *data )
{
    ui->ui_user_data = data;
}
