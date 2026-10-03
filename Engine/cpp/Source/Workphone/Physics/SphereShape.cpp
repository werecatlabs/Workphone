#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/SphereShape.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/State/States/SphereShapeStateData.hpp>

namespace workphone
{
    namespace physics
    {

        WP_CLASS_REGISTER_DERIVED( workphone::physics, SphereShape, PhysicsShape3<ISphereShape3> );

        SphereShape::SphereShape() = default;

        SphereShape::~SphereShape() = default;

        void SphereShape::load( SmartPtr<ISharedObject> data )
        {
            PhysicsShape3<ISphereShape3>::load( data );
        }

        void SphereShape::unload( SmartPtr<ISharedObject> data )
        {
            PhysicsShape3<ISphereShape3>::unload( data );
        }

        void SphereShape::setRadius( real_Num radius )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<SphereShapeStateData>() )
                {
                    state->sphere.setRadius( radius );
                }
            }
        }

        real_Num SphereShape::getRadius() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<SphereShapeStateData>() )
                {
                    return state->sphere.getRadius();
                }
            }

            return real_Num( 0.0 );
        }

    }  // namespace physics
}  // namespace workphone
