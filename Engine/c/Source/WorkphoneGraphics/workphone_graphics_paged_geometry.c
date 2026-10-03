/**
 * @file workphone_graphics_paged_geometry.c
 * @brief C89 implementation of the PagedGeometry paging core.
 *
 * Adapted from the renderer-facing C++ implementation in WPGraphicsOgre.
 */

#include "workphone_graphics_paged_geometry.h"
#include "workphone_graphics_camera.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define WP_PAGED_EPSILON 0.00001f
#define WP_PAGED_ROOT_TWO 1.41421356f

struct wp_geometry_page
{
    wp_geometry_page_manager *manager;
    void *page_data;
    wp_vec3f center;
    wp_s32 x_index;
    wp_s32 z_index;
    wp_u32 inactive_time;
    wp_s32 visible;
    wp_s32 fade_enabled;
    wp_s32 pending;
    wp_s32 loaded;
    wp_s32 needs_unload;
    wp_s32 keep_loaded;
    wp_s32 bounds_undefined;
    wp_paged_geometry_aabb true_bounds;
    void *user_data;
    wp_u32 query_flag;
};

struct wp_geometry_page_manager
{
    wp_paged_geometry *geometry;
    wp_geometry_page **grid;
    wp_geometry_page **scroll_buffer;
    wp_s32 grid_x;
    wp_s32 grid_z;
    wp_paged_geometry_bounds grid_bounds;
    wp_geometry_page_callbacks callbacks;
    void *factory_data;
    const void *detail_data;
    wp_f32 fade_length;
    wp_s32 fade_enabled;
    wp_u32 cache_timer;
    wp_u32 max_cache_interval;
    wp_u32 inactive_page_life;
    wp_f32 near_distance;
    wp_f32 near_distance_sq;
    wp_f32 far_distance;
    wp_f32 far_distance_sq;
    wp_f32 far_transition_distance;
    wp_f32 far_transition_distance_sq;
};

struct wp_paged_geometry
{
    wp_camera *camera;
    wp_camera *last_camera;
    wp_vec3f old_camera_position;
    wp_vec3f last_old_camera_position;
    wp_page_loader_callbacks loader_callbacks;
    void *loader_data;
    wp_geometry_page_manager **managers;
    wp_u32 manager_count;
    wp_u32 manager_capacity;
    wp_paged_geometry_bounds bounds;
    wp_f32 page_size;
    wp_s32 visible;
};

static wp_vec3f wp_paged_zero_vec3( void )
{
    wp_vec3f value;
    value.x = 0.0f;
    value.y = 0.0f;
    value.z = 0.0f;
    return value;
}

static wp_s32 wp_paged_floor_to_s32( wp_f32 value )
{
    return (wp_s32)floor( (double)value );
}

static wp_s32 wp_paged_ceil_to_s32( wp_f32 value )
{
    return (wp_s32)ceil( (double)value );
}

static wp_f32 wp_paged_bounds_width( wp_paged_geometry_bounds bounds )
{
    return bounds.right - bounds.left;
}

static wp_f32 wp_paged_bounds_height( wp_paged_geometry_bounds bounds )
{
    return bounds.bottom - bounds.top;
}

static void wp_paged_normalize_bounds( wp_paged_geometry_bounds *bounds )
{
    wp_f32 temporary;
    if( bounds->left > bounds->right )
    {
        temporary = bounds->left;
        bounds->left = bounds->right;
        bounds->right = temporary;
    }
    if( bounds->top > bounds->bottom )
    {
        temporary = bounds->top;
        bounds->top = bounds->bottom;
        bounds->bottom = temporary;
    }
}

static wp_geometry_page *wp_paged_grid_page( const wp_geometry_page_manager *manager, wp_s32 x,
                                             wp_s32 z )
{
    return manager->grid[z * manager->grid_x + x];
}

static void wp_paged_set_grid_page( wp_geometry_page_manager *manager, wp_s32 x, wp_s32 z,
                                    wp_geometry_page *page )
{
    manager->grid[z * manager->grid_x + x] = page;
}

static void wp_paged_clear_page_bounds( wp_geometry_page *page )
{
    page->true_bounds.min = wp_paged_zero_vec3();
    page->true_bounds.max = wp_paged_zero_vec3();
    page->bounds_undefined = 1;
}

static void wp_paged_page_info( const wp_geometry_page *page, wp_geometry_page_info *info )
{
    wp_f32 half_page_size;
    half_page_size = page->manager->geometry->page_size * 0.5f;
    info->bounds.left = page->center.x - half_page_size;
    info->bounds.right = page->center.x + half_page_size;
    info->bounds.top = page->center.z - half_page_size;
    info->bounds.bottom = page->center.z + half_page_size;
    info->center_point = page->center;
    info->x_index = page->x_index;
    info->z_index = page->z_index;
    info->user_data = page->user_data;
}

static void wp_paged_remove_page_entities( wp_geometry_page *page )
{
    if( page->manager->callbacks.remove_entities != NULL )
        page->manager->callbacks.remove_entities( page->page_data );
}

static void wp_paged_unload_page( wp_geometry_page *page )
{
    wp_paged_geometry *geometry;
    wp_geometry_page_info info;

    geometry = page->manager->geometry;
    wp_paged_page_info( page, &info );
    wp_paged_remove_page_entities( page );
    if( geometry->loader_callbacks.unload_page != NULL )
        geometry->loader_callbacks.unload_page( geometry->loader_data, &info );
    page->user_data = NULL;
    page->needs_unload = 0;
    wp_paged_clear_page_bounds( page );
    page->inactive_time = 0U;
    page->loaded = 0;
    page->pending = 0;
    page->fade_enabled = 0;
}

