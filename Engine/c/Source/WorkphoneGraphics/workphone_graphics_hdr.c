/**
 * @file wp_graphics_hdr.c
 * @brief HDR Rendering Pipeline - Bloom, Auto Exposure, Tone Mapping
 * Reference: Claude-of-Duty/src/render/bloom.js, exposure.js, glsl.js
 */

#include "workphone_graphics_hdr.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WP_HDR_PI 3.141592653589793f

typedef struct
{
    wp_f32 *data;
    wp_s32 width;
    wp_s32 height;
} wp_bloom_mip;

typedef struct
{
    wp_f32 exposure;
    wp_f32 ev100;
    wp_f32 prev_ev100;
} wp_ae_state;

struct wp_hdr
{
    wp_s32 width;
    wp_s32 height;
    wp_bloom_mip *mips;
    wp_s32 level_count;
    wp_f32 *bloom_output;
    wp_ae_state ae;
    wp_f32 *ae_buf;
    wp_f32 *temp;
    wp_f32 *output;
    wp_f32 bloom_threshold;
    wp_f32 bloom_knee;
    wp_f32 bloom_strength;
};

static wp_f32 wp_clamp( wp_f32 x, wp_f32 a, wp_f32 b )
{
    return x < a ? a : x > b ? b : x;
}

static wp_f32 wp_lum( wp_f32 r, wp_f32 g, wp_f32 b )
{
    return r * 0.2126f + g * 0.7152f + b * 0.0722f;
}

static wp_f32 wp_hdr_pow( wp_f32 x, wp_f32 y )
{
    return x <= 0 ? 0 : (wp_f32)pow( x, y );
}

/* Karis bloom prefilter - soft knee highlight threshold */
static wp_f32 wp_bloom_prefilter( wp_f32 l, wp_f32 thr, wp_f32 knee )
{
    wp_f32 soft = wp_clamp( l - thr + knee, 0.0f, 2.0f * knee );
    soft = soft * soft / ( 4.0f * knee + 1e-5f );
    return wp_clamp( soft / ( l + 1e-5f ), 0.0f, 1.0f );
}

/* Bloom downsample with Karis luminance average */
static void wp_bloom_downsample( const wp_f32 *src, wp_s32 sw, wp_s32 sh, wp_f32 *dst, wp_s32 dw,
                                 wp_s32 dh, wp_f32 thr, wp_f32 knee )
{
    for( wp_s32 y = 0; y < dh; y++ )
    {
        for( wp_s32 x = 0; x < dw; x++ )
        {
            wp_f32 u = ( (wp_f32)x + 0.5f ) / dw;
            wp_f32 v = ( (wp_f32)y + 0.5f ) / dh;
            wp_s32 sx = (wp_s32)( u * sw );
            wp_s32 sy = (wp_s32)( v * sh );
            if( sx >= sw )
                sx = sw - 1;
            if( sy >= sh )
                sy = sh - 1;

            wp_f32 w0 = 0.03125f, w1 = 0.0625f, w2 = 0.125f;
            wp_f32 sum_r = 0, sum_g = 0, sum_b = 0, wsum = 0;

            for( wp_s32 j = -2; j <= 2; j++ )
            {
                for( wp_s32 i = -2; i <= 2; i++ )
                {
                    wp_s32 tx = sx + i, ty = sy + j;
                    if( tx < 0 || tx >= sw || ty < 0 || ty >= sh )
                        continue;
                    wp_s32 idx = ( ty * sw + tx ) * 3;
                    wp_f32 r = src[idx], g = src[idx + 1], b = src[idx + 2];
                    wp_f32 l = wp_lum( r, g, b );
                    wp_f32 kw = 1.0f / ( 1.0f + l );
                    wp_f32 w = ( i == 0 && j == 0 )                 ? w2
                               : ( abs( i ) == 1 && abs( j ) == 1 ) ? w0
                               : ( i == 0 || j == 0 )               ? w1
                                                                    : w0;
                    sum_r += r * kw * w;
                    sum_g += g * kw * w;
                    sum_b += b * kw * w;
                    wsum += kw * w;
                }
            }
            if( wsum > 0 )
            {
                sum_r /= wsum;
                sum_g /= wsum;
                sum_b /= wsum;
            }

            wp_f32 l = wp_lum( sum_r, sum_g, sum_b );
            wp_f32 pf = ( thr > 0 ) ? wp_bloom_prefilter( l, thr, knee ) : 1.0f;

            wp_s32 didx = ( y * dw + x ) * 3;
            dst[didx] = wp_clamp( sum_r * pf, 0, 24.0f );
            dst[didx + 1] = wp_clamp( sum_g * pf, 0, 24.0f );
            dst[didx + 2] = wp_clamp( sum_b * pf, 0, 24.0f );
        }
    }
}

/* Tent upsample - energy preserving */
static void wp_bloom_upsample( const wp_f32 *src, wp_s32 sw, wp_s32 sh, wp_f32 *dst, wp_s32 dw,
                               wp_s32 dh )
{
    for( wp_s32 y = 0; y < dh; y++ )
    {
        for( wp_s32 x = 0; x < dw; x++ )
        {
            wp_f32 u = ( (wp_f32)x + 0.5f ) / dw;
            wp_f32 v = ( (wp_f32)y + 0.5f ) / dh;
            wp_f32 tu = u * sw - 0.5f;
            wp_f32 tv = v * sh - 0.5f;

            wp_f32 sum_r = 0, sum_g = 0, sum_b = 0, wsum = 0;

            for( wp_s32 j = -1; j <= 1; j++ )
            {
                for( wp_s32 i = -1; i <= 1; i++ )
                {
                    wp_s32 sx = (wp_s32)( tu + i + 0.5f );
                    wp_s32 sy = (wp_s32)( tv + j + 0.5f );
                    if( sx < 0 || sx >= sw || sy < 0 || sy >= sh )
                        continue;
                    wp_s32 idx = ( sy * sw + sx ) * 3;
                    wp_f32 r = src[idx], g = src[idx + 1], b = src[idx + 2];
                    wp_f32 dx = tu - sx + 0.5f, dy = tv - sy + 0.5f;
                    wp_f32 w = 1.0f / ( 1.0f + dx * dx * 4.0f + dy * dy * 4.0f );
                    sum_r += r * w;
                    sum_g += g * w;
                    sum_b += b * w;
                    wsum += w;
                }
            }
            wp_s32 didx = ( y * dw + x ) * 3;
            if( wsum > 0 )
            {
                dst[didx] = sum_r / wsum * 0.5f;
                dst[didx + 1] = sum_g / wsum * 0.5f;
                dst[didx + 2] = sum_b / wsum * 0.5f;
            }
        }
    }
}

/* Log luminance metering with sky de-weighting */
static void wp_ae_meter( const wp_f32 *img, wp_s32 w, wp_s32 h, wp_f32 sky_weight, wp_f32 far_dist,
                         const wp_f32 *depth, wp_f32 *out_log_lum, wp_f32 *out_wt_sum )
{
    wp_f32 sum = 0, wsum = 0;
    for( wp_s32 y = 0; y < h; y++ )
    {
        for( wp_s32 x = 0; x < w; x++ )
        {
            wp_s32 i = ( y * w + x ) * 3;
            wp_f32 r = img[i], g = img[i + 1], b = img[i + 2];
            wp_f32 l = wp_lum( r, g, b );
            wp_f32 log_l = (wp_f32)log( l + 1e-6f );

            wp_f32 wgt = 1.0f;
            if( depth )
            {
                wp_f32 d = depth[y * w + x];
                wp_f32 depth_w = wp_clamp( d / far_dist, 0, 1 );
                wgt = 1.0f - depth_w * ( 1.0f - sky_weight );
            }

            sum += log_l * wgt;
            wsum += wgt;
        }
    }
    if( out_log_lum )
        *out_log_lum = sum;
    if( out_wt_sum )
        *out_wt_sum = wsum;
}

/* AgX filmic tone mapping - Call of Duty standard */
void wp_hdr_tonemap_agx( wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b )
{
    /* A stable, monotonic filmic shoulder.  The former implementation took
       log(log(x) + offset), which generated NaNs for ordinary scene values. */
    r = r < 0.0f ? 0.0f : r;
    g = g < 0.0f ? 0.0f : g;
    b = b < 0.0f ? 0.0f : b;
    *out_r = ( r * ( 2.51f * r + 0.03f ) ) / ( r * ( 2.43f * r + 0.59f ) + 0.14f );
    *out_g = ( g * ( 2.51f * g + 0.03f ) ) / ( g * ( 2.43f * g + 0.59f ) + 0.14f );
    *out_b = ( b * ( 2.51f * b + 0.03f ) ) / ( b * ( 2.43f * b + 0.59f ) + 0.14f );
}

