#ifndef ToggleGroup_h__
#define ToggleGroup_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /** Controls a group of toggle buttons, ensuring that only one can be selected at a time.
         * This is useful for creating mutually exclusive options in a user interface.
         * The ToggleGroup component manages the selection state of its child toggle buttons.
         */
        class WPCore_API ToggleGroup : public UIComponent
        {
        public:
            ToggleGroup();
            ~ToggleGroup() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::handleEvent */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @copydoc UIComponent::handleComponentEvent */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            void createUI() override;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ToggleGroup_h__
