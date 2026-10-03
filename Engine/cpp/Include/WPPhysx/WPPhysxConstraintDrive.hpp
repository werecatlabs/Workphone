#ifndef WPPhysxConstraintDrive_h__
#define WPPhysxConstraintDrive_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <WPPhysx/WPPhysxSharedObject.hpp>
#include <Workphone/Physics/ConstraintDrive.hpp>

/**
 * @file WPPhysxConstraintDrive.hpp
 * @brief Wrapper that adapts the engine's generic `ConstraintDrive` interface to PhysX.
 *
 * This header declares `PhysxConstraintDrive`, a thin PhysX-backed implementation
 * of the engine's `ConstraintDrive` abstraction. It forwards calls to the
 * underlying PhysX objects and exposes getters/setters for drive parameters
 * such as force limit, stiffness and damping.
 */

namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX implementation of `ConstraintDrive`.
         *
         * `PhysxConstraintDrive` implements the engine-level `ConstraintDrive`
         * interface using PhysX primitives. The class is a shared-object
         * wrapper (see `PhysxSharedObject<T>`) and is intended to be used by
         * constraint/joint wrappers that require drive parameters for motion
         * limits, motorization and damping.
         */
        class PhysxConstraintDrive : public PhysxSharedObject<ConstraintDrive>
        {
        public:
            /**
             * @brief Construct a new PhysxConstraintDrive.
             *
             * Initializes internal state. Actual PhysX association may be
             * performed after construction by the owning constraint object.
             */
            PhysxConstraintDrive();

            /**
             * @brief Destroy the PhysxConstraintDrive.
             */
            ~PhysxConstraintDrive() override;

            /**
             * @brief Get the maximum force allowed for this drive.
             *
             * @return physics_Num Maximum force limit applied by the drive.
             */
            physics_Num getForceLimit() const override;

            /**
             * @brief Set the maximum force allowed for this drive.
             *
             * @param forceLimit New maximum force to apply.
             */
            void setForceLimit( physics_Num forceLimit ) override;

            /**
             * @brief Get the flags that control drive behavior.
             *
             * Flags typically indicate whether the drive is enabled for
             * position, velocity, etc. See `D6JointDriveFlagEnum` for details.
             *
             * @return D6JointDriveFlagEnum Current drive flags.
             */
            D6JointDriveFlagEnum getDriveFlags() const override;

            /**
             * @brief Set the flags that control drive behavior.
             *
             * @param driveFlags Bitmask from `D6JointDriveFlagEnum`.
             */
            void setDriveFlags( D6JointDriveFlagEnum driveFlags ) override;

            /**
             * @brief Set whether this drive interprets its target as an acceleration.
             *
             * When true the drive treats the target as an acceleration value
             * instead of a velocity/position target. Kept const to match the
             * interface; implementation may update PhysX internals.
             *
             * @param acceleration True to treat target as acceleration.
             */
            void setIsAcceleration( bool acceleration ) const override;

            /**
             * @brief Query whether the drive uses acceleration-mode targets.
             *
             * @return true if acceleration-mode is enabled, false otherwise.
             */
            bool isAcceleration() const override;

            /**
             * @brief Get the drive stiffness value.
             *
             * Stiffness controls how strongly the drive resists deviation from
             * its target.
             *
             * @return physics_Num Stiffness coefficient.
             */
            physics_Num getStiffness() const override;

            /**
             * @brief Set the drive stiffness value.
             *
             * @param stiffness New stiffness coefficient.
             */
            void setStiffness( physics_Num stiffness ) override;

            /**
             * @brief Get the drive damping value.
             *
             * Damping controls how much the drive resists velocity (dissipates energy).
             *
             * @return physics_Num Damping coefficient.
             */
            physics_Num getDamping() const override;

            /**
             * @brief Set the drive damping value.
             *
             * @param damping New damping coefficient.
             */
            void setDamping( physics_Num damping ) override;

            /**
             * @brief Macro used for runtime class registration.
             *
             * Expands to registration helpers used by the engine reflection
             * or factory system.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxConstraintDrive_h__
