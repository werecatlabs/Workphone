#ifndef WPPHYSICSMANAGER3_HPP
#define WPPHYSICSMANAGER3_HPP

#include <WPPhysics/WPPhysicsConversions3.hpp>
#include <Workphone/Physics/PhysicsManager.hpp>

namespace workphone::physics
{
    /**
     * @class WPPhysicsManager3
     * @brief Implementation of the physics management system.
     *
     * The WPPhysicsManager3 class is responsible for coordinating all physics-related
     * operations, including the creation and lifecycle management of scenes, materials,
     * collision shapes, rigid bodies, vehicles, and character controllers.
     */
    class WPPhysicsManager3 : public PhysicsManager
    {
    public:
        WPPhysicsManager3();
        ~WPPhysicsManager3() override;

        /** @brief Loads physics configuration or state from a shared object. */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @brief Unloads physics configuration or state. */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @brief Returns whether debug drawing is enabled for the physics world. */
        bool getEnableDebugDraw() const override;

        /** @brief Enables or disables physics debug drawing. */
        void setEnableDebugDraw( bool enableDebugDraw ) override;

        /** @brief Submits managed body bounds and queued forces to the debug renderer. */
        void debugDraw() override;

        /** @brief Submits physics debugging after the simulation update. */
        void postUpdate() override;

        /** @brief Creates and registers a new physics material. */
        SmartPtr<IPhysicsMaterial3> addMaterial() override;

        /** @brief Removes a physics material from the manager. */
        void removeMaterial( SmartPtr<IPhysicsMaterial3> material ) override;

        /** @brief Creates and registers a new physics scene. */
        SmartPtr<IPhysicsScene3> addScene() override;

        /** @brief Removes a physics scene from the manager. */
        void removeScene( SmartPtr<IPhysicsScene3> scene ) override;

        /** @brief Creates a collision shape based on a type hash and provided data. */
        SmartPtr<IPhysicsShape3> addCollisionShapeByType( hash64 type,
                                                          SmartPtr<ISharedObject> data ) override;

        /** @brief Removes a collision shape. Returns true if successful. */
        bool removeCollisionShape( SmartPtr<IPhysicsShape3> collisionShape ) override;

        /** @brief Removes a rigid body from the physics simulation. Returns true if successful. */
        bool removePhysicsBody( SmartPtr<IRigidBody3> body ) override;

        /** @brief Adds a new character controller to the simulation. */
        SmartPtr<ICharacterController3> addCharacter() override;

        /** @brief Adds a static rigid body at the specified transform. */
        SmartPtr<IRigidStatic3> addRigidStatic( const Transform3<real_Num> &transform ) override;

        /** @brief Adds a dynamic rigid body at the specified transform. */
        SmartPtr<IRigidDynamic3> addRigidDynamic( const Transform3<real_Num> &transform ) override;

        /** @brief Adds a static rigid body using a predefined collision shape. */
        SmartPtr<IRigidStatic3> addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape ) override;

        /** @brief Adds a static rigid body with a specific collision shape and properties. */
        SmartPtr<IRigidStatic3> addRigidStatic( SmartPtr<IPhysicsShape3> collisionShape,
                                                SmartPtr<Properties> properties ) override;

        /** @brief Creates a vehicle associated with the given chassis rigid body. */
        SmartPtr<IPhysicsVehicle3> addVehicle( SmartPtr<IRigidBody3> chassis ) override;

        /** @brief Removes a vehicle from the simulation. Returns true if successful. */
        bool removeVehicle( SmartPtr<IPhysicsVehicle3> vehicle ) override;

        /** @brief Creates a vehicle with a given chassis and specific properties. */
        SmartPtr<IPhysicsVehicle3> addVehicle( SmartPtr<IRigidBody3> chassis,
                                               const SmartPtr<Properties> &properties ) override;

        /** @brief Performs a raycast test in the physics world. */
        bool rayTest( const Vector3<real_Num> &start, const Vector3<real_Num> &direction,
                      Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal, u32 collisionType = 0,
                      u32 collisionMask = 0 ) override;

        /** @brief Checks if a line segment intersects any physics objects. */
        bool intersects( const Vector3<real_Num> &start, const Vector3<real_Num> &end,
                         Vector3<real_Num> &hitPos, Vector3<real_Num> &hitNormal,
                         SmartPtr<ISharedObject> &object, u32 collisionType = 0,
                         u32 collisionMask = 0 ) override;

