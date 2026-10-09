#ifndef WPPHYSICSNATIVERIGIDBODY2_HPP
#define WPPHYSICSNATIVERIGIDBODY2_HPP

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Interface/Physics/IRigidBody2.hpp>

extern "C" {
#include <WorkphonePhysics/workphone_physics_2d.h>
}

namespace workphone::physics
{
    /**
     * @class WPPhysicsNativeRigidBody2
     * @brief Implementation of a 2D rigid body in the physics simulation.
     *
     * This class represents a physical object in a 2D world, managing its dynamics,
     * kinematics, collision properties, and constraints. It serves as a wrapper around
     * the native wp_rigidbody.
     */
    class WPPhysicsRigidBody2 : public IRigidBody2
    {
    public:
        /**
         * @brief Constructs a new 2D rigid body.
         * @param type The type of rigid body (e.g., Dynamic, Static, Kinematic).
         */
        explicit WPPhysicsRigidBody2(wp_rigidbody_type type = WORKPHONE_RIGIDBODY_DYNAMIC);
        ~WPPhysicsRigidBody2() override;

        /** @brief Returns the underlying native rigid body pointer. */
        wp_rigidbody *getBody() const;
        void *getNativeObject() const;

        /** @brief Assigns a collision shape to this rigid body. */
        void setCollisionShape(const SmartPtr<IPhysicsShape2> &shape) override;

        /** @brief Gets the current collision shape. */
        const SmartPtr<IPhysicsShape2> &getCollisionShape() const override;

        /** @brief Sets the world position of the body. */
        void setPosition(const Vector2<real_Num> &position) override;

        /** @brief Gets the current world position. */
        Vector2<real_Num> getPosition() const override;

        /** @brief Sets the target position for interpolation or kinematic movement. */
        void setTargetPosition(const Vector2<real_Num> &position) override;

        /** @brief Gets the current target position. */
        Vector2<real_Num> getTargetPosition() const override;

        /** @brief Sets the rotation (orientation) of the body. */
        void setOrientation(real_Num orientation) override;

        /** @brief Gets the current rotation (orientation). */
        real_Num getOrientation() const override;

        /** @brief Gets the current angular velocity. */
        real_Num getAngularVelocity() const override;

        /** @brief Adds a force vector to the body. */
        void addForce(const Vector2<real_Num> &force) override;

        /** @brief Sets the constant force acting on the body. */
        void setForce(const Vector2<real_Num> &force) override;

        /** @brief Gets the current force acting on the body. */
        Vector2<real_Num> getForce() const override;

        /** @brief Adds torque to the body to change its angular velocity. */
        void addTorque(real_Num torque) override;

        /** @brief Sets the constant torque acting on the body. */
        void setTorque(real_Num torque) override;

        /** @brief Gets the current torque acting on the body. */
        real_Num getTorque() const override;

        /**
         * @brief Applies a velocity change.
         * @param velocity The velocity vector to add.
         * @param relPos The relative position where the impulse is applied.
         */
        void addVelocity(const Vector2<real_Num> &velocity,
                         const Vector2<real_Num> &relPos = Vector2<real_Num>::ZERO) override;

        /** @brief Sets the linear velocity of the body. */
        void setVelocity(const Vector2<real_Num> &velocity) override;

        /** @brief Gets the current linear velocity. */
        Vector2<real_Num> getVelocity() const override;

        /** @brief Sets the maximum allowable linear velocity. */
        void setMaxVelocity(const Vector2<real_Num> &velocity) override;

        /** @brief Gets the maximum linear velocity limit. */
        Vector2<real_Num> getMaxVelocity() const override;

        /** @brief Sets the linear damping value to simulate drag. */
        void setLinearDampValue(real_Num linearDampValue) override;

        /** @brief Gets the current linear damping coefficient. */
        real_Num getLinearDampValue() const override;

        /** @brief Sets the angular damping value. */
        void setAngularDampValue(real_Num angularDampValue) override;

        /** @brief Gets the current angular damping coefficient. */
        real_Num getAngularDampValue() const override;

        /** @brief Sets the air resistance coefficient. */
        void setAirResistance(real_Num airResistance) override;

        /** @brief Gets the current air resistance. */
        real_Num getAirResistance() const override;

        /** @brief Sets the coefficient of restitution (bounciness). */
        void setRestitution(real_Num restitution) override;

        /** @brief Gets the current restitution coefficient. */
        real_Num getRestitution() const override;

        /** @brief Sets the mass of the body. */
        void setMass(real_Num mass) override;

        /** @brief Gets the mass of the body. */
        real_Num getMass() const override;

        /** @brief Gets the inverse mass (1/mass), useful for physics calculations. */
        real_Num getMassInv() const override;

