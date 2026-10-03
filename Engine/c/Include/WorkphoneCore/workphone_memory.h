#ifndef workphone_memory_h__
#define workphone_memory_h__

#include "workphone_prerequisites.h"
#include "workphone_plugin.h"

struct wp_memory_status
{
    void *memory;
    unsigned int type;
    wp_size size;
    wp_size allocated;
    wp_size needed;
    wp_size calls;
};

struct wp_buffer_marker
{
    wp_bool active;
    wp_size offset;
};

struct wp_memory
{
    void *ptr;
    wp_size size;
};

struct wp_allocator
{
    wp_handle userdata;
    wp_plugin_alloc alloc;
    wp_plugin_free free;
};

struct wp_buffer
{
    struct wp_buffer_marker
        marker[WORKPHONE_BUFFER_MAX]; /**!< buffer marker to free a buffer to a certain offset */
    struct wp_allocator pool;         /**!< allocator callback for dynamic buffers */
    enum wp_allocation_type type;     /**!< memory management type */
    struct wp_memory memory;          /**!< memory and size of the current memory block */
    wp_f32 grow_factor;               /**!< growing factor for dynamic memory management */
    wp_size allocated;                /**!< total amount of memory allocated */
    wp_size needed; /**!< totally consumed memory given that enough memory is present */
    wp_size calls;  /**!< number of allocation calls */
    wp_size size;   /**!< current size of the buffer */
};

struct wp_pool
{
    struct wp_allocator alloc;
    enum wp_allocation_type type;
    unsigned int page_count;
    struct wp_page *pages;
    unsigned int capacity;
    wp_size size;
};

#ifdef WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
WORKPHONE_API void wp_buffer_init_default( struct wp_buffer * );
#endif

WORKPHONE_API void wp_buffer_init( struct wp_buffer *, const struct wp_allocator *, wp_size size );
WORKPHONE_API void wp_buffer_init_fixed( struct wp_buffer *, void *memory, wp_size size );
WORKPHONE_API void wp_buffer_info( struct wp_memory_status *, const struct wp_buffer * );
WORKPHONE_API void wp_buffer_push( struct wp_buffer *, enum wp_buffer_allocation_type type,
                                   const void *memory, wp_size size, wp_size align );
WORKPHONE_API void wp_buffer_mark( struct wp_buffer *, enum wp_buffer_allocation_type type );
WORKPHONE_API void wp_buffer_reset( struct wp_buffer *, enum wp_buffer_allocation_type type );
WORKPHONE_API void wp_buffer_clear( struct wp_buffer * );
WORKPHONE_API void wp_buffer_free( struct wp_buffer * );
WORKPHONE_API void *wp_buffer_memory( struct wp_buffer * );
WORKPHONE_API const void *wp_buffer_memory_const( const struct wp_buffer * );
WORKPHONE_API wp_size wp_buffer_total( const struct wp_buffer * );

#endif  // workphone_memory_h__
