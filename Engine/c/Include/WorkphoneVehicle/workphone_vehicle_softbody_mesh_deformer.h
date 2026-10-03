/**
 * @file workphone_vehicle_softbody_mesh_deformer.h
 * @brief C89 API for binding and deforming a vertex mesh to a soft-body
 *        vehicle.
 *
 * Ported from the Unity SoftBodyMeshDeformer. Vertices are bound to the
 * nearest soft-body nodes and displaced each frame to follow the simulated
 * lattice. Only positions are updated; callers should recompute normals
 * through their renderer or mesh pipeline.
 */

#ifndef WORKPHONE_VEHICLE_SOFTBODY_MESH_DEFORMER_H
#define WORKPHONE_VEHICLE_SOFTBODY_MESH_DEFORMER_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_game_actor.h"
#include "workphone_vehicle_softbody.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of node influences stored per vertex.
 */
#ifndef WP_SOFTBODY_MESH_DEFORMER_MAX_INFLUENCES
#    define WP_SOFTBODY_MESH_DEFORMER_MAX_INFLUENCES 4
#endif

/**
 * @brief Vertex deformer state.
 */
typedef struct wp_softbody_mesh_deformer
{
    wp_s32 vertex_count;
    wp_s32 influence_count;
    wp_s32 bound;
    wp_s32 node_count;

    wp_vec3f *original_vertices;
    wp_vec3f *deformed_vertices;
    wp_vec3f *rest_node_positions;
    wp_vec3f *current_node_positions;
    wp_s32 *bind_indices;
    wp_f32 *bind_weights;
} wp_softbody_mesh_deformer;

/**
 * @brief Initialise a deformer with no binding.
 */
void wp_softbody_mesh_deformer_init( wp_softbody_mesh_deformer *deformer );

/**
 * @brief Destroy a deformer and free all internal arrays.
 */
void wp_softbody_mesh_deformer_destroy( wp_softbody_mesh_deformer *deformer );

/**
 * @brief Bind the deformer to a vehicle and a set of local-space vertices.
 *
 * @param deformer           Deformer state.
 * @param vehicle            Soft-body vehicle to follow.
 * @param vertices           Local-space vertex positions in the mesh's
 *                           object space.
 * @param vertex_count       Number of vertices.
 * @param influences_per_vertex Number of node influences per vertex [1, 4].
 * @param mesh_to_world      Transform from mesh object space to world space.
 * @return Non-zero on success, zero if binding failed.
 */
wp_s32 wp_softbody_mesh_deformer_bind( wp_softbody_mesh_deformer *deformer,
                                        const wp_softbody_vehicle *vehicle,
                                        const wp_vec3f *vertices,
                                        wp_s32 vertex_count,
                                        wp_s32 influences_per_vertex,
                                        wp_transform3f mesh_to_world );

/**
 * @brief Update deformed vertex positions from the current vehicle state.
 *
 * @param deformer      Deformer state.
 * @param vehicle       Soft-body vehicle to read node positions from.
 * @param mesh_to_world Current transform from mesh object space to world
 *                      space.
 */
void wp_softbody_mesh_deformer_update( wp_softbody_mesh_deformer *deformer,
                                        const wp_softbody_vehicle *vehicle,
                                        wp_transform3f mesh_to_world );

/**
 * @brief Copy the current deformed vertex positions into a caller-supplied
 *        buffer.
 *
 * The buffer must hold at least vertex_count elements.
 */
void wp_softbody_mesh_deformer_get_vertices( const wp_softbody_mesh_deformer *deformer,
                                              wp_vec3f *out_vertices );

/**
 * @brief Return the number of bound vertices.
 */
wp_s32 wp_softbody_mesh_deformer_get_vertex_count( const wp_softbody_mesh_deformer *deformer );

/**
 * @brief Return non-zero if the deformer has been bound.
 */
wp_s32 wp_softbody_mesh_deformer_is_bound( const wp_softbody_mesh_deformer *deformer );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_SOFTBODY_MESH_DEFORMER_H */
