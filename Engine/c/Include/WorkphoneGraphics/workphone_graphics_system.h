/**
 * @file wp_graphics_system.h
 * @brief C API for the top-level graphics system and scene management.
 *
 * The graphics system is the entry powp_s32 for the rendering subsystem.  It
 * owns one or more graphics scenes, drives the update/render loop, and acts
 * as the factory for per-scene resources.
 *
 * A graphics scene manages a hierarchy of scene nodes and the graphics objects
 * attached to them.  Scene nodes carry spatial transforms; graphics objects
 * represent the renderable entities positioned by those nodes.
 */

#ifndef WORKPHONE_GRAPHICS_SYSTEM_H
#define WORKPHONE_GRAPHICS_SYSTEM_H

#include <stdint.h>
#include "workphone_graphics_renderer.h"
#include "workphone_graphics_object.h"
#include "workphone_graphics_scenenode.h"
#include "workphone_graphics_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_graphics_system wp_graphics_system;

/**
 * @brief Callback used by the single-entry frame driver.
 *
 * The callback owns target-specific frame orchestration. It may call
 * wp_graphics_system_render() while the appropriate render target is active.
 */
typedef void ( *wp_graphics_system_frame_func )( wp_graphics_system *system, wp_f32 delta_time,
                                                 void *user_data );

/* =========================================================================
 * Graphics system
 * ====================================================================== */

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new graphics system with no scenes.
 * @return Pointer to the created graphics system, or NULL on failure.
 */
wp_graphics_system *wp_graphics_system_create( void );

/**
 * @brief Destroys the graphics system and all scenes it owns.
 * @param system Pointer to the graphics system to destroy. Ignored if NULL.
 */
void wp_graphics_system_destroy( wp_graphics_system *system );

/* -------------------------------------------------------------------------
 * Scene management
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new graphics scene owned by this system.
 * @param system Pointer to the graphics system.
 * @return Pointer to the new scene, or NULL on failure.
 */
wp_graphics_scene *wp_graphics_system_create_scene( wp_graphics_system *system );

/**
 * @brief Destroys a scene and removes it from the system.
 *
 * All scene nodes and graphics objects that were created via this scene are
 * also destroyed.
 *
 * @param system Pointer to the graphics system.
 * @param scene  Pointer to the scene to destroy. Ignored if NULL or not owned.
 */
void wp_graphics_system_destroy_scene( wp_graphics_system *system, wp_graphics_scene *scene );

/**
 * @brief Returns the scene at the given zero-based index.
 * @param system Pointer to the graphics system.
 * @param index  Zero-based scene index.
 * @return Pointer to the scene, or NULL if the index is out of range.
 */
wp_graphics_scene *wp_graphics_system_get_scene( const wp_graphics_system *system, wp_s32 index );

/**
 * @brief Returns the number of scenes owned by the system.
 * @param system Pointer to the graphics system.
 * @return Number of active scenes.
 */
wp_s32 wp_graphics_system_get_scene_count( const wp_graphics_system *system );

/* -------------------------------------------------------------------------
 * Update and render
 * ---------------------------------------------------------------------- */

/**
 * @brief Advances the graphics system by one logical tick.
 * @param system Pointer to the graphics system.
 */
void wp_graphics_system_update( wp_graphics_system *system );

/**
 * @brief Issues draw calls for all managed scenes.
 * @param system Pointer to the graphics system.
 */
void wp_graphics_system_render( wp_graphics_system *system );

/**
 * @brief Advances and renders one complete graphics frame.
 *
 * Native scene state is updated exactly once. If a frame callback is registered,
 * it performs target-specific rendering; otherwise all native scenes are rendered
 * once using the currently bound renderer.
 *
 * @param system Pointer to the graphics system.
 * @param delta_time Elapsed frame time in seconds. Invalid values are clamped.
 */
void wp_graphics_system_render_frame( wp_graphics_system *system, wp_f32 delta_time );

/**
 * @brief Installs the target-specific renderer used by render_frame().
 * @param system Pointer to the graphics system.
 * @param frame_func Callback, or NULL to use the native single-target fallback.
 * @param user_data Opaque pointer passed to frame_func.
 */
void wp_graphics_system_set_frame_func( wp_graphics_system *system,
                                        wp_graphics_system_frame_func frame_func, void *user_data );

/* -------------------------------------------------------------------------
 * Native access
 * ---------------------------------------------------------------------- */

/**
 * @brief Retrieves the native implementation pointer.
 * @param system    Pointer to the graphics system.
 * @param pp_object Output pointer that receives the native object pointer.
 */
void wp_graphics_system_get_native( const wp_graphics_system *system, void **pp_object );

/**
 * @brief Sets the native implementation pointer.
 * @param system Pointer to the graphics system.
 * @param native Pointer to the native object.
 */
void wp_graphics_system_set_native( wp_graphics_system *system, void *native );

/* -------------------------------------------------------------------------
 * Renderer binding
 * ---------------------------------------------------------------------- */

/**
 * @brief Binds a renderer to the graphics system.
 *
 * The renderer is used by wp_graphics_system_render to issue draw calls for
 * every visible object that has a render callback registered.
 *
 * @param system   Pointer to the graphics system.
 * @param renderer Pointer to the renderer, or NULL to clear.
 */
void wp_graphics_system_set_renderer( wp_graphics_system *system, wp_renderer *renderer );

/**
 * @brief Returns the renderer currently bound to the graphics system.
 * @param system Pointer to the graphics system.
 * @return Pointer to the bound renderer, or NULL if none is set.
 */
wp_renderer *wp_graphics_system_get_renderer( const wp_graphics_system *system );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_SYSTEM_H */
