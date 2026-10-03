#ifndef WPPhysxRigidBody_h__
#define WPPhysxRigidBody_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Physics/RigidDynamic3.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <WPPhysx/WPPhysxRigidBody3.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX-backed implementation of a dynamic rigid body.
         *
         * This class implements the engine's `RigidDynamic3` interface using PhysX's
         * `PxRigidDynamic` actor. It exposes functions for applying forces/torques,
         * setting velocities, kinematic targets, damping, sleeping/wake control,
         * solver iteration counts, contact-report thresholds and other runtime properties.
         *
         * The semantics of many methods mirror PhysX `PxRigidDynamic` behavior (for
         * example: kinematic targets, wake/sleep behavior, damping and solver iteration counts).
         */
        class PhysxRigidDynamic : public PhysxRigidBody3<RigidDynamic3>
        {
        public:
            /**
             * @brief Listener used to receive state messages and state changes.
             *
             * The listener keeps a weak reference to its owner `PhysxRigidDynamic` to
             * avoid reference cycles. It forwards relevant state messages to the owner.
             */
            class StateListener : public IStateListener
            {
            public:
                /** Default constructor. */
                StateListener();

                /** Destructor. */
                ~StateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 *
                 * @param message The state message to process.
                 * @return True if the message was handled, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a state change notification.
                 *
                 * @param state The new state.
                 * @return True if the change was handled, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the owner `PhysxRigidDynamic` instance (if still alive).
                 *
                 * @return Smart pointer to the owner or null if the owner has been destroyed.
                 */
                SmartPtr<PhysxRigidDynamic> getOwner() const;

                /**
                 * @brief Set the owner of this listener.
                 *
                 * The listener stores a weak reference so that it does not extend the owner's lifetime.
                 *
                 * @param owner Owner rigid dynamic instance.
                 */
                void setOwner( SmartPtr<PhysxRigidDynamic> owner );

            protected:
                /// Weak pointer to the owning `PhysxRigidDynamic`.
                AtomicWeakPtr<PhysxRigidDynamic> m_owner;
            };

            /** Default constructor. Initializes internal state. */
            PhysxRigidDynamic();

            /** Virtual destructor. Releases any PhysX actor references. */
            ~PhysxRigidDynamic() override;

            /**
             * @copydoc ISharedObject::load
             *
             * Loads or initializes runtime resources required by this body. `data` may
             * contain serialized or configuration information used to create the actor.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * Releases runtime resources, removes the actor from scenes and clears state.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Adds a linear velocity at a point relative to the body's origin.
             *
             * Adds an instantaneous change in linear velocity that originates at the
             * provided relative position, producing both linear and angular effects.
             *
             * @param velocity Linear velocity to add (world space).
             * @param relPos Position relative to the body's origin where the velocity is applied (world
             * space).
             */
            void addVelocity( const Vector3<physics_Num> &velocity, const Vector3<physics_Num> &relPos );

            /**
             * @brief Sets the body's linear velocity.
             *
             * Replaces the current linear velocity with `velocity`.
             *
             * @param velocity New linear velocity (world space).
             */
            void setVelocity( const Vector3<physics_Num> &velocity );

            /**
             * @brief Returns the body's linear velocity.
             *
             * @return Current linear velocity (world space).
             */
            Vector3<physics_Num> getVelocity() const;

            /**
             * @brief Returns the body's angular velocity.
             *
             * @return Current angular velocity (world space).
             */
            Vector3<physics_Num> getAngularVelocity() const override;

            /**
             * @brief Apply a force to the body.
             *
             * Adds `force` to the body. The applied force may wake the body if it is sleeping.
             * The semantics of how the force is applied (accumulated vs. immediate) depend on the
             * engine's force update model.
             *
             * @param force Force vector in world space.
             */
            void addForce( const Vector3<physics_Num> &force ) override;

            /**
             * @brief Replace the accumulated force on the body.
             *
             * Sets the accumulated force to `force`, discarding any previously accumulated force.
             *
             * @param force Force vector in world space.
             */
            void setForce( const Vector3<physics_Num> &force );

            /**
             * @brief Get the currently accumulated force for the body.
             *
             * @return Accumulated force (world space).
             */
            Vector3<physics_Num> getForce() const;

            /**
             * @brief Apply a torque to the body.
             *
             * Adds `torque` to the body. The applied torque may wake the body if it is sleeping.
             *
             * @param torque Torque vector in world space.
             */
            void addTorque( const Vector3<physics_Num> &torque ) override;

            /**
             * @brief Replace the accumulated torque on the body.
             *
             * Sets the accumulated torque to `torque`, discarding any previously accumulated torque.
             *
             * @param torque Torque vector in world space.
             */
            void setTorque( const Vector3<physics_Num> &torque );

            /**
             * @brief Get the currently accumulated torque for the body.
             *
             * @return Accumulated torque (world space).
             */
            Vector3<physics_Num> getTorque() const;

            /**
             * @brief Returns the underlying PhysX dynamic actor pointer.
             *
             * May return nullptr if the actor has not been created or has been released.
             *
             * @return Pointer to `physx::PxRigidDynamic`.
             */
            physx::PxRigidDynamic *getActorDynamic() const;

            /**
             * @brief Sets the underlying PhysX dynamic actor pointer.
             *
             * Ownership semantics are not changed by this call; it simply assigns the
             * internal pointer. The caller is responsible for ensuring pointer validity.
             *
             * @param actor Pointer to a `physx::PxRigidDynamic` actor.
             */
            void setActorDynamic( physx::PxRigidDynamic *actor );

            /**
             * @brief Set or clear a rigid body flag.
             *
             * Flags affect runtime simulation (e.g. kinematic). Behavior follows PhysX semantics.
             *
             * @param flag The flag to change.
             * @param value True to set the flag, false to clear it.
             */
            void setRigidBodyFlag( RigidBodyFlagEnum flag, bool value ) override;

            /**
             * @brief Get the currently configured rigid body flags.
             *
             * @return Bitmask of `RigidBodyFlagEnum` flags.
             */
            RigidBodyFlagEnum getRigidBodyFlags() const override;

            /**
             * @brief Sets the body's linear velocity.
             *
             * @param linVel New linear velocity in world space.
             * @param autowake If true the body will be woken up automatically when velocity is set.
             */
            void setLinearVelocity( const Vector3<physics_Num> &linVel, bool autowake = true ) override;

            /**
             * @brief Retrieves the body's linear velocity.
             *
             * @return Current linear velocity in world space.
             */
            Vector3<physics_Num> getLinearVelocity() const override;

            /**
             * @brief Sets the body's angular velocity.
             *
             * @param angVel New angular velocity in world space.
             * @param autowake If true the body will be woken up automatically when velocity is set.
             */
            void setAngularVelocity( const Vector3<physics_Num> &angVel, bool autowake = true ) override;

            /**
             * @brief Clears forces applied to the body.
             *
             * @param mode Specifies how forces are cleared (Force vs Impulse semantics).
             */
            void clearForce( ForceModeEnum mode = ForceModeEnum::Force ) override;

            /**
             * @brief Clears torques applied to the body.
             *
             * @param mode Specifies how torques are cleared (Force vs Impulse semantics).
             */
            void clearTorque( ForceModeEnum mode = ForceModeEnum::Force ) override;

            /**
             * @brief Set or clear an actor-level flag.
             *
             * Actor flags affect higher-level behaviour such as disabling simulation.
             *
             * @param flag Actor flag to change.
             * @param value True to set the flag, false to clear it.
             */
            void setActorFlag( ActorFlagEnum flag, bool value ) override;

            /**
             * @brief Get the currently configured actor flags.
             *
             * @return Bitmask of `ActorFlagEnum` flags.
             */
            ActorFlagEnum getActorFlags() const override;

            /**
             * @brief Set a kinematic target transform for this actor.
             *
             * For kinematic actors, this stores a target pose that the actor will be moved to
             * during the next simulation step. Consecutive calls overwrite the previously set target.
             *
             * @param destination Desired global transform for the kinematic actor.
             */
            void setKinematicTarget( const Transform3<physics_Num> &destination ) override;

            /**
             * @brief Retrieve the previously set kinematic target, if any.
             *
             * @param[out] target Output transform that will receive the kinematic target.
             * @return True if a kinematic target is set and written to `target`, false otherwise.
             */
            bool getKinematicTarget( Transform3<physics_Num> &target ) override;

            /**
             * @brief Returns whether the body is currently kinematic.
             *
             * @return True if kinematic, false otherwise.
             */
            bool isKinematic() const override;

            /**
             * @brief Mark the body as kinematic or dynamic.
             *
             * @param kinematic True to make the body kinematic, false to make it dynamic.
             */
            void setKinematic( bool kinematic ) override;

            /**
             * @brief Set linear damping coefficient.
             *
             * @param damping Linear damping coefficient (non-negative).
             */
            void setLinearDamping( physics_Num damping ) override;

            /**
             * @brief Get linear damping coefficient.
             *
             * @return Current linear damping value.
             */
            physics_Num getLinearDamping() const override;

            /**
             * @brief Set angular damping coefficient.
             *
             * @param angDamp Angular damping coefficient (non-negative).
             */
            void setAngularDamping( physics_Num angDamp ) override;

            /**
             * @brief Get angular damping coefficient.
             *
             * @return Current angular damping value.
             */
            physics_Num getAngularDamping() const override;

            /**
             * @brief Set the maximum allowed angular velocity.
             *
             * Very high angular velocities may be clamped by the physics solver; this controls that
             * clamp.
             *
             * @param maxAngVel Maximum angular velocity allowed.
             */
            void setMaxAngularVelocity( physics_Num maxAngVel ) override;

            /**
             * @brief Get the maximum allowed angular velocity.
             *
             * @return Configured maximum angular velocity.
             */
            physics_Num getMaxAngularVelocity() const override;

            /**
             * @brief Returns true if the actor is currently sleeping.
             *
             * Sleeping actors are not simulated until woken.
             *
             * @return True if sleeping.
             */
            bool isSleeping() const override;

            /**
             * @brief Set the sleep threshold (mass-normalized kinetic energy).
             *
             * @param threshold Energy threshold below which the actor may go to sleep.
             */
            void setSleepThreshold( physics_Num threshold ) override;

            /**
             * @brief Get the sleep threshold.
             *
             * @return Sleep energy threshold.
             */
            physics_Num getSleepThreshold() const override;

            /**
             * @brief Set the stabilization threshold (mass-normalized kinetic energy).
             *
             * Actors above this threshold do not participate in stabilization.
             *
             * @param threshold Stabilization energy threshold.
             */
            void setStabilizationThreshold( physics_Num threshold ) override;

            /**
             * @brief Get the stabilization threshold.
             *
             * @return Stabilization energy threshold.
             */
            physics_Num getStabilizationThreshold() const override;

            /**
             * @brief Set the wake counter for the actor.
             *
             * Positive values will wake the actor.
             *
             * @param wakeCounterValue New wake counter value.
             */
            void setWakeCounter( physics_Num wakeCounterValue ) override;

            /**
             * @brief Get the actor's wake counter.
             *
             * @return Current wake counter value.
             */
            physics_Num getWakeCounter() const override;

            /** @brief Wake the actor up (if sleeping). */
            void wakeUp() override;

            /** @brief Force the actor to sleep. */
            void putToSleep() override;

            /**
             * @brief Set solver iteration counts used to resolve constraints and contacts.
             *
             * Increasing these values improves stability at the expense of performance.
             *
             * @param minPositionIters Minimum position iterations (>= 1).
             * @param minVelocityIters Minimum velocity iterations (>= 1).
             */
            void setSolverIterationCounts( u32 minPositionIters, u32 minVelocityIters = 1 ) override;

            /**
             * @brief Retrieve solver iteration counts.
             *
             * @param[out] minPositionIters Returned minimum position iterations.
             * @param[out] minVelocityIters Returned minimum velocity iterations.
             */
            void getSolverIterationCounts( u32 &minPositionIters, u32 &minVelocityIters ) const override;

            /**
             * @brief Get the force threshold used to generate contact reports for this body.
             *
             * @return Contact report threshold (force magnitude).
             */
            physics_Num getContactReportThreshold() const override;

            /**
             * @brief Set the contact report threshold for this body.
             *
             * When contact forces exceed this threshold a contact report may be generated.
             *
             * @param threshold Force threshold for contact reporting.
             */
            void setContactReportThreshold( physics_Num threshold ) override;

            /**
             * @brief Create a copy of this physics body.
             *
             * Returns a cloned `IPhysicsBody3` instance with equivalent properties and shapes.
             * Note: the clone may not share the same underlying PhysX actor pointer.
             *
             * @return Smart pointer to the cloned physics body.
             */
            SmartPtr<IPhysicsBody3> clone() override;

            /**
             * @brief Set the actor's transform immediately (active transform).
             *
             * This forces the actor into the given transform; use with care as it may
             * teleport objects in the simulation.
             *
             * @param transform Global transform to set.
             */
            void setActiveTransform( const Transform3<physics_Num> &transform );

            /**
             * @copydoc ISharedObject::getChildObjects
             *
             * Returns any runtime-registered child objects (shapes, listeners, etc.).
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc IPhysicsBody3::getProperties
             *
             * Retrieves serializable properties used for saving/inspection.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IPhysicsBody3::setProperties
             *
             * Apply serializable `properties` to configure this body.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Add a collision shape to this body.
             *
             * The shape will be attached to the internal actor and participate in collisions.
             *
             * @param shape Shape to add (ownership follows engine conventions).
             */
            void addShape( SmartPtr<IPhysicsShape3> shape );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Accumulated force applied to the body (world space).
            Vector3<physics_Num> m_force;

            /// Accumulated torque applied to the body (world space).
            Vector3<physics_Num> m_torque;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxRigidBody_h__
