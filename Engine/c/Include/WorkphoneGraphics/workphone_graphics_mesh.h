/**
 * @file wp_graphics_mesh.h
 * @brief C API for a graphics mesh.
 *
 * A mesh stores the raw geometry used for rendering: an interleaved vertex
 * buffer, an optional index buffer, and one or more submeshes that partition
 * the index buffer into material regions.
 *
 * Vertex data layout is controlled by the wp_vertex_format enumeration.
 * All vertex formats begin with a 3D position so that generic operations such
 * as AABB computation can access position data without format-specific
 * branching.
 */

#ifndef WORKPHONE_GRAPHICS_MESH_H
#define WORKPHONE_GRAPHICS_MESH_H

#include <stdint.h>
#include "workphone_vector.h"
#include "workphone_graphics_object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations
 * ---------------------------------------------------------------------- */

typedef struct wp_graphics_mesh wp_graphics_mesh;
typedef struct wp_renderer wp_renderer;
typedef struct wp_graphics_material wp_graphics_material;

/* -------------------------------------------------------------------------
 * Vertex structs
 * ---------------------------------------------------------------------- */

/** @brief Vertex with a 3D position only (12 bytes). */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space. */
} wp_graphics_mesh_vertex_p;

/** @brief Vertex with a 3D position and surface normal (24 bytes). */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space.        */
    wp_vec3f normal;   /**< Surface normal (should be unit length). */
} wp_graphics_mesh_vertex_pn;

/** @brief Vertex with a 3D position and one UV texture coordinate (20 bytes). */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space. */
    wp_vec2f uv;       /**< Primary texture coordinate.      */
} wp_graphics_mesh_vertex_pt;

/** @brief Vertex with position, normal, and UV texture coordinate (32 bytes). */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space.        */
    wp_vec3f normal;   /**< Surface normal (should be unit length). */
    wp_vec2f uv;       /**< Primary texture coordinate.             */
} wp_graphics_mesh_vertex_pnt;

/** @brief Vertex with position, normal, UV, and packed RGBA colour (36 bytes). */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space.             */
    wp_vec3f normal;   /**< Surface normal (should be unit length).       */
    wp_vec2f uv;       /**< Primary texture coordinate.                   */
    wp_u32 color;      /**< Packed RGBA colour (R in bits 24-31, A in 0). */
} wp_graphics_mesh_vertex_pntc;

/** @brief Vertex with position and packed RGBA colour (16 bytes). */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space.             */
    wp_u32 color;      /**< Packed RGBA colour (R in bits 24-31, A in 0). */
} wp_graphics_mesh_vertex_pc;

/** @brief Vertex with position, UV, and packed RGBA colour (24 bytes). */
typedef struct
{
    wp_vec3f position; /**< Vertex position in object space.             */
    wp_vec2f uv;       /**< Primary texture coordinate.                   */
    wp_u32 color;      /**< Packed RGBA colour (R in bits 24-31, A in 0). */
} wp_graphics_mesh_vertex_ptc;

/* -------------------------------------------------------------------------
 * Vertex format
 * ---------------------------------------------------------------------- */

/**
 * @brief Identifies the interleaved vertex layout stored in a mesh's vertex buffer.
 *
 * The stride (bytes per vertex) for each format can be obtained via
 * wp_vertex_format_stride().
 */
typedef enum wp_vertex_format
{
    WORKPHONE_VERTEX_FORMAT_P =
        0, /**< wp_graphics_mesh_vertex_p    - position only                    (12 B). */
    WORKPHONE_VERTEX_FORMAT_PN =
        1, /**< wp_graphics_mesh_vertex_pn   - position + normal                (24 B). */
    WORKPHONE_VERTEX_FORMAT_PT =
        2, /**< wp_graphics_mesh_vertex_pt   - position + uv                   (20 B). */
    WORKPHONE_VERTEX_FORMAT_PNT =
        3, /**< wp_graphics_mesh_vertex_pnt  - position + normal + uv          (32 B). */
    WORKPHONE_VERTEX_FORMAT_PNTC =
        4, /**< wp_graphics_mesh_vertex_pntc - position + normal + uv + colour  (36 B). */
    WORKPHONE_VERTEX_FORMAT_PC =
        5, /**< wp_graphics_mesh_vertex_pc   - position + colour                (16 B). */
    WORKPHONE_VERTEX_FORMAT_PTC =
        6 /**< wp_graphics_mesh_vertex_ptc  - position + uv + colour           (24 B). */
} wp_vertex_format;

