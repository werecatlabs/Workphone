#ifndef WPPHYSICSRIGIDBODY2_HPP
#define WPPHYSICSRIGIDBODY2_HPP

#include "WPPhysics/WPPhysicsPrerequisites.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody2D.hpp>
#include <Workphone/Interface/Physics/IRigidBody2.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager2D.hpp>
#include <Workphone/Interface/Physics/IPhysicsBodyEffectSnap2.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include "Workphone/Math/Transform2.hpp"
#include "WPPhysics/WPPhysicsBody2.hpp"

namespace workphone::physics
{

    /**
     * @class WPPhysicsRigidBody2
     * @brief Implementation of a 2D rigid body for the physics engine.
     *
     * This class manages the physical properties and state of a rigid body in 2D space,
     * including mass, friction, restitution, and dynamics.
     */
    class WPPhysicsRigidBody2 : public WPPhysicsBody2<IRigidBody2>
    {
    public:
        friend class ScriptReceiver;

        static const hash_type CONSTRAINBOUNDS_HASH;
        static const hash_type CAPSCREENPOSITION_HASH;
        static const hash_type ENABLE_COLLISION_HASH;
        static const hash_type DAMPLINEARVELOCITY_HASH;
        static const hash_type ENABLEPHYSICS_HASH;
        static const hash_type CONSTRAIN_X_HASH;
        static const hash_type CONSTRAIN_Y_HASH;
        static const hash_type ALLOWROTATION_HASH;
        static const hash_type SAME_TYPE_COLLISION_HASH;
        static const hash_type ENABLE_PARTICLE_COLLISION_HASH;
        static const hash_type USE_TARGET_POSITION_X_HASH;
        static const hash_type USE_TARGET_POSITION_Y_HASH;
        static const hash_type ENABLE_HASH;

        static const hash_type WORLD_ID_HASH;
        static const hash_type RESTITUTION_HASH;
        static const hash_type LINEAR_DAMP_VALUE_HASH;
        static const hash_type MASS_HASH;

        /**
         * @class ForceState
         * @brief Stores the linear and angular force state of the rigid body.
         */
        class ForceState : public ISharedObject
        {
        public:
            ForceState();

            ~ForceState() override;

            /** @brief Resets the force and torque to zero. */
            void reset();

            /** @brief Gets the current linear force. */
            Vector2<real_Num> getForce() const;

            /** @brief Sets the linear force. */
            void setForce( const Vector2<real_Num> &force );

            /** @brief Adds a force vector to the current force. */
            void addForce( const Vector2<real_Num> &force );

            /** @brief Gets the current torque. */
            real_Num getTorque() const;

            /** @brief Sets the torque. */
            void setTorque( const real_Num &torque );

            /** @brief Adds to the current torque. */
            void addTorque( const real_Num &torque );

        protected:
            /// Linear force applied to the body.
            Vector2<real_Num> m_force;

            /// Angular force (torque) applied to the body.
            real_Num m_torque;
        };

        using ForceStatePtr = SmartPtr<ForceState>;

        /**
         * @class DynamicState
         * @brief Stores the time-varying state of the rigid body (position, velocity, etc.).
         */
        class DynamicState : public ISharedObject
        {
        public:
            DynamicState();

            ~DynamicState() override;

            /** @brief Gets the current position. */
            Vector2<real_Num> getPosition() const;

            /** @brief Sets the current position. */
            void setPosition( const Vector2<real_Num> &position );

            /** @brief Gets the linear force in the dynamic state. */
            Vector2<real_Num> getForce() const;

            /** @brief Sets the linear force. */
            void setForce( const Vector2<real_Num> &force );

            /** @brief Adds a force vector. */
            void addForce( const Vector2<real_Num> &force );

            /** @brief Gets the torque. */
            real_Num getTorque() const;

            /** @brief Sets the torque. */
            void setTorque( const real_Num &torque );

            /** @brief Adds to the torque. */
            void addTorque( const real_Num &torque );

            /// Current world position of the rigid body.
            Vector2<real_Num> m_position;

            /// Target position for interpolation or movement.
            Vector2<real_Num> m_target;

            /// Starting position of the body.
            Vector2<real_Num> m_start;

        protected:
            /// Linear force applied in this state.
            Vector2<real_Num> m_force;

        public:
            /// Angular force (torque) in this state.
            real_Num m_torque;

            /// Current angular velocity.
            real_Num m_angularVelocity;

            /// Current rotation angle in radians.
            real_Num m_rotation;

            /// Accumulated animation time for state transitions.
            real_Num m_animationTime;
        };

        using DynamicStatePtr = SmartPtr<DynamicState>;

        /**
         * @class StaticState
         * @brief Stores static properties of the rigid body that change infrequently.
         */
        class StaticState : public ISharedObject
        {
        public:
            StaticState();

            /// Boundary constraints for the body.
            AABB2<real_Num> m_contraint;

            /// Gravity acceleration applied to this body.
            Vector2<real_Num> m_gravity;