static void wp_paged_load_page( wp_geometry_page *page )
{
    wp_paged_geometry *geometry;
    wp_geometry_page_info info;

    geometry = page->manager->geometry;
    wp_paged_page_info( page, &info );
    if( page->needs_unload )
    {
        wp_paged_remove_page_entities( page );
        if( geometry->loader_callbacks.unload_page != NULL )
            geometry->loader_callbacks.unload_page( geometry->loader_data, &info );
        page->user_data = NULL;
        page->needs_unload = 0;
        wp_paged_clear_page_bounds( page );
        info.user_data = NULL;
    }
    if( page->manager->callbacks.set_region != NULL )
        page->manager->callbacks.set_region( page->page_data, info.bounds.left, info.bounds.top,
                                             info.bounds.right, info.bounds.bottom );
    if( geometry->loader_callbacks.load_page != NULL )
        geometry->loader_callbacks.load_page( geometry->loader_data, page, &info );
    page->user_data = info.user_data;
    if( page->manager->callbacks.build != NULL )
        page->manager->callbacks.build( page->page_data );
    if( page->manager->callbacks.set_visible != NULL )
        page->manager->callbacks.set_visible( page->page_data, page->visible );
    page->inactive_time = 0U;
    page->loaded = 1;
    page->pending = 0;
    page->fade_enabled = 0;
}

static void wp_paged_unload_page_delayed( wp_geometry_page *page )
{
    page->needs_unload = 1;
    page->loaded = 0;
    page->pending = 0;
}

static void wp_paged_destroy_manager( wp_geometry_page_manager *manager )
{
    wp_s32 index;
    wp_s32 count;
    wp_geometry_page *page;

    if( manager == NULL )
        return;
    count = manager->grid_x * manager->grid_z;
    for( index = 0; index < count; ++index )
    {
        page = manager->grid[index];
        if( page == NULL )
            continue;
        if( page->loaded || page->needs_unload )
            wp_paged_unload_page( page );
        if( manager->callbacks.destroy_page != NULL )
            manager->callbacks.destroy_page( page->page_data );
        free( page );
    }
    free( manager->grid );
    free( manager->scroll_buffer );
    free( manager );
}

static void wp_paged_set_transition( wp_geometry_page_manager *manager, wp_f32 transition )
{
    if( transition > 0.0f )
    {
        manager->fade_length = transition;
        manager->fade_enabled = 1;
    }
    else
    {
        manager->fade_length = 0.0f;
        manager->fade_enabled = 0;
    }
    manager->far_transition_distance = manager->far_distance + manager->fade_length;
    manager->far_transition_distance_sq =
        manager->far_transition_distance * manager->far_transition_distance;
}

static wp_s32 wp_paged_init_manager_grid( wp_geometry_page_manager *manager, wp_u32 query_flag )
{
    wp_paged_geometry *geometry;
    wp_paged_geometry_bounds bounds;
    wp_s32 x;
    wp_s32 z;
    wp_s32 x_index_offset;
    wp_s32 z_index_offset;
    wp_geometry_page *page;

    geometry = manager->geometry;
    bounds = geometry->bounds;
    if( wp_paged_bounds_width( bounds ) < WP_PAGED_EPSILON )
    {
        manager->grid_x = (wp_s32)( 2.0f * manager->far_transition_distance / geometry->page_size ) + 4;
        if( manager->grid_x < 4 )
            manager->grid_x = 4;
        manager->grid_bounds.left = 0.0f;
        manager->grid_bounds.top = 0.0f;
        manager->grid_bounds.right = (wp_f32)manager->grid_x * geometry->page_size;
        manager->grid_bounds.bottom = manager->grid_bounds.right;
        manager->scroll_buffer =
            (wp_geometry_page **)calloc( (size_t)manager->grid_x, sizeof( wp_geometry_page * ) );
        if( manager->scroll_buffer == NULL )
            return 0;
    }
    else
    {
        manager->grid_bounds = bounds;
        manager->grid_x = wp_paged_ceil_to_s32( wp_paged_bounds_width( bounds ) / geometry->page_size );
    }
    manager->grid_z = manager->grid_x;
    manager->grid = (wp_geometry_page **)calloc( (size_t)manager->grid_x * (size_t)manager->grid_z,
                                                 sizeof( wp_geometry_page * ) );
    if( manager->grid == NULL )
        return 0;
    x_index_offset = wp_paged_floor_to_s32( manager->grid_bounds.left / geometry->page_size );
    z_index_offset = wp_paged_floor_to_s32( manager->grid_bounds.top / geometry->page_size );
    for( z = 0; z < manager->grid_z; ++z )
    {
        for( x = 0; x < manager->grid_x; ++x )
        {
            page = (wp_geometry_page *)calloc( 1, sizeof( *page ) );
            if( page == NULL )
                return 0;
            page->manager = manager;
            if( manager->callbacks.create_page != NULL )
            {
                page->page_data = manager->callbacks.create_page( manager->factory_data );
                if( page->page_data == NULL )
                {
                    free( page );
                    return 0;
                }
            }
            page->center.x = ( (wp_f32)x + 0.5f ) * geometry->page_size + manager->grid_bounds.left;
            page->center.y = 0.0f;
            page->center.z = ( (wp_f32)z + 0.5f ) * geometry->page_size + manager->grid_bounds.top;
            page->x_index = x + x_index_offset;
            page->z_index = z + z_index_offset;
            page->query_flag = query_flag;
            page->bounds_undefined = 1;
            if( manager->callbacks.init != NULL )
                manager->callbacks.init( page->page_data, geometry, manager->detail_data );
            wp_paged_set_grid_page( manager, x, z, page );
        }
    }
    return 1;
}

