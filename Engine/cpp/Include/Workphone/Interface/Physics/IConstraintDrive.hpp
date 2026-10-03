#ifndef __IConstraintDrive_h__
#define __IConstraintDrive_h__

#include <Workphone/Interface/Physics/IPhysicsSpring.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>

namespace workphone
{
    namespace physics
    {

        /** Interface for a constraint drive. */
        class WPCore_API IConstraintDrive : public IPhysicsSpring
        {
        public:
            /** Destructor. */
            ~IConstraintDrive() override;

            /** Gets the force limit. */
            virtual real_Num getForceLimit() const = 0;

            /** Sets the force limit. */
            virtual void setForceLimit( real_Num forceLimit ) = 0;

            /** Gets the flags. */
            virtual D6JointDriveFlagEnum getDriveFlags() const = 0;

            /** Sets the flags. */
            virtual void setDriveFlags( D6JointDriveFlagEnum driveFlags ) = 0;

            /** Sets acceleration. */
            virtual void setIsAcceleration( bool acceleration ) const = 0;

            /** Gets acceleration. */
            virtual bool isAcceleration() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // __IConstraintDrive_h__
