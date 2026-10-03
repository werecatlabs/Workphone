/**
 * @file wp_graphics_contact_shadows.c
 * @brief Contact Shadows implementation.
 *
 * Screen-space depth march shadows for filling the gap between CSM texels
 * and geometry - the "sticker" elimination system.
 *
 * Reference: G:\Claude-of-Duty-main\Claude-of-Duty-main\src\render\contact.js
 */

#include "workphone_graphics_contact_shadows.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Internal constants
 * ====================================================================== */

#define WP_CONTACT_PI 3.141592653589793f
#define WP_CONTACT_MAX_STEPS 14

/* Interleaved gradient noise constants */
#define WP_CONTACT_IGN_A 0.06711056f
#define WP_CONTACT_IGN_B 0.00583715f

void wp_contact_shadows_blur( wp_contact_shadows *ctx );

/* =========================================================================
 * Internal structures
 * ====================================================================== */

/**
 * Internal render targets
 */
typedef struct wp_contact_targets
{
    /** Raw shadow buffer (RG32F) */
    wp_f32 *raw_buffer;
    /** Blurred buffer A (RG32F) */
    wp_f32 *blur_a;
    /** Blurred buffer B (RG32F) */
    wp_f32 *blur_b;
    /** Final output buffer (points to blur_a or blur_b) */
    wp_f32 *output;
} wp_contact_targets;

/**
 * Internal state
 */
struct wp_contact_shadows
{
    /** Render width */
    wp_s32 width;
    /** Render height */
    wp_s32 height;
    /** Total pixels */
    wp_s32 pixel_count;
    /** Buffer size in floats (RG = 2 per pixel) */
    wp_s32 buffer_floats;

    /** Configuration */
    wp_contact_shadows_config config;

    /** Render targets */
    wp_contact_targets targets;

    /** Current frame */
    wp_s32 frame;

    /** Temporary working buffers */
    wp_f32 *temp_buffer;
    wp_s32 temp_buffer_size;

    /** Previous depth buffer for change detection */
    wp_f32 *prev_depth;
    wp_s32 depth_changed;
};

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/**
 * Interleaved gradient noise
 */
static wp_f32 wp_contact_ign( wp_f32 x, wp_f32 y )
{
    return (wp_f32)fmod( 52.9829189f * (wp_f32)fmod( x * WP_CONTACT_IGN_A + y * WP_CONTACT_IGN_B, 1.0f ),
                         1.0f );
}

/**
 * Clamp value to range
 */
static wp_f32 wp_contact_clamp( wp_f32 x, wp_f32 min_val, wp_f32 max_val )
{
    if( x < min_val )
        return min_val;
    if( x > max_val )
        return max_val;
    return x;
}

static wp_f32 wp_contact_max( wp_f32 a, wp_f32 b )
{
    return a > b ? a : b;
}

/**
 * Smooth step
 */
static wp_f32 wp_contact_smoothstep( wp_f32 edge0, wp_f32 edge1, wp_f32 x )
{
    wp_f32 t = wp_contact_clamp( ( x - edge0 ) / ( edge1 - edge0 ), 0.0f, 1.0f );
    return t * t * ( 3.0f - 2.0f * t );
}

/**
 * Linear depth to view space position
 */
