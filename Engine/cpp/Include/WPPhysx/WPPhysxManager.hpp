#ifndef __WPPhysxManager_h__
#define __WPPhysxManager_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>

#define MAX_NUM_INDEX_BUFFERS 16
#define NUM_PLAYER_CARS 1
#define NUM_NONPLAYER_4W_VEHICLES 0

namespace workphone
{
    namespace physics
    {

        /**
         * @brief PhysX backend implementation of the engine physics manager.
         *
         * This class wraps and owns the platform-specific PhysX objects required
         * to run the physics simulation (foundation, PxPhysics, cooker, vehicle
         * manager, dispatchers, materials, scenes, and managed shapes/bodies).
         *
         * It implements the generic PhysicsManager interface used by the engine
         * so higher-level systems remain renderer/physics-backend agnostic.
         *
         * Responsibilities:
         *  - Initialize and hold references to PhysX subsystems.
         *  - Create/destroy physics shapes, bodies, scenes and vehicles.
         *  - Provide query helpers (raycasts, intersection tests).
         *  - Manage a load/unload queue for asynchronous object lifecycle.
         *
         * @author Zane Desir
         * @version 1.0
         */
        class PhysxManager : public PhysicsManager
        {
        public:
            /**
             * @brief Construct a new PhysxManager.
             *
             * Does not perform heavy initialization � use load() to initialize
             * the internal PhysX objects and resources.
             */
            PhysxManager();

            /**
             * @brief Destroy the PhysxManager.
             *
             * Ensures proper release of PhysX resources and internal containers.
             */
            ~PhysxManager() override;

            /**
             * @copydoc PhysicsManager::load
             *
             * Initializes PhysX subsystems and resources. The optional data
             * parameter can be used to pass configuration or initialization
             * objects (materials, settings, etc.).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc PhysicsManager::unload
             *
             * Tears down PhysX subsystems and releases resources created by load().
             * Will also clear internal queues and managed objects.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc PhysicsManager::preUpdate
             *
             * Perform any per-frame preparations required before the physics
             * simulation step (e.g. processing pending loads).
             */
            void preUpdate() override;

            /**
             * @copydoc PhysicsManager::update
             *
             * Advance the physics simulation. This is where scenes are stepped
             * and simulation results are produced.
             */
            void update() override;

            /**
             * @copydoc PhysicsManager::postUpdate
             *
             * Perform any processing required after physics simulation completes,
             * such as writing back simulation results to game objects.
             */
            void postUpdate() override;

            /**
             * @copydoc PhysicsManager::addScene
             *
             * Create and return a new PhysX-backed physics scene.
             *
             * @return SmartPtr<IPhysicsScene3> New physics scene instance.
             */
            SmartPtr<IPhysicsScene3> addScene() override;

            /**
             * @copydoc PhysicsManager::removeScene
             *
             * Remove and destroy the supplied scene instance.
             *
             * @param scene Scene to remove.
             */
            void removeScene( SmartPtr<IPhysicsScene3> scene ) override;

            /**
             * @copydoc PhysicsManager::getEnableDebugDraw
             *
             * @return true if debug visualization (wireframes, contacts) is enabled.
             */
            bool getEnableDebugDraw() const override;

            /**
             * @copydoc PhysicsManager::setEnableDebugDraw
             *
             * Enable or disable debug visualization for physics objects.
             *
             * @param enableDebugDraw Set to true to enable debug drawing.
             */
            void setEnableDebugDraw( bool enableDebugDraw ) override;

            /** @copydoc IPhysicsManager::debugDraw */
            void debugDraw() override;

            /**
             * @copydoc IPhysicsManager3::createMaterial
             *
             * Create and register a default physics material.
             *
             * @return SmartPtr<IPhysicsMaterial3> Created material.
             */
            SmartPtr<IPhysicsMaterial3> addMaterial() override;

            /**
             * @copydoc IPhysicsManager3::destroyMaterial
             *
             * Destroy and unregister a material previously created by addMaterial().
             *
             * @param material Material to remove.
             */
            void removeMaterial( SmartPtr<IPhysicsMaterial3> material ) override;

            /**
             * @copydoc IPhysicsManager3::createCollisionShapeByType
             *
             * Create a collision shape based on a hashed type identifier and
             * optional initialization data.
             *
             * @param type Hash identifier for the desired shape type.
             * @param data Optional shape-specific initialization data.
             * @return SmartPtr<IPhysicsShape3> The created shape or null on failure.
             */
            SmartPtr<IPhysicsShape3> addCollisionShapeByType( hash64                  type,
                                                              SmartPtr<ISharedObject> data );

