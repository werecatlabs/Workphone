#ifndef ComponentEvent_h__
#define ComponentEvent_h__

#include <Workphone/Interface/Scene/IComponentEvent.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class ComponentEvent
         * @brief Implementation of the `IComponentEvent` interface used by components.
         *
         * `ComponentEvent` represents a named event that a component can expose. It
         * stores a label, a hashed event identifier and a list of listeners that will
         * be notified when the event is triggered.
         */
        class WPCore_API ComponentEvent : public IComponentEvent
        {
        public:
            /**
             * @brief Construct a new ComponentEvent with default values.
             */
            ComponentEvent();

            /**
             * @brief Destroy the ComponentEvent and release any held listeners.
             */
            ~ComponentEvent() override;

            /**
             * @brief Add a listener to this event.
             * @param listener Smart pointer to the listener to register. Duplicate
             * listeners are handled by the implementation.
             */
            void addListener( SmartPtr<IComponentEventListener> listener ) override;

            /**
             * @brief Remove a previously registered listener.
             * @param listener Smart pointer to the listener to remove.
             */
            void removeListener( SmartPtr<IComponentEventListener> listener ) override;

            /**
             * @brief Remove all registered listeners from this event.
             */
            void removeListeners() override;

            /**
             * @brief Get a copy of the registered listeners.
             * @return Array of smart pointers to `IComponentEventListener`.
             */
            Array<SmartPtr<IComponentEventListener>> getListeners() const override;

            /**
             * @brief Replace the current listener list with the provided array.
             * @param listeners New array of listeners to register.
             */
            void setListeners( const Array<SmartPtr<IComponentEventListener>> &listeners ) override;

            /**
             * @brief Get the human-readable label for this event.
             * @return String Label identifying the event.
             */
            String getLabel() const override;

            /**
             * @brief Set the human-readable label for this event.
             * @param label New label to assign.
             */
            void setLabel( const String &label ) override;

            /**
             * @copydoc IObject::toData
             * @return Serialized representation of the event (label, hash, listeners may be included).
             */
            SmartPtr<ISharedObject> toData() const override;

            /**
             * @copydoc IObject::fromData
             * @param data Serialized data used to restore the event state.
             */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IComponent::getProperties
             * @return Properties associated with the event (may be null).
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IComponent::setProperties
             * @param properties Properties object to associate with the event.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the hashed event identifier used for fast lookups.
             * @return hash_type Numeric hash representing the event.
             */
            hash_type getEventHash() const override;

            /**
             * @brief Set the hashed event identifier.
             * @param eventHash Hash value to store for this event.
             */
            void setEventHash( hash_type eventHash ) override;

        protected:
            /**
             * @brief Properties associated with this event.
             */
            SmartPtr<Properties> m_properties;

            /**
             * @brief Hashed identifier for this event (used for fast comparisons/lookups).
             */
            hash_type m_eventHash = 0;

            /**
             * @brief Human-readable label for the event.
             */
            FixedString<256> m_label;

            /**
             * @brief Registered listeners that will be notified when the event is fired.
             */
            Array<SmartPtr<IComponentEventListener>> m_listeners;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ComponentEvent_h__