static void wp_contact_view_pos( wp_f32 u, wp_f32 v, wp_f32 depth, const wp_mat4f *proj_inv,
                                 wp_f32 *out_x, wp_f32 *out_y, wp_f32 *out_z )
{
    wp_f32 nx = u * 2.0f - 1.0f;
    wp_f32 ny = v * 2.0f - 1.0f;

    /* Transform to clip space */
    wp_f32 hx =
        proj_inv->m[0][0] * nx + proj_inv->m[0][1] * ny + proj_inv->m[0][2] * depth + proj_inv->m[0][3];
    wp_f32 hy =
        proj_inv->m[1][0] * nx + proj_inv->m[1][1] * ny + proj_inv->m[1][2] * depth + proj_inv->m[1][3];
    wp_f32 hz =
        proj_inv->m[2][0] * nx + proj_inv->m[2][1] * ny + proj_inv->m[2][2] * depth + proj_inv->m[2][3];
    wp_f32 hw =
        proj_inv->m[3][0] * nx + proj_inv->m[3][1] * ny + proj_inv->m[3][2] * depth + proj_inv->m[3][3];

    /* Perspective divide */
    wp_f32 inv_w = 1.0f / hw;
    wp_f32 rx = hx * inv_w;
    wp_f32 ry = hy * inv_w;
    wp_f32 rz = hz * inv_w;

    /* Scale by inverse projection to get view direction */
    wp_f32 fx =
        proj_inv->m[0][0] * nx + proj_inv->m[0][1] * ny + proj_inv->m[0][2] + proj_inv->m[0][3] * inv_w;
    wp_f32 fy =
        proj_inv->m[1][0] * nx + proj_inv->m[1][1] * ny + proj_inv->m[1][2] + proj_inv->m[1][3] * inv_w;
    wp_f32 fz =
        proj_inv->m[2][0] * nx + proj_inv->m[2][1] * ny + proj_inv->m[2][2] + proj_inv->m[2][3] * inv_w;

    wp_f32 len = (wp_f32)sqrt( fx * fx + fy * fy + fz * fz );
    if( len < 1e-6f )
        len = 1e-6f;

    /* View position */
    *out_x = rx / len * depth;
    *out_y = ry / len * depth;
    *out_z = -depth; /* View space: negative Z is forward */
}

/**
 * Project view position to UV
 */
static void wp_contact_project_to_uv( wp_f32 vx, wp_f32 vy, wp_f32 vz, const wp_mat4f *proj,
                                      wp_f32 *out_u, wp_f32 *out_v )
{
    wp_f32 cx = proj->m[0][0] * vx + proj->m[0][1] * vy + proj->m[0][2] * vz + proj->m[0][3];
    wp_f32 cy = proj->m[1][0] * vx + proj->m[1][1] * vy + proj->m[1][2] * vz + proj->m[1][3];
    wp_f32 cw = proj->m[3][0] * vx + proj->m[3][1] * vy + proj->m[3][2] * vz + proj->m[3][3];

    wp_f32 inv_w = 1.0f / cw;
    *out_u = cx * inv_w * 0.5f + 0.5f;
    *out_v = cy * inv_w * 0.5f + 0.5f;
}

/**
 * Decode octahedral normal
 */
static void wp_contact_decode_normal( wp_f32 nx, wp_f32 ny, wp_f32 *out_x, wp_f32 *out_y, wp_f32 *out_z )
{
    wp_f32 z = 1.0f - (wp_f32)fabs( nx ) - (wp_f32)fabs( ny );
    wp_f32 tx = nx;
    wp_f32 ty = ny;

    if( z < 0.0f )
    {
        wp_f32 old_tx = tx;
        tx = ( 1.0f - (wp_f32)fabs( ty ) ) * ( tx >= 0.0f ? 1.0f : -1.0f );
        ty = ( 1.0f - (wp_f32)fabs( old_tx ) ) * ( ty >= 0.0f ? 1.0f : -1.0f );
    }

    /* Normalize */
    wp_f32 len = (wp_f32)sqrt( tx * tx + ty * ty + z * z );
    if( len < 1e-6f )
        len = 1e-6f;

    *out_x = tx / len;
    *out_y = ty / len;
    *out_z = z / len;
}

/**
 * Sample depth buffer with bounds check
 */
static wp_f32 wp_contact_sample_depth( const wp_f32 *buffer, wp_s32 w, wp_s32 h, wp_f32 u, wp_f32 v )
{
    if( u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f )
        return 1e8f; /* Large value = no geometry */

    wp_s32 ix = (wp_s32)( u * ( w - 1 ) );
    wp_s32 iy = (wp_s32)( v * ( h - 1 ) );
    wp_s32 idx = ( iy * w + ix );

    return buffer[idx];
}

