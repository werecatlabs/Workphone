/**
 * @file wp_graphics_gbuffer.c
 * @brief GBuffer system implementation.
 *
 * Reference: G:\Claude-of-Duty-main\Claude-of-Duty-main\src\render\prepass.js
 */

#include "workphone_graphics_gbuffer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Internal Constants
 * ====================================================================== */

#define WP_GBUFFER_FLOAT_EPSILON 1e-8f

/* =========================================================================
 * Internal Helper Functions
 * ====================================================================== */

/**
 * Identity matrix constant
 */
static const wp_mat4f wp_identity_matrix = { { { 1.0f, 0.0f, 0.0f, 0.0f },
                                               { 0.0f, 1.0f, 0.0f, 0.0f },
                                               { 0.0f, 0.0f, 1.0f, 0.0f },
                                               { 0.0f, 0.0f, 0.0f, 1.0f } } };

/**
 * Check if matrix is approximately identity
 */
static wp_s32 wp_gbuffer_is_identity( const wp_mat4f *m )
{
    if( !m )
        return 1;
    for( wp_s32 i = 0; i < 16; i++ )
    {
        wp_f32 expected = ( i % 5 == 0 ) ? 1.0f : 0.0f;
        if( fabsf( m->m[i / 4][i % 4] - expected ) > 1e-6f )
            return 0;
    }
    return 1;
}

/**
 * Copy matrix
 */
static void wp_gbuffer_copy_matrix( wp_mat4f *dest, const wp_mat4f *src )
{
    if( !dest || !src )
        return;
    memcpy( dest, src, sizeof( wp_mat4f ) );
}

/**
 * Find object index by ID
 */
static wp_s32 wp_gbuffer_find_object( wp_gbuffer *ctx, wp_s32 object_id )
{
    for( wp_s32 i = 0; i < ctx->object_count; i++ )
    {
        if( ctx->object_ids[i] == object_id )
            return i;
    }
    return -1;
}

/**
 * Add new object tracking entry
 */
static void wp_gbuffer_add_object( wp_gbuffer *ctx, wp_s32 object_id, const wp_mat4f *matrix )
{
    if( ctx->object_count >= WP_GBUFFER_MAX_OBJECTS )
        return;

    wp_s32 idx = ctx->object_count;
    ctx->object_ids[idx] = object_id;
    wp_gbuffer_copy_matrix( &ctx->curr_matrices[idx], matrix );
    wp_gbuffer_copy_matrix( &ctx->prev_matrices[idx], matrix );
    ctx->object_count++;
}

/* =========================================================================
 * Octahedral Normal Encoding/Decoding
 * ====================================================================== */

/**
 * Octahedral wrap for negative hemisphere
 */
static void wp_gbuffer_oct_wrap( wp_f32 *x, wp_f32 *y )
{
    wp_f32 nx = *x;
    wp_f32 ny = *y;
    *x = ( 1.0f - fabsf( ny ) ) * ( nx >= 0.0f ? 1.0f : -1.0f );
    *y = ( 1.0f - fabsf( nx ) ) * ( ny >= 0.0f ? 1.0f : -1.0f );
}

/**
 * Encode 3D normal to octahedral
 */
void wp_gbuffer_encode_normal( wp_f32 nx, wp_f32 ny, wp_f32 nz, wp_f32 *out_x, wp_f32 *out_y )
{
    /* Normalize */
    wp_f32 len = sqrtf( nx * nx + ny * ny + nz * nz );
    if( len < WP_GBUFFER_FLOAT_EPSILON )
    {
        *out_x = 0.0f;
        *out_y = 0.0f;
        return;
    }
    nx /= len;
    ny /= len;
    nz /= len;

    /* Project to octahedron */
    wp_f32 inv_sum = 1.0f / ( fabsf( nx ) + fabsf( ny ) + fabsf( nz ) + WP_GBUFFER_FLOAT_EPSILON );
    wp_f32 enc_x = nx * inv_sum;
    wp_f32 enc_y = ny * inv_sum;

    /* Wrap negative hemisphere */
    if( nz < 0.0f )
    {
        wp_f32 tx = enc_x;
        wp_f32 ty = enc_y;
        enc_x = ( 1.0f - fabsf( ty ) ) * ( tx >= 0.0f ? 1.0f : -1.0f );
        enc_y = ( 1.0f - fabsf( tx ) ) * ( ty >= 0.0f ? 1.0f : -1.0f );
    }

    *out_x = enc_x;
    *out_y = enc_y;
}

