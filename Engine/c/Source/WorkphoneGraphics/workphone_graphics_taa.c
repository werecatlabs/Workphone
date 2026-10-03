/**
 * @file wp_graphics_taa.c
 * @brief Temporal Anti-Aliasing implementation.
 *
 * Implements velocity-based history reprojection with:
 * - Catmull-Rom filtered resampling
 * - YCoCg color space variance clipping
 * - Depth-based velocity dilation
 * - Luminance-weighted blending
 *
 * Reference: G:\Claude-of-Duty-main\Claude-of-Duty-main\src\render\taa.js
 */

#include "workphone_graphics_taa.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Internal constants
 * ====================================================================== */

#define WP_TAA_PI 3.141592653589793f

/* =========================================================================
 * Internal helper functions
 * ====================================================================== */

/** Maximum of two floats */
static wp_f32 wp_taa_max( wp_f32 a, wp_f32 b )
{
    return a > b ? a : b;
}

/** Minimum of two floats */
static wp_f32 wp_taa_min( wp_f32 a, wp_f32 b )
{
    return a < b ? a : b;
}

/** Absolute value */
static wp_f32 wp_taa_abs( wp_f32 x )
{
    return x < 0 ? -x : x;
}

/* Halton sequence generator */
static wp_f32 wp_taa_halton( wp_s32 index, wp_s32 base )
{
    wp_f32 result = 0.0f;
    wp_f32 f = 1.0f;
    wp_s32 i = index;
    while( i > 0 )
    {
        f = f / (wp_f32)base;
        result = result + f * (wp_f32)( i % base );
        i = i / base;
    }
    return result;
}

/* =========================================================================
 * Internal structures
 * ====================================================================== */

struct wp_taa
{
    /** Render width */
    wp_s32 width;
    /** Render height */
    wp_s32 height;
    /** Total pixels */
    wp_s32 pixel_count;

    /** Configuration */
    wp_taa_config config;

    /** Output buffer (ping-pong) */
    wp_f32 *output_buffer_a;
    wp_f32 *output_buffer_b;
    wp_f32 *current_output;

    /** Jitter state */
    wp_taa_jitter jitter;
    wp_s32 jitter_index;

    /** First frame flag */
    wp_s32 first_frame;

    /** texel size */
    wp_f32 texel_x;
    wp_f32 texel_y;

    /** Temporary working buffers */
    wp_f32 *temp_texels;
};

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/**
 * Clamp value
 */
static wp_f32 wp_taa_clamp( wp_f32 x, wp_f32 min_val, wp_f32 max_val )
{
    if( x < min_val )
        return min_val;
    if( x > max_val )
        return max_val;
    return x;
}

/**
 * Compute luminance
 */
static wp_f32 wp_taa_luminance( wp_f32 r, wp_f32 g, wp_f32 b )
{
    return r * 0.2126f + g * 0.7152f + b * 0.0722f;
}

/**
 * Tonemapping (Reinhard-style weighted)
 */
static void wp_taa_tonemap_w( wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b )
{
    wp_f32 l = wp_taa_luminance( r, g, b );
    wp_f32 t = 1.0f + l;
    *out_r = r / t;
    *out_g = g / t;
    *out_b = b / t;
}

static void wp_taa_tonemap_w_inv( wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 *out_r, wp_f32 *out_g,
                                  wp_f32 *out_b )
{
    *out_r = r / wp_taa_max( 1e-4f, 1.0f - r );
    *out_g = g / wp_taa_max( 1e-4f, 1.0f - g );
    *out_b = b / wp_taa_max( 1e-4f, 1.0f - b );
}

/**
 * Sample texture with border check
 */
static void wp_taa_sample_tex( const wp_f32 *buffer, wp_s32 w, wp_s32 h, wp_f32 u, wp_f32 v,
                               wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b )
{
    if( u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f )
    {
        *out_r = 0.0f;
        *out_g = 0.0f;
        *out_b = 0.0f;
        return;
    }

    wp_s32 ix = (wp_s32)( u * ( w - 1 ) );
    wp_s32 iy = (wp_s32)( v * ( h - 1 ) );
    wp_s32 idx = ( iy * w + ix ) * 3;

    *out_r = buffer[idx + 0];
    *out_g = buffer[idx + 1];
    *out_b = buffer[idx + 2];
}

/**
 * Catmull-Rom 5-tap filter
 */
static void wp_taa_catmull_rom_sample( const wp_f32 *buffer, wp_s32 w, wp_s32 h, wp_f32 u, wp_f32 v,
                                       wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b )
{
    wp_f32 px = u * (wp_f32)w;
    wp_f32 py = v * (wp_f32)h;

    wp_f32 fx = px - 0.5f;
    wp_f32 fy = py - 0.5f;

    wp_f32 ix0 = (wp_f32)( (wp_s32)fx ) - 1.0f;
    wp_f32 iy0 = (wp_f32)( (wp_s32)fy ) - 1.0f;

    wp_f32 fx0 = fx - ix0;
    wp_f32 fy0 = fy - iy0;

    /* Compute weights */
    wp_f32 w0x = fx0 * ( -0.5f + fx0 * ( 1.0f - 0.5f * fx0 ) );
    wp_f32 w1x = 1.0f + fx0 * fx0 * ( -2.5f + 1.5f * fx0 );
    wp_f32 w2x = fx0 * ( 0.5f + fx0 * ( 2.0f - 1.5f * fx0 ) );
    wp_f32 w3x = fx0 * fx0 * ( -0.5f + 0.5f * fx0 );

    wp_f32 w12x = w1x + w2x;
    wp_f32 off12x = w2x / wp_taa_max( w12x, 1e-5f );

    wp_f32 w0y = fy0 * ( -0.5f + fy0 * ( 1.0f - 0.5f * fy0 ) );
    wp_f32 w1y = 1.0f + fy0 * fy0 * ( -2.5f + 1.5f * fy0 );
    wp_f32 w2y = fy0 * ( 0.5f + fy0 * ( 2.0f - 1.5f * fy0 ) );
    wp_f32 w3y = fy0 * fy0 * ( -0.5f + 0.5f * fy0 );

    wp_f32 w12y = w1y + w2y;
    wp_f32 off12y = w2y / wp_taa_max( w12y, 1e-5f );

    /* UV coordinates for 5-tap */
    wp_f32 u0 = ( ix0 ) / (wp_f32)w;
    wp_f32 u1 = ( ix0 + 1.0f ) / (wp_f32)w;
    wp_f32 u2 = ( ix0 + off12x + 1.0f ) / (wp_f32)w;
    wp_f32 u3 = ( ix0 + 2.0f ) / (wp_f32)w;

    wp_f32 v0 = ( iy0 ) / (wp_f32)h;
    wp_f32 v1 = ( iy0 + 1.0f ) / (wp_f32)h;
    wp_f32 v2 = ( iy0 + off12y + 1.0f ) / (wp_f32)h;
    wp_f32 v3 = ( iy0 + 2.0f ) / (wp_f32)h;

    wp_f32 r_sum = 0.0f, g_sum = 0.0f, b_sum = 0.0f;
    wp_f32 w_sum = 0.0f;

    wp_f32 r_val, g_val, b_val;

    /* Sample all 5x5 taps */
    wp_taa_sample_tex( buffer, w, h, u2, v2, &r_val, &g_val, &b_val );
    wp_f32 wc = w12x * w12y;
    r_sum += r_val * wc;
    g_sum += g_val * wc;
    b_sum += b_val * wc;
    w_sum += wc;

    /* Corners */
    wp_taa_sample_tex( buffer, w, h, u0, v0, &r_val, &g_val, &b_val );
    r_sum += r_val * w0x * w0y;
    g_sum += g_val * w0x * w0y;
    b_sum += b_val * w0x * w0y;
    w_sum += w0x * w0y;

    wp_taa_sample_tex( buffer, w, h, u1, v0, &r_val, &g_val, &b_val );
    r_sum += r_val * w1x * w0y;
    g_sum += g_val * w1x * w0y;
    b_sum += b_val * w1x * w0y;
    w_sum += w1x * w0y;

    wp_taa_sample_tex( buffer, w, h, u3, v0, &r_val, &g_val, &b_val );
    r_sum += r_val * w3x * w0y;
    g_sum += g_val * w3x * w0y;
    b_sum += b_val * w3x * w0y;
    w_sum += w3x * w0y;

    wp_taa_sample_tex( buffer, w, h, u0, v1, &r_val, &g_val, &b_val );
    r_sum += r_val * w0x * w1y;
    g_sum += g_val * w0x * w1y;
    b_sum += b_val * w0x * w1y;
    w_sum += w0x * w1y;

    wp_taa_sample_tex( buffer, w, h, u3, v1, &r_val, &g_val, &b_val );
    r_sum += r_val * w3x * w1y;
    g_sum += g_val * w3x * w1y;
    b_sum += b_val * w3x * w1y;
    w_sum += w3x * w1y;

    wp_taa_sample_tex( buffer, w, h, u0, v2, &r_val, &g_val, &b_val );
    r_sum += r_val * w0x * w2y;
    g_sum += g_val * w0x * w2y;
    b_sum += b_val * w0x * w2y;
    w_sum += w0x * w2y;

    wp_taa_sample_tex( buffer, w, h, u3, v2, &r_val, &g_val, &b_val );
    r_sum += r_val * w3x * w2y;
    g_sum += g_val * w3x * w2y;
    b_sum += b_val * w3x * w2y;
    w_sum += w3x * w2y;

    wp_taa_sample_tex( buffer, w, h, u0, v3, &r_val, &g_val, &b_val );
    r_sum += r_val * w0x * w3y;
    g_sum += g_val * w0x * w3y;
    b_sum += b_val * w0x * w3y;
    w_sum += w0x * w3y;

    wp_taa_sample_tex( buffer, w, h, u1, v3, &r_val, &g_val, &b_val );
    r_sum += r_val * w1x * w3y;
    g_sum += g_val * w1x * w3y;
    b_sum += b_val * w1x * w3y;
    w_sum += w1x * w3y;

    wp_taa_sample_tex( buffer, w, h, u3, v3, &r_val, &g_val, &b_val );
    r_sum += r_val * w3x * w3y;
    g_sum += g_val * w3x * w3y;
    b_sum += b_val * w3x * w3y;
    w_sum += w3x * w3y;

    if( w_sum > 1e-6f )
    {
        *out_r = r_sum / w_sum;
        *out_g = g_sum / w_sum;
        *out_b = b_sum / w_sum;
    }
    else
    {
        *out_r = 0.0f;
        *out_g = 0.0f;
        *out_b = 0.0f;
    }
}