/**
 * Sample normal buffer with bounds check
 */
static void wp_contact_sample_normal( const wp_f32 *buffer, wp_s32 w, wp_s32 h, wp_f32 u, wp_f32 v,
                                      wp_f32 *out_nx, wp_f32 *out_ny, wp_f32 *out_cov )
{
    if( u < 0.0f || u >= 1.0f || v < 0.0f || v >= 1.0f )
    {
        *out_nx = 0.0f;
        *out_ny = 0.0f;
        *out_cov = 0.0f;
        return;
    }

    /* Bilinear sampling */
    wp_f32 u_fract = u * ( w - 1 ) - (wp_f32)( (wp_s32)( u * ( w - 1 ) ) );
    wp_f32 v_fract = v * ( h - 1 ) - (wp_f32)( (wp_s32)( v * ( h - 1 ) ) );

    wp_s32 x0 = (wp_s32)( u * ( w - 1 ) );
    wp_s32 y0 = (wp_s32)( v * ( h - 1 ) );
    wp_s32 x1 = x0 < w - 1 ? x0 + 1 : x0;
    wp_s32 y1 = y0 < h - 1 ? y0 + 1 : y0;

    wp_s32 idx00 = ( y0 * w + x0 ) * 4;
    wp_s32 idx10 = ( y0 * w + x1 ) * 4;
    wp_s32 idx01 = ( y1 * w + x0 ) * 4;
    wp_s32 idx11 = ( y1 * w + x1 ) * 4;

    wp_f32 nx00 = buffer[idx00 + 0];
    wp_f32 ny00 = buffer[idx00 + 1];
    wp_f32 cov00 = buffer[idx00 + 2];

    wp_f32 nx10 = buffer[idx10 + 0];
    wp_f32 ny10 = buffer[idx10 + 1];
    wp_f32 cov10 = buffer[idx10 + 2];

    wp_f32 nx01 = buffer[idx01 + 0];
    wp_f32 ny01 = buffer[idx01 + 1];
    wp_f32 cov01 = buffer[idx01 + 2];

    wp_f32 nx11 = buffer[idx11 + 0];
    wp_f32 ny11 = buffer[idx11 + 1];
    wp_f32 cov11 = buffer[idx11 + 2];

    wp_f32 nx = nx00 * ( 1.0f - u_fract ) * ( 1.0f - v_fract ) + nx10 * u_fract * ( 1.0f - v_fract ) +
                nx01 * ( 1.0f - u_fract ) * v_fract + nx11 * u_fract * v_fract;

    wp_f32 ny = ny00 * ( 1.0f - u_fract ) * ( 1.0f - v_fract ) + ny10 * u_fract * ( 1.0f - v_fract ) +
                ny01 * ( 1.0f - u_fract ) * v_fract + ny11 * u_fract * v_fract;

    wp_f32 cov = cov00 * ( 1.0f - u_fract ) * ( 1.0f - v_fract ) + cov10 * u_fract * ( 1.0f - v_fract ) +
                 cov01 * ( 1.0f - u_fract ) * v_fract + cov11 * u_fract * v_fract;

    *out_nx = nx;
    *out_ny = ny;
    *out_cov = cov;
}

/* =========================================================================
 * Public API
 * ====================================================================== */

