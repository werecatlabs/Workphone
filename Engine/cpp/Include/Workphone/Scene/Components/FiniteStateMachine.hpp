#ifndef __WP__Component_FiniteStateMachine_h__
#define __WP__Component_FiniteStateMachine_h__

#include <Workphone/Scene/Components/Component.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Component that manages a finite state machine (FSM) for a scene object.
         * The FiniteStateMachine component encapsulates an FSM instance and provides
         * integration with the scene/component system. It allows loading FSM configuration
         * from data, exposing FSM state as properties, and handling FSM events.
         * Responsibilities: * - Hold a reference to an FSM instance (`IFSM`).
         * - Load and reload FSM configuration from serialized data.
         * Expose current, previous and new state information as properties.
         * Handle FSM events and route them to component logic or other systems.
         * @note This component does not implement the FSM logic itself; it relies on
         * an `IFSM` implementation to define states, transitions and events.
         */
        class WPCore_API FiniteStateMachine : public Component
        {
        public:
            /** @brief Property key constants for FSM state properties. */
            static const String newStateStr;
            static const String previousStateStr;
            static const String currentStateStr;

            FiniteStateMachine();
            ~FiniteStateMachine() override;

            /** @copydoc Component::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IFSM> m_fsm;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // FiniteStateMachine_h__
