#include "workphone_graphics_pipeline.h"
#include <stdlib.h>
#include <string.h>

struct wp_render_pipeline
{
    wp_s32 width, height;
    wp_pipeline_quality quality;
    wp_f32 *normal, *depth, *velocity, *hdr_input, *history, *output;
    wp_gtao *gtao;
    wp_ssr *ssr;
    wp_contact_shadows *contact;
    wp_taa *taa;
    wp_motion_blur *motion_blur;
    wp_dof *dof;
    wp_hdr *hdr;
    wp_csm *csm;
    wp_s32 frame, shadows_on, ao_on, ssr_on, contact_on, taa_on;
    wp_s32 motion_blur_on, dof_on, bloom_on, exposure_on;
    wp_render_debug_view debug_view;
    wp_mat4f previous_vp, current_vp, inverse_vp;
};

static wp_s32 valid_size( wp_s32 w, wp_s32 h, size_t *pixels )
{
    size_t sw, sh;
    if( w <= 0 || h <= 0 )
        return 0;
    sw = (size_t)w;
    sh = (size_t)h;
    if( sw > (size_t)-1 / sh || sw * sh > (size_t)-1 / ( 4u * sizeof( wp_f32 ) ) )
        return 0;
    if( pixels )
        *pixels = sw * sh;
    return 1;
}

static void apply_quality( wp_render_pipeline *p )
{
    p->shadows_on = p->quality >= WP_PIPELINE_QUALITY_MEDIUM;
    p->taa_on = p->quality >= WP_PIPELINE_QUALITY_MEDIUM;
    p->bloom_on = p->quality >= WP_PIPELINE_QUALITY_MEDIUM;
    p->exposure_on = p->quality >= WP_PIPELINE_QUALITY_MEDIUM;
    p->ao_on = p->quality >= WP_PIPELINE_QUALITY_HIGH;
    p->contact_on = p->quality >= WP_PIPELINE_QUALITY_MEDIUM;
    p->ssr_on = p->quality >= WP_PIPELINE_QUALITY_ULTRA;
    p->dof_on = p->quality >= WP_PIPELINE_QUALITY_ULTRA;
    p->motion_blur_on = p->quality >= WP_PIPELINE_QUALITY_CINEMATIC;
}

static void release_pipeline( wp_render_pipeline *p )
{
    if( !p )
        return;
    free( p->normal );
    free( p->depth );
    free( p->velocity );
    free( p->hdr_input );
    free( p->history );
    free( p->output );
    wp_gtao_destroy( p->gtao );
    wp_ssr_destroy( p->ssr );
    wp_contact_shadows_destroy( p->contact );
    wp_taa_destroy( p->taa );
    wp_mb_destroy( p->motion_blur );
    wp_dof_destroy( p->dof );
    wp_hdr_destroy( p->hdr );
    wp_csm_destroy( p->csm );
}

wp_render_pipeline *wp_pipeline_create( wp_s32 w, wp_s32 h, wp_pipeline_quality quality )
{
    wp_render_pipeline *p;
    size_t px;
    if( !valid_size( w, h, &px ) )
        return NULL;
    if( quality < WP_PIPELINE_QUALITY_LOW || quality > WP_PIPELINE_QUALITY_CINEMATIC )
        quality = WP_PIPELINE_QUALITY_HIGH;
    p = (wp_render_pipeline *)calloc( 1, sizeof( *p ) );
    if( !p )
        return NULL;
    p->width = w;
    p->height = h;
    p->quality = quality;
    p->normal = (wp_f32 *)calloc( px * 4u, sizeof( wp_f32 ) );
    p->depth = (wp_f32 *)calloc( px, sizeof( wp_f32 ) );
    p->velocity = (wp_f32 *)calloc( px * 2u, sizeof( wp_f32 ) );
    p->hdr_input = (wp_f32 *)calloc( px * 3u, sizeof( wp_f32 ) );
    p->history = (wp_f32 *)calloc( px * 3u, sizeof( wp_f32 ) );
    p->output = (wp_f32 *)calloc( px * 3u, sizeof( wp_f32 ) );
    p->gtao = wp_gtao_create( w, h, NULL );
    p->ssr = wp_ssr_create( w, h );
    p->contact = wp_contact_shadows_create( w, h, NULL );
    p->taa = wp_taa_create( w, h, NULL );
    p->motion_blur = wp_mb_create( w, h );
    p->dof = wp_dof_create( w, h );
    p->hdr = wp_hdr_create( w, h );
    p->csm = wp_csm_create( quality >= WP_PIPELINE_QUALITY_CINEMATIC ? 4096 : 2048 );
    if( !p->normal || !p->depth || !p->velocity || !p->hdr_input || !p->history || !p->output ||
        !p->gtao || !p->ssr || !p->contact || !p->taa || !p->motion_blur || !p->dof || !p->hdr ||
        !p->csm )
    {
        release_pipeline( p );
        free( p );
        return NULL;
    }
    apply_quality( p );
    return p;
}