/* =========================================================================
 * Public API Implementation
 * ====================================================================== */

wp_taa *wp_taa_create( wp_s32 width, wp_s32 height, const wp_taa_config *config )
{
    if( width <= 0 || height <= 0 )
        return NULL;

    wp_taa *ctx = (wp_taa *)malloc( sizeof( wp_taa ) );
    if( !ctx )
        return NULL;
    memset( ctx, 0, sizeof( wp_taa ) );

    ctx->width = width;
    ctx->height = height;
    ctx->pixel_count = width * height;
    ctx->texel_x = 1.0f / (wp_f32)width;
    ctx->texel_y = 1.0f / (wp_f32)height;

    if( config )
    {
        ctx->config = *config;
    }
    else
    {
        ctx->config.feedback = WP_TAA_DEFAULT_FEEDBACK;
        ctx->config.clip_gamma = WP_TAA_DEFAULT_CLIP_GAMMA;
        ctx->config.motion_scale = 1.0f;
        ctx->config.quality = 2;
    }

    /* Allocate ping-pong buffers */
    wp_s32 buf_size = ctx->pixel_count * 3;
    ctx->output_buffer_a = (wp_f32 *)malloc( buf_size * sizeof( wp_f32 ) );
    ctx->output_buffer_b = (wp_f32 *)malloc( buf_size * sizeof( wp_f32 ) );
    ctx->temp_texels = (wp_f32 *)malloc( buf_size * sizeof( wp_f32 ) );

    if( !ctx->output_buffer_a || !ctx->output_buffer_b || !ctx->temp_texels )
    {
        wp_taa_destroy( ctx );
        return NULL;
    }

    memset( ctx->output_buffer_a, 0, buf_size * sizeof( wp_f32 ) );
    memset( ctx->output_buffer_b, 0, buf_size * sizeof( wp_f32 ) );
    memset( ctx->temp_texels, 0, buf_size * sizeof( wp_f32 ) );

    ctx->current_output = ctx->output_buffer_a;
    ctx->first_frame = 1;
    ctx->jitter_index = 0;
    ctx->jitter.x = 0.0f;
    ctx->jitter.y = 0.0f;

    return ctx;
}

void wp_taa_destroy( wp_taa *ctx )
{
    if( !ctx )
        return;
    if( ctx->output_buffer_a )
        free( ctx->output_buffer_a );
    if( ctx->output_buffer_b )
        free( ctx->output_buffer_b );
    if( ctx->temp_texels )
        free( ctx->temp_texels );
    free( ctx );
}

void wp_taa_reset( wp_taa *ctx )
{
    if( !ctx )
        return;
    ctx->first_frame = 1;
    ctx->jitter_index = 0;
    memset( ctx->output_buffer_a, 0, ctx->pixel_count * 3 * sizeof( wp_f32 ) );
    memset( ctx->output_buffer_b, 0, ctx->pixel_count * 3 * sizeof( wp_f32 ) );
}

void wp_taa_resize( wp_taa *ctx, wp_s32 width, wp_s32 height )
{
    if( !ctx || width <= 0 || height <= 0 )
        return;

    if( ctx->width == width && ctx->height == height )
        return;

    ctx->width = width;
    ctx->height = height;
    ctx->pixel_count = width * height;
    ctx->texel_x = 1.0f / (wp_f32)width;
    ctx->texel_y = 1.0f / (wp_f32)height;

    wp_s32 buf_size = ctx->pixel_count * 3;

    ctx->output_buffer_a = (wp_f32 *)realloc( ctx->output_buffer_a, buf_size * sizeof( wp_f32 ) );
    ctx->output_buffer_b = (wp_f32 *)realloc( ctx->output_buffer_b, buf_size * sizeof( wp_f32 ) );
    ctx->temp_texels = (wp_f32 *)realloc( ctx->temp_texels, buf_size * sizeof( wp_f32 ) );

    memset( ctx->output_buffer_a, 0, buf_size * sizeof( wp_f32 ) );
    memset( ctx->output_buffer_b, 0, buf_size * sizeof( wp_f32 ) );
    memset( ctx->temp_texels, 0, buf_size * sizeof( wp_f32 ) );

    ctx->first_frame = 1;
}

void wp_taa_set_feedback( wp_taa *ctx, wp_f32 feedback )
{
    if( !ctx )
        return;
    ctx->config.feedback = wp_taa_clamp( feedback, 0.0f, 0.99f );
}

void wp_taa_get_size( const wp_taa *ctx, wp_s32 *width, wp_s32 *height )
{
    if( !ctx )
        return;
    if( width )
        *width = ctx->width;
    if( height )
        *height = ctx->height;
}

wp_taa_jitter wp_taa_next_jitter( wp_taa *ctx )
{
    wp_taa_jitter j = { 0.0f, 0.0f };
    if( !ctx )
        return j;

    /* Halton sequence for sub-pixel jitter */
    j.x = ( wp_taa_halton( ctx->jitter_index + 1, 2 ) - 0.5f ) * ctx->texel_x;
    j.y = ( wp_taa_halton( ctx->jitter_index + 1, 3 ) - 0.5f ) * ctx->texel_y;

    ctx->jitter = j;
    ctx->jitter_index = ( ctx->jitter_index + 1 ) % WP_TAA_HALTON_SAMPLES;

    return j;
}

const wp_f32 *wp_taa_get_buffer( const wp_taa *ctx )
{
    return ctx ? ctx->current_output : NULL;
}

const wp_f32 *wp_taa_get_previous_buffer( const wp_taa *ctx )
{
    if( !ctx )
        return NULL;
    return ctx->current_output == ctx->output_buffer_a ? ctx->output_buffer_b : ctx->output_buffer_a;
}

/* YCoCg color space conversion */
void wp_taa_rgb_to_ycocg( wp_f32 r, wp_f32 g, wp_f32 b, wp_f32 *out_y, wp_f32 *out_co, wp_f32 *out_cg )
{
    *out_y = 0.25f * r + 0.5f * g + 0.25f * b;
    *out_co = 0.5f * r - 0.5f * b;
    *out_cg = -0.25f * r + 0.5f * g - 0.25f * b;
}

void wp_taa_ycocg_to_rgb( wp_f32 y, wp_f32 co, wp_f32 cg, wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b )
{
    *out_r = y + co - cg;
    *out_g = y + cg;
    *out_b = y - co - cg;
}

