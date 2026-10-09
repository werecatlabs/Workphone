#ifndef WPPHYSICSCONSTRAINTD6_HPP
#define WPPHYSICSCONSTRAINTD6_HPP

#include <Workphone/Interface/Physics/IConstraintD6.hpp>

// C89 WorkphonePhysics backend
extern "C" {
#include <WorkphonePhysics/workphone_physics_constraint.h>
}

namespace workphone::physics
{
    class WPPhysicsConstraintD6 : public IConstraintD6
    {
    public:
        WPPhysicsConstraintD6();
        ~WPPhysicsConstraintD6() override;

        void setDrivePosition(const Transform3<real_Num> &pose) override;
        Transform3<real_Num> getDrivePosition() const override;
        void setDrive(D6DriveEnum index, SmartPtr<IConstraintDrive> drive) override;
        SmartPtr<IConstraintDrive> getDrive(D6DriveEnum index) const override;
        void setLinearLimit(SmartPtr<IConstraintLinearLimit> limit) override;
        SmartPtr<IConstraintLinearLimit> getLinearLimit() const override;
        void setMotion(D6AxisEnum axis, D6MotionEnum type) override;
        D6MotionEnum getMotion(D6AxisEnum axis) const override;

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

        /**
             * @brief Returns the underlying C89 WorkphonePhysics constraint handle.
             * @return Pointer to the internal wp_constraint.
             */
        wp_constraint *getConstraint() const;

        WP_CLASS_REGISTER_DECL;

    private:
        wp_constraint *m_constraint = nullptr; ///< Underlying C89 constraint (D6 joint)

        SmartPtr<IPhysicsBody3> m_bodyA; ///< First body involved in the constraint
        SmartPtr<IPhysicsBody3> m_bodyB; ///< Second body involved in the constraint

        // Cached drive/limit objects so getters can return the last value set.
        SmartPtr<IConstraintDrive> m_drives[WORKPHONE_D6_DRIVE_COUNT];
        SmartPtr<IConstraintLinearLimit> m_linearLimit;

        void *m_userData = nullptr; ///< Opaque user data
    };
}
#endif
