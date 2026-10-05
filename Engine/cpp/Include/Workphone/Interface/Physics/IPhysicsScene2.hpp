#ifndef IPhysicsWorld2_h__
#define IPhysicsWorld2_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Interface for a 2D physics scene.
         *
         * This class represents a physics simulation environment in 2D space.
         * It manages all physics objects, handles collision detection, and simulates
         * physical interactions between objects in a 2D plane.
         *
         * The scene provides functionality for:
         * - Managing rigid bodies and particles
         * - Updating physics simulation
         * - Setting scene properties (gravity, size, etc.)
         * - Handling 2D physics interactions
         * - Managing collision detection and response
         *
         * @see IPhysicsManager2D
         * @see IRigidBody2
         * @see IPhysicsParticle2
         */
        class WPCore_API IPhysicsScene2 : public ISharedObject
        {
        public:
            /** Destructor */
            ~IPhysicsScene2() override;

            /**
             * @brief Updates all rigid bodies in the scene.
             *
             * This method advances the physics simulation for all rigid bodies
             * by one time step, applying forces and resolving collisions.
             */
            virtual void updateRigidBodies() = 0;

            /**
             * @brief Updates all particles in the scene.
             *
             * This method advances the physics simulation for all particles
             * by one time step, applying forces and resolving collisions.
             */
            virtual void updateParticles() = 0;

            /**
             * @brief Adds a rigid body to the scene.
             *
             * @param body A shared pointer to the rigid body to be added.
             *             The body will participate in physics simulation once added.
             */
            virtual void addRigidBody( SmartPtr<IRigidBody2> body ) = 0;

            /**
             * @brief Removes a rigid body from the scene.
             *
             * @param body A shared pointer to the rigid body to be removed.
             *             The body will no longer participate in physics simulation.
             */
            virtual void removeRigidBody( SmartPtr<IRigidBody2> body ) = 0;

            /**
             * @brief Adds a particle to the scene.
             *
             * @param particle A shared pointer to the particle to be added.
             *                 The particle will participate in physics simulation once added.
             */
            virtual void addParticle( SmartPtr<IPhysicsParticle2> particle ) = 0;

            /**
             * @brief Removes a particle from the scene.
             *
             * @param particle A shared pointer to the particle to be removed.
             *                 The particle will no longer participate in physics simulation.
             */
            virtual void removeParticle( SmartPtr<IPhysicsParticle2> particle ) = 0;

            /**
             * @brief Sets the size of the physics scene.
             *
             * @param size The new size of the scene in world units.
             */
            virtual void setSize( const Vector2<real_Num> &size ) = 0;

            /**
             * @brief Gets the size of the physics scene.
             *
             * @return The current size of the scene in world units.
             */
            virtual Vector2<real_Num> getSize() const = 0;

            /**
             * @brief Sets the gravity vector for the scene.
             *
             * @param gravity The gravity vector to be applied to all physics objects.
             */
            virtual void setGravity( const Vector2<real_Num> &gravity ) = 0;

            /**
             * @brief Gets the current gravity vector of the scene.
             *
             * @return The current gravity vector being applied to physics objects.
             */
            virtual Vector2<real_Num> getGravity() const = 0;

            /**
             * @brief Gets the underlying implementation object.
             * @param object A pointer to store the implementation object.
             */
            virtual void _getObject( void **object ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // IPhysicsWorld2_h__
