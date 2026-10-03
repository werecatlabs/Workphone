/**
 * @file workphone_game_component_vehicle.c
 * @brief Implementation of the C vehicle component API.
 */

#include "workphone_game_component_vehicle.h"

#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

static wp_transform3f wp_transform3f_identity( void )
{
    wp_transform3f t;
    memset( &t, 0, sizeof( t ) );
    t.orientation.w = 1.0f;
    t.scale.x = 1.0f;
    t.scale.y = 1.0f;
    t.scale.z = 1.0f;
    return t;
}

/**
 * @brief Combine a parent world transform with a child local transform.
 *
 * This is the C equivalent of Transform3::transformFromParent.
 * Position is rotated by the parent orientation, scaled by the parent scale,
 * then offset by the parent position.  Orientation is the concatenation of
 * both quaternions.  Scale is component-wise multiplication.
 */
static wp_transform3f wp_transform3f_combine( const wp_transform3f *parent, const wp_transform3f *local )
{
    wp_transform3f out;
    wp_quatf pq, lq, rq;
    wp_f32 lx, ly, lz;
    wp_f32 rx, ry, rz;

    /* Rotate local position by parent orientation */
    pq = parent->orientation;
    lx = local->position.x;
    ly = local->position.y;
    lz = local->position.z;

    /* q * v * q^-1  (optimised for unit quaternion) */
    {
        wp_f32 tx = 2.0f * ( pq.y * lz - pq.z * ly );
        wp_f32 ty = 2.0f * ( pq.z * lx - pq.x * lz );
        wp_f32 tz = 2.0f * ( pq.x * ly - pq.y * lx );

        rx = lx + pq.w * tx + ( pq.y * tz - pq.z * ty );
        ry = ly + pq.w * ty + ( pq.z * tx - pq.x * tz );
        rz = lz + pq.w * tz + ( pq.x * ty - pq.y * tx );
    }

    /* Scale rotated position by parent scale, then add parent position */
    out.position.x = parent->position.x + rx * parent->scale.x;
    out.position.y = parent->position.y + ry * parent->scale.y;
    out.position.z = parent->position.z + rz * parent->scale.z;

    /* Orientation: parent * local */
    lq = local->orientation;
    rq.w = pq.w * lq.w - pq.x * lq.x - pq.y * lq.y - pq.z * lq.z;
    rq.x = pq.w * lq.x + pq.x * lq.w + pq.y * lq.z - pq.z * lq.y;
    rq.y = pq.w * lq.y - pq.x * lq.z + pq.y * lq.w + pq.z * lq.x;
    rq.z = pq.w * lq.z + pq.x * lq.y - pq.y * lq.x + pq.z * lq.w;
    out.orientation = rq;

    /* Scale: component-wise multiply */
    out.scale.x = parent->scale.x * local->scale.x;
    out.scale.y = parent->scale.y * local->scale.y;
    out.scale.z = parent->scale.z * local->scale.z;

    return out;
}

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

void wp_vehicle_component_init( wp_vehicle_component *vc )
{
    memset( vc, 0, sizeof( *vc ) );
    wp_game_component_init_with_type( &vc->base, WP_COMPONENT_TYPE_VEHICLE );

    vc->vc_state = WP_VEHICLE_COMPONENT_STATE_AWAKE;
    vc->local_transform = wp_transform3f_identity();
    vc->world_transform = wp_transform3f_identity();
    vc->owner = NULL;
    memset( &vc->vc_callbacks, 0, sizeof( vc->vc_callbacks ) );
    vc->vc_user_data = NULL;
}

void wp_vehicle_component_destroy( wp_vehicle_component *vc )
{
    vc->owner = NULL;
    vc->vc_user_data = NULL;
    vc->vc_state = WP_VEHICLE_COMPONENT_STATE_DESTROYED;
    memset( &vc->vc_callbacks, 0, sizeof( vc->vc_callbacks ) );

    wp_game_component_destroy( &vc->base );
}

/* =========================================================================
 * State
 * ====================================================================== */

enum wp_vehicle_component_state wp_vehicle_component_get_state( const wp_vehicle_component *vc )
{
    return vc->vc_state;
}

void wp_vehicle_component_set_state( wp_vehicle_component *vc, enum wp_vehicle_component_state state )
{
    vc->vc_state = state;
}

/* =========================================================================
 * Owner
 * ====================================================================== */

void *wp_vehicle_component_get_owner( const wp_vehicle_component *vc )
{
    return vc->owner;
}

void wp_vehicle_component_set_owner( wp_vehicle_component *vc, void *owner )
{
    vc->owner = owner;
}

/* =========================================================================
 * Transforms
 * ====================================================================== */

wp_transform3f wp_vehicle_component_get_local_transform( const wp_vehicle_component *vc )
{
    return vc->local_transform;
}

void wp_vehicle_component_set_local_transform( wp_vehicle_component *vc, wp_transform3f t )
{
    vc->local_transform = t;
}

wp_transform3f wp_vehicle_component_get_world_transform( const wp_vehicle_component *vc )
{
    return vc->world_transform;
}

void wp_vehicle_component_set_world_transform( wp_vehicle_component *vc, wp_transform3f t )
{
    vc->world_transform = t;
}

/* =========================================================================
 * Updates
 * ====================================================================== */

void wp_vehicle_component_update_transform( wp_vehicle_component *vc,
                                            const wp_transform3f *parent_world )
{
    if( parent_world )
    {
        vc->world_transform = wp_transform3f_combine( parent_world, &vc->local_transform );
    }

    if( vc->vc_callbacks.on_update_transform )
        vc->vc_callbacks.on_update_transform( vc );
}

void wp_vehicle_component_update_body_transform( wp_vehicle_component *vc,
                                                 const wp_transform3f *parent_world )
{
    /* Compute but do not store — mirrors C++ updateBodyTransform(). */
    if( parent_world )
    {
        (void)wp_transform3f_combine( parent_world, &vc->local_transform );
    }

    if( vc->vc_callbacks.on_update_body_transform )
        vc->vc_callbacks.on_update_body_transform( vc );
}

void wp_vehicle_component_update_geometry( wp_vehicle_component *vc )
{
    if( vc->vc_callbacks.on_update_geometry )
        vc->vc_callbacks.on_update_geometry( vc );
}

/* =========================================================================
 * Reset
 * ====================================================================== */

void wp_vehicle_component_reset( wp_vehicle_component *vc )
{
    if( vc->vc_callbacks.on_reset )
        vc->vc_callbacks.on_reset( vc );
}

/* =========================================================================
 * Callbacks
 * ====================================================================== */

void wp_vehicle_component_set_callbacks( wp_vehicle_component *vc, wp_vehicle_component_callbacks cbs )
{
    vc->vc_callbacks = cbs;
}

/* =========================================================================
 * User data
 * ====================================================================== */

void *wp_vehicle_component_get_user_data( const wp_vehicle_component *vc )
{
    return vc->vc_user_data;
}

void wp_vehicle_component_set_user_data( wp_vehicle_component *vc, void *data )
{
    vc->vc_user_data = data;
}
