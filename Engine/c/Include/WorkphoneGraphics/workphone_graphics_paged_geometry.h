/**
 * @file workphone_graphics_paged_geometry.h
 * @brief Renderer-independent paged geometry and distance-LOD management.
 *
 * This is a C89 adaptation of the PagedGeometry core.  Rendering is supplied
 * by page callbacks, which keeps the paging code independent of Ogre or any
 * other renderer.
 */

#ifndef WORKPHONE_GRAPHICS_PAGED_GEOMETRY_H
#define WORKPHONE_GRAPHICS_PAGED_GEOMETRY_H

#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_paged_geometry wp_paged_geometry;
typedef struct wp_geometry_page_manager wp_geometry_page_manager;
typedef struct wp_geometry_page wp_geometry_page;
typedef struct wp_camera wp_camera;

typedef struct wp_paged_geometry_bounds
{
    wp_f32 left;
    wp_f32 top;
    wp_f32 right;
    wp_f32 bottom;
} wp_paged_geometry_bounds;

typedef struct wp_paged_geometry_aabb
{
    wp_vec3f min;
    wp_vec3f max;
} wp_paged_geometry_aabb;

typedef struct wp_paged_geometry_color
{
    wp_f32 r;
    wp_f32 g;
    wp_f32 b;
    wp_f32 a;
} wp_paged_geometry_color;

/* Quaternion layout is w, x, y, z, matching Workphone's math API. */
typedef struct wp_paged_geometry_quat
{
    wp_f32 w;
    wp_f32 x;
    wp_f32 y;
    wp_f32 z;
} wp_paged_geometry_quat;

typedef struct wp_geometry_page_info
{
    wp_paged_geometry_bounds bounds;
    wp_vec3f center_point;
    wp_s32 x_index;
    wp_s32 z_index;
    void *user_data;
} wp_geometry_page_info;

/*
 * Page implementations are ordinary C objects addressed through page_data.
 * create_page and destroy_page own that object.  All other callbacks are
 * optional after the per-page object has been created.
 */
typedef struct wp_geometry_page_callbacks
{
    void *( *create_page )( void *factory_data );
    void ( *destroy_page )( void *page_data );
    void ( *init )( void *page_data, wp_paged_geometry *geometry, const void *detail_data );
    void ( *set_region )( void *page_data, wp_f32 left, wp_f32 top, wp_f32 right, wp_f32 bottom );
    void ( *add_entity )( void *page_data, void *entity, wp_vec3f position,
                          wp_paged_geometry_quat rotation, wp_vec3f scale,
                          wp_paged_geometry_color color );
    void ( *build )( void *page_data );
    void ( *remove_entities )( void *page_data );
    void ( *set_fade )( void *page_data, wp_s32 enabled, wp_f32 visible_distance,
                        wp_f32 invisible_distance );
    void ( *set_visible )( void *page_data, wp_s32 visible );
    void ( *update )( void *page_data );
} wp_geometry_page_callbacks;

typedef struct wp_page_loader_callbacks
{
    void ( *load_page )( void *loader_data, wp_geometry_page *page, wp_geometry_page_info *info );
    void ( *unload_page )( void *loader_data, wp_geometry_page_info *info );
    void ( *frame_update )( void *loader_data );
} wp_page_loader_callbacks;

/* Lifecycle. page_size must be positive. */
wp_paged_geometry *wp_paged_geometry_create( wp_camera *camera, wp_f32 page_size );
void wp_paged_geometry_destroy( wp_paged_geometry *geometry );

