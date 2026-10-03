#ifndef ConstraintD6_h__
#define ConstraintD6_h__

#include <Workphone/Interface/Physics/IConstraintD6.hpp>
#include <Workphone/Physics/PhysicsConstraint3.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class ConstraintD6
         * @brief 6-degree-of-freedom physics constraint implementation.
         *
         * This class implements a D6 (6-DoF) constraint used by the physics
         * subsystem. It stores drive configuration, linear limits and per-axis
         * motion types and provides load/unload and property serialization hooks.
         *
         * The class is a concrete implementation of `IConstraintD6` and derives
         * from `PhysicsConstraint3` to integrate with the engine's shared
         * object and scene systems.
         */
        class WPCore_API ConstraintD6 : public PhysicsConstraint3<IConstraintD6>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes drive position, motion types and clears drive/limit
             * pointers to safe defaults.
             */
            ConstraintD6();

            /**
             * @brief Virtual destructor.
             *
             * Releases any owned resources. Override to ensure derived class
             * cleanup when used polymorphically.
             */
            ~ConstraintD6() override;

            /**
             * @brief Load object data from a serialized representation.
             * @param data Shared object containing serialized data (typically a properties container).
             *
             * Implementations should restore internal state (drive position,
             * drives, motion types and limits) from `data`. This is called when
             * constructing the object from saved scene or configuration.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload object data and release resources.
             * @param data Optional shared object containing context for unloading.
             *
             * Implementations should persist or clear internal state as needed
             * and release references to other shared objects to avoid leaks.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Physics3SharedObject<T>::getChildObjects
             *
             * @return Array of child shared objects referenced by this constraint
             *         (e.g. drives, limits). Returned array must contain valid
             *         SmartPtr<ISharedObject> instances for serialization and editor UI.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc Physics3SharedObject<T>::getProperties
             *
             * @return Properties representation of this constraint suitable for
             *         serialization or editor inspection.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Physics3SharedObject<T>::setProperties
             *
             * Restore internal state from given properties object.
             *
             * @param properties Properties object containing serialized state.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Set the drive's target transform (pose).
             * @param pose Transform to use as the drive target position and orientation.
             *
             * The drive position is used when one or more D6 drives are active to
             * compute target offsets/forces for constrained bodies.
             */
            void setDrivePosition( const Transform3<real_Num> &pose ) override;

            /**
             * @brief Get the currently configured drive target transform.
             * @return Transform3<real_Num> representing the drive target pose.
             */
            Transform3<real_Num> getDrivePosition() const override;

            /**
             * @brief Assign a drive configuration for a given drive index.
             * @param index Drive index (member of D6DriveEnum).
             * @param drive Smart pointer to an IConstraintDrive instance.
             *
             * The supplied drive will be referenced by the constraint and returned
             * by `getChildObjects()` for serialization/inspection.
             */
            void setDrive( D6DriveEnum index, SmartPtr<IConstraintDrive> drive ) override;

            /**
             * @brief Retrieve the drive configuration for a given drive index.
             * @param index Drive index (member of D6DriveEnum).
             * @return SmartPtr<IConstraintDrive> or null if no drive assigned.
             */
            SmartPtr<IConstraintDrive> getDrive( D6DriveEnum index ) const override;

            /**
             * @brief Set the linear limit configuration for this constraint.
             * @param limit Linear limit object to apply to the constraint.
             *
             * The limit restricts translational movement along axes according to
             * the configured parameters.
             */
            void setLinearLimit( SmartPtr<IConstraintLinearLimit> limit ) override;

            /**
             * @brief Get the currently assigned linear limit.
             * @return SmartPtr<IConstraintLinearLimit> or null if none set.
             */
            SmartPtr<IConstraintLinearLimit> getLinearLimit() const override;

            /**
             * @brief Set the allowed motion type for the specified axis.
             * @param axis Axis to configure (member of D6AxisEnum).
             * @param type Motion type to set (member of D6MotionEnum).
             *
             * Motion types describe whether an axis is locked, limited, or free.
             */
            void setMotion( D6AxisEnum axis, D6MotionEnum type ) override;

            /**
             * @brief Get the configured motion type for the specified axis.
             * @param axis Axis to query (member of D6AxisEnum).
             * @return D6MotionEnum Motion type currently configured for the axis.
             */
            D6MotionEnum getMotion( D6AxisEnum axis ) const override;

            WP_CLASS_REGISTER_DECL;

        private:
            /**
             * @brief Convert an axis enum value to a human-readable name.
             * @param axis Axis enum to convert.
             * @return String Name for the axis (used for serialization/UI).
             */
            String getAxisName( D6AxisEnum axis ) const;

            /**
             * @brief Convert a motion enum value to a human-readable name.
             * @param motionType Motion enum to convert.
             * @return String Name for the motion type (used for serialization/UI).
             */
            String getMotionTypeName( D6MotionEnum motionType ) const;

            /**
             * @brief Parse a motion type enum from a string.
             * @param motionTypeName Motion type name to parse (case-insensitive).
             * @return D6MotionEnum Corresponding enum value. If parsing fails,
             *         returns a sensible default (typically eLOCK or eFREE depending on project
             * conventions).
             */
            D6MotionEnum parseMotionType( const String &motionTypeName ) const;

            /** @brief The target transform for the constraint's drives. */
            Transform3<real_Num> m_drivePosition;

            /** @brief Drives for each D6 drive index. */
            SmartPtr<IConstraintDrive> m_drives[static_cast<int>( D6DriveEnum::eCOUNT )];

            /** @brief Linear limit configuration for the constraint (can be null). */
            SmartPtr<IConstraintLinearLimit> m_linearLimit;

            /** @brief Motion type configured per axis. */
            D6MotionEnum m_motionTypes[static_cast<int>( D6AxisEnum::eCOUNT )];
        };
    }  // namespace physics
}  // namespace workphone

#endif  // ConstraintD6_h__
