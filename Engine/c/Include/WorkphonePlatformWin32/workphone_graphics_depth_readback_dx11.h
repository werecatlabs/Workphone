#ifndef WORKPHONE_GRAPHICS_DEPTH_READBACK_DX11_H
#define WORKPHONE_GRAPHICS_DEPTH_READBACK_DX11_H

#include "workphone_graphics_renderer_dx11.h"

#ifdef _WIN32
#    ifdef __cplusplus
extern "C" {
#    endif

typedef struct wp_depth_readback_dx11 wp_depth_readback_dx11;

wp_depth_readback_dx11 *wp_depth_readback_dx11_create( void );
void wp_depth_readback_dx11_destroy( wp_depth_readback_dx11 *readback );

wp_s32 wp_renderer_dx11_depth_surface( wp_renderer_dx11 *renderer, wp_u32 *width, wp_u32 *height,
                                       wp_u32 *samples );

/* Render-owner call. Writes width * height farthest-depth samples on success. Each
 * dimension is bounded by the scene depth texture and 256. Capacity counts floats. */
wp_s32 wp_renderer_dx11_read_depth( wp_depth_readback_dx11 *readback, wp_renderer_dx11 *renderer,
                                   wp_f32 *depth, wp_u32 width, wp_u32 height, wp_u32 capacity );

#    ifdef __cplusplus
}
#    endif
#endif

#endif
