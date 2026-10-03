/**
 * @file workphone_vehicle_softbody.c
 * @brief C89 implementation of the deformable soft-body vehicle chassis.
 */

#include "workphone_vehicle_softbody.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"
#include <string.h>
#include <stdlib.h>

/* =========================================================================
 * Default constants
 * ====================================================================== */

#define WP_SOFTBODY_VEHICLE_DEFAULT_SUBSTEPS 8
#define WP_SOFTBODY_VEHICLE_DEFAULT_VELOCITY_DAMPING 0.05f
#define WP_SOFTBODY_VEHICLE_DEFAULT_MAX_BEAM_FORCE 500000.0f
#define WP_SOFTBODY_VEHICLE_DEFAULT_MAX_NODE_SPEED 150.0f
#define WP_SOFTBODY_VEHICLE_DEFAULT_COLLISION_SKIN 0.002f
#define WP_SOFTBODY_VEHICLE_DEFAULT_RESTITUTION 0.05f
#define WP_SOFTBODY_VEHICLE_DEFAULT_FRICTION 10.0f
#define WP_SOFTBODY_VEHICLE_MIN_NODE_MASS 0.001f
#define WP_SOFTBODY_VEHICLE_MIN_NODE_RADIUS 0.001f
#define WP_SOFTBODY_VEHICLE_EPSILON 0.000001f

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_s32 wp_softbody_vehicle_is_valid_node_index( const wp_softbody_vehicle *vehicle,
                                                        wp_s32 node_index )
{
    if( vehicle == NULL || vehicle->nodes == NULL )
    {
        return wp_false;
    }

    if( node_index < 0 || node_index >= vehicle->node_count )
    {
        return wp_false;
    }

    return wp_true;
}

static wp_s32 wp_softbody_vehicle_is_valid_beam_index( const wp_softbody_vehicle *vehicle,
                                                        wp_s32 beam_index )
{
    if( vehicle == NULL || vehicle->beams == NULL )
    {
        return wp_false;
    }

    if( beam_index < 0 || beam_index >= vehicle->beam_count )
    {
        return wp_false;
    }

    return wp_true;
}

static void wp_softbody_vehicle_free_arrays( wp_softbody_vehicle *vehicle )
{
    if( vehicle->node_definitions != NULL )
    {
        free( vehicle->node_definitions );
        vehicle->node_definitions = NULL;
    }

    if( vehicle->beam_definitions != NULL )
    {
        free( vehicle->beam_definitions );
        vehicle->beam_definitions = NULL;
    }

    if( vehicle->nodes != NULL )
    {
        free( vehicle->nodes );
        vehicle->nodes = NULL;
    }

    if( vehicle->beams != NULL )
    {
        free( vehicle->beams );
        vehicle->beams = NULL;
    }

    if( vehicle->external_forces != NULL )
    {
        free( vehicle->external_forces );
        vehicle->external_forces = NULL;
    }

    vehicle->node_count = 0;
    vehicle->beam_count = 0;
    vehicle->is_initialized = wp_false;
}

static void wp_softbody_vehicle_prepare_forces( wp_softbody_vehicle *vehicle )
{
    wp_s32 i;

    for( i = 0; i < vehicle->node_count; ++i )
    {
        wp_softbody_node_state *node = &vehicle->nodes[i];

        if( node->anchored )
        {
            node->force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
            node->velocity = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
            continue;
        }

        {
            wp_f32 mass = 1.0f / node->inverse_mass;
            wp_vec3f gravity_force = wp_vec3f_scale( vehicle->gravity, mass );
            node->force = wp_vec3f_add( gravity_force, vehicle->external_forces[i] );
        }
    }
}

