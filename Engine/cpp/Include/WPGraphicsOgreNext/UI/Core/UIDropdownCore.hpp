#ifndef UIDropdownCore_h__
#define UIDropdownCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/UI/UIDropdown.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class UIButtonOgreNext
         * @brief OgreNext implementation of a UI button element.
         *
         * This class provides a concrete implementation of a UI button using the OgreNext
         * rendering engine and the Core UI library. It handles button rendering, state
         * management, and user interactions within the OgreNext graphics framework.
         */
        class UIDropdownCore : public UIElementCore<UIDropdown>
        {
        public:
            UIDropdownCore();
            ~UIDropdownCore() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Per-frame update � submits the button draw command and dispatches click events.
             *
             * Applies the current style to the WorkphoneCore context, emits a `wp_button_label`
             * call sized to this element's bounds, and fires `IEvent::CLICK_HASH` to all
             * registered object listeners if the button was clicked this frame.
             */
            void update() override;

            f32 getItemHeight() const;
            void setItemHeight( f32 itemHeight );

            f32 getDropdownWidth() const;
            void setDropdownWidth( f32 dropdownWidth );

            f32 getDropdownHeight() const;
            void setDropdownHeight( f32 dropdownHeight );

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void createStateContext();

            f32 m_itemHeight = 20.0f;       ///< Height of each item row in the dropdown list.
            f32 m_dropdownWidth = 200.0f;   ///< Width of the expanded dropdown popup.
            f32 m_dropdownHeight = 200.0f;  ///< Maximum height of the expanded dropdown popup.
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UIDropdownCore_h__
