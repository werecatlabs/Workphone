#ifndef __IConstraintD6_h__
#define __IConstraintD6_h__

#include <Workphone/Interface/Physics/IPhysicsConstraint3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 6-DOF (Degrees of Freedom) constraint in 3D physics.
         *
         * This interface extends IPhysicsConstraint3 to provide additional methods for configuring
         * 6-DOF constraints (also known as D6 or generic constraints). D6 constraints allow for
         * fine-grained control over all six degrees of freedom (translation and rotation along X, Y, Z),
         * including locking, limiting, or freeing each axis, as well as configuring drives and limits.
         *
         * Key functionalities include:
         * - Setting and getting the drive position
         * - Configuring drives for each axis (linear and angular)
         * - Setting and retrieving linear limits
         * - Controlling the motion type (locked, limited, free) for each axis
         *
         * @see IPhysicsConstraint3
         * @see D6AxisEnum
         * @see D6DriveEnum
         * @see D6MotionEnum
         * @see IConstraintDrive
         * @see IConstraintLinearLimit
         */
        class WPCore_API IConstraintD6 : public IPhysicsConstraint3
        {
        public:
            /** Destructor */
            ~IConstraintD6() override;

            /**
             * @brief Sets the drive position for the constraint.
             * @param pose The desired drive position as a transform.
             */
            virtual void setDrivePosition( const Transform3<real_Num> &pose ) = 0;

            /**
             * @brief Gets the current drive position for the constraint.
             * @return The current drive position as a transform.
             */
            virtual Transform3<real_Num> getDrivePosition() const = 0;

            /**
             * @brief Sets the drive for the specified drive index.
             * @param index The drive index (see D6DriveEnum).
             * @param drive The drive object to set.
             */
            virtual void setDrive( D6DriveEnum index, SmartPtr<IConstraintDrive> drive ) = 0;

            /**
             * @brief Gets the drive for the specified drive index.
             * @param index The drive index (see D6DriveEnum).
             * @return The drive object for the specified index.
             */
            virtual SmartPtr<IConstraintDrive> getDrive( D6DriveEnum index ) const = 0;

            /**
             * @brief Sets the linear limit for the constraint.
             * @param limit The linear limit object to set.
             */
            virtual void setLinearLimit( SmartPtr<IConstraintLinearLimit> limit ) = 0;

            /**
             * @brief Gets the current linear limit for the constraint.
             * @return The current linear limit object.
             */
            virtual SmartPtr<IConstraintLinearLimit> getLinearLimit() const = 0;

            /**
             * @brief Sets the motion type for the specified axis.
             * @param axis The axis to set the motion for (see D6AxisEnum).
             * @param type The motion type to set (locked, limited, or free).
             */
            virtual void setMotion( D6AxisEnum axis, D6MotionEnum type ) = 0;

            /**
             * @brief Gets the motion type for the specified axis.
             * @param axis The axis to get the motion type for (see D6AxisEnum).
             * @return The motion type for the specified axis.
             */
            virtual D6MotionEnum getMotion( D6AxisEnum axis ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IConstraintD6_h__
