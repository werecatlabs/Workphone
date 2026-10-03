#ifndef IPhysicsParticle3_h__
#define IPhysicsParticle3_h__

#include "Workphone/Interface/Physics/IPhysicsBody3.hpp"
#include "Workphone/Interface/Physics/IPhysicsShape3.hpp"

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 3D physics particle.
         *
         * This class represents a particle in the 3D physics simulation.
         * Particles are lightweight physics objects that can be used for effects
         * like smoke, fire, or debris. They have position, velocity, and can
         * participate in collision detection.
         *
         * The interface provides functionality for:
         * - Managing particle properties (position, velocity, etc.)
         * - Handling collision detection
         * - Managing particle lifetime and state
         * - Controlling particle behavior
         *
         * @see IPhysicsBody3
         * @see IPhysicsShape3
         * @see IPhysicsManager
         */
        class WPCore_API IPhysicsParticle3 : public IPhysicsBody3
        {
        public:
            /** Destructor */
            ~IPhysicsParticle3() override;

            /**
             * @brief Gets the collision shape of the particle.
             * @return A reference to the smart pointer containing the collision shape.
             */
            virtual SmartPtr<IPhysicsShape3> &getCollisionShape() = 0;

            /**
             * @brief Gets the collision shape of the particle (const version).
             * @return A const reference to the smart pointer containing the collision shape.
             */
            virtual const SmartPtr<IPhysicsShape3> &getCollisionShape() const = 0;

            /**
             * @brief Sets the collision shape for the particle.
             * @param shape The collision shape to assign to the particle.
             */
            virtual void setCollisionShape( SmartPtr<IPhysicsShape3> shape ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsParticle3_h__