/* -------------------------------------------------------------------------
 * Index format
 * ---------------------------------------------------------------------- */

/**
 * @brief Width of the elements in the index buffer.
 */
typedef enum wp_index_format
{
    WORKPHONE_INDEX_FORMAT_UINT16 = 0, /**< 16-bit unsigned integers (max 65535 unique vertices). */
    WORKPHONE_INDEX_FORMAT_UINT32 = 1  /**< 32-bit unsigned integers.                             */
} wp_index_format;

/* -------------------------------------------------------------------------
 * Primitive type
 * ---------------------------------------------------------------------- */

/**
 * @brief How the index/vertex buffer is interpreted during draw calls.
 */
typedef enum wp_primitive_type
{
    WORKPHONE_PRIMITIVE_TRIANGLE_LIST = 0,  /**< Independent triangles; 3 indices per primitive.      */
    WORKPHONE_PRIMITIVE_TRIANGLE_STRIP = 1, /**< Triangle strip; 2 shared indices per extra triangle.  */
    WORKPHONE_PRIMITIVE_LINE_LIST = 2,      /**< Independent line segments; 2 indices per segment.    */
    WORKPHONE_PRIMITIVE_LINE_STRIP = 3,     /**< Connected line strip; 1 new index per extra segment.  */
    WORKPHONE_PRIMITIVE_POINT_LIST = 4      /**< Independent points; 1 index per point.               */
} wp_primitive_type;

/* -------------------------------------------------------------------------
 * Submesh
 * ---------------------------------------------------------------------- */

/**
 * @brief Describes a contiguous region of the index buffer bound to a material.
 *
 * A mesh may contain multiple submeshes, each referencing a different range
 * of indices and associating it with a material identifier.  When a mesh has
 * no submeshes the entire index buffer (or vertex buffer when unindexed) is
 * rendered as a single draw call.
 */
typedef struct
{
    wp_u32 index_start; /**< Zero-based index of the first element in the index buffer. */
    wp_u32 index_count; /**< Number of indices in this submesh.                         */
    wp_u32 material_id; /**< Hash or enumeration value identifying the material.        */
} wp_submesh;

/* -------------------------------------------------------------------------
 * Vertex format helpers
 * ---------------------------------------------------------------------- */

/**
 * @brief Returns the size in bytes of a single vertex for the given format.
 * @param fmt Vertex format to query.
 * @return Stride in bytes, or 0 if fmt is not a recognised format.
 */
wp_u32 wp_vertex_format_stride( wp_vertex_format fmt );

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new empty mesh.
 *
 * The mesh starts with no vertices, no indices, no submeshes, and
 * WORKPHONE_PRIMITIVE_TRIANGLE_LIST as the primitive type.
 *
 * @return Pointer to the created mesh, or NULL on allocation failure.
 */
wp_graphics_mesh *wp_graphics_mesh_create( void );

/**
 * @brief Destroys a mesh and releases all owned vertex, index, and submesh memory.
 * @param mesh Pointer to the mesh to destroy. Ignored if NULL.
 */
void wp_graphics_mesh_destroy( wp_graphics_mesh *mesh );

/* -------------------------------------------------------------------------
 * Vertex buffer
 * ---------------------------------------------------------------------- */

/**
 * @brief Copies vertex data into the mesh, replacing any existing vertex buffer.
 *
 * The mesh takes ownership of an internal copy of the data; the caller may
 * free its buffer after this call returns.
 *
 * @param mesh  Pointer to the mesh.
 * @param fmt   Interleaved vertex layout of the supplied data.
 * @param data  Pointer to the source vertex data. Must not be NULL.
 * @param count Number of vertices to copy.
 * @return Non-zero on success; zero if allocation failed or parameters are invalid.
 */
wp_s32 wp_graphics_mesh_set_vertices( wp_graphics_mesh *mesh, wp_vertex_format fmt, const void *data,
                                      wp_u32 count );

/**
 * @brief Returns a read-only pointer to the mesh's vertex buffer.
 * @param mesh Pointer to the mesh.
 * @return Pointer to the first vertex, or NULL if no vertex data has been set.
 */
const void *wp_graphics_mesh_get_vertices( const wp_graphics_mesh *mesh );

