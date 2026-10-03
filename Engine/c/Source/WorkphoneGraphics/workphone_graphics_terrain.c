/**
 * @file workphone_graphics_terrain.c
 * @brief C89 implementation of the portable parts of OgreTerrain.
 */

#include "workphone_graphics_terrain.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define WP_TERRAIN_EPSILON 1.0e-6f

struct wp_graphics_terrain
{
    wp_u16 size;
    wp_u16 max_batch_size;
    wp_u16 min_batch_size;
    wp_u16 lod_level_count;
    wp_u16 tree_depth;
    wp_f32 world_size;
    wp_f32 base;
    wp_f32 scale;
    wp_vec3f position;
    wp_graphics_terrain_alignment alignment;
    wp_f32 *heights;
    wp_graphics_terrain_rect dirty_rect;
    wp_s32 modified;
    wp_s32 height_data_modified;
    void *native;
};

static wp_s32 wp_terrain_is_power_of_two( wp_u32 value )
{
    return value != 0U && ( value & ( value - 1U ) ) == 0U;
}

static wp_s32 wp_terrain_size_is_valid( wp_u16 size )
{
    return size >= 3U && wp_terrain_is_power_of_two( (wp_u32)size - 1U );
}

static wp_u16 wp_terrain_log2( wp_u32 value )
{
    wp_u16 result;

    result = 0U;
    while( value > 1U )
    {
        value >>= 1;
        ++result;
    }
    return result;
}

static wp_f32 wp_terrain_clampf( wp_f32 value, wp_f32 minimum, wp_f32 maximum )
{
    if( value < minimum )
        return minimum;
    if( value > maximum )
        return maximum;
    return value;
}

static wp_u32 wp_terrain_min_u32( wp_u32 a, wp_u32 b )
{
    return a < b ? a : b;
}

