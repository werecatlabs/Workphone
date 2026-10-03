/**
 * @file workphone_game_component_camera.c
 * @brief Implementation of the camera game component.
 */

#include "workphone_game_component_camera.h"
#include "workphone_game_component.h"
#include "workphone_game_actor.h"
#include "workphone_matrix.h"
#include "workphone_quat.h"
#include "workphone_vector.h"

#include <string.h>

/* =========================================================================
 * Default values
 * ====================================================================== */

#define WP_CAMERA_DEFAULT_FOV ( 0.7854f )    /* 45 degrees */
#define WP_CAMERA_DEFAULT_ASPECT ( 1.7778f ) /* 16:9       */
#define WP_CAMERA_DEFAULT_NEAR ( 0.1f )
#define WP_CAMERA_DEFAULT_FAR ( 1000.0f )
#define WP_CAMERA_DEFAULT_ORTHO_W ( 10.0f )
#define WP_CAMERA_DEFAULT_ORTHO_H ( 10.0f )

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void s_recompute_view( wp_camera_component *cam )
{
    wp_vec3f forward, up, eye, target;

    forward = wp_quatf_rotate_vec3( cam->orientation, wp_vec3f_make( 0.0f, 0.0f, -1.0f ) );
    up = wp_quatf_rotate_vec3( cam->orientation, wp_vec3f_make( 0.0f, 1.0f, 0.0f ) );
    eye = cam->position;
    target = wp_vec3f_add( eye, forward );

    wp_mat4f_look_at( &cam->view_matrix, eye.x, eye.y, eye.z, target.x, target.y, target.z, up.x, up.y,
                      up.z );
}

static void s_recompute_projection( wp_camera_component *cam )
{
    wp_f32 hw, hh;

    if( cam->projection == WP_CAMERA_PROJECTION_PERSPECTIVE )
    {
        wp_mat4f_perspective( &cam->projection_matrix, cam->fov_y, cam->aspect, cam->near_clip,
                              cam->far_clip );
    }
    else
    {
        hw = cam->ortho_width * 0.5f;
        hh = cam->ortho_height * 0.5f;
        wp_mat4f_ortho( &cam->projection_matrix, -hw, hw, -hh, hh, cam->near_clip, cam->far_clip );
    }
}

/* =========================================================================
 * Component callbacks
 * ====================================================================== */

static void s_on_create( wp_game_component *comp )
{
    wp_camera_component *cam = (wp_camera_component *)comp;
    s_recompute_view( cam );
    s_recompute_projection( cam );
}

static void s_on_destroy( wp_game_component *comp )
{
    (void)comp;
}

static void s_on_update( wp_game_component *comp, wp_f64 dt )
{
    wp_camera_component *cam = (wp_camera_component *)comp;
    wp_transform3f world;

    (void)dt;

    if( comp->actor )
    {
        world = comp->actor->world_transform;
        cam->position = world.position;
        cam->orientation = world.orientation;
    }

    s_recompute_view( cam );
}

