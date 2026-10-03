/**
 * @file workphone_physics_contact.h
 * @brief Helpers for public contact manifold value types.
 */

#ifndef WORKPHONE_PHYSICS_CONTACT_H
#define WORKPHONE_PHYSICS_CONTACT_H

#include "workphone_physics_narrowphase.h"

#ifdef __cplusplus
extern "C" {
#endif

void wp_contact_point_reset( wp_contact_point *point );
void wp_contact_manifold_reset( wp_contact_manifold *manifold );
wp_s32 wp_contact_manifold_add_point( wp_contact_manifold *manifold, wp_contact_point point );

wp_contact_point *wp_contact_manifold_get_point( wp_contact_manifold *manifold, wp_s32 index );
const wp_contact_point *wp_contact_manifold_get_point_const( const wp_contact_manifold *manifold,
                                                             wp_s32 index );

wp_f32 wp_contact_manifold_get_max_penetration( const wp_contact_manifold *manifold );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PHYSICS_CONTACT_H */
