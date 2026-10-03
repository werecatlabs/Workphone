#ifndef workphone_volume_h__
#define workphone_volume_h__

#include "workphone_image.h"
#include "workphone_vector.h"

/*
 * A volume atlas stores the z slices of a volume in a regular 2D grid.  The
 * layout is deliberately independent of a graphics API so it can be uploaded
 * as an ordinary 2D texture by each renderer.  This is also useful for data
 * produced by font and sprite-sheet tooling: a slice is simply another cell
 * in the atlas.
 */
struct wp_volume_atlas
{
    wp_handle texture;
    wp_u32 texture_width;
    wp_u32 texture_height;
    wp_u32 slice_width;
    wp_u32 slice_height;
    wp_u32 depth;
    wp_u32 columns;
    wp_u32 rows;
    wp_u32 stride_x;
    wp_u32 stride_y;
    wp_u32 padding_x;
    wp_u32 padding_y;
};

struct wp_volume_slice
{
    struct wp_rect pixels;
    wp_vec4f uv; /* left, top, right, bottom; half-texel inset */
};

/* Coordinates needed by a shader or software marcher for trilinear lookup. */
struct wp_volume_sample_coords
{
    struct wp_vec2f lower_uv;
    struct wp_vec2f upper_uv;
    wp_f32 slice_blend;
};

WORKPHONE_API void wp_volume_atlas_init( struct wp_volume_atlas *, wp_handle texture,
                                         wp_u32 texture_width, wp_u32 texture_height,
                                         wp_u32 slice_width, wp_u32 slice_height,
                                         wp_u32 depth, wp_u32 columns, wp_u32 rows,
                                         wp_u32 padding_x, wp_u32 padding_y );
WORKPHONE_API wp_bool wp_volume_atlas_is_valid( const struct wp_volume_atlas * );
WORKPHONE_API wp_bool wp_volume_atlas_slice( const struct wp_volume_atlas *, wp_u32 index,
                                             struct wp_volume_slice *out );
WORKPHONE_API wp_bool wp_volume_atlas_frame( const struct wp_volume_atlas *, wp_u32 index,
                                             struct wp_image *out );
WORKPHONE_API wp_bool wp_volume_atlas_sample_coords( const struct wp_volume_atlas *,
                                                     struct wp_vec3f volume_position,
                                                     struct wp_volume_sample_coords *out );

#endif  // workphone_volume_h__