static void wp_softbody_vehicle_solve_beams( wp_softbody_vehicle *vehicle, wp_f32 delta_time )
{
    wp_s32 i;

    for( i = 0; i < vehicle->beam_count; ++i )
    {
        wp_softbody_beam_state *beam = &vehicle->beams[i];
        wp_softbody_node_state *node_a;
        wp_softbody_node_state *node_b;
        wp_vec3f difference;
        wp_f32 current_length;
        wp_f32 inverse_length;
        wp_vec3f direction;
        wp_f32 extension;
        wp_f32 signed_strain;
        wp_f32 absolute_strain;
        wp_f32 relative_speed;
        wp_f32 spring_force;
        wp_f32 damping_force;
        wp_f32 force_magnitude;
        wp_vec3f force;

        if( beam->broken )
        {
            continue;
        }

        node_a = &vehicle->nodes[beam->node_a];
        node_b = &vehicle->nodes[beam->node_b];

        difference = wp_vec3f_sub( node_b->position, node_a->position );
        current_length = wp_vec3f_length( difference );

        if( current_length < WP_SOFTBODY_VEHICLE_EPSILON )
        {
            continue;
        }

        inverse_length = 1.0f / current_length;
        direction = wp_vec3f_scale( difference, inverse_length );

        extension = current_length - beam->rest_length;
        signed_strain = extension / beam->rest_length;
        absolute_strain = wp_absf( signed_strain );

        if( absolute_strain >= beam->break_strain )
        {
            beam->broken = wp_true;
            continue;
        }

        if( absolute_strain > beam->yield_strain && beam->plasticity > 0.0f )
        {
            wp_f32 plastic_blend = 1.0f - wp_expf( -beam->plasticity * delta_time );
            beam->rest_length = wp_lerpf( beam->rest_length, current_length, plastic_blend );
        }

        relative_speed = wp_vec3f_dot( wp_vec3f_sub( node_b->velocity, node_a->velocity ), direction );
        spring_force = beam->stiffness * extension;
        damping_force = beam->damping * relative_speed;
        force_magnitude = wp_clampf( spring_force + damping_force,
                                     -vehicle->maximum_beam_force,
                                     vehicle->maximum_beam_force );
        force = wp_vec3f_scale( direction, force_magnitude );

        if( !node_a->anchored )
        {
            node_a->force = wp_vec3f_add( node_a->force, force );
        }

        if( !node_b->anchored )
        {
            node_b->force = wp_vec3f_sub( node_b->force, force );
        }
    }
}

static void wp_softbody_vehicle_resolve_collision( wp_softbody_vehicle *vehicle,
                                                     wp_softbody_node_state *node,
                                                     wp_vec3f target_position, wp_f32 delta_time )
{
    wp_vec3f movement;
    wp_f32 distance;
    wp_vec3f direction;
    wp_f32 max_distance;
    wp_softbody_raycast_hit hit;

    movement = wp_vec3f_sub( target_position, node->position );
    distance = wp_vec3f_length( movement );

    if( distance < WP_SOFTBODY_VEHICLE_EPSILON )
    {
        node->position = target_position;
        return;
    }

    direction = wp_vec3f_scale( movement, 1.0f / distance );
    max_distance = distance + vehicle->collision_skin;

    node->position = target_position;

    if( vehicle->callbacks.raycast == NULL )
    {
        return;
    }

    hit = vehicle->callbacks.raycast( vehicle->callbacks.user_data,
                                       node->position,
                                       wp_vec3f_negate( direction ),
                                       max_distance,
                                       vehicle->collision_mask );

    if( !hit.hit )
    {
        return;
    }

    {
        wp_f32 safe_distance = wp_maxf( 0.0f, hit.distance - vehicle->collision_skin );
        wp_vec3f normal = wp_vec3f_normalize( hit.normal );
        wp_vec3f old_velocity = node->velocity;
        wp_f32 normal_speed = wp_vec3f_dot( old_velocity, normal );
        wp_f32 friction_multiplier;
        wp_vec3f inward_normal_velocity;
        wp_vec3f tangential_velocity;
        wp_vec3f new_velocity;

        node->position = wp_vec3f_add( node->position, wp_vec3f_scale( direction, -safe_distance ) );

        if( normal_speed >= 0.0f )
        {
            return;
        }

        friction_multiplier = 1.0f / ( 1.0f + vehicle->collision_friction * delta_time );
        inward_normal_velocity = wp_vec3f_scale( normal, normal_speed );
        tangential_velocity = wp_vec3f_sub( old_velocity, inward_normal_velocity );
        new_velocity = wp_vec3f_sub( wp_vec3f_scale( tangential_velocity, friction_multiplier ),
                                     wp_vec3f_scale( inward_normal_velocity, vehicle->restitution ) );

        node->velocity = new_velocity;

        if( vehicle->callbacks.apply_impulse != NULL && hit.body != NULL && !node->anchored )
        {
            wp_f32 node_mass = 1.0f / node->inverse_mass;
            wp_vec3f node_impulse = wp_vec3f_scale( wp_vec3f_sub( new_velocity, old_velocity ),
                                                     node_mass );
            vehicle->callbacks.apply_impulse( vehicle->callbacks.user_data,
                                                hit.body,
                                                hit.point,
                                                wp_vec3f_negate( node_impulse ) );
        }
    }
}

