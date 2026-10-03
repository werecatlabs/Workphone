/**
 * @file workphone_vehicle_softbody_demo.c
 * @brief C89 implementation of the procedural soft-body demo chassis.
 */

#include "workphone_vehicle_softbody_demo.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include <string.h>
#include <stdlib.h>

/* =========================================================================
 * Default constants
 * ====================================================================== */

#define WP_SOFTBODY_DEMO_DEFAULT_LENGTH 3.2f
#define WP_SOFTBODY_DEMO_DEFAULT_WIDTH 1.6f
#define WP_SOFTBODY_DEMO_DEFAULT_CHASSIS_HEIGHT 0.45f
#define WP_SOFTBODY_DEMO_DEFAULT_TOTAL_MASS 1100.0f
#define WP_SOFTBODY_DEMO_DEFAULT_NODE_RADIUS 0.11f
#define WP_SOFTBODY_DEMO_DEFAULT_BEAM_STIFFNESS 260000.0f
#define WP_SOFTBODY_DEMO_DEFAULT_BEAM_DAMPING 3000.0f
#define WP_SOFTBODY_DEMO_DEFAULT_YIELD_STRAIN 0.06f
#define WP_SOFTBODY_DEMO_DEFAULT_PLASTICITY 4.0f
#define WP_SOFTBODY_DEMO_DEFAULT_BREAK_STRAIN 0.42f

#define WP_SOFTBODY_DEMO_NODE_COUNT 12
#define WP_SOFTBODY_DEMO_MAX_BEAMS 128

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_s32 wp_softbody_demo_index( wp_s32 row, wp_s32 side, wp_s32 height )
{
    return row * 4 + side * 2 + height;
}

static wp_s32 wp_softbody_demo_has_beam( wp_u32 *pairs, wp_s32 pair_count,
                                          wp_s32 node_a, wp_s32 node_b )
{
    wp_s32 minimum = node_a < node_b ? node_a : node_b;
    wp_s32 maximum = node_a < node_b ? node_b : node_a;
    wp_u32 key = ( ( wp_u32 )minimum << 16 ) | ( wp_u32 )maximum;
    wp_s32 i;

    for( i = 0; i < pair_count; ++i )
    {
        if( pairs[i] == key )
        {
            return wp_true;
        }
    }

    return wp_false;
}