/*
 * Move an infinite grid while retaining all pages which remain in range.
 * The temporary grid lets X and Z shifts be handled in one C89-safe pass.
 */
static wp_s32 wp_paged_scroll_grid( wp_geometry_page_manager *manager, wp_s32 shift_x, wp_s32 shift_z )
{
    wp_geometry_page **new_grid;
    wp_geometry_page **spares;
    wp_s32 spare_count;
    wp_s32 spare_index;
    wp_s32 x;
    wp_s32 z;
    wp_s32 old_x;
    wp_s32 old_z;
    wp_s32 index;
    wp_geometry_page *page;
    wp_f32 page_size;

    new_grid = (wp_geometry_page **)calloc( (size_t)manager->grid_x * (size_t)manager->grid_z,
                                            sizeof( wp_geometry_page * ) );
    spares = (wp_geometry_page **)malloc( (size_t)manager->grid_x * (size_t)manager->grid_z *
                                          sizeof( wp_geometry_page * ) );
    if( new_grid == NULL || spares == NULL )
    {
        free( new_grid );
        free( spares );
        return 0;
    }
    spare_count = 0;
    for( z = 0; z < manager->grid_z; ++z )
    {
        for( x = 0; x < manager->grid_x; ++x )
        {
            old_x = x + shift_x;
            old_z = z + shift_z;
            if( old_x >= 0 && old_x < manager->grid_x && old_z >= 0 && old_z < manager->grid_z )
            {
                new_grid[z * manager->grid_x + x] = wp_paged_grid_page( manager, old_x, old_z );
            }
        }
    }
    for( index = 0; index < manager->grid_x * manager->grid_z; ++index )
    {
        page = manager->grid[index];
        for( spare_index = 0; spare_index < manager->grid_x * manager->grid_z; ++spare_index )
        {
            if( new_grid[spare_index] == page )
                break;
        }
        if( spare_index == manager->grid_x * manager->grid_z )
            spares[spare_count++] = page;
    }
    spare_index = 0;
    page_size = manager->geometry->page_size;
    for( z = 0; z < manager->grid_z; ++z )
    {
        for( x = 0; x < manager->grid_x; ++x )
        {
            index = z * manager->grid_x + x;
            if( new_grid[index] != NULL )
                continue;
            page = spares[spare_index++];
            if( page->loaded )
                wp_paged_unload_page_delayed( page );
            else
                page->pending = 0;
            page->keep_loaded = 0;
            page->center.x = manager->grid_bounds.left + (wp_f32)shift_x * page_size +
                             ( (wp_f32)x + 0.5f ) * page_size;
            page->center.z = manager->grid_bounds.top + (wp_f32)shift_z * page_size +
                             ( (wp_f32)z + 0.5f ) * page_size;
            page->x_index = wp_paged_floor_to_s32( page->center.x / page_size );
            page->z_index = wp_paged_floor_to_s32( page->center.z / page_size );
            new_grid[index] = page;
        }
    }
    free( manager->grid );
    manager->grid = new_grid;
    free( spares );
    return 1;
}

static void wp_paged_set_page_visible( wp_geometry_page *page, wp_s32 visible )
{
    visible = visible ? 1 : 0;
    if( page->visible == visible )
        return;
    if( page->manager->callbacks.set_visible != NULL )
        page->manager->callbacks.set_visible( page->page_data, visible );
    page->visible = visible;
}

static wp_f32 wp_paged_page_overlap( const wp_geometry_page *page, wp_f32 half_page_size )
{
    wp_f32 overlap;
    wp_f32 value;
    if( page->bounds_undefined )
        return 0.0f;
    overlap = 0.0f;
    value = page->true_bounds.max.x - half_page_size;
    if( value > overlap )
        overlap = value;
    value = page->true_bounds.max.z - half_page_size;
    if( value > overlap )
        overlap = value;
    value = -page->true_bounds.min.x - half_page_size;
    if( value > overlap )
        overlap = value;
    value = -page->true_bounds.min.z - half_page_size;
    if( value > overlap )
        overlap = value;
    return overlap;
}

