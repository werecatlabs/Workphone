#ifndef workphone_graphics_vertex_h__
#define workphone_graphics_vertex_h__

#include "workphone_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Vertex with a 3D position and a packed RGBA colour.
 */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space.              */
    wp_u32 color;      /**< Packed RGBA colour (R in bits 24-31, A in 0). */
} wp_vertex_pc;

/**
 * @brief Vertex with a 3D position, 2D texture coordinate, and packed colour.
 */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space.              */
    wp_vec2f uv;       /**< Texture coordinate (u, v).                    */
    wp_u32 color;      /**< Packed RGBA colour (R in bits 24-31, A in 0). */
} wp_vertex_ptc;

/**
 * @brief Lit mesh vertex with position, normal, texture coordinate and colour.
 */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space.              */
    wp_vec3f normal;   /**< Vertex normal in object space.                */
    wp_vec2f uv;       /**< Texture coordinate (u, v).                    */
    wp_u32 color;      /**< Packed RGBA colour (R in bits 24-31, A in 0). */
} wp_vertex_pntc;

#ifdef __cplusplus
}
#endif

#endif  // workphone_graphics_vertex_h__
