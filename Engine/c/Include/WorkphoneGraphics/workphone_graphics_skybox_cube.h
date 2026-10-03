/**
 * @file workphone_graphics_skybox_cube.h
 * @brief C API for cube skybox rendering.
 */

#ifndef WORKPHONE_GRAPHICS_SKYBOX_CUBE_H
#define WORKPHONE_GRAPHICS_SKYBOX_CUBE_H

#include <stdint.h>
#include "workphone_graphics_material.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to a cube skybox object.
 */
typedef struct wp_skybox_cube wp_skybox_cube;

/**
 * @brief Creates a new cube skybox object.
 * @return Pointer to the created object, or NULL on failure.
 */
wp_skybox_cube *wp_skybox_cube_create( void );

/**
 * @brief Destroys the cube skybox object and releases its resources.
 * @param skybox The object to destroy.
 */
void wp_skybox_cube_destroy( wp_skybox_cube *skybox );

/**
 * @brief Loads the skybox into the scene.
 * @param skybox   The skybox object.
 * @param scene_mgr Pointer to the scene manager.
 * @param camera    Pointer to the active camera.
 * @param size      The size of the skybox cube.
 */
void wp_skybox_cube_load( wp_skybox_cube *skybox, void *scene_mgr, void *camera, float size );

/**
 * @brief Loads the skybox using provided data.
 * @param skybox The skybox object.
 * @param data   The data object used for loading.
 */
void wp_skybox_cube_load_data( wp_skybox_cube *skybox, void *data );

/**
 * @brief Unloads the skybox from the scene.
 * @param skybox The skybox object.
 */
void wp_skybox_cube_unload( wp_skybox_cube *skybox );

/**
 * @brief Applies a material to the skybox.
 * @param skybox   The skybox object.
 * @param material The material to apply.
 */
void wp_skybox_cube_apply_material( wp_skybox_cube *skybox, wp_graphics_material *material );

/**
 * @brief Updates the skybox state (e.g., frustum alignment).
 * @param skybox The skybox object.
 */
void wp_skybox_cube_update( wp_skybox_cube *skybox );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_SKYBOX_CUBE_H */
