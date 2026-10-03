#ifndef workphone_image_h__
#define workphone_image_h__

#include "workphone_prerequisites.h"
#include "workphone_handle.h"

struct wp_image
{
    wp_handle handle;
    wp_u16 w, h;
    wp_u16 region[4];
};

WORKPHONE_API struct wp_image wp_image_handle( wp_handle );
WORKPHONE_API struct wp_image wp_image_ptr( void * );
WORKPHONE_API struct wp_image wp_image_id( wp_s32 );
WORKPHONE_API wp_bool wp_image_is_subimage( const struct wp_image *img );
WORKPHONE_API struct wp_image wp_subimage_ptr( void *, wp_u16 w, wp_u16 h, struct wp_rect sub_region );
WORKPHONE_API struct wp_image wp_subimage_id( int, wp_u16 w, wp_u16 h, struct wp_rect sub_region );
WORKPHONE_API struct wp_image wp_subimage_handle( wp_handle, wp_u16 w, wp_u16 h,
                                                  struct wp_rect sub_region );

#endif  // workphone_image_h__
