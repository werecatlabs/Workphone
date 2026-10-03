#ifndef workphone_list_view_h__
#define workphone_list_view_h__

#include "workphone_prerequisites.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wp_list_view
{
    /* public: */
    int begin, end, count;
    /* private: */
    int total_height;
    struct wp_context *ctx;
    wp_u32 *scroll_pointer;
    wp_u32 scroll_value;
};

WORKPHONE_API wp_bool wp_list_view_begin( struct wp_context *, struct wp_list_view *out, const char *id,
                                          wp_flags, int row_height, int row_count );
WORKPHONE_API void wp_list_view_end( struct wp_list_view * );

#ifdef __cplusplus
}
#endif

#endif  // workphone_list_view_h__