static void wp_softbody_vehicle_integrate_nodes( wp_softbody_vehicle *vehicle, wp_f32 delta_time )
{
    wp_f32 damping_multiplier;
    wp_f32 maximum_speed_squared;
    wp_s32 i;

    damping_multiplier = 1.0f / ( 1.0f + vehicle->global_velocity_damping * delta_time );
    maximum_speed_squared = vehicle->maximum_node_speed * vehicle->maximum_node_speed;

    for( i = 0; i < vehicle->node_count; ++i )
    {
        wp_softbody_node_state *node = &vehicle->nodes[i];
        wp_vec3f acceleration;
        wp_vec3f target_position;
        wp_f32 speed_squared;

        if( node->anchored )
        {
            continue;
        }

        acceleration = wp_vec3f_scale( node->force, node->inverse_mass );

        node->velocity = wp_vec3f_add( node->velocity, wp_vec3f_scale( acceleration, delta_time ) );
        node->velocity = wp_vec3f_scale( node->velocity, damping_multiplier );

        speed_squared = wp_vec3f_length_sq( node->velocity );
        if( speed_squared > maximum_speed_squared )
        {
            wp_f32 inverse_speed = 1.0f / wp_sqrtf( speed_squared );
            node->velocity = wp_vec3f_scale( node->velocity,
                                                vehicle->maximum_node_speed * inverse_speed );
        }

        target_position = wp_vec3f_add( node->position, wp_vec3f_scale( node->velocity, delta_time ) );

        wp_softbody_vehicle_resolve_collision( vehicle, node, target_position, delta_time );
    }
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_softbody_vehicle_init( wp_softbody_vehicle *vehicle )
{
    memset( vehicle, 0, sizeof( *vehicle ) );

    vehicle->substeps = WP_SOFTBODY_VEHICLE_DEFAULT_SUBSTEPS;
    vehicle->global_velocity_damping = WP_SOFTBODY_VEHICLE_DEFAULT_VELOCITY_DAMPING;
    vehicle->maximum_beam_force = WP_SOFTBODY_VEHICLE_DEFAULT_MAX_BEAM_FORCE;
    vehicle->maximum_node_speed = WP_SOFTBODY_VEHICLE_DEFAULT_MAX_NODE_SPEED;
    vehicle->collision_mask = 0xFFFFFFFFu;
    vehicle->collision_skin = WP_SOFTBODY_VEHICLE_DEFAULT_COLLISION_SKIN;
    vehicle->restitution = WP_SOFTBODY_VEHICLE_DEFAULT_RESTITUTION;
    vehicle->collision_friction = WP_SOFTBODY_VEHICLE_DEFAULT_FRICTION;
    vehicle->gravity = wp_vec3f_make( 0.0f, -9.81f, 0.0f );

    vehicle->transform.position = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    vehicle->transform.orientation = wp_quatf_identity();
    vehicle->transform.scale = wp_vec3f_make( 1.0f, 1.0f, 1.0f );

    vehicle->is_initialized = wp_false;
}

void wp_softbody_vehicle_destroy( wp_softbody_vehicle *vehicle )
{
    if( vehicle == NULL )
    {
        return;
    }

    wp_softbody_vehicle_free_arrays( vehicle );
}

void wp_softbody_vehicle_set_definitions( wp_softbody_vehicle *vehicle,
                                          const wp_softbody_node_def *nodes, wp_s32 node_count,
                                          const wp_softbody_beam_def *beams, wp_s32 beam_count )
{
    wp_s32 i;

    if( vehicle == NULL )
    {
        return;
    }

    wp_softbody_vehicle_free_arrays( vehicle );

    if( nodes == NULL || node_count <= 0 )
    {
        return;
    }

    vehicle->node_definitions = ( wp_softbody_node_def * )malloc(
        ( wp_size )node_count * sizeof( wp_softbody_node_def ) );
    if( vehicle->node_definitions == NULL )
    {
        return;
    }

    vehicle->nodes = ( wp_softbody_node_state * )malloc(
        ( wp_size )node_count * sizeof( wp_softbody_node_state ) );
    if( vehicle->nodes == NULL )
    {
        free( vehicle->node_definitions );
        vehicle->node_definitions = NULL;
        return;
    }

    vehicle->external_forces = ( wp_vec3f * )malloc(
        ( wp_size )node_count * sizeof( wp_vec3f ) );
    if( vehicle->external_forces == NULL )
    {
        free( vehicle->nodes );
        free( vehicle->node_definitions );
        vehicle->nodes = NULL;
        vehicle->node_definitions = NULL;
        return;
    }

    memcpy( vehicle->node_definitions, nodes,
            ( wp_size )node_count * sizeof( wp_softbody_node_def ) );
    memset( vehicle->external_forces, 0,
            ( wp_size )node_count * sizeof( wp_vec3f ) );

    vehicle->node_count = node_count;

    for( i = 0; i < node_count; ++i )
    {
        const wp_softbody_node_def *definition = &vehicle->node_definitions[i];
        wp_softbody_node_state *node = &vehicle->nodes[i];
        wp_f32 mass;
        wp_f32 radius;

        mass = wp_maxf( WP_SOFTBODY_VEHICLE_MIN_NODE_MASS, definition->mass );
        radius = wp_maxf( WP_SOFTBODY_VEHICLE_MIN_NODE_RADIUS, definition->radius );

        node->position = wp_game_util_transform_point( &vehicle->transform, definition->local_position );
        node->velocity = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
        node->force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
        node->inverse_mass = definition->anchored ? 0.0f : 1.0f / mass;
        node->radius = radius;
        node->anchored = definition->anchored;
    }

    vehicle->beam_count = 0;
    vehicle->beams = NULL;
    vehicle->beam_definitions = NULL;

    if( beams != NULL && beam_count > 0 )
    {
        vehicle->beam_definitions = ( wp_softbody_beam_def * )malloc(
            ( wp_size )beam_count * sizeof( wp_softbody_beam_def ) );
        vehicle->beams = ( wp_softbody_beam_state * )malloc(
            ( wp_size )beam_count * sizeof( wp_softbody_beam_state ) );

        if( vehicle->beam_definitions == NULL || vehicle->beams == NULL )
        {
            wp_softbody_vehicle_free_arrays( vehicle );
            return;
        }

        memcpy( vehicle->beam_definitions, beams,
                ( wp_size )beam_count * sizeof( wp_softbody_beam_def ) );
        vehicle->beam_count = beam_count;

        for( i = 0; i < beam_count; ++i )
        {
            const wp_softbody_beam_def *definition = &vehicle->beam_definitions[i];
            wp_softbody_beam_state *beam = &vehicle->beams[i];
            wp_vec3f node_a_position;
            wp_vec3f node_b_position;

            beam->node_a = definition->node_a;
            beam->node_b = definition->node_b;
            beam->stiffness = definition->stiffness;
            beam->damping = definition->damping;
            beam->yield_strain = definition->yield_strain;
            beam->plasticity = definition->plasticity;
            beam->break_strain = definition->break_strain;
            beam->broken = wp_false;

            if( beam->node_a >= 0 && beam->node_a < vehicle->node_count &&
                beam->node_b >= 0 && beam->node_b < vehicle->node_count )
            {
                node_a_position = vehicle->nodes[beam->node_a].position;
                node_b_position = vehicle->nodes[beam->node_b].position;
                beam->rest_length = wp_vec3f_distance( node_a_position, node_b_position );
            }
            else
            {
                beam->rest_length = 0.0f;
            }
        }
    }

    vehicle->is_initialized = wp_true;
}

void wp_softbody_vehicle_set_callbacks( wp_softbody_vehicle *vehicle,
                                          wp_softbody_vehicle_callbacks callbacks )
{
    if( vehicle == NULL )
    {
        return;
    }

    vehicle->callbacks = callbacks;
}

void wp_softbody_vehicle_set_transform( wp_softbody_vehicle *vehicle, wp_transform3f transform )
{
    if( vehicle == NULL )
    {
        return;
    }

    vehicle->transform = transform;
}

void wp_softbody_vehicle_step( wp_softbody_vehicle *vehicle, wp_f32 dt )
{
    wp_s32 step_count;
    wp_f32 step_delta_time;
    wp_s32 step;

    if( vehicle == NULL || !vehicle->is_initialized )
    {
        return;
    }

    step_count = vehicle->substeps;
    if( step_count < 1 )
    {
        step_count = 1;
    }
    step_delta_time = dt / ( wp_f32 )step_count;

    for( step = 0; step < step_count; ++step )
    {
        wp_softbody_vehicle_prepare_forces( vehicle );
        wp_softbody_vehicle_solve_beams( vehicle, step_delta_time );
        wp_softbody_vehicle_integrate_nodes( vehicle, step_delta_time );
    }

    wp_softbody_vehicle_clear_external_forces( vehicle );
}

void wp_softbody_vehicle_add_force( wp_softbody_vehicle *vehicle, wp_s32 node_index, wp_vec3f force )
{
    if( vehicle == NULL || !vehicle->is_initialized )
    {
        return;
    }

    if( !wp_softbody_vehicle_is_valid_node_index( vehicle, node_index ) )
    {
        return;
    }

    vehicle->external_forces[node_index] = wp_vec3f_add( vehicle->external_forces[node_index], force );
}

void wp_softbody_vehicle_add_impulse( wp_softbody_vehicle *vehicle, wp_s32 node_index,
                                     wp_vec3f impulse )
{
    wp_softbody_node_state *node;

    if( vehicle == NULL || !vehicle->is_initialized )
    {
        return;
    }

    if( !wp_softbody_vehicle_is_valid_node_index( vehicle, node_index ) )
    {
        return;
    }

    node = &vehicle->nodes[node_index];

    if( node->anchored )
    {
        return;
    }

    node->velocity = wp_vec3f_add( node->velocity, wp_vec3f_scale( impulse, node->inverse_mass ) );
}

void wp_softbody_vehicle_clear_external_forces( wp_softbody_vehicle *vehicle )
{
    if( vehicle == NULL || vehicle->external_forces == NULL || vehicle->node_count <= 0 )
    {
        return;
    }

    memset( vehicle->external_forces, 0,
            ( wp_size )vehicle->node_count * sizeof( wp_vec3f ) );
}

wp_s32 wp_softbody_vehicle_is_initialized( const wp_softbody_vehicle *vehicle )
{
    if( vehicle == NULL )
    {
        return wp_false;
    }

    return vehicle->is_initialized;
}

wp_s32 wp_softbody_vehicle_get_node_count( const wp_softbody_vehicle *vehicle )
{
    if( vehicle == NULL )
    {
        return 0;
    }

    return vehicle->node_count;
}

wp_s32 wp_softbody_vehicle_get_beam_count( const wp_softbody_vehicle *vehicle )
{
    if( vehicle == NULL )
    {
        return 0;
    }

    return vehicle->beam_count;
}

wp_vec3f wp_softbody_vehicle_get_node_position( const wp_softbody_vehicle *vehicle,
                                                wp_s32 node_index )
{
    if( !wp_softbody_vehicle_is_valid_node_index( vehicle, node_index ) )
    {
        if( vehicle == NULL )
        {
            return wp_vec3f_make( 0.0f, 0.0f, 0.0f );
        }

        return vehicle->transform.position;
    }

    return vehicle->nodes[node_index].position;
}

wp_vec3f wp_softbody_vehicle_get_node_velocity( const wp_softbody_vehicle *vehicle,
                                               wp_s32 node_index )
{
    if( !wp_softbody_vehicle_is_valid_node_index( vehicle, node_index ) )
    {
        return wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    }

    return vehicle->nodes[node_index].velocity;
}

wp_vec3f wp_softbody_vehicle_get_rest_local_position( const wp_softbody_vehicle *vehicle,
                                                      wp_s32 node_index )
{
    if( vehicle == NULL || vehicle->node_definitions == NULL )
    {
        return wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    }

    if( node_index < 0 || node_index >= vehicle->node_count )
    {
        return wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    }

    return vehicle->node_definitions[node_index].local_position;
}

wp_s32 wp_softbody_vehicle_is_beam_broken( const wp_softbody_vehicle *vehicle, wp_s32 beam_index )
{
    if( !wp_softbody_vehicle_is_valid_beam_index( vehicle, beam_index ) )
    {
        return wp_false;
    }

    return vehicle->beams[beam_index].broken;
}

wp_vec3f wp_softbody_vehicle_get_center_of_mass( const wp_softbody_vehicle *vehicle )
{
    wp_vec3f weighted_position;
    wp_f32 total_mass;
    wp_s32 i;

    if( vehicle == NULL || !vehicle->is_initialized || vehicle->node_count <= 0 )
    {
        if( vehicle == NULL )
        {
            return wp_vec3f_make( 0.0f, 0.0f, 0.0f );
        }

        return vehicle->transform.position;
    }

    weighted_position = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    total_mass = 0.0f;

    for( i = 0; i < vehicle->node_count; ++i )
    {
        const wp_softbody_node_state *node = &vehicle->nodes[i];
        wp_f32 mass;

        if( node->inverse_mass <= 0.0f )
        {
            continue;
        }

        mass = 1.0f / node->inverse_mass;
        weighted_position = wp_vec3f_add( weighted_position, wp_vec3f_scale( node->position, mass ) );
        total_mass += mass;
    }

    if( total_mass > 0.0f )
    {
        return wp_vec3f_scale( weighted_position, 1.0f / total_mass );
    }

    return vehicle->transform.position;
}

void wp_softbody_vehicle_get_frame( const wp_softbody_vehicle *vehicle,
                                     wp_s32 front_left_node, wp_s32 front_right_node,
                                     wp_s32 rear_left_node, wp_s32 rear_right_node,
                                     wp_vec3f *out_centre, wp_vec3f *out_right,
                                     wp_vec3f *out_up, wp_vec3f *out_forward )
{
    wp_vec3f front_left;
    wp_vec3f front_right;
    wp_vec3f rear_left;
    wp_vec3f rear_right;
    wp_vec3f front_centre;
    wp_vec3f rear_centre;
    wp_vec3f left_centre;
    wp_vec3f right_centre;

    front_left = wp_softbody_vehicle_get_node_position( vehicle, front_left_node );
    front_right = wp_softbody_vehicle_get_node_position( vehicle, front_right_node );
    rear_left = wp_softbody_vehicle_get_node_position( vehicle, rear_left_node );
    rear_right = wp_softbody_vehicle_get_node_position( vehicle, rear_right_node );

    front_centre = wp_vec3f_scale( wp_vec3f_add( front_left, front_right ), 0.5f );
    rear_centre = wp_vec3f_scale( wp_vec3f_add( rear_left, rear_right ), 0.5f );
    left_centre = wp_vec3f_scale( wp_vec3f_add( front_left, rear_left ), 0.5f );
    right_centre = wp_vec3f_scale( wp_vec3f_add( front_right, rear_right ), 0.5f );

    *out_centre = wp_vec3f_scale(
        wp_vec3f_add( wp_vec3f_add( front_left, front_right ),
                      wp_vec3f_add( rear_left, rear_right ) ),
        0.25f );

    *out_forward = wp_vec3f_normalize( wp_vec3f_sub( front_centre, rear_centre ) );
    *out_right = wp_vec3f_normalize( wp_vec3f_sub( right_centre, left_centre ) );
    *out_up = wp_vec3f_normalize( wp_vec3f_cross( *out_forward, *out_right ) );
    *out_right = wp_vec3f_normalize( wp_vec3f_cross( *out_up, *out_forward ) );
}

