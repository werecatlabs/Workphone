#ifndef IPhysicsConstraint2_h__
#define IPhysicsConstraint2_h__

#include <Workphone/Interface/Physics/IPhysicsConstraint.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Interface for a 2D physics constraint.
         *
         * This class represents a constraint between two physics bodies in a 2D environment.
         * Constraints are used to limit the relative motion between bodies, such as
         * joints, springs, or other physical connections.
         *
         * The interface provides functionality for:
         * - Managing the bodies involved in the constraint
         * - Setting up and configuring the constraint
         * - Managing constraint properties and behavior
         *
         * @see IPhysicsBody2D
         * @see IPhysicsManager2D
         * @see IPhysicsScene2
         */
        class WPCore_API IPhysicsConstraint2 : public IPhysicsConstraint
        {
        public:
            /** Destructor */
            ~IPhysicsConstraint2() override;

            /**
             * @brief Gets the first body involved in the constraint.
             *
             * @return A shared pointer to the first physics body.
             */
            virtual SmartPtr<IPhysicsBody2D> getBodyA() const = 0;

            /**
             * @brief Sets the first body involved in the constraint.
             *
             * @param bodyA A shared pointer to the physics body to set as the first body.
             */
            virtual void setBodyA( SmartPtr<IPhysicsBody2D> bodyA ) = 0;

            /**
             * @brief Gets the second body involved in the constraint.
             *
             * @return A shared pointer to the second physics body.
             */
            virtual SmartPtr<IPhysicsBody2D> getBodyB() const = 0;

            /**
             * @brief Sets the second body involved in the constraint.
             *
             * @param bodyB A shared pointer to the physics body to set as the second body.
             */
            virtual void setBodyB( SmartPtr<IPhysicsBody2D> bodyB ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsConstraint2_h__
