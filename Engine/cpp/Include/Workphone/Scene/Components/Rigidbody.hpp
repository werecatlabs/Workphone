#ifndef __Rigidbody_h__
#define __Rigidbody_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Component representing a physical rigid body.
         *
         * The `Rigidbody` component bridges the engine's scene graph and the underlying
         * physics implementation. It manages physical properties (mass, inertia, velocities),
         * collision filtering (group/collision masks), attached collision shapes and
         * constraints, and the associated physics actor (dynamic or static).
         *
         * The component participates in the engine update loop via `preUpdate`, `update`, and
         * `postUpdate` calls to synchronize state to/from the physics scene.
         */
        class WPCore_API Rigidbody : public Component
        {
        public:
            /** @brief Property key for mass used for serialization and reflection. */
            static const String MassStr;

            /** @brief Property key for kinematic state used for serialization and reflection. */
            static const String KinematicStr;

            /** @brief Property key for mass-space inertia tensor used for serialization. */
            static const String MassSpaceInertiaTensorStr;

            /** @brief Property key for the application-side maximum linear velocity. */
            static const String MaxLinearVelocityStr;

            /** @brief Property key for the application-side maximum angular velocity. */
            static const String MaxAngularVelocityStr;

            /** @brief Property key for linear velocity damping. */
            static const String LinearDampingStr;

            /** @brief Property key for angular velocity damping. */
            static const String AngularDampingStr;

            /** @brief Property key controlling whether scene gravity affects this body. */
            static const String UseGravityStr;

            /** @brief Property key controlling continuous collision detection. */
            static const String ContinuousCollisionDetectionStr;

            /** @brief Property key for the energy threshold below which the body may sleep. */
            static const String SleepThresholdStr;

            /** @brief Property key for the stabilization energy threshold. */
            static const String StabilizationThresholdStr;

            /** @brief Property key for position solver iterations. */
            static const String SolverPositionIterationsStr;

            /** @brief Property key for velocity solver iterations. */
            static const String SolverVelocityIterationsStr;

            /** @brief Property key for the contact reporting impulse threshold. */
            static const String ContactReportThresholdStr;

            /** @brief Property key for the collision group bitmask. */
            static const String GroupMaskStr;

            /** @brief Property key for the collision interaction mask. */
            static const String CollisionMaskStr;

            /** @brief Property key for whether the body overrides the actor collision mask. */
            static const String CollisionMaskOverrideStr;

            /** @brief Property key for cached local collision bounds. */
            static const String LocalBoundsStr;

            /** @brief Property key for the body's linear velocity. */
            static const String LinearVelocityStr;

            /** @brief Property key for the body's angular velocity. */
            static const String AngularVelocityStr;

            /** @brief Read-only property key indicating whether a dynamic actor exists. */
            static const String HasRigidDynamicStr;

            /** @brief Read-only property key indicating whether a static actor exists. */
            static const String HasRigidStaticStr;

            /** @brief Read-only property key indicating whether any physics actor exists. */
            static const String HasPhysicsBodyStr;

            /** @brief Read-only property key for attached shape count. */
            static const String NumShapesStr;

            /** @brief Read-only property key for attached constraint count. */
            static const String NumConstraintsStr;

            /** @brief Read-only property key indicating whether constraints are attached. */
            static const String HasConstraintsStr;

            /** @brief Read-only property key indicating whether the dynamic body is sleeping. */
            static const String IsSleepingStr;

            /**
             * @brief Construct a Rigidbody with sensible default physical properties.
             *
             * The physics actor is typically created when the component is added to a
             * physics scene or when shapes are attached.
             */
            Rigidbody();

            /**
             * @brief Destroy the Rigidbody and release any associated physics resources.
             *
             * Ensures the physics actor is removed from its scene before destruction.
             */
            ~Rigidbody() override;

            /**
             * @brief Initialize component state from a serialized data object.
             * @param data Serialized data used to populate properties (may be null).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload runtime resources or reset component state.
             * @param data Optional context object provided by the caller.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handle an engine event dispatched to this component.
             *
             * Implementations may react to lifecycle, physics or custom events. The
             * returned `Parameter` can carry a result or response value.
             *
             * @param eventType Category of the event.
             * @param eventValue Numeric/hash identifier for the event.
             * @param arguments Event arguments.
             * @param sender Event sender (may be null).
             * @param object Associated object (may be null).
             * @param event Event instance.
             * @return Parameter with an optional response value.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Perform global per-frame updates for Rigidbody components.
             *
             * Intended to be invoked by the physics or scene manager once per frame.
             */
            static void updateComponents();

            /**
             * @brief Called before the physics step to push any application-driven changes
             *        (for example kinematic transforms) into the physics actor.
             */
            void preUpdate() override;

            /**
             * @brief Main per-frame update; read back simulation results and apply any
             *        runtime processing required by the component.
             */
            void update() override;

            /**
             * @brief Called after the update loop for any late synchronization or cleanup.
             */
            void postUpdate() override;

            /**
             * @brief Change whether the body is kinematic or simulated by physics.
             * @param kinematicState True to make the body kinematic, false to make it dynamic.
             */
            void updateKinematicState( bool kinematicState );

            /**
             * @brief Get the dynamic physics actor associated with this component.
             * @return Pointer to `IRigidDynamic3` or null if not a dynamic body.
             */
            SmartPtr<physics::IRigidDynamic3> getRigidDynamic() const;

            /**
             * @brief Associate an existing dynamic physics actor with this component.
             * @param rigidDynamic The dynamic actor to attach.
             */
            void setRigidDynamic( SmartPtr<physics::IRigidDynamic3> rigidDynamic );

            /**
             * @brief Get the static physics actor associated with this component.
             * @return Pointer to `IRigidStatic3` or null if not a static body.
             */
            SmartPtr<physics::IRigidStatic3> getRigidStatic() const;

            /**
             * @brief Associate an existing static physics actor with this component.
             * @param rigidStatic The static actor to attach.
             */
            void setRigidStatic( SmartPtr<physics::IRigidStatic3> rigidStatic );

            /**
             * @brief Check whether this body is static and not simulated by physics.
             * @return True when the body is static.
             */
            bool isStatic() const;

            /**
             * @brief Check whether this body is kinematic (application-driven transform).
             * @return True when the body is kinematic.
             */
            bool isKinematic() const;

            /**
             * @brief Set the kinematic flag of the body.
             * @param kinematic True to make the body kinematic.
             */
            void setKinematic( bool kinematic );

            /**
             * @brief Get the collision group bitmask used for broadphase filtering.
             * @return Group mask bits.
             */
            u32 getGroupMask() const;

            /**
             * @brief Set the collision group bitmask used for filtering contacts.
             * @param groupMask New group mask value.
             */
            void setGroupMask( u32 groupMask );

            /**
             * @brief Get the collision mask that determines which groups this body collides with.
             * @return Collision mask bits.
             */
            u32 getCollisionMask() const;

            /**
             * @brief Set the collision mask that selects which groups this body interacts with.
             * @param collisionMask New collision mask value.
             */
            void setCollisionMask( u32 collisionMask );

            /** @brief True after the rigidbody collision mask has been explicitly configured. */
            bool hasCollisionMaskOverride() const;

            /** @brief Set whether the Rigidbody should override or inherit the actor collision mask. */
            void setCollisionMaskOverride( bool collisionMaskOverride );

            /** @brief Inherit an actor-level collision mask without marking it as explicit. */
            void applyActorCollisionMask( u32 collisionMask );

            /** @brief True when a dynamic physics body is currently allocated. */
            bool hasRigidDynamic() const;

            /** @brief True when a static physics body is currently allocated. */
            bool hasRigidStatic() const;

            /** @brief True when any physics body is currently allocated. */
            bool hasPhysicsBody() const;

            /** @brief Number of collision shapes attached to the current physics body. */
            u32 getNumShapes() const;

            /**
             * @brief Get the cached local-space axis-aligned bounding box for attached shapes.
             * @return Local-space AABB covering the collision geometry.
             */
            AABB3<real_Num> getLocalBounds() const;

            /**
             * @brief Override the cached local bounding box for this body.
             * @param bounds New local-space AABB.
             */
            void setLocalBounds( const AABB3<real_Num> &bounds );

            /**
             * @brief Get the physics scene that this body is registered with.
             * @return Pointer to the physics scene or null if not registered.
             */
            SmartPtr<physics::IPhysicsScene3> getScene() const;

            /**
             * @brief Associate this body with a physics scene.
             * @param scene Physics scene to register this body with.
             */
            void setScene( SmartPtr<physics::IPhysicsScene3> scene );

            /**
             * @brief Get the physics material currently used by this body's shapes.
             * @return Pointer to the material or null if none assigned.
             */
            SmartPtr<physics::IPhysicsMaterial3> getMaterial() const;

            /**
             * @brief Set the physics material to be used for contacts of this body.
             * @param material Material instance to assign.
             */
            void setMaterial( SmartPtr<physics::IPhysicsMaterial3> material );

            /** @brief Get the optional cloned physics body used internally. */
            SmartPtr<physics::IPhysicsBody3> getClonedActor() const;

            /** @brief Set the optional cloned physics body used internally. */
            void setClonedActor( SmartPtr<physics::IPhysicsBody3> clonedActor );

            /** @brief Get the active transform reference count. */
            s32 getTransformReferences() const;

            /** @brief Set the active transform reference count. */
            void setTransformReferences( s32 transformReferences );

            /**
             * @brief Get child objects owned by this component (for example shapes and constraints).
             * @return Array of shared pointers to child objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Retrieve a properties object representing this component's serializable state.
             * @return Properties container for this component.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply a properties container to update this component's settings.
             * @param properties Properties holding new values to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Push the component's transform to the physics actor.
             *
             * Used when teleporting or when kinematic bodies are driven by the scene graph.
             */
            void updateTransform() override;

            /**
             * @brief Update internal behavior in response to component flag changes.
             * @param flags New flags value.
             * @param oldFlags Previous flags value.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Handle finite-state-machine events for this component.
             * @param state Current FSM state.
             * @param eventType Event to process.
             * @return FSM handling result.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Update internal smoothing/interpolation state used for rendering transforms.
             */
            void updateSmoothTransformState() override;

            /**
             * @brief Get the linear velocity of the body in world-space.
             * @return Current linear velocity vector.
             */
            Vector3<real_Num> getLinearVelocity() const;

            /**
             * @brief Set the linear velocity of the body in world-space.
             * @param linearVelocity New linear velocity vector.
             */
            void setLinearVelocity( const Vector3<real_Num> &linearVelocity );

            /**
             * @brief Get the angular velocity of the body in world-space.
             * @return Current angular velocity vector.
             */
            Vector3<real_Num> getAngularVelocity() const;

            /**
             * @brief Set the angular velocity of the body in world-space.
             * @param angularVelocity New angular velocity vector.
             */
            void setAngularVelocity( const Vector3<real_Num> &angularVelocity );

            /**
             * @brief Get the application-side maximum linear speed that this component enforces.
             */
            real_Num getMaxLinearVelocity() const;

            /**
             * @brief Set the application-side maximum linear speed for this body.
             * @param maxLinearVelocity Maximum linear speed.
             */
            void setMaxLinearVelocity( real_Num maxLinearVelocity );

            /**
             * @brief Get the application-side maximum angular speed that this component enforces.
             */
            real_Num getMaxAngularVelocity() const;

            /**
             * @brief Set the application-side maximum angular speed for this body.
             * @param maxAngularVelocity Maximum angular speed.
             */
            void setMaxAngularVelocity( real_Num maxAngularVelocity );

            /** @brief Get the linear damping coefficient. */
            real_Num getLinearDamping() const;

            /** @brief Set the linear damping coefficient. Values are clamped to zero or greater. */
            void setLinearDamping( real_Num linearDamping );

            /** @brief Get the angular damping coefficient. */
            real_Num getAngularDamping() const;

            /** @brief Set the angular damping coefficient. Values are clamped to zero or greater. */
            void setAngularDamping( real_Num angularDamping );

            /** @brief Return whether scene gravity affects this body. */
            bool getUseGravity() const;

            /** @brief Enable or disable scene gravity for this body. */
            void setUseGravity( bool useGravity );

            /** @brief Return whether continuous collision detection is enabled. */
            bool getContinuousCollisionDetection() const;

            /** @brief Enable or disable continuous collision detection. */
            void setContinuousCollisionDetection( bool enabled );

            /** @brief Get the sleep energy threshold. */
            real_Num getSleepThreshold() const;

            /** @brief Set the sleep energy threshold. Values are clamped to zero or greater. */
            void setSleepThreshold( real_Num threshold );

            /** @brief Get the stabilization energy threshold. */
            real_Num getStabilizationThreshold() const;

            /** @brief Set the stabilization threshold. Values are clamped to zero or greater. */
            void setStabilizationThreshold( real_Num threshold );

            /** @brief Get the minimum number of position solver iterations. */
            u32 getSolverPositionIterations() const;

            /** @brief Set the position solver iteration count. Values are clamped to at least one. */
            void setSolverPositionIterations( u32 iterations );

            /** @brief Get the minimum number of velocity solver iterations. */
            u32 getSolverVelocityIterations() const;

            /** @brief Set the velocity solver iteration count. Values are clamped to at least one. */
            void setSolverVelocityIterations( u32 iterations );

            /** @brief Get the contact reporting threshold. */
            real_Num getContactReportThreshold() const;

            /** @brief Set the contact reporting threshold. Values are clamped to zero or greater. */
            void setContactReportThreshold( real_Num threshold );

            /** @brief Return true when the live dynamic body is sleeping. */
            bool isSleeping() const;

            /**
             * @brief Get the angular velocity expressed in local (body) space.
             * @return Local-space angular velocity vector.
             */
            Vector3<real_Num> getLocalAngularVelocity() const;

            /**
             * @brief Get the linear velocity expressed in local (body) space.
             * @return Local-space linear velocity vector.
             */
            Vector3<real_Num> getLocalLinearVelocity() const;

            /**
             * @brief Apply a force to the body's center of mass.
             * @param force Force vector in world-space applied at the center of mass.
             */
            void addForce( const Vector3<real_Num> &force );

            /**
             * @brief Apply a torque (moment) to the body.
             * @param torque Torque vector in world-space.
             */
            void addTorque( const Vector3<real_Num> &torque );

            /**
             * @brief Set the mass of the body. For dynamic bodies this must be > 0.
             * @param mass New mass value.
             */
            void setMass( real_Num mass );

            /**
             * @brief Get the current mass of the body.
             * @return Mass in world units.
             */
            real_Num getMass() const;

            /**
             * @brief Set mass and mass-space inertia (MOI) for the body.
             * @param mass Mass value to set.
             * @param moi Mass-space inertia diagonal (principal moments).
             */
            void setMassProps( real_Num mass, const Vector3<real_Num> &moi );

            /**
             * @brief Compute the velocity of a given world-space point attached to the body.
             * @param point World-space position of the point.
             * @return Velocity of the point in world-space.
             */
            Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &point );

            /**
             * @brief Compute the velocity of a point expressed in local space.
             * @param point Local-space position of the point.
             * @return Velocity vector in local-space.
             */
            Vector3<real_Num> getLocalPointVelocity( const Vector3<real_Num> &point );

            /**
             * @brief Get the mass-space (diagonal) inertia tensor used by the physics solver.
             * @return Diagonal inertia vector.
             */
            Vector3<real_Num> getMassSpaceInertiaTensor() const;

            /**
             * @brief Set the mass-space diagonal inertia tensor for this body.
             * @param massSpaceInertiaTensor Diagonal inertia values.
             */
            void setMassSpaceInertiaTensor( const Vector3<real_Num> &massSpaceInertiaTensor );

            /**
             * @brief Get the current world transform of the associated physics actor.
             * @return World-space transform (position + orientation).
             */
            Transform3<real_Num> getTransform() const;

            /**
             * @brief Rebuild or update collision shapes attached to the physics actor.
             * Called when shape geometry or collision properties change.
             */
            void updateShapes();

            /**
             * @brief Attach a constraint (joint) to this body.
             * @param constraint Constraint instance to attach.
             */
            void addConstraint( SmartPtr<Constraint> constraint );

            /**
             * @brief Detach a constraint previously attached to this body.
             * @param constraint Constraint instance to remove.
             */
            void removeConstraint( SmartPtr<Constraint> constraint );

            /**
             * @brief Check whether the specified constraint is attached to this body.
             * @param constraint Constraint to check for.
             * @return True if attached.
             */
            bool hasConstraint( SmartPtr<Constraint> constraint ) const;

            /**
             * @brief Determine whether any constraints are attached to this body.
             * @return True if one or more constraints exist.
             */
            bool hasConstraints() const;

            /** @brief Get the number of constraints currently attached to this body. */
            u32 getNumConstraints() const;

            /**
             * @brief Get a list of constraints currently attached to this body.
             * @return Array of constraint instances.
             */
            Array<SmartPtr<Constraint>> getConstraints() const;

            /**
             * @brief Replace the current set of constraints with the provided list.
             * @param constraints New array of constraints to attach.
             */
            void setConstraints( const Array<SmartPtr<Constraint>> &constraints );

            /**
             * @brief Retrieve the optional listener that receives Rigidbody callbacks.
             * @return Listener instance or null if none set.
             */
            SmartPtr<RigidbodyListener> getRigidbodyListener() const;

            /**
             * @brief Set a listener to receive events such as collision notifications.
             * @param rigidbodyListener Listener instance to register.
             */
            void setRigidbodyListener( SmartPtr<RigidbodyListener> rigidbodyListener );

            WP_CLASS_REGISTER_DECL;

        protected:
            void setCollisionMaskInternal( u32 collisionMask, bool explicitOverride );

            /**
             * @brief Applies collision filtering to the body and attached collision shape.
             */
            void applyCollisionFiltering();

            /**
             * @brief Updates the internal physics state of the Rigidbody.
             */
            void updatePhysicsState();

            /**
             * @brief Updates all constraints attached to the Rigidbody.
             */
            void updateConstraints();

            /**
             * @brief Creates the underlying physics rigidbody object.
             */
            void createRigidbodyObject();

            /**
             * @brief Removes the Rigidbody from the physics scene.
             */
            void removeFromScene();

            /**
             * @brief Adds the Rigidbody to the physics scene.
             */
            void addToScene();

            /**
             * @brief Destroys the underlying physics rigidbody object.
             */
            void destroyRigidbodyObject();

            /**
             * @brief Attaches the shape(s) to the Rigidbody in the physics engine.
             */
            void attachShape();

            /**
             * @brief Optional listener that receives callbacks for this Rigidbody (e.g. contacts).
             */
            AtomicSmartPtr<RigidbodyListener> m_rigidbodyListener;

            /**
             * @brief Pointer to the dynamic physics actor when this body is simulated.
             *
             * Null when the body is static or the actor has not been created.
             */
            AtomicSmartPtr<physics::IRigidDynamic3> m_rigidDynamic;

            /**
             * @brief Pointer to the static physics actor when this body is static.
             */
            AtomicSmartPtr<physics::IRigidStatic3> m_rigidStatic;

            /**
             * @brief Optional cloned physics body used for internal purposes (if any).
             */
            AtomicSmartPtr<physics::IPhysicsBody3> m_clonedActor;

            /**
             * @brief Physics scene this body is registered with (may be null).
             */
            AtomicSmartPtr<physics::IPhysicsScene3> m_scene;

            /**
             * @brief Physics material used for contact properties of attached shapes.
             */
            AtomicSmartPtr<physics::IPhysicsMaterial3> m_material;

            /**
             * @brief Cached local-space axis-aligned bounding box for attached shapes.
             */
            AtomicObject<AABB3<real_Num>> m_bounds;

            /**
             * @brief Diagonal mass-space inertia tensor (principal moments of inertia).
             */
            AtomicObject<Vector3<real_Num>> m_massSpaceInertiaTensor = Vector3<real_Num>::unit();

            /**
             * @brief Desired/current linear velocity, retained before a dynamic body is created.
             */
            AtomicObject<Vector3<real_Num>> m_linearVelocity = Vector3<real_Num>::zero();

            /**
             * @brief Desired/current angular velocity, retained before a dynamic body is created.
             */
            AtomicObject<Vector3<real_Num>> m_angularVelocity = Vector3<real_Num>::zero();

            /**
             * @brief Mass value of this body. Defaults to a large value to represent heavy bodies.
             */
            AtomicFloat<real_Num> m_mass = static_cast<real_Num>( 1000.0 );

            /**
             * @brief Collision group bitmask used for filtering.
             */
            atomic_u32 m_groupMask = 0;

            /**
             * @brief Collision mask indicating which groups this body collides with.
             */
            atomic_u32 m_collisionMask = 0;

            /** @brief True when collision mask came from component data or direct setter. */
            atomic_bool m_hasCollisionMaskOverride = false;

            /**
             * @brief True when this body is kinematic (driven by the application).
             */
            atomic_bool m_isKinematic = false;

            /**
             * @brief Reference counter used while copying simulated transforms back to the actor.
             */
            atomic_s32 m_transformReferences = 0;

            /**
             * @brief Application-side maximum linear speed (0 = no limit).
             */
            AtomicFloat<real_Num> m_maxLinearVelocity = static_cast<real_Num>( 0.0 );

            /**
             * @brief Application-side maximum angular speed (0 = no limit).
             */
            AtomicFloat<real_Num> m_maxAngularVelocity = static_cast<real_Num>( 0.0 );

            /** @brief Linear damping applied by the physics solver. */
            AtomicFloat<real_Num> m_linearDamping = static_cast<real_Num>( 0.0 );

            /** @brief Angular damping applied by the physics solver. */
            AtomicFloat<real_Num> m_angularDamping = static_cast<real_Num>( 0.05 );

            /** @brief Sleep energy threshold for dynamic bodies. */
            AtomicFloat<real_Num> m_sleepThreshold = static_cast<real_Num>( 0.005 );

            /** @brief Stabilization energy threshold for dynamic bodies. */
            AtomicFloat<real_Num> m_stabilizationThreshold = static_cast<real_Num>( 0.0 );

            /** @brief Contact reporting impulse threshold. */
            AtomicFloat<real_Num> m_contactReportThreshold = static_cast<real_Num>( 0.0 );

            /** @brief Minimum position iterations used by the constraint solver. */
            atomic_u32 m_solverPositionIterations = 4;

            /** @brief Minimum velocity iterations used by the constraint solver. */
            atomic_u32 m_solverVelocityIterations = 1;

            /** @brief Whether scene gravity affects this body. */
            atomic_bool m_useGravity = true;

            /** @brief Whether continuous collision detection is enabled. */
            atomic_bool m_continuousCollisionDetection = false;

            /**
             * @brief Weak references to constraints attached to this body.
             */
            ConcurrentArray<WeakPtr<Constraint>> m_constraints;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Rigidbody_h__
