#include "workphone_draw.h"
#include "workphone_command_buffer.h"
#include "workphone_font.h"
#include "workphone_ui.h"

#pragma warning( push )
#pragma warning( disable : 4116 )

void wp_command_buffer_init( struct wp_command_buffer *cb, struct wp_buffer *b,
                             enum wp_command_clipping clip )
{
    WORKPHONE_ASSERT( cb );
    WORKPHONE_ASSERT( b );

    if( !cb || !b )
        return;

    cb->base = b;
    cb->use_clipping = (wp_s32)clip;
    cb->begin = b->allocated;
    cb->end = b->allocated;
    cb->last = b->allocated;
}

void wp_command_buffer_reset( struct wp_command_buffer *b )
{
    WORKPHONE_ASSERT( b );
    if( !b )
        return;
    b->begin = 0;
    b->end = 0;
    b->last = 0;
    b->clip = wp_null_rect;
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    b->userdata.ptr = 0;
#endif
}

void *wp_command_buffer_push( struct wp_command_buffer *b, enum wp_command_type t, wp_size size )
{
    WORKPHONE_STORAGE const wp_size align = WORKPHONE_ALIGNOF( struct wp_command );
    struct wp_command *cmd;
    wp_size alignment;
    void *unaligned;
    void *memory;

    WORKPHONE_ASSERT( b );
    WORKPHONE_ASSERT( b->base );
    if( !b )
        return 0;
    cmd = (struct wp_command *)wp_buffer_alloc( b->base, WORKPHONE_BUFFER_FRONT, size, align );
    if( !cmd )
        return 0;

    /* make sure the offset to the next command is aligned */
    b->last = (wp_size)( (wp_byte *)cmd - (wp_byte *)b->base->memory.ptr );
    unaligned = (wp_byte *)cmd + size;
    memory = WORKPHONE_ALIGN_PTR( unaligned, align );
    alignment = (wp_size)( (wp_byte *)memory - (wp_byte *)unaligned );
#ifdef WORKPHONE_ZERO_COMMAND_MEMORY
    WORKPHONE_MEMSET( cmd, 0, size + alignment );
#endif

    cmd->type = t;
    cmd->next = b->base->allocated + alignment;
#ifdef WORKPHONE_INCLUDE_COMMAND_USERDATA
    cmd->userdata = b->userdata;
#endif
    b->end = cmd->next;
    return cmd;
}

void wp_push_scissor( struct wp_command_buffer *b, struct wp_rect r )
{
    struct wp_command_scissor *cmd;
    WORKPHONE_ASSERT( b );
    if( !b )
        return;

    b->clip.x = r.x;
    b->clip.y = r.y;
    b->clip.w = r.w;
    b->clip.h = r.h;
    cmd = (struct wp_command_scissor *)wp_command_buffer_push( b, WORKPHONE_COMMAND_SCISSOR,
                                                               sizeof( *cmd ) );

    if( !cmd )
        return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)WORKPHONE_MAX( 0, r.w );
    cmd->h = (unsigned short)WORKPHONE_MAX( 0, r.h );
}

void wp_stroke_line( struct wp_command_buffer *b, wp_f32 x0, wp_f32 y0, wp_f32 x1, wp_f32 y1,
                     wp_f32 line_thickness, struct wp_color c )
{
    struct wp_command_line *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || line_thickness <= 0 )
        return;
    cmd = (struct wp_command_line *)wp_command_buffer_push( b, WORKPHONE_COMMAND_LINE, sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->begin.x = (short)x0;
    cmd->begin.y = (short)y0;
    cmd->end.x = (short)x1;
    cmd->end.y = (short)y1;
    cmd->color = c;
}

void wp_stroke_curve( struct wp_command_buffer *b, wp_f32 ax, wp_f32 ay, wp_f32 ctrl0x, wp_f32 ctrl0y,
                      wp_f32 ctrl1x, wp_f32 ctrl1y, wp_f32 bx, wp_f32 by, wp_f32 line_thickness,
                      struct wp_color col )
{
    struct wp_command_curve *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || col.a == 0 || line_thickness <= 0 )
        return;

    cmd =
        (struct wp_command_curve *)wp_command_buffer_push( b, WORKPHONE_COMMAND_CURVE, sizeof( *cmd ) );
    if( !cmd )
        return;

    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->begin.x = (short)ax;
    cmd->begin.y = (short)ay;
    cmd->ctrl[0].x = (short)ctrl0x;
    cmd->ctrl[0].y = (short)ctrl0y;
    cmd->ctrl[1].x = (short)ctrl1x;
    cmd->ctrl[1].y = (short)ctrl1y;
    cmd->end.x = (short)bx;
    cmd->end.y = (short)by;
    cmd->color = col;
}

