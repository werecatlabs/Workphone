/**
 * @file workphone_physics_joint.c
 * @brief Joint convenience API implementation.
 */

#include "workphone_physics_joint.h"

wp_joint *wp_joint_create( wp_joint_type type, wp_rigidbody *body_a, wp_rigidbody *body_b )
{
    wp_joint *joint = wp_constraint_create( type );
    if( joint )
    {
        wp_constraint_set_body_a( joint, body_a );
        wp_constraint_set_body_b( joint, body_b );
    }
    return joint;
}

void wp_joint_destroy( wp_joint *joint )
{
    wp_constraint_destroy( joint );
}

wp_joint_type wp_joint_get_type( const wp_joint *joint )
{
    return wp_constraint_get_type( joint );
}

wp_rigidbody *wp_joint_get_body_a( const wp_joint *joint )
{
    return wp_constraint_get_body_a( joint );
}

wp_rigidbody *wp_joint_get_body_b( const wp_joint *joint )
{
    return wp_constraint_get_body_b( joint );
}

void wp_joint_set_bodies( wp_joint *joint, wp_rigidbody *body_a, wp_rigidbody *body_b )
{
    wp_constraint_set_body_a( joint, body_a );
    wp_constraint_set_body_b( joint, body_b );
}

wp_vec3f wp_joint_get_local_position( const wp_joint *joint, wp_s32 actor )
{
    return wp_constraint_get_local_position( joint, actor );
}

void wp_joint_set_local_position( wp_joint *joint, wp_s32 actor, wp_vec3f position )
{
    wp_constraint_set_local_position( joint, actor, position );
}

wp_quatf wp_joint_get_local_orientation( const wp_joint *joint, wp_s32 actor )
{
    return wp_constraint_get_local_orientation( joint, actor );
}

void wp_joint_set_local_orientation( wp_joint *joint, wp_s32 actor, wp_quatf orientation )
{
    wp_constraint_set_local_orientation( joint, actor, orientation );
}

wp_u32 wp_joint_get_flags( const wp_joint *joint )
{
    return wp_constraint_get_flags( joint );
}

void wp_joint_set_flags( wp_joint *joint, wp_u32 flags )
{
    wp_constraint_set_flags( joint, flags );
}
