#ifndef workphone_pool_h__
#define workphone_pool_h__

#include "workphone_prerequisites.h"

/* pool */
WORKPHONE_LIB void wp_pool_init( struct wp_pool *pool, const struct wp_allocator *alloc,
                                 wp_u32 capacity );
WORKPHONE_LIB void wp_pool_free( struct wp_pool *pool );
WORKPHONE_LIB void wp_pool_init_fixed( struct wp_pool *pool, void *memory, wp_size size );
WORKPHONE_LIB struct wp_page_element *wp_pool_alloc( struct wp_pool *pool );

#endif  // workphone_pool_h__