wp_contact_shadows *wp_contact_shadows_create( wp_s32 width, wp_s32 height,
                                               const wp_contact_shadows_config *config )
{
    wp_contact_shadows *ctx;
    size_t buffer_size;

    if( width <= 0 || height <= 0 )
        return NULL;

    ctx = (wp_contact_shadows *)malloc( sizeof( wp_contact_shadows ) );
    if( !ctx )
        return NULL;

    memset( ctx, 0, sizeof( wp_contact_shadows ) );

    ctx->width = width;
    ctx->height = height;
    ctx->pixel_count = width * height;
    ctx->buffer_floats = ctx->pixel_count * 2; /* RG = 2 floats per pixel */

    /* Default configuration */
    if( config )
    {
        ctx->config = *config;
    }
    else
    {
        ctx->config.length = WP_CONTACT_SHADOW_DEFAULT_LENGTH;
        ctx->config.thickness = WP_CONTACT_SHADOW_DEFAULT_THICKNESS;
        ctx->config.strength = WP_CONTACT_SHADOW_DEFAULT_STRENGTH;
        ctx->config.quality = 2; /* High quality */
    }

    buffer_size = (size_t)ctx->buffer_floats * sizeof( wp_f32 );

    /* Allocate buffers */
    ctx->targets.raw_buffer = (wp_f32 *)malloc( buffer_size );
    ctx->targets.blur_a = (wp_f32 *)malloc( buffer_size );
    ctx->targets.blur_b = (wp_f32 *)malloc( buffer_size );
    ctx->targets.output = ctx->targets.blur_a;

    ctx->temp_buffer = (wp_f32 *)malloc( buffer_size );
    ctx->prev_depth = (wp_f32 *)malloc( (size_t)ctx->pixel_count * sizeof( wp_f32 ) );

    if( !ctx->targets.raw_buffer || !ctx->targets.blur_a || !ctx->targets.blur_b || !ctx->temp_buffer ||
        !ctx->prev_depth )
    {
        wp_contact_shadows_destroy( ctx );
        return NULL;
    }

    /* Clear buffers */
    memset( ctx->targets.raw_buffer, 0, buffer_size );
    memset( ctx->targets.blur_a, 0, buffer_size );
    memset( ctx->targets.blur_b, 0, buffer_size );
    memset( ctx->prev_depth, 0, (size_t)ctx->pixel_count * sizeof( wp_f32 ) );

    return ctx;
}

void wp_contact_shadows_destroy( wp_contact_shadows *ctx )
{
    if( !ctx )
        return;

    if( ctx->targets.raw_buffer )
        free( ctx->targets.raw_buffer );
    if( ctx->targets.blur_a )
        free( ctx->targets.blur_a );
    if( ctx->targets.blur_b )
        free( ctx->targets.blur_b );
    if( ctx->temp_buffer )
        free( ctx->temp_buffer );
    if( ctx->prev_depth )
        free( ctx->prev_depth );

    free( ctx );
}

