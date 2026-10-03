/**
 * @file workphone_vehicle_softbody_controller.h
 * @brief C89 API for a simple soft-body car controller.
 *
 * Reads normalised driver inputs, derives the chassis frame from four corner
 * nodes, and steps each wheel.
 */

#ifndef WORKPHONE_VEHICLE_SOFTBODY_CONTROLLER_H
#define WORKPHONE_VEHICLE_SOFTBODY_CONTROLLER_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vector.h"
#include "workphone_vehicle_softbody.h"
#include "workphone_vehicle_softbody_wheel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple controller that drives a set of wheels attached to a
 *        soft-body vehicle.
 */
typedef struct wp_softbody_car_controller
{
    wp_softbody_vehicle *vehicle;
    wp_softbody_wheel *wheels;
    wp_s32 wheel_count;
    wp_s32 owns_wheels;

    wp_s32 front_left_node;
    wp_s32 front_right_node;
    wp_s32 rear_left_node;
    wp_s32 rear_right_node;
} wp_softbody_car_controller;

/**
 * @brief Initialise a controller with no vehicle or wheels.
 */
void wp_softbody_car_controller_init( wp_softbody_car_controller *controller );

/**
 * @brief Destroy a controller, freeing any wheels it allocated internally.
 */
void wp_softbody_car_controller_destroy( wp_softbody_car_controller *controller );

/**
 * @brief Attach the controller to a vehicle.
 */
void wp_softbody_car_controller_set_vehicle( wp_softbody_car_controller *controller,
                                              wp_softbody_vehicle *vehicle );

/**
 * @brief Use a caller-managed wheel array.
 */
void wp_softbody_car_controller_set_wheels( wp_softbody_car_controller *controller,
                                             wp_softbody_wheel *wheels, wp_s32 wheel_count );

/**
 * @brief Allocate and configure the default four-wheel layout.
 */
void wp_softbody_car_controller_set_default_wheels( wp_softbody_car_controller *controller );

/**
 * @brief Set the corner-node indices used to derive the chassis frame.
 */
void wp_softbody_car_controller_set_frame_nodes( wp_softbody_car_controller *controller,
                                                wp_s32 front_left, wp_s32 front_right,
                                                wp_s32 rear_left, wp_s32 rear_right );

/**
 * @brief Step the controller and all wheels.
 */
void wp_softbody_car_controller_step( wp_softbody_car_controller *controller,
                                       wp_f32 throttle, wp_f32 brake, wp_f32 steering,
                                       wp_f32 delta_time,
                                       const wp_softbody_vehicle_callbacks *callbacks );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_SOFTBODY_CONTROLLER_H */