/**
 * @brief Returns the number of vertices stored in the mesh.
 * @param mesh Pointer to the mesh.
 * @return Vertex count, or 0 if mesh is NULL.
 */
wp_u32 wp_graphics_mesh_get_vertex_count( const wp_graphics_mesh *mesh );

/**
 * @brief Returns the stride (bytes per vertex) determined by the active format.
 * @param mesh Pointer to the mesh.
 * @return Stride in bytes, or 0 if mesh is NULL.
 */
wp_u32 wp_graphics_mesh_get_vertex_stride( const wp_graphics_mesh *mesh );

/**
 * @brief Returns the vertex format currently set on the mesh.
 * @param mesh Pointer to the mesh.
 * @return Active vertex format enumeration value.
 */
wp_vertex_format wp_graphics_mesh_get_vertex_format( const wp_graphics_mesh *mesh );

/* -------------------------------------------------------------------------
 * Index buffer
 * ---------------------------------------------------------------------- */

/**
 * @brief Copies 16-bit index data into the mesh, replacing any existing index buffer.
 *
 * @param mesh  Pointer to the mesh.
 * @param data  Pointer to the source index data. Must not be NULL.
 * @param count Number of indices to copy.
 * @return Non-zero on success; zero if allocation failed or parameters are invalid.
 */
wp_s32 wp_graphics_mesh_set_indices_u16( wp_graphics_mesh *mesh, const uint16_t *data, wp_u32 count );

/**
 * @brief Copies 32-bit index data into the mesh, replacing any existing index buffer.
 *
 * @param mesh  Pointer to the mesh.
 * @param data  Pointer to the source index data. Must not be NULL.
 * @param count Number of indices to copy.
 * @return Non-zero on success; zero if allocation failed or parameters are invalid.
 */
wp_s32 wp_graphics_mesh_set_indices_u32( wp_graphics_mesh *mesh, const wp_u32 *data, wp_u32 count );

/**
 * @brief Returns a read-only pointer to the mesh's index buffer.
 * @param mesh Pointer to the mesh.
 * @return Pointer to the first index element, or NULL if no index buffer is set.
 */
const void *wp_graphics_mesh_get_indices( const wp_graphics_mesh *mesh );

/**
 * @brief Returns the number of indices in the mesh.
 * @param mesh Pointer to the mesh.
 * @return Index count, or 0 if mesh is NULL or no index buffer has been set.
 */
wp_u32 wp_graphics_mesh_get_index_count( const wp_graphics_mesh *mesh );

/**
 * @brief Returns the element width of the active index buffer.
 * @param mesh Pointer to the mesh.
 * @return WORKPHONE_INDEX_FORMAT_UINT16 or WORKPHONE_INDEX_FORMAT_UINT32.
 */
wp_index_format wp_graphics_mesh_get_index_format( const wp_graphics_mesh *mesh );

/* -------------------------------------------------------------------------
 * Primitive type
 * ---------------------------------------------------------------------- */

/**
 * @brief Sets the primitive topology used when drawing the mesh.
 * @param mesh Pointer to the mesh.
 * @param type New primitive type.
 */
void wp_graphics_mesh_set_primitive_type( wp_graphics_mesh *mesh, wp_primitive_type type );

/**
 * @brief Gets the primitive topology of the mesh.
 * @param mesh Pointer to the mesh.
 * @return Current primitive type.
 */
wp_primitive_type wp_graphics_mesh_get_primitive_type( const wp_graphics_mesh *mesh );

/* -------------------------------------------------------------------------
 * Submeshes
 * ---------------------------------------------------------------------- */

/**
 * @brief Adds a submesh defining an index range and material binding.
 *
 * @param mesh        Pointer to the mesh.
 * @param index_start First index in the index buffer belonging to this submesh.
 * @param index_count Number of indices in this submesh.
 * @param material_id Hash or enumeration value identifying the material.
 * @return Zero-based index of the new submesh, or -1 on failure.
 */
wp_s32 wp_graphics_mesh_add_submesh( wp_graphics_mesh *mesh, wp_u32 index_start, wp_u32 index_count,
                                     wp_u32 material_id );

/**
 * @brief Returns a read-only pointer to the submesh at the given index.
 * @param mesh  Pointer to the mesh.
 * @param index Zero-based submesh index.
 * @return Pointer to the submesh, or NULL if the index is out of range.
 */