void wp_contact_shadows_render( wp_contact_shadows *ctx, const wp_f32 *depth_buffer,
                                const wp_f32 *normal_buffer, const wp_mat4f *proj_matrix,
                                const wp_mat4f *proj_inv_matrix, const wp_vec3f *sun_dir_view,
                                wp_s32 frame )
{
    wp_s32 x, y, step_idx;
    wp_f32 len, thickness, strength;
    wp_s32 depth_changed = 0;

    if( !ctx || !depth_buffer || !normal_buffer )
        return;

    ctx->frame = frame;

    len = ctx->config.length;
    thickness = ctx->config.thickness;
    strength = ctx->config.strength;

    /* Check if depth buffer changed significantly */
    for( y = 0; y < ctx->height && !depth_changed; y += 8 )
    {
        for( x = 0; x < ctx->width && !depth_changed; x += 8 )
        {
            wp_s32 idx = y * ctx->width + x;
            wp_f32 diff = (wp_f32)fabs( depth_buffer[idx] - ctx->prev_depth[idx] );
            if( diff > 0.01f )
                depth_changed = 1;
        }
    }
    ctx->depth_changed = depth_changed;

    /* Update previous depth */
    memcpy( ctx->prev_depth, depth_buffer, (size_t)ctx->pixel_count * sizeof( wp_f32 ) );

    /* Ray march each pixel */
    for( y = 0; y < ctx->height; y++ )
    {
        for( x = 0; x < ctx->width; x++ )
        {
            wp_f32 u = ( (wp_f32)x + 0.5f ) / (wp_f32)ctx->width;
            wp_f32 v = ( (wp_f32)y + 0.5f ) / (wp_f32)ctx->height;
            wp_s32 pixel_idx = y * ctx->width + x;
            wp_s32 buf_idx = pixel_idx * 2;

            wp_f32 center_depth = depth_buffer[pixel_idx];
            wp_f32 nx, ny, cov;
            wp_contact_sample_normal( normal_buffer, ctx->width, ctx->height, u, v, &nx, &ny, &cov );

            /* Skip sky pixels (coverage < 0.5) */
            if( cov < 0.5f )
            {
                ctx->targets.raw_buffer[buf_idx + 0] = 1.0f; /* No shadow */
                ctx->targets.raw_buffer[buf_idx + 1] = center_depth;
                continue;
            }

            /* Get view position */
            wp_f32 px, py, pz;
            wp_contact_view_pos( u, v, center_depth, proj_inv_matrix, &px, &py, &pz );

            /* Decode normal */
            wp_f32 nnx, nny, nnz;
            wp_contact_decode_normal( nx, ny, &nnx, &nny, &nnz );

            /* Calculate N dot L */
            wp_f32 NdL = nnx * sun_dir_view->x + nny * sun_dir_view->y + nnz * sun_dir_view->z;

            /* Skip pixels facing away from sun */
            if( NdL <= 0.02f )
            {
                ctx->targets.raw_buffer[buf_idx + 0] = 1.0f;
                ctx->targets.raw_buffer[buf_idx + 1] = center_depth;
                continue;
            }

            /* Scale ray length by distance (closer = shorter rays) */
            wp_f32 scaled_len = len * wp_contact_clamp( center_depth * 0.08f + 0.75f, 0.75f, 2.5f );

            /* Random offset for stochastic sampling */
            wp_f32 jitter = wp_contact_ign( (wp_f32)x + 3.1717f * (wp_f32)( frame % 64 ), (wp_f32)y );

            /* Ray origin offset from surface */
            wp_f32 start_offset = 0.012f + center_depth * 0.0015f;
            wp_f32 ox = px + nnx * start_offset;
            wp_f32 oy = py + nny * start_offset;
            wp_f32 oz = pz + nnz * start_offset;

            /* Ray direction toward sun in view space */
            wp_f32 step_len = scaled_len / (wp_f32)WP_CONTACT_MAX_STEPS;
            wp_f32 sx = sun_dir_view->x * step_len;
            wp_f32 sy = sun_dir_view->y * step_len;
            wp_f32 sz = sun_dir_view->z * step_len;

            wp_f32 occlusion = 0.0f;

            /* Ray march */
            for( step_idx = 0; step_idx < WP_CONTACT_MAX_STEPS; step_idx++ )
            {
                wp_f32 step_t = (wp_f32)step_idx + jitter;
                wp_f32 spx = ox + sx * step_t;
                wp_f32 spy = oy + sy * step_t;
                wp_f32 spz = oz + sz * step_t;

                /* Project to UV */
                wp_f32 su, sv;
                wp_contact_project_to_uv( spx, spy, spz, proj_matrix, &su, &sv );

                /* Check bounds */
                if( su <= 0.0f || su >= 1.0f || sv <= 0.0f || sv >= 1.0f )
                    break;

                /* Sample scene depth at ray position */
                wp_f32 scene_depth =
                    wp_contact_sample_depth( depth_buffer, ctx->width, ctx->height, su, sv );

                /* Skip sky pixels */
                if( scene_depth > 1e7f )
                    continue;

                /* Calculate distance from ray to surface */
                wp_f32 diff = -spz - scene_depth;

                /* Bias to prevent self-shadowing */
                wp_f32 bias = 0.004f + scene_depth * 0.0025f;

                if( diff > bias && diff < thickness )
                {
                    /* Fade with distance traveled */
                    wp_f32 t_normalized = step_t / (wp_f32)WP_CONTACT_MAX_STEPS;
                    occlusion = wp_contact_max( occlusion, 1.0f - t_normalized * t_normalized );
                    break; /* Hit, stop marching */
                }
            }

            /* Compute final shadow value */
            wp_f32 shadow = 1.0f - occlusion * strength;
            shadow = wp_contact_clamp( shadow, 0.0f, 1.0f );

            ctx->targets.raw_buffer[buf_idx + 0] = shadow;
            ctx->targets.raw_buffer[buf_idx + 1] = center_depth;
        }
    }

    /* Apply bilateral blur */
    wp_contact_shadows_blur( ctx );
}