        /** @brief Sets a specific behavior flag. */
        void setFlag(u32 flag, bool value) override;

        /** @brief Checks if a specific behavior flag is set. */
        bool getFlag(u32 flag) const override;

        /** @brief Gets the rigid body type (Dynamic, Static, etc.). */
        u32 getBodyType() const override;

        /** @brief Sets the object type identifier. */
        void setObjectType(hash_type type) override;

        /** @brief Gets the object type identifier. */
        hash_type getObjectType() const override;

        /** @brief Sets the identifier for the world this body belongs to. */
        void setWorldId(hash_type worldId) override;

        /** @brief Gets the identifier for the world this body belongs to. */
        hash_type getWorldId() const override;

        /** @brief Enables or disables the body in the simulation. */
        void setEnabled(bool enabled) override;

        /** @brief Checks if the body is enabled. */
        bool isEnabled() const override;

        /** @brief Gets the local Axis-Aligned Bounding Box. */
        AABB2<real_Num> getLocalAABB() const override;

        /** @brief Gets the world-space Axis-Aligned Bounding Box. */
        AABB2<real_Num> getWorldAABB() const override;

        /** @brief Sets the material identifier for this body. */
        void setMaterialId(hash_type materialId) override;

        /** @brief Gets the material identifier. */
        hash_type getMaterialId() const override;

        /** @brief Sets the collision type mask. */
        void setCollisionType(u32 mask) override;

        /** @brief Gets the collision type mask. */
        u32 getCollisionType() const override;

        /** @brief Sets the collision mask used to filter interactions. */
        void setCollisionMask(u32 mask) override;

        /** @brief Gets the collision mask. */
        u32 getCollisionMask() const override;

        /** @brief Forces the body to sleep or wake it up. */
        void setSleep(bool sleep) override;

        /** @brief Checks if the body is currently in a sleeping state. */
        bool isSleeping() const override;

        /** @brief Sets the AABB for constraint calculations. */
        void setContraintAABB(const AABB2<real_Num> &contraintRect) override;

        /** @brief Gets the AABB for constraint calculations. */
        AABB2<real_Num> getContraintAABB() const override;

        /** @brief Returns true if the body is in kinematic mode. */
        bool getKinematicMode() const override;

        /** @brief Sets whether the body operates in kinematic mode. */
        void setKinematicMode(bool kinematicMode) override;

        /** @brief Gets the gravity vector applied to this body. */
        Vector2<real_Num> getGravity() const override;

        /** @brief Sets the gravity vector for this body. */
        void setGravity(const Vector2<real_Num> &gravity) override;

        /** @brief Checks if gravity is enabled for this body. */
        bool getEnableGravity() const override;

        /** @brief Enables or disables gravity for this body. */
        void setEnableGravity(bool enableGravity) override;

        /** @brief Returns a list of all constraints attached to this body. */
        Array<SmartPtr<IPhysicsConstraint2>> getConstraints() const override;

        /** @brief Removes all constraints attached to the body. */
        void removeConstraints() override;

        /** @brief Removes a specific constraint. */
        void removeConstraint(SmartPtr<IPhysicsConstraint2> constraint) override;

        /** @brief Attaches a constraint to the body. */
        void addConstraint(SmartPtr<IPhysicsConstraint2> constraint) override;

    private:
        wp_rigidbody *m_body = nullptr; ///< Pointer to the native rigid body.
        SmartPtr<IPhysicsShape2> m_collisionShape; ///< The collision shape associated with this body.
        Vector2<real_Num> m_targetPosition =
            Vector2<real_Num>::ZERO; ///< Target position for kinematic/interpolation.
        Vector2<real_Num> m_maxVelocity = Vector2<real_Num>::ZERO; ///< Maximum linear velocity limit.
        Vector2<real_Num> m_gravity =
            Vector2<real_Num>::ZERO; ///< Gravity vector specific to this body.
        AABB2<real_Num> m_constraintAABB; ///< AABB used for constraint processing.
        hash_type m_objectType = 0; ///< Type identifier for the object.
        hash_type m_worldId = 0; ///< Identifier of the world the body resides in.
        hash_type m_materialId = 0; ///< Material identifier.
        u32 m_flags = 0; ///< Bitflags for body properties.
        real_Num m_linearDamping = 0; ///< Linear damping coefficient.
        real_Num m_airResistance = 0; ///< Air resistance coefficient.
        real_Num m_restitution = 0; ///< Coefficient of restitution.
        bool m_kinematicMode = false; ///< Whether the body is kinematic.
        Array<SmartPtr<IPhysicsConstraint2>> m_constraints; ///< List of active constraints.
    };
} // namespace workphone::physics

#endif
