#ifndef WPPhysxRigidStatic3_h__
#define WPPhysxRigidStatic3_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Physics/RigidStatic3.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <WPPhysx/WPPhysxRigidBody3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief PhysX implementation of a rigid static physics body.
         *
         * This class wraps a PhysX `PxRigidStatic` and implements the
         * `RigidStatic3` interface through the `PhysxRigidBody3` CRTP base.
         * It exposes methods to manipulate velocities, forces and properties
         * and integrates with the engine's shared-object and state systems.
         */
        class PhysxRigidStatic : public PhysxRigidBody3<RigidStatic3>
        {
        public:
            /**
             * @brief Listener that forwards state messages to the owning rigid static.
             *
             * The listener keeps a weak reference to the owner `PhysxRigidStatic` to
             * avoid ownership cycles. It handles incoming state messages and state
             * changes coming from the engine's state system.
             */
            class StateListener : public IStateListener
            {
            public:
                StateListener();
                ~StateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message Smart pointer to the state message to process.
                 * @return True if the message was handled, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Called when a state object changes.
                 * @param state Smart pointer to the changed state object.
                 * @return True if the change was handled, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the owning `PhysxRigidStatic` if it still exists.
                 * @return Shared pointer to the owner or a null pointer if owner expired.
                 */
                SmartPtr<PhysxRigidStatic> getOwner() const;

                /**
                 * @brief Set the owner of this listener.
                 * @param owner Shared pointer to the `PhysxRigidStatic` that owns this listener.
                 */
                void setOwner( SmartPtr<PhysxRigidStatic> owner );

            protected:
                /// Weak reference to the owning `PhysxRigidStatic`.
                AtomicWeakPtr<PhysxRigidStatic> m_owner;
            };

            /**
             * @brief Construct a new `PhysxRigidStatic` instance.
             */
            PhysxRigidStatic();

            /**
             * @brief Destroy the `PhysxRigidStatic` and release PhysX resources.
             */
            ~PhysxRigidStatic() override;

            /**
             * @copydoc ISharedObject::load
             *
             * Loads runtime data required by the body. The `data` pointer is provided
             * by the engine's shared-object system.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * Unloads and releases runtime data associated with the body.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IRigidStatic3::setLinearVelocity
             * @param linVel Linear velocity vector to set.
             * @param autowake If true, wake the actor in the simulation immediately.
             */
            void setLinearVelocity( const Vector3<physics_Num> &linVel, bool autowake = true ) override;

            /**
             * @copydoc IRigidStatic3::getLinearVelocity
             * @return Current linear velocity of the object.
             */
            Vector3<physics_Num> getLinearVelocity() const override;

            /**
             * @copydoc IRigidStatic3::setAngularVelocity
             * @param angVel Angular velocity vector to set.
             * @param autowake If true, wake the actor in the simulation immediately.
             */
            void setAngularVelocity( const Vector3<physics_Num> &angVel, bool autowake = true ) override;

            /**
             * @copydoc IRigidStatic3::getAngularVelocity
             * @return Current angular velocity of the object.
             */
            Vector3<physics_Num> getAngularVelocity() const override;

            /**
             * @copydoc IRigidStatic3::addForce
             * @param force Force vector to apply to the body.
             */
            void addForce( const Vector3<physics_Num> &force ) override;

            /**
             * @copydoc IRigidStatic3::clearForce
             * @param mode The force mode to clear (force/impulse/etc.).
             */
            void clearForce( ForceModeEnum mode = ForceModeEnum::Force ) override;

            /**
             * @copydoc IRigidStatic3::addTorque
             * @param torque Torque vector to apply to the body.
             */
            void addTorque( const Vector3<physics_Num> &torque ) override;

            /**
             * @copydoc IRigidStatic3::clearTorque
             * @param mode The torque mode to clear (force/impulse/etc.).
             */
            void clearTorque( ForceModeEnum mode = ForceModeEnum::Force ) override;

            /**
             * @copydoc IRigidStatic3::clone
             * @return A deep copy of this physics body as an `IPhysicsBody3`.
             */
            SmartPtr<IPhysicsBody3> clone() override;

            /**
             * @brief Access the underlying PhysX `PxRigidStatic` pointer.
             * @return Raw pointer to the `PxRigidStatic` or nullptr if not set.
             */
            physx::PxRigidStatic *getRigidStatic() const;

            /**
             * @brief Set the underlying PhysX `PxRigidStatic` pointer.
             * @param rigidStatic Raw pointer to the PhysX rigid static actor. Ownership
             *        is not transferred; this class will not delete the pointer.
             */
            void setRigidStatic( physx::PxRigidStatic *rigidStatic );

            /**
             * @copydoc ISharedObject::getChildObjects
             * @return Array of child shared objects owned by this object.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc IPhysicsBody3::getProperties
             * @return Properties object describing the physical parameters of the body.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IPhysicsBody3::setProperties
             * @param properties New properties to apply to the physics body.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Raw pointer to the PhysX rigid static actor owned by this wrapper.
             *
             * This pointer may be null when the object is not yet loaded or after
             * it has been released.
             */
            physx::PxRigidStatic *m_rigidStatic = nullptr;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxRigidStatic3_h__
