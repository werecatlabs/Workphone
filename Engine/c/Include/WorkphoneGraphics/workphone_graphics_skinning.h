#ifndef WORKPHONE_GRAPHICS_SKINNING_H
#define WORKPHONE_GRAPHICS_SKINNING_H

#include <stddef.h>
#include "workphone_graphics_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WP_SKIN_MAX_JOINTS 256u
#define WP_SKIN_INFLUENCES 4u

typedef struct wp_skin_vertex
{
    wp_vec3f position;
    wp_vec3f normal;
    wp_u16 joints[WP_SKIN_INFLUENCES];
    wp_f32 weights[WP_SKIN_INFLUENCES];
} wp_skin_vertex;

typedef struct wp_skin_result
{
    wp_vec3f position;
    wp_vec3f normal;
} wp_skin_result;

/* Row-major matrices act on column vectors, translation in m[0..2][3].
 * Palette entries are model-space current_joint * inverse_bind.
 * This initial reference supports rigid/uniform-scale affine transforms only.
 * Validate all input before writing output; zero weights retain bind geometry.
 * Input/output arrays must not overlap. No allocations or global state. */
wp_s32 wp_skin_vertices( const wp_skin_vertex *vertices, wp_u32 vertex_count,
                         const wp_mat4f *palette, wp_u32 joint_count,
                         wp_skin_result *output );

#ifdef __cplusplus
}
#endif
#endif