        /** @brief Adds a D6 joint constraint between two physics bodies. */
        SmartPtr<IConstraintD6> addConstraintD6( SmartPtr<IPhysicsBody3> actor0,
                                                 const Transform3<real_Num> &localFrame0,
                                                 SmartPtr<IPhysicsBody3> actor1,
                                                 const Transform3<real_Num> &localFrame1 ) override;

        /** @brief Adds a fixed joint constraint between two physics bodies. */
        SmartPtr<IConstraintFixed3> addFixedConstraint(
            SmartPtr<IPhysicsBody3> actor0, const Transform3<real_Num> &localFrame0,
            SmartPtr<IPhysicsBody3> actor1, const Transform3<real_Num> &localFrame1 ) override;

        /** @brief Removes a physics constraint. */
        void removeConstraint( SmartPtr<IPhysicsConstraint3> constraint ) override;

        /** @brief Adds a constraint drive for controlling joint movement. */
        SmartPtr<IConstraintDrive> addConstraintDrive() override;

        /** @brief Adds a linear limit constraint. */
        SmartPtr<IConstraintLinearLimit> addConstraintLinearLimit( real_Num extent,
                                                                   real_Num contactDist ) override;

        /** @brief Creates a container for raycast hit results. */
        SmartPtr<IRaycastHit> addRaycastHitData() override;

        /** @brief Removes a raycast hit data container. */
        void removeRaycastHitData( SmartPtr<IRaycastHit> raycastHitData ) override;

        /** @brief Gets the task ID associated with state updates. */
        TaskId getStateTask() const override;

        /** @brief Gets the task ID associated with the physics simulation step. */
        TaskId getPhysicsTask() const override;

        /** @brief Loads an object into the physics system. */
        void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

        /** @brief Unloads an object from the physics system. */
        void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) override;

        /** @brief Gets the main physics simulation scene. */
        SmartPtr<IPhysicsScene3> getPhysicsScene() const override;

        /** @brief Sets the main physics simulation scene. */
        void setPhysicsScene( SmartPtr<IPhysicsScene3> physicsScene ) override;

        /** @brief Gets the scene used for object management. */
        SmartPtr<IPhysicsScene3> getObjectsScene() const override;

        /** @brief Sets the scene used for object management. */
        void setObjectsScene( SmartPtr<IPhysicsScene3> objectsScene ) override;

        /** @brief Gets the scene specialized for raycasting. */
        SmartPtr<IPhysicsScene3> getRaycastScene() const override;

        /** @brief Sets the scene specialized for raycasting. */
        void setRaycastScene( SmartPtr<IPhysicsScene3> raycastScene ) override;

        /** @brief Gets the scene used for control interactions. */
        SmartPtr<IPhysicsScene3> getControlsScene() const override;

        /** @brief Sets the scene used for control interactions. */
        void setControlsScene( SmartPtr<IPhysicsScene3> controlsScene ) override;

    private:
        wp_physics_system *m_system = nullptr;           ///< Pointer to the internal physics system.
        Array<SmartPtr<IPhysicsMaterial3>> m_materials;  ///< Collection of registered materials.
        Array<SmartPtr<IPhysicsScene3>> m_scenes;        ///< Collection of managed scenes.
        Array<SmartPtr<IPhysicsShape3>> m_shapes;        ///< Collection of created shapes.
        Array<SmartPtr<IRigidBody3>> m_bodies;           ///< Collection of managed rigid bodies.
        Array<SmartPtr<IPhysicsVehicle3>> m_vehicles;    ///< Collection of managed vehicles.
        Array<SmartPtr<ICharacterController3>> m_characters;  ///< Collection of managed characters.
        Array<SmartPtr<IRaycastHit>> m_raycastHits;  ///< Collection of raycast hit data buffers.
        SmartPtr<IPhysicsScene3> m_physicsScene;     ///< The main simulation scene.
        SmartPtr<IPhysicsScene3> m_objectsScene;     ///< The object management scene.
        SmartPtr<IPhysicsScene3> m_raycastScene;     ///< The raycasting scene.
        SmartPtr<IPhysicsScene3> m_controlsScene;    ///< The controls interaction scene.
    };
}  // namespace workphone::physics

#endif
