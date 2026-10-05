#ifndef WPPhysicsUtil_h__
#define WPPhysicsUtil_h__

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
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace physics
    {

        class WPPhysicsUtil
        {
        public:
            static wp_vec3f toWp( const Vector3<real_Num> &v );
            static Vector3<real_Num> fromWp( wp_vec3f v );
            static wp_quatf toWp( const Quaternion<real_Num> &q );
            static Quaternion<real_Num> fromWp( wp_quatf q );
            static wp_force_mode toWp( ForceModeEnum mode );

            static AABB3<real_Num> transformBounds( const AABB3<real_Num> &bounds,
                                                    const Transform3<real_Num> &transform );

            static Vector3<real_Num> absoluteVector( const Vector3<real_Num> &value );

            static AABB3F toFloatBounds( const AABB3<real_Num> &bounds );

            static AABB3<real_Num> mergeShapeBounds( const Array<SmartPtr<IPhysicsShape3>> &shapes );
        };

    }  // namespace physics
}  // namespace workphone

#endif  // WPPhysicsUtil_h__
