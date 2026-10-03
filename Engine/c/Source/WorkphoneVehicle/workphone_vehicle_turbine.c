/**
 * @file workphone_vehicle_turbine.c
 * @brief C99 implementation of the helicopter turbine engine simulation.
 */

#include "workphone_vehicle_turbine.h"
#include "workphone_math.h"

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/**
 * @brief Advance @p current toward @p target by at most @p max_delta per call.
 *
 * Equivalent to Unity's Mathf.MoveTowards.
 *
 * @param current   Starting value.
 * @param target    Desired end value.
 * @param max_delta Maximum step size (must be >= 0).
 * @return          Value moved toward target without overshoot.
 */
static wp_f32 wp_turbine_move_towards( wp_f32 current, wp_f32 target, wp_f32 max_delta )
{
    wp_f32 delta = target - current;

    if( delta < 0.0f )
        delta = -delta;

    if( delta <= max_delta )
        return target;

    return current + ( ( target > current ) ? max_delta : -max_delta );
}

/* =========================================================================
 * Internal sub-steps (called by wp_turbine_update)
 * ====================================================================== */

/* ==========================
 * FUEL CONTROL
 * ========================== */
static void wp_turbine_update_fuel( wp_turbine *t, wp_f32 dt )
{
    t->fuel_flow = wp_lerpf( t->fuel_flow, t->throttle_input, dt * t->fuel_response );
}

/* ==========================
 * ENGINE SPOOL PHYSICS
 * ========================== */
static void wp_turbine_update_spools( wp_turbine *t, wp_f32 dt )
{
    /* Gas-generator spool (N1) */
    wp_f32 target_n1 = t->fuel_flow * t->max_n1;

    t->n1 = wp_turbine_move_towards( t->n1, target_n1, t->n1_accel * dt );

    /* Power-turbine spool (N2) follows N1 */
    t->n2 = wp_turbine_move_towards( t->n2, t->n1, t->n2_accel * dt );
}

/* ==========================
 * GOVERNOR SYSTEM
 * ========================== */
static void wp_turbine_update_governor( wp_turbine *t, wp_f32 dt )
{
    wp_f32 rpm_error = t->target_rotor_rpm - t->rotor_rpm;
    wp_f32 correction = rpm_error * t->governor_gain;

    t->fuel_flow = wp_clampf( t->fuel_flow + correction * dt, 0.0f, 1.0f );
}

/* ==========================
 * ENGINE TEMPERATURE
 * ========================== */
static void wp_turbine_update_temperature( wp_turbine *t, wp_f32 dt )
{
    wp_f32 heat = t->fuel_flow * 900.0f;

    t->temperature = wp_lerpf( t->temperature, heat, dt );

    if( t->temperature > t->max_temp )
    {
        /* Simulate engine damage / power loss */
        t->fuel_flow *= 0.9f;
    }
}

/* ==========================
 * TORQUE OUTPUT
 * ========================== */
static void wp_turbine_compute_torque( wp_turbine *t )
{
    wp_f32 power_fraction = t->n2 / t->max_n2;

    t->output_torque = power_fraction * t->max_torque;
}

/* =========================================================================
 * Public API
 * ====================================================================== */

void wp_turbine_init( wp_turbine *t )
{
    /* Tunable parameters – mirror the C# defaults */
    t->max_n1 = 100.0f;
    t->max_n2 = 100.0f;
    t->n1_accel = 15.0f;
    t->n2_accel = 8.0f;
    t->fuel_response = 2.0f;
    t->target_rotor_rpm = 400.0f;
    t->governor_gain = 0.015f;
    t->gear_ratio = 25.0f;
    t->max_torque = 7000.0f;
    t->max_temp = 850.0f;

    /* Runtime state */
    t->fuel_flow = 0.0f;
    t->n1 = 0.0f;
    t->n2 = 0.0f;
    t->temperature = 0.0f;
    t->output_torque = 0.0f;
    t->rotor_rpm = 0.0f;
    t->throttle_input = 0.0f;
}

void wp_turbine_update( wp_turbine *t, wp_f32 dt )
{
    wp_turbine_update_fuel( t, dt );
    wp_turbine_update_spools( t, dt );
    wp_turbine_update_governor( t, dt );
    wp_turbine_update_temperature( t, dt );
    wp_turbine_compute_torque( t );
}

void wp_turbine_set_throttle( wp_turbine *t, wp_f32 throttle )
{
    t->throttle_input = wp_clampf( throttle, 0.0f, 1.0f );
}

void wp_turbine_set_rotor_rpm( wp_turbine *t, wp_f32 rpm )
{
    t->rotor_rpm = rpm;
}

wp_f32 wp_turbine_get_output_torque( const wp_turbine *t )
{
    return t->output_torque;
}

wp_f32 wp_turbine_get_n1( const wp_turbine *t )
{
    return t->n1;
}

wp_f32 wp_turbine_get_n2( const wp_turbine *t )
{
    return t->n2;
}

wp_f32 wp_turbine_get_temperature( const wp_turbine *t )
{
    return t->temperature;
}

wp_f32 wp_turbine_get_fuel_flow( const wp_turbine *t )
{
    return t->fuel_flow;
}