            /**
             * @brief Create a static rigid actor at the provided transform.
             *
             * @param transform World transform for the static actor.
             * @return SmartPtr<IRigidStatic3> Created static rigid actor.
             */
            SmartPtr<IRigidStatic3> addRigidStatic( const Transform3<physics_Num> &transform ) override;

            /**
             * @brief Create a dynamic rigid actor at the provided transform.
             *
             * @param transform World transform for the dynamic actor.
             * @return SmartPtr<IRigidDynamic3> Created dynamic rigid actor.
             */
            SmartPtr<IRigidDynamic3> addRigidDynamic(
                const Transform3<physics_Num> &transform ) override;

            /**
             * @brief Create a static rigid actor using the provided collision shape.
             *
             * @param collisionShape Collision shape to attach to the static actor.
             * @return SmartPtr<IRigidStatic3> Created static rigid actor.
             */
            SmartPtr<IRigidStatic3> addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape ) override;

            /**
             * @brief Create a static rigid actor using the provided collision shape and properties.
             *
             * @param collisionShape Shape to use for the rigid.
             * @param properties Properties (mass, friction hints, etc.) used to configure the actor.
             * @return SmartPtr<IRigidStatic3> Created static rigid actor.
             */
            SmartPtr<IRigidStatic3> addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape,
                                                    SmartPtr<Properties>     properties ) override;

            /**
             * @brief Create a vehicle from a chassis rigid and properties.
             *
             * @param chassis Rigid body that represents the vehicle chassis.
             * @param properties Vehicle configuration properties.
             * @return SmartPtr<IPhysicsVehicle3> Created vehicle instance.
             */
            SmartPtr<IPhysicsVehicle3> addVehicle( SmartPtr<IRigidBody3>       chassis,
                                                   const SmartPtr<Properties> &properties ) override;

            /**
             * @brief Create a vehicle using a scene director template.
             *
             * @param vehicleTemplate Template director containing vehicle data.
             * @return SmartPtr<IPhysicsVehicle3> Created vehicle instance.
             */
            SmartPtr<IPhysicsVehicle3> addVehicle( SmartPtr<IBuildDirector> vehicleTemplate );

            /**
             * @brief Remove and destroy a previously created collision shape.
             *
             * @param collisionShape Shape to remove.
             * @return true if removed successfully, false otherwise.
             */
            bool removeCollisionShape( SmartPtr<IPhysicsShape3> collisionShape ) override;

            /**
             * @brief Remove and destroy a physics body.
             *
             * @param body Body to remove.
             * @return true if removed successfully.
             */
            bool removePhysicsBody( SmartPtr<IRigidBody3> body ) override;

            /**
             * @brief Remove and destroy a vehicle.
             *
             * @param vehicle Vehicle to remove.
             * @return true if removed successfully.
             */
            bool removeVehicle( SmartPtr<IPhysicsVehicle3> vehicle ) override;

            /**
             * @brief Create and return a character controller instance.
             *
             * @return SmartPtr<ICharacterController3> New character controller.
             */
            SmartPtr<ICharacterController3> addCharacter() override;

            /**
             * @brief Perform a ray test in the physics world.
             *
             * Casts a ray from start along direction, returning the first hit
             * position and normal. Optional collision filters can be supplied.
             *
             * @param start Ray origin.
             * @param direction Ray direction (should be normalized or scaled to desired length).
             * @param hitPos Output hit position if any.
             * @param hitNormal Output surface normal at hit.
             * @param collisionType Optional collision type filter.
             * @param collisionMask Optional collision mask filter.
             * @return true if something was hit.
             */
            bool rayTest( const Vector3<physics_Num> &start, const Vector3<physics_Num> &direction,
                          Vector3<physics_Num> &hitPos, Vector3<physics_Num> &hitNormal,
                          u32 collisionType = 0, u32 collisionMask = 0 ) override;

            /**
             * @brief Test intersection between a segment and scene geometry.
             *
             * Returns first hit information and the object that was intersected.
             *
             * @param start Segment start.
             * @param end Segment end.
             * @param hitPos Output hit position.
             * @param hitNormal Output hit normal.
             * @param object Output pointer to intersected object.
             * @param collisionType Optional collision type filter.
             * @param collisionMask Optional collision mask filter.
             * @return true if intersection occurred.
             */
            bool intersects( const Vector3<physics_Num> &start, const Vector3<physics_Num> &end,
                             Vector3<physics_Num> &hitPos, Vector3<physics_Num> &hitNormal,
                             SmartPtr<ISharedObject> &object, u32 collisionType = 0,
                             u32 collisionMask = 0 ) override;

