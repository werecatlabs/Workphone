#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/MeshShape.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/State/States/MeshShapeStateData.hpp>

namespace workphone
{
    namespace physics
    {

        WP_CLASS_REGISTER_DERIVED( workphone::physics, MeshShape, PhysicsShape3<IMeshShape> );

        MeshShape::MeshShape() = default;

        MeshShape::~MeshShape() = default;

        void MeshShape::load( SmartPtr<ISharedObject> data )
        {
            PhysicsShape3<IMeshShape>::load( data );
        }

        void MeshShape::unload( SmartPtr<ISharedObject> data )
        {
            PhysicsShape3<IMeshShape>::unload( data );
        }

        bool MeshShape::isValid() const
        {
            auto valid = PhysicsShape3<IMeshShape>::isValid();
            return valid;
        }

        bool MeshShape::isConvex() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<MeshShapeStateData>() )
                {
                    return state->convex;
                }
            }

            return false;
        }

        void MeshShape::setConvex( bool convex )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<MeshShapeStateData>() )
                {
                    state->convex = convex;
                }
            }
        }
    }  // namespace physics
}  // namespace workphone