void wp_stroke_rect( struct wp_command_buffer *b, struct wp_rect rect, wp_f32 rounding,
                     wp_f32 line_thickness, struct wp_color c )
{
    struct wp_command_rect *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || c.a == 0 || rect.w == 0 || rect.h == 0 || line_thickness <= 0 )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *clip = &b->clip;
        if( !WORKPHONE_INTERSECT( rect.x, rect.y, rect.w, rect.h, clip->x, clip->y, clip->w, clip->h ) )
            return;
    }
    cmd = (struct wp_command_rect *)wp_command_buffer_push( b, WORKPHONE_COMMAND_RECT, sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->rounding = (unsigned short)rounding;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)WORKPHONE_MAX( 0, rect.w );
    cmd->h = (unsigned short)WORKPHONE_MAX( 0, rect.h );
    cmd->color = c;
}
void wp_fill_rect( struct wp_command_buffer *b, struct wp_rect rect, wp_f32 rounding, struct wp_color c )
{
    struct wp_command_rect_filled *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || c.a == 0 || rect.w == 0 || rect.h == 0 )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *clip = &b->clip;
        if( !WORKPHONE_INTERSECT( rect.x, rect.y, rect.w, rect.h, clip->x, clip->y, clip->w, clip->h ) )
            return;
    }

    cmd = (struct wp_command_rect_filled *)wp_command_buffer_push( b, WORKPHONE_COMMAND_RECT_FILLED,
                                                                   sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->rounding = (unsigned short)rounding;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)WORKPHONE_MAX( 0, rect.w );
    cmd->h = (unsigned short)WORKPHONE_MAX( 0, rect.h );
    cmd->color = c;
}
void wp_fill_rect_multi_color( struct wp_command_buffer *b, struct wp_rect rect, struct wp_color left,
                               struct wp_color top, struct wp_color right, struct wp_color bottom )
{
    struct wp_command_rect_multi_color *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || rect.w == 0 || rect.h == 0 )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *clip = &b->clip;
        if( !WORKPHONE_INTERSECT( rect.x, rect.y, rect.w, rect.h, clip->x, clip->y, clip->w, clip->h ) )
            return;
    }

    cmd = (struct wp_command_rect_multi_color *)wp_command_buffer_push(
        b, WORKPHONE_COMMAND_RECT_MULTI_COLOR, sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)WORKPHONE_MAX( 0, rect.w );
    cmd->h = (unsigned short)WORKPHONE_MAX( 0, rect.h );
    cmd->left = left;
    cmd->top = top;
    cmd->right = right;
    cmd->bottom = bottom;
}
void wp_stroke_circle( struct wp_command_buffer *b, struct wp_rect r, wp_f32 line_thickness,
                       struct wp_color c )
{
    struct wp_command_circle *cmd;
    if( !b || r.w == 0 || r.h == 0 || line_thickness <= 0 )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *clip = &b->clip;
        if( !WORKPHONE_INTERSECT( r.x, r.y, r.w, r.h, clip->x, clip->y, clip->w, clip->h ) )
            return;
    }

    cmd = (struct wp_command_circle *)wp_command_buffer_push( b, WORKPHONE_COMMAND_CIRCLE,
                                                              sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)WORKPHONE_MAX( r.w, 0 );
    cmd->h = (unsigned short)WORKPHONE_MAX( r.h, 0 );
    cmd->color = c;
}
void wp_fill_circle( struct wp_command_buffer *b, struct wp_rect r, struct wp_color c )
{
    struct wp_command_circle_filled *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || c.a == 0 || r.w == 0 || r.h == 0 )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *clip = &b->clip;
        if( !WORKPHONE_INTERSECT( r.x, r.y, r.w, r.h, clip->x, clip->y, clip->w, clip->h ) )
            return;
    }

    cmd = (struct wp_command_circle_filled *)wp_command_buffer_push( b, WORKPHONE_COMMAND_CIRCLE_FILLED,
                                                                     sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)WORKPHONE_MAX( r.w, 0 );
    cmd->h = (unsigned short)WORKPHONE_MAX( r.h, 0 );
    cmd->color = c;
}
void wp_stroke_arc( struct wp_command_buffer *b, wp_f32 cx, wp_f32 cy, wp_f32 radius, wp_f32 a_min,
                    wp_f32 a_max, wp_f32 line_thickness, struct wp_color c )
{
    struct wp_command_arc *cmd;
    if( !b || c.a == 0 || line_thickness <= 0 )
        return;
    cmd = (struct wp_command_arc *)wp_command_buffer_push( b, WORKPHONE_COMMAND_ARC, sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->cx = (short)cx;
    cmd->cy = (short)cy;
    cmd->r = (unsigned short)radius;
    cmd->a[0] = a_min;
    cmd->a[1] = a_max;
    cmd->color = c;
}
void wp_fill_arc( struct wp_command_buffer *b, wp_f32 cx, wp_f32 cy, wp_f32 radius, wp_f32 a_min,
                  wp_f32 a_max, struct wp_color c )
{
    struct wp_command_arc_filled *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || c.a == 0 )
        return;
    cmd = (struct wp_command_arc_filled *)wp_command_buffer_push( b, WORKPHONE_COMMAND_ARC_FILLED,
                                                                  sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->cx = (short)cx;
    cmd->cy = (short)cy;
    cmd->r = (unsigned short)radius;
    cmd->a[0] = a_min;
    cmd->a[1] = a_max;
    cmd->color = c;
}
void wp_stroke_triangle( struct wp_command_buffer *b, wp_f32 x0, wp_f32 y0, wp_f32 x1, wp_f32 y1,
                         wp_f32 x2, wp_f32 y2, wp_f32 line_thickness, struct wp_color c )
{
    struct wp_command_triangle *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || c.a == 0 || line_thickness <= 0 )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *clip = &b->clip;
        if( !WORKPHONE_INBOX( x0, y0, clip->x, clip->y, clip->w, clip->h ) &&
            !WORKPHONE_INBOX( x1, y1, clip->x, clip->y, clip->w, clip->h ) &&
            !WORKPHONE_INBOX( x2, y2, clip->x, clip->y, clip->w, clip->h ) )
            return;
    }

    cmd = (struct wp_command_triangle *)wp_command_buffer_push( b, WORKPHONE_COMMAND_TRIANGLE,
                                                                sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->a.x = (short)x0;
    cmd->a.y = (short)y0;
    cmd->b.x = (short)x1;
    cmd->b.y = (short)y1;
    cmd->c.x = (short)x2;
    cmd->c.y = (short)y2;
    cmd->color = c;
}
void wp_fill_triangle( struct wp_command_buffer *b, wp_f32 x0, wp_f32 y0, wp_f32 x1, wp_f32 y1,
                       wp_f32 x2, wp_f32 y2, struct wp_color c )
{
    struct wp_command_triangle_filled *cmd;
    WORKPHONE_ASSERT( b );
    if( !b || c.a == 0 )
        return;
    if( !b )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *clip = &b->clip;
        if( !WORKPHONE_INBOX( x0, y0, clip->x, clip->y, clip->w, clip->h ) &&
            !WORKPHONE_INBOX( x1, y1, clip->x, clip->y, clip->w, clip->h ) &&
            !WORKPHONE_INBOX( x2, y2, clip->x, clip->y, clip->w, clip->h ) )
            return;
    }

    cmd = (struct wp_command_triangle_filled *)wp_command_buffer_push(
        b, WORKPHONE_COMMAND_TRIANGLE_FILLED, sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->a.x = (short)x0;
    cmd->a.y = (short)y0;
    cmd->b.x = (short)x1;
    cmd->b.y = (short)y1;
    cmd->c.x = (short)x2;
    cmd->c.y = (short)y2;
    cmd->color = c;
}
void wp_stroke_polygon( struct wp_command_buffer *b, const wp_f32 *points, wp_s32 point_count,
                        wp_f32 line_thickness, struct wp_color col )
{
    wp_s32 i;
    wp_size size = 0;
    struct wp_command_polygon *cmd;

    WORKPHONE_ASSERT( b );
    if( !b || col.a == 0 || line_thickness <= 0 )
        return;
    size = sizeof( *cmd ) + sizeof( short ) * 2 * (wp_size)point_count;
    cmd = (struct wp_command_polygon *)wp_command_buffer_push( b, WORKPHONE_COMMAND_POLYGON, size );
    if( !cmd )
        return;
    cmd->color = col;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->point_count = (unsigned short)point_count;
    for( i = 0; i < point_count; ++i )
    {
        cmd->points[i].x = (short)points[i * 2];
        cmd->points[i].y = (short)points[i * 2 + 1];
    }
}
void wp_fill_polygon( struct wp_command_buffer *b, const wp_f32 *points, wp_s32 point_count,
                      struct wp_color col )
{
    wp_s32 i;
    wp_size size = 0;
    struct wp_command_polygon_filled *cmd;

