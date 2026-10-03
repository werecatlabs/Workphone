/**
 * @file workphone_vehicle_liftingline_wing.c
 * @brief C99 implementation of a lifting-line wing model.
 */

#include "workphone_vehicle_liftingline_wing.h"
#include "workphone_math.h"
#include "workphone_vector.h"
#include "workphone_quat.h"
#include <string.h>

/* =========================================================================
 * Internal helpers
 * ====================================================================== */

/* Calculate the area of a quadrilateral (A, B, C, D) using Bretschneider's formula */
static wp_f32 wp_wing_quad_area( wp_vec3f a, wp_vec3f b, wp_vec3f c, wp_vec3f d )
{
    wp_f32 ab = wp_vec3f_length( wp_vec3f_sub( b, a ) );
    wp_f32 bc = wp_vec3f_length( wp_vec3f_sub( c, b ) );
    wp_f32 cd = wp_vec3f_length( wp_vec3f_sub( d, c ) );
    wp_f32 da = wp_vec3f_length( wp_vec3f_sub( a, d ) );
    wp_f32 s = ( ab + bc + cd + da ) * 0.5f;
    wp_f32 area = wp_sqrtf( ( s - ab ) * ( s - bc ) * ( s - cd ) * ( s - da ) );
    return area;
}

/* =========================================================================
 * Core simulation
 * ====================================================================== */

void wp_liftingline_wing_init( wp_liftingline_wing *w )
{
    memset( w, 0, sizeof( *w ) );
    w->section_count = WP_LIFTINGLINE_WING_MAX_SECTIONS;
    w->air_density = 1.225f;
    w->cd_override = 0.045f;
    w->lift_slope = 2.0f * WORKPHONE_PI_F;  // Thin airfoil theory
    w->chord_length = 1.0f;
    w->span = 10.0f;
    w->area = w->chord_length * w->span;
}

void wp_liftingline_wing_set_state( wp_liftingline_wing *w, wp_vec3f position, wp_quatf orientation,
                                    wp_vec3f velocity, wp_vec3f angular_velocity )
{
    w->position = position;
    w->orientation = orientation;
    w->velocity = velocity;
    w->angular_velocity = angular_velocity;
}

void wp_liftingline_wing_set_geometry( wp_liftingline_wing *w, wp_f32 span, wp_f32 chord_length,
                                       wp_s32 section_count )
{
    w->span = span;
    w->chord_length = chord_length;
    w->section_count = section_count;
    w->area = span * chord_length;
}

void wp_liftingline_wing_fixed_update( wp_liftingline_wing *w, wp_f32 dt )
{
    /* For each section, compute local wind, angle of attack, lift, drag */
    wp_vec3f right = wp_quatf_rotate_vec3( w->orientation, wp_vec3f_make( 1.0f, 0.0f, 0.0f ) );
    wp_vec3f forward = wp_quatf_rotate_vec3( w->orientation, wp_vec3f_make( 0.0f, 0.0f, 1.0f ) );
    wp_vec3f up = wp_quatf_rotate_vec3( w->orientation, wp_vec3f_make( 0.0f, 1.0f, 0.0f ) );

    wp_vec3f total_lift = wp_vec3f_make( 0.0f, 0.0f, 0.0f );
    wp_vec3f total_drag = wp_vec3f_make( 0.0f, 0.0f, 0.0f );

    for( wp_s32 i = 0; i < w->section_count; ++i )
    {
        /* Section center position along the span */
        wp_f32 frac = ( (wp_f32)i + 0.5f ) / (wp_f32)w->section_count;
        wp_vec3f section_pos =
            wp_vec3f_add( w->position, wp_vec3f_scale( right, ( frac - 0.5f ) * w->span ) );

        /* Local wind at section (linear + angular) */
        wp_vec3f offset = wp_vec3f_sub( section_pos, w->position );
        wp_vec3f local_wind = wp_vec3f_add( w->velocity, wp_vec3f_cross( w->angular_velocity, offset ) );

        /* Angle of attack: between local wind and chord line (forward) */
        wp_vec3f wind_dir = wp_vec3f_normalize( local_wind );
        wp_f32 aoa =
            wp_rad2degf( wp_atan2f( wp_vec3f_dot( up, wind_dir ), wp_vec3f_dot( forward, wind_dir ) ) );

        /* Lift and drag coefficients */
        wp_f32 cl = w->lift_slope * ( aoa * WORKPHONE_DEG2RAD_F );
        wp_f32 cd = w->cd_override;

        /* Dynamic pressure */
        wp_f32 speed = wp_vec3f_length( local_wind );
        wp_f32 q = 0.5f * w->air_density * speed * speed;

        /* Section area */
        wp_f32 section_area = w->chord_length * ( w->span / w->section_count );

        /* Forces */
        wp_f32 lift_mag = cl * q * section_area;
        wp_f32 drag_mag = cd * q * section_area;

        /* Directions */
        wp_vec3f lift_dir = wp_vec3f_normalize( wp_vec3f_cross( wind_dir, right ) );
        wp_vec3f drag_dir = wp_vec3f_negate( wind_dir );

        wp_vec3f lift = wp_vec3f_scale( lift_dir, lift_mag );
        wp_vec3f drag = wp_vec3f_scale( drag_dir, drag_mag );

        total_lift = wp_vec3f_add( total_lift, lift );
        total_drag = wp_vec3f_add( total_drag, drag );
    }

    w->net_force = wp_vec3f_add( total_lift, total_drag );
    w->net_torque = wp_vec3f_make( 0.0f, 0.0f, 0.0f );  // Extend as needed
}
