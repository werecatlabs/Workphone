#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CPhysicsConstraint3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        CPhysicsConstraint3::CPhysicsConstraint3()
        {
        }
        CPhysicsConstraint3::~CPhysicsConstraint3()
        {
        }

        SmartPtr<IPhysicsBody3> CPhysicsConstraint3::getBodyA() const
        {
            return nullptr;
        }
        void CPhysicsConstraint3::setBodyA( SmartPtr<IPhysicsBody3> bodyA )
        {
        }
        SmartPtr<IPhysicsBody3> CPhysicsConstraint3::getBodyB() const
        {
            return nullptr;
        }
        void CPhysicsConstraint3::setBodyB( SmartPtr<IPhysicsBody3> bodyB )
        {
        }
        void CPhysicsConstraint3::setLocalPose( JointActorIndexEnum         actor,
                                                const Transform3<real_Num> &localPose )
        {
        }
        Transform3<real_Num> CPhysicsConstraint3::getLocalPose( JointActorIndexEnum actor ) const
        {
            return Transform3<real_Num>();
        }
        void CPhysicsConstraint3::setConstraintFlag( ConstraintFlagEnum flag, bool value )
        {
        }
        ConstraintFlagEnum CPhysicsConstraint3::getConstraintFlags() const
        {
            return static_cast<ConstraintFlagEnum>( 0 );
        }
        void CPhysicsConstraint3::setBreakForce( real_Num force, real_Num torque )
        {
        }
        void CPhysicsConstraint3::getBreakForce( real_Num &force, real_Num &torque ) const
        {
            force = 0.0f;
            torque = 0.0f;
        }
        void CPhysicsConstraint3::setProjectionLinearTolerance( real_Num tolerance )
        {
        }
        real_Num CPhysicsConstraint3::getProjectionLinearTolerance() const
        {
            return 0.0f;
        }
        void CPhysicsConstraint3::setProjectionAngularTolerance( real_Num tolerance )
        {
        }
        real_Num CPhysicsConstraint3::getProjectionAngularTolerance() const
        {
            return 0.0f;
        }
        void *CPhysicsConstraint3::getUserData() const
        {
            return nullptr;
        }
        void CPhysicsConstraint3::setUserData( void *userData )
        {
        }

        void CPhysicsConstraint3::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CPhysicsConstraint3::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CPhysicsConstraint3::typeInfo()
        {
            return sTypeInfo;
        }
        void CPhysicsConstraint3::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CPhysicsConstraint3::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