    WORKPHONE_ASSERT( b );
    if( !b || col.a == 0 )
        return;
    size = sizeof( *cmd ) + sizeof( short ) * 2 * (wp_size)point_count;
    cmd = (struct wp_command_polygon_filled *)wp_command_buffer_push(
        b, WORKPHONE_COMMAND_POLYGON_FILLED, size );
    if( !cmd )
        return;
    cmd->color = col;
    cmd->point_count = (unsigned short)point_count;
    for( i = 0; i < point_count; ++i )
    {
        cmd->points[i].x = (short)points[i * 2 + 0];
        cmd->points[i].y = (short)points[i * 2 + 1];
    }
}
void wp_stroke_polyline( struct wp_command_buffer *b, const wp_f32 *points, wp_s32 point_count,
                         wp_f32 line_thickness, struct wp_color col )
{
    wp_s32 i;
    wp_size size = 0;
    struct wp_command_polyline *cmd;

    WORKPHONE_ASSERT( b );
    if( !b || col.a == 0 || line_thickness <= 0 )
        return;
    size = sizeof( *cmd ) + sizeof( short ) * 2 * (wp_size)point_count;
    cmd = (struct wp_command_polyline *)wp_command_buffer_push( b, WORKPHONE_COMMAND_POLYLINE, size );
    if( !cmd )
        return;
    cmd->color = col;
    cmd->point_count = (unsigned short)point_count;
    cmd->line_thickness = (unsigned short)line_thickness;
    for( i = 0; i < point_count; ++i )
    {
        cmd->points[i].x = (short)points[i * 2];
        cmd->points[i].y = (short)points[i * 2 + 1];
    }
}
void wp_draw_image( struct wp_command_buffer *b, struct wp_rect r, const struct wp_image *img,
                    struct wp_color col )
{
    struct wp_command_image *cmd;
    WORKPHONE_ASSERT( b );
    if( !b )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *c = &b->clip;
        if( c->w == 0 || c->h == 0 ||
            !WORKPHONE_INTERSECT( r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h ) )
            return;
    }

    cmd =
        (struct wp_command_image *)wp_command_buffer_push( b, WORKPHONE_COMMAND_IMAGE, sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)WORKPHONE_MAX( 0, r.w );
    cmd->h = (unsigned short)WORKPHONE_MAX( 0, r.h );
    cmd->img = *img;
    cmd->col = col;
}
void wp_draw_nine_slice( struct wp_command_buffer *b, struct wp_rect r, const struct wp_nine_slice *slc,
                         struct wp_color col )
{
    struct wp_image img;
    const struct wp_image *slcimg = (const struct wp_image *)slc;
    wp_u16 rgnX, rgnY, rgnW, rgnH;
    rgnX = slcimg->region[0];
    rgnY = slcimg->region[1];
    rgnW = slcimg->region[2];
    rgnH = slcimg->region[3];

