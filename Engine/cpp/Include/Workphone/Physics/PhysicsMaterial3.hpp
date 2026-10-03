/**
 * @file PhysicsMaterial3.hpp
 * @brief Concrete Resource wrapper for a 3D physics material interface.
 *
 * Provides documentation for the PhysicsMaterial3 class which implements
 * IPhysicsMaterial3 and exposes material properties (friction, restitution)
 * and contact information. Intended to be used through Resource management
 * and to integrate with the engine's state/context system.
 */

#ifndef PhysicsMaterial3_h__
#define PhysicsMaterial3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/System/Resource.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class PhysicsMaterial3
         * @brief Resource-backed implementation of IPhysicsMaterial3.
         *
         * This class adapts the engine's resource management to a physics material
         * interface. It provides getters/setters for friction (directional),
         * dynamic/static friction components, and restitution. It also exposes
         * contact information and references to the two rigid bodies involved in
         * a contact. Additionally, this class keeps an optional state context and
         * handlers for state changes which can be overridden by derived classes.
         *
         * Lifecycle:
         * - Constructed as a Resource<IPhysicsMaterial3> and typically managed
         *   by the engine resource system.
         *
         * Thread-safety:
         * - The stored state context uses AtomicSmartPtr to allow safe concurrent
         *   reads/updates of the pointer itself; callers must still ensure safe
         *   use of the pointed object where required.
         */
        class WPCore_API PhysicsMaterial3 : public Resource<IPhysicsMaterial3>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes a PhysicsMaterial3 instance. Material properties will
             * have values as defined by the underlying implementation (or defaults).
             */
            PhysicsMaterial3();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup in derived classes and correct destruction
             * when referenced polymorphically.
             */
            ~PhysicsMaterial3() override;

            /**
             * @brief Get friction value for the given direction index.
             * @param direction Direction index or axis identifier (implementation-defined).
             * @return Friction coefficient for the requested direction.
             */
            f32 getFriction( s32 direction ) const override;

            /**
             * @brief Set friction value for the given direction index.
             * @param friction New friction coefficient.
             * @param direction Direction index or axis identifier (implementation-defined).
             */
            void setFriction( f32 friction, s32 direction ) override;

            /**
             * @brief Get dynamic (kinetic) friction for the given direction.
             * @param direction Direction index or axis identifier (implementation-defined).
             * @return Dynamic friction coefficient.
             */
            f32 getDynamicFriction( s32 direction ) const override;

            /**
             * @brief Set dynamic (kinetic) friction for the given direction.
             * @param friction New dynamic friction coefficient.
             * @param direction Direction index or axis identifier (implementation-defined).
             */
            void setDynamicFriction( f32 friction, s32 direction ) override;

            /**
             * @brief Get static friction for the given direction.
             * @param direction Direction index or axis identifier (implementation-defined).
             * @return Static friction coefficient.
             */
            f32 getStaticFriction( s32 direction ) const override;

            /**
             * @brief Set static friction for the given direction.
             * @param friction New static friction coefficient.
             * @param direction Direction index or axis identifier (implementation-defined).
             */
            void setStaticFriction( f32 friction, s32 direction ) override;

            /**
             * @brief Get restitution (bounciness) of the material.
             * @return Restitution coefficient in range typically [0, 1].
             */
            f32 getRestitution() const override;

            /**
             * @brief Set restitution (bounciness) of the material.
             * @param restitution Restitution coefficient (typically in [0, 1]).
             */
            void setRestitution( f32 restitution ) override;

            /**
             * @brief Get the rolling friction coefficient (spheres/capsules).
             */
            f32 getRollingFriction() const override;

            /**
             * @brief Set the rolling friction coefficient.
             */
            void setRollingFriction( f32 friction ) override;

            /**
             * @brief Get the friction combine mode.
             */
            FrictionCombineMode getFrictionCombineMode() const override;

            /**
             * @brief Set the friction combine mode.
             */
            void setFrictionCombineMode( FrictionCombineMode mode ) override;

            /**
             * @brief Get the restitution combine mode.
             */
            RestitutionCombineMode getRestitutionCombineMode() const override;

            /**
             * @brief Set the restitution combine mode.
             */
            void setRestitutionCombineMode( RestitutionCombineMode mode ) override;

            /**
             * @brief Get the data-driven material name.
             */
            String getMaterialName() const override;

            /**
             * @brief Set the data-driven material name.
             */
            void setMaterialName( const String &name ) override;

            /**
             * @brief Get the last reported contact position in world space.
             * @return 3D position of the contact point.
             *
             * The value and validity depend on the physics backend and whether
             * this material instance has an associated contact.
             */
            Vector3<real_Num> getContactPosition() const override;

            /**
             * @brief Get the contact normal at the last reported contact point.
             * @return Contact normal vector in world space.
             *
             * The returned normal is expected to be normalized by the physics backend.
             */
            Vector3<real_Num> getContactNormal() const override;

            /**
             * @brief Get the first rigid body involved in the contact.
             * @return SmartPtr to IRigidBody3 for body A, or null if not available.
             */
            SmartPtr<IRigidBody3> getPhysicsBodyA() const override;

            /**
             * @brief Get the second rigid body involved in the contact.
             * @return SmartPtr to IRigidBody3 for body B, or null if not available.
             */
            SmartPtr<IRigidBody3> getPhysicsBodyB() const override;

            /**
             * @brief Retrieve the associated state context for this material.
             * @return SmartPtr to IStateContext stored on this material (may be null).
             *
             * The state context can be used to query or manipulate runtime state
             * related to this material (for example, to drive visual or gameplay
             * changes when material state changes).
             */
            SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Attach or replace the state context for this material.
             * @param stateContext SmartPtr to the new IStateContext instance.
             *
             * This method sets the state context atomically; ownership and lifetime
             * semantics are handled by SmartPtr/AtomicSmartPtr.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            /**
             * @brief Handle a state message indicating state changes.
             * @param message Message describing the state change.
             *
             * Default implementation forwards or processes messages as appropriate.
             * Override in derived classes to react to particular message types.
             */
            virtual bool handleStateChanged( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handle a state object directly when it changes.
             * @param state New state object reference.
             *
             * Override in derived classes to react to state transitions directly.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Atomic pointer to the state context associated with this material.
             *
             * AtomicSmartPtr provides safe concurrent pointer exchanges; callers
             * must still ensure the thread-safety of operations performed on the
             * IStateContext itself.
             */
            AtomicSmartPtr<IStateContext> m_stateContext;
        };
    }  // namespace physics
}  // namespace workphone

#endif  // PhysicsMaterial3_h__