static void s_on_transform_updated( wp_game_component *comp )
{
    wp_camera_component *cam = (wp_camera_component *)comp;
    wp_transform3f world;

    if( comp->actor )
    {
        world = comp->actor->world_transform;
        cam->position = world.position;
        cam->orientation = world.orientation;
    }

    s_recompute_view( cam );
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_camera_component_init( wp_camera_component *cam )
{
    wp_component_callbacks cb;

    if( !cam )
        return;

    memset( cam, 0, sizeof( *cam ) );

    wp_game_component_init_with_type( &cam->base, WP_COMPONENT_TYPE_CAMERA );
    wp_game_component_set_name( &cam->base, "Camera" );

    cam->projection = WP_CAMERA_PROJECTION_PERSPECTIVE;
    cam->fov_y = WP_CAMERA_DEFAULT_FOV;
    cam->aspect = WP_CAMERA_DEFAULT_ASPECT;
    cam->near_clip = WP_CAMERA_DEFAULT_NEAR;
    cam->far_clip = WP_CAMERA_DEFAULT_FAR;
    cam->ortho_width = WP_CAMERA_DEFAULT_ORTHO_W;
    cam->ortho_height = WP_CAMERA_DEFAULT_ORTHO_H;
    cam->is_primary = 0;

    cam->position = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    cam->orientation = wp_quatf_identity();

    memset( &cb, 0, sizeof( cb ) );
    cb.on_create = s_on_create;
    cb.on_destroy = s_on_destroy;
    cb.on_update = s_on_update;
    cb.on_transform_updated = s_on_transform_updated;
    wp_game_component_set_callbacks( &cam->base, cb );

    s_recompute_view( cam );
    s_recompute_projection( cam );
}

void wp_camera_component_destroy( wp_camera_component *cam )
{
    if( !cam )
        return;

    wp_game_component_destroy( &cam->base );
}

/* =========================================================================
 * Update
 * ====================================================================== */

void wp_camera_component_update( wp_camera_component *cam, wp_f64 dt )
{
    if( !cam )
        return;

    wp_game_component_update( &cam->base, dt );
}

/* =========================================================================
 * Projection type
 * ====================================================================== */

void wp_camera_component_set_projection( wp_camera_component *cam, enum wp_camera_projection projection )
{
    if( !cam )
        return;

    cam->projection = projection;
    s_recompute_projection( cam );
}

enum wp_camera_projection wp_camera_component_get_projection( const wp_camera_component *cam )
{
    return cam ? cam->projection : WP_CAMERA_PROJECTION_PERSPECTIVE;
}

/* =========================================================================
 * Perspective parameters
 * ====================================================================== */

void wp_camera_component_set_fov_y( wp_camera_component *cam, wp_f32 fov_y )
{
    if( !cam )
        return;

    cam->fov_y = fov_y;
    if( cam->projection == WP_CAMERA_PROJECTION_PERSPECTIVE )
        s_recompute_projection( cam );
}

wp_f32 wp_camera_component_get_fov_y( const wp_camera_component *cam )
{
    return cam ? cam->fov_y : WP_CAMERA_DEFAULT_FOV;
}

void wp_camera_component_set_aspect( wp_camera_component *cam, wp_f32 aspect )
{
    if( !cam )
        return;

    cam->aspect = aspect;
    if( cam->projection == WP_CAMERA_PROJECTION_PERSPECTIVE )
        s_recompute_projection( cam );
}

wp_f32 wp_camera_component_get_aspect( const wp_camera_component *cam )
{
    return cam ? cam->aspect : WP_CAMERA_DEFAULT_ASPECT;
}

/* =========================================================================
 * Clip planes
 * ====================================================================== */

void wp_camera_component_set_near_clip( wp_camera_component *cam, wp_f32 near_clip )
{
    if( !cam )
        return;

    cam->near_clip = near_clip;
    s_recompute_projection( cam );
}

wp_f32 wp_camera_component_get_near_clip( const wp_camera_component *cam )
{
    return cam ? cam->near_clip : WP_CAMERA_DEFAULT_NEAR;
}

void wp_camera_component_set_far_clip( wp_camera_component *cam, wp_f32 far_clip )
{
    if( !cam )
        return;

    cam->far_clip = far_clip;
    s_recompute_projection( cam );
}

wp_f32 wp_camera_component_get_far_clip( const wp_camera_component *cam )
{
    return cam ? cam->far_clip : WP_CAMERA_DEFAULT_FAR;
}

/* =========================================================================
 * Orthographic parameters
 * ====================================================================== */

void wp_camera_component_set_ortho_size( wp_camera_component *cam, wp_f32 width, wp_f32 height )
{
    if( !cam )
        return;

    cam->ortho_width = width;
    cam->ortho_height = height;
    if( cam->projection == WP_CAMERA_PROJECTION_ORTHOGRAPHIC )
        s_recompute_projection( cam );
}

wp_f32 wp_camera_component_get_ortho_width( const wp_camera_component *cam )
{
    return cam ? cam->ortho_width : WP_CAMERA_DEFAULT_ORTHO_W;
}

wp_f32 wp_camera_component_get_ortho_height( const wp_camera_component *cam )
{
    return cam ? cam->ortho_height : WP_CAMERA_DEFAULT_ORTHO_H;
}

/* =========================================================================
 * Primary flag
 * ====================================================================== */

void wp_camera_component_set_primary( wp_camera_component *cam, wp_s32 primary )
{
    if( cam )
        cam->is_primary = primary;
}

wp_s32 wp_camera_component_is_primary( const wp_camera_component *cam )
{
    return cam ? cam->is_primary : 0;
}

/* =========================================================================
 * Position and orientation
 * ====================================================================== */

void wp_camera_component_set_position( wp_camera_component *cam, wp_vec3f position )
{
    if( !cam )
        return;

    cam->position = position;
    s_recompute_view( cam );
}

wp_vec3f wp_camera_component_get_position( const wp_camera_component *cam )
{
    wp_vec3f zero;
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    return cam ? cam->position : zero;
}

void wp_camera_component_set_orientation( wp_camera_component *cam, wp_quatf orientation )
{
    if( !cam )
        return;

    cam->orientation = orientation;
    s_recompute_view( cam );
}

wp_quatf wp_camera_component_get_orientation( const wp_camera_component *cam )
{
    return cam ? cam->orientation : wp_quatf_identity();
}

void wp_camera_component_look_at( wp_camera_component *cam, wp_vec3f target, wp_vec3f world_up )
{
    wp_vec3f forward, right, up;

    if( !cam )
        return;

    forward = wp_vec3f_normalize( wp_vec3f_sub( target, cam->position ) );
    right = wp_vec3f_normalize( wp_vec3f_cross( forward, world_up ) );
    up = wp_vec3f_cross( right, forward );

    wp_mat4f_look_at( &cam->view_matrix, cam->position.x, cam->position.y, cam->position.z, target.x,
                      target.y, target.z, up.x, up.y, up.z );
}

/* =========================================================================
 * Matrix accessors
 * ====================================================================== */

const wp_mat4f *wp_camera_component_get_view_matrix( const wp_camera_component *cam )
{
    return cam ? &cam->view_matrix : NULL;
}

const wp_mat4f *wp_camera_component_get_projection_matrix( const wp_camera_component *cam )
{
    return cam ? &cam->projection_matrix : NULL;
}

void wp_camera_component_get_view_projection( const wp_camera_component *cam, wp_mat4f *out )
{
    if( !cam || !out )
        return;

    wp_mat4f_mul( out, &cam->projection_matrix, &cam->view_matrix );
}
