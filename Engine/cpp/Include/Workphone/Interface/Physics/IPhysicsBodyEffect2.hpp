#ifndef IPhysicsBodyEffect2_h__
#define IPhysicsBodyEffect2_h__

#include <Workphone/Interface/Physics/IPhysicsEffect2.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Interface for physics body effects in 2D.
         *
         * This class represents an effect that can be applied to a physics body in 2D space.
         * Effects can include forces like wind, gravity, or other environmental influences
         * that affect the behavior of physics bodies.
         *
         * The interface provides functionality for:
         * - Managing the owner body of the effect
         * - Applying forces and influences to bodies
         * - Controlling effect properties and behavior
         *
         * @see IPhysicsEffect2
         * @see IPhysicsBody2D
         * @see IPhysicsManager2D
         */
        class WPCore_API IPhysicsBodyEffect2 : public IPhysicsEffect2
        {
        public:
            /** Destructor */
            ~IPhysicsBodyEffect2() override;

            /**
             * @brief Gets the physics body that owns this effect.
             * @return A pointer to the owner physics body.
             */
            virtual IPhysicsBody2D *getOwner() const = 0;

            /**
             * @brief Sets the physics body that owns this effect.
             * @param owner A pointer to the physics body to set as owner.
             */
            virtual void setOwner( IPhysicsBody2D *owner ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsBodyEffect2_h__
