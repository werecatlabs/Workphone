#ifndef workphone_popup_h__
#define workphone_popup_h__

#include "workphone_prerequisites.h"

WORKPHONE_API wp_bool wp_popup_begin( struct wp_context *, enum wp_popup_type, const wp_c8 *, wp_flags,
                                      struct wp_rect bounds );
WORKPHONE_API void wp_popup_close( struct wp_context * );
WORKPHONE_API void wp_popup_end( struct wp_context * );
WORKPHONE_API void wp_popup_get_scroll( const struct wp_context *, wp_u32 *offset_x, wp_u32 *offset_y );
WORKPHONE_API void wp_popup_set_scroll( struct wp_context *, wp_u32 offset_x, wp_u32 offset_y );

/* popup */
WORKPHONE_LIB wp_bool wp_nonblock_begin( struct wp_context *ctx, wp_flags flags, struct wp_rect body,
                                         struct wp_rect header, enum wp_panel_type panel_type );

#endif  // workphone_popup_h__
