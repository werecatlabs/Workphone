#ifndef ComponentEventListener_h__
#define ComponentEventListener_h__

#include <Workphone/Interface/Scene/IComponentEventListener.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Default implementation of an IComponentEventListener.
         *
         * This class links a component event to a specific actor, component and
         * function name. It can serialize to/from data objects and expose
         * component properties. The listener holds weak references to the
         * related event, actor and component to avoid ownership cycles.
         */
        class WPCore_API ComponentEventListener : public IComponentEventListener
        {
        public:
            /// Key name used when serializing/deserializing the actor reference.
            static const String actorStr;

            /// Key name used when serializing/deserializing the target function name.
            static const String functionStr;

            /// Key name used when serializing/deserializing the component reference.
            static const String componentStr;

            /**
             * @brief Create a new ComponentEventListener.
             *
             * Initializes the listener with empty actor/component/function
             * references.
             */
            ComponentEventListener();

            /**
             * @brief Virtual destructor.
             */
            ~ComponentEventListener() override;

            /**
             * @brief Handle an incoming event dispatched to this listener.
             *
             * This implementation will be invoked by the event system when the
             * bound component event occurs. Parameters are forwarded from the
             * dispatcher and can be used to call the configured function on the
             * target actor/component.
             *
             * @param eventType Type identifier of the incoming event.
             * @param eventValue Numeric/hash value associated with the event.
             * @param arguments Arguments forwarded with the event.
             * @param sender The object that sent the event.
             * @param object The object that the event is associated with.
             * @param event The original event object (may contain metadata).
             * @return A Parameter representing the handler's return value (if any).
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            /**
             * @copydoc IComponentEventListener::getActor
             *
             * @return Weak-reference promoted to a SmartPtr to the bound actor,
             *         or null if no actor is set.
             */
            SmartPtr<IGameActor> getActor() const override;

            /**
             * @copydoc IComponentEventListener::setActor
             *
             * @param actor SmartPtr to the actor to bind to this listener.
             */
            void setActor( SmartPtr<IGameActor> actor ) override;

            /**
             * @copydoc IComponentEventListener::getComponent
             *
             * @return The bound component (promoted from a weak pointer), or
             *         null if none is set.
             */
            SmartPtr<IComponent> getComponent() const override;

            /**
             * @copydoc IComponentEventListener::setComponent
             *
             * @param component SmartPtr to the component to bind.
             */
            void setComponent( SmartPtr<IComponent> component ) override;

            /**
             * @copydoc IComponentEventListener::getFunction
             *
             * @return The name of the function to call on the target when the
             *         event triggers.
             */
            String getFunction() const override;

            /**
             * @copydoc IComponentEventListener::setFunction
             *
             * @param function Name of the function to invoke when handling the event.
             */
            void setFunction( const String &function ) override;

            /**
             * @copydoc IComponentEventListener::toData
             *
             * Serialize this listener into a data object that can be persisted
             * or transmitted. The returned object contains keys for actor,
             * component and function (using the static key names defined above).
             *
             * @return A data object representing this listener's state.
             */
            SmartPtr<ISharedObject> toData() const override;

            /**
             * @copydoc IComponentEventListener::fromData
             *
             * Restore the listener state from a serialized data object. The
             * implementation expects keys matching the static key names
             * (actorStr, componentStr, functionStr).
             *
             * @param data Data object to read state from.
             */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IComponent::getProperties
             *
             * Exposes component properties associated with this listener. These
             * can be used by editors or serializers to inspect editable state.
             *
             * @return Properties object representing editable fields.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IComponent::setProperties
             *
             * Apply properties to this listener (typically used by editors or
             * deserializers).
             *
             * @param properties Properties to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the bound component event instance.
             *
             * @return SmartPtr to the associated IComponentEvent, or null if none.
             */
            SmartPtr<IComponentEvent> getEvent() const;

            /**
             * @brief Bind a component event to this listener.
             *
             * @param event SmartPtr to the event to bind.
             */
            void setEvent( SmartPtr<IComponentEvent> event );

        protected:
            /// Weak reference to the component event this listener is bound to.
            WeakPtr<IComponentEvent> m_event;

            /// Weak reference to the game actor that will receive the function call.
            WeakPtr<IGameActor> m_actor;

            /// Weak reference to the component (owner/target) for the callback.
            WeakPtr<IComponent> m_component;

            /// Name of the function to invoke when the event is handled.
            FixedString<256> m_function;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ComponentEventListener_h__