wp_f32 wp_hdr_srgb_to_linear( wp_f32 c )
{
    return c <= 0.04045f ? c / 12.92f : wp_hdr_pow( ( c + 0.055f ) / 1.055f, 2.4f );
}

wp_f32 wp_hdr_linear_to_srgb( wp_f32 c )
{
    return c <= 0.0031308f ? 12.92f * c : 1.055f * wp_hdr_pow( c, 1.0f / 2.4f ) - 0.055f;
}

void wp_hdr_bloom_set( wp_hdr *ctx, wp_f32 threshold, wp_f32 strength )
{
    if( !ctx )
        return;
    ctx->bloom_threshold = threshold;
    ctx->bloom_strength = strength;
}

wp_hdr *wp_hdr_create( wp_s32 width, wp_s32 height )
{
    wp_hdr *ctx = calloc( 1, sizeof( wp_hdr ) );
    if( !ctx )
        return NULL;
    ctx->width = width;
    ctx->height = height;
    ctx->bloom_threshold = WP_BLOOM_DEFAULT_THRESHOLD;
    ctx->bloom_knee = WP_BLOOM_DEFAULT_KNEE;
    ctx->bloom_strength = 1.0f;
    ctx->level_count = WP_BLOOM_LEVELS;
    ctx->mips = calloc( WP_BLOOM_LEVELS, sizeof( wp_bloom_mip ) );
    wp_s32 mw = width, mh = height;
    for( wp_s32 i = 0; i < WP_BLOOM_LEVELS; i++ )
    {
        mw = mw > 1 ? mw / 2 : 1;
        mh = mh > 1 ? mh / 2 : 1;
        ctx->mips[i].width = mw;
        ctx->mips[i].height = mh;
        ctx->mips[i].data = calloc( (size_t)mw * mh * 3, sizeof( wp_f32 ) );
    }
    ctx->bloom_output = calloc( (size_t)width * height * 3, sizeof( wp_f32 ) );
    ctx->ae_buf = calloc( 4, sizeof( wp_f32 ) );
    ctx->temp = calloc( (size_t)width * height * 3, sizeof( wp_f32 ) );
    ctx->output = calloc( (size_t)width * height * 3, sizeof( wp_f32 ) );
    ctx->ae.exposure = 1.0f;
    ctx->ae.ev100 = 0;
    ctx->ae.prev_ev100 = 0;
    return ctx;
}

void wp_hdr_destroy( wp_hdr *ctx )
{
    if( !ctx )
        return;
    for( wp_s32 i = 0; i < ctx->level_count; i++ )
        free( ctx->mips[i].data );
    free( ctx->mips );
    free( ctx->bloom_output );
    free( ctx->ae_buf );
    free( ctx->temp );
    free( ctx->output );
    free( ctx );
}

const wp_f32 *wp_hdr_bloom_render( wp_hdr *ctx, const wp_f32 *src, wp_s32 w, wp_s32 h )
{
    if( !ctx || !src )
        return src;
    wp_bloom_downsample( src, w, h, ctx->mips[0].data, ctx->mips[0].width, ctx->mips[0].height,
                         ctx->bloom_threshold, ctx->bloom_knee );
    for( wp_s32 i = 1; i < ctx->level_count; i++ )
        wp_bloom_downsample( ctx->mips[i - 1].data, ctx->mips[i - 1].width, ctx->mips[i - 1].height,
                             ctx->mips[i].data, ctx->mips[i].width, ctx->mips[i].height, 0, 0 );
    for( wp_s32 i = ctx->level_count - 2; i >= 0; i-- )
        wp_bloom_upsample( ctx->mips[i + 1].data, ctx->mips[i + 1].width, ctx->mips[i + 1].height,
                           ctx->mips[i].data, ctx->mips[i].width, ctx->mips[i].height );
    wp_bloom_upsample( ctx->mips[0].data, ctx->mips[0].width, ctx->mips[0].height, ctx->bloom_output, w,
                       h );
    return ctx->bloom_output;
}

