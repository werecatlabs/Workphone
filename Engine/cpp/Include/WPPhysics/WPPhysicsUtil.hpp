#ifndef WPPhysicsUtil_h__
#define WPPhysicsUtil_h__

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Transform3.hpp>

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

            static AABB3F toFloatBounds( const AABB3<real_Num> &bounds );

            static AABB3<real_Num> mergeShapeBounds( const Array<SmartPtr<IPhysicsShape3>> &shapes );
        };

    }  // namespace physics
}  // namespace workphone

#endif  // WPPhysicsUtil_h__
