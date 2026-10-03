/**
 * @file wp_graphics_camera.c
 * @brief Implementation of the C graphics camera API.
 */

#include "workphone_graphics_camera.h"
#include "workphone_graphics_scenenode.h"
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Internal structure
 * ====================================================================== */

typedef struct wp_camera
{
    wp_projection_type projection_type;
    wp_f32 fov_y;
    wp_f32 aspect_ratio;
    wp_f32 near_dist;
    wp_f32 far_dist;
    wp_f32 ortho_width;
    wp_vec3f position;
    wp_quatf orientation;
    wp_viewport viewport;
    wp_u32 visibility_mask;
    wp_scenenode *node;
    wp_graphics_scene *creator;
    void *native;
} wp_camera;

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_quatf camera_look_at_quat( wp_vec3f position, wp_vec3f target, wp_vec3f up )
{
    wp_vec3f forward, zaxis, xaxis, yaxis;
    wp_f32 m00, m01, m02, m10, m11, m12, m20, m21, m22;
    wp_f32 trace, s;
    wp_quatf q;

    memset( &q, 0, sizeof( wp_quatf ) );
    q.w = 1.0f;

    forward = wp_vec3f_sub( target, position );
    if( wp_vec3f_is_zero_length( forward ) )
    {
        return q;
    }
    forward = wp_vec3f_normalize( forward );

    /* Camera looks down -Z; zaxis is the backward axis */
    zaxis = wp_vec3f_negate( forward );

    xaxis = wp_vec3f_cross( up, zaxis );
    if( wp_vec3f_is_zero_length( xaxis ) )
    {
        return q;
    }
    xaxis = wp_vec3f_normalize( xaxis );

    yaxis = wp_vec3f_cross( zaxis, xaxis );

    /* Pack basis vectors into a row-major rotation matrix and convert to quaternion */
    m00 = xaxis.x;
    m01 = yaxis.x;
    m02 = zaxis.x;
    m10 = xaxis.y;
    m11 = yaxis.y;
    m12 = zaxis.y;
    m20 = xaxis.z;
    m21 = yaxis.z;
    m22 = zaxis.z;

    trace = m00 + m11 + m22;
    if( trace > 0.0f )
    {
        s = 0.5f / sqrtf( trace + 1.0f );
        q.w = 0.25f / s;
        q.x = ( m21 - m12 ) * s;
        q.y = ( m02 - m20 ) * s;
        q.z = ( m10 - m01 ) * s;
    }
    else if( m00 > m11 && m00 > m22 )
    {
        s = 2.0f * sqrtf( 1.0f + m00 - m11 - m22 );
        q.w = ( m21 - m12 ) / s;
        q.x = 0.25f * s;
        q.y = ( m01 + m10 ) / s;
        q.z = ( m02 + m20 ) / s;
    }
    else if( m11 > m22 )
    {
        s = 2.0f * sqrtf( 1.0f + m11 - m00 - m22 );
        q.w = ( m02 - m20 ) / s;
        q.x = ( m01 + m10 ) / s;
        q.y = 0.25f * s;
        q.z = ( m12 + m21 ) / s;
    }
    else
    {
        s = 2.0f * sqrtf( 1.0f + m22 - m00 - m11 );
        q.w = ( m10 - m01 ) / s;
        q.x = ( m02 + m20 ) / s;
        q.y = ( m12 + m21 ) / s;
        q.z = 0.25f * s;
    }

    return q;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

wp_camera *wp_camera_create( void )
{
    wp_camera *camera = (wp_camera *)malloc( sizeof( wp_camera ) );
    if( !camera )
    {
        return NULL;
    }

    memset( camera, 0, sizeof( wp_camera ) );

    camera->projection_type = WORKPHONE_PROJECTION_PERSPECTIVE;
    camera->fov_y = WORKPHONE_PI_F / 4.0f;
    camera->aspect_ratio = 1.0f;
    camera->near_dist = 0.1f;
    camera->far_dist = 1000.0f;
    camera->ortho_width = 10.0f;
    camera->orientation.w = 1.0f;
    camera->viewport.left = 0.0f;
    camera->viewport.top = 0.0f;
    camera->viewport.width = 1.0f;
    camera->viewport.height = 1.0f;
    camera->visibility_mask = 0xFFFFFFFFu;

    return camera;
}

void wp_camera_destroy( wp_camera *camera )
{
    if( !camera )
    {
        return;
    }

    free( camera );
}

/* =========================================================================
 * Projection
 * ====================================================================== */

void wp_camera_set_projection_type( wp_camera *camera, wp_projection_type type )
{
    if( !camera )
    {
        return;
    }

    camera->projection_type = type;
}

wp_projection_type wp_camera_get_projection_type( const wp_camera *camera )
{
    if( !camera )
    {
        return WORKPHONE_PROJECTION_PERSPECTIVE;
    }

    return camera->projection_type;
}

/* =========================================================================
 * Field of view (perspective)
 * ====================================================================== */

void wp_camera_set_fov_y( wp_camera *camera, wp_f32 fov_y_radians )
{
    if( !camera )
    {
        return;
    }

    camera->fov_y = fov_y_radians;
}

wp_f32 wp_camera_get_fov_y( const wp_camera *camera )
{
    if( !camera )
    {
        return WORKPHONE_PI_F / 4.0f;
    }

    return camera->fov_y;
}

/* =========================================================================
 * Aspect ratio
 * ====================================================================== */

void wp_camera_set_aspect_ratio( wp_camera *camera, wp_f32 aspect_ratio )
{
    if( !camera )
    {
        return;
    }

    camera->aspect_ratio = aspect_ratio;
}

wp_f32 wp_camera_get_aspect_ratio( const wp_camera *camera )
{
    if( !camera )
    {
        return 1.0f;
    }

    return camera->aspect_ratio;
}

/* =========================================================================
 * Clip distances
 * ====================================================================== */

void wp_camera_set_near_clip_distance( wp_camera *camera, wp_f32 near_dist )
{
    if( !camera )
    {
        return;
    }

    camera->near_dist = near_dist;
}

wp_f32 wp_camera_get_near_clip_distance( const wp_camera *camera )
{
    if( !camera )
    {
        return 0.1f;
    }

    return camera->near_dist;
}

void wp_camera_set_far_clip_distance( wp_camera *camera, wp_f32 far_dist )
{
    if( !camera )
    {
        return;
    }

    camera->far_dist = far_dist;
}

wp_f32 wp_camera_get_far_clip_distance( const wp_camera *camera )
{
    if( !camera )
    {
        return 1000.0f;
    }

    return camera->far_dist;
}

/* =========================================================================
 * Orthographic extents
 * ====================================================================== */

void wp_camera_set_ortho_width( wp_camera *camera, wp_f32 ortho_width )
{
    if( !camera )
    {
        return;
    }

    camera->ortho_width = ortho_width;
}

wp_f32 wp_camera_get_ortho_width( const wp_camera *camera )
{
    if( !camera )
    {
        return 10.0f;
    }

    return camera->ortho_width;
}

/* =========================================================================
 * Transform
 * ====================================================================== */

void wp_camera_set_position( wp_camera *camera, wp_vec3f position )
{
    if( !camera )
    {
        return;
    }

    if( camera->node )
    {
        wp_scenenode_set_position( camera->node, position );
    }
    else
    {
        camera->position = position;
    }
}

wp_vec3f wp_camera_get_position( const wp_camera *camera )
{
    wp_vec3f zero;
    memset( &zero, 0, sizeof( wp_vec3f ) );

    if( !camera )
    {
        return zero;
    }

    if( camera->node )
    {
        return wp_scenenode_get_position( camera->node );
    }

    return camera->position;
}

void wp_camera_set_orientation( wp_camera *camera, wp_quatf orientation )
{
    if( !camera )
    {
        return;
    }

    if( camera->node )
    {
        wp_scenenode_set_orientation( camera->node, orientation );
    }
    else
    {
        camera->orientation = orientation;
    }
}

wp_quatf wp_camera_get_orientation( const wp_camera *camera )
{
    wp_quatf identity;
    memset( &identity, 0, sizeof( wp_quatf ) );
    identity.w = 1.0f;

    if( !camera )
    {
        return identity;
    }

    if( camera->node )
    {
        return wp_scenenode_get_orientation( camera->node );
    }

    return camera->orientation;
}

void wp_camera_get_view_matrix( const wp_camera *camera, wp_mat4f *matrix )
{
    wp_mat4f world;
    wp_vec3f position;
    wp_vec3f forward;
    wp_vec3f up;
    wp_vec3f target;
    wp_quatf orientation;

    if( !matrix )
    {
        return;
    }
    wp_mat4f_identity( matrix );
    if( !camera )
    {
        return;
    }

    if( camera->node )
    {
        wp_scenenode_get_world_matrix( camera->node, &world );
        if( wp_mat4f_invert( matrix, &world ) )
        {
            return;
        }
    }

    position = camera->position;
    orientation = wp_quatf_normalize( camera->orientation );
    forward.x = 0.0f;
    forward.y = 0.0f;
    forward.z = -1.0f;
    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;
    forward = wp_quatf_rotate_vec3( orientation, forward );
    up = wp_quatf_rotate_vec3( orientation, up );
    target = wp_vec3f_add( position, forward );
    wp_mat4f_look_at( matrix, position.x, position.y, position.z, target.x, target.y, target.z, up.x,
                      up.y, up.z );
}

void wp_camera_get_projection_matrix( const wp_camera *camera, wp_mat4f *matrix )
{
    wp_f32 aspect;
    wp_f32 near_dist;
    wp_f32 far_dist;
    wp_f32 half_width;
    wp_f32 half_height;

    if( !matrix )
    {
        return;
    }
    wp_mat4f_identity( matrix );
    if( !camera )
    {
        return;
    }

    aspect = camera->aspect_ratio > 0.000001f ? camera->aspect_ratio : 1.0f;
    near_dist = camera->near_dist > 0.000001f ? camera->near_dist : 0.000001f;
    far_dist = camera->far_dist > near_dist ? camera->far_dist : near_dist + 1.0f;
    if( camera->projection_type == WORKPHONE_PROJECTION_ORTHOGRAPHIC )
    {
        half_width = camera->ortho_width > 0.000001f ? camera->ortho_width : 1.0f;
        half_height = half_width / aspect;
        wp_mat4f_ortho( matrix, -half_width, half_width, -half_height, half_height, near_dist,
                        far_dist );
    }
    else
    {
        wp_mat4f_perspective( matrix, camera->fov_y, aspect, near_dist, far_dist );
    }
}

void wp_camera_look_at( wp_camera *camera, wp_vec3f target, wp_vec3f up )
{
    wp_vec3f pos;
    wp_quatf q;

    if( !camera )
    {
        return;
    }

    pos = wp_camera_get_position( camera );
    q = camera_look_at_quat( pos, target, up );
    wp_camera_set_orientation( camera, q );
}

/* =========================================================================
 * Viewport
 * ====================================================================== */

void wp_camera_set_viewport( wp_camera *camera, wp_viewport viewport )
{
    if( !camera )
    {
        return;
    }

    camera->viewport = viewport;
}

wp_viewport wp_camera_get_viewport( const wp_camera *camera )
{
    wp_viewport full;
    full.left = 0.0f;
    full.top = 0.0f;
    full.width = 1.0f;
    full.height = 1.0f;

    if( !camera )
    {
        return full;
    }

    return camera->viewport;
}

/* =========================================================================
 * Visibility mask
 * ====================================================================== */

void wp_camera_set_visibility_mask( wp_camera *camera, wp_u32 mask )
{
    if( !camera )
    {
        return;
    }

    camera->visibility_mask = mask;
}

wp_u32 wp_camera_get_visibility_mask( const wp_camera *camera )
{
    if( !camera )
    {
        return 0xFFFFFFFFu;
    }

    return camera->visibility_mask;
}

/* =========================================================================
 * Scene node attachment
 * ====================================================================== */

void wp_camera_attach_to_node( wp_camera *camera, wp_scenenode *node )
{
    if( !camera )
    {
        return;
    }

    camera->node = node;
}

wp_scenenode *wp_camera_get_node( const wp_camera *camera )
{
    if( !camera )
    {
        return NULL;
    }

    return camera->node;
}

/* =========================================================================
 * Creator / graphics scene
 * ====================================================================== */

wp_graphics_scene *wp_camera_get_creator( const wp_camera *camera )
{
    if( !camera )
    {
        return NULL;
    }

    return camera->creator;
}

void wp_camera_set_creator( wp_camera *camera, wp_graphics_scene *scene )
{
    if( !camera )
    {
        return;
    }

    camera->creator = scene;
}

/* =========================================================================
 * Native access
 * ====================================================================== */

void wp_camera_get_native( const wp_camera *camera, void **pp_object )
{
    if( !pp_object )
    {
        return;
    }

    *pp_object = camera ? camera->native : NULL;
}

void wp_camera_set_native( wp_camera *camera, void *native )
{
    if( !camera )
    {
        return;
    }

    camera->native = native;
}
