#ifndef WP_NARROWPHASE_REFERENCE_H
#define WP_NARROWPHASE_REFERENCE_H
#include "workphone_physics_narrowphase.h"
#include "workphone_physics_rigidbody.h"
wp_s32 reference_box_box( wp_vec3f, wp_quatf, wp_vec3f, wp_vec3f, wp_quatf, wp_vec3f, wp_f32,
                          wp_contact_manifold * );
wp_s32 reference_triangle_box( wp_vec3f, wp_vec3f, wp_vec3f, wp_vec3f, wp_f32 );
float reference_segment_box( wp_vec3f, wp_vec3f, wp_vec3f );
#endif