void wp_contact_shadows_blur( wp_contact_shadows *ctx )
{
    wp_s32 x, y, i;
    wp_s32 radius = WP_CONTACT_SHADOW_BLUR_RADIUS;

    if( !ctx )
        return;

    /* Horizontal pass: raw -> blur_a */
    for( y = 0; y < ctx->height; y++ )
    {
        for( x = 0; x < ctx->width; x++ )
        {
            wp_s32 pixel_idx = y * ctx->width + x;
            wp_s32 buf_idx = pixel_idx * 2;

            wp_f32 center_shadow = ctx->targets.raw_buffer[buf_idx + 0];
            wp_f32 center_depth = ctx->targets.raw_buffer[buf_idx + 1];

            wp_f32 sum = center_shadow * 0.5f;
            wp_f32 w_sum = 0.5f;

            for( i = 1; i <= radius; i++ )
            {
                wp_s32 x_left = x - i;
                wp_s32 x_right = x + i;

                if( x_left >= 0 )
                {
                    wp_s32 left_idx = y * ctx->width + x_left;
                    wp_f32 left_shadow = ctx->targets.raw_buffer[left_idx * 2 + 0];
                    wp_f32 left_depth = ctx->targets.raw_buffer[left_idx * 2 + 1];
                    wp_f32 w = 0.3f / (wp_f32)i;
                    wp_f32 depth_diff = (wp_f32)fabs( left_depth - center_depth );
                    wp_f32 depth_w =
                        (wp_f32)exp( -depth_diff * 40.0f / wp_contact_max( 0.1f, center_depth ) );
                    sum += left_shadow * w * depth_w;
                    w_sum += w * depth_w;
                }

                if( x_right < ctx->width )
                {
                    wp_s32 right_idx = y * ctx->width + x_right;
                    wp_f32 right_shadow = ctx->targets.raw_buffer[right_idx * 2 + 0];
                    wp_f32 right_depth = ctx->targets.raw_buffer[right_idx * 2 + 1];
                    wp_f32 w = 0.3f / (wp_f32)i;
                    wp_f32 depth_diff = (wp_f32)fabs( right_depth - center_depth );
                    wp_f32 depth_w =
                        (wp_f32)exp( -depth_diff * 40.0f / wp_contact_max( 0.1f, center_depth ) );
                    sum += right_shadow * w * depth_w;
                    w_sum += w * depth_w;
                }
            }

            ctx->targets.blur_a[buf_idx + 0] = sum / w_sum;
            ctx->targets.blur_a[buf_idx + 1] = center_depth;
        }
    }

    /* Vertical pass: blur_a -> blur_b */
    for( y = 0; y < ctx->height; y++ )
    {
        for( x = 0; x < ctx->width; x++ )
        {
            wp_s32 pixel_idx = y * ctx->width + x;
            wp_s32 buf_idx = pixel_idx * 2;

            wp_f32 center_shadow = ctx->targets.blur_a[buf_idx + 0];
            wp_f32 center_depth = ctx->targets.blur_a[buf_idx + 1];

            wp_f32 sum = center_shadow * 0.5f;
            wp_f32 w_sum = 0.5f;

            for( i = 1; i <= radius; i++ )
            {
                wp_s32 y_top = y - i;
                wp_s32 y_bottom = y + i;

                if( y_top >= 0 )
                {
                    wp_s32 top_idx = y_top * ctx->width + x;
                    wp_f32 top_shadow = ctx->targets.blur_a[top_idx * 2 + 0];
                    wp_f32 top_depth = ctx->targets.blur_a[top_idx * 2 + 1];
                    wp_f32 w = 0.3f / (wp_f32)i;
                    wp_f32 depth_diff = (wp_f32)fabs( top_depth - center_depth );
                    wp_f32 depth_w =
                        (wp_f32)exp( -depth_diff * 40.0f / wp_contact_max( 0.1f, center_depth ) );
                    sum += top_shadow * w * depth_w;
                    w_sum += w * depth_w;
                }

                if( y_bottom < ctx->height )
                {
                    wp_s32 bottom_idx = y_bottom * ctx->width + x;
                    wp_f32 bottom_shadow = ctx->targets.blur_a[bottom_idx * 2 + 0];
                    wp_f32 bottom_depth = ctx->targets.blur_a[bottom_idx * 2 + 1];
                    wp_f32 w = 0.3f / (wp_f32)i;
                    wp_f32 depth_diff = (wp_f32)fabs( bottom_depth - center_depth );
                    wp_f32 depth_w =
                        (wp_f32)exp( -depth_diff * 40.0f / wp_contact_max( 0.1f, center_depth ) );
                    sum += bottom_shadow * w * depth_w;
                    w_sum += w * depth_w;
                }
            }

            ctx->targets.blur_b[buf_idx + 0] = sum / w_sum;
            ctx->targets.blur_b[buf_idx + 1] = center_depth;
        }
    }

    /* Output is in blur_b */
    ctx->targets.output = ctx->targets.blur_b;
}

