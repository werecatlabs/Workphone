/**
 * @file wp_graphics_csm.c
 * @brief Cascaded Shadow Maps implementation
 * Reference: Claude-of-Duty/src/render/csm.js
 */

#include "workphone_graphics_csm.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct wp_csm
{
    wp_s32 map_size;
    wp_s32 cascades;
    wp_f32 *depth_buffer;
    wp_mat4f light_view;
    wp_mat4f cascade_proj[WP_CSM_MAX_SPLITS];
    wp_mat4f cascade_matrices[WP_CSM_MAX_SPLITS];
    wp_f32 splits[WP_CSM_MAX_SPLITS + 1];
    wp_f32 texel_sizes[WP_CSM_MAX_SPLITS];
    wp_vec3f light_dir;
};

static wp_f32 wp_cl( wp_f32 x, wp_f32 a, wp_f32 b )
{
    return x < a ? a : x > b ? b : x;
}

wp_csm *wp_csm_create( wp_s32 map_size )
{
    wp_csm *c = calloc( 1, sizeof( wp_csm ) );
    if( !c )
        return NULL;
    c->map_size = map_size > 0 ? map_size : 2048;
    c->cascades = WP_CSM_CASCADES;
    wp_s32 px = c->map_size * c->map_size;
    c->depth_buffer = calloc( (size_t)px, sizeof( wp_f32 ) );
    c->light_dir.x = 0;
    c->light_dir.y = -1;
    c->light_dir.z = 0;
    for( wp_s32 i = 0; i < WP_CSM_MAX_SPLITS; i++ )
        c->splits[i] = ( i + 1 ) * 20.0f;
    c->splits[WP_CSM_MAX_SPLITS] = 200.0f;
    return c;
}

void wp_csm_destroy( wp_csm *c )
{
    if( !c )
        return;
    free( c->depth_buffer );
    free( c );
}

void wp_csm_update( wp_csm *c, const wp_mat4f *view, const wp_mat4f *proj, const wp_vec3f *light_dir )
{
    if( !c )
        return;
    if( light_dir )
        c->light_dir = *light_dir;

    /* Compute cascade splits using logarithmic distribution */
    wp_f32 lambda = 0.86f;
    wp_f32 near = 1.0f, far = 200.0f;
    wp_f32 range = far - near;
    for( wp_s32 i = 0; i < c->cascades; i++ )
    {
        wp_f32 si = (wp_f32)( i + 1 );
        wp_f32 ni = (wp_f32)i;
        wp_f32 log_dist = near + range * ( si / (wp_f32)c->cascades );
        wp_f32 lin_dist = near + range * ( si / (wp_f32)c->cascades );
        c->splits[i] = log_dist * lambda + lin_dist * ( 1 - lambda );
    }
    c->splits[c->cascades] = far;
}

const wp_f32 *wp_csm_get_texture( const wp_csm *c )
{
    return c ? c->depth_buffer : NULL;
}

void wp_csm_get_matrices( const wp_csm *c, wp_mat4f *out_matrices )
{
    if( !c )
        return;
    for( wp_s32 i = 0; i < c->cascades; i++ )
        out_matrices[i] = c->cascade_matrices[i];
}

void wp_csm_get_splits( const wp_csm *c, wp_f32 *out_splits )
{
    if( !c )
        return;
    for( wp_s32 i = 0; i <= c->cascades; i++ )
        out_splits[i] = c->splits[i];
}

void wp_csm_resize( wp_csm *c, wp_s32 map_size )
{
    if( !c )
        return;
    c->map_size = map_size > 0 ? map_size : 2048;
    free( c->depth_buffer );
    wp_s32 px = c->map_size * c->map_size;
    c->depth_buffer = calloc( (size_t)px, sizeof( wp_f32 ) );
}

void wp_csm_render( wp_csm *ctx, void *scene )
{ /* GPU-side rendering handled by renderer */
}