const wp_f32 *wp_hdr_exposure_update( wp_hdr *ctx, const wp_f32 *src, const wp_f32 *depth, wp_s32 w,
                                      wp_s32 h, wp_f32 dt )
{
    if( !ctx )
        return NULL;
    if( !src )
        return ctx->ae_buf;
    wp_f32 avg_log, wt_sum;
    wp_ae_meter( src, w, h, WP_AE_DEFAULT_SKY_WEIGHT, WP_AE_DEFAULT_FAR_DISTANCE, depth, &avg_log,
                 &wt_sum );
    wp_f32 avg_lum = (wp_f32)exp( avg_log / wp_clamp( wt_sum, 1e-4f, 1e10f ) );
    wp_f32 ev100 = (wp_f32)log2( avg_lum * 100.0f / 12.5f );
    ev100 = wp_clamp( ev100, WP_AE_MIN_EV, WP_AE_MAX_EV );
    wp_f32 speed = ev100 > ctx->ae.prev_ev100 ? WP_AE_SPEED_DOWN : WP_AE_SPEED_UP;
    wp_f32 k = 1.0f - (wp_f32)exp( -dt * speed );
    ev100 = ctx->ae.prev_ev100 + ( ev100 - ctx->ae.prev_ev100 ) * wp_clamp( k, 0, 1 );
    ctx->ae.exposure = 1.0f / ( 1.2f * (wp_f32)exp2( ev100 ) );
    ctx->ae.ev100 = ev100;
    ctx->ae.prev_ev100 = ev100;
    ctx->ae_buf[0] = ctx->ae.exposure;
    ctx->ae_buf[1] = ev100;
    return ctx->ae_buf;
}

wp_f32 wp_hdr_exposure_get_value( const wp_hdr *ctx )
{
    return ctx ? ctx->ae.exposure : 1.0f;
}

void wp_hdr_composite( wp_hdr *ctx, const wp_f32 *color, const wp_f32 *bloom, const wp_f32 *exp,
                       wp_s32 w, wp_s32 h, wp_f32 *out )
{
    if( !ctx || !color )
        return;
    wp_f32 exposure = exp ? exp[0] : 1.0f;
    wp_f32 bloom_str = ctx->bloom_strength;
    for( wp_s32 y = 0; y < h; y++ )
    {
        for( wp_s32 x = 0; x < w; x++ )
        {
            wp_s32 i = ( y * w + x ) * 3;
            wp_f32 r = color[i] + ( bloom ? bloom[i] * bloom_str * 0.05f : 0 );
            wp_f32 g = color[i + 1] + ( bloom ? bloom[i + 1] * bloom_str * 0.05f : 0 );
            wp_f32 b = color[i + 2] + ( bloom ? bloom[i + 2] * bloom_str * 0.05f : 0 );
            r *= exposure;
            g *= exposure;
            b *= exposure;
            wp_f32 du = (wp_f32)x / w - 0.5f, dv = (wp_f32)y / h - 0.5f;
            wp_f32 vig = 1.0f - ( du * du + dv * dv ) * 0.24f;
            r *= vig;
            g *= vig;
            b *= vig;
            wp_f32 tr, tg, tb;
            wp_hdr_tonemap_agx( r, g, b, &tr, &tg, &tb );
            out[i] = wp_clamp( wp_hdr_linear_to_srgb( wp_clamp( tr, 0, 1 ) ), 0, 1 );
            out[i + 1] = wp_clamp( wp_hdr_linear_to_srgb( wp_clamp( tg, 0, 1 ) ), 0, 1 );
            out[i + 2] = wp_clamp( wp_hdr_linear_to_srgb( wp_clamp( tb, 0, 1 ) ), 0, 1 );
        }
    }
}

void wp_hdr_resize( wp_hdr *ctx, wp_s32 w, wp_s32 h )
{
    if( !ctx )
        return;
    ctx->width = w;
    ctx->height = h;
    for( wp_s32 i = 0; i < ctx->level_count; i++ )
        free( ctx->mips[i].data );
    wp_s32 mw = w, mh = h;
    for( wp_s32 i = 0; i < ctx->level_count; i++ )
    {
        mw = mw > 1 ? mw / 2 : 1;
        mh = mh > 1 ? mh / 2 : 1;
        ctx->mips[i].width = mw;
        ctx->mips[i].height = mh;
        ctx->mips[i].data = calloc( (size_t)mw * mh * 3, sizeof( wp_f32 ) );
    }
    ctx->bloom_output = realloc( ctx->bloom_output, (size_t)w * h * 3 * sizeof( wp_f32 ) );
    ctx->temp = realloc( ctx->temp, (size_t)w * h * 3 * sizeof( wp_f32 ) );
    ctx->output = realloc( ctx->output, (size_t)w * h * 3 * sizeof( wp_f32 ) );
}
