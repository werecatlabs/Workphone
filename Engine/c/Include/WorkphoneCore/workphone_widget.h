#ifndef workphone_widget_h__
#define workphone_widget_h__

#include "workphone_prerequisites.h"

WORKPHONE_API enum wp_widget_layout_states wp_widget( struct wp_rect *, const struct wp_context * );
WORKPHONE_API enum wp_widget_layout_states wp_widget_fitting( struct wp_rect *,
                                                              const struct wp_context *,
                                                              struct wp_vec2f );
WORKPHONE_API struct wp_rect wp_widget_bounds( const struct wp_context * );
WORKPHONE_API struct wp_vec2f wp_widget_position( const struct wp_context * );
WORKPHONE_API struct wp_vec2f wp_widget_size( const struct wp_context * );
WORKPHONE_API float wp_widget_width( const struct wp_context * );
WORKPHONE_API float wp_widget_height( const struct wp_context * );
WORKPHONE_API wp_bool wp_widget_is_hovered( const struct wp_context * );
WORKPHONE_API wp_bool wp_widget_is_mouse_clicked( const struct wp_context *, enum wp_buttons );
WORKPHONE_API wp_bool wp_widget_has_mouse_click_down( const struct wp_context *, enum wp_buttons,
                                                      wp_bool down );
WORKPHONE_API void wp_spacing( struct wp_context *, int cols );
WORKPHONE_API void wp_widget_disable_begin( struct wp_context *ctx );
WORKPHONE_API void wp_widget_disable_end( struct wp_context *ctx );

WORKPHONE_API enum wp_widget_layout_states wp_widget( struct wp_rect *, const struct wp_context * );
WORKPHONE_API enum wp_widget_layout_states wp_widget_fitting( struct wp_rect *,
                                                              const struct wp_context *,
                                                              struct wp_vec2f );
WORKPHONE_API struct wp_rect wp_widget_bounds( const struct wp_context * );
WORKPHONE_API struct wp_vec2f wp_widget_position( const struct wp_context * );
WORKPHONE_API struct wp_vec2f wp_widget_size( const struct wp_context * );
WORKPHONE_API wp_f32 wp_widget_width( const struct wp_context * );
WORKPHONE_API wp_f32 wp_widget_height( const struct wp_context * );
WORKPHONE_API wp_bool wp_widget_is_hovered( const struct wp_context * );
WORKPHONE_API wp_bool wp_widget_is_mouse_clicked( const struct wp_context *, enum wp_buttons );
WORKPHONE_API wp_bool wp_widget_has_mouse_click_down( const struct wp_context *, enum wp_buttons,
                                                      wp_bool down );
WORKPHONE_API void wp_spacing( struct wp_context *, wp_s32 cols );
WORKPHONE_API void wp_widget_disable_begin( struct wp_context *ctx );
WORKPHONE_API void wp_widget_disable_end( struct wp_context *ctx );
WORKPHONE_LIB void wp_widget_state_reset( wp_flags *state );

#endif  // workphone_widget_h__
