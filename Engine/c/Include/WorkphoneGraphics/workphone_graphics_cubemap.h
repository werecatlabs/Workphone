/**
 * @file workphone_graphics_cubemap.h
 * @brief C API for cubemap creation and configuration.
 *
 * A cubemap is a set of six square textures mapped onto the faces of a cube,
 * used for environment mapping, skyboxes, and reflections.
 */

#ifndef WORKPHONE_GRAPHICS_CUBEMAP_H
#define WORKPHONE_GRAPHICS_CUBEMAP_H

#include <stdint.h>
#include "workphone_graphics_material.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WP_CUBEMAP_MAX_TEXTURES
#    define WP_CUBEMAP_MAX_TEXTURES 6
#endif

typedef struct wp_cubemap wp_cubemap;

wp_cubemap *wp_cubemap_create( void );
void wp_cubemap_destroy( wp_cubemap *cubemap );
void wp_cubemap_set_enabled( wp_cubemap *cubemap, wp_s32 enabled );
wp_s32 wp_cubemap_is_enabled( const wp_cubemap *cubemap );
void wp_cubemap_set_texture_name( wp_cubemap *cubemap, wp_s32 face, const wp_c8 *name );
const wp_c8 *wp_cubemap_get_texture_name( const wp_cubemap *cubemap, wp_s32 face );
wp_graphics_material *wp_cubemap_get_material( const wp_cubemap *cubemap );
void *wp_cubemap_get_native( const wp_cubemap *cubemap );
void wp_cubemap_set_native( wp_cubemap *cubemap, void *native );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_CUBEMAP_H */