            /**
             * @copydoc IPhysicsManager3::d6JointCreate
             *
             * Create a 6-DoF (D6) constraint between two physics bodies.
             */
            SmartPtr<IConstraintD6> addConstraintD6(
                SmartPtr<IPhysicsBody3> actor0, const Transform3<physics_Num> &localFrame0,
                SmartPtr<IPhysicsBody3> actor1, const Transform3<physics_Num> &localFrame1 ) override;

            /**
             * @copydoc IPhysicsManager3::fixedJointCreate
             *
             * Create a fixed constraint (no relative motion) between two bodies.
             */
            SmartPtr<IConstraintFixed3> addFixedConstraint(
                SmartPtr<IPhysicsBody3> actor0, const Transform3<physics_Num> &localFrame0,
                SmartPtr<IPhysicsBody3> actor1, const Transform3<physics_Num> &localFrame1 ) override;

            /**
             * @brief Create a drive constraint helper.
             *
             * @return SmartPtr<IConstraintDrive> Drive constraint instance.
             */
            SmartPtr<IConstraintDrive> addConstraintDrive() override;

            /**
             * @brief Create a linear limit constraint helper.
             *
             * @param extent Limit extent.
             * @param contactDist Optional contact distance (defaults to -1 for automatic).
             * @return SmartPtr<IConstraintLinearLimit> Linear limit instance.
             */
            SmartPtr<IConstraintLinearLimit> addConstraintLinearLimit(
                physics_Num extent, physics_Num contactDist = physics_Num( -1.0 ) ) override;

            /**
             * @brief Create raycast hit data container.
             *
             * @return SmartPtr<IRaycastHit> New raycast hit container.
             */
            SmartPtr<IRaycastHit> addRaycastHitData() override;

            /**
             * @copydoc IPhysicsManager3::getStateTask
             *
             * Returns the task identifier used for synchronizing state updates.
             */
            TaskId getStateTask() const override;

            /**
             * @copydoc IPhysicsManager3::getPhysicsTask
             *
             * Returns the task identifier used for the physics simulation task.
             */
            TaskId getPhysicsTask() const override;

            /**
             * @brief Queue an object for loading into the physics system.
             *
             * @param object Object to load.
             * @param forceQueue If true, forces queueing even if immediate load is possible.
             */
            void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

            /**
             * @brief Queue an object for unloading from the physics system.
             *
             * @param object Object to unload.
             * @param forceQueue If true, forces queueing even if immediate unload is possible.
             */
            void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

            /**
             * @brief Access raw PxPhysics pointer.
             *
             * @return physx::PxPhysics* Raw pointer to the PxPhysics instance (may be nullptr).
             */
            physx::PxPhysics *getPhysics() const;

            /**
             * @brief Set the internal PxPhysics pointer.
             *
             * Ownership semantics are not transferred; caller must ensure lifetime
             * is managed appropriately.
             */
            void setPhysics( physx::PxPhysics *physics );

            /**
             * @brief Access raw PxCooking pointer.
             * @return physx::PxCooking* Raw pointer to the PxCooking instance.
             */
            physx::PxCooking *getCooking() const;

            /**
             * @brief Set the internal PxCooking pointer.
             *
             * Ownership semantics are not transferred; caller must ensure lifetime
             * is managed appropriately.
             */
            void setCooking( physx::PxCooking *cooking );

            /**
             * @brief Get the engine default PhysX material.
             * @return physx::PxMaterial* Default material pointer.
             */
            physx::PxMaterial *getDefaultMaterial() const;

            /**
             * @brief Set the engine default PhysX material.
             *
             * Ownership semantics are not transferred; caller must ensure lifetime
             * is managed appropriately.
             */
            void setDefaultMaterial( physx::PxMaterial *defaultMaterial );

            /**
             * @brief Access the PhysxVehicleManager used for vehicle simulations.
             *
             * @return PhysxVehicleManager* Pointer to the vehicle manager.
             */
            PhysxVehicleManager *getVehicleManager() const;

            /**
             * @brief Set the PhysxVehicleManager used for vehicle simulations.
             * @param vehicleManager Pointer to the vehicle manager to set.
             */
            void setVehicleManager( PhysxVehicleManager *vehicleManager );

            /**
             * @brief Access the PhysxCooker helper.
             *
             * @return PhysxCooker* Pointer to cooker utility.
             */
            PhysxCooker *getCooker() const;

