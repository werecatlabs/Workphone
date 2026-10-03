/**
 * @file wp_quat.h
 * @brief C API for quaternion helpers (wp_f32 and wp_f64 variants).
 */

#ifndef WORKPHONE_QUAT_H
#define WORKPHONE_QUAT_H

#include "workphone_vector.h"
#include "workphone_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Types
 * ---------------------------------------------------------------------- */

typedef struct
{
    wp_f32 w, x, y, z;
} wp_quatf;

typedef struct
{
    wp_f64 w, x, y, z;
} wp_quatd;

/* -------------------------------------------------------------------------
 * wp_quatf (wp_f32) API
 * ---------------------------------------------------------------------- */

wp_quatf wp_quatf_make( wp_f32 w, wp_f32 x, wp_f32 y, wp_f32 z );
wp_quatf wp_quatf_identity( void );
wp_f32 wp_quatf_dot( wp_quatf a, wp_quatf b );
wp_f32 wp_quatf_length( wp_quatf q );
wp_quatf wp_quatf_normalize( wp_quatf q );
wp_quatf wp_quatf_conjugate( wp_quatf q );
wp_quatf wp_quatf_mul( wp_quatf a, wp_quatf b );
wp_vec3f wp_quatf_rotate_vec3( wp_quatf q, wp_vec3f v );
wp_quatf wp_quatf_from_axis_angle( wp_vec3f axis, wp_f32 angle_rad );
void wp_quatf_to_axis_angle( wp_quatf q, wp_vec3f *out_axis, wp_f32 *out_angle );
wp_quatf wp_quatf_slerp( wp_quatf a, wp_quatf b, wp_f32 t );

/* -------------------------------------------------------------------------
 * wp_quatd (wp_f64) API
 * ---------------------------------------------------------------------- */

wp_quatd wp_quatd_make( wp_f64 w, wp_f64 x, wp_f64 y, wp_f64 z );
wp_quatd wp_quatd_identity( void );
wp_f64 wp_quatd_dot( wp_quatd a, wp_quatd b );
wp_f64 wp_quatd_length( wp_quatd q );
wp_quatd wp_quatd_normalize( wp_quatd q );
wp_quatd wp_quatd_conjugate( wp_quatd q );
wp_quatd wp_quatd_mul( wp_quatd a, wp_quatd b );
wp_vec3d wp_quatd_rotate_vec3( wp_quatd q, wp_vec3d v );
wp_quatd wp_quatd_from_axis_angle( wp_vec3d axis, wp_f64 angle_rad );
void wp_quatd_to_axis_angle( wp_quatd q, wp_vec3d *out_axis, wp_f64 *out_angle );
wp_quatd wp_quatd_slerp( wp_quatd a, wp_quatd b, wp_f64 t );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_QUAT_H */
