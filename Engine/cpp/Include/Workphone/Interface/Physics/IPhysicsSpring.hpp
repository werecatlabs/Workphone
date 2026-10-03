#ifndef IPhysicsSpring_h__
#define IPhysicsSpring_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a physics spring.
         *
         * This class represents a spring in the physics simulation.
         * Springs are used to create elastic connections between physics objects,
         * providing forces based on their stiffness and damping properties.
         *
         * The interface provides functionality for:
         * - Setting and getting spring stiffness
         * - Managing spring damping
         * - Controlling spring behavior
         *
         * Springs are commonly used for:
         * - Suspension systems in vehicles
         * - Rope and chain simulations
         * - Soft body physics
         * - Joint constraints
         *
         * @see IPhysicsConstraint
         * @see IPhysicsManager
         */
        class WPCore_API IPhysicsSpring : public ISharedObject
        {
        public:
            /** Destructor */
            ~IPhysicsSpring() override;

            /**
             * @brief Gets the stiffness of the spring.
             *
             * The stiffness determines how strongly the spring resists deformation.
             * Higher values make the spring stiffer, while lower values make it more flexible.
             *
             * @return The current stiffness value.
             */
            virtual real_Num getStiffness() const = 0;

            /**
             * @brief Sets the stiffness of the spring.
             *
             * The stiffness determines how strongly the spring resists deformation.
             * Higher values make the spring stiffer, while lower values make it more flexible.
             *
             * @param stiffness The new stiffness value to set.
             */
            virtual void setStiffness( real_Num stiffness ) = 0;

            /**
             * @brief Gets the damping of the spring.
             *
             * The damping determines how quickly the spring's oscillations decay.
             * Higher values make the spring's motion more damped, while lower values
             * allow more oscillation.
             *
             * @return The current damping value.
             */
            virtual real_Num getDamping() const = 0;

            /**
             * @brief Sets the damping of the spring.
             *
             * The damping determines how quickly the spring's oscillations decay.
             * Higher values make the spring's motion more damped, while lower values
             * allow more oscillation.
             *
             * @param damping The new damping value to set.
             */
            virtual void setDamping( real_Num damping ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsSpring_h__
