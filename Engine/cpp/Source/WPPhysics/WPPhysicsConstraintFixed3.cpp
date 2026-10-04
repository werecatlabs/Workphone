#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraintFixed3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        WPPhysicsConstraintFixed3::WPPhysicsConstraintFixed3()
        {
        }
        WPPhysicsConstraintFixed3::~WPPhysicsConstraintFixed3()
        {
        }

        SmartPtr<IPhysicsBody3> WPPhysicsConstraintFixed3::getBodyA() const
        {
            return nullptr;
        }
        void WPPhysicsConstraintFixed3::setBodyA( SmartPtr<IPhysicsBody3> bodyA )
        {
        }
        SmartPtr<IPhysicsBody3> WPPhysicsConstraintFixed3::getBodyB() const
        {
            return nullptr;
        }
        void WPPhysicsConstraintFixed3::setBodyB( SmartPtr<IPhysicsBody3> bodyB )
        {
        }
        void WPPhysicsConstraintFixed3::setLocalPose( JointActorIndexEnum         actor,
                                              const Transform3<real_Num> &localPose )
        {
        }
        Transform3<real_Num> WPPhysicsConstraintFixed3::getLocalPose( JointActorIndexEnum actor ) const
        {
            return Transform3<real_Num>();
        }
        void WPPhysicsConstraintFixed3::setConstraintFlag( ConstraintFlagEnum flag, bool value )
        {
        }
        ConstraintFlagEnum WPPhysicsConstraintFixed3::getConstraintFlags() const
        {
            return static_cast<ConstraintFlagEnum>( 0 );
        }
        void WPPhysicsConstraintFixed3::setBreakForce( real_Num force, real_Num torque )
        {
        }
        void WPPhysicsConstraintFixed3::getBreakForce( real_Num &force, real_Num &torque ) const
        {
            force = 0.0f;
            torque = 0.0f;
        }
        void WPPhysicsConstraintFixed3::setProjectionLinearTolerance( real_Num tolerance )
        {
        }
        real_Num WPPhysicsConstraintFixed3::getProjectionLinearTolerance() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintFixed3::setProjectionAngularTolerance( real_Num tolerance )
        {
        }
        real_Num WPPhysicsConstraintFixed3::getProjectionAngularTolerance() const
        {
            return 0.0f;
        }
        void *WPPhysicsConstraintFixed3::getUserData() const
        {
            return nullptr;
        }
        void WPPhysicsConstraintFixed3::setUserData( void *userData )
        {
        }

        void WPPhysicsConstraintFixed3::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsConstraintFixed3::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsConstraintFixed3::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsConstraintFixed3::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsConstraintFixed3::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
