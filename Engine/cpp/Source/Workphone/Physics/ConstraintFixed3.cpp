#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/ConstraintFixed3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{
    namespace physics
    {

        WP_CLASS_REGISTER_DERIVED( workphone::physics, ConstraintFixed3,
                                   PhysicsConstraint3<IConstraintFixed3> );

        ConstraintFixed3::ConstraintFixed3() = default;

        ConstraintFixed3::~ConstraintFixed3() = default;

        void ConstraintFixed3::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                PhysicsConstraint3<IConstraintFixed3>::load( data );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void ConstraintFixed3::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                PhysicsConstraint3<IConstraintFixed3>::unload( data );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        Array<SmartPtr<ISharedObject>> ConstraintFixed3::getChildObjects() const
        {
            return Array<SmartPtr<ISharedObject>>();
        }

        SmartPtr<Properties> ConstraintFixed3::getProperties() const
        {
            auto properties = PhysicsConstraint3<IConstraintFixed3>::getProperties();
            return properties;
        }

        void ConstraintFixed3::setProperties( SmartPtr<Properties> properties )
        {
            PhysicsConstraint3<IConstraintFixed3>::setProperties( properties );
        }

    }  // namespace physics
}  // namespace workphone
