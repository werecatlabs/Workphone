#ifndef __IPhysicsParticle2__H
#define __IPhysicsParticle2__H

#include "Workphone/Interface/Physics/IPhysicsBody2D.hpp"
#include "Workphone/Interface/Physics/IPhysicsShape2.hpp"
#include "Workphone/Math/AABB2.hpp"
#include "Workphone/Math/Vector2.hpp"
#include "Workphone/Math/Quaternion.hpp"
#include "Workphone/Core/StringTypes.hpp"

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Interface for a 2D physics particle.
         *
         * This class represents a particle in the 2D physics simulation.
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
         * @see IPhysicsBody2D
         * @see IPhysicsShape2
         * @see IPhysicsManager2D
         */
        class WPCore_API IPhysicsParticle2 : public IPhysicsBody2D
        {
        public:
            /** Destructor */
            ~IPhysicsParticle2() override;

            /**
             * @brief Sets the collision shape for the particle.
             *
             * @param shape The collision shape to attach to the particle.
             */
            virtual void setCollisionShape( SmartPtr<IPhysicsShape2> shape ) = 0;

            /**
             * @brief Gets the collision shape of the particle.
             *
             * @return A shared pointer to the particle's collision shape.
             */
            virtual const SmartPtr<IPhysicsShape2> &getCollisionShape() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif
