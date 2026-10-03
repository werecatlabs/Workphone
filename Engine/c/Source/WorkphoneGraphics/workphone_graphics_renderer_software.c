/**
 * @file wp_renderer_software.c
 * @brief Implementation of the CPU-based software renderer.
 */

#include "workphone_graphics_renderer_software.h"
#include "workphone_graphics_viewport.h"
#include "workphone_math.h"
#include <float.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Macros
 * ====================================================================== */

#define SW_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define SW_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define SW_CLAMP( x, min_val, max_val ) ( SW_MIN( SW_MAX( ( x ), ( min_val ) ), ( max_val ) ) )

/* =========================================================================
 * Internal structure
 * ====================================================================== */

struct wp_renderer_software
{
    wp_u8 *color_buffer;
    wp_f32 *depth_buffer;
    wp_u8 *resolve_buffer;
    wp_s32 width;
    wp_s32 height;
    wp_pixel_format format;
    wp_s32 bytes_per_pixel;

    wp_f32 clear_r;
    wp_f32 clear_g;
    wp_f32 clear_b;
    wp_f32 clear_a;
    wp_f32 clear_depth;

    wp_viewport_i viewport;
    wp_s32 scissor_enabled;
    wp_viewport_i scissor;

    wp_blend_mode blend_mode;
    wp_fill_mode fill_mode;
    wp_cull_mode cull_mode;
    wp_s32 depth_test_enabled;
    wp_s32 depth_write_enabled;
    wp_depth_func depth_func;

    wp_mat4f world_matrix;
    wp_mat4f view_matrix;
    wp_mat4f proj_matrix;
    wp_mat4f mvp_matrix;
    wp_s32 mvp_dirty;

    wp_s32 frame_active;
    wp_s32 frame_resolved;

    const wp_u8 *texture_pixels;
    wp_s32 texture_width;
    wp_s32 texture_height;
    wp_pixel_format texture_format;

    void *native;
};

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_s32 sw_bytes_per_pixel( wp_pixel_format fmt )
{
    switch( fmt )
    {
    case WORKPHONE_PIXEL_FORMAT_RGBA8:
    case WORKPHONE_PIXEL_FORMAT_BGRA8:
        return 4;
    case WORKPHONE_PIXEL_FORMAT_RGB8:
    case WORKPHONE_PIXEL_FORMAT_BGR8:
        return 3;
    default:
        return 0;
    }
}

static wp_s32 sw_get_buffer_sizes( wp_s32 width, wp_s32 height, wp_s32 bytes_per_pixel,
                                   size_t *pixel_count, size_t *color_bytes, size_t *depth_bytes )
{
    size_t w, h, count;

    if( width <= 0 || height <= 0 || bytes_per_pixel <= 0 )
        return 0;

    w = (size_t)width;
    h = (size_t)height;
    if( w > SIZE_MAX / h )
        return 0;

    count = w * h;
    if( count > SIZE_MAX / (size_t)bytes_per_pixel || count > SIZE_MAX / sizeof( wp_f32 ) )
        return 0;

    if( pixel_count )
        *pixel_count = count;
    if( color_bytes )
        *color_bytes = count * (size_t)bytes_per_pixel;
    if( depth_bytes )
        *depth_bytes = count * sizeof( wp_f32 );
    return 1;
}

static wp_viewport_i sw_clamp_rect( wp_viewport_i rect, wp_s32 width, wp_s32 height )
{
    wp_viewport_i result;
    int64_t x0, y0, x1, y1;

    x0 = (int64_t)rect.x;
    y0 = (int64_t)rect.y;
    x1 = x0 + ( rect.width > 0 ? (int64_t)rect.width : 0 );
    y1 = y0 + ( rect.height > 0 ? (int64_t)rect.height : 0 );

    x0 = SW_CLAMP( x0, 0, (int64_t)width );
    y0 = SW_CLAMP( y0, 0, (int64_t)height );
    x1 = SW_CLAMP( x1, x0, (int64_t)width );
    y1 = SW_CLAMP( y1, y0, (int64_t)height );

    result.x = (wp_s32)x0;
    result.y = (wp_s32)y0;
    result.width = (wp_s32)( x1 - x0 );
    result.height = (wp_s32)( y1 - y0 );
    return result;
}

static void sw_mat4f_identity( wp_mat4f *m )
{
    memset( m, 0, sizeof( wp_mat4f ) );
    m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = 1.0f;
}

/* Row-major matrix multiply: result = a * b */
static void sw_mat4f_mul( wp_mat4f *result, const wp_mat4f *a, const wp_mat4f *b )
{
    wp_mat4f tmp;
    wp_s32 i, j, k;

    for( i = 0; i < 4; i++ )
        for( j = 0; j < 4; j++ )
        {
            tmp.m[i][j] = 0.0f;
            for( k = 0; k < 4; k++ )
                tmp.m[i][j] += a->m[i][k] * b->m[k][j];
        }

    *result = tmp;
}

/* Column-vector multiply: result = m * v */
static wp_vec4f sw_mat4f_mul_vec4( const wp_mat4f *m, wp_vec4f v )
{
    wp_vec4f r;

    r.x = m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z + m->m[0][3] * v.w;
    r.y = m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z + m->m[1][3] * v.w;
    r.z = m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z + m->m[2][3] * v.w;
    r.w = m->m[3][0] * v.x + m->m[3][1] * v.y + m->m[3][2] * v.z + m->m[3][3] * v.w;

    return r;
}

static void sw_update_mvp( wp_renderer_software *r )
{
    wp_mat4f vw;

    if( !r->mvp_dirty )
        return;

    sw_mat4f_mul( &vw, &r->view_matrix, &r->world_matrix );
    sw_mat4f_mul( &r->mvp_matrix, &r->proj_matrix, &vw );
    r->mvp_dirty = 0;
}

/* Unpack RGBA8 packed as R[31:24] G[23:16] B[15:8] A[7:0] */
static void sw_unpack_color( wp_u32 packed, wp_f32 *cr, wp_f32 *cg, wp_f32 *cb, wp_f32 *ca )
{
    *cr = ( ( packed >> 24 ) & 0xFF ) / 255.0f;
    *cg = ( ( packed >> 16 ) & 0xFF ) / 255.0f;
    *cb = ( ( packed >> 8 ) & 0xFF ) / 255.0f;
    *ca = ( ( packed ) & 0xFF ) / 255.0f;
}

static void sw_sample_texture( const wp_renderer_software *r, wp_f32 u, wp_f32 v, wp_f32 *tr, wp_f32 *tg,
                               wp_f32 *tb, wp_f32 *ta )
{
    const wp_u8 *p;
    wp_s32 x, y, bpp;

    *tr = *tg = *tb = *ta = 1.0f;
    if( !r->texture_pixels || r->texture_width <= 0 || r->texture_height <= 0 )
        return;

    bpp = sw_bytes_per_pixel( r->texture_format );
    if( bpp == 0 )
        return;

    u = SW_CLAMP( u, 0.0f, 1.0f );
    v = SW_CLAMP( v, 0.0f, 1.0f );
    x = SW_CLAMP( (wp_s32)( u * (wp_f32)( r->texture_width - 1 ) + 0.5f ), 0, r->texture_width - 1 );
    y = SW_CLAMP( (wp_s32)( v * (wp_f32)( r->texture_height - 1 ) + 0.5f ), 0, r->texture_height - 1 );
    p = r->texture_pixels + ( (size_t)y * (size_t)r->texture_width + (size_t)x ) * (size_t)bpp;

    switch( r->texture_format )
    {
    case WORKPHONE_PIXEL_FORMAT_RGBA8:
        *tr = p[0] / 255.0f;
        *tg = p[1] / 255.0f;
        *tb = p[2] / 255.0f;
        *ta = p[3] / 255.0f;
        break;
    case WORKPHONE_PIXEL_FORMAT_BGRA8:
        *tb = p[0] / 255.0f;
        *tg = p[1] / 255.0f;
        *tr = p[2] / 255.0f;
        *ta = p[3] / 255.0f;
        break;
    case WORKPHONE_PIXEL_FORMAT_RGB8:
        *tr = p[0] / 255.0f;
        *tg = p[1] / 255.0f;
        *tb = p[2] / 255.0f;
        break;
    case WORKPHONE_PIXEL_FORMAT_BGR8:
        *tb = p[0] / 255.0f;
        *tg = p[1] / 255.0f;
        *tr = p[2] / 255.0f;
        break;
    default:
        break;
    }
}

static void sw_read_pixel_from( const wp_renderer_software *r, const wp_u8 *buffer, wp_s32 x, wp_s32 y,
                                wp_f32 *cr, wp_f32 *cg, wp_f32 *cb, wp_f32 *ca )
{
    const wp_u8 *p = buffer + ( (size_t)y * (size_t)r->width + (size_t)x ) * (size_t)r->bytes_per_pixel;

    switch( r->format )
    {
    case WORKPHONE_PIXEL_FORMAT_RGBA8:
        *cr = p[0] / 255.0f;
        *cg = p[1] / 255.0f;
        *cb = p[2] / 255.0f;
        *ca = p[3] / 255.0f;
        break;
    case WORKPHONE_PIXEL_FORMAT_BGRA8:
        *cb = p[0] / 255.0f;
        *cg = p[1] / 255.0f;
        *cr = p[2] / 255.0f;
        *ca = p[3] / 255.0f;
        break;
    case WORKPHONE_PIXEL_FORMAT_RGB8:
        *cr = p[0] / 255.0f;
        *cg = p[1] / 255.0f;
        *cb = p[2] / 255.0f;
        *ca = 1.0f;
        break;
    case WORKPHONE_PIXEL_FORMAT_BGR8:
        *cb = p[0] / 255.0f;
        *cg = p[1] / 255.0f;
        *cr = p[2] / 255.0f;
        *ca = 1.0f;
        break;
    default:
        *cr = *cg = *cb = *ca = 0.0f;
        break;
    }
}

static void sw_read_pixel( const wp_renderer_software *r, wp_s32 x, wp_s32 y, wp_f32 *cr, wp_f32 *cg,
                           wp_f32 *cb, wp_f32 *ca )
{
    sw_read_pixel_from( r, r->color_buffer, x, y, cr, cg, cb, ca );
}

static void sw_write_pixel_raw( wp_renderer_software *r, wp_s32 x, wp_s32 y, wp_f32 fr, wp_f32 fg,
                                wp_f32 fb, wp_f32 fa )
{
    wp_u8 *p =
        r->color_buffer + ( (size_t)y * (size_t)r->width + (size_t)x ) * (size_t)r->bytes_per_pixel;
    wp_u8 ur, ug, ub, ua;

    ur = (wp_u8)( SW_CLAMP( fr, 0.0f, 1.0f ) * 255.0f );
    ug = (wp_u8)( SW_CLAMP( fg, 0.0f, 1.0f ) * 255.0f );
    ub = (wp_u8)( SW_CLAMP( fb, 0.0f, 1.0f ) * 255.0f );
    ua = (wp_u8)( SW_CLAMP( fa, 0.0f, 1.0f ) * 255.0f );

    switch( r->format )
    {
    case WORKPHONE_PIXEL_FORMAT_RGBA8:
        p[0] = ur;
        p[1] = ug;
        p[2] = ub;
        p[3] = ua;
        break;
    case WORKPHONE_PIXEL_FORMAT_BGRA8:
        p[0] = ub;
        p[1] = ug;
        p[2] = ur;
        p[3] = ua;
        break;
    case WORKPHONE_PIXEL_FORMAT_RGB8:
        p[0] = ur;
        p[1] = ug;
        p[2] = ub;
        break;
    case WORKPHONE_PIXEL_FORMAT_BGR8:
        p[0] = ub;
        p[1] = ug;
        p[2] = ur;
        break;
    default:
        break;
    }
}

/* Write one pixel applying scissor, depth test, and blending. */
static void sw_write_pixel( wp_renderer_software *r, wp_s32 x, wp_s32 y, wp_f32 depth, wp_f32 sr,
                            wp_f32 sg, wp_f32 sb, wp_f32 sa )
{
    size_t idx;
    wp_s32 depth_pass;
    wp_f32 stored, dr, dg, db, da, out_r, out_g, out_b, out_a;

    if( x < 0 || y < 0 || x >= r->width || y >= r->height )
        return;

    if( !isfinite( depth ) || depth < 0.0f || depth > 1.0f )
        return;

    if( r->scissor_enabled &&
        ( x < r->scissor.x || y < r->scissor.y || x >= r->scissor.x + r->scissor.width ||
          y >= r->scissor.y + r->scissor.height ) )
        return;

    idx = (size_t)y * (size_t)r->width + (size_t)x;
    stored = r->depth_buffer[idx];

    if( r->depth_test_enabled )
    {
        switch( r->depth_func )
        {
        case WORKPHONE_DEPTH_FUNC_NEVER:
            depth_pass = 0;
            break;
        case WORKPHONE_DEPTH_FUNC_LESS:
            depth_pass = depth < stored;
            break;
        case WORKPHONE_DEPTH_FUNC_EQUAL:
            depth_pass = depth == stored;
            break;
        case WORKPHONE_DEPTH_FUNC_LEQUAL:
            depth_pass = depth <= stored;
            break;
        case WORKPHONE_DEPTH_FUNC_GREATER:
            depth_pass = depth > stored;
            break;
        case WORKPHONE_DEPTH_FUNC_NOTEQUAL:
            depth_pass = depth != stored;
            break;
        case WORKPHONE_DEPTH_FUNC_GEQUAL:
            depth_pass = depth >= stored;
            break;
        case WORKPHONE_DEPTH_FUNC_ALWAYS:
            depth_pass = 1;
            break;
        default:
            depth_pass = 0;
            break;
        }
        if( !depth_pass )
            return;
    }

    r->frame_resolved = 0;

    if( r->depth_write_enabled )
        r->depth_buffer[idx] = depth;

    switch( r->blend_mode )
    {
    default:
    case WORKPHONE_BLEND_MODE_NONE:
        out_r = sr;
        out_g = sg;
        out_b = sb;
        out_a = sa;
        break;
    case WORKPHONE_BLEND_MODE_ALPHA:
        sw_read_pixel( r, x, y, &dr, &dg, &db, &da );
        out_r = sr * sa + dr * ( 1.0f - sa );
        out_g = sg * sa + dg * ( 1.0f - sa );
        out_b = sb * sa + db * ( 1.0f - sa );
        out_a = sa + da * ( 1.0f - sa );
        break;
    case WORKPHONE_BLEND_MODE_ADDITIVE:
        sw_read_pixel( r, x, y, &dr, &dg, &db, &da );
        out_r = SW_CLAMP( sr * sa + dr, 0.0f, 1.0f );
        out_g = SW_CLAMP( sg * sa + dg, 0.0f, 1.0f );
        out_b = SW_CLAMP( sb * sa + db, 0.0f, 1.0f );
        out_a = 1.0f;
        break;
    case WORKPHONE_BLEND_MODE_MULTIPLY:
        sw_read_pixel( r, x, y, &dr, &dg, &db, &da );
        out_r = sr * dr;
        out_g = sg * dg;
        out_b = sb * db;
        out_a = sa * da;
        break;
    }

    sw_write_pixel_raw( r, x, y, out_r, out_g, out_b, out_a );
}

/*
 * Transform a position through MVP → NDC → screen space.
 * Returns 0 if the clip-space w <= 0 (behind or on the near plane).
 *
 * Depth is mapped from NDC z ∈ [-1, 1] to [0, 1].
 * Screen y is flipped so that NDC +y maps to the top of the viewport.
 */
static wp_s32 sw_transform_vertex( wp_renderer_software *r, wp_vec3f pos, wp_f32 *sx, wp_f32 *sy,
                                   wp_f32 *sd )
{
    wp_vec4f v, clip;
    wp_f32 inv_w, ndc_x, ndc_y, ndc_z;

    sw_update_mvp( r );

    v.x = pos.x;
    v.y = pos.y;
    v.z = pos.z;
    v.w = 1.0f;
    clip = sw_mat4f_mul_vec4( &r->mvp_matrix, v );

    if( !isfinite( clip.x ) || !isfinite( clip.y ) || !isfinite( clip.z ) || !isfinite( clip.w ) ||
        clip.w <= FLT_EPSILON )
        return 0;

    inv_w = 1.0f / clip.w;
    ndc_x = clip.x * inv_w;
    ndc_y = clip.y * inv_w;
    ndc_z = clip.z * inv_w;
    if( !isfinite( ndc_x ) || !isfinite( ndc_y ) || !isfinite( ndc_z ) )
        return 0;

    *sx = ( ndc_x + 1.0f ) * 0.5f * (wp_f32)r->viewport.width + (wp_f32)r->viewport.x;
    *sy = ( 1.0f - ndc_y ) * 0.5f * (wp_f32)r->viewport.height + (wp_f32)r->viewport.y;
    *sd = ( ndc_z + 1.0f ) * 0.5f;

    return 1;
}

static wp_s32 sw_clip_test( wp_f32 p, wp_f32 q, wp_f32 *t0, wp_f32 *t1 )
{
    wp_f32 t;

    if( p == 0.0f )
        return q >= 0.0f;

    t = q / p;
    if( p < 0.0f )
    {
        if( t > *t1 )
            return 0;
        if( t > *t0 )
            *t0 = t;
    }
    else
    {
        if( t < *t0 )
            return 0;
        if( t < *t1 )
            *t1 = t;
    }
    return 1;
}

/* Bresenham line with linearly interpolated depth and colour. */
static void sw_draw_line( wp_renderer_software *r, wp_f32 fx0, wp_f32 fy0, wp_f32 d0, wp_f32 r0,
                          wp_f32 g0, wp_f32 b0, wp_f32 a0, wp_f32 fx1, wp_f32 fy1, wp_f32 d1, wp_f32 r1,
                          wp_f32 g1, wp_f32 b1, wp_f32 a1 )
{
    wp_s32 x0, y0, x1, y1, dx, dy, sx, sy, err, len, step, e2;
    wp_f32 t0, t1, t, od0, or0, og0, ob0, oa0, od1, or1, og1, ob1, oa1;
    wp_f32 left, top, right, bottom, line_dx, line_dy;

    if( r->viewport.width <= 0 || r->viewport.height <= 0 || !isfinite( fx0 ) || !isfinite( fy0 ) ||
        !isfinite( fx1 ) || !isfinite( fy1 ) )
        return;

    left = (wp_f32)r->viewport.x;
    top = (wp_f32)r->viewport.y;
    right = (wp_f32)( r->viewport.x + r->viewport.width - 1 );
    bottom = (wp_f32)( r->viewport.y + r->viewport.height - 1 );
    line_dx = fx1 - fx0;
    line_dy = fy1 - fy0;
    t0 = 0.0f;
    t1 = 1.0f;

    if( !sw_clip_test( -line_dx, fx0 - left, &t0, &t1 ) ||
        !sw_clip_test( line_dx, right - fx0, &t0, &t1 ) ||
        !sw_clip_test( -line_dy, fy0 - top, &t0, &t1 ) ||
        !sw_clip_test( line_dy, bottom - fy0, &t0, &t1 ) )
        return;

    od0 = d0;
    or0 = r0;
    og0 = g0;
    ob0 = b0;
    oa0 = a0;
    od1 = d1;
    or1 = r1;
    og1 = g1;
    ob1 = b1;
    oa1 = a1;

    fx1 = fx0 + line_dx * t1;
    fy1 = fy0 + line_dy * t1;
    d1 = od0 + ( od1 - od0 ) * t1;
    r1 = or0 + ( or1 - or0 ) * t1;
    g1 = og0 + ( og1 - og0 ) * t1;
    b1 = ob0 + ( ob1 - ob0 ) * t1;
    a1 = oa0 + ( oa1 - oa0 ) * t1;

    fx0 += line_dx * t0;
    fy0 += line_dy * t0;
    d0 = od0 + ( od1 - od0 ) * t0;
    r0 = or0 + ( or1 - or0 ) * t0;
    g0 = og0 + ( og1 - og0 ) * t0;
    b0 = ob0 + ( ob1 - ob0 ) * t0;
    a0 = oa0 + ( oa1 - oa0 ) * t0;

    x0 = (wp_s32)fx0;
    y0 = (wp_s32)fy0;
    x1 = (wp_s32)fx1;
    y1 = (wp_s32)fy1;

    dx = abs( x1 - x0 );
    dy = abs( y1 - y0 );
    sx = ( x0 < x1 ) ? 1 : -1;
    sy = ( y0 < y1 ) ? 1 : -1;
    err = dx - dy;
    len = SW_MAX( dx, dy );
    step = 0;

    for( ;; )
    {
        t = ( len > 0 ) ? (wp_f32)step / (wp_f32)len : 0.0f;
        sw_write_pixel( r, x0, y0, d0 + ( d1 - d0 ) * t, r0 + ( r1 - r0 ) * t, g0 + ( g1 - g0 ) * t,
                        b0 + ( b1 - b0 ) * t, a0 + ( a1 - a0 ) * t );

        if( x0 == x1 && y0 == y1 )
            break;

        e2 = 2 * err;
        if( e2 > -dy )
        {
            err -= dy;
            x0 += sx;
        }
        if( e2 < dx )
        {
            err += dx;
            y0 += sy;
        }
        step++;
    }
}

static void sw_draw_point( wp_renderer_software *r, wp_f32 x, wp_f32 y, wp_f32 depth, wp_f32 cr,
                           wp_f32 cg, wp_f32 cb, wp_f32 ca )
{
    if( !isfinite( x ) || !isfinite( y ) || x < (wp_f32)r->viewport.x || y < (wp_f32)r->viewport.y ||
        x >= (wp_f32)( r->viewport.x + r->viewport.width ) ||
        y >= (wp_f32)( r->viewport.y + r->viewport.height ) )
        return;

    sw_write_pixel( r, (wp_s32)x, (wp_s32)y, depth, cr, cg, cb, ca );
}

/*
 * Rasterize a triangle with barycentric interpolation.
 *
 * Signed area:  area = (V1-V0) × (V2-V0)
 *   area < 0  →  CW in screen space  = front face (OpenGL/column-vector convention)
 *   area > 0  →  CCW in screen space = back  face
 *
 * Barycentric weights (w0 for V0, w1 for V1, w2 for V2):
 *   w_i = SignedArea(sub-triangle opposite V_i, P) / TotalArea
 * All three are in [0,1] and sum to 1 for any interior point.
 */
static void sw_rasterize_triangle( wp_renderer_software *r, wp_f32 x0, wp_f32 y0, wp_f32 d0, wp_f32 r0,
                                   wp_f32 g0, wp_f32 b0, wp_f32 a0, wp_f32 x1, wp_f32 y1, wp_f32 d1,
                                   wp_f32 r1, wp_f32 g1, wp_f32 b1, wp_f32 a1, wp_f32 x2, wp_f32 y2,
                                   wp_f32 d2, wp_f32 r2, wp_f32 g2, wp_f32 b2, wp_f32 a2 )
{
    wp_f32 area, inv_area, pfx, pfy, w0, w1, w2;
    wp_f32 bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y;
    wp_s32 min_x, min_y, max_x, max_y, px, py;

    area = ( x1 - x0 ) * ( y2 - y0 ) - ( y1 - y0 ) * ( x2 - x0 );

    if( !isfinite( area ) || fabsf( area ) <= FLT_EPSILON )
        return;

    if( r->cull_mode == WORKPHONE_CULL_MODE_BACK && area > 0.0f )
        return;
    if( r->cull_mode == WORKPHONE_CULL_MODE_FRONT && area < 0.0f )
        return;

    if( r->fill_mode == WORKPHONE_FILL_MODE_WIREFRAME )
    {
        sw_draw_line( r, x0, y0, d0, r0, g0, b0, a0, x1, y1, d1, r1, g1, b1, a1 );
        sw_draw_line( r, x1, y1, d1, r1, g1, b1, a1, x2, y2, d2, r2, g2, b2, a2 );
        sw_draw_line( r, x2, y2, d2, r2, g2, b2, a2, x0, y0, d0, r0, g0, b0, a0 );
        return;
    }

    if( r->fill_mode == WORKPHONE_FILL_MODE_POINT )
    {
        sw_draw_point( r, x0, y0, d0, r0, g0, b0, a0 );
        sw_draw_point( r, x1, y1, d1, r1, g1, b1, a1 );
        sw_draw_point( r, x2, y2, d2, r2, g2, b2, a2 );
        return;
    }

    inv_area = 1.0f / area;

    if( r->viewport.width <= 0 || r->viewport.height <= 0 )
        return;

    bbox_min_x = SW_MIN( SW_MIN( x0, x1 ), x2 );
    bbox_min_y = SW_MIN( SW_MIN( y0, y1 ), y2 );
    bbox_max_x = SW_MAX( SW_MAX( x0, x1 ), x2 );
    bbox_max_y = SW_MAX( SW_MAX( y0, y1 ), y2 );

    if( bbox_max_x < (wp_f32)r->viewport.x || bbox_max_y < (wp_f32)r->viewport.y ||
        bbox_min_x > (wp_f32)( r->viewport.x + r->viewport.width - 1 ) ||
        bbox_min_y > (wp_f32)( r->viewport.y + r->viewport.height - 1 ) )
        return;

    min_x = (wp_s32)floorf( SW_MAX( bbox_min_x, (wp_f32)r->viewport.x ) );
    min_y = (wp_s32)floorf( SW_MAX( bbox_min_y, (wp_f32)r->viewport.y ) );
    max_x = (wp_s32)ceilf( SW_MIN( bbox_max_x, (wp_f32)( r->viewport.x + r->viewport.width - 1 ) ) );
    max_y = (wp_s32)ceilf( SW_MIN( bbox_max_y, (wp_f32)( r->viewport.y + r->viewport.height - 1 ) ) );

    if( min_x > max_x || min_y > max_y )
        return;

    for( py = min_y; py <= max_y; py++ )
    {
        for( px = min_x; px <= max_x; px++ )
        {
            pfx = (wp_f32)px + 0.5f;
            pfy = (wp_f32)py + 0.5f;

            /* Barycentric weights: w_i = area of sub-triangle / total area.
             * Dividing signed areas by signed total area always yields
             * non-negative values for interior points regardless of winding. */
            w0 = ( ( x2 - x1 ) * ( pfy - y1 ) - ( y2 - y1 ) * ( pfx - x1 ) ) * inv_area;
            w1 = ( ( x0 - x2 ) * ( pfy - y2 ) - ( y0 - y2 ) * ( pfx - x2 ) ) * inv_area;
            w2 = ( ( x1 - x0 ) * ( pfy - y0 ) - ( y1 - y0 ) * ( pfx - x0 ) ) * inv_area;

            if( w0 < 0.0f || w1 < 0.0f || w2 < 0.0f )
                continue;

            sw_write_pixel( r, px, py, w0 * d0 + w1 * d1 + w2 * d2, w0 * r0 + w1 * r1 + w2 * r2,
                            w0 * g0 + w1 * g1 + w2 * g2, w0 * b0 + w1 * b1 + w2 * b2,
                            w0 * a0 + w1 * a1 + w2 * a2 );
        }
    }
}

static void sw_rasterize_triangle_textured( wp_renderer_software *r, wp_f32 x0, wp_f32 y0, wp_f32 d0,
                                            wp_f32 u0, wp_f32 v0, wp_f32 r0, wp_f32 g0, wp_f32 b0,
                                            wp_f32 a0, wp_f32 x1, wp_f32 y1, wp_f32 d1, wp_f32 u1,
                                            wp_f32 v1, wp_f32 r1, wp_f32 g1, wp_f32 b1, wp_f32 a1,
                                            wp_f32 x2, wp_f32 y2, wp_f32 d2, wp_f32 u2, wp_f32 v2,
                                            wp_f32 r2, wp_f32 g2, wp_f32 b2, wp_f32 a2 )
{
    wp_f32 area, inv_area, pfx, pfy, w0, w1, w2;
    wp_f32 bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y;
    wp_f32 tr, tg, tb, ta, cr, cg, cb, ca, u, v;
    wp_s32 min_x, min_y, max_x, max_y, px, py;

    area = ( x1 - x0 ) * ( y2 - y0 ) - ( y1 - y0 ) * ( x2 - x0 );
    if( !isfinite( area ) || fabsf( area ) <= FLT_EPSILON )
        return;
    if( r->cull_mode == WORKPHONE_CULL_MODE_BACK && area > 0.0f )
        return;
    if( r->cull_mode == WORKPHONE_CULL_MODE_FRONT && area < 0.0f )
        return;
    if( r->fill_mode != WORKPHONE_FILL_MODE_SOLID || r->viewport.width <= 0 || r->viewport.height <= 0 )
    {
        sw_rasterize_triangle( r, x0, y0, d0, r0, g0, b0, a0, x1, y1, d1, r1, g1, b1, a1, x2, y2, d2, r2,
                               g2, b2, a2 );
        return;
    }

    inv_area = 1.0f / area;
    bbox_min_x = SW_MIN( SW_MIN( x0, x1 ), x2 );
    bbox_min_y = SW_MIN( SW_MIN( y0, y1 ), y2 );
    bbox_max_x = SW_MAX( SW_MAX( x0, x1 ), x2 );
    bbox_max_y = SW_MAX( SW_MAX( y0, y1 ), y2 );
    if( bbox_max_x < (wp_f32)r->viewport.x || bbox_max_y < (wp_f32)r->viewport.y ||
        bbox_min_x > (wp_f32)( r->viewport.x + r->viewport.width - 1 ) ||
        bbox_min_y > (wp_f32)( r->viewport.y + r->viewport.height - 1 ) )
        return;

    min_x = (wp_s32)floorf( SW_MAX( bbox_min_x, (wp_f32)r->viewport.x ) );
    min_y = (wp_s32)floorf( SW_MAX( bbox_min_y, (wp_f32)r->viewport.y ) );
    max_x = (wp_s32)ceilf( SW_MIN( bbox_max_x, (wp_f32)( r->viewport.x + r->viewport.width - 1 ) ) );
    max_y = (wp_s32)ceilf( SW_MIN( bbox_max_y, (wp_f32)( r->viewport.y + r->viewport.height - 1 ) ) );

    for( py = min_y; py <= max_y; py++ )
    {
        for( px = min_x; px <= max_x; px++ )
        {
            pfx = (wp_f32)px + 0.5f;
            pfy = (wp_f32)py + 0.5f;
            w0 = ( ( x2 - x1 ) * ( pfy - y1 ) - ( y2 - y1 ) * ( pfx - x1 ) ) * inv_area;
            w1 = ( ( x0 - x2 ) * ( pfy - y2 ) - ( y0 - y2 ) * ( pfx - x2 ) ) * inv_area;
            w2 = ( ( x1 - x0 ) * ( pfy - y0 ) - ( y1 - y0 ) * ( pfx - x0 ) ) * inv_area;
            if( w0 < 0.0f || w1 < 0.0f || w2 < 0.0f )
                continue;

            u = w0 * u0 + w1 * u1 + w2 * u2;
            v = w0 * v0 + w1 * v1 + w2 * v2;
            cr = w0 * r0 + w1 * r1 + w2 * r2;
            cg = w0 * g0 + w1 * g1 + w2 * g2;
            cb = w0 * b0 + w1 * b1 + w2 * b2;
            ca = w0 * a0 + w1 * a1 + w2 * a2;
            sw_sample_texture( r, u, v, &tr, &tg, &tb, &ta );
            sw_write_pixel( r, px, py, w0 * d0 + w1 * d1 + w2 * d2, cr * tr, cg * tg, cb * tb, ca * ta );
        }
    }
}

static void sw_draw_triangle_pc( wp_renderer_software *r, const wp_vertex_pc *v0, const wp_vertex_pc *v1,
                                 const wp_vertex_pc *v2 )
{
    wp_f32 sx0, sy0, sd0, sx1, sy1, sd1, sx2, sy2, sd2;
    wp_f32 cr0, cg0, cb0, ca0, cr1, cg1, cb1, ca1, cr2, cg2, cb2, ca2;

    if( !sw_transform_vertex( r, v0->position, &sx0, &sy0, &sd0 ) )
        return;
    if( !sw_transform_vertex( r, v1->position, &sx1, &sy1, &sd1 ) )
        return;
    if( !sw_transform_vertex( r, v2->position, &sx2, &sy2, &sd2 ) )
        return;

    sw_unpack_color( v0->color, &cr0, &cg0, &cb0, &ca0 );
    sw_unpack_color( v1->color, &cr1, &cg1, &cb1, &ca1 );
    sw_unpack_color( v2->color, &cr2, &cg2, &cb2, &ca2 );

    sw_rasterize_triangle( r, sx0, sy0, sd0, cr0, cg0, cb0, ca0, sx1, sy1, sd1, cr1, cg1, cb1, ca1, sx2,
                           sy2, sd2, cr2, cg2, cb2, ca2 );
}

