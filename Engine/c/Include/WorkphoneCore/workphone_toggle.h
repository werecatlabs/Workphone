
#ifndef workphone_toggle_h__
#define workphone_toggle_h__

#include "workphone_prerequisites.h"

#ifdef __cplusplus
extern "C" {
#endif

/* === Checkbox (stateless read — returns new state, does not write back) === */

/** Returns wp_true if the checkbox is active after this frame. Does not write back. */
WORKPHONE_API wp_bool wp_check_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                     wp_bool active );
WORKPHONE_API wp_bool wp_check_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                           wp_bool active, wp_flags widget_alignment,
                                           wp_flags text_alignment );
WORKPHONE_API wp_u32 wp_check_flags_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                          wp_u32 flags, wp_u32 value );
WORKPHONE_API wp_bool wp_check_label( struct wp_context *ctx, const wp_c8 *label, wp_bool active );
WORKPHONE_API wp_u32 wp_check_flags_label( struct wp_context *ctx, const wp_c8 *label, wp_u32 flags,
                                           wp_u32 value );

/* === Checkbox (stateful — writes back through pointer, returns wp_true if changed) === */

WORKPHONE_API wp_bool wp_checkbox_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                        wp_bool *active );
WORKPHONE_API wp_bool wp_checkbox_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                              wp_bool *active, wp_flags widget_alignment,
                                              wp_flags text_alignment );
WORKPHONE_API wp_bool wp_checkbox_flags_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                              wp_u32 *flags, wp_u32 value );
WORKPHONE_API wp_bool wp_checkbox_label( struct wp_context *ctx, const wp_c8 *label, wp_bool *active );
WORKPHONE_API wp_bool wp_checkbox_label_align( struct wp_context *ctx, const wp_c8 *label,
                                               wp_bool *active, wp_flags widget_alignment,
                                               wp_flags text_alignment );
WORKPHONE_API wp_bool wp_checkbox_flags_label( struct wp_context *ctx, const wp_c8 *label, wp_u32 *flags,
                                               wp_u32 value );

/* === Radio / Option button === */

/** Stateless — returns new active state. */
WORKPHONE_API wp_bool wp_option_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                      wp_bool is_active );
WORKPHONE_API wp_bool wp_option_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                            wp_bool is_active, wp_flags widget_alignment,
                                            wp_flags text_alignment );
WORKPHONE_API wp_bool wp_option_label( struct wp_context *ctx, const wp_c8 *label, wp_bool active );
WORKPHONE_API wp_bool wp_option_label_align( struct wp_context *ctx, const wp_c8 *label, wp_bool active,
                                             wp_flags widget_alignment, wp_flags text_alignment );

/** Stateful — writes back through pointer, returns wp_true if changed. */
WORKPHONE_API wp_bool wp_radio_text( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                     wp_bool *active );
WORKPHONE_API wp_bool wp_radio_text_align( struct wp_context *ctx, const wp_c8 *text, wp_s32 len,
                                           wp_bool *active, wp_flags widget_alignment,
                                           wp_flags text_alignment );
WORKPHONE_API wp_bool wp_radio_label( struct wp_context *ctx, const wp_c8 *label, wp_bool *active );
WORKPHONE_API wp_bool wp_radio_label_align( struct wp_context *ctx, const wp_c8 *label, wp_bool *active,
                                            wp_flags widget_alignment, wp_flags text_alignment );

#ifdef __cplusplus
}
#endif

#endif  // workphone_toggle_h__
