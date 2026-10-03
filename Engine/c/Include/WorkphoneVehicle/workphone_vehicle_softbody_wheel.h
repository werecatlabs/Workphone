/**
 * @file workphone_vehicle_softbody_wheel.h
 * @brief C89 API for a soft-body vehicle wheel.
 *
 * Each wheel is attached to one soft-body node and performs a downward
 * suspension raycast, computes spring/damper and tyre friction forces, and
 * writes the resulting chassis force back to the node.
 */

#ifndef WORKPHONE_VEHICLE_SOFTBODY_WHEEL_H
#define WORKPHONE_VEHICLE_SOFTBODY_WHEEL_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_vehicle_softbody.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runtime state for one deformable-vehicle wheel.
 */
typedef struct wp_softbody_wheel
{
    /* Identification. */
    wp_c8 name[WP_SOFTBODY_VEHICLE_MAX_NAME];
    wp_s32 node_index;
    wp_s32 steering;
    wp_s32 driven;

    /* Suspension. */
    wp_f32 suspension_top_offset;
    wp_f32 rest_length;
    wp_f32 travel;
    wp_f32 spring_rate;
    wp_f32 damper_rate;

    /* Tyre. */
    wp_f32 wheel_radius;
    wp_f32 wheel_inertia;
    wp_f32 longitudinal_stiffness;
    wp_f32 cornering_stiffness;
    wp_f32 friction_coefficient;
    wp_f32 rolling_resistance;

    /* Controls. */
    wp_f32 maximum_steering_angle;
    wp_f32 maximum_drive_torque;
    wp_f32 maximum_brake_torque;

    /* Collision filtering for ground probes. */
    wp_u32 ground_mask;

    /* Runtime outputs. */
    wp_s32 grounded;
    wp_vec3f contact_point;
    wp_f32 angular_velocity;

    /* Internal state. */
    wp_s32 has_previous_length;
    wp_f32 previous_length;
} wp_softbody_wheel;

/**
 * @brief Initialise a wheel with default parameters matching a small car.
 */
void wp_softbody_wheel_init( wp_softbody_wheel *wheel );

/**
 * @brief Perform one fixed-rate wheel step.
 *
 * @param wheel          Wheel state.
 * @param vehicle        Owning soft-body vehicle.
 * @param chassis_right  World-space right axis of the chassis.
 * @param chassis_up     World-space up axis of the chassis.
 * @param chassis_forward World-space forward axis of the chassis.
 * @param throttle       Normalised throttle demand [-1, 1].
 * @param brake          Normalised brake demand [0, 1].
 * @param steering_input Normalised steering demand [-1, 1].
 * @param delta_time     Fixed time step in seconds.
 * @param callbacks      Physics callbacks for raycasting and ground-body
 *                       interaction. May be NULL to treat the ground as static.
 */
void wp_softbody_wheel_step( wp_softbody_wheel *wheel,
                             wp_softbody_vehicle *vehicle,
                             wp_vec3f chassis_right,
                             wp_vec3f chassis_up,
                             wp_vec3f chassis_forward,
                             wp_f32 throttle,
                             wp_f32 brake,
                             wp_f32 steering_input,
                             wp_f32 delta_time,
                             const wp_softbody_vehicle_callbacks *callbacks );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_SOFTBODY_WHEEL_H */
