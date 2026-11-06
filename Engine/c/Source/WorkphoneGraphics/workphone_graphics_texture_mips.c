#include "workphone_graphics_texture_mips.h"

static wp_u32 wp_texture_half_extent( wp_u32 extent )
{
    return extent > 1 ? extent / 2 : 1;
}

wp_s32 wp_graphics_texture_next_mip_extent( wp_u32 width, wp_u32 height, wp_u32 atlas_columns,
                                          wp_u32 *next_width, wp_u32 *next_height )
{
    wp_u32 half_width;
    if( !width || !height || !atlas_columns || width % atlas_columns || !next_width ||
        !next_height || ( width == 1 && height == 1 ) )
    {
        return 0;
    }

    half_width = wp_texture_half_extent( width );
    if( half_width % atlas_columns )
    {
        return 0;
    }

    *next_width = half_width;
    *next_height = wp_texture_half_extent( height );
    return 1;
}

wp_u32 wp_graphics_texture_first_mip( wp_u32 width, wp_u32 height, wp_u32 mip_count,
                                     wp_s32 quality, wp_u32 maximum )
{
    wp_u32 first = 0;
    wp_u32 skip = quality == 0 ? 2u : quality == 1 ? 1u : 0u;
    if( !width || !height || !mip_count )
    {
        return 0;
    }

    while( first + 1 < mip_count &&
           ( first < skip || ( maximum && ( width > maximum || height > maximum ) ) ) )
    {
        width = wp_texture_half_extent( width );
        height = wp_texture_half_extent( height );
        ++first;
    }
    return first;
}
