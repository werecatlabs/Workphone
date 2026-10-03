/**
 * @file workphone_graphics_skybox.h
 * @brief C API for skybox creation and configuration.
 *
 * A skybox is a cube or sphere rendered at a fixed distance from the camera,
 * textured with environment images to simulate distant scenery.
 */

#ifndef WORKPHONE_GRAPHICS_SKYBOX_H
#define WORKPHONE_GRAPHICS_SKYBOX_H

#include <stdint.h>
#include "workphone_graphics_material.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WP_SKYBOX_MAX_TEXTURES
#    define WP_SKYBOX_MAX_TEXTURES 6
#endif

typedef struct wp_skybox wp_skybox;

wp_skybox *wp_skybox_create( void );
void wp_skybox_destroy( wp_skybox *skybox );
void wp_skybox_set_enabled( wp_skybox *skybox, wp_s32 enabled );
wp_s32 wp_skybox_is_enabled( const wp_skybox *skybox );
void wp_skybox_set_distance( wp_skybox *skybox, float distance );
float wp_skybox_get_distance( const wp_skybox *skybox );
void wp_skybox_set_texture_name( wp_skybox *skybox, wp_s32 face, const wp_c8 *name );
const wp_c8 *wp_skybox_get_texture_name( const wp_skybox *skybox, wp_s32 face );
wp_graphics_material *wp_skybox_get_material( const wp_skybox *skybox );
void *wp_skybox_get_native( const wp_skybox *skybox );
void wp_skybox_set_native( wp_skybox *skybox, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_SKYBOX_H */
