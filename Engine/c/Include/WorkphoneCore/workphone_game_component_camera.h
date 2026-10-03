/**
 * @file workphone_game_component_camera.h
 * @brief C API for the camera game component.
 */

#ifndef WORKPHONE_GAME_COMPONENT_CAMERA_H
#define WORKPHONE_GAME_COMPONENT_CAMERA_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_quat.h"
#include "workphone_matrix.h"
#include "workphone_game_component.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Camera projection type
 * ---------------------------------------------------------------------- */

enum wp_camera_projection
{
    WP_CAMERA_PROJECTION_PERSPECTIVE = 0,
    WP_CAMERA_PROJECTION_ORTHOGRAPHIC
};

/* -------------------------------------------------------------------------
 * Camera component
 *
 * The base wp_game_component is the first member so a wp_camera_component *
 * can be safely cast to wp_game_component * and back.
 * ---------------------------------------------------------------------- */

typedef struct wp_camera_component
{
    wp_game_component base;

    enum wp_camera_projection projection;

    wp_f32 fov_y;     /**< Vertical field of view in radians (perspective). */
    wp_f32 aspect;    /**< Viewport width / height ratio (perspective).     */
    wp_f32 near_clip; /**< Near clip plane distance.                        */
    wp_f32 far_clip;  /**< Far clip plane distance.                         */

    wp_f32 ortho_width;  /**< World-space width of ortho frustum.  */
    wp_f32 ortho_height; /**< World-space height of ortho frustum. */

    wp_s32 is_primary; /**< Non-zero if this is the active scene camera. */

    wp_vec3f position;
    wp_quatf orientation;

    wp_mat4f view_matrix;
    wp_mat4f projection_matrix;
} wp_camera_component;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_camera_component_init( wp_camera_component *cam );
void wp_camera_component_destroy( wp_camera_component *cam );

/* ---- Update ----------------------------------------------------------- */

void wp_camera_component_update( wp_camera_component *cam, wp_f64 dt );

/* ---- Projection type -------------------------------------------------- */

void wp_camera_component_set_projection( wp_camera_component *cam,
                                         enum wp_camera_projection projection );
enum wp_camera_projection wp_camera_component_get_projection( const wp_camera_component *cam );

/* ---- Perspective parameters ------------------------------------------ */

void wp_camera_component_set_fov_y( wp_camera_component *cam, wp_f32 fov_y );
wp_f32 wp_camera_component_get_fov_y( const wp_camera_component *cam );

void wp_camera_component_set_aspect( wp_camera_component *cam, wp_f32 aspect );
wp_f32 wp_camera_component_get_aspect( const wp_camera_component *cam );

/* ---- Clip planes ------------------------------------------------------ */

void wp_camera_component_set_near_clip( wp_camera_component *cam, wp_f32 near_clip );
wp_f32 wp_camera_component_get_near_clip( const wp_camera_component *cam );

void wp_camera_component_set_far_clip( wp_camera_component *cam, wp_f32 far_clip );
wp_f32 wp_camera_component_get_far_clip( const wp_camera_component *cam );

/* ---- Orthographic parameters ----------------------------------------- */

void wp_camera_component_set_ortho_size( wp_camera_component *cam, wp_f32 width, wp_f32 height );
wp_f32 wp_camera_component_get_ortho_width( const wp_camera_component *cam );
wp_f32 wp_camera_component_get_ortho_height( const wp_camera_component *cam );

/* ---- Primary flag ----------------------------------------------------- */

void wp_camera_component_set_primary( wp_camera_component *cam, wp_s32 primary );
wp_s32 wp_camera_component_is_primary( const wp_camera_component *cam );

/* ---- Position and orientation ---------------------------------------- */

void wp_camera_component_set_position( wp_camera_component *cam, wp_vec3f position );
wp_vec3f wp_camera_component_get_position( const wp_camera_component *cam );

void wp_camera_component_set_orientation( wp_camera_component *cam, wp_quatf orientation );
wp_quatf wp_camera_component_get_orientation( const wp_camera_component *cam );

void wp_camera_component_look_at( wp_camera_component *cam, wp_vec3f target, wp_vec3f world_up );

/* ---- Matrix accessors ------------------------------------------------- */

const wp_mat4f *wp_camera_component_get_view_matrix( const wp_camera_component *cam );
const wp_mat4f *wp_camera_component_get_projection_matrix( const wp_camera_component *cam );
void wp_camera_component_get_view_projection( const wp_camera_component *cam, wp_mat4f *out );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_COMPONENT_CAMERA_H */