            /// Maximum allowed linear velocity.
            Vector2<real_Num> m_maxVelocity;

            /// Bit-field flags for body configuration.
            atomic_u32 m_flags;

            /// Unique identifier of the body.
            hash_type m_id;

            /// Linear damping coefficient.
            real_Num m_linearDampValue;

            /// Angular damping coefficient.
            real_Num m_angularDampValue;

            /// Bounciness coefficient (0 to 1).
            real_Num m_restitution;

            /// Mass of the body.
            real_Num m_mass;
            /// Inverse mass (1/mass), cached for performance.
            real_Num m_invMass;

            /// Surface friction coefficient.
            real_Num m_friction;

            /// Moment of inertia.
            real_Num m_I;
            /// Inverse moment of inertia (1/I).
            real_Num m_invI;

            /// Coefficient for air resistance.
            real_Num m_airResistance;

            /// Identifier of the physics world this body belongs to.
            hash_type m_worldId;

            /// Type identifier for the object represented by this body.
            hash_type m_objectType;

            /// Flag indicating a change in state.
            atomic_u32 m_flagChange;

            /// Flag indicating a required update.
            atomic_u32 m_flagUpdate;

            /// Collision category type.
            atomic_u32 m_collisionType;

            /// Collision mask for filtering interactions.
            atomic_u32 m_collisionMask;

            /// Timestamp/Index for the next state update.
            atomic_u32 m_nextStateUpdate;

            /// Timestamp/Index of the last state update.
            atomic_u32 m_lastStateUpdate;

            /// Whether the body is currently sleeping.
            atomic_bool m_sleep;

            /// Whether the physics for this body are enabled.
            atomic_bool m_enabled;

            /// Whether the body is in kinematic mode (not affected by forces).
            bool m_kinematicMode;
        };

        using StaticStatePtr = SmartPtr<StaticState>;

        WPPhysicsRigidBody2( IPhysicsManager2D *creator );

        ~WPPhysicsRigidBody2() override;

        void handleEvent( const SmartPtr<IEvent> &event );

        const String &getComponentType() const;

        u32 getComponentTypeId() const;

        //
        // IPhysicsBody2 functions
        //
        void setPosition( const Vector2<real_Num> &position ) override;

        Vector2<real_Num> getPosition() const override;

        void setTargetPosition( const Vector2<real_Num> &position ) override;

        Vector2<real_Num> getTargetPosition() const override;

        void setOrientation( real_Num orientation ) override;

        real_Num getOrientation() const override;

        real_Num getAngularVelocity() const override;

        void addForce( const Vector2<real_Num> &force ) override;

        void setForce( const Vector2<real_Num> &force ) override;

        Vector2<real_Num> getForce() const override;

        void addTorque( real_Num torque ) override;

        void setTorque( real_Num torque ) override;

        real_Num getTorque() const override;

        void addVelocity( const Vector2<real_Num> &velocity,
                          const Vector2<real_Num> &relPos = Vector2<real_Num>::ZERO ) override;

        void setVelocity( const Vector2<real_Num> &velocity ) override;

        Vector2<real_Num> getVelocity() const override;

        void setMaxVelocity( const Vector2<real_Num> &velocity ) override;

        Vector2<real_Num> getMaxVelocity() const override;

        void setLinearDampValue( real_Num linearDampValue ) override;

        real_Num getLinearDampValue() const override;

        void setAngularDampValue( real_Num angularDampValue ) override;

        real_Num getAngularDampValue() const override;

        void setAirResistance( real_Num airResistance ) override;

        real_Num getAirResistance() const override;

        void setFlag( u32 flag, bool value ) override;

        bool getFlag( u32 flag ) const override;

        inline bool _getFlag( u32 flag ) const;

        u32 getBodyType() const override;

        void setObjectType( hash_type type ) override;

        hash_type getObjectType() const override;

        void setWorldId( hash_type worldId ) override;

        hash_type getWorldId() const override;

        inline hash_type _getWorldId() const;

        void setEnabled( bool enabled ) override;

        bool isEnabled() const override;

        AABB2<real_Num> getLocalAABB() const override;

        AABB2<real_Num> getWorldAABB() const override;

        void setMaterialId( hash_type materialId ) override;

        hash_type getMaterialId() const override;

        void setUserData( void *userData ) override;

        void *getUserData() const override;

        void setCollisionType( u32 mask ) override;

        u32 getCollisionType() const override;

        //
        // IPhysicsRigidBody2 functions
        //

        void setCollisionShape( const SmartPtr<IPhysicsShape2> &shape ) override;

        const SmartPtr<IPhysicsShape2> &getCollisionShape() const override;

        void setContraintAABB( const AABB2<real_Num> &contraintRect ) override;

        AABB2<real_Num> getContraintAABB() const override;

        //
        // RigidBody2 functions
        //
        u32 getId() const;

        void addVector( const Vector2<real_Num> &vector );

        Transform2<real_Num> getTransformState() const;

        void setRestitution( real_Num restitution ) override;

        real_Num getRestitution() const override;

        real_Num getFriction() const;

