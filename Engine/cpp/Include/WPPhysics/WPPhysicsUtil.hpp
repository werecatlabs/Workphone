#ifndef WPPhysicsUtil_h__
#define WPPhysicsUtil_h__

#include <WPPhysics/WPPhysicsConversions3.hpp>
#include <WPPhysics/WPPhysicsVehicleWheel.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>

namespace workphone
{
    namespace physics
    {

        class WPPhysicsUtil
        {
        public:
            static AABB3<real_Num> transformBounds( const AABB3<real_Num> &bounds,
                                             const Transform3<real_Num> &transform );

            static Vector3<real_Num> absoluteVector( const Vector3<real_Num> &value );
        };

    }  // namespace physics
}  // namespace workphone

#endif  // WPPhysicsUtil_h__