const wp_f32 *wp_contact_shadows_get_buffer( const wp_contact_shadows *ctx )
{
    if( !ctx )
        return NULL;
    return ctx->targets.output;
}

void wp_contact_shadows_get_size( const wp_contact_shadows *ctx, wp_s32 *width, wp_s32 *height )
{
    if( ctx )
    {
        if( width )
            *width = ctx->width;
        if( height )
            *height = ctx->height;
    }
    else
    {
        if( width )
            *width = 0;
        if( height )
            *height = 0;
    }
}

void wp_contact_shadows_set_length( wp_contact_shadows *ctx, wp_f32 len )
{
    if( ctx )
        ctx->config.length = len;
}

void wp_contact_shadows_set_strength( wp_contact_shadows *ctx, wp_f32 s )
{
    if( ctx )
        ctx->config.strength = s;
}

void wp_contact_shadows_resize( wp_contact_shadows *ctx, wp_s32 width, wp_s32 height )
{
    wp_f32 *old_raw, *old_blur_a, *old_blur_b, *old_prev;
    size_t new_size;

    if( !ctx || width <= 0 || height <= 0 )
        return;

    /* Save old buffers */
    old_raw = ctx->targets.raw_buffer;
    old_blur_a = ctx->targets.blur_a;
    old_blur_b = ctx->targets.blur_b;
    old_prev = ctx->prev_depth;

    /* Update dimensions */
    ctx->width = width;
    ctx->height = height;
    ctx->pixel_count = width * height;
    ctx->buffer_floats = ctx->pixel_count * 2;

    new_size = (size_t)ctx->buffer_floats * sizeof( wp_f32 );

    /* Allocate new buffers */
    ctx->targets.raw_buffer = (wp_f32 *)malloc( new_size );
    ctx->targets.blur_a = (wp_f32 *)malloc( new_size );
    ctx->targets.blur_b = (wp_f32 *)malloc( new_size );
    ctx->targets.output = ctx->targets.blur_a;
    ctx->temp_buffer = (wp_f32 *)realloc( ctx->temp_buffer, new_size );
    ctx->prev_depth = (wp_f32 *)malloc( (size_t)ctx->pixel_count * sizeof( wp_f32 ) );

    /* Clear new buffers */
    if( ctx->targets.raw_buffer )
        memset( ctx->targets.raw_buffer, 0, new_size );
    if( ctx->targets.blur_a )
        memset( ctx->targets.blur_a, 0, new_size );
    if( ctx->targets.blur_b )
        memset( ctx->targets.blur_b, 0, new_size );
    if( ctx->prev_depth )
        memset( ctx->prev_depth, 0, (size_t)ctx->pixel_count * sizeof( wp_f32 ) );

    /* Free old buffers */
    free( old_raw );
    free( old_blur_a );
    free( old_blur_b );
    free( old_prev );
}