/**
 * Decode octahedral normal to 3D vector
 */
void wp_gbuffer_decode_normal( wp_f32 encoded_x, wp_f32 encoded_y, wp_f32 *out_x, wp_f32 *out_y,
                               wp_f32 *out_z )
{
    wp_f32 nx = encoded_x;
    wp_f32 ny = encoded_y;
    wp_f32 nz = 1.0f - fabsf( nx ) - fabsf( ny );

    /* Recover z and re-normalize */
    if( nz < 0.0f )
    {
        wp_f32 tx = nx;
        nx = ( 1.0f - fabsf( ny ) ) * ( tx >= 0.0f ? 1.0f : -1.0f );
        ny = ( 1.0f - fabsf( tx ) ) * ( ny >= 0.0f ? 1.0f : -1.0f );
    }

    /* Normalize to get proper z */
    wp_f32 len = sqrtf( nx * nx + ny * ny + nz * nz );
    if( len > WP_GBUFFER_FLOAT_EPSILON )
    {
        *out_x = nx / len;
        *out_y = ny / len;
        *out_z = nz / len;
    }
    else
    {
        *out_x = 0.0f;
        *out_y = 1.0f;
        *out_z = 0.0f;
    }
}

/* =========================================================================
 * Public API Implementation
 * ====================================================================== */

wp_gbuffer *wp_gbuffer_create( wp_s32 width, wp_s32 height )
{
    if( width <= 0 || height <= 0 )
        return NULL;

    wp_gbuffer *ctx = (wp_gbuffer *)malloc( sizeof( wp_gbuffer ) );
    if( !ctx )
        return NULL;
    memset( ctx, 0, sizeof( wp_gbuffer ) );

    ctx->width = width;
    ctx->height = height;

    /* Allocate buffers */
    wp_s32 pixel_count = width * height;
    wp_s32 normal_size = pixel_count * 4;   /* RGBA */
    wp_s32 velocity_size = pixel_count * 2; /* RG */
    wp_s32 depth_size = pixel_count;        /* R */

    ctx->normal_buffer = (wp_f32 *)malloc( normal_size * sizeof( wp_f32 ) );
    ctx->velocity_buffer = (wp_f32 *)malloc( velocity_size * sizeof( wp_f32 ) );
    ctx->depth_buffer = (wp_f32 *)malloc( depth_size * sizeof( wp_f32 ) );

    /* Allocate object tracking arrays */
    ctx->prev_matrices = (wp_mat4f *)malloc( WP_GBUFFER_MAX_OBJECTS * sizeof( wp_mat4f ) );
    ctx->curr_matrices = (wp_mat4f *)malloc( WP_GBUFFER_MAX_OBJECTS * sizeof( wp_mat4f ) );
    ctx->object_ids = (wp_s32 *)malloc( WP_GBUFFER_MAX_OBJECTS * sizeof( wp_s32 ) );
    ctx->seen_ids = (wp_s32 *)malloc( WP_GBUFFER_MAX_OBJECTS * sizeof( wp_s32 ) );

    if( !ctx->normal_buffer || !ctx->velocity_buffer || !ctx->depth_buffer || !ctx->prev_matrices ||
        !ctx->curr_matrices || !ctx->object_ids || !ctx->seen_ids )
    {
        wp_gbuffer_destroy( ctx );
        return NULL;
    }

    /* Initialize buffers */
    memset( ctx->normal_buffer, 0, normal_size * sizeof( wp_f32 ) );
    memset( ctx->velocity_buffer, 0, velocity_size * sizeof( wp_f32 ) );
    memset( ctx->depth_buffer, 0, depth_size * sizeof( wp_f32 ) );

    /* Initialize identity matrices */
    for( wp_s32 i = 0; i < WP_GBUFFER_MAX_OBJECTS; i++ )
    {
        wp_gbuffer_copy_matrix( &ctx->prev_matrices[i], &wp_identity_matrix );
        wp_gbuffer_copy_matrix( &ctx->curr_matrices[i], &wp_identity_matrix );
    }

    /* Initialize VP matrices as identity */
    wp_gbuffer_copy_matrix( &ctx->prev_vp, &wp_identity_matrix );
    wp_gbuffer_copy_matrix( &ctx->curr_vp, &wp_identity_matrix );

    return ctx;
}

