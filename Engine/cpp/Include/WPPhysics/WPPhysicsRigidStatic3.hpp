#ifndef WPPHYSICSRIGIDSTATIC3_HPP
#define WPPHYSICSRIGIDSTATIC3_HPP

#include <WPPhysics/WPPhysicsUtil.hpp>
#include <Workphone/Physics/RigidStatic3.hpp>

namespace workphone::physics
{
    /**
     * @class WPPhysicsRigidStatic3
     * @brief Implementation of a static rigid body in the physics engine.
     *
     * This class provides a concrete implementation of the RigidStatic3 interface.
     * Static bodies are non-movable objects that can collide with dynamic bodies
     * but remain fixed in the physics simulation.
     */
    class WPPhysicsRigidStatic3 : public RigidStatic3
    {
    public:
        WPPhysicsRigidStatic3();
        ~WPPhysicsRigidStatic3() override;

        /** @brief Gets the physics scene this body is associated with. */
        SmartPtr<IPhysicsScene3> getScene() const override;
        /** @brief Associates this body with a physics scene. */
        void setScene(SmartPtr<IPhysicsScene3> scene) override;
        /** @brief Sets the world transform of the body. */
        void setTransform(const Transform3<real_Num> &transform) override;
        /** @brief Gets the current world transform of the body. */
        Transform3<real_Num> getTransform() const override;
        /** @brief Sets a specific actor flag. */
        void setActorFlag(ActorFlagEnum flag, bool value) override;
        /** @brief Gets the current actor flags. */
        ActorFlagEnum getActorFlags() const override;
        /** @brief Gets the mass of the body. */
        real_Num getMass() const override;
        /** @brief Sets the mass of the body. */
        void setMass(real_Num mass) override;
        /** @brief Sets the collision category type. */
        void setCollisionType(u32 type) override;
        /** @brief Gets the collision category type. */
        u32 getCollisionType() const override;
        /** @brief Sets the collision filter mask. */
        void setCollisionMask(u32 mask) override;
        /** @brief Gets the collision filter mask. */
        u32 getCollisionMask() const override;
        /** @brief Enables or disables the body in the simulation. */
        void setEnabled(bool enabled) override;
        /** @brief Checks if the body is enabled. */
        bool isEnabled() const override;
        /** @brief Gets user data associated with a specific ID. */
        void *getUserDataById(u32 id) const override;
        /** @brief Sets user data for a specific ID. */
        void setUserDataById(u32 id, void *userData) override;
        /** @brief Gets the general user data pointer. */
        void *getUserData() const override;
        /** @brief Sets the general user data pointer. */
        void setUserData(void *userData) override;
        /** @brief Checks if the body is in kinematic mode. */
        bool getKinematicMode() const override;
        /** @brief Sets whether the body is in kinematic mode. */
        void setKinematicMode(bool kinematicMode) override;
        /** @brief Creates a copy of the physics body. */
        SmartPtr<IPhysicsBody3> clone() override;
        /** @brief Forces the body to wake up from sleep. */
        void wakeUp() override;
        /** @brief Gets the state context for interpolation and synchronization. */
        SmartPtr<IStateContext> getStateContext() const override;
        /** @brief Sets the state context. */
        void setStateContext(SmartPtr<IStateContext> stateContext) override;
        /** @brief Retrieves the underlying raw object pointer. */
        void _getObject(void **object) const override;

        /** @brief Sets a specific rigid body flag. */
        void setRigidBodyFlag(RigidBodyFlagEnum flag, bool value) override;
        /** @brief Gets the current rigid body flags. */
        RigidBodyFlagEnum getRigidBodyFlags() const override;
        /** @brief Adds a collision shape to the body. */
        void addShape(SmartPtr<IPhysicsShape3> shape) override;
        /** @brief Removes a collision shape from the body. */
        void removeShape(SmartPtr<IPhysicsShape3> shape, bool wakeOnLostTouch = true) override;
        /** @brief Returns the list of attached collision shapes. */
        Array<SmartPtr<IPhysicsShape3>> getShapes() const override;
        /** @brief Gets the number of attached collision shapes. */
        u32 getNumShapes() const override;
        /** @brief Sets the inertia tensor in mass space. */
        void setMassSpaceInertiaTensor(const Vector3<real_Num> &inertia) override;
        /** @brief Gets the inertia tensor in mass space. */
        Vector3<real_Num> getMassSpaceInertiaTensor() const override;
        /** @brief Gets the inverse inertia tensor in mass space. */
        Vector3<real_Num> getMassSpaceInvInertiaTensor() const override;
        /** @brief Gets the local AABB of the body. */
        AABB3<real_Num> getAABB() const override;
        void setAABB(const AABB3<real_Num> &bounds) override;
        void setRadius(real_Num radius) override;
        real_Num getRadius() const override;

        AABB3<real_Num> getLocalAABB() const override;
        /** @brief Gets the world AABB of the body. */
        AABB3<real_Num> getWorldAABB() const override;

        /**
         * @brief Returns the underlying raw physics body.
         * @return Pointer to the internal wp_rigidbody.
         */
        wp_rigidbody *getBody() const;

    private:
        wp_rigidbody *m_body = nullptr; ///< Underlying physics engine body object
        WeakPtr<IPhysicsScene3> m_scene; ///< Reference to the scene this body belongs to
        SmartPtr<IStateContext> m_stateContext; ///< Context for state management and interpolation
        Array<SmartPtr<IPhysicsShape3>> m_shapes; ///< List of collision shapes attached to this body
        ActorFlagEnum m_actorFlags = static_cast<ActorFlagEnum>(0); ///< General actor flags
        RigidBodyFlagEnum m_rigidBodyFlags =
            static_cast<RigidBodyFlagEnum>(0); ///< Rigid body specific flags
        bool m_enabled = true; ///< Whether the body is active in the simulation
        void *m_userDataById[4] = {}; ///< Fixed-size array for indexed user data pointers
    };
} // namespace workphone::physics

#endif
