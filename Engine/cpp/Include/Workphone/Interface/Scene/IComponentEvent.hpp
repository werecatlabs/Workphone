#ifndef _WP_IComponentEvent_h__
#define _WP_IComponentEvent_h__

#include <Workphone/Interface/System/IEvent.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Interface for a component event within the scene system.
         *
         * This interface defines the contract for events that are associated with components in the
         * scene. It allows for the management of event listeners, event labeling, and event hashing.
         *
         * @see IEvent
         */
        class WPCore_API IComponentEvent : public IEvent
        {
        public:
            /**
             * @brief Types of component events.
             */
            enum class EventType
            {
                Loading, /**< Event related to loading operations. */
                Object,  /**< Event related to object operations. */
                UI,      /**< Event related to user interface operations. */

                Count /**< Number of event types. */
            };

            /**
             * @brief Virtual destructor.
             */
            ~IComponentEvent() override;

            /**
             * @brief Adds a listener to this component event.
             *
             * @param listener The listener to add.
             */
            virtual void addListener( SmartPtr<IComponentEventListener> listener ) = 0;

            /**
             * @brief Removes a listener from this component event.
             *
             * @param listener The listener to remove.
             */
            virtual void removeListener( SmartPtr<IComponentEventListener> listener ) = 0;

            /**
             * @brief Removes all listeners from this component event.
             */
            virtual void removeListeners() = 0;

            /**
             * @brief Gets the listeners currently registered to this event.
             *
             * @return An array of smart pointers to the registered listeners.
             */
            virtual Array<SmartPtr<IComponentEventListener>> getListeners() const = 0;

            /**
             * @brief Sets the listeners for this event, replacing any existing listeners.
             *
             * @param listeners The array of listeners to set.
             */
            virtual void setListeners( const Array<SmartPtr<IComponentEventListener>> &listeners ) = 0;

            /**
             * @brief Gets the label associated with this event.
             *
             * @return The event label as a string.
             */
            virtual String getLabel() const = 0;

            /**
             * @brief Sets the label for this event.
             *
             * @param label The label to set.
             */
            virtual void setLabel( const String &label ) = 0;

            /**
             * @brief Gets the hash value associated with this event.
             *
             * @return The event hash.
             */
            virtual hash_type getEventHash() const = 0;

            /**
             * @brief Sets the hash value for this event.
             *
             * @param eventHash The hash value to set.
             */
            virtual void setEventHash( hash_type eventHash ) = 0;

            // 'c' style linked list.
            AtomicRawPtr<IComponentEvent> m_next;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // IEvent_h__
