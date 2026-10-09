#ifndef WPPHYSICSCONSTRAINTDRIVE_HPP
#define WPPHYSICSCONSTRAINTDRIVE_HPP

#include <Workphone/Interface/Physics/IConstraintDrive.hpp>

namespace workphone::physics
{
    class WPPhysicsConstraintDrive : public IConstraintDrive
    {
    public:
        WPPhysicsConstraintDrive();
        ~WPPhysicsConstraintDrive() override;

        real_Num getForceLimit() const override;
        void setForceLimit(real_Num forceLimit) override;
        D6JointDriveFlagEnum getDriveFlags() const override;
        void setDriveFlags(D6JointDriveFlagEnum driveFlags) override;
        void setIsAcceleration(bool acceleration) const override;
        bool isAcceleration() const override;
        real_Num getStiffness() const override;
        void setStiffness(real_Num stiffness) override;
        real_Num getDamping() const override;
        void setDamping(real_Num damping) override;

        void *getUserData() const override;
        void setUserData(void *userData) override;

        WP_CLASS_REGISTER_DECL;
    };
}
#endif
