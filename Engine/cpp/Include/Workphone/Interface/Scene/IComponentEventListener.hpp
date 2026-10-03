#ifndef _WP_IComponentEventListener_h__
#define _WP_IComponentEventListener_h__

#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class IComponentEventListener
         * @brief Interface for listening to component-related events in the scene system.
         *
         * This interface extends IEventListener and provides methods for associating the listener with
         * specific actors and components, as well as for managing function callbacks and serializable
         * properties. Implementations of this interface can be used to observe and respond to changes or
         * actions occurring on scene components, such as loading, object manipulation, or UI events.
         *
         * @see IEventListener, IComponent, IActor, Properties
         */
        class WPCore_API IComponentEventListener : public IEventListener
        {
        public:
            /**
             * @brief Types of component event listeners.
             *
             * Used to categorize the type of event the listener is interested in.
             */
            enum class Type
            {
                Loading, /**< Listener for loading-related events. */
                Object,  /**< Listener for object-related events. */
                UI,      /**< Listener for user interface events. */

                Count /**< Number of event listener types. */
            };

            /**
             * @brief Virtual destructor.
             */
            ~IComponentEventListener() override;

            /**
             * @brief Gets the actor associated with this event listener.
             *
             * @return Smart pointer to the associated actor, or nullptr if not set.
             */
            virtual SmartPtr<IGameActor> getActor() const = 0;

            /**
             * @brief Sets the actor associated with this event listener.
             *
             * @param actor Smart pointer to the actor to associate, or nullptr to clear.
             */
            virtual void setActor( SmartPtr<IGameActor> actor ) = 0;

            /**
             * @brief Gets the component associated with this event listener.
             *
             * @return Smart pointer to the associated component, or nullptr if not set.
             */
            virtual SmartPtr<IComponent> getComponent() const = 0;

            /**
             * @brief Sets the component associated with this event listener.
             *
             * @param component Smart pointer to the component to associate, or nullptr to clear.
             */
            virtual void setComponent( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Gets the function name or identifier associated with this event listener.
             *
             * This is typically used to specify a callback or script function to invoke when the event
             * occurs.
             *
             * @return The function name or identifier as a string.
             */
            virtual String getFunction() const = 0;

            /**
             * @brief Sets the function name or identifier for this event listener.
             *
             * @param function The function name or identifier as a string.
             */
            virtual void setFunction( const String &function ) = 0;

            /**
             * @brief Gets the data associated with this event listener as a properties object.
             *
             * The returned Properties object can be used for serialization, editor integration, or
             * scripting.
             *
             * @return Smart pointer to the properties object containing the listener's data.
             */
            SmartPtr<Properties> getProperties() const override = 0;

            /**
             * @brief Sets the data for this event listener from a properties object.
             *
             * The provided Properties object should contain the relevant data to configure the listener.
             *
             * @param properties Smart pointer to the properties object to set.
             */
            void setProperties( SmartPtr<Properties> properties ) override = 0;

            // 'c' style linked list.
            AtomicRawPtr<IComponentEventListener> m_next;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // IEvent_h__
