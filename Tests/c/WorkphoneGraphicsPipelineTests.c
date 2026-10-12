#include "workphone_graphics_pipeline.h"

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

static int require_true( int condition, const char *message )
{
    if( condition ) return 1;
    fprintf( stderr, "pipeline test failed: %s\n", message );
    return 0;
}

static wp_f32 exposure_sequence( wp_f32 delta_time, int frames )
{
    wp_render_pipeline *pipeline;
    wp_mat4f identity;
    wp_f32 color[3];
    wp_f32 result;
    int frame;
    pipeline = wp_pipeline_create( 1, 1, WP_PIPELINE_QUALITY_LOW );
    if( !pipeline ) return -1.0f;
    memset( &identity, 0, sizeof( identity ) );
    identity.m[0][0] = identity.m[1][1] = 1.0f;
    identity.m[2][2] = identity.m[3][3] = 1.0f;
    color[0] = color[1] = color[2] = 8.0f;
    wp_pipeline_enable_exposure( pipeline, 1 );
    wp_pipeline_set_delta_time( pipeline, delta_time );
    /* Resize must retain the selected clock, including a paused clock. */
    wp_pipeline_resize( pipeline, 2, 1 );
    wp_pipeline_resize( pipeline, 1, 1 );
    wp_pipeline_set_frame_inputs( pipeline, color, NULL, NULL, NULL );
    for( frame = 0; frame < frames; ++frame )
        wp_pipeline_render_frame( pipeline, NULL, &identity, &identity, NULL, 0, NULL );
    result = wp_pipeline_get_output( pipeline, NULL, NULL )[0];
    wp_pipeline_destroy( pipeline );
    return result;
}

int main( void )
{
    wp_render_pipeline *pipeline;
    wp_mat4f identity;
    wp_f32 color[4 * 4 * 3];
    wp_f32 normal[4 * 4 * 4];
    wp_f32 depth[4 * 4];
    wp_f32 velocity[4 * 4 * 2];
    const wp_f32 *output;
    wp_s32 width;
    wp_s32 height;
    int i;
    wp_f32 at_30hz, at_60hz, paused, clamped;

    at_30hz = exposure_sequence( 1.0f / 30.0f, 30 );
    at_60hz = exposure_sequence( 1.0f / 60.0f, 60 );
    if( !require_true( at_30hz >= 0.0f && at_60hz >= 0.0f &&
                       fabs( at_30hz - at_60hz ) < 0.00002,
                       "exposure adapts by elapsed time at 30 and 60 Hz" ) ) return 1;
    paused = exposure_sequence( 0.0f, 1 );
    if( !require_true( paused >= 0.0f && paused == exposure_sequence( 0.0f, 20 ) &&
                       paused == exposure_sequence( -1.0f, 20 ),
                       "zero/negative intervals freeze exposure, including after resize" ) ) return 1;
    clamped = exposure_sequence( 0.25f, 1 );
    if( !require_true( clamped >= 0.0f && clamped == exposure_sequence( FLT_MAX, 1 ) &&
                       fabs( clamped - paused ) > 0.0001,
                       "long stalls clamp exposure adaptation and elapsed time changes output" ) ) return 1;

    if( !require_true( wp_pipeline_create( 0, 4, WP_PIPELINE_QUALITY_LOW ) == NULL,
                       "zero width must be rejected" ) ) return 1;
    if( !require_true( wp_pipeline_create( 4, -1, WP_PIPELINE_QUALITY_LOW ) == NULL,
                       "negative height must be rejected" ) ) return 1;

    pipeline = wp_pipeline_create( 4, 4, WP_PIPELINE_QUALITY_LOW );
    if( !require_true( pipeline != NULL, "valid creation" ) ) return 1;

    memset( &identity, 0, sizeof(identity) );
    identity.m[0][0] = identity.m[1][1] = 1.0f;
    identity.m[2][2] = identity.m[3][3] = 1.0f;
    memset( normal, 0, sizeof(normal) );
    memset( velocity, 0, sizeof(velocity) );
    for( i = 0; i < 16; ++i )
    {
        color[i * 3 + 0] = 0.25f + (wp_f32)i * 0.01f;
        color[i * 3 + 1] = 0.5f;
        color[i * 3 + 2] = 1.0f;
        normal[i * 4 + 2] = 1.0f;
        normal[i * 4 + 3] = 1.0f;
        depth[i] = 1.0f;
    }

    wp_pipeline_set_frame_inputs( pipeline, color, normal, depth, velocity );
    wp_pipeline_set_view_proj( pipeline, &identity, &identity, &identity );
    wp_pipeline_render_frame( pipeline, NULL, &identity, &identity, NULL, 0, NULL );
    output = wp_pipeline_get_output( pipeline, &width, &height );
    if( !require_true( output != NULL && width == 4 && height == 4,
                       "output dimensions" ) ) return 1;
    for( i = 0; i < 4 * 4 * 3; ++i )
    {
        if( !require_true( isfinite( output[i] ) && output[i] >= 0.0f && output[i] <= 1.0f,
                           "finite display-referred output" ) ) return 1;
    }

    wp_pipeline_resize( pipeline, 1, 1 );
    output = wp_pipeline_get_output( pipeline, &width, &height );
    if( !require_true( output != NULL && width == 1 && height == 1,
                       "transactional resize" ) ) return 1;
    wp_pipeline_destroy( pipeline );

    pipeline = wp_pipeline_create( 4, 4, WP_PIPELINE_QUALITY_HIGH );
    if( !require_true( pipeline != NULL, "high-quality creation" ) ) return 1;
    wp_pipeline_set_frame_inputs( pipeline, color, normal, depth, velocity );
    wp_pipeline_set_view_proj( pipeline, &identity, &identity, &identity );
    wp_pipeline_render_frame( pipeline, NULL, &identity, &identity, NULL, 0, NULL );
    output = wp_pipeline_get_output( pipeline, &width, &height );
    for( i = 0; i < 4 * 4 * 3; ++i )
    {
        if( !require_true( output != NULL && isfinite( output[i] ),
                           "high-quality effects produce finite output" ) ) return 1;
    }
    color[0] = 2.0f;
    wp_pipeline_set_frame_inputs( pipeline, color, normal, depth, velocity );
    wp_pipeline_render_frame( pipeline, NULL, &identity, &identity, NULL, 0, NULL );
    output = wp_pipeline_get_output( pipeline, &width, &height );
    if( !require_true( output != NULL && isfinite( output[0] ) && output[0] > 0.0f,
                       "second temporal frame is resolved, not an unwritten buffer" ) ) return 1;
    wp_pipeline_destroy( pipeline );
    return 0;
}
