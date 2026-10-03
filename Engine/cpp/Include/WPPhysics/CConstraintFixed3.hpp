#ifndef __CConstraintFixed3_h__
#define __CConstraintFixed3_h__

#include <Workphone/Interface/Physics/IConstraintFixed3.hpp>

namespace workphone
{
    namespace physics
    {
        class CConstraintFixed3 : public IConstraintFixed3
        {
        public:
            CConstraintFixed3();
            virtual ~CConstraintFixed3() override;

            virtual SmartPtr<IPhysicsBody3> getBodyA() const override;
            virtual void                    setBodyA( SmartPtr<IPhysicsBody3> bodyA ) override;
            virtual SmartPtr<IPhysicsBody3> getBodyB() const override;
            virtual void                    setBodyB( SmartPtr<IPhysicsBody3> bodyB ) override;
            virtual void                 setLocalPose( JointActorIndexEnum         actor,
                                                       const Transform3<real_Num> &localPose ) override;
            virtual Transform3<real_Num> getLocalPose( JointActorIndexEnum actor ) const override;
            virtual void               setConstraintFlag( ConstraintFlagEnum flag, bool value ) override;
            virtual ConstraintFlagEnum getConstraintFlags() const override;
            virtual void               setBreakForce( real_Num force, real_Num torque ) override;
            virtual void               getBreakForce( real_Num &force, real_Num &torque ) const override;
            virtual void               setProjectionLinearTolerance( real_Num tolerance ) override;
            virtual real_Num           getProjectionLinearTolerance() const override;
            virtual void               setProjectionAngularTolerance( real_Num tolerance ) override;
            virtual real_Num           getProjectionAngularTolerance() const override;

            virtual void *getUserData() const override;
            virtual void  setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    } // namespace physics
} // namespace workphone
#endif
