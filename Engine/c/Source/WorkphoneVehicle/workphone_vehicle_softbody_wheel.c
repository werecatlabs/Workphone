/**
 * @file workphone_vehicle_softbody_wheel.c
 * @brief C89 implementation of the soft-body vehicle wheel.
 */

#include "workphone_vehicle_softbody_wheel.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"
#include <string.h>

/* =========================================================================
 * Default constants
 * ====================================================================== */

#define WP_SOFTBODY_WHEEL_DEFAULT_SUSPENSION_TOP_OFFSET 0.15f
#define WP_SOFTBODY_WHEEL_DEFAULT_REST_LENGTH 0.42f
#define WP_SOFTBODY_WHEEL_DEFAULT_TRAVEL 0.18f
#define WP_SOFTBODY_WHEEL_DEFAULT_SPRING_RATE 42000.0f
#define WP_SOFTBODY_WHEEL_DEFAULT_DAMPER_RATE 4500.0f
#define WP_SOFTBODY_WHEEL_DEFAULT_WHEEL_RADIUS 0.34f
#define WP_SOFTBODY_WHEEL_DEFAULT_WHEEL_INERTIA 1.2f
#define WP_SOFTBODY_WHEEL_DEFAULT_LONGITUDINAL_STIFFNESS 9000.0f
#define WP_SOFTBODY_WHEEL_DEFAULT_CORNERING_STIFFNESS 7500.0f
#define WP_SOFTBODY_WHEEL_DEFAULT_FRICTION_COEFFICIENT 1.15f
#define WP_SOFTBODY_WHEEL_DEFAULT_ROLLING_RESISTANCE 0.15f
#define WP_SOFTBODY_WHEEL_DEFAULT_MAX_STEERING_ANGLE 30.0f
#define WP_SOFTBODY_WHEEL_DEFAULT_MAX_DRIVE_TORQUE 900.0f
#define WP_SOFTBODY_WHEEL_DEFAULT_MAX_BRAKE_TORQUE 1800.0f
#define WP_SOFTBODY_WHEEL_EPSILON 0.000001f
#define WP_SOFTBODY_WHEEL_MIN_DT 0.000001f

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static void wp_softbody_wheel_apply_airborne_torque( wp_softbody_wheel *wheel,
                                                      wp_f32 drive_torque,
                                                      wp_f32 brake,
                                                      wp_f32 delta_time )
{
    wp_f32 requested_brake_torque;
    wp_f32 torque_to_stop;
    wp_f32 brake_torque;

    requested_brake_torque = brake * wheel->maximum_brake_torque;
    torque_to_stop = -wheel->angular_velocity * wheel->wheel_inertia /
                     wp_maxf( delta_time, WP_SOFTBODY_WHEEL_MIN_DT );
    brake_torque = wp_clampf( torque_to_stop, -requested_brake_torque, requested_brake_torque );

    wheel->angular_velocity += ( drive_torque + brake_torque ) / wheel->wheel_inertia * delta_time;
    wheel->angular_velocity /= 1.0f + wheel->rolling_resistance * delta_time;
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_softbody_wheel_init( wp_softbody_wheel *wheel )
{
    if( wheel == NULL )
    {
        return;
    }

    memset( wheel, 0, sizeof( *wheel ) );

    wheel->name[0] = '\0';
    wheel->node_index = -1;
    wheel->steering = wp_false;
    wheel->driven = wp_false;

    wheel->suspension_top_offset = WP_SOFTBODY_WHEEL_DEFAULT_SUSPENSION_TOP_OFFSET;
    wheel->rest_length = WP_SOFTBODY_WHEEL_DEFAULT_REST_LENGTH;
    wheel->travel = WP_SOFTBODY_WHEEL_DEFAULT_TRAVEL;
    wheel->spring_rate = WP_SOFTBODY_WHEEL_DEFAULT_SPRING_RATE;
    wheel->damper_rate = WP_SOFTBODY_WHEEL_DEFAULT_DAMPER_RATE;

    wheel->wheel_radius = WP_SOFTBODY_WHEEL_DEFAULT_WHEEL_RADIUS;
    wheel->wheel_inertia = WP_SOFTBODY_WHEEL_DEFAULT_WHEEL_INERTIA;
    wheel->longitudinal_stiffness = WP_SOFTBODY_WHEEL_DEFAULT_LONGITUDINAL_STIFFNESS;
    wheel->cornering_stiffness = WP_SOFTBODY_WHEEL_DEFAULT_CORNERING_STIFFNESS;
    wheel->friction_coefficient = WP_SOFTBODY_WHEEL_DEFAULT_FRICTION_COEFFICIENT;
    wheel->rolling_resistance = WP_SOFTBODY_WHEEL_DEFAULT_ROLLING_RESISTANCE;

    wheel->maximum_steering_angle = WP_SOFTBODY_WHEEL_DEFAULT_MAX_STEERING_ANGLE;
    wheel->maximum_drive_torque = WP_SOFTBODY_WHEEL_DEFAULT_MAX_DRIVE_TORQUE;
    wheel->maximum_brake_torque = WP_SOFTBODY_WHEEL_DEFAULT_MAX_BRAKE_TORQUE;

    wheel->ground_mask = 0xFFFFFFFFu;

    wheel->angular_velocity = 0.0f;
    wheel->has_previous_length = wp_false;
}

void wp_softbody_wheel_step( wp_softbody_wheel *wheel,
                             wp_softbody_vehicle *vehicle,
                             wp_vec3f chassis_right,
                             wp_vec3f chassis_up,
                             wp_vec3f chassis_forward,
                             wp_f32 throttle,
                             wp_f32 brake,
                             wp_f32 steering_input,
                             wp_f32 delta_time,
                             const wp_softbody_vehicle_callbacks *callbacks )
{
    wp_vec3f node_position;
    wp_vec3f suspension_origin;
    wp_f32 maximum_cast_length;
    wp_f32 drive_torque;
    wp_softbody_raycast_hit hit;
    wp_f32 current_length;
    wp_f32 compression;
    wp_f32 compression_speed;
    wp_f32 normal_load;
    wp_vec3f suspension_force;
    wp_f32 steering_angle;
    wp_vec3f wheel_forward;
    wp_vec3f wheel_right;
    wp_vec3f ground_velocity;
    wp_vec3f relative_velocity;
    wp_f32 forward_speed;
    wp_f32 lateral_speed;
    wp_f32 tyre_surface_speed;
    wp_f32 longitudinal_slip_speed;
    wp_f32 longitudinal_force;
    wp_f32 lateral_force;
    wp_f32 maximum_tyre_force;
    wp_f32 tyre_force_magnitude;
    wp_f32 scale;
    wp_vec3f tyre_force;
    wp_vec3f total_force;
    wp_f32 reaction_torque;
    wp_f32 requested_brake_torque;
    wp_f32 torque_to_stop_wheel;
    wp_f32 brake_torque;
    wp_f32 total_wheel_torque;

    if( wheel == NULL || vehicle == NULL )
    {
        return;
    }

    if( wheel->node_index < 0 || wheel->node_index >= wp_softbody_vehicle_get_node_count( vehicle ) )
    {
        return;
    }

    node_position = wp_softbody_vehicle_get_node_position( vehicle, wheel->node_index );
    suspension_origin = wp_vec3f_add( node_position,
                                        wp_vec3f_scale( chassis_up, wheel->suspension_top_offset ) );
    maximum_cast_length = wheel->rest_length + wheel->travel;
    drive_torque = wheel->driven ? throttle * wheel->maximum_drive_torque : 0.0f;

    wheel->grounded = wp_false;

    if( callbacks == NULL || callbacks->raycast == NULL )
    {
        wp_softbody_wheel_apply_airborne_torque( wheel, drive_torque, brake, delta_time );
        wheel->has_previous_length = wp_false;
        return;
    }

    hit = callbacks->raycast( callbacks->user_data,
                                 suspension_origin,
                                 wp_vec3f_negate( chassis_up ),
                                 maximum_cast_length,
                                 wheel->ground_mask );

    if( !hit.hit )
    {
        wp_softbody_wheel_apply_airborne_torque( wheel, drive_torque, brake, delta_time );
        wheel->has_previous_length = wp_false;
        return;
    }

    wheel->contact_point = hit.point;
    wheel->grounded = wp_true;

    current_length = hit.distance;

    if( !wheel->has_previous_length )
    {
        wheel->previous_length = current_length;
        wheel->has_previous_length = wp_true;
    }

    compression = wp_maxf( 0.0f, wheel->rest_length - current_length );
    compression_speed = ( wheel->previous_length - current_length ) /
                        wp_maxf( delta_time, WP_SOFTBODY_WHEEL_MIN_DT );
    wheel->previous_length = current_length;

    normal_load = wp_maxf( 0.0f, compression * wheel->spring_rate +
                                compression_speed * wheel->damper_rate );
    suspension_force = wp_vec3f_scale( chassis_up, normal_load );

    steering_angle = wheel->steering
                         ? steering_input * wheel->maximum_steering_angle
                         : 0.0f;

    {
        wp_quatf rotation = wp_quatf_from_axis_angle( chassis_up,
                                                       steering_angle * WORKPHONE_DEG2RAD_F );
        wheel_forward = wp_quatf_rotate_vec3( rotation, chassis_forward );
    }

    {
        wp_f32 projection = wp_vec3f_dot( wheel_forward, hit.normal );
        wheel_forward = wp_vec3f_sub( wheel_forward, wp_vec3f_scale( hit.normal, projection ) );
    }

    if( wp_vec3f_length_sq( wheel_forward ) < 0.001f )
    {
        wheel_forward = chassis_forward;
    }
    else
    {
        wheel_forward = wp_vec3f_normalize( wheel_forward );
    }

    wheel_right = wp_vec3f_normalize( wp_vec3f_cross( hit.normal, wheel_forward ) );
    wheel_forward = wp_vec3f_cross( wheel_right, hit.normal );

    ground_velocity = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    if( callbacks->get_point_velocity != NULL && hit.body != NULL )
    {
        ground_velocity = callbacks->get_point_velocity( callbacks->user_data, hit.body, hit.point );
    }

    relative_velocity = wp_vec3f_sub( wp_softbody_vehicle_get_node_velocity( vehicle, wheel->node_index ),
                                       ground_velocity );
    forward_speed = wp_vec3f_dot( relative_velocity, wheel_forward );
    lateral_speed = wp_vec3f_dot( relative_velocity, wheel_right );

    tyre_surface_speed = wheel->angular_velocity * wheel->wheel_radius;
    longitudinal_slip_speed = tyre_surface_speed - forward_speed;

    longitudinal_force = longitudinal_slip_speed * wheel->longitudinal_stiffness;
    lateral_force = -lateral_speed * wheel->cornering_stiffness;

    maximum_tyre_force = wheel->friction_coefficient * normal_load;
    tyre_force_magnitude = wp_sqrtf( longitudinal_force * longitudinal_force +
                                      lateral_force * lateral_force );

    if( tyre_force_magnitude > maximum_tyre_force && tyre_force_magnitude > WP_SOFTBODY_WHEEL_EPSILON )
    {
        scale = maximum_tyre_force / tyre_force_magnitude;
        longitudinal_force *= scale;
        lateral_force *= scale;
    }

    tyre_force = wp_vec3f_add( wp_vec3f_scale( wheel_forward, longitudinal_force ),
                                wp_vec3f_scale( wheel_right, lateral_force ) );
    total_force = wp_vec3f_add( suspension_force, tyre_force );

    wp_softbody_vehicle_add_force( vehicle, wheel->node_index, total_force );

    if( callbacks->add_force_at_position != NULL && hit.body != NULL )
    {
        callbacks->add_force_at_position( callbacks->user_data,
                                           hit.body,
                                           hit.point,
                                           wp_vec3f_negate( total_force ) );
    }

    reaction_torque = -longitudinal_force * wheel->wheel_radius;
    requested_brake_torque = brake * wheel->maximum_brake_torque;
    torque_to_stop_wheel = -wheel->angular_velocity * wheel->wheel_inertia /
                           wp_maxf( delta_time, WP_SOFTBODY_WHEEL_MIN_DT );
    brake_torque = wp_clampf( torque_to_stop_wheel, -requested_brake_torque, requested_brake_torque );

    total_wheel_torque = drive_torque + reaction_torque + brake_torque;
    wheel->angular_velocity += total_wheel_torque / wheel->wheel_inertia * delta_time;
    wheel->angular_velocity /= 1.0f + wheel->rolling_resistance * delta_time;
}
