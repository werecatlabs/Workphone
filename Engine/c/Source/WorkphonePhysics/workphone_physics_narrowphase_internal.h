#ifndef WORKPHONE_PHYSICS_NARROWPHASE_INTERNAL_H
#define WORKPHONE_PHYSICS_NARROWPHASE_INTERNAL_H
#include "workphone_physics_narrowphase.h"
void wp_manifold_refresh_materials( wp_contact_manifold *manifold );
wp_s32 wp_triangle_box_overlaps( wp_vec3f a, wp_vec3f b, wp_vec3f c, wp_vec3f half,
                                 wp_f32 tolerance );
wp_f32 wp_segment_box_closest( wp_vec3f start, wp_vec3f end, wp_vec3f half, wp_vec3f *on_segment,
                               wp_vec3f *on_box );
#endif
