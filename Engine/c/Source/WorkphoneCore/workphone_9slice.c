#include "workphone.h"
#include "workphone_prerequisites.h"

struct wp_nine_slice wp_sub9slice_ptr( void *ptr, wp_u16 w, wp_u16 h, struct wp_rect rgn, wp_u16 l,
                                       wp_u16 t, wp_u16 r, wp_u16 b )
{
    struct wp_nine_slice s;
    struct wp_image *i = &s.img;
    wp_zero( &s, sizeof( s ) );
    i->handle.ptr = ptr;
    i->w = w;
    i->h = h;
    i->region[0] = (wp_u16)rgn.x;
    i->region[1] = (wp_u16)rgn.y;
    i->region[2] = (wp_u16)rgn.w;
    i->region[3] = (wp_u16)rgn.h;
    s.l = l;
    s.t = t;
    s.r = r;
    s.b = b;
    return s;
}

struct wp_nine_slice wp_sub9slice_id( wp_s32 id, wp_u16 w, wp_u16 h, struct wp_rect rgn, wp_u16 l,
                                      wp_u16 t, wp_u16 r, wp_u16 b )
{
    struct wp_nine_slice s;
    struct wp_image *i = &s.img;
    wp_zero( &s, sizeof( s ) );
    i->handle.id = id;
    i->w = w;
    i->h = h;
    i->region[0] = (wp_u16)rgn.x;
    i->region[1] = (wp_u16)rgn.y;
    i->region[2] = (wp_u16)rgn.w;
    i->region[3] = (wp_u16)rgn.h;
    s.l = l;
    s.t = t;
    s.r = r;
    s.b = b;
    return s;
}

struct wp_nine_slice wp_sub9slice_handle( wp_handle handle, wp_u16 w, wp_u16 h, struct wp_rect rgn,
                                          wp_u16 l, wp_u16 t, wp_u16 r, wp_u16 b )
{
    struct wp_nine_slice s;
    struct wp_image *i = &s.img;
    wp_zero( &s, sizeof( s ) );
    i->handle = handle;
    i->w = w;
    i->h = h;
    i->region[0] = (wp_u16)rgn.x;
    i->region[1] = (wp_u16)rgn.y;
    i->region[2] = (wp_u16)rgn.w;
    i->region[3] = (wp_u16)rgn.h;
    s.l = l;
    s.t = t;
    s.r = r;
    s.b = b;
    return s;
}

struct wp_nine_slice wp_nine_slice_handle( wp_handle handle, wp_u16 l, wp_u16 t, wp_u16 r, wp_u16 b )
{
    struct wp_nine_slice s;
    struct wp_image *i = &s.img;
    wp_zero( &s, sizeof( s ) );
    i->handle = handle;
    i->w = 0;
    i->h = 0;
    i->region[0] = 0;
    i->region[1] = 0;
    i->region[2] = 0;
    i->region[3] = 0;
    s.l = l;
    s.t = t;
    s.r = r;
    s.b = b;
    return s;
}

struct wp_nine_slice wp_nine_slice_ptr( void *ptr, wp_u16 l, wp_u16 t, wp_u16 r, wp_u16 b )
{
    struct wp_nine_slice s;
    struct wp_image *i = &s.img;
    wp_zero( &s, sizeof( s ) );
    WORKPHONE_ASSERT( ptr );
    i->handle.ptr = ptr;
    i->w = 0;
    i->h = 0;
    i->region[0] = 0;
    i->region[1] = 0;
    i->region[2] = 0;
    i->region[3] = 0;
    s.l = l;
    s.t = t;
    s.r = r;
    s.b = b;
    return s;
}

struct wp_nine_slice wp_nine_slice_id( wp_s32 id, wp_u16 l, wp_u16 t, wp_u16 r, wp_u16 b )
{
    struct wp_nine_slice s;
    struct wp_image *i = &s.img;
    wp_zero( &s, sizeof( s ) );
    i->handle.id = id;
    i->w = 0;
    i->h = 0;
    i->region[0] = 0;
    i->region[1] = 0;
    i->region[2] = 0;
    i->region[3] = 0;
    s.l = l;
    s.t = t;
    s.r = r;
    s.b = b;
    return s;
}

wp_s32 wp_nine_slice_is_sub9slice( const struct wp_nine_slice *slice )
{
    WORKPHONE_ASSERT( slice );
    return !( slice->img.w == 0 && slice->img.h == 0 );
}
