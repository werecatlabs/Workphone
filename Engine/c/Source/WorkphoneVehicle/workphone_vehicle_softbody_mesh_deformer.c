/**
 * @file workphone_vehicle_softbody_mesh_deformer.c
 * @brief C89 implementation of the soft-body mesh deformer.
 */

#include "workphone_vehicle_softbody_mesh_deformer.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_game_util.h"
#include <string.h>
#include <stdlib.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void wp_softbody_mesh_deformer_free_arrays( wp_softbody_mesh_deformer *deformer )
{
    if( deformer->original_vertices != NULL )
    {
        free( deformer->original_vertices );
        deformer->original_vertices = NULL;
    }

    if( deformer->deformed_vertices != NULL )
    {
        free( deformer->deformed_vertices );
        deformer->deformed_vertices = NULL;
    }

    if( deformer->rest_node_positions != NULL )
    {
        free( deformer->rest_node_positions );
        deformer->rest_node_positions = NULL;
    }

    if( deformer->current_node_positions != NULL )
    {
        free( deformer->current_node_positions );
        deformer->current_node_positions = NULL;
    }

    if( deformer->bind_indices != NULL )
    {
        free( deformer->bind_indices );
        deformer->bind_indices = NULL;
    }

    if( deformer->bind_weights != NULL )
    {
        free( deformer->bind_weights );
        deformer->bind_weights = NULL;
    }

    deformer->vertex_count = 0;
    deformer->influence_count = 0;
    deformer->node_count = 0;
    deformer->bound = wp_false;
}

static void wp_softbody_mesh_deformer_insert_nearest( wp_f32 distance, wp_s32 node_index,
                                                       wp_f32 *best_distances,
                                                       wp_s32 *best_indices,
                                                       wp_s32 influence_count )
{
    wp_s32 slot;

    for( slot = 0; slot < influence_count; ++slot )
    {
        wp_s32 move;

        if( distance >= best_distances[slot] )
        {
            continue;
        }

        for( move = influence_count - 1; move > slot; --move )
        {
            best_distances[move] = best_distances[move - 1];
            best_indices[move] = best_indices[move - 1];
        }

        best_distances[slot] = distance;
        best_indices[slot] = node_index;
        return;
    }
}

