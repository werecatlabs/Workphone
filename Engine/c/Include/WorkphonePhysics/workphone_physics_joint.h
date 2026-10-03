/**
 * @file workphone_physics_joint.h
 * @brief Joint convenience API backed by wp_constraint.
 */

#ifndef WORKPHONE_PHYSICS_JOINT_H
#define WORKPHONE_PHYSICS_JOINT_H

#include "workphone_physics_constraint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef wp_constraint wp_joint;
typedef wp_constraint_type wp_joint_type;

#define WORKPHONE_JOINT_FIXED WORKPHONE_CONSTRAINT_FIXED
#define WORKPHONE_JOINT_D6 WORKPHONE_CONSTRAINT_D6

wp_joint *wp_joint_create( wp_joint_type type, wp_rigidbody *body_a, wp_rigidbody *body_b );
void wp_joint_destroy( wp_joint *joint );
wp_joint_type wp_joint_get_type( const wp_joint *joint );

wp_rigidbody *wp_joint_get_body_a( const wp_joint *joint );
wp_rigidbody *wp_joint_get_body_b( const wp_joint *joint );
void wp_joint_set_bodies( wp_joint *joint, wp_rigidbody *body_a, wp_rigidbody *body_b );

wp_vec3f wp_joint_get_local_position( const wp_joint *joint, wp_s32 actor );
void wp_joint_set_local_position( wp_joint *joint, wp_s32 actor, wp_vec3f position );
wp_quatf wp_joint_get_local_orientation( const wp_joint *joint, wp_s32 actor );
void wp_joint_set_local_orientation( wp_joint *joint, wp_s32 actor, wp_quatf orientation );

wp_u32 wp_joint_get_flags( const wp_joint *joint );
void wp_joint_set_flags( wp_joint *joint, wp_u32 flags );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_PHYSICS_JOINT_H */
