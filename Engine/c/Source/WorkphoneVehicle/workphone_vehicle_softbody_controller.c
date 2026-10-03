/**
 * @file workphone_vehicle_softbody_controller.c
 * @brief C89 implementation of the soft-body car controller.
 */

#include "workphone_vehicle_softbody_controller.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include <string.h>
#include <stdlib.h>

/* =========================================================================
 * Default constants
 * ====================================================================== */

#define WP_SOFTBODY_CAR_DEFAULT_FRONT_LEFT_NODE 8
#define WP_SOFTBODY_CAR_DEFAULT_FRONT_RIGHT_NODE 10
#define WP_SOFTBODY_CAR_DEFAULT_REAR_LEFT_NODE 0
#define WP_SOFTBODY_CAR_DEFAULT_REAR_RIGHT_NODE 2

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_softbody_car_controller_init( wp_softbody_car_controller *controller )
{
    if( controller == NULL )
    {
        return;
    }

    memset( controller, 0, sizeof( *controller ) );

    controller->front_left_node = WP_SOFTBODY_CAR_DEFAULT_FRONT_LEFT_NODE;
    controller->front_right_node = WP_SOFTBODY_CAR_DEFAULT_FRONT_RIGHT_NODE;
    controller->rear_left_node = WP_SOFTBODY_CAR_DEFAULT_REAR_LEFT_NODE;
    controller->rear_right_node = WP_SOFTBODY_CAR_DEFAULT_REAR_RIGHT_NODE;
}

void wp_softbody_car_controller_destroy( wp_softbody_car_controller *controller )
{
    if( controller == NULL )
    {
        return;
    }

    if( controller->owns_wheels && controller->wheels != NULL )
    {
        free( controller->wheels );
    }

    controller->wheels = NULL;
    controller->wheel_count = 0;
    controller->owns_wheels = wp_false;
}

void wp_softbody_car_controller_set_vehicle( wp_softbody_car_controller *controller,
                                              wp_softbody_vehicle *vehicle )
{
    if( controller == NULL )
    {
        return;
    }

    controller->vehicle = vehicle;
}

void wp_softbody_car_controller_set_wheels( wp_softbody_car_controller *controller,
                                             wp_softbody_wheel *wheels, wp_s32 wheel_count )
{
    if( controller == NULL )
    {
        return;
    }

    if( controller->owns_wheels && controller->wheels != NULL )
    {
        free( controller->wheels );
    }

    controller->wheels = wheels;
    controller->wheel_count = wheel_count;
    controller->owns_wheels = wp_false;
}

void wp_softbody_car_controller_set_default_wheels( wp_softbody_car_controller *controller )
{
    wp_softbody_wheel *wheels;

    if( controller == NULL )
    {
        return;
    }

    if( controller->owns_wheels && controller->wheels != NULL )
    {
        free( controller->wheels );
    }

    wheels = ( wp_softbody_wheel * )malloc( 4 * sizeof( wp_softbody_wheel ) );
    if( wheels == NULL )
    {
        controller->wheels = NULL;
        controller->wheel_count = 0;
        controller->owns_wheels = wp_false;
        return;
    }

    wp_softbody_wheel_init( &wheels[0] );
    wheels[0].node_index = controller->front_left_node;
    wheels[0].steering = wp_true;
    wheels[0].driven = wp_true;

    wp_softbody_wheel_init( &wheels[1] );
    wheels[1].node_index = controller->front_right_node;
    wheels[1].steering = wp_true;
    wheels[1].driven = wp_true;

    wp_softbody_wheel_init( &wheels[2] );
    wheels[2].node_index = controller->rear_left_node;
    wheels[2].steering = wp_false;
    wheels[2].driven = wp_false;

    wp_softbody_wheel_init( &wheels[3] );
    wheels[3].node_index = controller->rear_right_node;
    wheels[3].steering = wp_false;
    wheels[3].driven = wp_false;

    controller->wheels = wheels;
    controller->wheel_count = 4;
    controller->owns_wheels = wp_true;
}

void wp_softbody_car_controller_set_frame_nodes( wp_softbody_car_controller *controller,
                                                wp_s32 front_left, wp_s32 front_right,
                                                wp_s32 rear_left, wp_s32 rear_right )
{
    if( controller == NULL )
    {
        return;
    }

    controller->front_left_node = front_left;
    controller->front_right_node = front_right;
    controller->rear_left_node = rear_left;
    controller->rear_right_node = rear_right;
}

void wp_softbody_car_controller_step( wp_softbody_car_controller *controller,
                                       wp_f32 throttle, wp_f32 brake, wp_f32 steering,
                                       wp_f32 delta_time,
                                       const wp_softbody_vehicle_callbacks *callbacks )
{
    wp_vec3f centre;
    wp_vec3f right;
    wp_vec3f up;
    wp_vec3f forward;
    wp_s32 i;

    if( controller == NULL || controller->vehicle == NULL )
    {
        return;
    }

    if( !wp_softbody_vehicle_is_initialized( controller->vehicle ) )
    {
        return;
    }

    throttle = wp_clampf( throttle, -1.0f, 1.0f );
    steering = wp_clampf( steering, -1.0f, 1.0f );
    brake = wp_clampf( brake, 0.0f, 1.0f );

    wp_softbody_vehicle_get_frame( controller->vehicle,
                                    controller->front_left_node,
                                    controller->front_right_node,
                                    controller->rear_left_node,
                                    controller->rear_right_node,
                                    &centre, &right, &up, &forward );

    for( i = 0; i < controller->wheel_count; ++i )
    {
        wp_softbody_wheel_step( &controller->wheels[i],
                                controller->vehicle,
                                right, up, forward,
                                throttle, brake, steering,
                                delta_time,
                                callbacks );
    }
}
