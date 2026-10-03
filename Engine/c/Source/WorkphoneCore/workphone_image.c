#include "workphone_image.h"
#include "workphone_context.h"
#include "workphone_math.h"
#include "workphone_widget.h"

struct wp_image wp_subimage_ptr( void *ptr, wp_u16 w, wp_u16 h, struct wp_rect r )
{
    struct wp_image s;
    wp_zero( &s, sizeof( s ) );
    s.handle.ptr = ptr;
    s.w = w;
    s.h = h;
    s.region[0] = (wp_u16)r.x;
    s.region[1] = (wp_u16)r.y;
    s.region[2] = (wp_u16)r.w;
    s.region[3] = (wp_u16)r.h;
    return s;
}

struct wp_image wp_subimage_id( wp_s32 id, wp_u16 w, wp_u16 h, struct wp_rect r )
{
    struct wp_image s;
    wp_zero( &s, sizeof( s ) );
    s.handle.id = id;
    s.w = w;
    s.h = h;
    s.region[0] = (wp_u16)r.x;
    s.region[1] = (wp_u16)r.y;
    s.region[2] = (wp_u16)r.w;
    s.region[3] = (wp_u16)r.h;
    return s;
}

struct wp_image wp_subimage_handle( wp_handle handle, wp_u16 w, wp_u16 h, struct wp_rect r )
{
    struct wp_image s;
    wp_zero( &s, sizeof( s ) );
    s.handle = handle;
    s.w = w;
    s.h = h;
    s.region[0] = (wp_u16)r.x;
    s.region[1] = (wp_u16)r.y;
    s.region[2] = (wp_u16)r.w;
    s.region[3] = (wp_u16)r.h;
    return s;
}

struct wp_image wp_image_handle( wp_handle handle )
{
    struct wp_image s;
    wp_zero( &s, sizeof( s ) );
    s.handle = handle;
    s.w = 0;
    s.h = 0;
    s.region[0] = 0;
    s.region[1] = 0;
    s.region[2] = 0;
    s.region[3] = 0;
    return s;
}

struct wp_image wp_image_ptr( void *ptr )
{
    struct wp_image s;
    wp_zero( &s, sizeof( s ) );
    s.handle.ptr = ptr;
    s.w = 0;
    s.h = 0;
    s.region[0] = 0;
    s.region[1] = 0;
    s.region[2] = 0;
    s.region[3] = 0;
    return s;
}

struct wp_image wp_image_id( wp_s32 id )
{
    struct wp_image s;
    wp_zero( &s, sizeof( s ) );
    s.handle.id = id;
    s.w = 0;
    s.h = 0;
    s.region[0] = 0;
    s.region[1] = 0;
    s.region[2] = 0;
    s.region[3] = 0;
    return s;
}
wp_bool wp_image_is_subimage( const struct wp_image *img )
{
    WORKPHONE_ASSERT( img );
    return !( img->w == 0 && img->h == 0 );
}
void wp_image( struct wp_context *ctx, struct wp_image img )
{
    struct wp_window *win;
    struct wp_rect bounds;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    if( !wp_widget( &bounds, ctx ) )
        return;
    wp_draw_image( &win->buffer, bounds, &img, wp_white );
}
void wp_image_color( struct wp_context *ctx, struct wp_image img, struct wp_color col )
{
    struct wp_window *win;
    struct wp_rect bounds;

    WORKPHONE_ASSERT( ctx );
    WORKPHONE_ASSERT( ctx->current );
    WORKPHONE_ASSERT( ctx->current->layout );
    if( !ctx || !ctx->current || !ctx->current->layout )
        return;

    win = ctx->current;
    if( !wp_widget( &bounds, ctx ) )
        return;
    wp_draw_image( &win->buffer, bounds, &img, col );
}