            /**
             * @brief Set the PhysxCooker helper.
             */
            void setCooker( PhysxCooker *cooker );

            /**
             * @brief Get pointer to the array of standard materials.
             * @return physx::PxMaterial* const Pointer to the material array.
             */
            physx::PxMaterial *getStandardMaterials() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Create a sphere collision shape and register it. */
            SmartPtr<ISphereShape3> createSphere();

            /** Create a box collision shape and register it. */
            SmartPtr<IBoxShape3> createBox();

            /** Create a plane collision shape and register it. */
            SmartPtr<IPlaneShape3> createPlane();

            /** Create a mesh collision shape and register it. */
            SmartPtr<IMeshShape> createMesh();

            /** Create a terrain collision shape and register it. */
            SmartPtr<ITerrainShape> createTerrain();

            /** Create a terrain shape from a scene director template. */
            SmartPtr<ITerrainShape> createTerrain( SmartPtr<IBuildDirector> objectTemplate );

            /** Create an empty rigid body wrapper. */
            SmartPtr<IRigidBody3> createRigidBody();

            /** Create a rigid body with the supplied collision shape. */
            SmartPtr<IRigidBody3> createRigidBody( SmartPtr<IPhysicsShape3> collisionShape );

            /** Create a rigid body with the supplied shape and properties. */
            SmartPtr<IRigidBody3> createRigidBody( SmartPtr<IPhysicsShape3> collisionShape,
                                                   SmartPtr<Properties>     properties );

            /** Create a rigid body at the specified transform. */
            SmartPtr<IRigidBody3> createRigidBody( const Transform3<physics_Num> &transform );

            /** Custom allocator used by PhysX. */
            SharedPtr<physx::PxAllocatorCallback> m_allocator;

            /** Error callback used by PhysX for logging errors/warnings. */
            SharedPtr<physx::PxErrorCallback> m_errorOutput;

            /** PhysX controller manager (raw pointer from SDK). */
            physx::PxControllerManager *m_controllerManager;

            /** PhysX foundation pointer (non-owning raw pointer wrapper). */
            RawPtr<physx::PxFoundation> m_foundation;

            /** Atomic wrapper around the PxPhysics pointer to support threaded access. */
            AtomicRawPtr<physx::PxPhysics> m_physics;

            /** Cooking interface used for mesh/convex cooking. */
            RawPtr<physx::PxCooking> m_cooking;

            /** Default material used when none is specified. */
            RawPtr<physx::PxMaterial> m_defaultMaterial;

            /** Manager for vehicle related systems. */
            RawPtr<PhysxVehicleManager> m_vehicleManager;

            /** CPU dispatcher used by PhysX for simulation tasks. */
            RawPtr<physx::PxDefaultCpuDispatcher> m_cpuDispatcher;

            /** Helper to convert engine assets into PhysX cooked forms. */
            RawPtr<PhysxCooker> m_cooker;

            /** Array of standard materials indexed by material id. */
            RawPtr<physx::PxMaterial> m_standardMaterials[MAX_NUM_INDEX_BUFFERS];

            /** Queue of objects pending load/unload processed on update. */
            ConcurrentQueue<SmartPtr<ISharedObject>> m_loadQueue;

            /** Managed collections of created shapes. */
            ConcurrentArray<SmartPtr<ISphereShape3>> m_sphereShapes;
            ConcurrentArray<SmartPtr<IBoxShape3>>    m_boxShapes;
            ConcurrentArray<SmartPtr<IMeshShape>>    m_meshShapes;
            ConcurrentArray<SmartPtr<ITerrainShape>> m_terrainShapes;
            ConcurrentArray<SmartPtr<IPlaneShape3>>  m_planeShapes;

            /** Managed physics bodies. */
            ConcurrentArray<SmartPtr<PhysxRigidDynamic>>        m_rigidBodies;
            ConcurrentArray<SmartPtr<PhysxRigidStatic>>         m_staticBodies;
            ConcurrentArray<SmartPtr<PhysxCharacterController>> m_characters;
            ConcurrentArray<SmartPtr<IPhysicsVehicle3>>         m_vehicles;

            ConcurrentArray<SmartPtr<IPhysicsMaterial3>> m_materials;

            /** Scenes created/owned by this manager. */
            ConcurrentArray<SmartPtr<PhysxScene>> m_scenes;

            /** Flag to enable debug drawing of physics objects. */
            atomic_bool m_enableDebugDraw = false;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxManager_h__
