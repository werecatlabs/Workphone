#ifndef workphone_page_element_h__
#define workphone_page_element_h__

#include "workphone_prerequisites.h"

/* page-element */
WORKPHONE_API struct wp_page_element *wp_create_page_element( struct wp_context *ctx );
WORKPHONE_API void wp_liwp_page_element_into_freelist( struct wp_context *ctx,
                                                       struct wp_page_element *elem );
WORKPHONE_API void wp_free_page_element( struct wp_context *ctx, struct wp_page_element *elem );

#endif  // workphone_page_element_h__
