/**
 * @file workphone_game_component_vehicle.h
 * @brief C API for a vehicle component attached to a game component.
 *
 * Mirrors the C++ IVehicleComponent / VehicleComponent.  A vehicle component
 * carries its own local and world transforms, a back-pointer to an owning
 * vehicle entity, and callbacks for transform / geometry updates.
 */

#ifndef WORKPHONE_GAME_COMPONENT_VEHICLE_H
#define WORKPHONE_GAME_COMPONENT_VEHICLE_H

#include "workphone_game_component.h"
#include "workphone_game_actor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Vehicle component state
 * ---------------------------------------------------------------------- */

/**
 * @brief Operational states for a vehicle component.
 */
enum wp_vehicle_component_state
{
    WP_VEHICLE_COMPONENT_STATE_AWAKE = 0,
    WP_VEHICLE_COMPONENT_STATE_DESTROYED,
    WP_VEHICLE_COMPONENT_STATE_EDIT,
    WP_VEHICLE_COMPONENT_STATE_PLAY,
    WP_VEHICLE_COMPONENT_STATE_RESET,
    WP_VEHICLE_COMPONENT_STATE_COUNT
};

/* -------------------------------------------------------------------------
 * Vehicle component callbacks
 * ---------------------------------------------------------------------- */

struct wp_vehicle_component;

/**
 * @brief Optional callbacks invoked during vehicle component updates.
 */
typedef struct wp_vehicle_component_callbacks
{
    void ( *on_update_transform )( struct wp_vehicle_component *vc );
    void ( *on_update_body_transform )( struct wp_vehicle_component *vc );
    void ( *on_update_geometry )( struct wp_vehicle_component *vc );
    void ( *on_reset )( struct wp_vehicle_component *vc );
} wp_vehicle_component_callbacks;

/* -------------------------------------------------------------------------
 * Vehicle component structure
 * ---------------------------------------------------------------------- */

/**
 * @brief A vehicle component that extends a base game component with
 *        vehicle-specific transforms, owner pointer and state.
 */
typedef struct wp_vehicle_component
{
    wp_game_component base;

    enum wp_vehicle_component_state vc_state;

    wp_transform3f local_transform;
    wp_transform3f world_transform;

    void *owner;

    wp_vehicle_component_callbacks vc_callbacks;

    void *vc_user_data;
} wp_vehicle_component;

/* ---- Lifecycle -------------------------------------------------------- */

void wp_vehicle_component_init( wp_vehicle_component *vc );
void wp_vehicle_component_destroy( wp_vehicle_component *vc );

/* ---- State ------------------------------------------------------------ */

enum wp_vehicle_component_state wp_vehicle_component_get_state( const wp_vehicle_component *vc );
void wp_vehicle_component_set_state( wp_vehicle_component *vc, enum wp_vehicle_component_state state );

/* ---- Owner ------------------------------------------------------------ */

void *wp_vehicle_component_get_owner( const wp_vehicle_component *vc );
void wp_vehicle_component_set_owner( wp_vehicle_component *vc, void *owner );

/* ---- Transforms ------------------------------------------------------- */

wp_transform3f wp_vehicle_component_get_local_transform( const wp_vehicle_component *vc );
void wp_vehicle_component_set_local_transform( wp_vehicle_component *vc, wp_transform3f t );

wp_transform3f wp_vehicle_component_get_world_transform( const wp_vehicle_component *vc );
void wp_vehicle_component_set_world_transform( wp_vehicle_component *vc, wp_transform3f t );

/* ---- Updates ---------------------------------------------------------- */

void wp_vehicle_component_update_transform( wp_vehicle_component *vc,
                                            const wp_transform3f *parent_world );
void wp_vehicle_component_update_body_transform( wp_vehicle_component *vc,
                                                 const wp_transform3f *parent_world );
void wp_vehicle_component_update_geometry( wp_vehicle_component *vc );

/* ---- Reset ------------------------------------------------------------ */

void wp_vehicle_component_reset( wp_vehicle_component *vc );

/* ---- Callbacks -------------------------------------------------------- */

void wp_vehicle_component_set_callbacks( wp_vehicle_component *vc, wp_vehicle_component_callbacks cbs );

/* ---- User data -------------------------------------------------------- */

void *wp_vehicle_component_get_user_data( const wp_vehicle_component *vc );
void wp_vehicle_component_set_user_data( wp_vehicle_component *vc, void *data );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GAME_COMPONENT_VEHICLE_H */