void wp_pipeline_destroy( wp_render_pipeline *p )
{
    if( !p )
        return;
    release_pipeline( p );
    free( p );
}

void wp_pipeline_set_frame_inputs( wp_render_pipeline *p, const wp_f32 *color, const wp_f32 *normal,
                              const wp_f32 *depth, const wp_f32 *velocity )
{
    size_t px;
    if( !p || !valid_size( p->width, p->height, &px ) )
        return;
    if( color )
        memcpy( p->hdr_input, color, px * 3u * sizeof( wp_f32 ) );
    if( normal )
        memcpy( p->normal, normal, px * 4u * sizeof( wp_f32 ) );
    if( depth )
        memcpy( p->depth, depth, px * sizeof( wp_f32 ) );
    if( velocity )
        memcpy( p->velocity, velocity, px * 2u * sizeof( wp_f32 ) );
}

void wp_pipeline_render_frame( wp_render_pipeline *p, void *scene, const wp_mat4f *view, const wp_mat4f *proj,
                          void *lights, wp_s32 light_count, wp_f32 *out_color )
{
    const wp_f32 *source, *bloom, *exposure, *taa_history;
    wp_mat4f inverse_projection;
    wp_vec3f sun_direction;
    size_t px;
    (void)view;
    (void)lights;
    (void)light_count;
    if( !p || !proj || !valid_size( p->width, p->height, &px ) )
        return;
    if( !wp_mat4f_invert( &inverse_projection, proj ) )
        inverse_projection = p->inverse_vp;
    sun_direction.x = 0.3f;
    sun_direction.y = -0.8f;
    sun_direction.z = 0.52f;
    if( p->shadows_on )
        wp_csm_render( p->csm, scene );
    if( p->ao_on )
        wp_gtao_render( p->gtao, p->depth, p->normal, &inverse_projection, proj, p->frame, 1 );
    if( p->ssr_on )
        wp_ssr_render( p->ssr, p->hdr_input, p->depth, p->normal, p->velocity, proj, &inverse_projection,
                       p->frame );
    if( p->contact_on )
        wp_contact_shadows_render( p->contact, p->depth, p->normal, proj, &inverse_projection,
                                   &sun_direction, p->frame );
    source = p->hdr_input;
    if( p->taa_on )
    {
        wp_taa_next_jitter( p->taa );
        taa_history = p->frame > 0 ? wp_taa_get_buffer( p->taa ) : NULL;
        wp_taa_resolve( p->taa, source, taa_history, p->velocity, p->normal, p->depth, &p->current_vp,
                        &p->previous_vp, &p->inverse_vp, p->frame );
        source = wp_taa_get_buffer( p->taa );
    }
    if( p->motion_blur_on )
    {
        wp_mb_render( p->motion_blur, source, p->velocity, p->depth, p->normal, p->frame, 0.5f );
        source = wp_mb_get_texture( p->motion_blur );
    }
    if( p->dof_on )
    {
        wp_dof_render( p->dof, source, p->depth, p->frame, 5.0f );
        source = wp_dof_get_texture( p->dof );
    }
    bloom = p->bloom_on ? wp_hdr_bloom_render( p->hdr, source, p->width, p->height ) : NULL;
    exposure = p->exposure_on
                   ? wp_hdr_exposure_update( p->hdr, source, p->depth, p->width, p->height, 0.016f )
                   : NULL;
    wp_hdr_composite( p->hdr, source, bloom, exposure, p->width, p->height, p->output );
    if( out_color )
        memcpy( out_color, p->output, px * 3u * sizeof( wp_f32 ) );
    p->previous_vp = p->current_vp;
    ++p->frame;
}

