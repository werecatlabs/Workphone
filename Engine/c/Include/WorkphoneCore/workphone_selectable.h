#ifndef workphone_selectable_h__
#define workphone_selectable_h__

#include "workphone_prerequisites.h"

/* selectable */
WORKPHONE_LIB void wp_draw_selectable( struct wp_command_buffer *out, wp_flags state,
                                       const struct wp_style_selectable *style, wp_bool active,
                                       const struct wp_rect *bounds, const struct wp_rect *icon,
                                       const struct wp_image *img, enum wp_symbol_type sym,
                                       const wp_c8 *string, wp_s32 len, wp_flags align,
                                       const struct wp_user_font *font );
WORKPHONE_LIB wp_bool wp_do_selectable( wp_flags *state, struct wp_command_buffer *out,
                                        struct wp_rect bounds, const wp_c8 *str, wp_s32 len,
                                        wp_flags align, wp_bool *value,
                                        const struct wp_style_selectable *style,
                                        const struct wp_input *in, const struct wp_user_font *font );
WORKPHONE_LIB wp_bool wp_do_selectable_image( wp_flags *state, struct wp_command_buffer *out,
                                              struct wp_rect bounds, const wp_c8 *str, wp_s32 len,
                                              wp_flags align, wp_bool *value, const struct wp_image *img,
                                              const struct wp_style_selectable *style,
                                              const struct wp_input *in,
                                              const struct wp_user_font *font );

#endif  // workphone_selectable_h__
