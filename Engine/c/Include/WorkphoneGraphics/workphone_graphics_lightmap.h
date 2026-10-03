/**
 * @file workphone_graphics_lightmap.h
 * @brief C API for lightmap generation and storage.
 *
 * A lightmap stores pre-computed light-intensity values for a mesh surface in
 * a 2D texture. Texels are filled by rasterising each mesh triangle in UV
 * (lightmap) space and evaluating a directional-light + ambient model at the
 * interpolated world-space position and normal. Texels not covered by any
 * triangle are filled from the nearest valid neighbour so that bilinear
 * filtering does not produce dark-fringe artefacts.
 */

#ifndef WORKPHONE_GRAPHICS_LIGHTMAP_H
#define WORKPHONE_GRAPHICS_LIGHTMAP_H

#include <stdint.h>
#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WP_LIGHTMAP_MAX_NAME
#    define WP_LIGHTMAP_MAX_NAME 256
#endif

typedef struct wp_lightmap wp_lightmap;
typedef struct wp_lightmapper wp_lightmapper;

/* =========================================================================
 * wp_lightmap  --  per-mesh baked lightmap texture
 * ====================================================================== */

wp_lightmap *wp_lightmap_create( wp_s32 tex_size );
void wp_lightmap_destroy( wp_lightmap *lm );
wp_s32 wp_lightmap_get_width( const wp_lightmap *lm );
wp_s32 wp_lightmap_get_height( const wp_lightmap *lm );
const wp_c8 *wp_lightmap_get_name( const wp_lightmap *lm );
void wp_lightmap_set_name( wp_lightmap *lm, const wp_c8 *name );
const wp_u8 *wp_lightmap_get_pixels( const wp_lightmap *lm );
wp_u8 wp_lightmap_get_intensity( const wp_lightmap *lm, wp_s32 x, wp_s32 y );
void wp_lightmap_light_triangle( wp_lightmap *lm, wp_vec3f p1, wp_vec3f p2, wp_vec3f p3, wp_vec3f n1,
                                 wp_vec3f n2, wp_vec3f n3, wp_vec2f t1, wp_vec2f t2, wp_vec2f t3,
                                 wp_vec3f light_dir, wp_f32 ambient );
void wp_lightmap_fill_invalid_pixels( wp_lightmap *lm );
void *wp_lightmap_get_native( const wp_lightmap *lm );
void wp_lightmap_set_native( wp_lightmap *lm, void *native );

/* =========================================================================
 * wp_lightmapper  --  lightmap generation manager
 * ====================================================================== */

wp_lightmapper *wp_lightmapper_create( void );
void wp_lightmapper_destroy( wp_lightmapper *lmr );
wp_s32 wp_lightmapper_get_tex_size( const wp_lightmapper *lmr );
void wp_lightmapper_set_tex_size( wp_lightmapper *lmr, wp_s32 tex_size );
wp_vec3f wp_lightmapper_get_light_dir( const wp_lightmapper *lmr );
void wp_lightmapper_set_light_dir( wp_lightmapper *lmr, wp_vec3f dir );
wp_f32 wp_lightmapper_get_ambient( const wp_lightmapper *lmr );
void wp_lightmapper_set_ambient( wp_lightmapper *lmr, wp_f32 ambient );
wp_s32 wp_lightmapper_get_auto_size( const wp_lightmapper *lmr );
void wp_lightmapper_set_auto_size( wp_lightmapper *lmr, wp_s32 enabled );
wp_lightmap *wp_lightmapper_create_lightmap( wp_lightmapper *lmr, const wp_c8 *name, wp_s32 tex_size );
wp_s32 wp_lightmapper_get_lightmap_count( const wp_lightmapper *lmr );
wp_lightmap *wp_lightmapper_get_lightmap( const wp_lightmapper *lmr, wp_s32 index );
void wp_lightmapper_remove_lightmap( wp_lightmapper *lmr, wp_lightmap *lm );
void wp_lightmapper_clear_lightmaps( wp_lightmapper *lmr );
void wp_lightmapper_generate( wp_lightmapper *lmr );
void wp_lightmapper_reset_counter( void );
wp_s32 wp_lightmapper_next_counter( void );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_LIGHTMAP_H */