static void wp_softbody_demo_add_beam( wp_softbody_beam_def *beams,
                                        wp_s32 *beam_count,
                                        wp_u32 *pairs,
                                        wp_s32 *pair_count,
                                        wp_s32 node_a, wp_s32 node_b,
                                        const wp_softbody_demo_params *params )
{
    wp_s32 minimum;
    wp_s32 maximum;
    wp_u32 key;

    if( node_a == node_b )
    {
        return;
    }

    minimum = node_a < node_b ? node_a : node_b;
    maximum = node_a < node_b ? node_b : node_a;
    key = ( ( wp_u32 )minimum << 16 ) | ( wp_u32 )maximum;

    if( wp_softbody_demo_has_beam( pairs, *pair_count, node_a, node_b ) )
    {
        return;
    }

    pairs[*pair_count] = key;
    ( *pair_count )++;

    beams[*beam_count].node_a = node_a;
    beams[*beam_count].node_b = node_b;
    beams[*beam_count].stiffness = params->beam_stiffness;
    beams[*beam_count].damping = params->beam_damping;
    beams[*beam_count].yield_strain = params->yield_strain;
    beams[*beam_count].plasticity = params->plasticity;
    beams[*beam_count].break_strain = params->break_strain;
    ( *beam_count )++;
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_softbody_demo_car_init( wp_softbody_demo_car *demo, wp_softbody_vehicle *vehicle )
{
    if( demo == NULL )
    {
        return;
    }

    memset( demo, 0, sizeof( *demo ) );

    demo->vehicle = vehicle;

    demo->params.length = WP_SOFTBODY_DEMO_DEFAULT_LENGTH;
    demo->params.width = WP_SOFTBODY_DEMO_DEFAULT_WIDTH;
    demo->params.chassis_height = WP_SOFTBODY_DEMO_DEFAULT_CHASSIS_HEIGHT;
    demo->params.total_mass = WP_SOFTBODY_DEMO_DEFAULT_TOTAL_MASS;
    demo->params.node_radius = WP_SOFTBODY_DEMO_DEFAULT_NODE_RADIUS;
    demo->params.beam_stiffness = WP_SOFTBODY_DEMO_DEFAULT_BEAM_STIFFNESS;
    demo->params.beam_damping = WP_SOFTBODY_DEMO_DEFAULT_BEAM_DAMPING;
    demo->params.yield_strain = WP_SOFTBODY_DEMO_DEFAULT_YIELD_STRAIN;
    demo->params.plasticity = WP_SOFTBODY_DEMO_DEFAULT_PLASTICITY;
    demo->params.break_strain = WP_SOFTBODY_DEMO_DEFAULT_BREAK_STRAIN;
}

void wp_softbody_demo_car_set_params( wp_softbody_demo_car *demo,
                                       const wp_softbody_demo_params *params )
{
    if( demo == NULL || params == NULL )
    {
        return;
    }

    demo->params = *params;
}

void wp_softbody_demo_car_build_chassis( wp_softbody_demo_car *demo )
{
    wp_softbody_node_def nodes[WP_SOFTBODY_DEMO_NODE_COUNT];
    wp_softbody_beam_def beams[WP_SOFTBODY_DEMO_MAX_BEAMS];
    wp_u32 pairs[WP_SOFTBODY_DEMO_MAX_BEAMS];
    wp_s32 beam_count;
    wp_s32 pair_count;
    wp_f32 half_length;
    wp_f32 half_width;
    wp_f32 node_mass;
    wp_s32 row;
    wp_s32 side;
    wp_s32 height;
    wp_s32 next_row;

    if( demo == NULL || demo->vehicle == NULL )
    {
        return;
    }

    memset( nodes, 0, sizeof( nodes ) );
    memset( beams, 0, sizeof( beams ) );
    memset( pairs, 0, sizeof( pairs ) );

    beam_count = 0;
    pair_count = 0;

    half_length = demo->params.length * 0.5f;
    half_width = demo->params.width * 0.5f;
    node_mass = demo->params.total_mass / 12.0f;

    /* Three longitudinal sections: rear, middle and front. */
    for( row = 0; row < 3; ++row )
    {
        wp_f32 row_alpha = ( wp_f32 )row / 2.0f;
        wp_f32 z = wp_lerpf( -half_length, half_length, row_alpha );

        for( side = 0; side < 2; ++side )
        {
            wp_f32 x = side == 0 ? -half_width : half_width;

            for( height = 0; height < 2; ++height )
            {
                wp_f32 y = height == 0 ? 0.0f : demo->params.chassis_height;
                wp_s32 index = wp_softbody_demo_index( row, side, height );

                nodes[index].local_position = wp_vec3f_make( x, y, z );
                nodes[index].mass = node_mass;
                nodes[index].radius = demo->params.node_radius;
                nodes[index].anchored = wp_false;
            }
        }
    }

    /* Each cross-section. */
    for( row = 0; row < 3; ++row )
    {
        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 0, 0 ),
                                    wp_softbody_demo_index( row, 0, 1 ), &demo->params );
        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 1, 0 ),
                                    wp_softbody_demo_index( row, 1, 1 ), &demo->params );

        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 0, 0 ),
                                    wp_softbody_demo_index( row, 1, 0 ), &demo->params );
        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 0, 1 ),
                                    wp_softbody_demo_index( row, 1, 1 ), &demo->params );

        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 0, 0 ),
                                    wp_softbody_demo_index( row, 1, 1 ), &demo->params );
        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 1, 0 ),
                                    wp_softbody_demo_index( row, 0, 1 ), &demo->params );
    }

    /* Connect neighbouring sections. */
    for( row = 0; row < 2; ++row )
    {
        next_row = row + 1;

        /* Longitudinal rails. */
        for( side = 0; side < 2; ++side )
        {
            for( height = 0; height < 2; ++height )
            {
                wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                            wp_softbody_demo_index( row, side, height ),
                                            wp_softbody_demo_index( next_row, side, height ),
                                            &demo->params );
            }
        }

        /* Side-panel diagonals. */
        for( side = 0; side < 2; ++side )
        {
            wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                        wp_softbody_demo_index( row, side, 0 ),
                                        wp_softbody_demo_index( next_row, side, 1 ),
                                        &demo->params );
            wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                        wp_softbody_demo_index( row, side, 1 ),
                                        wp_softbody_demo_index( next_row, side, 0 ),
                                        &demo->params );
        }

        /* Floor and roof diagonals. */
        for( height = 0; height < 2; ++height )
        {
            wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                        wp_softbody_demo_index( row, 0, height ),
                                        wp_softbody_demo_index( next_row, 1, height ),
                                        &demo->params );
            wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                        wp_softbody_demo_index( row, 1, height ),
                                        wp_softbody_demo_index( next_row, 0, height ),
                                        &demo->params );
        }

        /* Full volume diagonals. */
        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 0, 0 ),
                                    wp_softbody_demo_index( next_row, 1, 1 ),
                                    &demo->params );
        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 1, 0 ),
                                    wp_softbody_demo_index( next_row, 0, 1 ),
                                    &demo->params );
        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 0, 1 ),
                                    wp_softbody_demo_index( next_row, 1, 0 ),
                                    &demo->params );
        wp_softbody_demo_add_beam( beams, &beam_count, pairs, &pair_count,
                                    wp_softbody_demo_index( row, 1, 1 ),
                                    wp_softbody_demo_index( next_row, 0, 0 ),
                                    &demo->params );
    }

    wp_softbody_vehicle_set_definitions( demo->vehicle, nodes, WP_SOFTBODY_DEMO_NODE_COUNT,
                                          beams, beam_count );
}
