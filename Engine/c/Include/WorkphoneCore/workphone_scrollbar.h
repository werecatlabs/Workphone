#ifndef workphone_scrollbar_h__
#define workphone_scrollbar_h__

#include "workphone_prerequisites.h"

/* scrollbar */
WORKPHONE_LIB wp_f32 wp_scrollbar_behavior( wp_flags *state, struct wp_input *in, wp_s32 has_scrolling,
                                            const struct wp_rect *scroll, const struct wp_rect *cursor,
                                            const struct wp_rect *empty0, const struct wp_rect *empty1,
                                            wp_f32 scroll_offset, wp_f32 target, wp_f32 scroll_step,
                                            enum wp_orientation o );
WORKPHONE_LIB void wp_draw_scrollbar( struct wp_command_buffer *out, wp_flags state,
                                      const struct wp_style_scrollbar *style,
                                      const struct wp_rect *bounds, const struct wp_rect *scroll );
WORKPHONE_LIB wp_f32 wp_do_scrollbarv( wp_flags *state, struct wp_command_buffer *out,
                                       struct wp_rect scroll, wp_s32 has_scrolling, wp_f32 offset,
                                       wp_f32 target, wp_f32 step, wp_f32 button_pixel_inc,
                                       const struct wp_style_scrollbar *style, struct wp_input *in,
                                       const struct wp_user_font *font );
WORKPHONE_LIB wp_f32 wp_do_scrollbarh( wp_flags *state, struct wp_command_buffer *out,
                                       struct wp_rect scroll, wp_s32 has_scrolling, wp_f32 offset,
                                       wp_f32 target, wp_f32 step, wp_f32 button_pixel_inc,
                                       const struct wp_style_scrollbar *style, struct wp_input *in,
                                       const struct wp_user_font *font );

#endif  // workphone_scrollbar_h__
