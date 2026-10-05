#ifndef __IPhysicsManager2d__H
#define __IPhysicsManager2d__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Interface for managing 2D physics simulation and objects.
         *
         * This class serves as the central manager for all physics-related functionality in a 2D
         * environment. It provides methods for creating and managing physics scenes, shapes, bodies, and
         * particles. The manager handles the lifecycle of physics objects and coordinates their
         * interactions.
         *
         * Key responsibilities include:
         * - Creating and managing physics scenes (worlds)
         * - Creating and managing collision shapes
         * - Managing rigid bodies and particles
         * - Handling physics simulation updates
         * - Managing collision detection
         *
         * @see IPhysicsScene2
         * @see IPhysicsShape2
         * @see IRigidBody2
         * @see IPhysicsParticle2
         */
        class WPCore_API IPhysicsManager2D : public ISharedObject
        {
        public:
            /** Destructor */
            ~IPhysicsManager2D() override;

            /**
             * @brief Updates all rigid bodies in the physics manager.
             *
             * This method advances the physics simulation for all rigid bodies
             * by one time step, applying forces and resolving collisions.
             * Can be called from a separate thread for parallel processing.
             */
            virtual void updateRigidBodies() = 0;

            /**
             * @brief Updates all particles in the physics manager.
             *
             * This method advances the physics simulation for all particles
             * by one time step, applying forces and resolving collisions.
             * Can be called from a separate thread for parallel processing.
             */
            virtual void updateParticles() = 0;

            /**
             * @brief Creates a new physics scene (world) with the specified ID.
             *
             * @param id The unique identifier for the new physics scene.
             * @return A shared pointer to the newly created physics scene.
             */
            virtual SmartPtr<IPhysicsScene2> addWorld( u32 id ) = 0;

            /**
             * @brief Removes a physics scene from the manager.
             *
             * @param world A shared pointer to the physics scene to be removed.
             */
            virtual void removeWorld( SmartPtr<IPhysicsScene2> world ) = 0;

            /**
             * @brief Finds a physics scene by its ID.
             *
             * @param id The ID of the physics scene to find.
             * @return A shared pointer to the found physics scene, or null if not found.
             */
            virtual SmartPtr<IPhysicsScene2> findWorld( u32 id ) const = 0;

            /**
             * @brief Gets all physics scenes managed by this manager.
             *
             * @return An array of shared pointers to all physics scenes.
             */
            virtual Array<SmartPtr<IPhysicsScene2>> getWorlds() const = 0;

            /**
             * @brief Creates a collision shape based on the specified type.
             *
             * Creates a collision shape with default properties based on the type hash.
             *
             * @param type The type hash of the collision shape to create.
             * @return A shared pointer to the newly created collision shape.
             */
            virtual SmartPtr<IPhysicsShape2> createCollisionShapeByType( hash32 type ) = 0;

            /**
             * @brief Creates a collision shape of the specified template type.
             *
             * @tparam T The type of collision shape to create.
             * @return A shared pointer to the newly created collision shape.
             */
            template <class T>
            SmartPtr<T> createCollisionShape()
            {
                auto typeInfo = T::typeInfo();
                auto typeHash = typeInfo->getHash();
                auto shape = createCollisionShapeByType( typeHash );
                return workphone::static_pointer_cast<T>( shape );
            }

            /**
             * @brief Creates a new rigid body.
             *
             * @return A shared pointer to the newly created rigid body.
             */
            virtual SmartPtr<IRigidBody2> createRigidBody() = 0;

            /**
             * @brief Creates a new rigid body with the specified collision shape.
             *
             * @param collisionShape The collision shape to attach to the rigid body.
             * @return A shared pointer to the newly created rigid body.
             */
            virtual SmartPtr<IRigidBody2> createRigidBody( SmartPtr<IPhysicsShape2> collisionShape ) = 0;

            /**
             * @brief Creates a new particle with the specified type and collision shape.
             *
             * @param particleType The type of particle to create.
             * @param collisionShape The collision shape to attach to the particle.
             * @return A shared pointer to the newly created particle.
             */
            virtual SmartPtr<IPhysicsParticle2> createParticle(
                u8 particleType, SmartPtr<IPhysicsShape2> collisionShape ) = 0;

            /**
             * @brief Creates a new soft body.
             *
             * @return A shared pointer to the newly created soft body.
             */
            virtual SmartPtr<IPhysicsSoftBody2> createSoftBody() = 0;

            /**
             * @brief Clears all physics objects from the manager.
             *
             * This removes all scenes, bodies, shapes, and particles from the manager.
             */
            virtual void clear() = 0;

            /**
             * @brief Gets a particle by its ID.
             *
             * @param id The ID of the particle to find.
             * @return A shared pointer to the found particle, or null if not found.
             */
            virtual SmartPtr<IPhysicsParticle2> getParticle( u32 id ) const = 0;

            /**
             * @brief Called when the flags of a physics body change.
             *
             * @param body The physics body whose flags have changed.
             */
            virtual void OnChangeFlags( IPhysicsBody2D *body ) = 0;

            /**
             * @brief Checks if two physics bodies are colliding.
             *
             * @param bodyA The first physics body to check.
             * @param bodyB The second physics body to check.
             * @return True if the bodies are colliding, false otherwise.
             */
            virtual bool isColliding( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB ) const = 0;

            /**
             * @brief Gets the underlying implementation object.
             * @param object A pointer to store the implementation object.
             */
            virtual void _getObject( void **object ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif
