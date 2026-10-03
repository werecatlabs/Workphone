#ifndef _IPhysicsBody2_H
#define _IPhysicsBody2_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Transform2.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/AABB2.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 2D physics body.
         *
         * This pure virtual interface defines the contract for all 2D physics bodies used
         * by the physics subsystem. Implementations represent objects that participate in
         * a physics simulation (static, dynamic, kinematic, particles, etc.).
         *
         * Implementations are responsible for applying simulation-specific semantics to
         * the operations defined here (for example, how forces, velocities, or kinematic
         * targets are integrated).
         *
         * Typical responsibilities exposed by this interface:
         * - Querying and controlling transform (position, orientation)
         * - Managing linear and angular motion (velocity, forces, torque)
         * - Material properties (mass, restitution, damping, air resistance)
         * - Collision filtering (type, mask) and material assignment
         * - Sleeping and kinematic modes
         * - Gravity control and per-body effects or constraints
         *
         * Note:
         * - Coordinate space for position/velocity/force is implementation-defined but
         *   usually world space. Check the concrete implementation for exact semantics.
         *
         * @see IRigidBody2
         * @see IPhysicsParticle2
         * @see IPhysicsManager2D
         */
        class WPCore_API IPhysicsBody2D : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures derived implementations are correctly destroyed through this interface.
             */
            ~IPhysicsBody2D() override;

            /**
             * @brief Set the body's world position.
             * @param position New world-space position for the body.
             *
             * The effect of setting position depends on the implementation: it may
             * immediately teleport the body, be queued for the next simulation step,
             * or be ignored for certain static/managed bodies.
             */
            virtual void setPosition( const Vector2<real_Num> &position ) = 0;

            /**
             * @brief Get the body's world position.
             * @return World-space position of the body.
             */
            virtual Vector2<real_Num> getPosition() const = 0;

            /**
             * @brief Set the target (desired) position for the body.
             * @param position Desired target position.
             *
             * Target positions are typically used by kinematic bodies or smoothing logic
             * where the physics implementation will attempt to move the body towards the
             * target over time. Behaviour is implementation-specific.
             */
            virtual void setTargetPosition( const Vector2<real_Num> &position ) = 0;

            /**
             * @brief Get the currently set target position.
             * @return Target (desired) position for the body.
             */
            virtual Vector2<real_Num> getTargetPosition() const = 0;

            /**
             * @brief Set the body's orientation (rotation).
             * @param orientation Orientation angle (implementation may use radians or degrees;
             *                    consult the concrete implementation).
             *
             * The orientation is the rotation applied to the body about its local origin.
             */
            virtual void setOrientation( real_Num orientation ) = 0;

            /**
             * @brief Get the body's orientation (rotation).
             * @return Orientation angle.
             */
            virtual real_Num getOrientation() const = 0;

            /**
             * @brief Get the body's angular velocity.
             * @return Angular velocity (signed scalar).
             */
            virtual real_Num getAngularVelocity() const = 0;

            /**
             * @brief Add a force to the body.
             * @param force Force vector applied to the body's center (world space).
             *
             * Implementations may accumulate multiple forces per simulation step.
             */
            virtual void addForce( const Vector2<real_Num> &force ) = 0;

            /**
             * @brief Set the net force acting on the body.
             * @param force New net force vector (replaces any previously accumulated force).
             */
            virtual void setForce( const Vector2<real_Num> &force ) = 0;

            /**
             * @brief Get the current net force on the body.
             * @return Net force vector.
             */
            virtual Vector2<real_Num> getForce() const = 0;

            /**
             * @brief Add a torque (moment) to the body.
             * @param torque Scalar torque value applied about the body's origin.
             */
            virtual void addTorque( real_Num torque ) = 0;

            /**
             * @brief Set the body's net torque.
             * @param torque New net torque (replaces previously accumulated torque).
             */
            virtual void setTorque( real_Num torque ) = 0;

            /**
             * @brief Get the body's current torque.
             * @return Net torque acting on the body.
             */
            virtual real_Num getTorque() const = 0;

            /**
             * @brief Apply linear velocity to a point on the body.
             * @param velocity Velocity vector to add (world space).
             * @param relPos Relative point (local or world depending on implementation) to apply the
             * velocity to.
             *
             * Use relPos to apply a velocity impulse at a point offset from the center to
             * produce both translation and rotation.
             */
            virtual void addVelocity( const Vector2<real_Num> &velocity,
                                      const Vector2<real_Num> &relPos = Vector2<real_Num>::ZERO ) = 0;

            /**
             * @brief Set the body's linear velocity.
             * @param velocity New linear velocity vector.
             */
            virtual void setVelocity( const Vector2<real_Num> &velocity ) = 0;

            /**
             * @brief Get the body's linear velocity.
             * @return Current linear velocity vector.
             */
            virtual Vector2<real_Num> getVelocity() const = 0;

            /**
             * @brief Set the maximum allowed linear velocity for the body.
             * @param velocity Maximum velocity vector (per-axis or magnitude depending on impl).
             *
             * Enforced by the specific physics implementation to clamp or limit motion.
             */
            virtual void setMaxVelocity( const Vector2<real_Num> &velocity ) = 0;

            /**
             * @brief Get the maximum allowed linear velocity.
             * @return Maximum velocity for the body.
             */
            virtual Vector2<real_Num> getMaxVelocity() const = 0;

            /**
             * @brief Set linear damping (reduces linear velocity over time).
             * @param linearDampValue Damping coefficient (non-negative).
             */
            virtual void setLinearDampValue( real_Num linearDampValue ) = 0;

            /**
             * @brief Get the linear damping coefficient.
             * @return Linear damping value.
             */
            virtual real_Num getLinearDampValue() const = 0;

            /**
             * @brief Set angular damping (reduces angular velocity over time).
             * @param angularDampValue Angular damping coefficient (non-negative).
             */
            virtual void setAngularDampValue( real_Num angularDampValue ) = 0;

            /**
             * @brief Get the angular damping coefficient.
             * @return Angular damping value.
             */
            virtual real_Num getAngularDampValue() const = 0;

            /**
             * @brief Set the air resistance applied to the body.
             * @param airResistance Air resistance coefficient (implementation-specific units).
             *
             * May be used to simulate drag proportional to velocity or velocity squared.
             */
            virtual void setAirResistance( real_Num airResistance ) = 0;

            /**
             * @brief Get the air resistance coefficient.
             * @return Current air resistance value.
             */
            virtual real_Num getAirResistance() const = 0;

            /**
             * @brief Set restitution (bounciness) of the body.
             * @param restitution Restitution coefficient (usually in range [0,1]).
             */
            virtual void setRestitution( real_Num restitution ) = 0;

            /**
             * @brief Get restitution (bounciness).
             * @return Restitution coefficient.
             */
            virtual real_Num getRestitution() const = 0;

            /**
             * @brief Set the body's mass.
             * @param mass Mass value (positive for dynamic bodies).
             *
             * Some implementations may treat zero or infinite mass specially (static bodies).
             */
            virtual void setMass( real_Num mass ) = 0;

            /**
             * @brief Get the body's mass.
             * @return Mass value.
             */
            virtual real_Num getMass() const = 0;

            /**
             * @brief Get the inverse mass of the body.
             * @return Inverse mass (1 / mass) or zero for infinite mass.
             */
            virtual real_Num getMassInv() const = 0;

            /**
             * @brief Set or clear a numeric flag on the body.
             * @param flag Flag identifier.
             * @param value True to set the flag, false to clear it.
             *
             * Flags are implementation-defined boolean attributes used to control
             * custom behaviour or optimizations.
             */
            virtual void setFlag( u32 flag, bool value ) = 0;

            /**
             * @brief Query a numeric flag on the body.
             * @param flag Flag identifier to query.
             * @return True if the flag is set, false otherwise.
             */
            virtual bool getFlag( u32 flag ) const = 0;

            /**
             * @brief Get the body type identifier.
             * @return Numeric identifier describing the body type (implementation-defined).
             */
            virtual u32 getBodyType() const = 0;

            /**
             * @brief Set the object type for this body.
             * @param type Hash or identifier representing the logical object type.
             *
             * Object type may be used for higher-level filtering, gameplay queries, or
             * material lookup.
             */
            virtual void setObjectType( hash_type type ) = 0;

            /**
             * @brief Get the object type identifier.
             * @return Hash or identifier previously set with setObjectType().
             */
            virtual hash_type getObjectType() const = 0;

            /**
             * @brief Set the ID of the world this body belongs to.
             * @param worldId Identifier of the physics world/scene.
             *
             * Useful for associating a body with a particular simulation instance.
             */
            virtual void setWorldId( hash_type worldId ) = 0;

            /**
             * @brief Get the world identifier this body belongs to.
             * @return World identifier.
             */
            virtual hash_type getWorldId() const = 0;

            /**
             * @brief Enable or disable the body in the simulation.
             * @param enabled True to enable simulation for this body, false to disable.
             *
             * Disabled bodies are excluded from simulation and collision queries depending
             * on the implementation.
             */
            virtual void setEnabled( bool enabled ) = 0;

            /**
             * @brief Check whether the body is enabled in the simulation.
             * @return True if the body is enabled; false otherwise.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Get the body's local axis-aligned bounding box.
             * @return Local-space AABB enclosing the current collision geometry.
             *
             * The returned AABB is expressed in the body's local coordinate system.
             */
            virtual AABB2<real_Num> getLocalAABB() const = 0;

            /**
             * @brief Get the body's world axis-aligned bounding box.
             * @return World-space AABB enclosing the collision geometry.
             *
             * This AABB is typically used for broad-phase collision culling and spatial queries.
             */
            virtual AABB2<real_Num> getWorldAABB() const = 0;

            /**
             * @brief Assign a material identifier to the body.
             * @param materialId Material hash/identifier used to look up physical properties.
             */
            virtual void setMaterialId( hash_type materialId ) = 0;

            /**
             * @brief Get the body's material identifier.
             * @return Material identifier.
             */
            virtual hash_type getMaterialId() const = 0;

            /**
             * @brief Set this body's collision type.
             * @param mask Collision type mask (implementation-defined).
             *
             * Collision type is used together with the collision mask to determine which
             * bodies should interact.
             */
            virtual void setCollisionType( u32 mask ) = 0;

            /**
             * @brief Get this body's collision type mask.
             * @return Collision type mask.
             */
            virtual u32 getCollisionType() const = 0;

            /**
             * @brief Set the collision mask controlling which types this body collides with.
             * @param mask Collision mask.
             */
            virtual void setCollisionMask( u32 mask ) = 0;

            /**
             * @brief Get the collision mask used when deciding collision partners.
             * @return Collision mask.
             */
            virtual u32 getCollisionMask() const = 0;

            /**
             * @brief Put the body to sleep or wake it.
             * @param sleep True to request sleeping, false to wake the body.
             *
             * Sleeping bodies are typically excluded from simulation until re-awakened.
             */
            virtual void setSleep( bool sleep ) = 0;

            /**
             * @brief Query whether the body is currently sleeping.
             * @return True if the body is sleeping; false otherwise.
             */
            virtual bool isSleeping() const = 0;

            /**
             * @brief Set a constraint axis-aligned bounding box for the body.
             * @param contraintRect Constraint AABB in world or local space (impl-specific).
             *
             * This AABB can be used by the physics manager or body to restrict motion or for
             * optimization. Note the method name preserves the original spelling.
             */
            virtual void setContraintAABB( const AABB2<real_Num> &contraintRect ) = 0;

            /**
             * @brief Get the constraint AABB previously set on the body.
             * @return Constraint AABB.
             */
            virtual AABB2<real_Num> getContraintAABB() const = 0;

            /**
             * @brief Get whether the body is in kinematic mode.
             * @return True if the body is kinematic; false otherwise.
             *
             * Kinematic bodies are typically driven by user code (setPosition/setTargetPosition)
             * rather than by physics dynamics.
             */
            virtual bool getKinematicMode() const = 0;

            /**
             * @brief Enable or disable kinematic mode for the body.
             * @param kinematicMode True to enable kinematic behaviour.
             */
            virtual void setKinematicMode( bool kinematicMode ) = 0;

            /**
             * @brief Get the custom gravity vector applied to the body.
             * @return Gravity vector in world space.
             *
             * Some physics implementations allow per-body gravity overriding the world gravity.
             */
            virtual Vector2<real_Num> getGravity() const = 0;

            /**
             * @brief Set a custom gravity vector for the body.
             * @param gravity Gravity vector to apply to this body.
             */
            virtual void setGravity( const Vector2<real_Num> &gravity ) = 0;

            /**
             * @brief Query whether per-body gravity is enabled for this body.
             * @return True if gravity is enabled; false otherwise.
             */
            virtual bool getEnableGravity() const = 0;

            /**
             * @brief Enable or disable gravity for this body.
             * @param enableGravity True to enable gravity.
             */
            virtual void setEnableGravity( bool enableGravity ) = 0;

            /**
             * @brief Add a physics effect to this body.
             * @param effect Smart pointer to an effect instance.
             *
             * Effects provide modular, composable behaviour (forces, modifiers, etc.) applied
             * to the body by the physics system.
             */
            virtual void addEffect( SmartPtr<IPhysicsEffect2> effect ) = 0;

            /**
             * @brief Remove a previously added physics effect.
             * @param effect Smart pointer to the effect to remove.
             */
            virtual void removeEffect( SmartPtr<IPhysicsEffect2> effect ) = 0;

            /**
             * @brief Get the list of constraints attached to this body.
             * @return Array of constraints (smart pointers).
             */
            virtual Array<SmartPtr<IPhysicsConstraint2>> getConstraints() const = 0;

            /**
             * @brief Remove all constraints from the body.
             *
             * Useful when destroying or reinitializing the body.
             */
            virtual void removeConstraints() = 0;

            /**
             * @brief Remove a single constraint from the body.
             * @param constraint Constraint to remove.
             */
            virtual void removeConstraint( SmartPtr<IPhysicsConstraint2> constraint ) = 0;

            /**
             * @brief Add a constraint to this body.
             * @param constraint Constraint to attach.
             */
            virtual void addConstraint( SmartPtr<IPhysicsConstraint2> constraint ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif
