#ifndef WPPHYSICSPARTICLE2_HPP
#define WPPHYSICSPARTICLE2_HPP

#include "WPPhysics/WPPhysicsPrerequisites.hpp"
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsParticle2.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager2D.hpp>
#include "Workphone/Math/Transform2.hpp"
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include "WPPhysics/WPPhysicsBody2.hpp"

namespace workphone::physics
{
    /**
     * @class WPPhysicsParticle2
     * @brief Implementation of a 2D physics particle.
     *
     * Inherits from WPPhysicsBody2 and implements IPhysicsParticle2, providing
     * the core functionality for particle-based physics simulation including
     * motion, forces, and collision properties.
     */
    class WPPhysicsParticle2 : public WPPhysicsBody2<IPhysicsParticle2>
    {
    public:
        WPPhysicsParticle2(IPhysicsManager2D *creator);

        ~WPPhysicsParticle2() override;

        void update(const s32 &task, const time_interval &t, const time_interval &dt);

        const String &getComponentType() const;

        u32 getComponentTypeId() const;

        u32 getId() const;

        //
        // IPhysicsBody2 functions
        //
        void setPosition(const Vector2<real_Num> &position) override;

        Vector2<real_Num> getPosition() const override;

        void setRelativePosition(const Vector2<real_Num> &position);

        Vector2<real_Num> getRelativePosition() const;

        void setTargetPosition(const Vector2<real_Num> &position) override;

        Vector2<real_Num> getTargetPosition() const override;

        void setOrientation(real_Num orientation) override;

        real_Num getOrientation() const override;

        real_Num getAngularVelocity() const override;

        void addForce(const Vector2<real_Num> &force) override;

        void setForce(const Vector2<real_Num> &force) override;

        Vector2<real_Num> getForce() const override;

        void addTorque(real_Num torque) override;

        void setTorque(real_Num torque) override;

        real_Num getTorque() const override;

        void addVelocity(const Vector2<real_Num> &velocity,
                         const Vector2<real_Num> &relPos = Vector2<real_Num>::ZERO) override;

        void setVelocity(const Vector2<real_Num> &velocity) override;

        Vector2<real_Num> getVelocity() const override;

        void setMaxVelocity(const Vector2<real_Num> &velocity) override;

        Vector2<real_Num> getMaxVelocity() const override;

        void setLinearDampValue(real_Num linearDampValue) override;

        real_Num getLinearDampValue() const override;

        void setAngularDampValue(real_Num angularDampValue) override;

        real_Num getAngularDampValue() const override;

        void setAirResistance(real_Num airResistance) override;

        real_Num getAirResistance() const override;

        void setFlag(u32 flag, bool value) override;

        bool getFlag(u32 flag) const override;

        u32 getBodyType() const override;

        void setObjectType(hash_type type) override;

        hash_type getObjectType() const override;

        void setWorldId(hash_type worldId) override;

        hash_type getWorldId() const override;

        void setEnabled(bool enabled) override;

        bool isEnabled() const override;

        AABB2<real_Num> getLocalAABB() const override;

        AABB2<real_Num> getWorldAABB() const override;

        void setMaterialId(hash_type materialId) override;

        hash_type getMaterialId() const override;

        void setUserData(void *userData) override;

        void *getUserData() const override;

        void setRestitution(real_Num restitution) override;

        real_Num getRestitution() const override;

        void setMass(real_Num mass) override;

        real_Num getMass() const override;

        real_Num getMassInv() const override;

        void setCollisionType(u32 mask) override;

        u32 getCollisionType() const override;

        /** */
        void setContraintAABB(const AABB2<real_Num> &contraintRect) override;

        /** */
        AABB2<real_Num> getContraintAABB() const override;

        //
        // IPhysicsParticle2 functions
        //

        //
        // FluidParticle2d functions
        //

        void setCollisionShape(SmartPtr<IPhysicsShape2> shape) override;

        const SmartPtr<IPhysicsShape2> &getCollisionShape() const override;

        // these are called on the physics engine
        void _addVector(const Vector2<real_Num> &vector);

        void _setVelocity(const Vector2<real_Num> &velocity);

        Transform2<real_Num> _getTransformState() const;

        void setSleep(bool sleep) override;

        bool isSleeping() const override;

        SmartPtr<IStateContext> &getStateContext();

        const SmartPtr<IStateContext> &getStateContext() const;

        bool getKinematicMode() const override;

        void setKinematicMode(bool kinematicMode) override;

        Vector2<real_Num> getGravity() const override;

        void setGravity(const Vector2<real_Num> &gravity) override;

        bool getEnableGravity() const override;

        void setEnableGravity(bool enableGravity) override;

    protected:
        /// @brief State context for the particle.
        SmartPtr<IStateContext> m_stateContext;

        /// @brief Pointer to the physics manager that created this particle.
        IPhysicsManager2D *m_creator;

        /// @brief The collision shape the particle uses.
        SmartPtr<IPhysicsShape2> m_shape;

        /// @brief The current position of the particle.
        Vector2<real_Num> m_position;

        /// @brief The current velocity of the particle.
        Vector2<real_Num> m_velocity;

        /// @brief The maximum allowable velocity of the particle.
        Vector2<real_Num> m_maxVelocity;
        /// @brief The target position for interpolation or steering.
        Vector2<real_Num> m_targetPosition;
        /// @brief The accumulated force acting on the particle.
        Vector2<real_Num> m_force;
        /// @brief The gravity vector applied to this particle.
        Vector2<real_Num> m_gravity;
        /// @brief The AABB constraint defining the allowed movement area.
        AABB2<real_Num> m_constraintAABB;

        /// @brief User-defined data associated with the particle.
        void *m_userData;

        /// @brief The coefficient of restitution for bounces.
        real_Num m_restitution;
        /// @brief The mass of the particle.
        real_Num m_mass = static_cast<real_Num>(1);
        /// @brief The current orientation of the particle.
        real_Num m_orientation = static_cast<real_Num>(0);
        /// @brief The current angular velocity.
        real_Num m_angularVelocity = static_cast<real_Num>(0);
        /// @brief The current torque applied to the particle.
        real_Num m_torque = static_cast<real_Num>(0);
        /// @brief Linear damping factor to simulate drag.
        real_Num m_linearDamping = static_cast<real_Num>(0);
        /// @brief Angular damping factor.
        real_Num m_angularDamping = static_cast<real_Num>(0);
        /// @brief Resistance factor from air.
        real_Num m_airResistance = static_cast<real_Num>(0);

        /// @brief Bitmask of flags representing the particle's state.
        atomic_u32 m_flags;

        /// @brief Unique identifier for the particle.
        u32 m_id;

        /// @brief The hash type identifying the object type.
        hash_type m_objectType;

        /// @brief The identifier of the world this particle belongs to.
        atomic_u64 m_worldId;

        /// @brief Collision type mask for filtering interactions.
        atomic_u32 m_collisionType;

        /// @brief Collision mask.
        atomic_u32 m_mask;

        /// @brief Material identifier for physical properties.
        hash_type m_materialId = 0;
        /// @brief Flag indicating if the particle is in kinematic mode.
        bool m_kinematicMode;
        /// @brief Flag indicating if the particle is sleeping.
        bool m_sleeping = false;

        /// @brief Static counter for generating unique particle IDs.
        static u32 m_nextId;
    };
} // namespace workphone::physics

// end namespace

#endif