static void wp_paged_update_manager( wp_geometry_page_manager *manager, wp_u32 delta_time,
                                     wp_vec3f camera_position, wp_vec3f camera_speed,
                                     wp_s32 *cache_enabled, wp_geometry_page_manager *previous )
{
    wp_f32 cache_distance;
    wp_f32 cache_distance_sq;
    wp_f32 distance_sq;
    wp_f32 dx;
    wp_f32 dz;
    wp_f32 speed;
    wp_f32 overlap;
    wp_f32 page_length_sq;
    wp_f32 half_page_size;
    wp_f32 fade_near;
    wp_f32 fade_far;
    wp_u32 pending_count;
    wp_u32 cache_interval;
    wp_s32 x1;
    wp_s32 x2;
    wp_s32 z1;
    wp_s32 z2;
    wp_s32 shift_x;
    wp_s32 shift_z;
    wp_s32 x;
    wp_s32 z;
    wp_s32 index;
    wp_s32 count;
    wp_s32 visible;
    wp_s32 enable_fade;
    wp_geometry_page *page;

    cache_distance = manager->far_transition_distance + manager->geometry->page_size;
    cache_distance_sq = cache_distance * cache_distance;
    x1 = wp_paged_floor_to_s32( ( camera_position.x - cache_distance - manager->grid_bounds.left ) /
                                manager->geometry->page_size );
    x2 = wp_paged_floor_to_s32( ( camera_position.x + cache_distance - manager->grid_bounds.left ) /
                                manager->geometry->page_size );
    z1 = wp_paged_floor_to_s32( ( camera_position.z - cache_distance - manager->grid_bounds.top ) /
                                manager->geometry->page_size );
    z2 = wp_paged_floor_to_s32( ( camera_position.z + cache_distance - manager->grid_bounds.top ) /
                                manager->geometry->page_size );

    if( manager->scroll_buffer != NULL )
    {
        shift_x = 0;
        shift_z = 0;
        if( x1 < 0 )
            shift_x = x1;
        else if( x2 >= manager->grid_x - 1 )
            shift_x = x2 - ( manager->grid_x - 1 );
        if( z1 < 0 )
            shift_z = z1;
        else if( z2 >= manager->grid_z - 1 )
            shift_z = z2 - ( manager->grid_z - 1 );
        if( shift_x != 0 || shift_z != 0 )
        {
            if( wp_paged_scroll_grid( manager, shift_x, shift_z ) )
            {
                manager->grid_bounds.left += (wp_f32)shift_x * manager->geometry->page_size;
                manager->grid_bounds.right += (wp_f32)shift_x * manager->geometry->page_size;
                manager->grid_bounds.top += (wp_f32)shift_z * manager->geometry->page_size;
                manager->grid_bounds.bottom += (wp_f32)shift_z * manager->geometry->page_size;
                x1 -= shift_x;
                x2 -= shift_x;
                z1 -= shift_z;
                z2 -= shift_z;
            }
        }
    }
    if( x1 < 0 )
        x1 = 0;
    if( z1 < 0 )
        z1 = 0;
    if( x2 >= manager->grid_x )
        x2 = manager->grid_x - 1;
    if( z2 >= manager->grid_z )
        z2 = manager->grid_z - 1;

    for( z = z1; z <= z2; ++z )
    {
        for( x = x1; x <= x2; ++x )
        {
            page = wp_paged_grid_page( manager, x, z );
            dx = camera_position.x - page->center.x;
            dz = camera_position.z - page->center.z;
            distance_sq = dx * dx + dz * dz;
            if( distance_sq <= cache_distance_sq )
            {
                if( !page->loaded )
                {
                    if( distance_sq >= manager->near_distance_sq &&
                        distance_sq < manager->far_transition_distance_sq )
                        wp_paged_load_page( page );
                    else
                        page->pending = 1;
                }
                else
                    page->inactive_time = 0U;
            }
        }
    }

    pending_count = 0U;
    count = manager->grid_x * manager->grid_z;
    for( index = 0; index < count; ++index )
        if( manager->grid[index]->pending )
            ++pending_count;
    speed =
        (wp_f32)sqrt( (double)( camera_speed.x * camera_speed.x + camera_speed.z * camera_speed.z ) );
    if( speed <= WP_PAGED_EPSILON || pending_count == 0U )
        cache_interval = manager->max_cache_interval;
    else
    {
        cache_interval =
            (wp_u32)( manager->geometry->page_size * 0.8f / ( speed * (wp_f32)pending_count ) );
        if( cache_interval > manager->max_cache_interval )
            cache_interval = manager->max_cache_interval;
    }
    manager->cache_timer += delta_time;
    if( manager->cache_timer >= cache_interval && *cache_enabled )
    {
        for( index = 0; index < count; ++index )
        {
            page = manager->grid[index];
            if( !page->pending )
                continue;
            page->pending = 0;
            dx = camera_position.x - page->center.x;
            dz = camera_position.z - page->center.z;
            distance_sq = dx * dx + dz * dz;
            if( distance_sq <= cache_distance_sq )
            {
                wp_paged_load_page( page );
                *cache_enabled = 0;
                break;
            }
        }
        manager->cache_timer = 0U;
    }

    half_page_size = manager->geometry->page_size * 0.5f;
    for( index = 0; index < count; ++index )
    {
        page = manager->grid[index];
        if( !page->loaded )
            continue;
        if( page->inactive_time >= manager->inactive_page_life )
        {
            if( !page->keep_loaded )
            {
                wp_paged_unload_page( page );
                continue;
            }
            page->inactive_time = 0U;
        }
        dx = camera_position.x - page->center.x;
        dz = camera_position.z - page->center.z;
        distance_sq = dx * dx + dz * dz;
        overlap = wp_paged_page_overlap( page, half_page_size );
        page_length_sq = ( manager->geometry->page_size + overlap ) * WP_PAGED_ROOT_TWO;
        page_length_sq *= page_length_sq;
        visible = 0;
        enable_fade = 0;
        fade_near = 0.0f;
        fade_far = 0.0f;
        if( distance_sq + page_length_sq >= manager->near_distance_sq &&
            distance_sq - page_length_sq < manager->far_transition_distance_sq )
        {
            if( manager->fade_enabled && distance_sq + page_length_sq >= manager->far_distance_sq )
            {
                visible = 1;
                enable_fade = 1;
                fade_near = manager->far_distance;
                fade_far = manager->far_transition_distance;
            }
            else if( previous != NULL && previous->fade_enabled &&
                     distance_sq - page_length_sq < previous->far_transition_distance_sq )
            {
                visible = 1;
                enable_fade = 1;
                fade_near = previous->far_distance +
                            ( previous->far_transition_distance - previous->far_distance ) * 0.5f;
                fade_far = previous->far_distance;
            }
            if( enable_fade != page->fade_enabled )
            {
                if( manager->callbacks.set_fade != NULL )
                    manager->callbacks.set_fade( page->page_data, enable_fade, fade_near, fade_far );
                page->fade_enabled = enable_fade;
            }
        }
        if( distance_sq >= manager->near_distance_sq && distance_sq < manager->far_distance_sq )
            visible = 1;
        if( !manager->geometry->visible )
            visible = 0;
        wp_paged_set_page_visible( page, visible );
        if( manager->callbacks.update != NULL )
            manager->callbacks.update( page->page_data );
        if( (wp_u32)( ~0U ) - page->inactive_time < delta_time )
            page->inactive_time = (wp_u32)( ~0U );
        else
            page->inactive_time += delta_time;
    }
}

