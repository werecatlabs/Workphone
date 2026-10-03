#ifndef __WP_ICOMPONENT_H__
#define __WP_ICOMPONENT_H__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/Scene/ISubComponent.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class IComponent
         * @brief Interface for scene components attached to game actors.
         *
         * Components are modular objects that attach to an actor and provide behavior,
         * rendering, collision, or other responsibilities. This interface extends
         * IResource and exposes lifecycle, state, hierarchy and event APIs used by
         * the scene and component systems.
         *
         * Implementations are expected to be lightweight wrappers around specific
         * functionality and cooperate with an IComponentSystem. Many functions are
         * virtual and intended to be overridden by concrete component types.
         *
         * Typical responsibilities:
         * - Maintain reference to the owning actor.
         * - Manage subcomponents and per-component runtime state objects.
         * - Respond to actor/scene events and propagate hierarchy/visibility changes.
         *
         * @note Implementations should not assume thread-safety unless explicitly
         * documented; synchronization is the caller's responsibility.
         *
         * @author Zane Desir
         * @version 1.0
         */
        class WPCore_API IComponent : public IResource
        {
        public:
            /**
             * @brief Component lifecycle and runtime states.
             *
             * These states represent the component's mode or lifecycle stage and are
             * used by scene management code to drive behavior (for example, switching
             * between edit and play behavior).
             */
            enum class State
            {
                None,      /**< No state assigned. */
                Create,    /**< Component is being created/initialized. */
                Destroyed, /**< Component has been destroyed and should be considered invalid. */
                Edit,      /**< Editor/edit mode. Component may expose editor-only behavior. */
                Play,      /**< Runtime/play mode. Normal runtime behavior is expected. */
                Reset,     /**< Component is being reset to defaults. */
                Count      /**< Sentinel value: number of states. */
            };

            /**
             * @name Component Event Identifiers
             * Static hash identifiers used for triggering and listening to component events.
             * @{ */
            static const hash_type actorFlagsChanged; /**< Event: actor flags changed on owning actor. */
            static const hash_type actorReset;        /**< Event: owning actor was reset. */
            static const hash_type actorUnload;       /**< Event: owning actor was unloaded. */
            static const hash_type sceneWasLoaded; /**< Event: scene containing the actor was loaded. */
            static const hash_type parentChanged;  /**< Event: component's parent changed. */
            static const hash_type hierarchyChanged; /**< Event: component hierarchy changed. */
            static const hash_type childAdded;       /**< Event: a child subcomponent was added. */
            static const hash_type childRemoved;     /**< Event: a child subcomponent was removed. */
            static const hash_type
                childAddedInHierarchy; /**< Event: child was added somewhere in the hierarchy. */
            static const hash_type
                childRemovedInHierarchy; /**< Event: child was removed somewhere in the hierarchy. */
            static const hash_type visibilityChanged; /**< Event: component visibility changed. */
            static const hash_type enabledChanged;    /**< Event: component enabled state changed. */
            static const hash_type staticChanged; /**< Event: component static/dynamic state changed. */
            static const hash_type
                triggerCollisionEnter; /**< Event: trigger collision enter for this component. */
            static const hash_type
                triggerCollisionLeave; /**< Event: trigger collision leave for this component. */
            static const hash_type componentLoaded; /**< Event: component finished loading. */
            /** @} */
            /**
             * @name Component Flag Identifiers
             * Bit flags used to manage component-level state.
             * @{ */
            static const u32 ComponentReservedFlag; /**< Reserved flag value (zero) for future use. */
            static const u32
                ComponentEnabledFlag; /**< Bit flag that indicates component enabled state. */
            /** @} */

            /**
             * @brief Names of the possible component states.
             */
            static const Array<String> componentStateNames;

            /**
             * @brief String identifier for the enabled state.
             */
            static const String enabledStr;

            /**
             * @brief String identifier for the dirty state.
             */
            static const String dirtyStr;

            /**
             * @brief String identifier for the component name.
             */
            static const String nameStr;

            /**
             * @brief String identifier for the component flags.
             */
            static const String componentFlagsStr;

            /**
             * @brief String identifier for the component state.
             */
            static const String stateStr;

            IComponent();

            IComponent( u32 poolTypeID );

            /**
             * @brief Virtual destructor.
             *
             * Ensure proper cleanup for derived implementations. Derived destructors
             * should release resources and detach from systems where appropriate.
             */
            ~IComponent() override;

            /**
             * @brief Called when component flags change.
             * @param flags New flags bitmask.
             * @param oldFlags Previous flags bitmask.
             *
             * Implementations should react to changes in the component's flags
             * (for example, enabling/disabling behavior, toggling static state, etc.).
             */
            virtual void updateFlags( u32 flags, u32 oldFlags ) = 0;

            /**
             * @brief Retrieves a raw pointer to the owning actor.
             * @return Raw pointer to the IGameActor, or nullptr if not attached.
             *
             * This does not increase the actor's reference count; use getActor() when ownership
             * semantics are required.
             */
            virtual IGameActor *getActorPtr() const = 0;

            /**
             * @brief Retrieves a smart pointer to the owning actor.
             * @return SmartPtr to the owning actor (increments reference count).
             */
            virtual SmartPtr<IGameActor> getActor() const = 0;

            /**
             * @brief Attaches or detaches this component to/from an actor.
             * @param actor Smart pointer to the actor to attach to, or nullptr to detach.
             *
             * Implementations should update internal state and notify the component system if required.
             */
            virtual void setActor( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Retrieves the component's properties container.
             * @return SmartPtr to the Properties object, or nullptr if no properties are defined.
             *
             * Properties provide serialized and configurable values for the component.
             */
            SmartPtr<Properties> getProperties() const override = 0;

            /**
             * @brief Replaces the component's properties container.
             * @param properties SmartPtr to the new Properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override = 0;

            /**
             * @brief Synchronize component transform from the owning actor.
             *
             * Called when the actor's transform changes; implementations should
             * update any local transforms or transforms used for rendering/physics.
             */
            virtual void updateTransform() = 0;

            /**
             * @brief Apply a specific transform to the component.
             * @param transform Transform value to apply.
             */
            virtual void updateTransform( const Transform3<real_Num> &transform ) = 0;

            /**
             * @brief Recalculate visibility state for this component.
             *
             * This may take into account actor visibility, component flags, LOD,
             * editor overrides, or hierarchical visibility rules.
             */
            virtual void updateVisibility() = 0;

            /**
             * @brief Update ordering/priorities used by rendering or update traversal.
             *
             * Typical uses: sorting renderable components, changing update order inside a parent.
             */
            virtual void updateOrder() = 0;

            /**
             * @brief Refresh material assignments or material-related state.
             *
             * Implementations should rebind or re-evaluate materials used for rendering.
             */
            virtual void updateMaterials() = 0;

            /**
             * @brief Notify dependent components to update if this component's state changed.
             *
             * Used when a component's change requires other components to refresh their cached data.
             */
            virtual void updateDependentComponents() = 0;

            /**
             * @brief Update the internal state used for smooth/animated transforms.
             *
             * Implementations that interpolate transforms should update their interpolation state here.
             */
            virtual void updateSmoothTransformState() = 0;

            /**
             * @brief Set the component runtime state.
             * @param state New state to apply.
             */
            virtual void setState( State state ) = 0;

            /**
             * @brief Retrieve component-level bit flags.
             * @return Current flags bitmask.
             */
            virtual u32 getComponentFlags() const = 0;

            /**
             * @brief Replace the component-level bit flags.
             * @param flags New bitmask to set.
             */
            virtual void setComponentFlags( u32 flags ) = 0;

            /**
             * @brief Set or clear a single component flag.
             * @param flag Bitmask for the flag to change.
             * @param value True to set the flag, false to clear it.
             */
            virtual void setComponentFlag( u32 flag, bool value ) = 0;

            /**
             * @brief Check whether a specific flag is set.
             * @param flag Flag bit to test.
             * @return True when the flag is set, false otherwise.
             */
            virtual bool getComponentFlag( u32 flag ) const = 0;

            /**
             * @brief Enable or disable the component.
             * @param enabled True to enable the component, false to disable it.
             *
             * Enabling/disabling typically affects update callbacks, visibility and
             * event handling. Implementations should emit enabledChanged events where appropriate.
             */
            virtual void setEnabled( bool enabled ) = 0;

            /**
             * @brief Query whether the component is enabled.
             * @return True if enabled, otherwise false.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Get the component's current State value.
             * @return Current State enumeration value.
             */
            virtual State getState() const = 0;

            /**
             * @brief Access runtime component events.
             * @return Array of SmartPtr<IComponentEvent> currently registered.
             *
             * Events represent callbacks or scripted hooks associated with the component.
             */
            virtual Array<SmartPtr<IComponentEvent>> getEvents() const = 0;

            /**
             * @brief Replace the component's event array.
             * @param events New array of events.
             */
            virtual void setEvents( Array<SmartPtr<IComponentEvent>> events ) = 0;

            /**
             * @brief Add a component-level event.
             * @param event Event instance to add.
             */
            virtual void addEvent( SmartPtr<IComponentEvent> event ) = 0;

            /**
             * @brief Remove a previously added component event.
             * @param event Event instance to remove.
             */
            virtual void removeEvent( SmartPtr<IComponentEvent> event ) = 0;

            /**
             * @brief Remove all component events.
             */
            virtual void removeEvents() = 0;

            /**
             * @brief Add a child subcomponent.
             * @param child Subcomponent to add.
             *
             * The component takes ownership via SmartPtr and the child will be managed
             * as part of this component's hierarchy.
             */
            virtual void addSubComponent( SmartPtr<ISubComponent> child ) = 0;

            /**
             * @brief Remove a child subcomponent.
             * @param child Subcomponent to remove (must match an existing child).
             */
            virtual void removeSubComponent( SmartPtr<ISubComponent> child ) = 0;

            /**
             * @brief Remove a subcomponent by its index in the child array.
             * @param index Zero-based index of the child to remove.
             */
            virtual void removeSubComponentByIndex( u32 index ) = 0;

            /**
             * @brief Get the number of subcomponents attached to this component.
             * @return Count of subcomponents.
             */
            virtual u32 getNumSubComponents() const = 0;

            /**
             * @brief Get a subcomponent by index.
             * @param index Zero-based child index.
             * @return SmartPtr to the subcomponent at the specified index.
             */
            virtual SmartPtr<ISubComponent> getSubComponentByIndex( u32 index ) const = 0;

            /**
             * @brief Get all direct subcomponents.
             * @return Array of SmartPtr<ISubComponent> containing direct children.
             */
            virtual Array<SmartPtr<ISubComponent>> getSubComponents() const = 0;

            /**
             * @brief Replace the subcomponent list.
             * @param components New array of subcomponents to adopt.
             */
            virtual void setSubComponents( const Array<SmartPtr<ISubComponent>> &components ) = 0;

            /**
             * @brief Get the mutex used for thread-safe operations on this component.
             * @return Reference to the RecursiveSpinMutex instance.
             */
            virtual SmartPtr<IComponent> getParent() const = 0;

            /**
             * @brief Set the parent component for this component.
             * @param parent SmartPtr to the new parent component.
             */
            virtual void setParent( SmartPtr<IComponent> parent ) = 0;

            /**
             * @brief Compare a tag string against the actor's tag.
             * @param tag Tag to compare.
             * @return True if tags match, otherwise false.
             *
             * Useful for lightweight filtering without acquiring the actor pointer.
             */
            virtual bool compareTag( const String &tag ) const = 0;

            /**
             * @brief Handle an incoming event targeted at this component.
             * @param eventType Type of the incoming event.
             * @param eventValue Event identifier (hash).
             * @param arguments Array of event parameters.
             * @param sender Smart pointer to the sending object (may be nullptr).
             * @param object Smart pointer to a related object (may be nullptr).
             * @param event Smart pointer to the original event object (may be nullptr).
             * @return Parameter containing the event handler result.
             *
             * Implementations should process the event or forward it to subcomponents
             * and return meaningful result parameters used by callers.
             */
            virtual Parameter handleEvent( EventType eventType, hash_type eventValue,
                                           const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                           SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) = 0;

            /**
             * @brief Add an opaque state object associated with this component.
             * @param state State object to add (SmartPtr to ISharedObject).
             *
             * State objects are typed containers used to attach auxiliary runtime data
             * to a component. They are retrievable via getStateByTypeId or getComponentStateByType.
             */
            virtual void addState( SmartPtr<ISharedObject> state ) = 0;

            /**
             * @brief Remove an attached state object.
             * @param state State object to remove.
             */
            virtual void removeState( SmartPtr<ISharedObject> state ) = 0;

            /**
             * @brief Get a state object by its type id.
             * @param typeId Numeric type identifier (obtained from typeInfo()).
             * @return Reference to the SmartPtr holding the state; may be nullptr SmartPtr.
             *
             * The returned reference allows modifying the stored SmartPtr in-place.
             */
            virtual SmartPtr<ISharedObject> &getStateByTypeId( u32 typeId ) = 0;

            /**
             * @brief Get a state object by its type id (const variant).
             * @param typeId Numeric type identifier.
             * @return Const reference to the SmartPtr holding the state.
             */
            virtual const SmartPtr<ISharedObject> &getStateByTypeId( u32 typeId ) const = 0;

            /**
             * @brief Get all attached state objects.
             * @return Array of SmartPtr<ISharedObject> representing all states.
             */
            virtual Array<SmartPtr<ISharedObject>> getStates() const = 0;

            /**
             * @brief Get the component's axis-aligned bounding box in local space.
             * @return AABB3<real_Num> bounding the component; may be empty if not applicable.
             *
             * Used by spatial queries, culling and editor visualization.
             */
            virtual AABB3<real_Num> getBoundingBox() const = 0;

            /**
             * @brief Get a raw pointer to the component system that manages this component.
             * @return Raw pointer to IComponentSystem or nullptr if not assigned.
             */
            virtual IComponentSystem *getComponentSystemPtr() const = 0;

            /**
             * @brief Get a smart pointer to the component system that manages this component.
             * @return SmartPtr<IComponentSystem> to the owning system.
             */
            virtual SmartPtr<IComponentSystem> getComponentSystem() const = 0;

            /**
             * @brief Assign the component system that will manage this component.
             * @param componentSystem SmartPtr to the component system.
             */
            virtual void setComponentSystem( SmartPtr<IComponentSystem> componentSystem ) = 0;

            /**
             * @brief Opaque pointer to system-specific component data.
             * @return Void pointer previously set via setComponentSystemData.
             *
             * This allows component systems to attach bookkeeping data to components.
             */
            virtual void *getComponentSystemData() const = 0;

            /**
             * @brief Set opaque system-specific component data.
             * @param componentSystemData Void pointer owned/managed by the caller or system.
             */
            virtual void setComponentSystemData( void *componentSystemData ) = 0;

            /**
             * @brief Gets the fsm object.
             * @return Pointer to the fsm object.
             */
            virtual IFSM *getFsmPtr() const = 0;

            /**
             * @brief Gets the fsm object.
             * @return Pointer to the fsm object.
             */
            virtual SmartPtr<IFSM> getFsm() const = 0;

            /**
             * @brief Sets the fsm object.
             * @param fsm SmartPtr to the fsm object.
             */
            virtual void setFsm( SmartPtr<IFSM> fsm ) = 0;

            /**
             * @brief Sets up the transform update for static objects.
             */
            virtual void updateStatic() = 0;

            /**
             * @brief Retrieve a typed component state object.
             * @tparam T Concrete state type that exposes static typeInfo().
             * @return SafePtr<T> to the stored state instance or nullptr-equivalent if not present.
             *
             * The method uses T::typeInfo() to locate the state. Caller must ensure T
             * is derived from ISharedObject or compatible with stored states.
             */
            template <class T>
            SafePtr<T> getComponentStateByType();

            /**
             * @brief Const variant of getComponentStateByType.
             * @tparam T Concrete state type.
             * @return Const SafePtr<T> to the stored state instance.
             */
            template <class T>
            SafePtr<T> getComponentStateByType() const;

            /**
             * @brief Create and attach a subcomponent using the factory manager.
             * @tparam T Type of subcomponent to create.
             * @return SmartPtr<T> to the newly created and loaded subcomponent.
             *
             * This method requires a valid application manager and factory manager.
             * It asserts if those singletons are not available (see WP_ASSERT).
             */
            template <class T>
            SmartPtr<T> addSubComponentByType();

            /**
             * @brief Find and remove the first subcomponent of the specified type.
             * @tparam T Type of subcomponent to remove.
             *
             * If multiple children of the same type exist, only the first found is removed.
             */
            template <class T>
            void removeSubComponentByType();

            /**
             * @brief Get all direct child subcomponents that derive from T.
             * @tparam T Desired subcomponent type.
             * @return Array of SmartPtr<T> matching direct children of the requested type.
             *
             * The returned array contains SmartPtr instances cast to T using isDerived<T>() checks.
             */
            template <class T>
            Array<SmartPtr<T>> getSubComponentsByType() const;

            // 'c' style linked list.
            AtomicRawPtr<IComponent> m_next;

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        SmartPtr<T> IComponent::addSubComponentByType()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto component = factoryManager->make_ptr<T>();
            addSubComponent( component );
            component->load( nullptr );
            return component;
        }

        template <class T>
        void IComponent::removeSubComponentByType()
        {
            auto subComponents = getSubComponentsByType<T>();
            if( !subComponents.empty() )
            {
                auto subComponent = subComponents.front();
                removeSubComponent( subComponent );
            }
        }

        template <class T>
        Array<SmartPtr<T>> IComponent::getSubComponentsByType() const
        {
            // Get all child components of this component.
            auto children = getSubComponents();

            // Create an array for storing child components of the specified type.
            Array<SmartPtr<T>> childrenByType;
            childrenByType.reserve( children.size() );

            // Loop over all child components.
            for( auto &child : children )
            {
                // Check if the child component is derived from the specified type.
                if( child->isDerived<T>() )
                {
                    // If the child is of the specified type, add it to the array of components of that
                    // type.
                    childrenByType.push_back( child );
                }
            }

            // Return the array of child components that are of the specified type.
            return childrenByType;
        }

        template <class T>
        SafePtr<T> IComponent::getComponentStateByType()
        {
            auto typeInfo = T::typeInfo();
            auto p = getStateByTypeId( typeInfo );
            return workphone::static_pointer_cast<T>( p );
        }

        template <class T>
        SafePtr<T> IComponent::getComponentStateByType() const
        {
            auto typeInfo = T::typeInfo();
            auto p = getStateByTypeId( typeInfo );
            return workphone::static_pointer_cast<T>( p );
        }

    }  // namespace scene
}  // namespace workphone

#endif
