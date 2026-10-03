#ifndef __WP_Component_h__
#define __WP_Component_h__

#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/System/FSMListener.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class Component
         * @brief Concrete base implementation of the scene `IComponent` interface.
         *
         * The `Component` class provides common functionality used by all scene components:
         * - lifecycle methods (`load`, `unload`, `reload`)
         * - flag and enabled state management
         * - actor and component-system association
         * - event handling and FSM integration
         * - child subcomponent management and state storage
         *
         * Subclasses should override the protected hooks such as `handleComponentEvent`
         * and `updateComponentState` to implement component-specific behavior.
         */
        class WPCore_API Component : public Resource<IComponent>
        {
        public:
            /**
             * @class ComponentFSMListener
             * @brief FSM listener that routes
             * state machine events to the owning component.
             * This listener holds a weak
             * reference to the component and is responsible for notifying the component when FSM
             * transitions or events occur.
             */
            class ComponentFSMListener : public FSMListener
            {
            public:
                /**
                 * @brief Construct a new FSM listener.
                 */
                ComponentFSMListener();

                /**
                 * @brief Destroy the FSM listener.
                 */
                ~ComponentFSMListener() override;

                /**
                 * @brief Unload any data associated with the FSM listener.
                 *
                 * Implementations should release or reset any resources associated with
                 * the provided data pointer.
                 *
                 * @param data Shared object previously passed to the listener.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle an FSM event.
                 *
                 * Called when the finite state machine triggers an event. Implementations
                 * should forward or map the FSM event to component logic and return the
                 * appropriate FSMReturnType.
                 *
                 * @param state Current FSM state id.
                 * @param eventType Event type raised by the FSM.
                 * @return FSMReturnType Result code for the FSM runtime.
                 */
                FSMReturnType handleEvent( u32 state, FSMEvent eventType ) override;

                /**
                 * @brief Get the owning `Component` as a `SmartPtr`.
                 * @return SmartPtr<Component> Owner component or nullptr if expired.
                 */
                SmartPtr<Component> getOwner() const;

                /**
                 * @brief Set the owning `Component` for this FSM listener.
                 * @param owner Smart pointer to the owner component.
                 */
                void setOwner( SmartPtr<Component> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /**
                 * @brief Weak pointer to the owning component to avoid strong reference cycles.
                 */
                AtomicWeakPtr<Component> m_owner;
            };

            /**
             * @brief Default construct a `Component`.
             */
            Component();

            Component( u32 poolTypeId );

            /**
             * @brief Deleted copy constructor to prevent accidental copying of components.
             */
            Component( const Component &other ) = delete;

            /**
             * @brief Destructor. Cleans up listeners and references held by the component.
             */
            ~Component() override;

            /**
             * @copydoc IComponent::load
             * @param data Configuration or serialized data used to initialize the component.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IComponent::unload
             * @param data Optional data describing unload context.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IComponent::reload
             * @param data Optional data used to reload or reinitialize the component.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IComponent::updateFlags
             * @param flags New combined flag bits.
             * @param oldFlags Previous combined flag bits.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Get a raw (non-owning) pointer to the actor this component is attached to.
             * @return IGameActor* Raw pointer or nullptr if not attached.
             */
            IGameActor *getActorPtr() const override;

            /**
             * @brief Get the owning smart pointer to the actor this component is attached to.
             * @return SmartPtr<IGameActor> Smart pointer to the actor or nullptr if not set.
             */
            SmartPtr<IGameActor> getActor() const override;

            /**
             * @brief Associate this component with the provided actor.
             * @param actor Smart pointer to the actor to attach to.
             */
            void setActor( SmartPtr<IGameActor> actor ) override;

            /**
             * @brief Return the current component flags bitmask.
             * @return u32 Bitmask of component flags.
             */
            u32 getComponentFlags() const override;

            /**
             * @brief Replace the component flags bitmask.
             * @param flags New bitmask to store.
             */
            void setComponentFlags( u32 flags ) override;

            /**
             * @brief Set or clear a single flag in the component flags bitmask.
             * @param flag Flag bit to modify.
             * @param value True to set the bit, false to clear it.
             */
            void setComponentFlag( u32 flag, bool value ) override;

            /**
             * @brief Test whether a given component flag is set.
             * @param flag Flag bit to test.
             * @return true if the bit is set; otherwise false.
             */
            bool getComponentFlag( u32 flag ) const override;

            /**
             * @brief Enable or disable the component.
             * @param enabled True to enable; false to disable.
             */
            void setEnabled( bool enabled ) override;

            /**
             * @brief Query whether the component is enabled.
             * @return true if enabled; otherwise false.
             */
            bool isEnabled() const override;

            /**
             * @copydoc IComponent::toData
             * @return Serialized representation of the component state.
             */
            SmartPtr<ISharedObject> toData() const override;

            /**
             * @copydoc IComponent::fromData
             * @param data Serialized representation to restore component state from.
             */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IComponent::getChildObjects
             * @return Array of child `ISharedObject` instances owned by the component.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc IComponent::getProperties
             * @return Properties object associated with the component or nullptr.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IComponent::setProperties
             * @param properties Properties to associate with the component.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc IComponent::updateTransform
             * @brief Recalculate any cached transform-dependent data for the component.
             */
            void updateTransform() override;

            /**
             * @copydoc IComponent::updateTransform
             * @param transform New transform to apply to the component.
             */
            void updateTransform( const Transform3<real_Num> &transform ) override;

            /**
             * @copydoc IComponent::updateVisibility
             * @brief Recompute visibility criteria for the component (default does nothing).
             */
            void updateVisibility() override;

            /**
             * @copydoc IComponent::updateOrder
             * @brief Notify the system that the component's draw/interaction order changed.
             */
            void updateOrder() override;

            /**
             * @copydoc IComponent::updateMaterials
             * @brief Update material references or parameter bindings used by the component.
             */
            void updateMaterials() override;

            /**
             * @copydoc IComponent::updateDependentComponents
             * @brief Trigger updates on components that depend on this component's state.
             */
            void updateDependentComponents() override;

            /**
             * @copydoc IComponent::setState
             * @param state New state to set for the component FSM or logical state.
             */
            void setState( State state ) override;

            /**
             * @copydoc IComponent::getState
             * @return Current component state.
             */
            State getState() const override;

            /**
             * @copydoc IComponent::getEvents
             * @return Array of events registered on this component.
             */
            Array<SmartPtr<IComponentEvent>> getEvents() const override;

            /**
             * @copydoc IComponent::setEvents
             * @param events New list of events to register on this component.
             */
            void setEvents( Array<SmartPtr<IComponentEvent>> events ) override;

            /**
             * @copydoc IComponent::addEvent
             * @param event Event to append to this component's event list.
             */
            void addEvent( SmartPtr<IComponentEvent> event ) override;

            /**
             * @copydoc IComponent::removeEvent
             * @param event Event to remove if it is present.
             */
            void removeEvent( SmartPtr<IComponentEvent> event ) override;

            /**
             * @copydoc IComponent::removeEvents
             * @brief Remove and clear all events from this component.
             */
            void removeEvents() override;

            /**
             * @copydoc IComponent::addSubComponent
             * @param child Subcomponent to add as a child of this component.
             */
            void addSubComponent( SmartPtr<ISubComponent> child ) override;

            /**
             * @copydoc IComponent::removeSubComponent
             * @param child Subcomponent to detach and remove.
             */
            void removeSubComponent( SmartPtr<ISubComponent> child ) override;

            /**
             * @copydoc IComponent::removeSubComponentByIndex
             * @param index Index of the subcomponent to remove.
             */
            void removeSubComponentByIndex( u32 index ) override;

            /**
             * @copydoc IComponent::getNumSubComponents
             * @return Number of attached subcomponents.
             */
            u32 getNumSubComponents() const override;

            /**
             * @copydoc IComponent::getSubComponentByIndex
             * @param index Index of the requested subcomponent.
             * @return SmartPtr<ISubComponent> Subcomponent at the given index.
             */
            SmartPtr<ISubComponent> getSubComponentByIndex( u32 index ) const override;

            /**
             * @copydoc IComponent::getSubComponents
             * @return All subcomponents attached to this component.
             */
            Array<SmartPtr<ISubComponent>> getSubComponents() const override;

            /**
             * @copydoc IComponent::setSubComponents
             * @param components Array of subcomponents to replace current children.
             */
            void setSubComponents( const Array<SmartPtr<ISubComponent>> &components ) override;

            SmartPtr<IComponent> getParent() const override;
            void setParent( SmartPtr<IComponent> parent ) override;

            /**
             * @copydoc IComponent::compareTag
             * @param tag Tag string to compare against the component's tag.
             * @return true if tags match; otherwise false.
             */
            bool compareTag( const String &tag ) const override;

            /**
             * @copydoc IComponent::handleEvent
             * @brief Primary event entry point for this component.
             * @return Parameter Packed result returned by the event handling.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            /**
             * @copydoc IComponent::addState
             * @param state State object to store for this component.
             */
            void addState( SmartPtr<ISharedObject> state ) override;

            /**
             * @copydoc IComponent::removeState
             * @param state State object to remove from storage.
             */
            void removeState( SmartPtr<ISharedObject> state ) override;

            /**
             * @copydoc IComponent::getStateByTypeId
             * @param typeId Type identifier used to look up a stored state object.
             * @return SmartPtr<ISharedObject>& Reference to the internal stored smart pointer.
             */
            SmartPtr<ISharedObject> &getStateByTypeId( u32 typeId ) override;

            /**
             * @copydoc IComponent::getStateByTypeId
             * @param typeId Type identifier used to look up a stored state object.
             * @return const SmartPtr<ISharedObject>& Const reference to the stored state pointer.
             */
            const SmartPtr<ISharedObject> &getStateByTypeId( u32 typeId ) const override;

            /**
             * @copydoc IComponent::getStates
             * @return Array of all stored state objects.
             */
            Array<SmartPtr<ISharedObject>> getStates() const override;

            /**
             * @copydoc IComponent::getBoundingBox
             * @return Axis-aligned bounding box of the component in local or world space
             * depending on component semantics.
             */
            AABB3<real_Num> getBoundingBox() const override;

            /**
             * @brief Get raw pointer to the component system that owns or manages this component.
             * @return IComponentSystem* Raw pointer or nullptr if none.
             */
            IComponentSystem *getComponentSystemPtr() const override;

            /**
             * @brief Get smart pointer to the component system that owns this component.
             * @return SmartPtr<IComponentSystem> Smart pointer or nullptr.
             */
            SmartPtr<IComponentSystem> getComponentSystem() const override;

            /**
             * @brief Associate this component with a component system manager.
             * @param componentSystem Smart pointer to the component system.
             */
            void setComponentSystem( SmartPtr<IComponentSystem> componentSystem ) override;

            /**
             * @brief Retrieve opaque per-system data pointer associated with the component.
             * @return void* Opaque pointer stored by the component system.
             */
            void *getComponentSystemData() const override;

            /**
             * @brief Set opaque per-system data pointer associated with the component.
             * @param componentSystemData Pointer to store.
             */
            void setComponentSystemData( void *componentSystemData ) override;

            /**
             * @copydoc IComponent::updateSmoothTransformState
             * @brief Update any interpolation or smoothing state for transforms.
             */
            void updateSmoothTransformState() override;

            /**
             * @brief Get raw pointer to the component's FSM instance if present.
             * @return IFSM* Raw pointer or nullptr.
             */
            IFSM *getFsmPtr() const override;

            /**
             * @brief Get smart pointer to the component's FSM instance if present.
             * @return SmartPtr<IFSM> Smart pointer or nullptr.
             */
            SmartPtr<IFSM> getFsm() const override;

            /**
             * @brief Associate an FSM instance with this component.
             * @param fsm Smart pointer to the FSM instance to attach.
             */
            void setFsm( SmartPtr<IFSM> fsm ) override;

            /**
             * @brief Sets up the transform update for static objects.
             */
            void updateStatic() override;

            /**
             * @brief Acquire the actor's internal mutex.
             *
             * Use this to protect multi-step operations on the actor that must be atomic.
             */
            void lock() override;

            /** @brief Try to acquire the actor mutex without blocking. */
            bool try_lock() override;

            /** @brief Release the actor mutex. */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handle an FSM-related event for this component.
             *
             * Override this method in derived classes to react to FSM events and
             * return the next state or action code.
             *
             * @param state Current FSM state identifier.
             * @param eventType FSM event type that occurred.
             * @return FSMReturnType Result code used by the FSM runtime.
             */
            virtual FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType );

            /**
             * @brief Update derived component-specific state.
             *
             * Called when the component's logical state changes and subclasses
             * should refresh any cached data or trigger side-effects.
             */
            virtual void updateComponentState();

            /**
             * @brief Weak (atomic) reference to the component system that manages this component.
             */
            AtomicWeakPtr<IComponentSystem> m_componentSystem;

            /**
             * @brief Weak reference to the component's finite state machine (FSM) instance.
             */
            WeakPtr<IFSM> m_componentFSM;

            /**
             * @brief Listener attached to the component's FSM to receive events.
             */
            SmartPtr<IFSMListener> m_componentFsmListener;

            /**
             * @brief Weak pointer to the parent component (if this component is a
             * child).
             */
            AtomicWeakPtr<IComponent> m_parent;

            /**
             * @brief Weak pointer to the actor this component is attached to.
             */
            AtomicWeakPtr<IGameActor> m_actor;

            /**
             * @brief Atomic storage for the component flag bitmask.
             */
            atomic_u32 m_componentFlags = 0;

            /**
             * @brief Opaque pointer for component-system-specific per-component data.
             */
            AtomicValue<void *> m_componentSystemData = nullptr;

            /**
             * @brief Registered high-level component events associated with this component.
             */
            Array<SmartPtr<IComponentEvent>> m_events;

            /**
             * @brief Raw pointers to stored state objects (non-owning array used for lookup).
             */
            Array<ISharedObject *> m_componentStatesPtr;

            /**
             * @brief Thread-safe array of owned state objects.
             */
            ConcurrentArray<SmartPtr<ISharedObject>> m_componentStates;

            /**
             * @brief Thread-safe array of attached child subcomponents.
             */
            ConcurrentArray<SmartPtr<ISubComponent>> m_children;

            /**
             * @brief Class-local static ID extension used for runtime type/ID generation.
             */
            static u32 m_idExt;
        };

        /**
         * @brief Return a raw pointer to the actor this component is attached to.
         * @note Inline for performance and convenience.
         * @return IGameActor* Raw pointer or nullptr.
         */
        inline IGameActor *Component::getActorPtr() const
        {
            return m_actor.get();
        }

        /**
         * @brief Return a raw pointer to the component's FSM instance.
         * @return IFSM* Raw pointer or nullptr.
         */
        inline IFSM *Component::getFsmPtr() const
        {
            return m_componentFSM.get();
        }

    }  // namespace scene
}  // namespace workphone

#endif  // __WP_Component_h__