wp_paged_geometry *wp_paged_geometry_create( wp_camera *camera, wp_f32 page_size )
{
    wp_paged_geometry *geometry;
    if( page_size <= 0.0f )
        return NULL;
    geometry = (wp_paged_geometry *)calloc( 1, sizeof( *geometry ) );
    if( geometry == NULL )
        return NULL;
    geometry->camera = camera;
    geometry->page_size = page_size;
    geometry->visible = 1;
    if( camera != NULL )
        geometry->old_camera_position = wp_camera_get_position( camera );
    else
        geometry->old_camera_position = wp_paged_zero_vec3();
    return geometry;
}

void wp_paged_geometry_destroy( wp_paged_geometry *geometry )
{
    if( geometry == NULL )
        return;
    wp_paged_geometry_remove_detail_levels( geometry );
    free( geometry->managers );
    free( geometry );
}

wp_s32 wp_paged_geometry_set_page_size( wp_paged_geometry *geometry, wp_f32 page_size )
{
    if( geometry == NULL || page_size <= 0.0f || geometry->manager_count != 0U )
        return 0;
    geometry->page_size = page_size;
    return 1;
}

wp_f32 wp_paged_geometry_get_page_size( const wp_paged_geometry *geometry )
{
    return geometry != NULL ? geometry->page_size : 0.0f;
}

wp_s32 wp_paged_geometry_set_bounds( wp_paged_geometry *geometry, wp_paged_geometry_bounds bounds )
{
    wp_f32 width;
    wp_f32 height;
    if( geometry == NULL || geometry->manager_count != 0U )
        return 0;
    wp_paged_normalize_bounds( &bounds );
    width = wp_paged_bounds_width( bounds );
    height = wp_paged_bounds_height( bounds );
    if( width <= 0.0f || height <= 0.0f || (wp_f32)fabs( (double)( width - height ) ) > 0.01f )
        return 0;
    geometry->bounds = bounds;
    return 1;
}

void wp_paged_geometry_set_infinite( wp_paged_geometry *geometry )
{
    if( geometry == NULL || geometry->manager_count != 0U )
        return;
    memset( &geometry->bounds, 0, sizeof( geometry->bounds ) );
}

wp_paged_geometry_bounds wp_paged_geometry_get_bounds( const wp_paged_geometry *geometry )
{
    wp_paged_geometry_bounds bounds;
    memset( &bounds, 0, sizeof( bounds ) );
    if( geometry != NULL )
        bounds = geometry->bounds;
    return bounds;
}

void wp_paged_geometry_set_camera( wp_paged_geometry *geometry, wp_camera *camera )
{
    wp_camera *temporary_camera;
    wp_vec3f temporary_position;
    if( geometry == NULL )
        return;
    if( camera != NULL && camera == geometry->last_camera )
    {
        temporary_camera = geometry->camera;
        geometry->camera = geometry->last_camera;
        geometry->last_camera = temporary_camera;
        temporary_position = geometry->old_camera_position;
        geometry->old_camera_position = geometry->last_old_camera_position;
        geometry->last_old_camera_position = temporary_position;
    }
    else
    {
        geometry->last_camera = geometry->camera;
        geometry->last_old_camera_position = geometry->old_camera_position;
        geometry->camera = camera;
        if( camera != NULL )
            geometry->old_camera_position = wp_camera_get_position( camera );
    }
}

wp_camera *wp_paged_geometry_get_camera( const wp_paged_geometry *geometry )
{
    return geometry != NULL ? geometry->camera : NULL;
}

void wp_paged_geometry_set_page_loader( wp_paged_geometry *geometry,
                                        const wp_page_loader_callbacks *callbacks, void *loader_data )
{
    if( geometry == NULL )
        return;
    memset( &geometry->loader_callbacks, 0, sizeof( geometry->loader_callbacks ) );
    if( callbacks != NULL )
        geometry->loader_callbacks = *callbacks;
    geometry->loader_data = loader_data;
}