/* Configuration which affects the grid is rejected after adding a detail level. */
wp_s32 wp_paged_geometry_set_page_size( wp_paged_geometry *geometry, wp_f32 page_size );
wp_f32 wp_paged_geometry_get_page_size( const wp_paged_geometry *geometry );
wp_s32 wp_paged_geometry_set_bounds( wp_paged_geometry *geometry, wp_paged_geometry_bounds bounds );
void wp_paged_geometry_set_infinite( wp_paged_geometry *geometry );
wp_paged_geometry_bounds wp_paged_geometry_get_bounds( const wp_paged_geometry *geometry );
void wp_paged_geometry_set_camera( wp_paged_geometry *geometry, wp_camera *camera );
wp_camera *wp_paged_geometry_get_camera( const wp_paged_geometry *geometry );
void wp_paged_geometry_set_page_loader( wp_paged_geometry *geometry,
                                        const wp_page_loader_callbacks *callbacks, void *loader_data );

/* Detail levels must be added from nearest to farthest. */
wp_geometry_page_manager *wp_paged_geometry_add_detail_level(
    wp_paged_geometry *geometry, wp_f32 max_range, wp_f32 transition_length,
    const wp_geometry_page_callbacks *callbacks, void *factory_data, const void *detail_data,
    wp_u32 query_flag );
void wp_paged_geometry_remove_detail_levels( wp_paged_geometry *geometry );
wp_u32 wp_paged_geometry_get_detail_level_count( const wp_paged_geometry *geometry );

/*
 * delta_milliseconds is explicit to make the portable implementation
 * deterministic and easy to integrate with any application clock.
 */
void wp_paged_geometry_update( wp_paged_geometry *geometry, wp_u32 delta_milliseconds );

/* Reloading and preloading operations mirror the original C++ API. */
void wp_paged_geometry_reload( wp_paged_geometry *geometry );
void wp_paged_geometry_reload_page( wp_paged_geometry *geometry, wp_vec3f point );
void wp_paged_geometry_reload_pages_radius( wp_paged_geometry *geometry, wp_vec3f center,
                                            wp_f32 radius );
void wp_paged_geometry_reload_pages_bounds( wp_paged_geometry *geometry,
                                            wp_paged_geometry_bounds bounds );
void wp_paged_geometry_preload( wp_paged_geometry *geometry, wp_paged_geometry_bounds bounds );
void wp_paged_geometry_reset_preloaded( wp_paged_geometry *geometry );

void wp_paged_geometry_set_visible( wp_paged_geometry *geometry, wp_s32 visible );
wp_s32 wp_paged_geometry_is_visible( const wp_paged_geometry *geometry );

/* PageLoader helper corresponding to PageLoader::addEntity(). */
void wp_geometry_page_add_entity( wp_geometry_page *page, void *entity, wp_vec3f position,
                                  wp_paged_geometry_quat rotation, wp_vec3f scale,
                                  wp_paged_geometry_color color,
                                  const wp_paged_geometry_aabb *entity_local_bounds );
void *wp_geometry_page_get_data( wp_geometry_page *page );
wp_vec3f wp_geometry_page_get_center( const wp_geometry_page *page );
wp_s32 wp_geometry_page_is_loaded( const wp_geometry_page *page );
wp_s32 wp_geometry_page_is_visible( const wp_geometry_page *page );
wp_u32 wp_geometry_page_get_query_flag( const wp_geometry_page *page );
wp_paged_geometry_aabb wp_geometry_page_get_bounding_box( const wp_geometry_page *page );

/* Advanced per-detail-level tuning and inspection. */
wp_f32 wp_geometry_page_manager_get_near_range( const wp_geometry_page_manager *manager );
wp_f32 wp_geometry_page_manager_get_far_range( const wp_geometry_page_manager *manager );
wp_f32 wp_geometry_page_manager_get_transition( const wp_geometry_page_manager *manager );
void wp_geometry_page_manager_set_cache_speed( wp_geometry_page_manager *manager,
                                               wp_u32 max_cache_interval, wp_u32 inactive_page_life );
wp_u32 wp_geometry_page_manager_get_loaded_count( const wp_geometry_page_manager *manager );
wp_u32 wp_geometry_page_manager_get_pending_count( const wp_geometry_page_manager *manager );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_PAGED_GEOMETRY_H */
