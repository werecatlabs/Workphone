#ifndef workphone_edit_h__
#define workphone_edit_h__

#include "workphone_prerequisites.h"
#include "workphone_plugin.h"

/* edit */
WORKPHONE_LIB void wp_edit_draw_text( struct wp_command_buffer *out, const struct wp_style_edit *style,
                                      wp_f32 pos_x, wp_f32 pos_y, wp_f32 x_offset, const wp_c8 *text,
                                      wp_s32 byte_len, wp_f32 row_height,
                                      const struct wp_user_font *font, struct wp_color background,
                                      struct wp_color foreground, wp_bool is_selected );
WORKPHONE_LIB wp_flags wp_do_edit( wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds,
                                   wp_flags flags, wp_plugin_filter filter, struct wp_text_edit *edit,
                                   const struct wp_style_edit *style, struct wp_input *in,
                                   const struct wp_user_font *font );

WORKPHONE_API wp_flags wp_edit_string( struct wp_context *, wp_flags, char *buffer, int *len, int max,
                                       wp_plugin_filter );
WORKPHONE_API wp_flags wp_edit_string_zero_terminated( struct wp_context *, wp_flags, char *buffer,
                                                       int max, wp_plugin_filter );
WORKPHONE_API wp_flags wp_edit_buffer( struct wp_context *, wp_flags, struct wp_text_edit *,
                                       wp_plugin_filter );
WORKPHONE_API void wp_edit_focus( struct wp_context *, wp_flags flags );
WORKPHONE_API void wp_edit_unfocus( struct wp_context * );

#endif  // workphone_edit_h__