wp_geometry_page_manager *wp_paged_geometry_add_detail_level(
    wp_paged_geometry *geometry, wp_f32 max_range, wp_f32 transition_length,
    const wp_geometry_page_callbacks *callbacks, void *factory_data, const void *detail_data,
    wp_u32 query_flag )
{
    wp_geometry_page_manager *manager;
    wp_geometry_page_manager **new_managers;
    wp_f32 minimum_range;
    wp_u32 new_capacity;

    if( geometry == NULL || callbacks == NULL || callbacks->create_page == NULL ||
        callbacks->destroy_page == NULL )
        return NULL;
    minimum_range = 0.0f;
    if( geometry->manager_count != 0U )
        minimum_range = geometry->managers[geometry->manager_count - 1U]->far_distance;
    if( max_range <= minimum_range )
        return NULL;
    manager = (wp_geometry_page_manager *)calloc( 1, sizeof( *manager ) );
    if( manager == NULL )
        return NULL;
    manager->geometry = geometry;
    manager->callbacks = *callbacks;
    manager->factory_data = factory_data;
    manager->detail_data = detail_data;
    manager->max_cache_interval = 200U;
    manager->inactive_page_life = 2000U;
    manager->near_distance = minimum_range;
    manager->near_distance_sq = minimum_range * minimum_range;
    manager->far_distance = max_range;
    manager->far_distance_sq = max_range * max_range;
    wp_paged_set_transition( manager, transition_length );
    if( !wp_paged_init_manager_grid( manager, query_flag ) )
    {
        wp_paged_destroy_manager( manager );
        return NULL;
    }
    if( geometry->manager_count == geometry->manager_capacity )
    {
        new_capacity = geometry->manager_capacity == 0U ? 4U : geometry->manager_capacity * 2U;
        new_managers = (wp_geometry_page_manager **)realloc(
            geometry->managers, (size_t)new_capacity * sizeof( wp_geometry_page_manager * ) );
        if( new_managers == NULL )
        {
            wp_paged_destroy_manager( manager );
            return NULL;
        }
        geometry->managers = new_managers;
        geometry->manager_capacity = new_capacity;
    }
    geometry->managers[geometry->manager_count++] = manager;
    return manager;
}

void wp_paged_geometry_remove_detail_levels( wp_paged_geometry *geometry )
{
    wp_u32 index;
    if( geometry == NULL )
        return;
    for( index = 0U; index < geometry->manager_count; ++index )
        wp_paged_destroy_manager( geometry->managers[index] );
    geometry->manager_count = 0U;
}

wp_u32 wp_paged_geometry_get_detail_level_count( const wp_paged_geometry *geometry )
{
    return geometry != NULL ? geometry->manager_count : 0U;
}

void wp_paged_geometry_update( wp_paged_geometry *geometry, wp_u32 delta_milliseconds )
{
    wp_vec3f camera_position;
    wp_vec3f camera_speed;
    wp_geometry_page_manager *previous;
    wp_s32 cache_enabled;
    wp_u32 index;

    if( geometry == NULL || geometry->camera == NULL )
        return;
    camera_position = wp_camera_get_position( geometry->camera );
    if( delta_milliseconds == 0U )
        camera_speed = wp_paged_zero_vec3();
    else
    {
        camera_speed.x =
            ( camera_position.x - geometry->old_camera_position.x ) / (wp_f32)delta_milliseconds;
        camera_speed.y =
            ( camera_position.y - geometry->old_camera_position.y ) / (wp_f32)delta_milliseconds;
        camera_speed.z =
            ( camera_position.z - geometry->old_camera_position.z ) / (wp_f32)delta_milliseconds;
    }
    geometry->old_camera_position = camera_position;
    if( geometry->loader_callbacks.frame_update != NULL )
        geometry->loader_callbacks.frame_update( geometry->loader_data );
    cache_enabled = 1;
    previous = NULL;
    for( index = 0U; index < geometry->manager_count; ++index )
    {
        wp_paged_update_manager( geometry->managers[index], delta_milliseconds, camera_position,
                                 camera_speed, &cache_enabled, previous );
        previous = geometry->managers[index];
    }
}

void wp_paged_geometry_reload( wp_paged_geometry *geometry )
{
    wp_u32 manager_index;
    wp_s32 page_index;
    wp_geometry_page_manager *manager;
    wp_geometry_page *page;
    if( geometry == NULL )
        return;
    for( manager_index = 0U; manager_index < geometry->manager_count; ++manager_index )
    {
        manager = geometry->managers[manager_index];
        for( page_index = 0; page_index < manager->grid_x * manager->grid_z; ++page_index )
        {
            page = manager->grid[page_index];
            if( page->loaded || page->needs_unload )
                wp_paged_unload_page( page );
            page->pending = 0;
        }
    }
}

static wp_s32 wp_paged_clamp_grid_index( wp_s32 value, wp_s32 size )
{
    if( value < 0 )
        return 0;
    if( value >= size )
        return size - 1;
    return value;
}

void wp_paged_geometry_reload_page( wp_paged_geometry *geometry, wp_vec3f point )
{
    wp_u32 index;
    wp_s32 x;
    wp_s32 z;
    wp_geometry_page_manager *manager;
    wp_geometry_page *page;
    if( geometry == NULL )
        return;
    for( index = 0U; index < geometry->manager_count; ++index )
    {
        manager = geometry->managers[index];
        x = wp_paged_floor_to_s32( ( point.x - manager->grid_bounds.left ) / geometry->page_size );
        z = wp_paged_floor_to_s32( ( point.z - manager->grid_bounds.top ) / geometry->page_size );
        if( x < 0 || z < 0 || x >= manager->grid_x || z >= manager->grid_z )
            continue;
        page = wp_paged_grid_page( manager, x, z );
        if( page->loaded || page->needs_unload )
            wp_paged_unload_page( page );
        page->pending = 0;
    }
}

