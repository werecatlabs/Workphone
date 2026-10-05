/**
 * @file PhysicsManager.hpp
 * @brief Header file for the PhysicsManager class which manages physics simulation and objects
 * @author Zane Desir
 * @date 31/10/2021
 */

#ifndef WP_CPHYSICSMANAGER_H
#define WP_CPHYSICSMANAGER_H

#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class PhysicsManager
         * @brief Implementation of the IPhysicsManager interface that manages all physics-related
         * functionality
         *
         * The PhysicsManager is a central component responsible for:
         * - Managing physics objects (rigid bodies, constraints, vehicles, etc.)
         * - Handling physics simulation across multiple scenes
         * - Providing collision detection and raycasting capabilities
         * - Managing physics materials and debug visualization
         *
         * The manager maintains separate physics scenes for different purposes:
         * - Main physics simulation
         * - Character controls
         * - Soft body simulation
         * - Particle systems
         * - Raycasting
         * - Object interactions
         */
        class WPCore_API PhysicsManager : public IPhysicsManager
        {
        public:
            /**
             * @brief Default constructor
             * Initializes a new instance of the PhysicsManager
             */
            PhysicsManager();

            /**
             * @brief Destructor
             * Cleans up all physics resources and scenes
             */
            ~PhysicsManager() override;

            /**
             * @brief Loads physics configuration and resources
             * @param data Configuration data for physics setup
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads physics resources and cleans up
             * @param data Configuration data for cleanup
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets whether debug drawing is enabled
             * @return true if debug visualization is enabled, false otherwise
             */
            bool getEnableDebugDraw() const override;

            /**
             * @brief Sets whether debug drawing should be enabled
             * @param enableDebugDraw true to enable debug visualization, false to disable
             */
            void setEnableDebugDraw( bool enableDebugDraw ) override;

            /** @copydoc IPhysicsManager::debugDraw */
            void debugDraw() override;

            /** @copydoc IPhysicsManager::queueDebugForce */
            void queueDebugForce( hash_type bodyId, const Vector3<real_Num> &position,
                                  const Vector3<real_Num> &force ) override;

            /** @copydoc IPhysicsManager::getDebugForceScale */
            real_Num getDebugForceScale() const override;

            /** @copydoc IPhysicsManager::setDebugForceScale */
            void setDebugForceScale( real_Num scale ) override;

            /**
             * @brief Creates a new physics material
             * @return Smart pointer to the newly created physics material
             */
            SmartPtr<IPhysicsMaterial3> addMaterial() override;

            /**
             * @brief Removes a physics material
             * @param material The material to remove
             */
            void removeMaterial( SmartPtr<IPhysicsMaterial3> material ) override;

            /**
             * @brief Creates a new physics scene
             * @return Smart pointer to the newly created physics scene
             */
            SmartPtr<IPhysicsScene3> addScene() override;

            /**
             * @brief Removes a physics scene
             * @param scene The scene to remove
             */
            void removeScene( SmartPtr<IPhysicsScene3> scene ) override;

            /**
             * @brief Removes a collision shape
             * @param collisionShape The collision shape to remove
             * @return true if removal was successful, false otherwise
             */
            bool removeCollisionShape( SmartPtr<IPhysicsShape3> collisionShape ) override;

            /**
             * @brief Removes a physics body
             * @param body The physics body to remove
             * @return true if removal was successful, false otherwise
             */
            bool removePhysicsBody( SmartPtr<IRigidBody3> body ) override;

            /**
             * @brief Creates a new character controller
             * @return Smart pointer to the newly created character controller
             */
            SmartPtr<ICharacterController3> addCharacter() override;

            /**
             * @brief Creates a new static rigid body
             * @param transform Initial transform for the rigid body
             * @return Smart pointer to the newly created static rigid body
             */
            SmartPtr<IRigidStatic3> addRigidStatic( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Creates a new dynamic rigid body
             * @param transform Initial transform for the rigid body
             * @return Smart pointer to the newly created dynamic rigid body
             */
            SmartPtr<IRigidDynamic3> addRigidDynamic( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Creates a new static rigid body with a collision shape
             * @param collisionShape The collision shape to use
             * @return Smart pointer to the newly created static rigid body
             */
            SmartPtr<IRigidStatic3> addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape ) override;

            /**
             * @brief Creates a new static rigid body with a collision shape and properties
             * @param collisionShape The collision shape to use
             * @param properties Additional properties for the rigid body
             * @return Smart pointer to the newly created static rigid body
             */
            SmartPtr<IRigidStatic3> addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape,
                                                    SmartPtr<Properties> properties ) override;

            /**
             * @brief Performs a ray test in the physics world
             * @param start Starting point of the ray
             * @param direction Direction of the ray
             * @param hitPos Output parameter for hit position
             * @param hitNormal Output parameter for hit normal
             * @param collisionType Type of collision to test for
             * @param collisionMask Mask for collision filtering
             * @return true if ray hit something, false otherwise
             */
            bool rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                          Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal, u32 collisionType = 0,
                          u32 collisionMask = 0 ) override;

            /**
             * @brief Performs an intersection test between two points
             * @param start Starting point
             * @param end Ending point
             * @param hitPos Output parameter for hit position
             * @param hitNormal Output parameter for hit normal
             * @param object Output parameter for hit object
             * @param collisionType Type of collision to test for
             * @param collisionMask Mask for collision filtering
             * @return true if intersection found, false otherwise
             */
            bool intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                             Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                             SmartPtr<ISharedObject> &object, u32 collisionType = 0,
                             u32 collisionMask = 0 ) override;

            /**
             * @brief Creates a new D6 constraint between two physics bodies
             * @param actor0 First physics body
             * @param localFrame0 Local frame for first body
             * @param actor1 Second physics body
             * @param localFrame1 Local frame for second body
             * @return Smart pointer to the newly created constraint
             */
            SmartPtr<IConstraintD6> addConstraintD6( SmartPtr<IPhysicsBody3> actor0,
                                                     const Transform3<real_Num> &localFrame0,
                                                     SmartPtr<IPhysicsBody3> actor1,
                                                     const Transform3<real_Num> &localFrame1 ) override;

            /**
             * @brief Creates a new fixed constraint between two physics bodies
             * @param actor0 First physics body
             * @param localFrame0 Local frame for first body
             * @param actor1 Second physics body
             * @param localFrame1 Local frame for second body
             * @return Smart pointer to the newly created constraint
             */
            SmartPtr<IConstraintFixed3> addFixedConstraint(
                SmartPtr<IPhysicsBody3> actor0, const Transform3<real_Num> &localFrame0,
                SmartPtr<IPhysicsBody3> actor1, const Transform3<real_Num> &localFrame1 ) override;

            /**
             * @brief Removes a physics constraint
             * @param constraint The constraint to remove
             */
            void removeConstraint( SmartPtr<IPhysicsConstraint3> constraint ) override;

            /**
             * @brief Creates a new constraint drive
             * @return Smart pointer to the newly created constraint drive
             */
            SmartPtr<IConstraintDrive> addConstraintDrive() override;

            /**
             * @brief Creates a new linear limit constraint
             * @param extent The extent of the limit
             * @param contactDist The contact distance
             * @return Smart pointer to the newly created linear limit
             */
            SmartPtr<IConstraintLinearLimit> addConstraintLinearLimit(
                real_Num extent, real_Num contactDist = static_cast<real_Num>( -1.0 ) ) override;

            /**
             * @brief Creates new raycast hit data
             * @return Smart pointer to the newly created raycast hit data
             */
            SmartPtr<IRaycastHit> addRaycastHitData() override;

            /**
             * @brief Removes raycast hit data
             * @param raycastHitData The raycast hit data to remove
             */
            void removeRaycastHitData( SmartPtr<IRaycastHit> raycastHitData ) override;

            /**
             * @brief Gets the state task
             * @return The state task
             */
            TaskId getStateTask() const override;

            /**
             * @brief Gets the physics task
             * @return The physics task
             */
            TaskId getPhysicsTask() const override;

            /**
             * @brief Loads a physics object
             * @param object The object to load
             * @param forceQueue Whether to force queue the load operation
             */
            void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

            /**
             * @brief Unloads a physics object
             * @param object The object to unload
             * @param forceQueue Whether to force queue the unload operation
             */
            void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

            /**
             * @brief Gets the main physics scene
             * @return Smart pointer to the physics scene
             */
            SmartPtr<IPhysicsScene3> getPhysicsScene() const override;

            /**
             * @brief Sets the main physics scene
             * @param physicsScene The physics scene to set
             */
            void setPhysicsScene( SmartPtr<IPhysicsScene3> physicsScene ) override;

            /**
             * @brief Gets the objects scene
             * @return Smart pointer to the objects scene
             */
            SmartPtr<IPhysicsScene3> getObjectsScene() const override;

            /**
             * @brief Sets the objects scene
             * @param objectsScene The objects scene to set
             */
            void setObjectsScene( SmartPtr<IPhysicsScene3> objectsScene ) override;

            /**
             * @brief Gets the raycast scene
             * @return Smart pointer to the raycast scene
             */
            SmartPtr<IPhysicsScene3> getRaycastScene() const override;

            /**
             * @brief Sets the raycast scene
             * @param raycastScene The raycast scene to set
             */
            void setRaycastScene( SmartPtr<IPhysicsScene3> raycastScene ) override;

            /**
             * @brief Gets the controls scene
             * @return Smart pointer to the controls scene
             */
            SmartPtr<IPhysicsScene3> getControlsScene() const override;

            /**
             * @brief Sets the controls scene
             * @param controlsScene The controls scene to set
             */
            void setControlsScene( SmartPtr<IPhysicsScene3> controlsScene ) override;

            /**
             * @brief Locks the object for thread safety
             */
            void lock() override;

            /**
             * @brief Attempts to lock the object for thread safety
             * @return true if lock was acquired, false otherwise
             */
            bool try_lock() override;

            /**
             * @brief Unlocks the object
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            struct DebugForceCommand
            {
                hash_type bodyId = 0;
                Vector3<real_Num> position = Vector3<real_Num>::zero();
                Vector3<real_Num> force = Vector3<real_Num>::zero();
            };

            /** Returns the active graphics debug renderer, or null in headless configurations. */
            SmartPtr<render::IDebug> getDebugRenderer() const;

            /** Draws the world AABB and transformed local OBB for a body. */
            void drawDebugBody( render::IDebug &debug, const IRigidBody3 &body ) const;

            /** Drains and draws forces recorded by queueDebugForce. */
            void drawQueuedDebugForces( render::IDebug &debug );

            SmartPtr<IPhysicsScene3> m_physicsScene;   ///< Main physics simulation scene
            SmartPtr<IPhysicsScene3> m_controlsScene;  ///< Scene for character controls
            SmartPtr<IPhysicsScene3> m_softbodyScene;  ///< Scene for soft body simulation
            SmartPtr<IPhysicsScene3> m_particleScene;  ///< Scene for particle systems
            SmartPtr<IPhysicsScene3> m_raycastScene;   ///< Scene for raycasting
            SmartPtr<IPhysicsScene3> m_objectsScene;   ///< Scene for object interactions

            Array<SmartPtr<IPhysicsConstraint3>> m_constraints;  ///< List of active physics constraints

            ConcurrentQueue<DebugForceCommand> m_debugForces;  ///< Cross-thread force draw queue.
            atomic_f32 m_debugForceScale = 0.01f;              ///< Force-to-world-line scale.
            atomic_bool m_debugDrawEnabled = false;            ///< Base implementation toggle.

            mutable RecursiveSpinMutex m_mutex;  ///< Mutex for thread safety
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // WP_CPHYSICSMANAGER_H