static void sw_draw_triangle_ptc( wp_renderer_software *r, const wp_vertex_ptc *v0,
                                  const wp_vertex_ptc *v1, const wp_vertex_ptc *v2 )
{
    wp_f32 sx0, sy0, sd0, sx1, sy1, sd1, sx2, sy2, sd2;
    wp_f32 cr0, cg0, cb0, ca0, cr1, cg1, cb1, ca1, cr2, cg2, cb2, ca2;

    if( !sw_transform_vertex( r, v0->position, &sx0, &sy0, &sd0 ) )
        return;
    if( !sw_transform_vertex( r, v1->position, &sx1, &sy1, &sd1 ) )
        return;
    if( !sw_transform_vertex( r, v2->position, &sx2, &sy2, &sd2 ) )
        return;

    sw_unpack_color( v0->color, &cr0, &cg0, &cb0, &ca0 );
    sw_unpack_color( v1->color, &cr1, &cg1, &cb1, &ca1 );
    sw_unpack_color( v2->color, &cr2, &cg2, &cb2, &ca2 );

    sw_rasterize_triangle_textured( r, sx0, sy0, sd0, v0->uv.x, v0->uv.y, cr0, cg0, cb0, ca0, sx1, sy1,
                                    sd1, v1->uv.x, v1->uv.y, cr1, cg1, cb1, ca1, sx2, sy2, sd2, v2->uv.x,
                                    v2->uv.y, cr2, cg2, cb2, ca2 );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_renderer_software *wp_renderer_software_create( wp_s32 width, wp_s32 height, wp_pixel_format format )
{
    wp_renderer_software *r;
    wp_s32 bpp;
    size_t pixel_count, color_bytes, depth_bytes, i;

    bpp = sw_bytes_per_pixel( format );
    if( !sw_get_buffer_sizes( width, height, bpp, &pixel_count, &color_bytes, &depth_bytes ) )
        return NULL;

    r = (wp_renderer_software *)malloc( sizeof( wp_renderer_software ) );
    if( !r )
        return NULL;

    memset( r, 0, sizeof( wp_renderer_software ) );

    r->color_buffer = (wp_u8 *)malloc( color_bytes );
    if( !r->color_buffer )
    {
        free( r );
        return NULL;
    }

    r->depth_buffer = (wp_f32 *)malloc( depth_bytes );
    if( !r->depth_buffer )
    {
        free( r->color_buffer );
        free( r );
        return NULL;
    }

    memset( r->color_buffer, 0, color_bytes );

    r->width = width;
    r->height = height;
    r->format = format;
    r->bytes_per_pixel = bpp;

    r->clear_r = r->clear_g = r->clear_b = r->clear_a = 0.0f;
    r->clear_depth = 1.0f;

    r->viewport.x = 0;
    r->viewport.y = 0;
    r->viewport.width = width;
    r->viewport.height = height;

    r->scissor_enabled = 0;
    r->blend_mode = WORKPHONE_BLEND_MODE_NONE;
    r->fill_mode = WORKPHONE_FILL_MODE_SOLID;
    r->cull_mode = WORKPHONE_CULL_MODE_BACK;
    r->depth_test_enabled = 1;
    r->depth_write_enabled = 1;
    r->depth_func = WORKPHONE_DEPTH_FUNC_LESS;

    sw_mat4f_identity( &r->world_matrix );
    sw_mat4f_identity( &r->view_matrix );
    sw_mat4f_identity( &r->proj_matrix );
    sw_mat4f_identity( &r->mvp_matrix );
    r->mvp_dirty = 0;

    r->frame_active = 0;
    r->frame_resolved = 0;

    for( i = 0; i < pixel_count; i++ )
        r->depth_buffer[i] = r->clear_depth;

    return r;
}

void wp_renderer_software_destroy( wp_renderer_software *renderer )
{
    if( !renderer )
        return;

    free( renderer->color_buffer );
    free( renderer->depth_buffer );
    free( renderer->resolve_buffer );
    free( renderer );
}

/* =========================================================================
 * Resize
 * ====================================================================== */

wp_s32 wp_renderer_software_resize( wp_renderer_software *renderer, wp_s32 width, wp_s32 height )
{
    wp_u8 *new_color;
    wp_f32 *new_depth;
    wp_s32 bpp;
    size_t pixel_count, color_bytes, depth_bytes, i;

    if( !renderer )
        return 0;

    bpp = sw_bytes_per_pixel( renderer->format );
    if( !sw_get_buffer_sizes( width, height, bpp, &pixel_count, &color_bytes, &depth_bytes ) )
        return 0;

    new_color = (wp_u8 *)malloc( color_bytes );
    if( !new_color )
        return 0;

    new_depth = (wp_f32 *)malloc( depth_bytes );
    if( !new_depth )
    {
        free( new_color );
        return 0;
    }

    free( renderer->color_buffer );
    free( renderer->depth_buffer );
    free( renderer->resolve_buffer );

    renderer->color_buffer = new_color;
    renderer->depth_buffer = new_depth;
    renderer->resolve_buffer = NULL;
    renderer->width = width;
    renderer->height = height;
    renderer->bytes_per_pixel = bpp;

    renderer->viewport.x = 0;
    renderer->viewport.y = 0;
    renderer->viewport.width = width;
    renderer->viewport.height = height;

    renderer->scissor = sw_clamp_rect( renderer->scissor, width, height );
    renderer->frame_resolved = 0;

    memset( renderer->color_buffer, 0, color_bytes );

    for( i = 0; i < pixel_count; i++ )
        renderer->depth_buffer[i] = renderer->clear_depth;

    return 1;
}

static wp_f32 sw_luminance( wp_f32 r, wp_f32 g, wp_f32 b )
{
    return r * 0.299f + g * 0.587f + b * 0.114f;
}

/*
 * A small, edge-directed resolve for the immediate-mode framebuffer.  It
 * leaves flat areas and axis-aligned edges untouched, while smoothing the
 * stair steps on diagonal high-contrast edges.  The source copy is required
 * so that every output pixel observes the same, unresolved neighbourhood.
 */
static void sw_resolve_antialiasing( wp_renderer_software *renderer )
{
    size_t color_bytes;
    wp_s32 x, y;

    if( renderer->width < 3 || renderer->height < 3 ||
        !sw_get_buffer_sizes( renderer->width, renderer->height, renderer->bytes_per_pixel, NULL,
                              &color_bytes, NULL ) )
        return;

    if( !renderer->resolve_buffer )
    {
        renderer->resolve_buffer = (wp_u8 *)malloc( color_bytes );
        if( !renderer->resolve_buffer )
            return;
    }

    memcpy( renderer->resolve_buffer, renderer->color_buffer, color_bytes );

    for( y = 1; y < renderer->height - 1; y++ )
    {
        for( x = 1; x < renderer->width - 1; x++ )
        {
            wp_f32 cr, cg, cb, ca;
            wp_f32 nr, ng, nb, na, sr, sg, sb, sa;
            wp_f32 er, eg, eb, ea, wr, wg, wb, wa;
            wp_f32 ner, neg, neb, nwr, nwg, nwb;
            wp_f32 ser, seg, seb, swr, swg, swb, unused_alpha;
            wp_f32 lc, ln, ls, le, lw, lne, lnw, lse, lsw;
            wp_f32 luma_min, luma_max, contrast, gradient_x, gradient_y;
            wp_f32 out_r, out_g, out_b, out_a;

            sw_read_pixel_from( renderer, renderer->resolve_buffer, x, y, &cr, &cg, &cb, &ca );
            sw_read_pixel_from( renderer, renderer->resolve_buffer, x, y - 1, &nr, &ng, &nb, &na );
            sw_read_pixel_from( renderer, renderer->resolve_buffer, x, y + 1, &sr, &sg, &sb, &sa );
            sw_read_pixel_from( renderer, renderer->resolve_buffer, x + 1, y, &er, &eg, &eb, &ea );
            sw_read_pixel_from( renderer, renderer->resolve_buffer, x - 1, y, &wr, &wg, &wb, &wa );
            sw_read_pixel_from( renderer, renderer->resolve_buffer, x + 1, y - 1, &ner, &neg, &neb,
                                &unused_alpha );
            sw_read_pixel_from( renderer, renderer->resolve_buffer, x - 1, y - 1, &nwr, &nwg, &nwb,
                                &unused_alpha );
            sw_read_pixel_from( renderer, renderer->resolve_buffer, x + 1, y + 1, &ser, &seg, &seb,
                                &unused_alpha );
            sw_read_pixel_from( renderer, renderer->resolve_buffer, x - 1, y + 1, &swr, &swg, &swb,
                                &unused_alpha );

            lc = sw_luminance( cr, cg, cb );
            ln = sw_luminance( nr, ng, nb );
            ls = sw_luminance( sr, sg, sb );
            le = sw_luminance( er, eg, eb );
            lw = sw_luminance( wr, wg, wb );
            lne = sw_luminance( ner, neg, neb );
            lnw = sw_luminance( nwr, nwg, nwb );
            lse = sw_luminance( ser, seg, seb );
            lsw = sw_luminance( swr, swg, swb );

            luma_min = SW_MIN( lc, SW_MIN( SW_MIN( ln, ls ), SW_MIN( le, lw ) ) );
            luma_max = SW_MAX( lc, SW_MAX( SW_MAX( ln, ls ), SW_MAX( le, lw ) ) );
            contrast = luma_max - luma_min;
            if( contrast < SW_MAX( 1.0f / 32.0f, luma_max * 0.125f ) )
                continue;

            gradient_x = fabsf( ( lne + 2.0f * le + lse ) - ( lnw + 2.0f * lw + lsw ) );
            gradient_y = fabsf( ( lsw + 2.0f * ls + lse ) - ( lnw + 2.0f * ln + lne ) );

            if( gradient_x > gradient_y )
            {
                /* Vertical edge: blend along it, not across it. */
                out_r = cr * 0.5f + ( nr + sr ) * 0.25f;
                out_g = cg * 0.5f + ( ng + sg ) * 0.25f;
                out_b = cb * 0.5f + ( nb + sb ) * 0.25f;
                out_a = ca * 0.5f + ( na + sa ) * 0.25f;
            }
            else
            {
                /* Horizontal edge: blend along it, not across it. */
                out_r = cr * 0.5f + ( wr + er ) * 0.25f;
                out_g = cg * 0.5f + ( wg + eg ) * 0.25f;
                out_b = cb * 0.5f + ( wb + eb ) * 0.25f;
                out_a = ca * 0.5f + ( wa + ea ) * 0.25f;
            }

            sw_write_pixel_raw( renderer, x, y, out_r, out_g, out_b, out_a );
        }
    }
}

/* =========================================================================
 * Frame lifecycle
 * ====================================================================== */

void wp_renderer_software_begin_frame( wp_renderer_software *renderer )
{
    if( !renderer )
        return;

    renderer->frame_active = 1;
    renderer->frame_resolved = 0;
    sw_update_mvp( renderer );
}

void wp_renderer_software_render( wp_renderer_software *renderer, enum wp_anti_aliasing anti_aliasing )
{
    if( !renderer || anti_aliasing != WORKPHONE_ANTI_ALIASING_ON || renderer->frame_resolved )
        return;

    sw_resolve_antialiasing( renderer );
    renderer->frame_resolved = 1;
}

void wp_renderer_software_end_frame( wp_renderer_software *renderer )
{
    if( !renderer )
        return;

    renderer->frame_active = 0;
}

/* =========================================================================
 * Clear
 * ====================================================================== */

void wp_renderer_software_set_clear_color( wp_renderer_software *renderer, wp_f32 r, wp_f32 g, wp_f32 b,
                                           wp_f32 a )
{
    if( !renderer )
        return;

    renderer->clear_r = SW_CLAMP( r, 0.0f, 1.0f );
    renderer->clear_g = SW_CLAMP( g, 0.0f, 1.0f );
    renderer->clear_b = SW_CLAMP( b, 0.0f, 1.0f );
    renderer->clear_a = SW_CLAMP( a, 0.0f, 1.0f );
}

void wp_renderer_software_set_clear_depth( wp_renderer_software *renderer, wp_f32 depth )
{
    if( !renderer )
        return;

    renderer->clear_depth = SW_CLAMP( depth, 0.0f, 1.0f );
}

void wp_renderer_software_clear( wp_renderer_software *renderer, wp_u32 flags )
{
    wp_s32 x0, y0, x1, y1, x, y, bpp;
    size_t idx;
    wp_u8 ur, ug, ub, ua;

    if( !renderer )
        return;

    if( renderer->scissor_enabled )
    {
        x0 = renderer->scissor.x;
        y0 = renderer->scissor.y;
        x1 = renderer->scissor.x + renderer->scissor.width;
        y1 = renderer->scissor.y + renderer->scissor.height;
    }
    else
    {
        x0 = 0;
        y0 = 0;
        x1 = renderer->width;
        y1 = renderer->height;
    }

    x0 = SW_MAX( x0, 0 );
    y0 = SW_MAX( y0, 0 );
    x1 = SW_MIN( x1, renderer->width );
    y1 = SW_MIN( y1, renderer->height );

    if( x0 >= x1 || y0 >= y1 || !( flags & WORKPHONE_CLEAR_FLAG_ALL ) )
        return;

    bpp = renderer->bytes_per_pixel;
    ur = (wp_u8)( SW_CLAMP( renderer->clear_r, 0.0f, 1.0f ) * 255.0f );
    ug = (wp_u8)( SW_CLAMP( renderer->clear_g, 0.0f, 1.0f ) * 255.0f );
    ub = (wp_u8)( SW_CLAMP( renderer->clear_b, 0.0f, 1.0f ) * 255.0f );
    ua = (wp_u8)( SW_CLAMP( renderer->clear_a, 0.0f, 1.0f ) * 255.0f );
    renderer->frame_resolved = 0;

    for( y = y0; y < y1; y++ )
    {
        for( x = x0; x < x1; x++ )
        {
            idx = (size_t)y * (size_t)renderer->width + (size_t)x;

            if( flags & WORKPHONE_CLEAR_FLAG_COLOR )
            {
                wp_u8 *p = renderer->color_buffer + idx * bpp;
                switch( renderer->format )
                {
                case WORKPHONE_PIXEL_FORMAT_RGBA8:
                    p[0] = ur;
                    p[1] = ug;
                    p[2] = ub;
                    p[3] = ua;
                    break;
                case WORKPHONE_PIXEL_FORMAT_BGRA8:
                    p[0] = ub;
                    p[1] = ug;
                    p[2] = ur;
                    p[3] = ua;
                    break;
                case WORKPHONE_PIXEL_FORMAT_RGB8:
                    p[0] = ur;
                    p[1] = ug;
                    p[2] = ub;
                    break;
                case WORKPHONE_PIXEL_FORMAT_BGR8:
                    p[0] = ub;
                    p[1] = ug;
                    p[2] = ur;
                    break;
                default:
                    break;
                }
            }

            if( flags & WORKPHONE_CLEAR_FLAG_DEPTH )
                renderer->depth_buffer[idx] = renderer->clear_depth;
        }
    }
}

/* =========================================================================
 * Viewport and scissor
 * ====================================================================== */

void wp_renderer_software_set_viewport( wp_renderer_software *renderer, wp_viewport_i viewport )
{
    if( !renderer )
        return;

    renderer->viewport = sw_clamp_rect( viewport, renderer->width, renderer->height );
}

wp_viewport_i wp_renderer_software_get_viewport( const wp_renderer_software *renderer )
{
    wp_viewport_i zero;

    if( !renderer )
    {
        memset( &zero, 0, sizeof( wp_viewport_i ) );
        return zero;
    }

    return renderer->viewport;
}

void wp_renderer_software_set_scissor_enabled( wp_renderer_software *renderer, wp_s32 enabled )
{
    if( !renderer )
        return;

    renderer->scissor_enabled = enabled != 0;
}

void wp_renderer_software_set_scissor_rect( wp_renderer_software *renderer, wp_viewport_i scissor )
{
    if( !renderer )
        return;

    renderer->scissor = sw_clamp_rect( scissor, renderer->width, renderer->height );
}

/* =========================================================================
 * Framebuffer access
 * ====================================================================== */

const void *wp_renderer_software_get_framebuffer( const wp_renderer_software *renderer )
{
    return renderer ? renderer->color_buffer : NULL;
}

wp_s32 wp_renderer_software_set_framebuffer( wp_renderer_software *renderer, const void *pixels,
                                             wp_pixel_format format )
{
    size_t bytes;
    if( !renderer || !pixels || format != renderer->format )
        return 0;
    bytes = (size_t)renderer->width * (size_t)renderer->height * (size_t)renderer->bytes_per_pixel;
    memcpy( renderer->color_buffer, pixels, bytes );
    return 1;
}

const wp_f32 *wp_renderer_software_get_depth_buffer( const wp_renderer_software *renderer )
{
    return renderer ? renderer->depth_buffer : NULL;
}

wp_s32 wp_renderer_software_get_width( const wp_renderer_software *renderer )
{
    return renderer ? renderer->width : 0;
}

wp_s32 wp_renderer_software_get_height( const wp_renderer_software *renderer )
{
    return renderer ? renderer->height : 0;
}

wp_pixel_format wp_renderer_software_get_pixel_format( const wp_renderer_software *renderer )
{
    return renderer ? renderer->format : WORKPHONE_PIXEL_FORMAT_RGBA8;
}

/* =========================================================================
 * Render state
 * ====================================================================== */

void wp_renderer_software_set_blend_mode( wp_renderer_software *renderer, wp_blend_mode mode )
{
    if( !renderer || mode < WORKPHONE_BLEND_MODE_NONE || mode > WORKPHONE_BLEND_MODE_MULTIPLY )
        return;

    renderer->blend_mode = mode;
}

wp_blend_mode wp_renderer_software_get_blend_mode( const wp_renderer_software *renderer )
{
    return renderer ? renderer->blend_mode : WORKPHONE_BLEND_MODE_NONE;
}

void wp_renderer_software_set_fill_mode( wp_renderer_software *renderer, wp_fill_mode mode )
{
    if( !renderer || mode < WORKPHONE_FILL_MODE_SOLID || mode > WORKPHONE_FILL_MODE_POINT )
        return;

    renderer->fill_mode = mode;
}

wp_fill_mode wp_renderer_software_get_fill_mode( const wp_renderer_software *renderer )
{
    return renderer ? renderer->fill_mode : WORKPHONE_FILL_MODE_SOLID;
}

void wp_renderer_software_set_cull_mode( wp_renderer_software *renderer, wp_cull_mode mode )
{
    if( !renderer || mode < WORKPHONE_CULL_MODE_NONE || mode > WORKPHONE_CULL_MODE_FRONT )
        return;

    renderer->cull_mode = mode;
}

wp_cull_mode wp_renderer_software_get_cull_mode( const wp_renderer_software *renderer )
{
    return renderer ? renderer->cull_mode : WORKPHONE_CULL_MODE_BACK;
}

void wp_renderer_software_set_depth_test_enabled( wp_renderer_software *renderer, wp_s32 enabled )
{
    if( !renderer )
        return;

    renderer->depth_test_enabled = enabled != 0;
}

wp_s32 wp_renderer_software_get_depth_test_enabled( const wp_renderer_software *renderer )
{
    return renderer ? renderer->depth_test_enabled : 0;
}

void wp_renderer_software_set_depth_write_enabled( wp_renderer_software *renderer, wp_s32 enabled )
{
    if( !renderer )
        return;

    renderer->depth_write_enabled = enabled != 0;
}

wp_s32 wp_renderer_software_get_depth_write_enabled( const wp_renderer_software *renderer )
{
    return renderer ? renderer->depth_write_enabled : 0;
}

void wp_renderer_software_set_depth_func( wp_renderer_software *renderer, wp_depth_func func )
{
    if( !renderer || func < WORKPHONE_DEPTH_FUNC_NEVER || func > WORKPHONE_DEPTH_FUNC_ALWAYS )
        return;

    renderer->depth_func = func;
}

wp_depth_func wp_renderer_software_get_depth_func( const wp_renderer_software *renderer )
{
    return renderer ? renderer->depth_func : WORKPHONE_DEPTH_FUNC_LESS;
}

/* =========================================================================
 * Transform matrices
 * ====================================================================== */

void wp_renderer_software_set_world_matrix( wp_renderer_software *renderer, const wp_mat4f *mat )
{
    if( !renderer || !mat )
        return;

    renderer->world_matrix = *mat;
    renderer->mvp_dirty = 1;
}

void wp_renderer_software_set_view_matrix( wp_renderer_software *renderer, const wp_mat4f *mat )
{
    if( !renderer || !mat )
        return;

    renderer->view_matrix = *mat;
    renderer->mvp_dirty = 1;
}

void wp_renderer_software_set_projection_matrix( wp_renderer_software *renderer, const wp_mat4f *mat )
{
    if( !renderer || !mat )
        return;

    renderer->proj_matrix = *mat;
    renderer->mvp_dirty = 1;
}

void wp_renderer_software_get_world_matrix( const wp_renderer_software *renderer, wp_mat4f *mat )
{
    if( renderer && mat )
        *mat = renderer->world_matrix;
}

void wp_renderer_software_get_view_matrix( const wp_renderer_software *renderer, wp_mat4f *mat )
{
    if( renderer && mat )
        *mat = renderer->view_matrix;
}

void wp_renderer_software_get_projection_matrix( const wp_renderer_software *renderer, wp_mat4f *mat )
{
    if( renderer && mat )
        *mat = renderer->proj_matrix;
}

void wp_renderer_software_set_texture( wp_renderer_software *renderer, const void *pixels, wp_s32 width,
                                       wp_s32 height, wp_pixel_format format )
{
    if( !renderer )
        return;

    if( !pixels || width <= 0 || height <= 0 || sw_bytes_per_pixel( format ) == 0 )
    {
        renderer->texture_pixels = NULL;
        renderer->texture_width = 0;
        renderer->texture_height = 0;
        return;
    }

    renderer->texture_pixels = (const wp_u8 *)pixels;
    renderer->texture_width = width;
    renderer->texture_height = height;
    renderer->texture_format = format;
}

/* =========================================================================
 * Draw calls
 * ====================================================================== */

void wp_renderer_software_draw_triangles_pc( wp_renderer_software *renderer,
                                             const wp_vertex_pc *vertices, wp_s32 vertex_count )
{
    wp_s32 i;

    if( !renderer || !vertices || vertex_count < 3 )
        return;

    for( i = 0; i <= vertex_count - 3; i += 3 )
        sw_draw_triangle_pc( renderer, &vertices[i], &vertices[i + 1], &vertices[i + 2] );
}

void wp_renderer_software_draw_indexed_triangles_pc( wp_renderer_software *renderer,
                                                     const wp_vertex_pc *vertices, wp_s32 vertex_count,
                                                     const uint16_t *indices, wp_s32 index_count )
{
    wp_s32 i;

    if( !renderer || !vertices || !indices || vertex_count <= 0 || index_count < 3 )
        return;

    for( i = 0; i <= index_count - 3; i += 3 )
    {
        if( (wp_s32)indices[i] >= vertex_count || (wp_s32)indices[i + 1] >= vertex_count ||
            (wp_s32)indices[i + 2] >= vertex_count )
            continue;

        sw_draw_triangle_pc( renderer, &vertices[indices[i]], &vertices[indices[i + 1]],
                             &vertices[indices[i + 2]] );
    }
}

void wp_renderer_software_draw_lines_pc( wp_renderer_software *renderer, const wp_vertex_pc *vertices,
                                         wp_s32 vertex_count )
{
    wp_s32 i;
    wp_f32 sx0, sy0, sd0, sx1, sy1, sd1;
    wp_f32 cr0, cg0, cb0, ca0, cr1, cg1, cb1, ca1;

    if( !renderer || !vertices || vertex_count < 2 )
        return;

    for( i = 0; i <= vertex_count - 2; i += 2 )
    {
        if( !sw_transform_vertex( renderer, vertices[i].position, &sx0, &sy0, &sd0 ) )
            continue;
        if( !sw_transform_vertex( renderer, vertices[i + 1].position, &sx1, &sy1, &sd1 ) )
            continue;

        sw_unpack_color( vertices[i].color, &cr0, &cg0, &cb0, &ca0 );
        sw_unpack_color( vertices[i + 1].color, &cr1, &cg1, &cb1, &ca1 );

        sw_draw_line( renderer, sx0, sy0, sd0, cr0, cg0, cb0, ca0, sx1, sy1, sd1, cr1, cg1, cb1, ca1 );
    }
}

void wp_renderer_software_draw_points_pc( wp_renderer_software *renderer, const wp_vertex_pc *vertices,
                                          wp_s32 vertex_count )
{
    wp_s32 i;
    wp_f32 sx, sy, sd, cr, cg, cb, ca;

    if( !renderer || !vertices || vertex_count <= 0 )
        return;

    for( i = 0; i < vertex_count; i++ )
    {
        if( !sw_transform_vertex( renderer, vertices[i].position, &sx, &sy, &sd ) )
            continue;

        sw_unpack_color( vertices[i].color, &cr, &cg, &cb, &ca );
        sw_draw_point( renderer, sx, sy, sd, cr, cg, cb, ca );
    }
}

void wp_renderer_software_draw_triangles_ptc( wp_renderer_software *renderer,
                                              const wp_vertex_ptc *vertices, wp_s32 vertex_count )
{
    wp_s32 i;

    if( !renderer || !vertices || vertex_count < 3 )
        return;

    for( i = 0; i <= vertex_count - 3; i += 3 )
        sw_draw_triangle_ptc( renderer, &vertices[i], &vertices[i + 1], &vertices[i + 2] );
}

void wp_renderer_software_draw_indexed_triangles_ptc( wp_renderer_software *renderer,
                                                      const wp_vertex_ptc *vertices, wp_s32 vertex_count,
                                                      const uint16_t *indices, wp_s32 index_count )
{
    wp_s32 i;

    if( !renderer || !vertices || !indices || vertex_count <= 0 || index_count < 3 )
        return;

    for( i = 0; i <= index_count - 3; i += 3 )
    {
        if( (wp_s32)indices[i] >= vertex_count || (wp_s32)indices[i + 1] >= vertex_count ||
            (wp_s32)indices[i + 2] >= vertex_count )
            continue;

        sw_draw_triangle_ptc( renderer, &vertices[indices[i]], &vertices[indices[i + 1]],
                              &vertices[indices[i + 2]] );
    }
}

void wp_renderer_software_draw_indexed_triangles_ptc_u32( wp_renderer_software *renderer,
                                                          const wp_vertex_ptc *vertices,
                                                          wp_s32 vertex_count, const wp_u32 *indices,
                                                          wp_s32 index_count )
{
    wp_s32 i;

    if( !renderer || !vertices || !indices || vertex_count <= 0 || index_count < 3 )
        return;

    for( i = 0; i <= index_count - 3; i += 3 )
    {
        if( indices[i] >= (wp_u32)vertex_count || indices[i + 1] >= (wp_u32)vertex_count ||
            indices[i + 2] >= (wp_u32)vertex_count )
            continue;

        sw_draw_triangle_ptc( renderer, &vertices[indices[i]], &vertices[indices[i + 1]],
                              &vertices[indices[i + 2]] );
    }
}

/* =========================================================================
 * Native object access
 * ====================================================================== */

void wp_renderer_software_get_native( const wp_renderer_software *renderer, void **pp_object )
{
    if( !pp_object )
        return;

    *pp_object = renderer ? renderer->native : NULL;
}

void wp_renderer_software_set_native( wp_renderer_software *renderer, void *native )
{
    if( !renderer )
        return;

    renderer->native = native;
}