const wp_submesh *wp_graphics_mesh_get_submesh( const wp_graphics_mesh *mesh, wp_s32 index );

/**
 * @brief Returns the number of submeshes defined on this mesh.
 * @param mesh Pointer to the mesh.
 * @return Submesh count, or 0 if mesh is NULL.
 */
wp_s32 wp_graphics_mesh_get_submesh_count( const wp_graphics_mesh *mesh );

/**
 * @brief Removes the submesh at the given index.
 *
 * Submeshes above the removed index are shifted down by one position.
 *
 * @param mesh  Pointer to the mesh.
 * @param index Zero-based index of the submesh to remove.
 */
void wp_graphics_mesh_remove_submesh( wp_graphics_mesh *mesh, wp_s32 index );

/**
 * @brief Removes all submeshes from the mesh without freeing the backing store.
 * @param mesh Pointer to the mesh.
 */
void wp_graphics_mesh_clear_submeshes( wp_graphics_mesh *mesh );

/* -------------------------------------------------------------------------
 * AABB
 * ---------------------------------------------------------------------- */

/**
 * @brief Returns the local axis-aligned bounding box of the mesh.
 *
 * If the AABB has not been computed or manually set the returned box is
 * zero-initialised.  Call wp_graphics_mesh_compute_aabb() to derive it from vertex
 * positions.
 *
 * @param mesh Pointer to the mesh.
 * @return Local AABB. Returns a zeroed struct if mesh is NULL.
 */
wp_aabb3f wp_graphics_mesh_get_local_aabb( const wp_graphics_mesh *mesh );

/**
 * @brief Manually sets the local axis-aligned bounding box.
 *
 * Use this when the AABB is known ahead of time and the overhead of
 * wp_graphics_mesh_compute_aabb() is not desired.
 *
 * @param mesh Pointer to the mesh.
 * @param aabb AABB to assign.
 */
void wp_graphics_mesh_set_local_aabb( wp_graphics_mesh *mesh, wp_aabb3f aabb );

/**
 * @brief Recomputes the AABB by iterating over all vertex positions.
 *
 * All vertex formats place the position at byte offset 0, so this function
 * works regardless of the active vertex format.
 *
 * @param mesh Pointer to the mesh. No-op if mesh is NULL or has no vertices.
 */
void wp_graphics_mesh_compute_aabb( wp_graphics_mesh *mesh );

/* -------------------------------------------------------------------------
 * Dirty flag
 * ---------------------------------------------------------------------- */

/**
 * @brief Returns non-zero if the mesh has been modified since the last GPU upload.
 * @param mesh Pointer to the mesh.
 * @return Non-zero if dirty, zero otherwise.
 */
wp_s32 wp_graphics_mesh_is_dirty( const wp_graphics_mesh *mesh );

/**
 * @brief Marks the mesh as modified so the renderer will re-upload its data.
 * @param mesh Pointer to the mesh.
 */
void wp_graphics_mesh_mark_dirty( wp_graphics_mesh *mesh );

/**
 * @brief Clears the dirty flag after a successful GPU upload.
 * @param mesh Pointer to the mesh.
 */
void wp_graphics_mesh_clear_dirty( wp_graphics_mesh *mesh );

/* -------------------------------------------------------------------------
 * Native GPU handle
 * ---------------------------------------------------------------------- */

/**
 * @brief Returns the native backend resource handle for this mesh.
 *
 * The meaning of the pointer is backend-specific (e.g. a D3D buffer pair
 * or a software renderer's internal structure).
 *
 * @param mesh Pointer to the mesh.
 * @return Native handle, or NULL if none has been set.
 */
void *wp_graphics_mesh_get_native( const wp_graphics_mesh *mesh );

/**
 * @brief Sets the native backend resource handle for this mesh.
 * @param mesh   Pointer to the mesh.
 * @param native Pointer to the native resource, or NULL to clear.
 */
void wp_graphics_mesh_set_native( wp_graphics_mesh *mesh, void *native );

/**
 * @brief Submits a mesh through the renderer-independent C graphics API.
 *
 * Vertex conversion is cached on the mesh and rebuilt only after mesh data is
 * changed. The material is non-owning and may be NULL.
 */
void wp_graphics_mesh_render( wp_graphics_mesh *mesh, wp_renderer *renderer,
                              const wp_graphics_material *material );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_MESH_H */