static void wp_paged_reload_manager_area( wp_geometry_page_manager *manager,
                                          wp_paged_geometry_bounds area, wp_s32 use_radius,
                                          wp_vec3f center, wp_f32 radius_sq )
{
    wp_s32 x1;
    wp_s32 x2;
    wp_s32 z1;
    wp_s32 z2;
    wp_s32 x;
    wp_s32 z;
    wp_f32 dx;
    wp_f32 dz;
    wp_geometry_page *page;

    x1 = wp_paged_floor_to_s32( ( area.left - manager->grid_bounds.left ) /
                                manager->geometry->page_size );
    x2 = wp_paged_floor_to_s32( ( area.right - manager->grid_bounds.left ) /
                                manager->geometry->page_size );
    z1 = wp_paged_floor_to_s32( ( area.top - manager->grid_bounds.top ) / manager->geometry->page_size );
    z2 = wp_paged_floor_to_s32( ( area.bottom - manager->grid_bounds.top ) /
                                manager->geometry->page_size );
    x1 = wp_paged_clamp_grid_index( x1, manager->grid_x );
    x2 = wp_paged_clamp_grid_index( x2, manager->grid_x );
    z1 = wp_paged_clamp_grid_index( z1, manager->grid_z );
    z2 = wp_paged_clamp_grid_index( z2, manager->grid_z );
    for( z = z1; z <= z2; ++z )
    {
        for( x = x1; x <= x2; ++x )
        {
            page = wp_paged_grid_page( manager, x, z );
            if( use_radius )
            {
                dx = page->center.x - center.x;
                dz = page->center.z - center.z;
                if( dx * dx + dz * dz > radius_sq )
                    continue;
            }
            if( page->loaded || page->needs_unload )
                wp_paged_unload_page( page );
            page->pending = 0;
        }
    }
}

void wp_paged_geometry_reload_pages_radius( wp_paged_geometry *geometry, wp_vec3f center, wp_f32 radius )
{
    wp_paged_geometry_bounds area;
    wp_u32 index;
    if( geometry == NULL || radius < 0.0f )
        return;
    area.left = center.x - radius;
    area.right = center.x + radius;
    area.top = center.z - radius;
    area.bottom = center.z + radius;
    for( index = 0U; index < geometry->manager_count; ++index )
        wp_paged_reload_manager_area( geometry->managers[index], area, 1, center, radius * radius );
}

void wp_paged_geometry_reload_pages_bounds( wp_paged_geometry *geometry,
                                            wp_paged_geometry_bounds bounds )
{
    wp_u32 index;
    wp_vec3f center;
    if( geometry == NULL )
        return;
    wp_paged_normalize_bounds( &bounds );
    center = wp_paged_zero_vec3();
    for( index = 0U; index < geometry->manager_count; ++index )
        wp_paged_reload_manager_area( geometry->managers[index], bounds, 0, center, 0.0f );
}

void wp_paged_geometry_preload( wp_paged_geometry *geometry, wp_paged_geometry_bounds bounds )
{
    wp_u32 manager_index;
    wp_geometry_page_manager *manager;
    wp_geometry_page *page;
    wp_s32 x1;
    wp_s32 x2;
    wp_s32 z1;
    wp_s32 z2;
    wp_s32 x;
    wp_s32 z;
    if( geometry == NULL )
        return;
    wp_paged_normalize_bounds( &bounds );
    for( manager_index = 0U; manager_index < geometry->manager_count; ++manager_index )
    {
        manager = geometry->managers[manager_index];
        x1 = wp_paged_floor_to_s32( ( bounds.left - manager->far_distance - manager->grid_bounds.left ) /
                                    geometry->page_size );
        x2 = wp_paged_floor_to_s32(
            ( bounds.right + manager->far_distance - manager->grid_bounds.left ) / geometry->page_size );
        z1 = wp_paged_floor_to_s32( ( bounds.top - manager->far_distance - manager->grid_bounds.top ) /
                                    geometry->page_size );
        z2 = wp_paged_floor_to_s32(
            ( bounds.bottom + manager->far_distance - manager->grid_bounds.top ) / geometry->page_size );
        x1 = wp_paged_clamp_grid_index( x1, manager->grid_x );
        x2 = wp_paged_clamp_grid_index( x2, manager->grid_x );
        z1 = wp_paged_clamp_grid_index( z1, manager->grid_z );
        z2 = wp_paged_clamp_grid_index( z2, manager->grid_z );
        for( z = z1; z <= z2; ++z )
        {
            for( x = x1; x <= x2; ++x )
            {
                page = wp_paged_grid_page( manager, x, z );
                if( !page->loaded )
                    wp_paged_load_page( page );
                page->pending = 0;
                page->keep_loaded = 1;
            }
        }
    }
}

void wp_paged_geometry_reset_preloaded( wp_paged_geometry *geometry )
{
    wp_u32 manager_index;
    wp_s32 page_index;
    wp_geometry_page_manager *manager;
    if( geometry == NULL )
        return;
    for( manager_index = 0U; manager_index < geometry->manager_count; ++manager_index )
    {
        manager = geometry->managers[manager_index];
        for( page_index = 0; page_index < manager->grid_x * manager->grid_z; ++page_index )
            manager->grid[page_index]->keep_loaded = 0;
    }
}

void wp_paged_geometry_set_visible( wp_paged_geometry *geometry, wp_s32 visible )
{
    if( geometry != NULL )
        geometry->visible = visible ? 1 : 0;
}

wp_s32 wp_paged_geometry_is_visible( const wp_paged_geometry *geometry )
{
    return geometry != NULL ? geometry->visible : 0;
}

static wp_vec3f wp_paged_rotate_vector( wp_paged_geometry_quat q, wp_vec3f value )
{
    wp_vec3f result;
    wp_f32 tx;
    wp_f32 ty;
    wp_f32 tz;
    wp_f32 length_sq;
    length_sq = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if( length_sq <= WP_PAGED_EPSILON )
        return value;
    length_sq = 2.0f / length_sq;
    tx = length_sq * ( q.y * value.z - q.z * value.y );
    ty = length_sq * ( q.z * value.x - q.x * value.z );
    tz = length_sq * ( q.x * value.y - q.y * value.x );
    result.x = value.x + q.w * tx + q.y * tz - q.z * ty;
    result.y = value.y + q.w * ty + q.z * tx - q.x * tz;
    result.z = value.z + q.w * tz + q.x * ty - q.y * tx;
    return result;
}