const wp_f32 *wp_pipeline_get_output( const wp_render_pipeline *p, wp_s32 *w, wp_s32 *h )
{
    if( w )
        *w = p ? p->width : 0;
    if( h )
        *h = p ? p->height : 0;
    return p ? p->output : NULL;
}
const wp_f32 *wp_pipeline_get_gbuffer_normal( const wp_render_pipeline *p )
{
    return p ? p->normal : NULL;
}
const wp_f32 *wp_pipeline_get_gbuffer_depth( const wp_render_pipeline *p )
{
    return p ? p->depth : NULL;
}
const wp_f32 *wp_pipeline_get_gbuffer_velocity( const wp_render_pipeline *p )
{
    return p ? p->velocity : NULL;
}

void wp_pipeline_resize( wp_render_pipeline *p, wp_s32 w, wp_s32 h )
{
    wp_render_pipeline *n;
    wp_s32 flags[9];
    if( !p || ( p->width == w && p->height == h ) )
        return;
    n = wp_pipeline_create( w, h, p->quality );
    if( !n )
        return;
    flags[0] = p->shadows_on;
    flags[1] = p->ao_on;
    flags[2] = p->ssr_on;
    flags[3] = p->contact_on;
    flags[4] = p->taa_on;
    flags[5] = p->motion_blur_on;
    flags[6] = p->dof_on;
    flags[7] = p->bloom_on;
    flags[8] = p->exposure_on;
    release_pipeline( p );
    *p = *n;
    free( n );
    p->shadows_on = flags[0];
    p->ao_on = flags[1];
    p->ssr_on = flags[2];
    p->contact_on = flags[3];
    p->taa_on = flags[4];
    p->motion_blur_on = flags[5];
    p->dof_on = flags[6];
    p->bloom_on = flags[7];
    p->exposure_on = flags[8];
}

void wp_pipeline_set_view_proj( wp_render_pipeline *p, const wp_mat4f *current, const wp_mat4f *previous,
                           const wp_mat4f *inverse )
{
    if( !p )
        return;
    if( current )
        p->current_vp = *current;
    if( previous )
        p->previous_vp = *previous;
    if( inverse )
        p->inverse_vp = *inverse;
}

void wp_pipeline_enable_shadows( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->shadows_on = !!e;
}
void wp_pipeline_enable_ao( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->ao_on = !!e;
}
void wp_pipeline_enable_ssr( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->ssr_on = !!e;
}
void wp_pipeline_enable_contact_shadows( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->contact_on = !!e;
}
void wp_pipeline_enable_taa( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->taa_on = !!e;
}
void wp_pipeline_enable_motion_blur( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->motion_blur_on = !!e;
}
void wp_pipeline_enable_dof( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->dof_on = !!e;
}
void wp_pipeline_enable_bloom( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->bloom_on = !!e;
}
void wp_pipeline_enable_exposure( wp_render_pipeline *p, wp_s32 e )
{
    if( p )
        p->exposure_on = !!e;
}
void wp_pipeline_set_shadow_quality( wp_render_pipeline *p, wp_s32 c, wp_s32 s )
{
    if( p && s > 0 )
        wp_csm_resize( p->csm, s );
    (void)c;
}
void wp_pipeline_set_ao_params( wp_render_pipeline *p, wp_f32 r, wp_f32 i )
{
    if( p )
    {
        wp_gtao_set_radius( p->gtao, r );
        wp_gtao_set_intensity( p->gtao, i );
    }
}
void wp_pipeline_set_taa_params( wp_render_pipeline *p, wp_f32 f )
{
    if( p )
        wp_taa_set_feedback( p->taa, f );
}
void wp_pipeline_set_bloom_params( wp_render_pipeline *p, wp_f32 t, wp_f32 s )
{
    if( p )
        wp_hdr_bloom_set( p->hdr, t, s );
}
void wp_pipeline_set_debug_view( wp_render_pipeline *p, wp_render_debug_view m )
{
    if( p )
        p->debug_view = m;
}
