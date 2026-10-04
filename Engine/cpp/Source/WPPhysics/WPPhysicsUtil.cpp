#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsUtil.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{

workphone::Vector3<workphone::real_Num> WPPhysicsUtil::absoluteVector(
        const Vector3<real_Num> &value )
    {
        return Vector3<real_Num>( Math<real_Num>::Abs( value.X() ), Math<real_Num>::Abs( value.Y() ),
                                  Math<real_Num>::Abs( value.Z() ) );
    }

    workphone::AABB3<workphone::real_Num> WPPhysicsUtil::transformBounds(
        const AABB3<real_Num> &bounds, const Transform3<real_Num> &transform )
    {
        if( bounds.isInfinite() )
        {
            auto result = AABB3<real_Num>();
            result.setInfinite();
            return result;
        }
        if( bounds.isNull() )
        {
            auto result = AABB3<real_Num>();
            result.setNull();
            return result;
        }

        Vector3<real_Num> corners[8];
        bounds.getEdges( corners );
        auto result = AABB3<real_Num>( transform.transformPoint( corners[0] ) );
        for( u32 i = 1; i < 8; ++i )
        {
            result.merge( transform.transformPoint( corners[i] ) );
        }
        return result;
    }

}
