#ifndef WPPHYSICSBOUNDS3_HPP
#define WPPHYSICSBOUNDS3_HPP

#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone::physics::detail
{
    inline AABB3F toFloatBounds( const AABB3<real_Num> &bounds )
    {
        auto result = AABB3F();
        if( bounds.isNull() )
        {
            result.setNull();
            return result;
        }
        if( bounds.isInfinite() )
        {
            result.setInfinite();
            return result;
        }

        const auto minimum = bounds.getMinimum();
        const auto maximum = bounds.getMaximum();
        return AABB3F( Vector3F( static_cast<f32>( minimum.X() ), static_cast<f32>( minimum.Y() ),
                                 static_cast<f32>( minimum.Z() ) ),
                       Vector3F( static_cast<f32>( maximum.X() ), static_cast<f32>( maximum.Y() ),
                                 static_cast<f32>( maximum.Z() ) ) );
    }

    inline AABB3<real_Num> transformBounds( const AABB3<real_Num> &bounds,
                                            const Transform3<real_Num> &transform )
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

    inline AABB3<real_Num> mergeShapeBounds( const Array<SmartPtr<IPhysicsShape3>> &shapes )
    {
        auto result = AABB3<real_Num>();
        result.setNull();
        auto hasFiniteBounds = false;

        for( const auto &shape : shapes )
        {
            if( !shape || !shape->isEnabled() )
            {
                continue;
            }

            const auto backendShape = dynamic_cast<WPPhysicsShape3Backend *>( shape.get() );
            if( !backendShape )
            {
                continue;
            }

            const auto bounds = backendShape->getAABB();
            if( bounds.isInfinite() )
            {
                result.setInfinite();
                return result;
            }
            if( bounds.isNull() )
            {
                continue;
            }

            if( !hasFiniteBounds )
            {
                result = bounds;
                hasFiniteBounds = true;
            }
            else
            {
                result.merge( bounds );
            }
        }

        return result;
    }
}  // namespace workphone::physics::detail

#endif
