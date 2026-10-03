#ifndef _ClawUIToggleButton_H
#define _ClawUIToggleButton_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include "ClawUIButton.hpp"
#include "ClawUIToggleGroup.hpp"
#include <WPGraphics/UI/ClawUIElement.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>

struct wp_context;

namespace workphone
{
    namespace ui
    {
        /**
         * @brief Uses the shared Prototype<IUIToggle> instantiation exported by Workphone.dll.
         *
         * This avoids duplicate definition errors when linking against Workphone.lib.
         */
#if defined( WP_PLATFORM_WIN32 ) && !defined( _WP_STATIC_LIB_ ) && !defined( WPCore_EXPORTS )
        extern template class core::Prototype<IUIToggle>;
#endif

        /**
         * @class ClawUIToggleButton
         * @brief UI toggle button implementation backed by the IUIToggle interface.
         */
        class ClawUIToggleButton : public ClawUIElement<IUIToggle>
        {
        public:
            /**
             * @brief Constructs a toggle button with default toggle state and label settings.
             */
            ClawUIToggleButton();

            /**
             * @brief Destroys the toggle button instance.
             */
            ~ClawUIToggleButton() override;

            /**
             * @brief Handles input events and updates the toggle state when activated.
             */
            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            /**
             * @brief Sets whether the button is currently toggled on.
             */
            void setToggled( bool toggled ) override;

            /**
             * @brief Gets whether the button is currently toggled on.
             */
            bool isToggled() const override;

            /**
             * @brief Gets the toggle behavior used by this button.
             */
            ToggleType getToggleType() const override;

            /**
             * @brief Sets the toggle behavior used by this button.
             */
            void setToggleType( ToggleType toggleType ) override;

            /**
             * @brief Gets the current toggle state.
             */
            ToggleState getToggleState() const override;

            /**
             * @brief Sets the current toggle state.
             */
            void setToggleState( ToggleState toggleState ) override;

            /**
             * @brief Returns whether the label text should be shown.
             */
            bool getShowLabel() const override;

            /**
             * @brief Controls whether the label text is shown.
             */
            void setShowLabel( bool showLabel ) override;

            /**
             * @brief Returns the text displayed on the toggle button.
             */
            String getLabel() const override;

            /**
             * @brief Sets the text displayed on the toggle button.
             */
            void setLabel( const String &label ) override;

            /**
             * @brief Sets the label text size.
             */
            void setTextSize( f32 textSize ) override;

            /**
             * @brief Gets the label text size.
             */
            f32 getTextSize() const override;

            /**
             * @brief Draws the toggle button using the provided graphics context.
             */
            void draw( struct wp_context *ctx ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            SmartPtr<ClawUIToggleGroup> m_toggleGroup; ///< Toggle group that coordinates mutual exclusion.
            ToggleType m_toggleType = ToggleType::ToggleButton; ///< Toggle behavior for this control.
            ToggleState m_toggleState = ToggleState::Off; ///< Current toggle state.
            f32 m_textSize = 1.0f; ///< Label text size multiplier.
            bool m_showLabel = true; ///< True when the label should be rendered.
            String m_label; ///< Text displayed on the button.
            String m_toggleMaterial; ///< Material used to render the toggle appearance.
            bool m_isToggled = false; ///< True when the button is in the toggled-on state.
        };
    }  // end namespace ui
}  // namespace workphone

#endif
