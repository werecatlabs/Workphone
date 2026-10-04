#ifndef WPPHYSICSCONSTRAINTDRIVE_HPP
#define WPPHYSICSCONSTRAINTDRIVE_HPP

#include <Workphone/Interface/Physics/IConstraintDrive.hpp>

namespace workphone
{
    namespace physics
    {
        class WPPhysicsConstraintDrive : public IConstraintDrive
        {
        public:
            WPPhysicsConstraintDrive();
            virtual ~WPPhysicsConstraintDrive() override;

            virtual real_Num getForceLimit() const override;
            virtual void setForceLimit( real_Num forceLimit ) override;
            virtual D6JointDriveFlagEnum getDriveFlags() const override;
            virtual void setDriveFlags( D6JointDriveFlagEnum driveFlags ) override;
            virtual void setIsAcceleration( bool acceleration ) const override;
            virtual bool isAcceleration() const override;
            virtual real_Num getStiffness() const override;
            virtual void setStiffness( real_Num stiffness ) override;
            virtual real_Num getDamping() const override;
            virtual void setDamping( real_Num damping ) override;

            virtual void *getUserData() const override;
            virtual void setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone
#endif
