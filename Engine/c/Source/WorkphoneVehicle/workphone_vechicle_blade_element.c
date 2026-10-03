/**
 * @file workphone_vechicle_blade_element.c
 * @brief C99 implementation of the blade-element-theory (BET) rotor
 *        simulation.
 */

#include "workphone_vechicle_blade_element.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/**
 * @brief Compute the velocity of a world-space powp_s32 on the rigid body.
 *
 * Equivalent to Unity's Rigidbody.GetPointVelocity:
 *   point_velocity = linear_velocity + cross(angular_velocity, offset)
 * where offset = world_powp_s32 - body_position (approximating CoM at origin).
 *
 * @param r         Pointer to the rotor instance (provides body state).
 * @param world_pos World-space position of the powp_s32 of interest.
 * @return          Velocity of the powp_s32 in m/s.
 */
static wp_vec3f wp_bero_point_velocity( const wp_blade_element_rotor *r, wp_vec3f world_pos )
{
    wp_vec3f offset = wp_vec3f_sub( world_pos, r->position );
    return wp_vec3f_add( r->velocity, wp_vec3f_cross( r->angular_velocity, offset ) );
}

/* =========================================================================
 * Core simulation
 * ====================================================================== */

static void wp_bero_simulate( wp_blade_element_rotor *r )
{
    /* World-space constants for this tick */
    static const wp_vec3f world_up = { 0.0f, 1.0f, 0.0f };

    wp_f32 omega = r->rotor_rpm * WORKPHONE_PI_F * 2.0f / 60.0f;
    wp_vec3f right = wp_quatf_rotate_vec3( r->orientation, wp_vec3f_make( 1.0f, 0.0f, 0.0f ) );
    wp_vec3f omega_vec = wp_vec3f_scale( world_up, omega );

    wp_vec3f total_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    wp_vec3f total_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

    wp_s32 blade;
    for( blade = 0; blade < r->blade_count; ++blade )
    {
        /* Azimuth angle for this blade in radians */
        wp_f32 blade_angle = (wp_f32)blade * WORKPHONE_PI_F * 2.0f / (wp_f32)r->blade_count;

        /* Quaternion that rotates the blade arm around world up */
        wp_quatf blade_rot = wp_quatf_from_axis_angle( world_up, blade_angle );

        /* Cyclic pitch mixing at this azimuth position */
        wp_f32 cyclic_pitch =
            r->cyclic_input.x * wp_sinf( blade_angle ) + r->cyclic_input.y * wp_cosf( blade_angle );

        wp_s32 seg;
        for( seg = 1; seg <= r->segments_per_blade; ++seg )
        {
            wp_f32 radius = (wp_f32)seg * r->segment_length;

            /* Local-space position of this blade segment
             * (rotated around world up by the blade azimuth angle) */
            wp_vec3f local_pos = wp_quatf_rotate_vec3( blade_rot, wp_vec3f_make( radius, 0.0f, 0.0f ) );

            /* World-space position: transform.TransformPoint(localPos)
             * = position + orientation * localPos                       */
            wp_vec3f world_pos =
                wp_vec3f_add( r->position, wp_quatf_rotate_vec3( r->orientation, local_pos ) );

            /* Tangential velocity from rotor spin:
             * cross(omega * world_up, localPos)                         */
            wp_vec3f tangential_vel = wp_vec3f_cross( omega_vec, local_pos );

            /* Rigid-body powp_s32 velocity at this world position:
             * linear_velocity + cross(angular_velocity, worldPos - pos) */
            wp_vec3f point_vel = wp_bero_point_velocity( r, world_pos );

            /* Total air velocity seen by this segment */
            wp_vec3f air_velocity = wp_vec3f_add( tangential_vel, point_vel );

            wp_f32 speed = wp_vec3f_length( air_velocity );

            if( speed < 0.01f )
                continue;

            wp_vec3f airflow_dir = wp_vec3f_normalize( air_velocity );

            /* Blade pitch at this azimuth: collective + cyclic mixing */
            wp_f32 pitch = r->collective_pitch + cyclic_pitch;
            wp_f32 alpha = pitch * WORKPHONE_DEG2RAD_F;

            /* Lift and drag coefficients */
            wp_f32 cl = r->lift_slope * alpha;
            wp_f32 cd = r->drag_coeff + cl * cl * 0.02f;

            /* Dynamic pressure * planform area */
            wp_f32 area = r->chord_length * r->segment_length;
            wp_f32 q = 0.5f * r->air_density * speed * speed * area;

            wp_f32 lift_mag = q * cl;
            wp_f32 drag_mag = q * cd;

            /* Lift direction: perpendicular to airflow and rotor right axis
             * liftDir = normalize(cross(airflowDir, transform.right))   */
            wp_vec3f lift_cross = wp_vec3f_cross( airflow_dir, right );

            /* Guard against degenerate case (airflow parallel to right) */
            if( wp_vec3f_is_zero_length( lift_cross ) )
                continue;

            wp_vec3f lift_dir = wp_vec3f_normalize( lift_cross );
            wp_vec3f drag_dir = wp_vec3f_negate( airflow_dir );

            wp_vec3f force = wp_vec3f_add( wp_vec3f_scale( lift_dir, lift_mag ),
                                           wp_vec3f_scale( drag_dir, drag_mag ) );

            total_force = wp_vec3f_add( total_force, force );
            total_torque = wp_vec3f_add( total_torque, wp_vec3f_cross( local_pos, force ) );
        }
    }

    r->net_force = total_force;
    r->net_torque = total_torque;
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_blade_element_rotor_init( wp_blade_element_rotor *r )
{
    /* Geometry */
    r->blade_count = 4;
    r->segments_per_blade = 12;
    r->rotor_radius = 6.5f;
    r->chord_length = 0.35f;

    /* Aerodynamics */
    r->air_density = 1.225f;
    r->lift_slope = 5.7f;
    r->drag_coeff = 0.01f;

    /* Rotor state */
    r->rotor_rpm = 0.0f;
    r->collective_pitch = 0.0f;
    r->cyclic_input = wp_vec2f_make( 0.0f, 0.0f );

    /* Derived fields */
    wp_blade_element_rotor_rebuild_derived( r );

    /* Rigid-body state */
    r->position = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    r->orientation = wp_quatf_identity();
    r->velocity = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    r->angular_velocity = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

    /* Outputs */
    r->net_force = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    r->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
}

void wp_blade_element_rotor_rebuild_derived( wp_blade_element_rotor *r )
{
    if( r->segments_per_blade > 0 )
        r->segment_length = r->rotor_radius / (wp_f32)r->segments_per_blade;
    else
        r->segment_length = 0.0f;
}

void wp_blade_element_rotor_fixed_update( wp_blade_element_rotor *r, wp_f32 dt )
{
    (void)dt;
    wp_bero_simulate( r );
}

void wp_blade_element_rotor_set_rotor_state( wp_blade_element_rotor *r, wp_f32 rpm,
                                             wp_f32 collective_pitch, wp_f32 cyclic_x, wp_f32 cyclic_y )
{
    r->rotor_rpm = rpm;
    r->collective_pitch = collective_pitch;
    r->cyclic_input = wp_vec2f_make( cyclic_x, cyclic_y );
}

void wp_blade_element_rotor_set_state( wp_blade_element_rotor *r, wp_vec3f position,
                                       wp_quatf orientation, wp_vec3f velocity,
                                       wp_vec3f angular_velocity )
{
    r->position = position;
    r->orientation = orientation;
    r->velocity = velocity;
    r->angular_velocity = angular_velocity;
}