void wp_gbuffer_destroy( wp_gbuffer *ctx )
{
    if( !ctx )
        return;
    if( ctx->normal_buffer )
        free( ctx->normal_buffer );
    if( ctx->velocity_buffer )
        free( ctx->velocity_buffer );
    if( ctx->depth_buffer )
        free( ctx->depth_buffer );
    if( ctx->prev_matrices )
        free( ctx->prev_matrices );
    if( ctx->curr_matrices )
        free( ctx->curr_matrices );
    if( ctx->object_ids )
        free( ctx->object_ids );
    if( ctx->seen_ids )
        free( ctx->seen_ids );
    free( ctx );
}

void wp_gbuffer_resize( wp_gbuffer *ctx, wp_s32 width, wp_s32 height )
{
    if( !ctx || width <= 0 || height <= 0 )
        return;

    if( ctx->width == width && ctx->height == height )
        return;

    ctx->width = width;
    ctx->height = height;

    wp_s32 pixel_count = width * height;
    wp_s32 normal_size = pixel_count * 4;
    wp_s32 velocity_size = pixel_count * 2;
    wp_s32 depth_size = pixel_count;

    ctx->normal_buffer = (wp_f32 *)realloc( ctx->normal_buffer, normal_size * sizeof( wp_f32 ) );
    ctx->velocity_buffer = (wp_f32 *)realloc( ctx->velocity_buffer, velocity_size * sizeof( wp_f32 ) );
    ctx->depth_buffer = (wp_f32 *)realloc( ctx->depth_buffer, depth_size * sizeof( wp_f32 ) );

    memset( ctx->normal_buffer, 0, normal_size * sizeof( wp_f32 ) );
    memset( ctx->velocity_buffer, 0, velocity_size * sizeof( wp_f32 ) );
    memset( ctx->depth_buffer, 0, depth_size * sizeof( wp_f32 ) );
}

void wp_gbuffer_begin_record( wp_gbuffer *ctx )
{
    if( !ctx )
        return;
    ctx->seen_count = 0;
}

void wp_gbuffer_record_matrix( wp_gbuffer *ctx, wp_s32 object_id, const wp_mat4f *matrix )
{
    if( !ctx || !matrix )
        return;

    /* Track seen objects */
    if( ctx->seen_count < WP_GBUFFER_MAX_OBJECTS )
    {
        ctx->seen_ids[ctx->seen_count++] = object_id;
    }

    /* Find existing or add new */
    wp_s32 idx = wp_gbuffer_find_object( ctx, object_id );
    if( idx >= 0 )
    {
        /* Rotate: prev <- curr, curr <- new */
        wp_gbuffer_copy_matrix( &ctx->prev_matrices[idx], &ctx->curr_matrices[idx] );
        wp_gbuffer_copy_matrix( &ctx->curr_matrices[idx], matrix );
    }
    else
    {
        wp_gbuffer_add_object( ctx, object_id, matrix );
    }
}

