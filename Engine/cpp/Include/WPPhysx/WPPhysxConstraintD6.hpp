#ifndef WPPhysxConstraintD6_h__
#define WPPhysxConstraintD6_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <WPPhysx/WPPhysxConstraint.hpp>
#include <Workphone/Physics/ConstraintD6.hpp>

/**
 * @file
 * @brief PhysX implementation of a 6 degree-of-freedom constraint wrapper.
 *
 * This header declares `PhysxConstraintD6`, a concrete PhysX-backed
 * implementation of the engine's `ConstraintD6` interface. The class
 * adapts the generic constraint API to PhysX-specific constructs and stores
 * any required local state (drive pose, limits, drives, etc.).
 */
namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX-backed implementation of a D6 (6-DOF) constraint.
         *
         * `PhysxConstraintD6` implements `PhysxConstraint<ConstraintD6>` and
         * provides the glue between the engine's `ConstraintD6` API and
         * the underlying PhysX constraint object. It stores a drive
         * reference pose and exposes methods to load/unload configuration,
         * configure drives and limits, and respond to state change messages.
         */
        class PhysxConstraintD6 : public PhysxConstraint<ConstraintD6>
        {
        public:
            /**
             * @brief Construct a new PhysxConstraintD6 object.
             */
            PhysxConstraintD6();

            /**
             * @brief Destroy the PhysxConstraintD6 object.
             */
            ~PhysxConstraintD6() override;

            /**
             * @brief Load constraint configuration from a shared data object.
             *
             * This will typically read drive parameters, limits and other
             * D6-specific configuration and apply them to the underlying
             * PhysX constraint instance.
             *
             * @param data Shared object containing serialized configuration.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload or clear any runtime configuration previously loaded.
             *
             * Implementations should release or reset any resources that were
             * created by `load`.
             *
             * @param data Optional shared object associated with the load.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Set the drive (target) pose used by D6 drives.
             *
             * The drive position is a reference transformation that drive
             * targets are measured relative to.
             *
             * @param pose Drive reference transform in local space.
             */
            void setDrivePosition( const Transform3<physics_Num> &pose ) override;

            /**
             * @brief Get the currently configured drive reference pose.
             *
             * @return Transform3<physics_Num> The stored drive pose.
             */
            Transform3<physics_Num> getDrivePosition() const override;

            /**
             * @brief Assign a drive configuration for the given drive index.
             *
             * Drives control target velocities or positions for selected
             * axes of the D6 constraint.
             *
             * @param index Drive index (D6DriveEnum).
             * @param drive Drive configuration object.
             */
            void setDrive( D6DriveEnum index, SmartPtr<IConstraintDrive> drive ) override;

            /**
             * @brief Retrieve the configured drive for the specified index.
             *
             * @param index Drive index to query.
             * @return SmartPtr<IConstraintDrive> Configured drive or null.
             */
            SmartPtr<IConstraintDrive> getDrive( D6DriveEnum index ) const override;

            /**
             * @brief Set the linear limit parameters for the D6 constraint.
             *
             * Linear limits constrain translation along the primary axis.
             *
             * @param limit Linear limit configuration object.
             */
            void setLinearLimit( SmartPtr<IConstraintLinearLimit> limit ) override;

            /**
             * @brief Get the currently configured linear limit object.
             *
             * @return SmartPtr<IConstraintLinearLimit> Current linear limit.
             */
            SmartPtr<IConstraintLinearLimit> getLinearLimit() const override;

            /**
             * @brief Configure motion type for a specific axis.
             *
             * Motion type indicates whether the axis is locked, limited or free.
             *
             * @param axis Axis to configure (D6AxisEnum).
             * @param type Motion type to set (D6MotionEnum).
             */
            void setMotion( D6AxisEnum axis, D6MotionEnum type ) override;

            /**
             * @brief Query the motion type configured for an axis.
             *
             * @param axis Axis to query.
             * @return D6MotionEnum Motion setting for the axis.
             */
            D6MotionEnum getMotion( D6AxisEnum axis ) const override;

            /**
             * @brief Handle a state change message.
             *
             * Some components broadcast state messages; the constraint can
             * react to those messages to update internal configuration.
             *
             * @param message The state message being delivered.
             */
            void handleStateChanged( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle a state object change.
             *
             * Alternative overload used when a full state object is provided.
             *
             * @param state The changed state object.
             */
            void handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Register this class with the reflection/serialization system.
             *
             * Macro expands to the required declarations for the project's
             * runtime type registration facilities.
             */
            WP_CLASS_REGISTER_DECL;

        private:
            /**
             * @brief Stored drive reference pose for drive-based control.
             *
             * This is the transform used by constraint drives as the target
             * position/orientation. Keeping a local copy avoids repeated
             * queries against the PhysX object and allows the engine to
             * serialize/deserialize the configured pose.
             */
            Transform3<physics_Num> m_drivePosition;
        };
    } // namespace physics
} // namespace workphone

#endif // WPPhysxConstraintD6_h__