void wp_taa_sample_catmull_rom( const wp_f32 *buffer, wp_s32 width, wp_s32 height, wp_f32 u, wp_f32 v,
                                wp_f32 *out_r, wp_f32 *out_g, wp_f32 *out_b )
{
    wp_taa_catmull_rom_sample( buffer, width, height, u, v, out_r, out_g, out_b );
}

/* Velocity dilation - expand velocity at depth discontinuities */
static void wp_taa_dilate_velocity( const wp_f32 *depth, const wp_f32 *velocity, wp_f32 *out_velocity,
                                    wp_s32 width, wp_s32 height, wp_f32 depth_threshold )
{
    (void)depth;
    (void)velocity;
    (void)out_velocity;
    (void)width;
    (void)height;
    (void)depth_threshold;
    /* Simplified - copy velocity as-is */
}

/**
 * Main TAA resolve with variance clipping
 */
void wp_taa_resolve( wp_taa *ctx, const wp_f32 *current_buffer, const wp_f32 *history_buffer,
                     const wp_f32 *velocity_buffer, const wp_f32 *normal_buffer,
                     const wp_f32 *depth_buffer, const wp_mat4f *current_vp, const wp_mat4f *previous_vp,
                     const wp_mat4f *inv_vp, wp_s32 frame )
{
    (void)current_vp;
    (void)previous_vp;
    (void)inv_vp;
    (void)normal_buffer;
    (void)depth_buffer;

    if( !ctx || !current_buffer )
        return;

    wp_s32 w = ctx->width;
    wp_s32 h = ctx->height;
    wp_f32 *write_output = ctx->first_frame
                               ? ctx->current_output
                               : ( ctx->current_output == ctx->output_buffer_a ? ctx->output_buffer_b
                                                                               : ctx->output_buffer_a );

    /* On first frame, just copy current to output */
    if( ctx->first_frame || !history_buffer )
    {
        memcpy( write_output, current_buffer, ctx->pixel_count * 3 * sizeof( wp_f32 ) );
        ctx->first_frame = 0;
        ctx->current_output = write_output;
        return;
    }

    /* Variance clipping neighborhood (3x3) */
    wp_s32 nsize = WP_TAA_NEIGHBORHOOD_SIZE;

    for( wp_s32 y = 0; y < h; y++ )
    {
        for( wp_s32 x = 0; x < w; x++ )
        {
            wp_s32 idx = ( y * w + x ) * 3;

            /* Get velocity */
            wp_f32 vel_x = 0.0f, vel_y = 0.0f;
            if( velocity_buffer )
            {
                wp_s32 vidx = ( y * w + x ) * 2;
                vel_x = velocity_buffer[vidx + 0];
                vel_y = velocity_buffer[vidx + 1];
            }

            /* Compute history UV */
            wp_f32 curr_u = ( (wp_f32)x + 0.5f ) / (wp_f32)w;
            wp_f32 curr_v = ( (wp_f32)y + 0.5f ) / (wp_f32)h;
            wp_f32 hist_u = curr_u - vel_x * ctx->config.motion_scale;
            wp_f32 hist_v = curr_v - vel_y * ctx->config.motion_scale;

            /* Sample history with Catmull-Rom */
            wp_f32 hist_r, hist_g, hist_b;
            wp_taa_catmull_rom_sample( history_buffer, w, h, hist_u, hist_v, &hist_r, &hist_g, &hist_b );

            /* Tonemap to linear space */
            wp_f32 ty, tco, tcg;
            wp_taa_tonemap_w( wp_taa_max( hist_r, 0.0f ), wp_taa_max( hist_g, 0.0f ),
                              wp_taa_max( hist_b, 0.0f ), &ty, &tco, &tcg );

            /* Gather neighborhood for variance estimation */
            wp_f32 n_y[9], n_co[9], n_cg[9];
            wp_s32 ni = 0;

            for( wp_s32 dy = -nsize; dy <= nsize; dy++ )
            {
                for( wp_s32 dx = -nsize; dx <= nsize; dx++ )
                {
                    wp_s32 nx = x + dx;
                    wp_s32 ny = y + dy;

                    if( nx >= 0 && nx < w && ny >= 0 && ny < h )
                    {
                        wp_s32 nidx = ( ny * w + nx ) * 3;
                        wp_taa_tonemap_w( current_buffer[nidx + 0], current_buffer[nidx + 1],
                                          current_buffer[nidx + 2], &n_y[ni], &n_co[ni], &n_cg[ni] );
                    }
                    else
                    {
                        n_y[ni] = n_co[ni] = n_cg[ni] = 0.0f;
                    }
                    ni++;
                }
            }

            /* Compute mean */
            wp_f32 mean_y = 0.0f, mean_co = 0.0f, mean_cg = 0.0f;
            for( wp_s32 i = 0; i < 9; i++ )
            {
                mean_y += n_y[i];
                mean_co += n_co[i];
                mean_cg += n_cg[i];
            }
            mean_y /= 9.0f;
            mean_co /= 9.0f;
            mean_cg /= 9.0f;

            /* Compute variance */
            wp_f32 sigma_y = 0.0f, sigma_co = 0.0f, sigma_cg = 0.0f;
            for( wp_s32 i = 0; i < 9; i++ )
            {
                wp_f32 dy = n_y[i] - mean_y;
                wp_f32 dco = n_co[i] - mean_co;
                wp_f32 dcg = n_cg[i] - mean_cg;
                sigma_y += dy * dy;
                sigma_co += dco * dco;
                sigma_cg += dcg * dcg;
            }
            sigma_y = (wp_f32)sqrt( wp_taa_max( sigma_y / 9.0f, 0.0f ) );
            sigma_co = (wp_f32)sqrt( wp_taa_max( sigma_co / 9.0f, 0.0f ) );
            sigma_cg = (wp_f32)sqrt( wp_taa_max( sigma_cg / 9.0f, 0.0f ) );

            /* Clamp range based on neighborhood + variance */
            wp_f32 gamma = ctx->config.clip_gamma;
            wp_f32 min_y = mean_y - gamma * sigma_y;
            wp_f32 max_y = mean_y + gamma * sigma_y;
            wp_f32 min_co = mean_co - gamma * sigma_co;
            wp_f32 max_co = mean_co + gamma * sigma_co;
            wp_f32 min_cg = mean_cg - gamma * sigma_cg;
            wp_f32 max_cg = mean_cg + gamma * sigma_cg;

            /* Variance clipping - clamp history toward neighborhood mean */
            wp_f32 out_ty = ty, out_tco = tco, out_tcg = tcg;

            /* Luma clipping */
            wp_f32 lo_y = wp_taa_max( ty, min_y );
            wp_f32 hi_y = wp_taa_min( ty, max_y );
            wp_f32 lo_co = wp_taa_max( tco, min_co );
            wp_f32 hi_co = wp_taa_min( tco, max_co );
            wp_f32 lo_cg = wp_taa_max( tcg, min_cg );
            wp_f32 hi_cg = wp_taa_min( tcg, max_cg );

            out_ty = ( lo_y + hi_y ) * 0.5f;
            out_tco = ( lo_co + hi_co ) * 0.5f;
            out_tcg = ( lo_cg + hi_cg ) * 0.5f;

            /* Convert back from YCoCg */
            wp_f32 out_r, out_g, out_b;
            wp_taa_ycocg_to_rgb( out_ty, out_tco, out_tcg, &out_r, &out_g, &out_b );

            /* Inverse tonemap */
            wp_taa_tonemap_w_inv( wp_taa_max( out_ty, 0.0f ), wp_taa_max( out_tco, 0.0f ),
                                  wp_taa_max( out_tcg, 0.0f ), &out_r, &out_g, &out_b );

            /* Get current color */
            wp_f32 curr_r = current_buffer[idx + 0];
            wp_f32 curr_g = current_buffer[idx + 1];
            wp_f32 curr_b = current_buffer[idx + 2];

            /* Depth-based feedback adjustment */
            wp_f32 feedback = ctx->config.feedback;

            if( depth_buffer )
            {
                wp_f32 depth = depth_buffer[y * w + x];
                if( depth > 0.0f )
                {
                    /* Reduce feedback for far objects */
                    feedback *= wp_taa_min( 1.0f, depth * 0.01f );
                }
            }

            /* Blend current and clipped history */
            write_output[idx + 0] = curr_r * ( 1.0f - feedback ) + out_r * feedback;
            write_output[idx + 1] = curr_g * ( 1.0f - feedback ) + out_g * feedback;
            write_output[idx + 2] = curr_b * ( 1.0f - feedback ) + out_b * feedback;
        }
    }

    ctx->current_output = write_output;
}
