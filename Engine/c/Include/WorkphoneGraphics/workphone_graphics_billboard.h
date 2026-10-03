/**
 * @file wp_graphics_camera.h
 * @brief C API for a camera used to render a view of the graphics scene.
 *
 * A camera defines a viewpowp_s32 and projection into the scene graph. It
 * supports both perspective and orthographic projections and can be attached
 * to a scene node so that its position and orientation follow the node's
 * world transform.
 */

#ifndef WORKPHONE_GRAPHICS_CAMERA_H
#define WORKPHONE_GRAPHICS_CAMERA_H

#include <stdint.h>
#include "workphone_graphics_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_camera wp_camera;
typedef struct wp_scenenode wp_scenenode;
typedef struct wp_graphics_scene wp_graphics_scene;

/* -------------------------------------------------------------------------
 * Viewport type
 * ---------------------------------------------------------------------- */

/**
 * @brief Normalised screen rectangle describing the output area of a camera.
 *
 * All coordinates are in the range [0, 1] relative to the render target
 * dimensions. (left, top) is the upper-left corner; (width, height) is
 * the extent.
 */
typedef struct
{
    wp_f32 left;   /**< Left edge in normalised [0, 1] coordinates. */
    wp_f32 top;    /**< Top edge in normalised [0, 1] coordinates. */
    wp_f32 width;  /**< Viewport width in normalised [0, 1] coordinates. */
    wp_f32 height; /**< Viewport height in normalised [0, 1] coordinates. */
} wp_viewport;

/* -------------------------------------------------------------------------
 * Projection type
 * ---------------------------------------------------------------------- */

/**
 * @brief Projection modes supported by a camera.
 */
typedef enum wp_projection_type
{
    WORKPHONE_PROJECTION_PERSPECTIVE = 0, /**< Standard frustum perspective projection. */
    WORKPHONE_PROJECTION_ORTHOGRAPHIC = 1 /**< Parallel orthographic projection. */
} wp_projection_type;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new camera with default perspective settings.
 *
 * Default values: vertical FOV of PI/4 radians (45 degrees), aspect ratio
 * 1.0, near clip 0.1, far clip 1000.0, full-screen viewport, no attached
 * scene node.
 *
 * @return Pointer to the created camera, or NULL on failure.
 */
wp_camera *wp_camera_create( void );

/**
 * @brief Destroys a camera and frees its resources.
 * @param camera Pointer to the camera to destroy. Ignored if NULL.
 */
void wp_camera_destroy( wp_camera *camera );

/* -------------------------------------------------------------------------
 * Projection
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the projection type (perspective or orthographic).
 * @param camera Pointer to the camera.
 * @param type   Projection type to apply.
 */
void wp_camera_set_projection_type( wp_camera *camera, wp_projection_type type );

/**
 * @brief Gets the current projection type.
 * @param camera Pointer to the camera.
 * @return Current projection type.
 */
wp_projection_type wp_camera_get_projection_type( const wp_camera *camera );

/* -------------------------------------------------------------------------
 * Field of view (perspective)
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the vertical field of view for perspective projection.
 * @param camera         Pointer to the camera.
 * @param fov_y_radians  Vertical FOV in radians (e.g. WORKPHONE_PI_F / 4 for 45 degrees).
 *
 * @note Has no visual effect when the camera is in orthographic mode.
 */
void wp_camera_set_fov_y( wp_camera *camera, wp_f32 fov_y_radians );

/**
 * @brief Gets the vertical field of view.
 * @param camera Pointer to the camera.
 * @return Vertical FOV in radians.
 */
wp_f32 wp_camera_get_fov_y( const wp_camera *camera );

/* -------------------------------------------------------------------------
 * Aspect ratio
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the aspect ratio (width / height) used for projection.
 * @param camera       Pointer to the camera.
 * @param aspect_ratio Width-to-height ratio (e.g. 16.0f / 9.0f).
 */
void wp_camera_set_aspect_ratio( wp_camera *camera, wp_f32 aspect_ratio );

/**
 * @brief Gets the current aspect ratio.
 * @param camera Pointer to the camera.
 * @return Width-to-height aspect ratio.
 */
wp_f32 wp_camera_get_aspect_ratio( const wp_camera *camera );

/* -------------------------------------------------------------------------
 * Clip distances
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the near clip distance.
 * @param camera    Pointer to the camera.
 * @param near_dist Distance to the near clip plane (must be > 0).
 */
void wp_camera_set_near_clip_distance( wp_camera *camera, wp_f32 near_dist );

/**
 * @brief Gets the near clip distance.
 * @param camera Pointer to the camera.
 * @return Distance to the near clip plane.
 */
wp_f32 wp_camera_get_near_clip_distance( const wp_camera *camera );

/**
 * @brief Sets the far clip distance.
 * @param camera   Pointer to the camera.
 * @param far_dist Distance to the far clip plane (must be > near clip distance).
 */
void wp_camera_set_far_clip_distance( wp_camera *camera, wp_f32 far_dist );

/**
 * @brief Gets the far clip distance.
 * @param camera Pointer to the camera.
 * @return Distance to the far clip plane.
 */
wp_f32 wp_camera_get_far_clip_distance( const wp_camera *camera );

/* -------------------------------------------------------------------------
 * Orthographic extents
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the half-width of the orthographic projection volume.
 * @param camera      Pointer to the camera.
 * @param ortho_width Half-width of the orthographic projection in world units.
 *
 * @note The height is derived from ortho_width and the current aspect ratio.
 *       Has no visual effect in perspective mode.
 */
void wp_camera_set_ortho_width( wp_camera *camera, wp_f32 ortho_width );

/**
 * @brief Gets the half-width of the orthographic projection volume.
 * @param camera Pointer to the camera.
 * @return Orthographic half-width in world units.
 */
wp_f32 wp_camera_get_ortho_width( const wp_camera *camera );

/* -------------------------------------------------------------------------
 * Transform
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the world-space position of the camera.
 * @param camera   Pointer to the camera.
 * @param position World-space position.
 *
 * @note When the camera is attached to a scene node, this sets the node's
 *       local position rather than a free-standing override.
 */
void wp_camera_set_position( wp_camera *camera, wp_vec3f position );

/**
 * @brief Gets the world-space position of the camera.
 * @param camera Pointer to the camera.
 * @return World-space camera position.
 */
wp_vec3f wp_camera_get_position( const wp_camera *camera );

/**
 * @brief Sets the world-space orientation of the camera.
 * @param camera      Pointer to the camera.
 * @param orientation Orientation as a unit quaternion.
 *
 * @note When the camera is attached to a scene node, this sets the node's
 *       local orientation rather than a free-standing override.
 */
void wp_camera_set_orientation( wp_camera *camera, wp_quatf orientation );

/**
 * @brief Gets the world-space orientation of the camera.
 * @param camera Pointer to the camera.
 * @return Orientation as a unit quaternion.
 */
wp_quatf wp_camera_get_orientation( const wp_camera *camera );

/**
 * @brief Orients the camera to look at a world-space target point.
 * @param camera Pointer to the camera.
 * @param target World-space powp_s32 to look at.
 * @param up     World-space up vector (e.g. {0.0f, 1.0f, 0.0f}).
 */
void wp_camera_look_at( wp_camera *camera, wp_vec3f target, wp_vec3f up );

/* -------------------------------------------------------------------------
 * Viewport
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the normalised viewport rectangle for this camera.
 * @param camera   Pointer to the camera.
 * @param viewport Normalised viewport; all components should be in [0, 1].
 */
void wp_camera_set_viewport( wp_camera *camera, wp_viewport viewport );

/**
 * @brief Gets the current viewport rectangle.
 * @param camera Pointer to the camera.
 * @return Current normalised viewport.
 */
wp_viewport wp_camera_get_viewport( const wp_camera *camera );

/* -------------------------------------------------------------------------
 * Visibility mask
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the visibility mask used to cull graphics objects.
 * @param camera Pointer to the camera.
 * @param mask   Bitmask ANDed with each object's visibility flags; objects
 *               whose result is zero are not rendered by this camera.
 */
void wp_camera_set_visibility_mask( wp_camera *camera, wp_u32 mask );

/**
 * @brief Gets the current visibility mask.
 * @param camera Pointer to the camera.
 * @return Current visibility mask bitmask.
 */
wp_u32 wp_camera_get_visibility_mask( const wp_camera *camera );

/* -------------------------------------------------------------------------
 * Scene node attachment
 * ---------------------------------------------------------------------- */

/**
 * @brief Attaches the camera to a scene node.
 *
 * While attached, the camera inherits the node's world-space position and
 * orientation. Calls to wp_camera_set_position / wp_camera_set_orientation
 * will modify the node's local transform.
 *
 * @param camera Pointer to the camera.
 * @param node   Pointer to the scene node to attach to, or NULL to detach.
 */
void wp_camera_attach_to_node( wp_camera *camera, wp_scenenode *node );

/**
 * @brief Gets the scene node currently attached to the camera.
 * @param camera Pointer to the camera.
 * @return Pointer to the attached scene node, or NULL if none.
 */
wp_scenenode *wp_camera_get_node( const wp_camera *camera );

/* -------------------------------------------------------------------------
 * Creator / graphics scene
 * ---------------------------------------------------------------------- */

/**
 * @brief Gets the graphics scene that created this camera.
 * @param camera Pointer to the camera.
 * @return Pointer to the creator graphics scene, or NULL if none.
 */
wp_graphics_scene *wp_camera_get_creator( const wp_camera *camera );

/**
 * @brief Sets the creator graphics scene for this camera.
 * @param camera Pointer to the camera.
 * @param scene  Pointer to the graphics scene, or NULL to clear.
 */
void wp_camera_set_creator( wp_camera *camera, wp_graphics_scene *scene );

/* -------------------------------------------------------------------------
 * Native access
 * ---------------------------------------------------------------------- */

/**
 * @brief Retrieves the underlying renderer-native camera object pointer.
 * @param camera    Pointer to the camera.
 * @param pp_object Output pointer that receives the native object pointer.
 *
 * @note The concrete type and ownership semantics of the native pointer are
 *       renderer-dependent. The value may be NULL if no native object exists.
 */
void wp_camera_get_native( const wp_camera *camera, void **pp_object );

/**
 * @brief Sets the underlying renderer-native camera object pointer.
 * @param camera Pointer to the camera.
 * @param native Pointer to the native camera object.
 */
void wp_camera_set_native( wp_camera *camera, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_CAMERA_H */