void wp_contact_shadows_sample( const wp_contact_shadows *ctx, wp_f32 u, wp_f32 v, wp_f32 *out_shadow,
                                wp_f32 *out_depth )
{
    if( !ctx )
    {
        if( out_shadow )
            *out_shadow = 1.0f;
        if( out_depth )
            *out_depth = 0.0f;
        return;
    }

    /* Bilinear sampling */
    wp_f32 u_f = u * ( ctx->width - 1 );
    wp_f32 v_f = v * ( ctx->height - 1 );

    wp_s32 x0 = (wp_s32)u_f;
    wp_s32 y0 = (wp_s32)v_f;
    wp_s32 x1 = x0 < ctx->width - 1 ? x0 + 1 : x0;
    wp_s32 y1 = y0 < ctx->height - 1 ? y0 + 1 : y0;

    wp_f32 x_frac = u_f - x0;
    wp_f32 y_frac = v_f - y0;

    wp_s32 idx00 = ( y0 * ctx->width + x0 ) * 2;
    wp_s32 idx10 = ( y0 * ctx->width + x1 ) * 2;
    wp_s32 idx01 = ( y1 * ctx->width + x0 ) * 2;
    wp_s32 idx11 = ( y1 * ctx->width + x1 ) * 2;

    wp_f32 s00 = ctx->targets.output[idx00 + 0];
    wp_f32 s10 = ctx->targets.output[idx10 + 0];
    wp_f32 s01 = ctx->targets.output[idx01 + 0];
    wp_f32 s11 = ctx->targets.output[idx11 + 0];

    wp_f32 d00 = ctx->targets.output[idx00 + 1];
    wp_f32 d10 = ctx->targets.output[idx10 + 1];
    wp_f32 d01 = ctx->targets.output[idx01 + 1];
    wp_f32 d11 = ctx->targets.output[idx11 + 1];

    wp_f32 s0 = s00 * ( 1.0f - x_frac ) + s10 * x_frac;
    wp_f32 s1 = s01 * ( 1.0f - x_frac ) + s11 * x_frac;
    wp_f32 shadow = s0 * ( 1.0f - y_frac ) + s1 * y_frac;

    wp_f32 d0 = d00 * ( 1.0f - x_frac ) + d10 * x_frac;
    wp_f32 d1 = d01 * ( 1.0f - x_frac ) + d11 * x_frac;
    wp_f32 depth = d0 * ( 1.0f - y_frac ) + d1 * y_frac;

    if( out_shadow )
        *out_shadow = shadow;
    if( out_depth )
        *out_depth = depth;
}

const wp_c8 *wp_contact_shadows_get_glsl_chunk( void )
{
    static const wp_c8 *chunk =
        "/* Contact shadow sampling GLSL */\n"
        "uniform sampler2D owContactTex;\n"
        "uniform vec2 owScreenTexel;\n"
        "uniform vec3 owSunDirView;\n"
        "uniform vec3 owSunDirWorld;\n"
        "\n"
        "float owContactShadow( vec3 lightDirView ) {\n"
        "    if ( owFeat.y < 0.5 ) return 1.0;\n"
        "    if ( dot( lightDirView, owSunDirView ) < 0.999 ) return 1.0;\n"
        "    return texture2D( owContactTex, gl_FragCoord.xy * owScreenTexel ).r;\n"
        "}\n";
    return chunk;
}
