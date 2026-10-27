#ifndef WORKPHONE_GRAPHICS_LIGHT_GRID_H
#define WORKPHONE_GRAPHICS_LIGHT_GRID_H
#include "workphone_types.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WP_LIGHT_GRID_MAX_LIGHTS 128
#define WP_LIGHT_GRID_MAX_X 16
#define WP_LIGHT_GRID_MAX_Y 9
#define WP_LIGHT_GRID_MAX_Z 16
#define WP_LIGHT_GRID_CELL_LIGHTS 32
/* Shader-compatible float8: world position/radius and linear RGB radiance/padding. */
typedef struct wp_local_light { wp_f32 position[3], radius, radiance[3], padding; } wp_local_light;
typedef struct wp_light_grid_camera
{
    wp_f32 view_projection[16]; /* row-major, column-vector multiplication */
    wp_f32 position[3], forward[3], z_near, z_far;
} wp_light_grid_camera;
/* Conservative sphere/frustum culling. Output cells are offset/count pairs,
 * ordered x then y then linear positive camera depth. Arrays are caller-owned.
 * Each cell admits the first 32 lights deterministically; discarded references
 * are reported explicitly. Invalid inputs leave all output arrays untouched. */
wp_s32 wp_light_grid_build( const wp_local_light *lights, wp_u32 light_count,
    const wp_light_grid_camera *camera, wp_u32 grid_x, wp_u32 grid_y, wp_u32 grid_z,
    wp_u32 *cell_offset_count, wp_u32 cell_capacity, wp_u32 *indices,
    wp_u32 index_capacity, wp_u32 *index_count, wp_u32 *discarded_references );
#ifdef __cplusplus
}
#endif
#endif
