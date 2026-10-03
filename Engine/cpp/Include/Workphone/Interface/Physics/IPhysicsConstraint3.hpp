#ifndef IPhysicsConstraint3_h__
#define IPhysicsConstraint3_h__

#include <Workphone/Interface/Physics/IPhysicsConstraint.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 3D physics constraint.
         *
         * This interface represents a constraint between two physics bodies in a 3D environment.
         * Constraints are used to limit the relative motion between bodies, such as joints, springs,
         * or other physical connections. This interface provides methods for configuring the constraint,
         * managing the bodies involved, and setting constraint properties such as break force, flags,
         * and local poses.
         *
         * Key functionalities include:
         * - Managing the two bodies involved in the constraint
         * - Setting and getting local poses for each actor
         * - Configuring constraint flags and break forces
         * - Querying and modifying constraint properties
         *
         * @see IPhysicsBody3
         * @see IPhysicsManager
         * @see ConstraintFlagEnum
         */
        class WPCore_API IPhysicsConstraint3 : public IPhysicsConstraint
        {
        public:
            /** Destructor */
            ~IPhysicsConstraint3() override;

            /**
             * @brief Gets the first body involved in the constraint.
             * @return A smart pointer to the first physics body.
             */
            virtual SmartPtr<IPhysicsBody3> getBodyA() const = 0;

            /**
             * @brief Sets the first body involved in the constraint.
             * @param bodyA A smart pointer to the physics body to set as the first body.
             */
            virtual void setBodyA( SmartPtr<IPhysicsBody3> bodyA ) = 0;

            /**
             * @brief Gets the second body involved in the constraint.
             * @return A smart pointer to the second physics body.
             */
            virtual SmartPtr<IPhysicsBody3> getBodyB() const = 0;

            /**
             * @brief Sets the second body involved in the constraint.
             * @param bodyB A smart pointer to the physics body to set as the second body.
             */
            virtual void setBodyB( SmartPtr<IPhysicsBody3> bodyB ) = 0;

            /**
             * @brief Sets the local pose of a joint actor.
             * @param actor The actor whose local pose to set (e.g., eACTOR0 or eACTOR1).
             * @param localPose The local pose to set for the actor.
             */
            virtual void setLocalPose( JointActorIndexEnum actor,
                                       const Transform3<real_Num> &localPose ) = 0;

            /**
             * @brief Gets the local pose of a joint actor.
             * @param actor The actor whose local pose to get.
             * @return The local pose of the specified joint actor.
             */
            virtual Transform3<real_Num> getLocalPose( JointActorIndexEnum actor ) const = 0;

            /**
             * @brief Sets a constraint flag.
             * @param flag The constraint flag to set.
             * @param value The value to set the flag to (true to enable, false to disable).
             */
            virtual void setConstraintFlag( ConstraintFlagEnum flag, bool value ) = 0;

            /**
             * @brief Gets the current constraint flags.
             * @return The currently set constraint flags.
             */
            virtual ConstraintFlagEnum getConstraintFlags() const = 0;

            /**
             * @brief Sets the force and torque at which the constraint should break.
             * @param force The linear force threshold for breaking the constraint.
             * @param torque The angular torque threshold for breaking the constraint.
             */
            virtual void setBreakForce( real_Num force, real_Num torque ) = 0;

            /**
             * @brief Gets the force and torque at which the constraint breaks.
             * @param force Output parameter for the linear force threshold.
             * @param torque Output parameter for the angular torque threshold.
             */
            virtual void getBreakForce( real_Num &force, real_Num &torque ) const = 0;

            /**
             * @brief Sets the linear tolerance for projection.
             * @param tolerance The linear tolerance value.
             */
            virtual void setProjectionLinearTolerance( real_Num tolerance ) = 0;

            /**
             * @brief Gets the linear tolerance for projection.
             * @return The linear tolerance value.
             */
            virtual real_Num getProjectionLinearTolerance() const = 0;

            /**
             * @brief Sets the angular tolerance for projection.
             * @param tolerance The angular tolerance value.
             */
            virtual void setProjectionAngularTolerance( real_Num tolerance ) = 0;

            /**
             * @brief Gets the angular tolerance for projection.
             * @return The angular tolerance value.
             */
            virtual real_Num getProjectionAngularTolerance() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsConstraint3_h__