static void wp_softbody_mesh_deformer_build_bindings( wp_softbody_mesh_deformer *deformer )
{
    wp_s32 vertex_index;

    for( vertex_index = 0; vertex_index < deformer->vertex_count; ++vertex_index )
    {
        wp_f32 best_distances[WP_SOFTBODY_MESH_DEFORMER_MAX_INFLUENCES];
        wp_s32 best_indices[WP_SOFTBODY_MESH_DEFORMER_MAX_INFLUENCES];
        wp_vec3f vertex;
        wp_s32 influence;
        wp_s32 node_index;
        wp_f32 weight_sum;

        for( influence = 0; influence < deformer->influence_count; ++influence )
        {
            best_distances[influence] = 3.402823466e+38f; /* FLT_MAX */
            best_indices[influence] = 0;
        }

        vertex = deformer->original_vertices[vertex_index];

        for( node_index = 0; node_index < deformer->node_count; ++node_index )
        {
            wp_vec3f delta = wp_vec3f_sub( vertex, deformer->rest_node_positions[node_index] );
            wp_f32 squared_distance = wp_vec3f_length_sq( delta );

            wp_softbody_mesh_deformer_insert_nearest( squared_distance, node_index,
                                                       best_distances, best_indices,
                                                       deformer->influence_count );
        }

        weight_sum = 0.0f;

        for( influence = 0; influence < deformer->influence_count; ++influence )
        {
            wp_f32 distance = wp_sqrtf( best_distances[influence] );
            wp_f32 weight = 1.0f / wp_maxf( distance, 0.001f );
            wp_s32 binding_index = vertex_index * deformer->influence_count + influence;

            deformer->bind_indices[binding_index] = best_indices[influence];
            deformer->bind_weights[binding_index] = weight;
            weight_sum += weight;
        }

        if( weight_sum <= 0.0f )
        {
            continue;
        }

        for( influence = 0; influence < deformer->influence_count; ++influence )
        {
            wp_s32 binding_index = vertex_index * deformer->influence_count + influence;
            deformer->bind_weights[binding_index] /= weight_sum;
        }
    }
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_softbody_mesh_deformer_init( wp_softbody_mesh_deformer *deformer )
{
    if( deformer == NULL )
    {
        return;
    }

    memset( deformer, 0, sizeof( *deformer ) );
}

void wp_softbody_mesh_deformer_destroy( wp_softbody_mesh_deformer *deformer )
{
    if( deformer == NULL )
    {
        return;
    }

    wp_softbody_mesh_deformer_free_arrays( deformer );
}

wp_s32 wp_softbody_mesh_deformer_bind( wp_softbody_mesh_deformer *deformer,
                                        const wp_softbody_vehicle *vehicle,
                                        const wp_vec3f *vertices,
                                        wp_s32 vertex_count,
                                        wp_s32 influences_per_vertex,
                                        wp_transform3f mesh_to_world )
{
    wp_s32 i;
    wp_s32 node_count;
    wp_transform3f world_to_mesh;

    if( deformer == NULL || vehicle == NULL || vertices == NULL || vertex_count <= 0 )
    {
        return wp_false;
    }

    if( !wp_softbody_vehicle_is_initialized( vehicle ) )
    {
        return wp_false;
    }

    wp_softbody_mesh_deformer_free_arrays( deformer );

    node_count = wp_softbody_vehicle_get_node_count( vehicle );

    deformer->influence_count = ( wp_s32 )wp_clampf( ( wp_f32 )influences_per_vertex, 1.0f,
                                              ( wp_f32 )WP_SOFTBODY_MESH_DEFORMER_MAX_INFLUENCES );
    deformer->vertex_count = vertex_count;
    deformer->node_count = node_count;

    deformer->original_vertices = ( wp_vec3f * )malloc(
        ( wp_size )vertex_count * sizeof( wp_vec3f ) );
    deformer->deformed_vertices = ( wp_vec3f * )malloc(
        ( wp_size )vertex_count * sizeof( wp_vec3f ) );
    deformer->rest_node_positions = ( wp_vec3f * )malloc(
        ( wp_size )node_count * sizeof( wp_vec3f ) );
    deformer->current_node_positions = ( wp_vec3f * )malloc(
        ( wp_size )node_count * sizeof( wp_vec3f ) );
    deformer->bind_indices = ( wp_s32 * )malloc(
        ( wp_size )vertex_count * deformer->influence_count * sizeof( wp_s32 ) );
    deformer->bind_weights = ( wp_f32 * )malloc(
        ( wp_size )vertex_count * deformer->influence_count * sizeof( wp_f32 ) );

    if( deformer->original_vertices == NULL ||
        deformer->deformed_vertices == NULL ||
        deformer->rest_node_positions == NULL ||
        deformer->current_node_positions == NULL ||
        deformer->bind_indices == NULL ||
        deformer->bind_weights == NULL )
    {
        wp_softbody_mesh_deformer_free_arrays( deformer );
        return wp_false;
    }

    memcpy( deformer->original_vertices, vertices,
            ( wp_size )vertex_count * sizeof( wp_vec3f ) );

    world_to_mesh.position = wp_game_util_inverse_transform_point( &mesh_to_world,
                                                                    wp_vec3f_make( 0.0f, 0.0f, 0.0f ) );
    world_to_mesh.orientation = wp_quatf_conjugate( mesh_to_world.orientation );
    world_to_mesh.scale = wp_vec3f_make( 1.0f, 1.0f, 1.0f ); /* Scale inverse not used for positions. */

    for( i = 0; i < node_count; ++i )
    {
        wp_vec3f node_world = wp_game_util_transform_point( &vehicle->transform,
                                                               wp_softbody_vehicle_get_rest_local_position( vehicle, i ) );
        deformer->rest_node_positions[i] = wp_game_util_inverse_transform_point( &mesh_to_world, node_world );
    }

    wp_softbody_mesh_deformer_build_bindings( deformer );

    deformer->bound = wp_true;
    return wp_true;
}

void wp_softbody_mesh_deformer_update( wp_softbody_mesh_deformer *deformer,
                                        const wp_softbody_vehicle *vehicle,
                                        wp_transform3f mesh_to_world )
{
    wp_s32 i;
    wp_s32 vertex_index;
    wp_transform3f world_to_mesh;

    if( deformer == NULL || vehicle == NULL || !deformer->bound )
    {
        return;
    }

    if( !wp_softbody_vehicle_is_initialized( vehicle ) )
    {
        return;
    }

    world_to_mesh.position = wp_game_util_inverse_transform_point( &mesh_to_world,
                                                                    wp_vec3f_make( 0.0f, 0.0f, 0.0f ) );
    world_to_mesh.orientation = wp_quatf_conjugate( mesh_to_world.orientation );
    world_to_mesh.scale = wp_vec3f_make( 1.0f, 1.0f, 1.0f );

    for( i = 0; i < deformer->node_count; ++i )
    {
        deformer->current_node_positions[i] = wp_game_util_inverse_transform_point(
            &world_to_mesh,
            wp_softbody_vehicle_get_node_position( vehicle, i ) );
    }

    for( vertex_index = 0; vertex_index < deformer->vertex_count; ++vertex_index )
    {
        wp_vec3f original_vertex = deformer->original_vertices[vertex_index];
        wp_vec3f deformed_vertex = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
        wp_s32 influence;

        for( influence = 0; influence < deformer->influence_count; ++influence )
        {
            wp_s32 binding_index = vertex_index * deformer->influence_count + influence;
            wp_s32 node_index = deformer->bind_indices[binding_index];
            wp_f32 weight = deformer->bind_weights[binding_index];
            wp_vec3f local_offset = wp_vec3f_sub( original_vertex,
                                                   deformer->rest_node_positions[node_index] );

            deformed_vertex = wp_vec3f_add( deformed_vertex,
                                              wp_vec3f_scale( wp_vec3f_add( deformer->current_node_positions[node_index],
                                                                            local_offset ),
                                                              weight ) );
        }

        deformer->deformed_vertices[vertex_index] = deformed_vertex;
    }
}

void wp_softbody_mesh_deformer_get_vertices( const wp_softbody_mesh_deformer *deformer,
                                              wp_vec3f *out_vertices )
{
    if( deformer == NULL || out_vertices == NULL || !deformer->bound )
    {
        return;
    }

    memcpy( out_vertices, deformer->deformed_vertices,
            ( wp_size )deformer->vertex_count * sizeof( wp_vec3f ) );
}

wp_s32 wp_softbody_mesh_deformer_get_vertex_count( const wp_softbody_mesh_deformer *deformer )
{
    if( deformer == NULL || !deformer->bound )
    {
        return 0;
    }

    return deformer->vertex_count;
}

wp_s32 wp_softbody_mesh_deformer_is_bound( const wp_softbody_mesh_deformer *deformer )
{
    if( deformer == NULL )
    {
        return wp_false;
    }

    return deformer->bound;
}
