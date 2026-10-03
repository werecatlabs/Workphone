#include "workphone_volume.h"
#include "workphone_math.h"
#include "workphone_memory.h"

void wp_volume_atlas_init( struct wp_volume_atlas *atlas, wp_handle texture, wp_u32 texture_width,
                           wp_u32 texture_height, wp_u32 slice_width, wp_u32 slice_height, wp_u32 depth,
                           wp_u32 columns, wp_u32 rows, wp_u32 padding_x, wp_u32 padding_y )
{
    WORKPHONE_ASSERT( atlas );
    if( !atlas )
        return;

    wp_zero( atlas, sizeof( *atlas ) );
    atlas->texture = texture;
    atlas->texture_width = texture_width;
    atlas->texture_height = texture_height;
    atlas->slice_width = slice_width;
    atlas->slice_height = slice_height;
    atlas->depth = depth;
    atlas->columns = columns;
    atlas->rows = rows;
    atlas->padding_x = padding_x;
    atlas->padding_y = padding_y;

    /* Strides include the gutter between cells. */
    atlas->stride_x = slice_width + padding_x;
    atlas->stride_y = slice_height + padding_y;
}

wp_bool wp_volume_atlas_is_valid( const struct wp_volume_atlas *atlas )
{
    wp_u32 required_width;
    wp_u32 required_height;
    wp_u32 capacity;

    if( !atlas || !atlas->texture_width || !atlas->texture_height || !atlas->slice_width ||
        !atlas->slice_height || !atlas->depth || !atlas->columns || !atlas->rows || !atlas->stride_x ||
        !atlas->stride_y )
        return wp_false;

    /* Reject arithmetic overflow before calculating the occupied extent. */
    if( atlas->slice_width > WORKPHONE_UINT_MAX - atlas->padding_x ||
        atlas->slice_height > WORKPHONE_UINT_MAX - atlas->padding_y ||
        atlas->columns > WORKPHONE_UINT_MAX / atlas->stride_x ||
        atlas->rows > WORKPHONE_UINT_MAX / atlas->stride_y )
        return wp_false;

    required_width = atlas->stride_x * atlas->columns - atlas->padding_x;
    required_height = atlas->stride_y * atlas->rows - atlas->padding_y;
    capacity = ( atlas->columns > WORKPHONE_UINT_MAX / atlas->rows ) ? WORKPHONE_UINT_MAX
                                                                     : atlas->columns * atlas->rows;
    return required_width <= atlas->texture_width && required_height <= atlas->texture_height &&
           atlas->depth <= capacity;
}

wp_bool wp_volume_atlas_slice( const struct wp_volume_atlas *atlas, wp_u32 index,
                               struct wp_volume_slice *out )
{
    wp_u32 column, row, x, y;
    wp_f32 inset_x, inset_y;

    WORKPHONE_ASSERT( out );
    if( !out )
        return wp_false;
    wp_zero( out, sizeof( *out ) );
    if( !wp_volume_atlas_is_valid( atlas ) || index >= atlas->depth )
        return wp_false;

    column = index % atlas->columns;
    row = index / atlas->columns;
    x = column * atlas->stride_x;
    y = row * atlas->stride_y;
    out->pixels =
        wp_make_rect( (wp_f32)x, (wp_f32)y, (wp_f32)atlas->slice_width, (wp_f32)atlas->slice_height );

    /* Half a texel keeps bilinear filtering inside this cell, including when
     * the atlas is used with linear filtering for ray marching. */
    inset_x = 0.5f / (wp_f32)atlas->texture_width;
    inset_y = 0.5f / (wp_f32)atlas->texture_height;
    out->uv =
        wp_vec4f_make( (wp_f32)x / (wp_f32)atlas->texture_width + inset_x,
                       (wp_f32)y / (wp_f32)atlas->texture_height + inset_y,
                       (wp_f32)( x + atlas->slice_width ) / (wp_f32)atlas->texture_width - inset_x,
                       (wp_f32)( y + atlas->slice_height ) / (wp_f32)atlas->texture_height - inset_y );
    return wp_true;
}

wp_bool wp_volume_atlas_frame( const struct wp_volume_atlas *atlas, wp_u32 index, struct wp_image *out )
{
    struct wp_volume_slice slice;

    WORKPHONE_ASSERT( out );
    if( !out )
        return wp_false;
    wp_zero( out, sizeof( *out ) );
    if( !wp_volume_atlas_slice( atlas, index, &slice ) || atlas->texture_width > 65535u ||
        atlas->texture_height > 65535u || atlas->slice_width > 65535u || atlas->slice_height > 65535u )
        return wp_false;

    *out = wp_subimage_handle( atlas->texture, (wp_u16)atlas->texture_width,
                               (wp_u16)atlas->texture_height, slice.pixels );
    return wp_true;
}

wp_bool wp_volume_atlas_sample_coords( const struct wp_volume_atlas *atlas, wp_vec3f volume_position,
                                       struct wp_volume_sample_coords *out )
{
    struct wp_volume_slice lower, upper;
    wp_f32 x, y, z, slice_position;
    wp_u32 lower_index, upper_index;

    WORKPHONE_ASSERT( out );
    if( !out )
        return wp_false;

    wp_zero( out, sizeof( *out ) );

    if( !wp_volume_atlas_is_valid( atlas ) )
        return wp_false;

    WORKPHONE_CLAMP( volume_position.x, 0.0f, 1.0f );
    WORKPHONE_CLAMP( volume_position.y, 0.0f, 1.0f );
    WORKPHONE_CLAMP( volume_position.z, 0.0f, 1.0f );

    x = volume_position.x;
    y = volume_position.y;
    z = volume_position.z;

    slice_position = z * (wp_f32)( atlas->depth - 1 );
    lower_index = (wp_u32)slice_position;
    upper_index = ( lower_index + 1 < atlas->depth ) ? lower_index + 1 : lower_index;
    out->slice_blend = slice_position - (wp_f32)lower_index;

    if( !wp_volume_atlas_slice( atlas, lower_index, &lower ) ||
        !wp_volume_atlas_slice( atlas, upper_index, &upper ) )
        return wp_false;

    out->lower_uv.x = lower.uv.x + ( lower.uv.z - lower.uv.x ) * x;
    out->lower_uv.y = lower.uv.y + ( lower.uv.w - lower.uv.y ) * y;
    out->upper_uv.x = upper.uv.x + ( upper.uv.z - upper.uv.x ) * x;
    out->upper_uv.y = upper.uv.y + ( upper.uv.w - upper.uv.y ) * y;
    return wp_true;
}
