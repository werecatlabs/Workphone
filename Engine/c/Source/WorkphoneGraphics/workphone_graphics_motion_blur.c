/**
 * @file wp_graphics_motion_blur.c
 * @brief Motion Blur implementation
 * Reference: Claude-of-Duty/src/render/motionblur.js
 */

#include "workphone_graphics_motion_blur.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct wp_motion_blur
{
    wp_s32 width;
    wp_s32 height;
    wp_s32 tile_w;
    wp_s32 tile_h;
    wp_f32 *tile_buffer;
    wp_f32 *output_buffer;
    wp_f32 shutter;
    wp_f32 max_radius;
    wp_f32 intensity;
};

static wp_f32 wp_cl( wp_f32 x, wp_f32 a, wp_f32 b )
{
    return x < a ? a : x > b ? b : x;
}

static wp_f32 wp_ign( wp_f32 x, wp_f32 y )
{
    return (wp_f32)fmod( 52.9829189f * (wp_f32)fmod( x * 0.06711056f + y * 0.00583715f, 1.0f ), 1.0f );
}

static void wp_tile_max( const wp_f32 *vel, wp_s32 vw, wp_s32 vh, wp_s32 tw, wp_s32 th, wp_f32 *out )
{
    for( wp_s32 y = 0; y < th; y++ )
    {
        for( wp_s32 x = 0; x < tw; x++ )
        {
            wp_f32 best_u = 0, best_v = 0, best_len = 0;
            wp_s32 x0 = x * 16, y0 = y * 16;
            for( wp_s32 ty = 0; ty < 8; ty++ )
            {
                for( wp_s32 tx = 0; tx < 8; tx++ )
                {
                    wp_s32 sx = x0 + ( tx * 2 );
                    wp_s32 sy = y0 + ( ty * 2 );
                    if( sx >= vw || sy >= vh )
                        continue;
                    wp_s32 i = ( sy * vw + sx ) * 2;
                    wp_f32 u = vel[i], v = vel[i + 1];
                    wp_f32 len = u * u + v * v;
                    if( len > best_len )
                    {
                        best_len = len;
                        best_u = u;
                        best_v = v;
                    }
                }
            }
            wp_s32 o = ( y * tw + x ) * 2;
            out[o] = best_u;
            out[o + 1] = best_v;
        }
    }
}

