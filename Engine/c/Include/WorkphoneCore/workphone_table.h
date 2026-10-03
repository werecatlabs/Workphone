#ifndef workphone_table_h__
#define workphone_table_h__

#include "workphone_prerequisites.h"

/* table */
WORKPHONE_API struct wp_table *wp_create_table( struct wp_context *ctx );
WORKPHONE_API void wp_remove_table( struct wp_window *win, struct wp_table *tbl );
WORKPHONE_API void wp_free_table( struct wp_context *ctx, struct wp_table *tbl );
WORKPHONE_API void wp_push_table( struct wp_window *win, struct wp_table *tbl );
WORKPHONE_API wp_u32 *wp_add_value( struct wp_context *ctx, struct wp_window *win, wp_hash name,
                                    wp_u32 value );
WORKPHONE_API wp_u32 *wp_find_value( const struct wp_window *win, wp_hash name );

#endif  // workphone_table_h__