static wp_vec3f wp_terrain_vec3( wp_f32 x, wp_f32 y, wp_f32 z )
{
    wp_vec3f result;

    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

static wp_vec3f wp_terrain_sub( wp_vec3f a, wp_vec3f b )
{
    return wp_terrain_vec3( a.x - b.x, a.y - b.y, a.z - b.z );
}

static wp_vec3f wp_terrain_add( wp_vec3f a, wp_vec3f b )
{
    return wp_terrain_vec3( a.x + b.x, a.y + b.y, a.z + b.z );
}

static wp_vec3f wp_terrain_scale_vec( wp_vec3f value, wp_f32 scale )
{
    return wp_terrain_vec3( value.x * scale, value.y * scale, value.z * scale );
}

static wp_f32 wp_terrain_dot( wp_vec3f a, wp_vec3f b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static wp_vec3f wp_terrain_cross( wp_vec3f a, wp_vec3f b )
{
    return wp_terrain_vec3( a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x );
}

static wp_vec3f wp_terrain_normalize( wp_vec3f value )
{
    wp_f32 length;

    length = (wp_f32)sqrt( (double)wp_terrain_dot( value, value ) );
    if( length <= WP_TERRAIN_EPSILON )
        return wp_terrain_vec3( 0.0f, 0.0f, 0.0f );
    return wp_terrain_scale_vec( value, 1.0f / length );
}

static void wp_terrain_update_scale( wp_graphics_terrain *terrain )
{
    terrain->base = -terrain->world_size * 0.5f;
    terrain->scale = terrain->world_size / (wp_f32)( terrain->size - 1U );
}

static void wp_terrain_determine_lod_levels( wp_graphics_terrain *terrain )
{
    wp_u32 leaf_count;

    terrain->lod_level_count = (wp_u16)( wp_terrain_log2( (wp_u32)terrain->size - 1U ) -
                                         wp_terrain_log2( (wp_u32)terrain->min_batch_size - 1U ) + 1U );
    leaf_count = ( (wp_u32)terrain->size - 1U ) / ( (wp_u32)terrain->max_batch_size - 1U );
    terrain->tree_depth = (wp_u16)( wp_terrain_log2( leaf_count ) + 1U );
}

static void wp_terrain_merge_dirty_rect( wp_graphics_terrain *terrain, wp_u32 left, wp_u32 top,
                                         wp_u32 right, wp_u32 bottom )
{
    wp_graphics_terrain_rect *rect;

    rect = &terrain->dirty_rect;
    if( rect->right <= rect->left || rect->bottom <= rect->top )
    {
        rect->left = left;
        rect->top = top;
        rect->right = right;
        rect->bottom = bottom;
    }
    else
    {
        if( left < rect->left )
            rect->left = left;
        if( top < rect->top )
            rect->top = top;
        if( right > rect->right )
            rect->right = right;
        if( bottom > rect->bottom )
            rect->bottom = bottom;
    }
    terrain->modified = 1;
    terrain->height_data_modified = 1;
}

wp_graphics_terrain *wp_graphics_terrain_create( wp_u16 size, wp_f32 world_size )
{
    wp_graphics_terrain *terrain;
    wp_u32 count;

    if( !wp_terrain_size_is_valid( size ) || world_size <= 0.0f )
        return NULL;
    terrain = (wp_graphics_terrain *)malloc( sizeof( *terrain ) );
    if( terrain == NULL )
        return NULL;
    memset( terrain, 0, sizeof( *terrain ) );
    count = (wp_u32)size * (wp_u32)size;
    terrain->heights = (wp_f32 *)calloc( count, sizeof( wp_f32 ) );
    if( terrain->heights == NULL )
    {
        free( terrain );
        return NULL;
    }
    terrain->size = size;
    terrain->world_size = world_size;
    terrain->alignment = WP_GRAPHICS_TERRAIN_ALIGN_X_Z;
    terrain->max_batch_size = size < 65U ? size : 65U;
    terrain->min_batch_size = size < 17U ? size : 17U;
    wp_terrain_update_scale( terrain );
    wp_terrain_determine_lod_levels( terrain );
    return terrain;
}

void wp_graphics_terrain_destroy( wp_graphics_terrain *terrain )
{
    if( terrain == NULL )
        return;
    free( terrain->heights );
    free( terrain );
}

wp_s32 wp_graphics_terrain_set_size( wp_graphics_terrain *terrain, wp_u16 size )
{
    wp_f32 *new_heights;
    wp_u16 old_size;
    wp_u32 x;
    wp_u32 y;
    wp_f32 source_x;
    wp_f32 source_y;
    wp_u32 x0;
    wp_u32 y0;
    wp_u32 x1;
    wp_u32 y1;
    wp_f32 fx;
    wp_f32 fy;
    wp_f32 h0;
    wp_f32 h1;

    if( terrain == NULL || !wp_terrain_size_is_valid( size ) )
        return 0;
    if( size == terrain->size )
        return 1;
    new_heights = (wp_f32 *)malloc( (wp_u32)size * (wp_u32)size * sizeof( wp_f32 ) );
    if( new_heights == NULL )
        return 0;
    old_size = terrain->size;
    for( y = 0U; y < (wp_u32)size; ++y )
    {
        source_y = (wp_f32)y * (wp_f32)( old_size - 1U ) / (wp_f32)( size - 1U );
        y0 = (wp_u32)source_y;
        y1 = wp_terrain_min_u32( y0 + 1U, (wp_u32)old_size - 1U );
        fy = source_y - (wp_f32)y0;
        for( x = 0U; x < (wp_u32)size; ++x )
        {
            source_x = (wp_f32)x * (wp_f32)( old_size - 1U ) / (wp_f32)( size - 1U );
            x0 = (wp_u32)source_x;
            x1 = wp_terrain_min_u32( x0 + 1U, (wp_u32)old_size - 1U );
            fx = source_x - (wp_f32)x0;
            h0 = terrain->heights[y0 * old_size + x0] * ( 1.0f - fx ) +
                 terrain->heights[y0 * old_size + x1] * fx;
            h1 = terrain->heights[y1 * old_size + x0] * ( 1.0f - fx ) +
                 terrain->heights[y1 * old_size + x1] * fx;
            new_heights[y * size + x] = h0 * ( 1.0f - fy ) + h1 * fy;
        }
    }
    free( terrain->heights );
    terrain->heights = new_heights;
    terrain->size = size;
    if( terrain->max_batch_size > size )
        terrain->max_batch_size = size;
    if( terrain->min_batch_size > terrain->max_batch_size )
        terrain->min_batch_size = terrain->max_batch_size;
    wp_terrain_update_scale( terrain );
    wp_terrain_determine_lod_levels( terrain );
    wp_graphics_terrain_dirty( terrain );
    return 1;
}

wp_u16 wp_graphics_terrain_get_size( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->size : 0U;
}

wp_s32 wp_graphics_terrain_set_height_data( wp_graphics_terrain *terrain, const wp_f32 *heights,
                                            wp_u32 count )
{
    wp_u32 required;

    if( terrain == NULL || heights == NULL )
        return 0;
    required = (wp_u32)terrain->size * (wp_u32)terrain->size;
    if( count < required )
        return 0;
    memcpy( terrain->heights, heights, required * sizeof( wp_f32 ) );
    wp_graphics_terrain_dirty( terrain );
    return 1;
}

wp_f32 *wp_graphics_terrain_get_height_data( wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->heights : NULL;
}

const wp_f32 *wp_graphics_terrain_get_height_data_const( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->heights : NULL;
}

wp_f32 wp_graphics_terrain_get_height_at_point( const wp_graphics_terrain *terrain, wp_u32 x, wp_u32 y )
{
    if( terrain == NULL || terrain->heights == NULL )
        return 0.0f;
    x = wp_terrain_min_u32( x, (wp_u32)terrain->size - 1U );
    y = wp_terrain_min_u32( y, (wp_u32)terrain->size - 1U );
    return terrain->heights[y * terrain->size + x];
}

void wp_graphics_terrain_set_height_at_point( wp_graphics_terrain *terrain, wp_u32 x, wp_u32 y,
                                              wp_f32 height )
{
    if( terrain == NULL || terrain->heights == NULL )
        return;
    x = wp_terrain_min_u32( x, (wp_u32)terrain->size - 1U );
    y = wp_terrain_min_u32( y, (wp_u32)terrain->size - 1U );
    terrain->heights[y * terrain->size + x] = height;
    wp_terrain_merge_dirty_rect( terrain, x, y, x + 1U, y + 1U );
}

wp_f32 wp_graphics_terrain_get_height_at_terrain_position( const wp_graphics_terrain *terrain, wp_f32 x,
                                                           wp_f32 y )
{
    wp_f32 factor;
    wp_f32 grid_x;
    wp_f32 grid_y;
    wp_u32 start_x;
    wp_u32 start_y;
    wp_u32 end_x;
    wp_u32 end_y;
    wp_f32 x_param;
    wp_f32 y_param;
    wp_f32 h0;
    wp_f32 h1;
    wp_f32 h2;
    wp_f32 h3;

    if( terrain == NULL )
        return 0.0f;
    x = wp_terrain_clampf( x, 0.0f, 1.0f );
    y = wp_terrain_clampf( y, 0.0f, 1.0f );
    factor = (wp_f32)( terrain->size - 1U );
    grid_x = x * factor;
    grid_y = y * factor;
    start_x = (wp_u32)grid_x;
    start_y = (wp_u32)grid_y;
    if( start_x >= (wp_u32)terrain->size - 1U )
        start_x = (wp_u32)terrain->size - 2U;
    if( start_y >= (wp_u32)terrain->size - 1U )
        start_y = (wp_u32)terrain->size - 2U;
    end_x = start_x + 1U;
    end_y = start_y + 1U;
    x_param = grid_x - (wp_f32)start_x;
    y_param = grid_y - (wp_f32)start_y;
    h0 = wp_graphics_terrain_get_height_at_point( terrain, start_x, start_y );
    h1 = wp_graphics_terrain_get_height_at_point( terrain, end_x, start_y );
    h2 = wp_graphics_terrain_get_height_at_point( terrain, end_x, end_y );
    h3 = wp_graphics_terrain_get_height_at_point( terrain, start_x, end_y );

    if( ( start_y & 1U ) != 0U )
    {
        if( 1.0f - y_param > x_param )
            return h0 + x_param * ( h1 - h0 ) + y_param * ( h3 - h0 );
        return ( 1.0f - y_param ) * h1 + ( x_param + y_param - 1.0f ) * h2 + ( 1.0f - x_param ) * h3;
    }
    if( y_param > x_param )
        return h0 + x_param * ( h2 - h3 ) + y_param * ( h3 - h0 );
    return h0 + x_param * ( h1 - h0 ) + y_param * ( h2 - h1 );
}

wp_f32 wp_graphics_terrain_get_height_at_world_position( const wp_graphics_terrain *terrain,
                                                         wp_vec3f world_position )
{
    wp_vec3f terrain_position;

    terrain_position = wp_graphics_terrain_world_to_terrain( terrain, world_position );
    return wp_graphics_terrain_get_height_at_terrain_position( terrain, terrain_position.x,
                                                               terrain_position.y );
}

wp_vec3f wp_graphics_terrain_get_normal_at_terrain_position( const wp_graphics_terrain *terrain,
                                                             wp_f32 x, wp_f32 y )
{
    wp_f32 step;
    wp_f32 left;
    wp_f32 right;
    wp_f32 down;
    wp_f32 up;
    wp_vec3f terrain_normal;
    wp_vec3f world_normal;

    if( terrain == NULL )
        return wp_terrain_vec3( 0.0f, 1.0f, 0.0f );
    step = 1.0f / (wp_f32)( terrain->size - 1U );
    left = wp_graphics_terrain_get_height_at_terrain_position( terrain, x - step, y );
    right = wp_graphics_terrain_get_height_at_terrain_position( terrain, x + step, y );
    down = wp_graphics_terrain_get_height_at_terrain_position( terrain, x, y - step );
    up = wp_graphics_terrain_get_height_at_terrain_position( terrain, x, y + step );
    terrain_normal = wp_terrain_vec3( left - right, down - up, 2.0f * terrain->scale );
    world_normal = wp_graphics_terrain_terrain_to_world_axes( terrain->alignment, terrain_normal );
    return wp_terrain_normalize( world_normal );
}

void wp_graphics_terrain_set_alignment( wp_graphics_terrain *terrain,
                                        wp_graphics_terrain_alignment alignment )
{
    if( terrain == NULL )
        return;
    if( alignment < WP_GRAPHICS_TERRAIN_ALIGN_X_Z || alignment > WP_GRAPHICS_TERRAIN_ALIGN_X_Y )
        return;
    if( terrain->alignment != alignment )
    {
        terrain->alignment = alignment;
        terrain->modified = 1;
    }
}

wp_graphics_terrain_alignment wp_graphics_terrain_get_alignment( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->alignment : WP_GRAPHICS_TERRAIN_ALIGN_X_Z;
}

void wp_graphics_terrain_set_position( wp_graphics_terrain *terrain, wp_vec3f position )
{
    if( terrain != NULL )
    {
        terrain->position = position;
        terrain->modified = 1;
    }
}

wp_vec3f wp_graphics_terrain_get_position( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->position : wp_terrain_vec3( 0.0f, 0.0f, 0.0f );
}

void wp_graphics_terrain_set_world_size( wp_graphics_terrain *terrain, wp_f32 world_size )
{
    if( terrain == NULL || world_size <= 0.0f )
        return;
    if( terrain->world_size != world_size )
    {
        terrain->world_size = world_size;
        wp_terrain_update_scale( terrain );
        wp_graphics_terrain_dirty( terrain );
    }
}

wp_f32 wp_graphics_terrain_get_world_size( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->world_size : 0.0f;
}

wp_vec3f wp_graphics_terrain_world_to_terrain_axes( wp_graphics_terrain_alignment alignment,
                                                    wp_vec3f vector )
{
    wp_vec3f result;

    result = vector;
    if( alignment == WP_GRAPHICS_TERRAIN_ALIGN_X_Z )
        result = wp_terrain_vec3( vector.x, -vector.z, vector.y );
    else if( alignment == WP_GRAPHICS_TERRAIN_ALIGN_Y_Z )
        result = wp_terrain_vec3( -vector.z, vector.y, vector.x );
    return result;
}

wp_vec3f wp_graphics_terrain_terrain_to_world_axes( wp_graphics_terrain_alignment alignment,
                                                    wp_vec3f vector )
{
    wp_vec3f result;

    result = vector;
    if( alignment == WP_GRAPHICS_TERRAIN_ALIGN_X_Z )
        result = wp_terrain_vec3( vector.x, vector.z, -vector.y );
    else if( alignment == WP_GRAPHICS_TERRAIN_ALIGN_Y_Z )
        result = wp_terrain_vec3( vector.z, vector.y, -vector.x );
    return result;
}

wp_vec3f wp_graphics_terrain_terrain_to_world( const wp_graphics_terrain *terrain,
                                               wp_vec3f terrain_position )
{
    wp_vec3f scaled;
    wp_vec3f local;

    if( terrain == NULL )
        return wp_terrain_vec3( 0.0f, 0.0f, 0.0f );
    scaled.x = terrain_position.x * terrain->world_size + terrain->base;
    scaled.y = terrain_position.y * terrain->world_size + terrain->base;
    scaled.z = terrain_position.z;
    local = wp_graphics_terrain_terrain_to_world_axes( terrain->alignment, scaled );
    return wp_terrain_add( local, terrain->position );
}

wp_vec3f wp_graphics_terrain_world_to_terrain( const wp_graphics_terrain *terrain,
                                               wp_vec3f world_position )
{
    wp_vec3f local;
    wp_vec3f axes;

    if( terrain == NULL || terrain->world_size <= 0.0f )
        return wp_terrain_vec3( 0.0f, 0.0f, 0.0f );
    local = wp_terrain_sub( world_position, terrain->position );
    axes = wp_graphics_terrain_world_to_terrain_axes( terrain->alignment, local );
    axes.x = ( axes.x - terrain->base ) / terrain->world_size;
    axes.y = ( axes.y - terrain->base ) / terrain->world_size;
    return axes;
}

wp_vec3f wp_graphics_terrain_get_point( const wp_graphics_terrain *terrain, wp_u32 x, wp_u32 y )
{
    wp_vec3f terrain_position;

    if( terrain == NULL )
        return wp_terrain_vec3( 0.0f, 0.0f, 0.0f );
    x = wp_terrain_min_u32( x, (wp_u32)terrain->size - 1U );
    y = wp_terrain_min_u32( y, (wp_u32)terrain->size - 1U );
    terrain_position.x = (wp_f32)x / (wp_f32)( terrain->size - 1U );
    terrain_position.y = (wp_f32)y / (wp_f32)( terrain->size - 1U );
    terrain_position.z = terrain->heights[y * terrain->size + x];
    return wp_graphics_terrain_terrain_to_world( terrain, terrain_position );
}

wp_f32 wp_graphics_terrain_get_min_height( const wp_graphics_terrain *terrain )
{
    wp_u32 count;
    wp_u32 i;
    wp_f32 result;

    if( terrain == NULL || terrain->heights == NULL )
        return 0.0f;
    count = (wp_u32)terrain->size * (wp_u32)terrain->size;
    result = terrain->heights[0];
    for( i = 1U; i < count; ++i )
        if( terrain->heights[i] < result )
            result = terrain->heights[i];
    return result;
}

wp_f32 wp_graphics_terrain_get_max_height( const wp_graphics_terrain *terrain )
{
    wp_u32 count;
    wp_u32 i;
    wp_f32 result;

    if( terrain == NULL || terrain->heights == NULL )
        return 0.0f;
    count = (wp_u32)terrain->size * (wp_u32)terrain->size;
    result = terrain->heights[0];
    for( i = 1U; i < count; ++i )
        if( terrain->heights[i] > result )
            result = terrain->heights[i];
    return result;
}

wp_f32 wp_graphics_terrain_get_bounding_radius( const wp_graphics_terrain *terrain )
{
    wp_f32 half;
    wp_f32 height;

    if( terrain == NULL )
        return 0.0f;
    half = terrain->world_size * 0.5f;
    height = wp_graphics_terrain_get_max_height( terrain );
    if( -wp_graphics_terrain_get_min_height( terrain ) > height )
        height = -wp_graphics_terrain_get_min_height( terrain );
    return (wp_f32)sqrt( (double)( half * half * 2.0f + height * height ) );
}

wp_s32 wp_graphics_terrain_is_modified( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->modified : 0;
}

wp_s32 wp_graphics_terrain_is_height_data_modified( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->height_data_modified : 0;
}

void wp_graphics_terrain_clear_modified( wp_graphics_terrain *terrain )
{
    if( terrain == NULL )
        return;
    terrain->modified = 0;
    terrain->height_data_modified = 0;
    memset( &terrain->dirty_rect, 0, sizeof( terrain->dirty_rect ) );
}

void wp_graphics_terrain_dirty( wp_graphics_terrain *terrain )
{
    if( terrain != NULL )
        wp_terrain_merge_dirty_rect( terrain, 0U, 0U, terrain->size, terrain->size );
}

wp_graphics_terrain_rect wp_graphics_terrain_get_dirty_rect( const wp_graphics_terrain *terrain )
{
    wp_graphics_terrain_rect empty;

    memset( &empty, 0, sizeof( empty ) );
    return terrain != NULL ? terrain->dirty_rect : empty;
}

void *wp_graphics_terrain_get_native( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->native : NULL;
}

void wp_graphics_terrain_set_native( wp_graphics_terrain *terrain, void *native )
{
    if( terrain != NULL )
        terrain->native = native;
}

wp_u16 wp_graphics_terrain_get_lod_level_count( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->lod_level_count : 0U;
}

wp_u16 wp_graphics_terrain_get_tree_depth( const wp_graphics_terrain *terrain )
{
    return terrain != NULL ? terrain->tree_depth : 0U;
}

wp_s32 wp_graphics_terrain_set_batch_sizes( wp_graphics_terrain *terrain, wp_u16 max_batch_size,
                                            wp_u16 min_batch_size )
{
    wp_u32 terrain_quads;
    wp_u32 max_quads;
    wp_u32 min_quads;

    if( terrain == NULL || !wp_terrain_size_is_valid( max_batch_size ) ||
        !wp_terrain_size_is_valid( min_batch_size ) || max_batch_size > terrain->size ||
        max_batch_size < min_batch_size || max_batch_size > WP_GRAPHICS_TERRAIN_MAX_BATCH_SIZE )
        return 0;
    terrain_quads = (wp_u32)terrain->size - 1U;
    max_quads = (wp_u32)max_batch_size - 1U;
    min_quads = (wp_u32)min_batch_size - 1U;
    if( terrain_quads % max_quads != 0U || max_quads % min_quads != 0U )
        return 0;
    terrain->max_batch_size = max_batch_size;
    terrain->min_batch_size = min_batch_size;
    wp_terrain_determine_lod_levels( terrain );
    terrain->modified = 1;
    return 1;
}

wp_u32 wp_graphics_terrain_get_index_count_for_batch_size( wp_u16 batch_size )
{
    wp_u32 main_indices_per_row;
    wp_u32 row_count;
    wp_u32 main_index_count;
    wp_u32 skirt_index_count;

    if( batch_size < 2U )
        return 0U;
    main_indices_per_row = (wp_u32)batch_size * 2U + 1U;
    row_count = (wp_u32)batch_size - 1U;
    main_index_count = main_indices_per_row * row_count;
    skirt_index_count = ( (wp_u32)batch_size - 1U ) * 2U * 4U + 2U;
    return main_index_count + skirt_index_count;
}

wp_u16 wp_graphics_terrain_calc_skirt_vertex_index( wp_u16 main_index, wp_u16 vertex_data_size,
                                                    wp_s32 is_column, wp_u16 skirt_row_column_count,
                                                    wp_u16 skirt_row_column_skip )
{
    wp_u32 row;
    wp_u32 column;
    wp_u32 base;
    wp_u32 skirt_number;
    wp_u32 result;

    if( vertex_data_size == 0U || skirt_row_column_skip == 0U )
        return 0U;
    row = main_index / vertex_data_size;
    column = main_index % vertex_data_size;
    base = (wp_u32)vertex_data_size * vertex_data_size;
    if( is_column )
    {
        skirt_number = column / skirt_row_column_skip;
        result = base + (wp_u32)skirt_row_column_count * vertex_data_size +
                 (wp_u32)vertex_data_size * skirt_number + row;
    }
    else
    {
        skirt_number = row / skirt_row_column_skip;
        result = base + (wp_u32)vertex_data_size * skirt_number + column;
    }
    return (wp_u16)result;
}

wp_s32 wp_graphics_terrain_populate_index_buffer( wp_u16 *indices, wp_u32 capacity, wp_u16 batch_size,
                                                  wp_u16 vertex_data_size, wp_u16 vertex_increment,
                                                  wp_u16 x_offset, wp_u16 y_offset,
                                                  wp_u16 skirt_row_column_count,
                                                  wp_u16 skirt_row_column_skip )
{
    wp_u32 required;
    wp_u16 *output;
    wp_u32 row_size;
    wp_u32 row_count;
    wp_s32 current_vertex;
    wp_s32 right_to_left;
    wp_u32 row;
    wp_u32 column;
    wp_u32 side;
    wp_s32 edge_increment;
    wp_s32 skirt_increment;
    wp_s32 skirt_index;

    required = wp_graphics_terrain_get_index_count_for_batch_size( batch_size );
    if( indices == NULL || capacity < required || batch_size < 2U || vertex_data_size == 0U ||
        vertex_increment == 0U || skirt_row_column_skip == 0U )
        return 0;
    output = indices;
    row_size = (wp_u32)vertex_data_size * vertex_increment;
    row_count = (wp_u32)batch_size - 1U;
    current_vertex = (wp_s32)( ( (wp_u32)batch_size - 1U ) * vertex_increment +
                               (wp_u32)y_offset * vertex_data_size + x_offset );
    right_to_left = 1;
    for( row = 0U; row < row_count; ++row )
    {
        for( column = 0U; column < batch_size; ++column )
        {
            *output++ = (wp_u16)current_vertex;
            *output++ = (wp_u16)( current_vertex + (wp_s32)row_size );
            if( column + 1U < batch_size )
                current_vertex += right_to_left ? -(wp_s32)vertex_increment : (wp_s32)vertex_increment;
        }
        right_to_left = !right_to_left;
        current_vertex += (wp_s32)row_size;
        *output++ = (wp_u16)current_vertex;
    }

    for( side = 0U; side < 4U; ++side )
    {
        edge_increment = 0;
        skirt_increment = 0;
        if( side == 0U )
        {
            edge_increment = -(wp_s32)vertex_increment;
            skirt_increment = -(wp_s32)vertex_increment;
        }
        else if( side == 1U )
        {
            edge_increment = -(wp_s32)row_size;
            skirt_increment = -(wp_s32)vertex_increment;
        }
        else if( side == 2U )
        {
            edge_increment = (wp_s32)vertex_increment;
            skirt_increment = (wp_s32)vertex_increment;
        }
        else
        {
            edge_increment = (wp_s32)row_size;
            skirt_increment = (wp_s32)vertex_increment;
        }
        skirt_index = (wp_s32)wp_graphics_terrain_calc_skirt_vertex_index(
            (wp_u16)current_vertex, vertex_data_size, ( side % 2U ) != 0U, skirt_row_column_count,
            skirt_row_column_skip );
        for( column = 0U; column < (wp_u32)batch_size - 1U; ++column )
        {
            *output++ = (wp_u16)current_vertex;
            *output++ = (wp_u16)skirt_index;
            current_vertex += edge_increment;
            skirt_index += skirt_increment;
        }
        if( side == 3U )
        {
            *output++ = (wp_u16)current_vertex;
            *output++ = (wp_u16)skirt_index;
        }
    }
    return (wp_s32)( output - indices );
}

static wp_s32 wp_terrain_ray_triangle( wp_vec3f origin, wp_vec3f direction, wp_vec3f a, wp_vec3f b,
                                       wp_vec3f c, wp_f32 *distance )
{
    wp_vec3f edge1;
    wp_vec3f edge2;
    wp_vec3f p;
    wp_vec3f t;
    wp_vec3f q;
    wp_f32 determinant;
    wp_f32 inverse;
    wp_f32 u;
    wp_f32 v;
    wp_f32 hit_distance;

    edge1 = wp_terrain_sub( b, a );
    edge2 = wp_terrain_sub( c, a );
    p = wp_terrain_cross( direction, edge2 );
    determinant = wp_terrain_dot( edge1, p );
    if( determinant > -WP_TERRAIN_EPSILON && determinant < WP_TERRAIN_EPSILON )
        return 0;
    inverse = 1.0f / determinant;
    t = wp_terrain_sub( origin, a );
    u = wp_terrain_dot( t, p ) * inverse;
    if( u < 0.0f || u > 1.0f )
        return 0;
    q = wp_terrain_cross( t, edge1 );
    v = wp_terrain_dot( direction, q ) * inverse;
    if( v < 0.0f || u + v > 1.0f )
        return 0;
    hit_distance = wp_terrain_dot( edge2, q ) * inverse;
    if( hit_distance < 0.0f )
        return 0;
    *distance = hit_distance;
    return 1;
}

wp_graphics_terrain_ray_result wp_graphics_terrain_ray_intersects( const wp_graphics_terrain *terrain,
                                                                   wp_vec3f origin, wp_vec3f direction,
                                                                   wp_f32 distance_limit )
{
    wp_graphics_terrain_ray_result result;
    wp_u32 x;
    wp_u32 y;
    wp_vec3f v0;
    wp_vec3f v1;
    wp_vec3f v2;
    wp_vec3f v3;
    wp_f32 distance;
    wp_f32 direction_length;

    memset( &result, 0, sizeof( result ) );
    if( terrain == NULL )
        return result;
    direction_length = (wp_f32)sqrt( (double)wp_terrain_dot( direction, direction ) );
    if( direction_length <= WP_TERRAIN_EPSILON )
        return result;
    direction = wp_terrain_scale_vec( direction, 1.0f / direction_length );
    result.distance = FLT_MAX;
    for( y = 0U; y < (wp_u32)terrain->size - 1U; ++y )
    {
        for( x = 0U; x < (wp_u32)terrain->size - 1U; ++x )
        {
            v0 = wp_graphics_terrain_get_point( terrain, x, y );
            v1 = wp_graphics_terrain_get_point( terrain, x + 1U, y );
            v2 = wp_graphics_terrain_get_point( terrain, x + 1U, y + 1U );
            v3 = wp_graphics_terrain_get_point( terrain, x, y + 1U );
            if( ( y & 1U ) != 0U )
            {
                if( wp_terrain_ray_triangle( origin, direction, v0, v1, v3, &distance ) &&
                    distance < result.distance )
                    result.distance = distance;
                if( wp_terrain_ray_triangle( origin, direction, v1, v2, v3, &distance ) &&
                    distance < result.distance )
                    result.distance = distance;
            }
            else
            {
                if( wp_terrain_ray_triangle( origin, direction, v0, v2, v3, &distance ) &&
                    distance < result.distance )
                    result.distance = distance;
                if( wp_terrain_ray_triangle( origin, direction, v0, v1, v2, &distance ) &&
                    distance < result.distance )
                    result.distance = distance;
            }
        }
    }
    if( result.distance != FLT_MAX && ( distance_limit <= 0.0f || result.distance <= distance_limit ) )
    {
        result.hit = 1;
        result.position = wp_terrain_add( origin, wp_terrain_scale_vec( direction, result.distance ) );
    }
    else
    {
        result.distance = 0.0f;
    }
    return result;
}