void wp_gbuffer_end_record( wp_gbuffer *ctx )
{
    if( !ctx )
        return;

    /* Clean up stale entries if tracking map grew too large */
    if( ctx->object_count > ctx->seen_count * WP_GBUFFER_CLEANUP_MULTIPLIER + WP_GBUFFER_CLEANUP_MARGIN )
    {
        wp_s32 write_idx = 0;
        for( wp_s32 i = 0; i < ctx->object_count; i++ )
        {
            /* Check if this object was seen this frame */
            wp_s32 seen = 0;
            for( wp_s32 j = 0; j < ctx->seen_count; j++ )
            {
                if( ctx->object_ids[i] == ctx->seen_ids[j] )
                {
                    seen = 1;
                    break;
                }
            }
            if( seen )
            {
                if( write_idx != i )
                {
                    ctx->object_ids[write_idx] = ctx->object_ids[i];
                    wp_gbuffer_copy_matrix( &ctx->prev_matrices[write_idx], &ctx->prev_matrices[i] );
                    wp_gbuffer_copy_matrix( &ctx->curr_matrices[write_idx], &ctx->curr_matrices[i] );
                }
                write_idx++;
            }
        }
        ctx->object_count = write_idx;
    }
}

void wp_gbuffer_get_prev_matrix( wp_gbuffer *ctx, wp_s32 object_id, wp_mat4f *out_mat )
{
    if( !ctx || !out_mat )
    {
        return;
    }

    wp_s32 idx = wp_gbuffer_find_object( ctx, object_id );
    if( idx >= 0 )
    {
        wp_gbuffer_copy_matrix( out_mat, &ctx->prev_matrices[idx] );
    }
    else
    {
        wp_gbuffer_copy_matrix( out_mat, &wp_identity_matrix );
    }
}

void wp_gbuffer_set_view_proj( wp_gbuffer *ctx, const wp_mat4f *curr_vp, const wp_mat4f *prev_vp )
{
    if( !ctx )
        return;

    if( curr_vp )
    {
        wp_gbuffer_copy_matrix( &ctx->curr_vp, curr_vp );
    }
    if( prev_vp )
    {
        wp_gbuffer_copy_matrix( &ctx->prev_vp, prev_vp );
    }
}

void wp_gbuffer_clear( wp_gbuffer *ctx, wp_s32 full )
{
    if( !ctx )
        return;

    wp_s32 pixel_count = ctx->width * ctx->height;

    /* Clear depth always */
    memset( ctx->depth_buffer, 0, pixel_count * sizeof( wp_f32 ) );

    if( full )
    {
        /* Full clear - also clear normal and velocity */
        memset( ctx->normal_buffer, 0, pixel_count * 4 * sizeof( wp_f32 ) );
        memset( ctx->velocity_buffer, 0, pixel_count * 2 * sizeof( wp_f32 ) );
    }
}

void wp_gbuffer_render( wp_gbuffer *ctx, void *scene, void *camera, wp_s32 clear )
{
    (void)ctx;
    (void)scene;
    (void)camera;
    /* Software fallback - in real implementation this would
     * iterate geometry and fill buffers via GPU rendering */
    wp_gbuffer_clear( ctx, clear ? 1 : 0 );
}

const wp_f32 *wp_gbuffer_get_normal_buffer( const wp_gbuffer *ctx )
{
    return ctx ? ctx->normal_buffer : NULL;
}

const wp_f32 *wp_gbuffer_get_velocity_buffer( const wp_gbuffer *ctx )
{
    return ctx ? ctx->velocity_buffer : NULL;
}

const wp_f32 *wp_gbuffer_get_depth_buffer( const wp_gbuffer *ctx )
{
    return ctx ? ctx->depth_buffer : NULL;
}

/* Utility functions */
wp_f32 wp_gbuffer_get_coverage( const wp_f32 *normal_sample )
{
    if( !normal_sample )
        return WP_GBUFFER_COVERAGE_STATIC;
    return normal_sample[2]; /* Z component = coverage */
}

wp_f32 wp_gbuffer_get_material_id( const wp_f32 *normal_sample )
{
    if( !normal_sample )
        return 0.0f;
    return normal_sample[3]; /* W component = material_id */
}

wp_s32 wp_gbuffer_is_dynamic( const wp_f32 *normal_sample )
{
    wp_f32 coverage = wp_gbuffer_get_coverage( normal_sample );
    return ( coverage < WP_GBUFFER_COVERAGE_THRESHOLD ) ? 1 : 0;
}
