#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraint3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        WPPhysicsConstraint3::WPPhysicsConstraint3()
        {
        }
        WPPhysicsConstraint3::~WPPhysicsConstraint3()
        {
        }

        SmartPtr<IPhysicsBody3> WPPhysicsConstraint3::getBodyA() const
        {
            return nullptr;
        }
        void WPPhysicsConstraint3::setBodyA( SmartPtr<IPhysicsBody3> bodyA )
        {
        }
        SmartPtr<IPhysicsBody3> WPPhysicsConstraint3::getBodyB() const
        {
            return nullptr;
        }
        void WPPhysicsConstraint3::setBodyB( SmartPtr<IPhysicsBody3> bodyB )
        {
        }
        void WPPhysicsConstraint3::setLocalPose( JointActorIndexEnum         actor,
                                                const Transform3<real_Num> &localPose )
        {
        }
        Transform3<real_Num> WPPhysicsConstraint3::getLocalPose( JointActorIndexEnum actor ) const
        {
            return Transform3<real_Num>();
        }
        void WPPhysicsConstraint3::setConstraintFlag( ConstraintFlagEnum flag, bool value )
        {
        }
        ConstraintFlagEnum WPPhysicsConstraint3::getConstraintFlags() const
        {
            return static_cast<ConstraintFlagEnum>( 0 );
        }
        void WPPhysicsConstraint3::setBreakForce( real_Num force, real_Num torque )
        {
        }
        void WPPhysicsConstraint3::getBreakForce( real_Num &force, real_Num &torque ) const
        {
            force = 0.0f;
            torque = 0.0f;
        }
        void WPPhysicsConstraint3::setProjectionLinearTolerance( real_Num tolerance )
        {
        }
        real_Num WPPhysicsConstraint3::getProjectionLinearTolerance() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraint3::setProjectionAngularTolerance( real_Num tolerance )
        {
        }
        real_Num WPPhysicsConstraint3::getProjectionAngularTolerance() const
        {
            return 0.0f;
        }
        void *WPPhysicsConstraint3::getUserData() const
        {
            return nullptr;
        }
        void WPPhysicsConstraint3::setUserData( void *userData )
        {
        }

        void WPPhysicsConstraint3::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsConstraint3::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsConstraint3::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsConstraint3::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsConstraint3::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
