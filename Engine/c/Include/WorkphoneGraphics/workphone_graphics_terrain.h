/**
 * @file workphone_graphics_terrain.h
 * @brief Renderer-independent height-field terrain support.
 *
 * The coordinate and triangle conventions used here match OgreTerrain:
 * terrain X/Y address the height field, terrain Z is height, and alternate
 * rows use alternate triangle diagonals.
 */

#ifndef WORKPHONE_GRAPHICS_TERRAIN_H
#define WORKPHONE_GRAPHICS_TERRAIN_H

#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WP_GRAPHICS_TERRAIN_MAX_BATCH_SIZE 129

typedef struct wp_graphics_terrain wp_graphics_terrain;

typedef enum wp_graphics_terrain_alignment
{
    WP_GRAPHICS_TERRAIN_ALIGN_X_Z = 0,
    WP_GRAPHICS_TERRAIN_ALIGN_Y_Z = 1,
    WP_GRAPHICS_TERRAIN_ALIGN_X_Y = 2
} wp_graphics_terrain_alignment;

typedef struct wp_graphics_terrain_rect
{
    wp_u32 left;
    wp_u32 top;
    wp_u32 right;
    wp_u32 bottom;
} wp_graphics_terrain_rect;

typedef struct wp_graphics_terrain_ray_result
{
    wp_s32 hit;
    wp_f32 distance;
    wp_vec3f position;
} wp_graphics_terrain_ray_result;

/* Lifecycle and height storage.  A valid size is 2^n + 1 and at least 3. */
wp_graphics_terrain *wp_graphics_terrain_create( wp_u16 size, wp_f32 world_size );
void wp_graphics_terrain_destroy( wp_graphics_terrain *terrain );
wp_s32 wp_graphics_terrain_set_size( wp_graphics_terrain *terrain, wp_u16 size );
wp_u16 wp_graphics_terrain_get_size( const wp_graphics_terrain *terrain );
wp_s32 wp_graphics_terrain_set_height_data( wp_graphics_terrain *terrain, const wp_f32 *heights,
                                            wp_u32 count );
wp_f32 *wp_graphics_terrain_get_height_data( wp_graphics_terrain *terrain );
const wp_f32 *wp_graphics_terrain_get_height_data_const( const wp_graphics_terrain *terrain );
wp_f32 wp_graphics_terrain_get_height_at_point( const wp_graphics_terrain *terrain, wp_u32 x, wp_u32 y );
void wp_graphics_terrain_set_height_at_point( wp_graphics_terrain *terrain, wp_u32 x, wp_u32 y,
                                              wp_f32 height );

/* Terrain-space coordinates are normalised in X and Y. */
wp_f32 wp_graphics_terrain_get_height_at_terrain_position( const wp_graphics_terrain *terrain, wp_f32 x,
                                                           wp_f32 y );
wp_f32 wp_graphics_terrain_get_height_at_world_position( const wp_graphics_terrain *terrain,
                                                         wp_vec3f world_position );
wp_vec3f wp_graphics_terrain_get_normal_at_terrain_position( const wp_graphics_terrain *terrain,
                                                             wp_f32 x, wp_f32 y );

/* Placement and coordinate conversion. */
void wp_graphics_terrain_set_alignment( wp_graphics_terrain *terrain,
                                        wp_graphics_terrain_alignment alignment );
wp_graphics_terrain_alignment wp_graphics_terrain_get_alignment( const wp_graphics_terrain *terrain );
void wp_graphics_terrain_set_position( wp_graphics_terrain *terrain, wp_vec3f position );
wp_vec3f wp_graphics_terrain_get_position( const wp_graphics_terrain *terrain );
void wp_graphics_terrain_set_world_size( wp_graphics_terrain *terrain, wp_f32 world_size );
wp_f32 wp_graphics_terrain_get_world_size( const wp_graphics_terrain *terrain );
wp_vec3f wp_graphics_terrain_get_point( const wp_graphics_terrain *terrain, wp_u32 x, wp_u32 y );
wp_vec3f wp_graphics_terrain_terrain_to_world( const wp_graphics_terrain *terrain,
                                               wp_vec3f terrain_position );
wp_vec3f wp_graphics_terrain_world_to_terrain( const wp_graphics_terrain *terrain,
                                               wp_vec3f world_position );
wp_vec3f wp_graphics_terrain_world_to_terrain_axes( wp_graphics_terrain_alignment alignment,
                                                    wp_vec3f vector );
wp_vec3f wp_graphics_terrain_terrain_to_world_axes( wp_graphics_terrain_alignment alignment,
                                                    wp_vec3f vector );

/* Bounds, state, and renderer integration. */
wp_f32 wp_graphics_terrain_get_min_height( const wp_graphics_terrain *terrain );
wp_f32 wp_graphics_terrain_get_max_height( const wp_graphics_terrain *terrain );
wp_f32 wp_graphics_terrain_get_bounding_radius( const wp_graphics_terrain *terrain );
wp_s32 wp_graphics_terrain_is_modified( const wp_graphics_terrain *terrain );
wp_s32 wp_graphics_terrain_is_height_data_modified( const wp_graphics_terrain *terrain );
void wp_graphics_terrain_clear_modified( wp_graphics_terrain *terrain );
void wp_graphics_terrain_dirty( wp_graphics_terrain *terrain );
wp_graphics_terrain_rect wp_graphics_terrain_get_dirty_rect( const wp_graphics_terrain *terrain );
void *wp_graphics_terrain_get_native( const wp_graphics_terrain *terrain );
void wp_graphics_terrain_set_native( wp_graphics_terrain *terrain, void *native );

/* LOD and Ogre-compatible triangle-strip index generation. */
wp_u16 wp_graphics_terrain_get_lod_level_count( const wp_graphics_terrain *terrain );
wp_u16 wp_graphics_terrain_get_tree_depth( const wp_graphics_terrain *terrain );
wp_s32 wp_graphics_terrain_set_batch_sizes( wp_graphics_terrain *terrain, wp_u16 max_batch_size,
                                            wp_u16 min_batch_size );
wp_u32 wp_graphics_terrain_get_index_count_for_batch_size( wp_u16 batch_size );
wp_u16 wp_graphics_terrain_calc_skirt_vertex_index( wp_u16 main_index, wp_u16 vertex_data_size,
                                                    wp_s32 is_column, wp_u16 skirt_row_column_count,
                                                    wp_u16 skirt_row_column_skip );
wp_s32 wp_graphics_terrain_populate_index_buffer( wp_u16 *indices, wp_u32 capacity, wp_u16 batch_size,
                                                  wp_u16 vertex_data_size, wp_u16 vertex_increment,
                                                  wp_u16 x_offset, wp_u16 y_offset,
                                                  wp_u16 skirt_row_column_count,
                                                  wp_u16 skirt_row_column_skip );

wp_graphics_terrain_ray_result wp_graphics_terrain_ray_intersects( const wp_graphics_terrain *terrain,
                                                                   wp_vec3f origin, wp_vec3f direction,
                                                                   wp_f32 distance_limit );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_TERRAIN_H */
