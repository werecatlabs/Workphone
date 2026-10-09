#ifndef WPPHYSICSRIGIDDYNAMIC3_HPP
#define WPPHYSICSRIGIDDYNAMIC3_HPP

#include <WPPhysics/WPPhysicsUtil.hpp>
#include <Workphone/Physics/RigidDynamic3.hpp>

namespace workphone::physics
{
    /**
     * @class WPPhysicsRigidDynamic3
     * @brief Implementation of a dynamic rigid body in the physics engine.
     *
     * This class provides a concrete implementation of the RigidDynamic3 interface,
     * managing the underlying wp_rigidbody and providing controls for physical
     * properties, forces, and kinematic behavior.
     */
    class WPPhysicsRigidDynamic3 : public RigidDynamic3
    {
    public:
        explicit WPPhysicsRigidDynamic3(wp_rigidbody_type type);
        ~WPPhysicsRigidDynamic3() override;

        // --- IPhysicsBody3 Interface Implementation ---

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

        // --- RigidDynamic3 Interface Implementation ---

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
        /** @brief Sets the linear velocity. */
        void setLinearVelocity(const Vector3<real_Num> &linVel, bool autowake = true) override;
        /** @brief Gets the current linear velocity. */
        Vector3<real_Num> getLinearVelocity() const override;
        /** @brief Sets the angular velocity. */
        void setAngularVelocity(const Vector3<real_Num> &angVel, bool autowake = true) override;
        /** @brief Gets the current angular velocity. */
        Vector3<real_Num> getAngularVelocity() const override;
        /** @brief Applies a force to the center of mass. */
        void addForce(const Vector3<real_Num> &force) override;
        /** @brief Clears applied forces. */
        void clearForce(ForceModeEnum mode = ForceModeEnum::Force) override;
        /** @brief Applies torque to the body. */
        void addTorque(const Vector3<real_Num> &torque) override;
        /** @brief Clears applied torque. */
        void clearTorque(ForceModeEnum mode = ForceModeEnum::Force) override;
        /** @brief Gets the local AABB of the body. */
        AABB3<real_Num> getAABB() const override;
        void setAABB(const AABB3<real_Num> &bounds) override;
        void setRadius(real_Num radius) override;
        real_Num getRadius() const override;

        AABB3<real_Num> getLocalAABB() const override;
        /** @brief Gets the world AABB of the body. */
        AABB3<real_Num> getWorldAABB() const override;
        /** @brief Sets the local pose of the center of mass. */
        void setCMassLocalPose(const Transform3<real_Num> &pose) override;
        /** @brief Gets the local pose of the center of mass. */
        Transform3<real_Num> getCMassLocalPose() const override;
        /** @brief Sets the inertia tensor in mass space. */
        void setMassSpaceInertiaTensor(const Vector3<real_Num> &m) override;
        /** @brief Gets the inertia tensor in mass space. */
        Vector3<real_Num> getMassSpaceInertiaTensor() const override;
        /** @brief Gets the inverse inertia tensor in mass space. */
        Vector3<real_Num> getMassSpaceInvInertiaTensor() const override;

        /** @brief Sets the target transform for kinematic interpolation. */
        void setKinematicTarget(const Transform3<real_Num> &destination) override;
        /** @brief Retrieves the current kinematic target. */
        bool getKinematicTarget(Transform3<real_Num> &target) override;
        /** @brief Checks if the body is behaving as a kinematic object. */
        bool isKinematic() const override;
        /** @brief Toggles kinematic behavior. */
        void setKinematic(bool kinematic) override;
        /** @brief Sets the linear damping coefficient. */
        void setLinearDamping(real_Num damping) override;
        /** @brief Gets the linear damping coefficient. */
        real_Num getLinearDamping() const override;
        /** @brief Sets the angular damping coefficient. */
        void setAngularDamping(real_Num damping) override;
        /** @brief Gets the angular damping coefficient. */
        real_Num getAngularDamping() const override;
        /** @brief Sets the maximum angular velocity. */
        void setMaxAngularVelocity(real_Num maxAngVel) override;
        /** @brief Gets the maximum angular velocity. */
        real_Num getMaxAngularVelocity() const override;
        /** @brief Checks if the body is currently sleeping. */
        bool isSleeping() const override;
        /** @brief Sets the threshold for the body to enter sleep mode. */
        void setSleepThreshold(real_Num threshold) override;
        /** @brief Gets the sleep threshold. */
        real_Num getSleepThreshold() const override;
        /** @brief Sets the threshold to prevent jittering during stabilization. */
        void setStabilizationThreshold(real_Num threshold) override;
        /** @brief Gets the stabilization threshold. */
        real_Num getStabilizationThreshold() const override;
        /** @brief Sets the internal wake counter. */
        void setWakeCounter(real_Num wakeCounterValue) override;
        /** @brief Gets the current wake counter value. */
        real_Num getWakeCounter() const override;
        /** @brief Manually puts the body to sleep. */
        void putToSleep() override;
        /** @brief Sets the number of iterations for the solver. */
        void setSolverIterationCounts(u32 minPositionIters, u32 minVelocityIters = 1) override;
        /** @brief Retrieves the current solver iteration counts. */
        void getSolverIterationCounts(u32 &minPositionIters, u32 &minVelocityIters) const override;
        /** @brief Gets the distance threshold for contact reporting. */
        real_Num getContactReportThreshold() const override;
        /** @brief Sets the distance threshold for contact reporting. */
        void setContactReportThreshold(real_Num threshold) override;

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

        Transform3<real_Num> m_kinematicTarget; ///< Destination transform when in kinematic mode
        Transform3<real_Num> m_cmassLocalPose; ///< Local offset of the center of mass

        real_Num m_stabilizationThreshold =
            0.0; ///< Threshold to prevent jittering
        real_Num m_wakeCounter =
            0.0; ///< Internal counter for sleep/wake logic
        real_Num m_contactReportThreshold =
            0.0; ///< Distance threshold for contact reporting

        u32 m_minPositionIters = 4; ///< Minimum iterations for position constraint solver
        u32 m_minVelocityIters = 1; ///< Minimum iterations for velocity constraint solver

        ActorFlagEnum m_actorFlags = static_cast<ActorFlagEnum>(0); ///< General actor flags
        RigidBodyFlagEnum m_rigidBodyFlags =
            static_cast<RigidBodyFlagEnum>(0); ///< Rigid body specific flags

        bool m_hasKinematicTarget = false; ///< Indicates if a kinematic target is currently set
        bool m_enabled = true; ///< Whether the body is active in the simulation

        void *m_userDataById[4] = {}; ///< Fixed-size array for indexed user data pointers
    };
} // namespace workphone::physics

#endif