    /* top-left */
    img.handle = slcimg->handle;
    img.w = slcimg->w;
    img.h = slcimg->h;
    img.region[0] = rgnX;
    img.region[1] = rgnY;
    img.region[2] = slc->l;
    img.region[3] = slc->t;

    wp_draw_image( b, wp_make_rect( r.x, r.y, (wp_f32)slc->l, (wp_f32)slc->t ), &img, col );

#define IMG_RGN( x, y, w, h ) \
    img.region[0] = (wp_u16)( x ); \
    img.region[1] = (wp_u16)( y ); \
    img.region[2] = (wp_u16)( w ); \
    img.region[3] = (wp_u16)( h );

    /* top-center */
    IMG_RGN( rgnX + slc->l, rgnY, rgnW - slc->l - slc->r, slc->t );
    wp_draw_image(
        b, wp_make_rect( r.x + (wp_f32)slc->l, r.y, (wp_f32)( r.w - slc->l - slc->r ), (wp_f32)slc->t ),
        &img, col );

    /* top-right */
    IMG_RGN( rgnX + rgnW - slc->r, rgnY, slc->r, slc->t );
    wp_draw_image( b, wp_make_rect( r.x + r.w - (wp_f32)slc->r, r.y, (wp_f32)slc->r, (wp_f32)slc->t ),
                   &img, col );

    /* center-left */
    IMG_RGN( rgnX, rgnY + slc->t, slc->l, rgnH - slc->t - slc->b );
    wp_draw_image(
        b, wp_make_rect( r.x, r.y + (wp_f32)slc->t, (wp_f32)slc->l, (wp_f32)( r.h - slc->t - slc->b ) ),
        &img, col );