static void wp_paged_expand_entity_bounds( wp_geometry_page *page, wp_vec3f position,
                                           wp_paged_geometry_quat rotation, wp_vec3f scale,
                                           const wp_paged_geometry_aabb *bounds )
{
    wp_s32 corner;
    wp_vec3f local;
    wp_vec3f transformed;
    wp_vec3f relative;
    if( bounds == NULL )
        return;
    relative.x = position.x - page->center.x;
    relative.y = position.y - page->center.y;
    relative.z = position.z - page->center.z;
    for( corner = 0; corner < 8; ++corner )
    {
        local.x = ( corner & 1 ) ? bounds->max.x : bounds->min.x;
        local.y = ( corner & 2 ) ? bounds->max.y : bounds->min.y;
        local.z = ( corner & 4 ) ? bounds->max.z : bounds->min.z;
        local.x *= scale.x;
        local.y *= scale.y;
        local.z *= scale.z;
        transformed = wp_paged_rotate_vector( rotation, local );
        transformed.x += relative.x;
        transformed.y += relative.y;
        transformed.z += relative.z;
        if( page->bounds_undefined )
        {
            page->true_bounds.min = transformed;
            page->true_bounds.max = transformed;
            page->bounds_undefined = 0;
        }
        else
        {
            if( transformed.x < page->true_bounds.min.x )
                page->true_bounds.min.x = transformed.x;
            if( transformed.y < page->true_bounds.min.y )
                page->true_bounds.min.y = transformed.y;
            if( transformed.z < page->true_bounds.min.z )
                page->true_bounds.min.z = transformed.z;
            if( transformed.x > page->true_bounds.max.x )
                page->true_bounds.max.x = transformed.x;
            if( transformed.y > page->true_bounds.max.y )
                page->true_bounds.max.y = transformed.y;
            if( transformed.z > page->true_bounds.max.z )
                page->true_bounds.max.z = transformed.z;
        }
    }
}

void wp_geometry_page_add_entity( wp_geometry_page *page, void *entity, wp_vec3f position,
                                  wp_paged_geometry_quat rotation, wp_vec3f scale,
                                  wp_paged_geometry_color color,
                                  const wp_paged_geometry_aabb *entity_local_bounds )
{
    if( page == NULL )
        return;
    if( page->manager->callbacks.add_entity != NULL )
        page->manager->callbacks.add_entity( page->page_data, entity, position, rotation, scale, color );
    wp_paged_expand_entity_bounds( page, position, rotation, scale, entity_local_bounds );
}

void *wp_geometry_page_get_data( wp_geometry_page *page )
{
    return page != NULL ? page->page_data : NULL;
}

wp_vec3f wp_geometry_page_get_center( const wp_geometry_page *page )
{
    return page != NULL ? page->center : wp_paged_zero_vec3();
}

wp_s32 wp_geometry_page_is_loaded( const wp_geometry_page *page )
{
    return page != NULL ? page->loaded : 0;
}

wp_s32 wp_geometry_page_is_visible( const wp_geometry_page *page )
{
    return page != NULL && page->loaded && page->visible;
}

wp_u32 wp_geometry_page_get_query_flag( const wp_geometry_page *page )
{
    return page != NULL ? page->query_flag : 0U;
}

wp_paged_geometry_aabb wp_geometry_page_get_bounding_box( const wp_geometry_page *page )
{
    wp_paged_geometry_aabb bounds;
    bounds.min = wp_paged_zero_vec3();
    bounds.max = wp_paged_zero_vec3();
    if( page != NULL && !page->bounds_undefined )
        bounds = page->true_bounds;
    return bounds;
}

wp_f32 wp_geometry_page_manager_get_near_range( const wp_geometry_page_manager *manager )
{
    return manager != NULL ? manager->near_distance : 0.0f;
}

wp_f32 wp_geometry_page_manager_get_far_range( const wp_geometry_page_manager *manager )
{
    return manager != NULL ? manager->far_distance : 0.0f;
}

wp_f32 wp_geometry_page_manager_get_transition( const wp_geometry_page_manager *manager )
{
    return manager != NULL ? manager->fade_length : 0.0f;
}

void wp_geometry_page_manager_set_cache_speed( wp_geometry_page_manager *manager,
                                               wp_u32 max_cache_interval, wp_u32 inactive_page_life )
{
    if( manager == NULL )
        return;
    manager->max_cache_interval = max_cache_interval;
    manager->inactive_page_life = inactive_page_life;
}

wp_u32 wp_geometry_page_manager_get_loaded_count( const wp_geometry_page_manager *manager )
{
    wp_s32 index;
    wp_u32 count;
    if( manager == NULL )
        return 0U;
    count = 0U;
    for( index = 0; index < manager->grid_x * manager->grid_z; ++index )
        if( manager->grid[index]->loaded )
            ++count;
    return count;
}

wp_u32 wp_geometry_page_manager_get_pending_count( const wp_geometry_page_manager *manager )
{
    wp_s32 index;
    wp_u32 count;
    if( manager == NULL )
        return 0U;
    count = 0U;
    for( index = 0; index < manager->grid_x * manager->grid_z; ++index )
        if( manager->grid[index]->pending )
            ++count;
    return count;
}
