#ifndef ConstraintDrive_h__
#define ConstraintDrive_h__

#include <Workphone/Interface/Physics/IConstraintDrive.hpp>
#include <Workphone/Physics/PhysicsShape3.hpp>
#include <Workphone/Physics/PhysicsSpring.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Implementation of a constraint drive for physics joints.
         *
         * This class provides a concrete implementation of IConstraintDrive,
         * which is used to drive motion in physics constraints (particularly D6 joints).
         *
         * Constraint drives are used to:
         * - Apply forces or accelerations to joint degrees of freedom
         * - Control the motion of joints with spring-damper behavior
         * - Implement motor-like behavior in joints
         *
         * The drive combines spring stiffness and damping to create realistic
         * joint behavior, with configurable force limits and drive modes.
         *
         * @see IConstraintDrive
         * @see IPhysicsSpring
         * @see D6JointDriveFlagEnum
         */
        class WPCore_API ConstraintDrive : public PhysicsSpring<IConstraintDrive>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the constraint drive with default values:
             * - Stiffness: 0.0
             * - Damping: 0.0
             * - Force limit: 0.0
             * - Drive flags: eACCELERATION
             * - Is acceleration: true
             */
            ConstraintDrive();

            /**
             * @brief Destructor.
             */
            ~ConstraintDrive() override;

            /**
             * @brief Loads constraint drive data from a shared object.
             *
             * This method is used to initialize the constraint drive with
             * settings from a data source (e.g., serialized data, configuration file).
             *
             * @param data The shared object containing the drive configuration data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads constraint drive data and resets to defaults.
             *
             * This method cleans up the constraint drive and resets all
             * properties to their default values.
             *
             * @param data The shared object (unused in this implementation).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the stiffness of the spring.
             *
             * @return The current stiffness value.
             */
            real_Num getStiffness() const override;

            /**
             * @brief Sets the stiffness of the spring.
             *
             * @param stiffness The new stiffness value to set.
             */
            void setStiffness( real_Num stiffness ) override;

            /**
             * @brief Gets the damping of the spring.
             *
             * @return The current damping value.
             */
            real_Num getDamping() const override;

            /**
             * @brief Sets the damping of the spring.
             *
             * @param damping The new damping value to set.
             */
            void setDamping( real_Num damping ) override;

            /**
             * @brief Gets the force limit.
             *
             * @return The current force limit value.
             */
            real_Num getForceLimit() const override;

            /**
             * @brief Sets the force limit.
             *
             * @param forceLimit The new force limit value to set.
             */
            void setForceLimit( real_Num forceLimit ) override;

            /**
             * @brief Gets the drive flags.
             *
             * @return The current drive flags.
             */
            D6JointDriveFlagEnum getDriveFlags() const override;

            /**
             * @brief Sets the drive flags.
             *
             * @param driveFlags The new drive flags to set.
             */
            void setDriveFlags( D6JointDriveFlagEnum driveFlags ) override;

            /**
             * @brief Sets whether the drive uses acceleration mode.
             *
             * @param acceleration True for acceleration mode, false for force mode.
             */
            void setIsAcceleration( bool acceleration ) const override;

            /**
             * @brief Gets whether the drive uses acceleration mode.
             *
             * @return True if in acceleration mode, false if in force mode.
             */
            bool isAcceleration() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** The spring stiffness value */
            real_Num m_stiffness;

            /** The spring damping value */
            real_Num m_damping;

            /** The maximum force that can be applied by the drive */
            real_Num m_forceLimit;

            /** Flags controlling the drive behavior */
            D6JointDriveFlagEnum m_driveFlags;

            /** Whether the drive operates in acceleration mode */
            bool m_isAcceleration;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // ConstraintDrive_h__
