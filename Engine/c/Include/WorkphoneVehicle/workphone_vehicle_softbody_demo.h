/**
 * @file workphone_vehicle_softbody_demo.h
 * @brief C89 API for building a demonstration soft-body vehicle chassis.
 *
 * Procedurally generates a box-shaped lattice of nodes and beams matching
 * the original Unity SoftBodyDemoCar implementation.
 */

#ifndef WORKPHONE_VEHICLE_SOFTBODY_DEMO_H
#define WORKPHONE_VEHICLE_SOFTBODY_DEMO_H

#include "workphone_config.h"
#include "workphone_types.h"
#include "workphone_vehicle_softbody.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parameters for the demonstration chassis.
 */
typedef struct wp_softbody_demo_params
{
    wp_f32 length;
    wp_f32 width;
    wp_f32 chassis_height;
    wp_f32 total_mass;
    wp_f32 node_radius;
    wp_f32 beam_stiffness;
    wp_f32 beam_damping;
    wp_f32 yield_strain;
    wp_f32 plasticity;
    wp_f32 break_strain;
} wp_softbody_demo_params;

/**
 * @brief State for the demo-car builder.
 */
typedef struct wp_softbody_demo_car
{
    wp_softbody_vehicle *vehicle;
    wp_softbody_demo_params params;
} wp_softbody_demo_car;

/**
 * @brief Initialise a demo-car builder with default dimensions and
 *        material properties.
 */
void wp_softbody_demo_car_init( wp_softbody_demo_car *demo, wp_softbody_vehicle *vehicle );

/**
 * @brief Set build parameters.
 */
void wp_softbody_demo_car_set_params( wp_softbody_demo_car *demo,
                                       const wp_softbody_demo_params *params );

/**
 * @brief Build the node/beam definitions and apply them to the vehicle.
 */
void wp_softbody_demo_car_build_chassis( wp_softbody_demo_car *demo );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_VEHICLE_SOFTBODY_DEMO_H */
