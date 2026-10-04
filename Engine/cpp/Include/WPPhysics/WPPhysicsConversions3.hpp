#ifndef WPPHYSICSCONVERSIONS3_HPP
#define WPPHYSICSCONVERSIONS3_HPP

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>

extern "C" {
#include <WorkphonePhysics/workphone_physics.h>
#include <WorkphonePhysics/workphone_physics_material.h>
#include <WorkphonePhysics/workphone_physics_material_registry.h>
#include <WorkphonePhysics/workphone_physics_collisionsettings.h>
#include <WorkphonePhysics/workphone_physics_collisionshape.h>
#include <WorkphonePhysics/workphone_physics_rigidbody.h>
#include <WorkphonePhysics/workphone_physics_scene.h>
}

namespace workphone::physics::detail
{
    inline wp_vec3f toWp( const Vector3<real_Num> &v )
    {
        wp_vec3f r = { static_cast<wp_f32>( v.X() ), static_cast<wp_f32>( v.Y() ),
                       static_cast<wp_f32>( v.Z() ) };
        return r;
    }

    inline Vector3<real_Num> fromWp( wp_vec3f v )
    {
        return Vector3<real_Num>( static_cast<real_Num>( v.x ), static_cast<real_Num>( v.y ),
                                  static_cast<real_Num>( v.z ) );
    }

    inline wp_quatf toWp( const Quaternion<real_Num> &q )
    {
        wp_quatf r = { static_cast<wp_f32>( q.w ), static_cast<wp_f32>( q.x ),
                       static_cast<wp_f32>( q.y ), static_cast<wp_f32>( q.z ) };
        return r;
    }

    inline Quaternion<real_Num> fromWp( wp_quatf q )
    {
        return Quaternion<real_Num>( static_cast<real_Num>( q.w ), static_cast<real_Num>( q.x ),
                                     static_cast<real_Num>( q.y ), static_cast<real_Num>( q.z ) );
    }

    inline wp_force_mode toWp( ForceModeEnum mode )
    {
        switch( mode )
        {
        case ForceModeEnum::Impulse:
            return WORKPHONE_FORCE_MODE_IMPULSE;
        case ForceModeEnum::VelocityChange:
            return WORKPHONE_FORCE_MODE_VELOCITY_CHANGE;
        case ForceModeEnum::Acceleration:
            return WORKPHONE_FORCE_MODE_ACCELERATION;
        default:
            return WORKPHONE_FORCE_MODE_FORCE;
        }
    }
}  // namespace workphone::physics::detail

#endif
