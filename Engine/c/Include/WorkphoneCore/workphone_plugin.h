#ifndef workphone_plugin_h__
#define workphone_plugin_h__

#include "workphone_prerequisites.h"
#include "workphone_handle.h"

typedef void *( *wp_plugin_alloc )( wp_handle, void *old, wp_size );
typedef void ( *wp_plugin_free )( wp_handle, void *old );
typedef wp_bool ( *wp_plugin_filter )( const struct wp_text_edit *, wp_rune unicode );
typedef void ( *wp_plugin_paste )( wp_handle, struct wp_text_edit * );
typedef void ( *wp_plugin_copy )( wp_handle, const wp_c8 *, wp_s32 len );

#endif  // workphone_plugin_h__