static void wp_blur_pass( wp_motion_blur *ctx, const wp_f32 *color, const wp_f32 *vel,
                          const wp_f32 *tile, const wp_f32 *depth, const wp_f32 *normal, wp_s32 frame )
{
    for( wp_s32 y = 0; y < ctx->height; y++ )
    {
        for( wp_s32 x = 0; x < ctx->width; x++ )
        {
            wp_s32 i = ( y * ctx->width + x ) * 3;
            wp_f32 cr = color[i], cg = color[i + 1], cb = color[i + 2];

            wp_f32 px = (wp_f32)x / ctx->width;
            wp_f32 py = (wp_f32)y / ctx->height;

            /* Get velocity from tile */
            wp_s32 tx = x / 16, ty = y / 16;
            wp_s32 ti = ( ty * ctx->tile_w + tx ) * 2;
            wp_f32 vel_u = tile[ti], vel_v = tile[ti + 1];

            /* Check own velocity */
            wp_s32 vi = ( y * ctx->width + x ) * 2;
            wp_f32 own_u = vel[vi], own_v = vel[vi + 1];
            wp_f32 own_len = own_u * own_u + own_v * own_v;
            wp_f32 tile_len = vel_u * vel_u + vel_v * vel_v;
            if( own_len > tile_len )
            {
                vel_u = own_u;
                vel_v = own_v;
                tile_len = own_len;
            }

            /* Scale by shutter */
            wp_f32 sv = vel_u * ctx->shutter;
            wp_f32 svv = vel_v * ctx->shutter;

            wp_f32 px_len = (wp_f32)sqrt( sv * sv + svv * svv ) * ctx->height;
            if( px_len < 1.0f )
            {
                memcpy( ctx->output_buffer + i, color + i, 3 * sizeof( wp_f32 ) );
                continue;
            }
            if( px_len > ctx->max_radius )
            {
                sv *= ctx->max_radius / px_len;
                svv *= ctx->max_radius / px_len;
            }

            /* Get center depth for weighting */
            wp_f32 center_d = depth ? depth[y * ctx->width + x] : 1e5f;
            wp_f32 cov = normal ? normal[( y * ctx->width + x ) * 4 + 2] : 1.0f;
            if( cov < 0.5f )
                center_d = 1e5f;

            /* Accumulate samples */
            wp_f32 sum_r = cr, sum_g = cg, sum_b = cb;
            wp_f32 wsum = 1.0f;
            wp_f32 j = wp_ign( (wp_f32)x + 2.717f * (wp_f32)( frame % 64 ), (wp_f32)y ) - 0.5f;

            for( wp_s32 t = 1; t <= WP_MB_TAPS; t++ )
            {
                wp_f32 tc = ( (wp_f32)t + j ) / (wp_f32)WP_MB_TAPS;
                for( wp_s32 s = 0; s < 2; s++ )
                {
                    wp_f32 ox = ( s == 0 ? 1.0f : -1.0f );
                    wp_f32 ou = px + ox * sv * tc * 0.5f;
                    wp_f32 ov = py + ox * svv * tc * 0.5f;
                    if( ou < 0 || ou >= 1 || ov < 0 || ov >= 1 )
                        continue;

                    wp_s32 sx = (wp_s32)( ou * ctx->width );
                    wp_s32 sy = (wp_s32)( ov * ctx->height );
                    if( sx >= ctx->width )
                        sx = ctx->width - 1;
                    if( sy >= ctx->height )
                        sy = ctx->height - 1;

                    wp_f32 sd = depth ? depth[sy * ctx->width + sx] : 0;
                    wp_f32 sc = normal ? normal[( sy * ctx->width + sx ) * 4 + 2] : 1.0f;
                    if( sc < 0.5f )
                        sd = 1e5f;

                    /* Depth-weighted accumulation */
                    wp_f32 dw =
                        1.0f -
                        wp_cl( (wp_f32)fabs( sd - center_d ) / wp_cl( center_d, 1.0f, 1.5f ), 0, 1 );
                    dw = wp_cl( 0.15f + 0.85f * dw, 0.15f, 1.0f ) * ( 1.0f - tc * 0.35f );

                    wp_s32 si = ( sy * ctx->width + sx ) * 3;
                    sum_r += color[si] * dw;
                    sum_g += color[si + 1] * dw;
                    sum_b += color[si + 2] * dw;
                    wsum += dw;
                }
            }

            wp_f32 o_r = sum_r / wsum;
            wp_f32 o_g = sum_g / wsum;
            wp_f32 o_b = sum_b / wsum;

            ctx->output_buffer[i] = cr + ( o_r - cr ) * ctx->intensity;
            ctx->output_buffer[i + 1] = cg + ( o_g - cg ) * ctx->intensity;
            ctx->output_buffer[i + 2] = cb + ( o_b - cb ) * ctx->intensity;
        }
    }
}

wp_motion_blur *wp_mb_create( wp_s32 w, wp_s32 h )
{
    wp_motion_blur *c = calloc( 1, sizeof( wp_motion_blur ) );
    if( !c )
        return NULL;
    c->width = w;
    c->height = h;
    c->tile_w = ( w + 15 ) / 16;
    c->tile_h = ( h + 15 ) / 16;
    c->tile_buffer = calloc( (size_t)c->tile_w * c->tile_h * 2, sizeof( wp_f32 ) );
    c->output_buffer = calloc( (size_t)w * h * 3, sizeof( wp_f32 ) );
    c->shutter = WP_MB_DEFAULT_SHUTTER;
    c->max_radius = WP_MB_DEFAULT_MAX_RADIUS;
    c->intensity = WP_MB_DEFAULT_INTENSITY;
    return c;
}

void wp_mb_destroy( wp_motion_blur *c )
{
    if( !c )
        return;
    free( c->tile_buffer );
    free( c->output_buffer );
    free( c );
}

void wp_mb_render( wp_motion_blur *c, const wp_f32 *color, const wp_f32 *vel, const wp_f32 *depth,
                   const wp_f32 *normal, wp_s32 frame, wp_f32 shutter )
{
    if( !c )
        return;
    if( shutter >= 0 )
        c->shutter = shutter;
    wp_tile_max( vel, c->width, c->height, c->tile_w, c->tile_h, c->tile_buffer );
    wp_blur_pass( c, color, vel, c->tile_buffer, depth, normal, frame );
}

const wp_f32 *wp_mb_get_texture( const wp_motion_blur *c )
{
    return c ? c->output_buffer : NULL;
}

void wp_mb_resize( wp_motion_blur *c, wp_s32 w, wp_s32 h )
{
    if( !c )
        return;
    c->width = w;
    c->height = h;
    c->tile_w = ( w + 15 ) / 16;
    c->tile_h = ( h + 15 ) / 16;
    c->tile_buffer = realloc( c->tile_buffer, (size_t)c->tile_w * c->tile_h * 2 * sizeof( wp_f32 ) );
    c->output_buffer = realloc( c->output_buffer, (size_t)w * h * 3 * sizeof( wp_f32 ) );
}