        void setFriction( real_Num friction );

        void setMass( real_Num mass ) override;

        inline real_Num getMass() const override;

        inline real_Num getMassInv() const override;

        void setSleep( bool sleep ) override;

        bool isSleeping() const override;

        SmartPtr<IStateContext> &getStateContext();

        const SmartPtr<IStateContext> &getStateContext() const;

        void _processMessage( SmartPtr<IStateMessage> message );

        bool getKinematicMode() const override;

        void setKinematicMode( bool kinematicMode ) override;

        Vector2<real_Num> getGravity() const override;

        void setGravity( const Vector2<real_Num> &gravity ) override;

        DynamicState *getPreviousState() const;

        void setPreviousState( DynamicState *previousState );

        DynamicState *getCurrentState() const;

        void setCurrentState( DynamicState *currentState );

        ForceState *getForceState() const;

        void setForceState( ForceState *forceState );

        bool getGrounded() const;

        void setGrounded( bool grounded );

        void addEffect( SmartPtr<IPhysicsEffect2> effect ) override;

        void removeEffect( SmartPtr<IPhysicsEffect2> effect ) override;

        /// for ode solver-binding
        int m_odeTag;

    private:
        /** Caps the velocity vector pass in. */
        void capVelocity( const Vector2<real_Num> &position, Vector2<real_Num> &velocity );

        /** Caps the position passed. */
        void capPosition( Vector2<real_Num> &position );

        void checkTargetPosition( const Vector2<real_Num> &vector, Vector2<real_Num> &position );

        class ScriptReceiver : public IScriptReceiver
        {
        public:
            ScriptReceiver( WPPhysicsRigidBody2 *body );

            s32 setProperty( hash_type id, const Parameter &param ) override;

            s32 setProperty( hash_type id, const Parameters &params ) override;

            s32 setProperty( hash_type hash, void *param ) override;

            s32 getProperty( hash_type id, Parameter &param ) const override;

            s32 getProperty( hash_type id, Parameters &params ) const override;

            s32 getProperty( hash_type hash, void *param ) const override;

        protected:
            WPPhysicsRigidBody2 *m_body;
        };

        class RigidBody2StateListener : public IStateListener
        {
        public:
            RigidBody2StateListener( WPPhysicsRigidBody2 *body );

            ~RigidBody2StateListener() override;

            void OnStateChanged( const SmartPtr<IStateMessage> &message );

            void OnStateChanged( const SmartPtr<IState> &state );

        protected:
            WPPhysicsRigidBody2 *m_body;
            u32 m_taskId;
        };

        class CollisionListener : public IEventListener
        {
        public:
            CollisionListener( WPPhysicsRigidBody2 *body );

            ~CollisionListener() override;

            void OnMaterialSetup( IPhysicsMaterial2 *material );

            int OnContactStart( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB );

            int OnContactEnd( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB );

            int OnContactBreak( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB );

            int OnNoContact( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB );

            int OnContact( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB );

            int OnContactProcess( IPhysicsMaterial2 *material );

            WPPhysicsRigidBody2 *m_body;
        };

        class PhysicsBodyEffectSnap2 : public IPhysicsBodyEffectSnap2
        {
        public:
            PhysicsBodyEffectSnap2();

            ~PhysicsBodyEffectSnap2() override;

            void handleEvent( const SmartPtr<IEvent> &event ) override;

            IPhysicsBody2D *getOwner() const override;

            void setOwner( IPhysicsBody2D *owner ) override;

            Vector2<real_Num> getTarget() const override;

            void setTarget( const Vector2<real_Num> &target ) override;

            bool getUseAxis( int axis ) const override;

            void setUseAxis( int axis, bool useAxis ) override;

            WPPhysicsRigidBody2 *m_body;
            Vector2<real_Num> m_target;
            bool m_axis[2];
        };

        void update( const s32 &task, const time_interval &t, const time_interval &dt );

        void updateForce( const s32 &task, const time_interval &t, const time_interval &dt );

        void updateGravity( const s32 &task, const time_interval &t, const time_interval &dt );

        void updateKinematic( const s32 &task, const time_interval &t, const time_interval &dt );

        void advancePosition( const s32 &task, const time_interval &t, const time_interval &dt );

        void postUpdate( const s32 &task, const time_interval &t, const time_interval &dt );

        void restorePosition();

        void updateFlags();

        ///
        IPhysicsManager2D *m_creator;

        ///
        void *m_userData;

        SmartPtr<IPhysicsShape2> m_shape;

        SmartPtr<IStateContext> m_stateContext;
        SmartPtr<IStateListener> m_stateListener;

        mutable DynamicStatePtr m_previousState;
        mutable DynamicStatePtr m_currentState;
        mutable ForceStatePtr m_forceState;
        mutable StaticStatePtr m_staticState;

        bool m_isGrounded;

        /// The next generated id of the body.
        static u32 m_nextId;
    };
}  // namespace workphone::physics

// end namespace

#include "WPPhysicsRigidBody2.inl"

#endif