    /* center */
    IMG_RGN( rgnX + slc->l, rgnY + slc->t, rgnW - slc->l - slc->r, rgnH - slc->t - slc->b );
    wp_draw_image( b,
                   wp_make_rect( r.x + (wp_f32)slc->l, r.y + (wp_f32)slc->t,
                                 (wp_f32)( r.w - slc->l - slc->r ), (wp_f32)( r.h - slc->t - slc->b ) ),
                   &img, col );

    /* center-right */
    IMG_RGN( rgnX + rgnW - slc->r, rgnY + slc->t, slc->r, rgnH - slc->t - slc->b );
    wp_draw_image( b,
                   wp_make_rect( r.x + r.w - (wp_f32)slc->r, r.y + (wp_f32)slc->t, (wp_f32)slc->r,
                                 (wp_f32)( r.h - slc->t - slc->b ) ),
                   &img, col );

    /* bottom-left */
    IMG_RGN( rgnX, rgnY + rgnH - slc->b, slc->l, slc->b );
    wp_draw_image( b, wp_make_rect( r.x, r.y + r.h - (wp_f32)slc->b, (wp_f32)slc->l, (wp_f32)slc->b ),
                   &img, col );

    /* bottom-center */
    IMG_RGN( rgnX + slc->l, rgnY + rgnH - slc->b, rgnW - slc->l - slc->r, slc->b );
    wp_draw_image( b,
                   wp_make_rect( r.x + (wp_f32)slc->l, r.y + r.h - (wp_f32)slc->b,
                                 (wp_f32)( r.w - slc->l - slc->r ), (wp_f32)slc->b ),
                   &img, col );

    /* bottom-right */
    IMG_RGN( rgnX + rgnW - slc->r, rgnY + rgnH - slc->b, slc->r, slc->b );
    wp_draw_image( b,
                   wp_make_rect( r.x + r.w - (wp_f32)slc->r, r.y + r.h - (wp_f32)slc->b, (wp_f32)slc->r,
                                 (wp_f32)slc->b ),
                   &img, col );

#undef IMG_RGN
}
void wp_push_custom( struct wp_command_buffer *b, struct wp_rect r, wp_command_custom_callback cb,
                     wp_handle usr )
{
    struct wp_command_custom *cmd;
    WORKPHONE_ASSERT( b );
    if( !b )
        return;
    if( b->use_clipping )
    {
        const struct wp_rect *c = &b->clip;
        if( c->w == 0 || c->h == 0 ||
            !WORKPHONE_INTERSECT( r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h ) )
            return;
    }

    cmd = (struct wp_command_custom *)wp_command_buffer_push( b, WORKPHONE_COMMAND_CUSTOM,
                                                              sizeof( *cmd ) );
    if( !cmd )
        return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)WORKPHONE_MAX( 0, r.w );
    cmd->h = (unsigned short)WORKPHONE_MAX( 0, r.h );
    cmd->callback_data = usr;
    cmd->callback = cb;
}
void wp_draw_text( struct wp_command_buffer *b, struct wp_rect r, const wp_c8 *string, wp_s32 length,
                   const struct wp_user_font *font, struct wp_color bg, struct wp_color fg )
{
    wp_f32 text_width = 0;
    struct wp_command_text *cmd;

    WORKPHONE_ASSERT( b );
    WORKPHONE_ASSERT( font );
    if( !b || !string || !length || ( bg.a == 0 && fg.a == 0 ) )
        return;

    if( b->use_clipping )
    {
        const struct wp_rect *c = &b->clip;
        if( c->w == 0 || c->h == 0 ||
            !WORKPHONE_INTERSECT( r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h ) )
            return;
    }

    /* make sure text fits inside bounds */
    text_width = font->width( font->userdata, font->height, string, length );

    if( text_width > r.w )
    {
        wp_s32 glyphs = 0;
        wp_f32 txt_width = (wp_f32)text_width;
        length = wp_text_clamp( font, string, length, r.w, &glyphs, &txt_width, 0, 0 );
    }

    if( !length )
        return;
    cmd = (struct wp_command_text *)wp_command_buffer_push( b, WORKPHONE_COMMAND_TEXT,
                                                            sizeof( *cmd ) + (wp_size)( length + 1 ) );
    if( !cmd )
        return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)r.w;
    cmd->h = (unsigned short)r.h;
    cmd->background = bg;
    cmd->foreground = fg;
    cmd->font = font;
    cmd->length = length;
    cmd->height = font->height;
    WORKPHONE_MEMCPY( cmd->string, string, (wp_size)length );
    cmd->string[length] = '\0';
}
