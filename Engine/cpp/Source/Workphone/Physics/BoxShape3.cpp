#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/BoxShape3.hpp>
#include <Workphone/State/States/BoxShapeStateData.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone
{
    namespace physics
    {
        WP_CLASS_REGISTER_DERIVED( workphone::physics, BoxShape3, PhysicsShape3<IBoxShape3> );

        BoxShape3::BoxShape3() = default;

        BoxShape3::~BoxShape3() = default;

        void BoxShape3::load( SmartPtr<ISharedObject> data )
        {
            PhysicsShape3<IBoxShape3>::load( data );
        }

        void BoxShape3::unload( SmartPtr<ISharedObject> data )
        {
            PhysicsShape3<IBoxShape3>::unload( data );
        }

        Vector3<real_Num> BoxShape3::getExtents() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<BoxShapeStateData>() )
                {
                    return state->extents;
                }
            }

            return {};
        }

        void BoxShape3::setExtents( const Vector3<real_Num> &extents )
        {
            if( !MathUtil<physics_Num>::equals( extents, getExtents() ) )
            {
                if( auto stateContext = getStateContext() )
                {
                    if( auto state = stateContext->invalidateStateData<BoxShapeStateData>() )
                    {
                        state->extents = extents;
                    }
                }
            }
        }

        AABB3<real_Num> BoxShape3::getAABB() const
        {
            auto extents = getExtents();
            return AABB3<real_Num>( -extents, extents );
        }

        void BoxShape3::setAABB( const AABB3<real_Num> &box )
        {
            auto extents = box.getExtent();
            setExtents( extents );
        }

        bool BoxShape3::isValid() const
        {
            if( isLoaded() )
            {
                if( isAttached() )
                {
                    auto extents = getExtents();
                    if( extents.x > 0 && extents.y > 0 && extents.z > 0 )
                    {
                        return true;
                    }
                }
            }

            return false;
        }

    }  // namespace physics
}  // namespace workphone
