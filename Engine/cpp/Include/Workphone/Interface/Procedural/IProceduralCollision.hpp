#ifndef __IProceduralCollision_h__
#define __IProceduralCollision_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Triangle3.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <Workphone/Math/Line2.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IProceduralCollision : public ISharedObject
        {
        public:
            ~IProceduralCollision() override = default;

            virtual bool rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &dir,
                                  Vector3<real_Num> &outHitPos ) = 0;
            virtual bool rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &dir,
                                  Vector3<real_Num> &outHitPos, Triangle3<real_Num> &outTriangle ) = 0;
            virtual bool rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &dir,
                                  Vector3<real_Num> &outHitPos, Triangle3<real_Num> &outTriangle,
                                  const Array<String> &filter ) = 0;

            virtual bool intersects( const SmartPtr<IProceduralCity> &city,
                                     const Sphere3<real_Num> &sphere ) = 0;

            //
            // road functions
            //
            virtual bool intersects( const SmartPtr<IRoad> &srcRoad,
                                     const Sphere3<real_Num> &sphere ) = 0;
            virtual bool intersects( const SmartPtr<IRoad> &srcRoad, const Sphere3<real_Num> &sphere,
                                     SmartPtr<IRoadNode> &collidingNode ) const = 0;
            virtual bool intersects( const SmartPtr<IRoad> &srcRoad, const AABB3<real_Num> &box ) = 0;
            virtual bool intersects( const SmartPtr<IRoad> &srcRoad,
                                     const Polygon2<real_Num> &polygon ) = 0;
            virtual bool intersects( const SmartPtr<IRoad> &srcRoad, const Line2<real_Num> &line,
                                     Vector2<real_Num> &intersectionPoint, bool checkStart ) = 0;
            virtual bool intersects( const SmartPtr<IRoad> &srcRoad, SmartPtr<IRoad> road,
                                     Vector3<real_Num> &intersectionPoint, bool checkStart ) const = 0;
            virtual bool intersects( const SmartPtr<IRoad> &srcRoad, SmartPtr<IRoad> road,
                                     Vector3<real_Num> &intersectionPoint,
                                     Array<SmartPtr<IRoadNode>> &nodes, bool checkStart ) const = 0;
            virtual bool intersects( const SmartPtr<IRoad> &srcRoad, SmartPtr<IRoadNode> node,
                                     Array<SmartPtr<IRoadNode>> &nodes, bool checkStart ) const = 0;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ICollisionManager_h__
