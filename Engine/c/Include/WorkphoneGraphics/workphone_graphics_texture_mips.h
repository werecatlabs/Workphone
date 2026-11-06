#ifndef WORKPHONE_GRAPHICS_TEXTURE_MIPS_H
#define WORKPHONE_GRAPHICS_TEXTURE_MIPS_H

#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compute the next whole-image mip extent while keeping atlas tiles separate.
 * Returns zero for invalid extents, invalid atlas layouts, or the end of the
 * chain. Output extents are written only on success. */
wp_s32 wp_graphics_texture_next_mip_extent( wp_u32 width, wp_u32 height, wp_u32 atlas_columns,
                                          wp_u32 *next_width, wp_u32 *next_height );

/* Select the first resident level of a validated, contiguous mip chain.
 * Quality 0 skips up to two levels and quality 1 skips up to one. A nonzero
 * maximum caps both dimensions when an available level fits. The last level
 * remains usable when the authored chain ends before reaching the target size.
 * Returns zero for an empty chain or zero base dimensions. */
wp_u32 wp_graphics_texture_first_mip( wp_u32 width, wp_u32 height, wp_u32 mip_count,
                                     wp_s32 quality, wp_u32 maximum );

#ifdef __cplusplus
}
#endif

#endif
