#ifndef __IPhysicsManager3__H
#define __IPhysicsManager3__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for managing 3D physics simulation and objects.
         *
         * This class serves as the central manager for all physics-related functionality in a 3D
         * environment. It provides methods for creating and managing physics scenes, materials, shapes,
         * bodies, and constraints. The manager handles the lifecycle of physics objects and coordinates
         * their interactions.
         *
         * Key responsibilities include:
         * - Creating and managing physics scenes
         * - Managing physics materials and their properties
         * - Creating and managing collision shapes
         * - Managing rigid bodies and character controllers
         * - Handling physics constraints and joints
         * - Managing vehicle physics
         * - Debug visualization
         */
        class WPCore_API IPhysicsManager : public ISharedObject
        {
        public:
            /** Destructor. */
            ~IPhysicsManager() override;

            /**
             * @brief Gets whether debug visualization is enabled.
             * @return True if debug visualization is enabled, false otherwise.
             */
            virtual bool getEnableDebugDraw() const = 0;

            /**
             * @brief Sets whether debug visualization is enabled.
             * @param enableDebugDraw True to enable debug visualization, false to disable.
             */
            virtual void setEnableDebugDraw( bool enableDebugDraw ) = 0;

            /**
             * Submits the current physics debug representation to the graphics debug
             * renderer.
             * Implementations draw managed body bounds and any forces queued
             * since the last call.
             */
            virtual void debugDraw() = 0;

            /**
             * Records a force for drawing at the next physics debug submission.
             * This
             * entry point is thread-safe so forces may be recorded from simulation workers.
             */
            virtual void queueDebugForce( hash_type bodyId, const Vector3<real_Num> &position,
                                          const Vector3<real_Num> &force ) = 0;

            /** @brief Gets the world-space line scale applied to force vectors. */
            virtual real_Num getDebugForceScale() const = 0;

            /** @brief Sets the world-space line scale applied to force vectors. */
            virtual void setDebugForceScale( real_Num scale ) = 0;

            /**
             * @brief Creates a new physics material with default properties.
             * @return A smart pointer to the newly created physics material.
             */
            virtual SmartPtr<IPhysicsMaterial3> addMaterial() = 0;

            /**
             * @brief Removes a physics material from the manager.
             * @param material The material to remove.
             */
            virtual void removeMaterial( SmartPtr<IPhysicsMaterial3> material ) = 0;

            /**
             * @brief Creates a new physics scene with default properties.
             * @return A smart pointer to the newly created physics scene.
             */
            virtual SmartPtr<IPhysicsScene3> addScene() = 0;

            /**
             * @brief Removes a physics scene from the manager.
             * @param scene The scene to remove.
             */
            virtual void removeScene( SmartPtr<IPhysicsScene3> scene ) = 0;

            /**
             * @brief Creates a collision shape based on the specified type and data.
             * @param type The type of collision shape to create.
             * @param data Additional data needed to create the shape.
             * @return A smart pointer to the newly created collision shape.
             */
            virtual SmartPtr<IPhysicsShape3> addCollisionShapeByType( hash64 type,
                                                                      SmartPtr<ISharedObject> data ) = 0;

            /**
             * @brief Removes a collision shape from the manager.
             * @param collisionShape The collision shape to remove.
             * @return True if the shape was successfully removed, false otherwise.
             */
            virtual bool removeCollisionShape( SmartPtr<IPhysicsShape3> collisionShape ) = 0;

            /**
             * @brief Removes a physics body from the manager.
             * @param body The physics body to remove.
             * @return True if the body was successfully removed, false otherwise.
             */
            virtual bool removePhysicsBody( SmartPtr<IRigidBody3> body ) = 0;

            /**
             * @brief Creates a new character controller.
             * @return A smart pointer to the newly created character controller.
             */
            virtual SmartPtr<ICharacterController3> addCharacter() = 0;

            /**
             * @brief Creates a new static rigid body at the specified transform.
             * @param transform The initial transform of the rigid body.
             * @return A smart pointer to the newly created static rigid body.
             */
            virtual SmartPtr<IRigidStatic3> addRigidStatic( const Transform3<real_Num> &transform ) = 0;

            /**
             * @brief Creates a new dynamic rigid body at the specified transform.
             * @param transform The initial transform of the rigid body.
             * @return A smart pointer to the newly created dynamic rigid body.
             */
            virtual SmartPtr<IRigidDynamic3> addRigidDynamic(
                const Transform3<real_Num> &transform ) = 0;

            /**
             * @brief Creates a new static rigid body with the specified collision shape.
             * @param collisionShape The collision shape to attach to the rigid body.
             * @return A smart pointer to the newly created static rigid body.
             */
            virtual SmartPtr<IRigidStatic3> addRigidStatic(
                SmartPtr<IPhysicsShape3> collisionShape ) = 0;

            /**
             * @brief Creates a new static rigid body with the specified collision shape and properties.
             * @param collisionShape The collision shape to attach to the rigid body.
             * @param properties Additional properties for the rigid body.
             * @return A smart pointer to the newly created static rigid body.
             */
            virtual SmartPtr<IRigidStatic3> addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape,
                                                            SmartPtr<Properties> properties ) = 0;

            /**
             * @brief Creates a new vehicle with the specified chassis.
             * @param chassis The rigid body to use as the vehicle's chassis.
             * @return A smart pointer to the newly created vehicle.
             */
            virtual SmartPtr<IPhysicsVehicle3> addVehicle( SmartPtr<IRigidBody3> chassis ) = 0;

            /**
             * @brief Removes a vehicle from the manager.
             * @param vehicle The vehicle to remove.
             * @return True if the vehicle was successfully removed, false otherwise.
             */
            virtual bool removeVehicle( SmartPtr<IPhysicsVehicle3> vehicle ) = 0;

            /**
             * @brief Creates a new D6 constraint between two bodies.
             * @param actor0 The first body involved in the constraint.
            /** Adds a vehicle. */
            virtual SmartPtr<IPhysicsVehicle3> addVehicle( SmartPtr<IRigidBody3> chassis,
                                                           const SmartPtr<Properties> &properties ) = 0;

            /** Performs a ray intersection test. */
            virtual bool rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                                  Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                  u32 collisionType = 0, u32 collisionMask = 0 ) = 0;

            /**
             * @brief Performs a line intersection test.
             * @param start The start point of the line.
             * @param end The end point of the line.
             * @param hitPos The hit position of the intersection.
             * @param hitNormal The surface normal of the intersection.
             * @param object The object that intersects.
             * @param collisionType The collision type.
             * @param collisionMask The collision mask.
             */
            virtual bool intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                                     Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                                     SmartPtr<ISharedObject> &object, u32 collisionType = 0,
                                     u32 collisionMask = 0 ) = 0;

            /** Creates a 6 degree of freedom joint. */
            virtual SmartPtr<IConstraintD6> addConstraintD6(
                SmartPtr<IPhysicsBody3> actor0, const Transform3<real_Num> &localFrame0,
                SmartPtr<IPhysicsBody3> actor1, const Transform3<real_Num> &localFrame1 ) = 0;

            /** Creates a fixed joint. */
            virtual SmartPtr<IConstraintFixed3> addFixedConstraint(
                SmartPtr<IPhysicsBody3> actor0, const Transform3<real_Num> &localFrame0,
                SmartPtr<IPhysicsBody3> actor1, const Transform3<real_Num> &localFrame1 ) = 0;

            /** Removes a constraint. */
            virtual void removeConstraint( SmartPtr<IPhysicsConstraint3> constraint ) = 0;

            /** Creates a constraint drive. */
            virtual SmartPtr<IConstraintDrive> addConstraintDrive() = 0;

            /** Creates a constraint linear limit. */
            virtual SmartPtr<IConstraintLinearLimit> addConstraintLinearLimit(
                real_Num extent, real_Num contactDist = static_cast<real_Num>( -1.0 ) ) = 0;

            /** Creates raycast hit data. */
            virtual SmartPtr<IRaycastHit> addRaycastHitData() = 0;

            /** Remove raycast hit data. */
            virtual void removeRaycastHitData( SmartPtr<IRaycastHit> raycastHitData ) = 0;

            /** The task used to update. */
            virtual TaskId getStateTask() const = 0;

            /** The task the physics is updated on. */
            virtual TaskId getPhysicsTask() const = 0;

            /**
             * @brief Loads a physics object.
             * @param object The object to be loaded.
             * @param forceQueue Forces the object to be queued for deferred loading.
             */
            virtual void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) = 0;

            /** Unloads a physics object.
             * @param object The object to be unloaded.
             * @param forceQueue Forces the object to be queued for deferred unloading.
             */
            virtual void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) = 0;

            /**
             * Get the physics scene for the application.
             * @return The physics scene for the application.
             */
            virtual SmartPtr<IPhysicsScene3> getPhysicsScene() const = 0;

            /**
             * Set the physics scene for the application.
             * @param physicsScene The new physics scene for the application.
             */
            virtual void setPhysicsScene( SmartPtr<IPhysicsScene3> physicsScene ) = 0;

            /**
             * Get the objects scene for the application.
             * @return The objects scene for the application.
             */
            virtual SmartPtr<IPhysicsScene3> getObjectsScene() const = 0;

            /**
             * Set the objects scene for the application.
             * @param objectsScene The new objects scene for the application.
             */
            virtual void setObjectsScene( SmartPtr<IPhysicsScene3> objectsScene ) = 0;

            /**
             * Get the raycast scene for the application.
             * @return The raycast scene for the application.
             */
            virtual SmartPtr<IPhysicsScene3> getRaycastScene() const = 0;

            /**
             * Set the raycast scene for the application.
             * @param raycastScene The new raycast scene for the application.
             */
            virtual void setRaycastScene( SmartPtr<IPhysicsScene3> raycastScene ) = 0;

            /**
             * Get the controls scene for the application.
             * @return The controls scene for the application.
             */
            virtual SmartPtr<IPhysicsScene3> getControlsScene() const = 0;

            /**
             * Set the controls scene for the application.
             * @param controlsScene The new controls scene for the application.
             */
            virtual void setControlsScene( SmartPtr<IPhysicsScene3> controlsScene ) = 0;

            /**
             * @brief Creates a collision shape of the specified template type.
             * @tparam T The type of collision shape to create.
             * @param data Additional data needed to create the shape.
             * @return A smart pointer to the newly created collision shape.
             */
            template <class T>
            SmartPtr<T> addCollisionShape( SmartPtr<ISharedObject> data );

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        SmartPtr<T> IPhysicsManager::addCollisionShape( SmartPtr<ISharedObject> data )
        {
            auto typeInfo = T::typeInfo();
            WP_ASSERT( typeInfo != 0 );

            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            auto typeHash = typeManager->getHash( typeInfo );
            WP_ASSERT( typeHash != 0 );

            auto shape = addCollisionShapeByType( typeHash, data );
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( shape ) );
            return workphone::static_pointer_cast<T>( shape );
        }
    }  // namespace physics
}  // namespace workphone

#endif
