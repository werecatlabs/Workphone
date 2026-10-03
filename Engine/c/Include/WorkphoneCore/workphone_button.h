#ifndef workphone_button_h__
#define workphone_button_h__

#include "workphone_prerequisites.h"

/* button */
WORKPHONE_API wp_bool wp_button_behavior( wp_flags *state, struct wp_rect r, const struct wp_input *i,
                                          wp_button_behavior_enum behavior );
WORKPHONE_API const struct wp_style_item *wp_draw_button( struct wp_command_buffer *out,
                                                          const struct wp_rect *bounds, wp_flags state,
                                                          const struct wp_style_button *style );
WORKPHONE_API wp_bool wp_do_button( wp_flags *state, struct wp_command_buffer *out, struct wp_rect r,
                                    const struct wp_style_button *style, const struct wp_input *in,
                                    wp_button_behavior_enum behavior, struct wp_rect *content );
WORKPHONE_API void wp_draw_button_text( struct wp_command_buffer *out, const struct wp_rect *bounds,
                                        const struct wp_rect *content, wp_flags state,
                                        const struct wp_style_button *style, const wp_c8 *txt,
                                        wp_s32 len, wp_flags text_alignment,
                                        const struct wp_user_font *font );
WORKPHONE_API wp_bool wp_do_button_text( wp_flags *state, struct wp_command_buffer *out,
                                         struct wp_rect bounds, const wp_c8 *string, wp_s32 len,
                                         wp_flags align, wp_button_behavior_enum behavior,
                                         const struct wp_style_button *style, const struct wp_input *in,
                                         const struct wp_user_font *font );
WORKPHONE_API void wp_draw_button_symbol( struct wp_command_buffer *out, const struct wp_rect *bounds,
                                          const struct wp_rect *content, wp_flags state,
                                          const struct wp_style_button *style, enum wp_symbol_type type,
                                          const struct wp_user_font *font );
WORKPHONE_API wp_bool wp_do_button_symbol( wp_flags *state, struct wp_command_buffer *out,
                                           struct wp_rect bounds, wp_symbol_type symbol,
                                           wp_button_behavior_enum behavior,
                                           const struct wp_style_button *style,
                                           const struct wp_input *in, const struct wp_user_font *font );
WORKPHONE_API void wp_draw_button_image( struct wp_command_buffer *out, const struct wp_rect *bounds,
                                         const struct wp_rect *content, wp_flags state,
                                         const struct wp_style_button *style,
                                         const struct wp_image *img );
WORKPHONE_API wp_bool wp_do_button_image( wp_flags *state, struct wp_command_buffer *out,
                                          struct wp_rect bounds, struct wp_image img,
                                          wp_button_behavior_enum behavior,
                                          const struct wp_style_button *style,
                                          const struct wp_input *in );
WORKPHONE_API void wp_draw_button_text_symbol( struct wp_command_buffer *out,
                                               const struct wp_rect *bounds, const struct wp_rect *label,
                                               const struct wp_rect *symbol, wp_flags state,
                                               const struct wp_style_button *style, const wp_c8 *str,
                                               wp_s32 len, wp_symbol_type type,
                                               const struct wp_user_font *font );
WORKPHONE_API wp_bool wp_do_button_text_symbol(
    wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds, wp_symbol_type symbol,
    const wp_c8 *str, wp_s32 len, wp_flags align, wp_button_behavior_enum behavior,
    const struct wp_style_button *style, const struct wp_user_font *font, const struct wp_input *in );
WORKPHONE_API void wp_draw_button_text_image( struct wp_command_buffer *out,
                                              const struct wp_rect *bounds, const struct wp_rect *label,
                                              const struct wp_rect *image, wp_flags state,
                                              const struct wp_style_button *style, const wp_c8 *str,
                                              wp_s32 len, const struct wp_user_font *font,
                                              const struct wp_image *img );
WORKPHONE_API wp_bool wp_do_button_text_image(
    wp_flags *state, struct wp_command_buffer *out, struct wp_rect bounds, struct wp_image img,
    const wp_c8 *str, wp_s32 len, wp_flags align, wp_button_behavior_enum behavior,
    const struct wp_style_button *style, const struct wp_user_font *font, const struct wp_input *in );

WORKPHONE_API wp_bool wp_button_text( struct wp_context *, const char *title, int len );
WORKPHONE_API wp_bool wp_button_label( struct wp_context *, const char *title );
WORKPHONE_API wp_bool wp_button_color( struct wp_context *, struct wp_color );
WORKPHONE_API wp_bool wp_button_symbol( struct wp_context *, wp_symbol_type );
WORKPHONE_API wp_bool wp_button_image( struct wp_context *, struct wp_image img );
WORKPHONE_API wp_bool wp_button_symbol_label( struct wp_context *, wp_symbol_type, const char *,
                                              wp_flags text_alignment );
WORKPHONE_API wp_bool wp_button_symbol_text( struct wp_context *, wp_symbol_type, const char *, int,
                                             wp_flags alignment );
WORKPHONE_API wp_bool wp_button_image_label( struct wp_context *, struct wp_image img, const char *,
                                             wp_flags text_alignment );
WORKPHONE_API wp_bool wp_button_image_text( struct wp_context *, struct wp_image img, const char *, int,
                                            wp_flags alignment );
WORKPHONE_API wp_bool wp_button_text_styled( struct wp_context *, const struct wp_style_button *,
                                             const char *title, int len );
WORKPHONE_API wp_bool wp_button_label_styled( struct wp_context *, const struct wp_style_button *,
                                              const char *title );
WORKPHONE_API wp_bool wp_button_symbol_styled( struct wp_context *, const struct wp_style_button *,
                                               wp_symbol_type );
WORKPHONE_API wp_bool wp_button_image_styled( struct wp_context *, const struct wp_style_button *,
                                              struct wp_image img );
WORKPHONE_API wp_bool wp_button_symbol_text_styled( struct wp_context *, const struct wp_style_button *,
                                                    wp_symbol_type, const char *, int,
                                                    wp_flags alignment );
WORKPHONE_API wp_bool wp_button_symbol_label_styled( struct wp_context *ctx,
                                                     const struct wp_style_button *style,
                                                     wp_symbol_type symbol, const char *title,
                                                     wp_flags align );
WORKPHONE_API wp_bool wp_button_image_label_styled( struct wp_context *, const struct wp_style_button *,
                                                    struct wp_image img, const char *,
                                                    wp_flags text_alignment );
WORKPHONE_API wp_bool wp_button_image_text_styled( struct wp_context *, const struct wp_style_button *,
                                                   struct wp_image img, const char *, int,
                                                   wp_flags alignment );
WORKPHONE_API void wp_button_set_behavior( struct wp_context *, wp_button_behavior_enum );
WORKPHONE_API wp_bool wp_button_push_behavior( struct wp_context *, wp_button_behavior_enum );
WORKPHONE_API wp_bool wp_button_pop_behavior( struct wp_context * );

#endif  // workphone_button_h__
