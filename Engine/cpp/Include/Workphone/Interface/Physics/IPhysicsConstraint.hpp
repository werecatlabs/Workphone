#ifndef __IPhysicsConstraint_h__
#define __IPhysicsConstraint_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a physics constraint.
         *
         * This class represents the base interface for all physics constraints in the system.
         * Constraints are used to limit the relative motion between physics bodies, such as
         * joints, springs, or other physical connections.
         *
         * The interface provides functionality for:
         * - Managing constraint properties (break force, flags, etc.)
         * - Controlling constraint behavior
         * - Handling constraint state and transformations
         * - Managing constraint visualization
         *
         * Constraints are commonly used for:
         * - Creating joints between bodies
         * - Implementing springs and dampers
         * - Limiting motion in specific directions
         * - Creating complex mechanical systems
         *
         * @see IPhysicsConstraint3
         * @see IPhysicsBody3
         * @see IPhysicsManager
         */
        class WPCore_API IPhysicsConstraint : public ISharedObject
        {
        public:
            /** Destructor */
            ~IPhysicsConstraint() override;

            /**
             * @brief Get the user data for the constraint.
             */
            void *getUserData() const override = 0;

            /**
             * @brief Set the user data for the constraint.
             * @param userData The user data to set.
             */
            void setUserData( void *userData ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsConstraint_h__
