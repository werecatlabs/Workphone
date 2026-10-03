#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/PlaneShape.hpp>
#include <Workphone/State/States/PlaneShapeState.hpp>

namespace workphone
{
    namespace physics
    {
        WP_CLASS_REGISTER_DERIVED( workphone::physics, PlaneShape, PhysicsShape3<IPlaneShape3> );

        PlaneShape::PlaneShape() = default;

        PlaneShape::~PlaneShape() = default;

        real_Num PlaneShape::getDistance() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<PlaneShapeState>() )
                {
                    return state->plane.getDistance();
                }
            }

            return (real_Num)0.0;
        }

        void PlaneShape::setDistance( real_Num distance )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<PlaneShapeState>() )
                {
                    return state->plane.setDistance( distance );
                }
            }
        }

        Vector3<real_Num> PlaneShape::getNormal() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<PlaneShapeState>() )
                {
                    return state->plane.getNormal();
                }
            }

            return Vector3<real_Num>();
        }

        void PlaneShape::setNormal( const Vector3<real_Num> &normal )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<PlaneShapeState>() )
                {
                    return state->plane.setNormal( normal );
                }
            }
        }

        Plane3<real_Num> PlaneShape::getPlane() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<PlaneShapeState>() )
                {
                    return state->plane;
                }
            }

            return Plane3<real_Num>();
        }
    }  // namespace physics
}  // namespace workphone
