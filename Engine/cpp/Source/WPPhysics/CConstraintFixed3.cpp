#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CConstraintFixed3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        CConstraintFixed3::CConstraintFixed3()
        {
        }
        CConstraintFixed3::~CConstraintFixed3()
        {
        }

        SmartPtr<IPhysicsBody3> CConstraintFixed3::getBodyA() const
        {
            return nullptr;
        }
        void CConstraintFixed3::setBodyA( SmartPtr<IPhysicsBody3> bodyA )
        {
        }
        SmartPtr<IPhysicsBody3> CConstraintFixed3::getBodyB() const
        {
            return nullptr;
        }
        void CConstraintFixed3::setBodyB( SmartPtr<IPhysicsBody3> bodyB )
        {
        }
        void CConstraintFixed3::setLocalPose( JointActorIndexEnum         actor,
                                              const Transform3<real_Num> &localPose )
        {
        }
        Transform3<real_Num> CConstraintFixed3::getLocalPose( JointActorIndexEnum actor ) const
        {
            return Transform3<real_Num>();
        }
        void CConstraintFixed3::setConstraintFlag( ConstraintFlagEnum flag, bool value )
        {
        }
        ConstraintFlagEnum CConstraintFixed3::getConstraintFlags() const
        {
            return static_cast<ConstraintFlagEnum>( 0 );
        }
        void CConstraintFixed3::setBreakForce( real_Num force, real_Num torque )
        {
        }
        void CConstraintFixed3::getBreakForce( real_Num &force, real_Num &torque ) const
        {
            force = 0.0f;
            torque = 0.0f;
        }
        void CConstraintFixed3::setProjectionLinearTolerance( real_Num tolerance )
        {
        }
        real_Num CConstraintFixed3::getProjectionLinearTolerance() const
        {
            return 0.0f;
        }
        void CConstraintFixed3::setProjectionAngularTolerance( real_Num tolerance )
        {
        }
        real_Num CConstraintFixed3::getProjectionAngularTolerance() const
        {
            return 0.0f;
        }
        void *CConstraintFixed3::getUserData() const
        {
            return nullptr;
        }
        void CConstraintFixed3::setUserData( void *userData )
        {
        }

        void CConstraintFixed3::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CConstraintFixed3::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CConstraintFixed3::typeInfo()
        {
            return sTypeInfo;
        }
        void CConstraintFixed3::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CConstraintFixed3::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
