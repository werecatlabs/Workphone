#ifndef WPPHYSICSCONSTRAINT3_HPP
#define WPPHYSICSCONSTRAINT3_HPP

#include <Workphone/Interface/Physics/IPhysicsConstraint3.hpp>

namespace workphone::physics
{
    class WPPhysicsConstraint3 : public IPhysicsConstraint3
    {
    public:
        WPPhysicsConstraint3();
        ~WPPhysicsConstraint3() override;

        SmartPtr<IPhysicsBody3> getBodyA() const override;
        void setBodyA(SmartPtr<IPhysicsBody3> bodyA) override;
        SmartPtr<IPhysicsBody3> getBodyB() const override;
        void setBodyB(SmartPtr<IPhysicsBody3> bodyB) override;

        void setLocalPose(JointActorIndexEnum actor,
                          const Transform3<real_Num> &localPose) override;
        Transform3<real_Num> getLocalPose(JointActorIndexEnum actor) const override;
        void setConstraintFlag(ConstraintFlagEnum flag, bool value) override;
        ConstraintFlagEnum getConstraintFlags() const override;
        void setBreakForce(real_Num force, real_Num torque) override;
        void getBreakForce(real_Num &force, real_Num &torque) const override;
        void setProjectionLinearTolerance(real_Num tolerance) override;
        real_Num getProjectionLinearTolerance() const override;
        void setProjectionAngularTolerance(real_Num tolerance) override;
        real_Num getProjectionAngularTolerance() const override;

        void *getUserData() const override;
        void setUserData(void *userData) override;

        WP_CLASS_REGISTER_DECL;
    };
}
#endif
