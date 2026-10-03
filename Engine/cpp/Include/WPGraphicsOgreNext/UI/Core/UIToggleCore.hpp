#ifndef __UIToggleCore_h__
#define __UIToggleCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/UI/UIToggle.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @brief UI toggle component implementation using OgreNext rendering backend.
         *
         * This class provides a toggle/checkbox UI element with customizable appearance
         * and behavior. It supports both checkbox and radio button toggle types, with
         * optional text labels and configurable visual states.
         */
        class UIToggleCore : public UIElementCore<UIToggle>
        {
        public:
            /**
             * @brief Constructs a new UIToggleOgreNext instance.
             */
            UIToggleCore();

            /**
             * @brief Destroys the UIToggleOgreNext instance.
             */
            ~UIToggleCore() override;

            /**
             * @brief Loads the toggle component with the specified data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the toggle component and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Per-frame update — submits the toggle draw command and dispatches change events.
             *
             * Calls the appropriate WorkphoneCore immediate-mode widget (checkbox or radio button)
             * based on the current ToggleType, then fires IEvent::CLICK_HASH to all registered
             * object listeners if the toggle state changed this frame.
             */
            void update() override;

            void setToggled( bool toggled ) override;
            bool isToggled() const override;

            ToggleType getToggleType() const override;
            void setToggleType( ToggleType toggleType ) override;

            ToggleState getToggleState() const override;
            void setToggleState( ToggleState toggleState ) override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            void setTextSize( f32 textSize ) override;
            f32 getTextSize() const override;

            bool getShowLabel() const override;
            void setShowLabel( bool showLabel ) override;

            ColourF getNormalColour() const;
            void setNormalColour( const ColourF &colour );

            ColourF getHoverColour() const;
            void setHoverColour( const ColourF &colour );

            ColourF getActiveColour() const;
            void setActiveColour( const ColourF &colour );

            ColourF getCursorNormalColour() const;
            void setCursorNormalColour( const ColourF &colour );

            ColourF getCursorHoverColour() const;
            void setCursorHoverColour( const ColourF &colour );

            ColourF getTextNormalColour() const;
            void setTextNormalColour( const ColourF &colour );

            ColourF getTextHoverColour() const;
            void setTextHoverColour( const ColourF &colour );

            ColourF getTextActiveColour() const;
            void setTextActiveColour( const ColourF &colour );

            ColourF getTextBackgroundColour() const;
            void setTextBackgroundColour( const ColourF &colour );

            ColourF getBorderColour() const;
            void setBorderColour( const ColourF &colour );

            f32 getBorderWidth() const;
            void setBorderWidth( f32 width );

            Vector2F getPadding() const;
            void setPadding( const Vector2F &padding );

            Vector2F getTouchPadding() const;
            void setTouchPadding( const Vector2F &touchPadding );

            f32 getSpacing() const;
            void setSpacing( f32 spacing );

            ColourF getSwitchOffColour() const;
            void setSwitchOffColour( const ColourF &colour );

            ColourF getSwitchOnColour() const;
            void setSwitchOnColour( const ColourF &colour );

            ColourF getSwitchHoverColour() const;
            void setSwitchHoverColour( const ColourF &colour );

            ColourF getSwitchActiveColour() const;
            void setSwitchActiveColour( const ColourF &colour );

            ColourF getSwitchTextColour() const;
            void setSwitchTextColour( const ColourF &colour );

            ColourF getSwitchBorderColour() const;
            void setSwitchBorderColour( const ColourF &colour );

            f32 getSwitchBorderWidth() const;
            void setSwitchBorderWidth( f32 width );

            f32 getSwitchRounding() const;
            void setSwitchRounding( f32 rounding );

            Vector2F getSwitchPadding() const;
            void setSwitchPadding( const Vector2F &padding );

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void applyStyle( struct wp_context *ctx ) const;

            /// Text label element displayed alongside the toggle.
            SmartPtr<IUIText> m_label;

            /// Background image element for the toggle control.
            SmartPtr<IUIImage> m_bgImage;

            /// Image element representing the toggle indicator.
            SmartPtr<IUIImage> m_toggleImage;

            /// Pointer to the underlying Core checkbox widget.
            struct wp_checkbox_label *m_checkbox = nullptr;

            /// The label text content.
            String m_text = "Text";

            f32 m_textSize = 1.0f;

            /// Current checked state of the toggle.
            bool m_checked = false;

            /// Flag indicating whether the label should be displayed.
            bool m_showLabel = true;

            /// The type of toggle control (checkbox or radio button).
            ToggleType m_toggleType = ToggleType::CheckBox;

            /// The current visual state of the toggle.
            ToggleState m_toggleState = ToggleState::Off;

            ColourF m_normalColour = ColourF( 0.18f, 0.18f, 0.18f, 1.0f );
            ColourF m_hoverColour = ColourF( 0.24f, 0.24f, 0.24f, 1.0f );
            ColourF m_activeColour = ColourF( 0.24f, 0.24f, 0.24f, 1.0f );
            ColourF m_cursorNormalColour = ColourF::White;
            ColourF m_cursorHoverColour = ColourF::White;
            ColourF m_textNormalColour = ColourF::White;
            ColourF m_textHoverColour = ColourF::White;
            ColourF m_textActiveColour = ColourF::White;
            ColourF m_textBackgroundColour = ColourF( 0.0f, 0.0f, 0.0f, 0.0f );
            ColourF m_borderColour = ColourF( 0.0f, 0.0f, 0.0f, 0.0f );
            f32 m_borderWidth = 0.0f;
            Vector2F m_padding = Vector2F( 2.0f, 2.0f );
            Vector2F m_touchPadding = Vector2F::zero();
            f32 m_spacing = 4.0f;

            ColourF m_switchOffColour = ColourF( 0.20f, 0.20f, 0.20f, 1.0f );
            ColourF m_switchOnColour = ColourF( 0.15f, 0.45f, 0.22f, 1.0f );
            ColourF m_switchHoverColour = ColourF( 0.28f, 0.28f, 0.28f, 1.0f );
            ColourF m_switchActiveColour = ColourF( 0.12f, 0.38f, 0.18f, 1.0f );
            ColourF m_switchTextColour = ColourF::White;
            ColourF m_switchBorderColour = ColourF( 0.0f, 0.0f, 0.0f, 0.0f );
            f32 m_switchBorderWidth = 0.0f;
            f32 m_switchRounding = 12.0f;
            Vector2F m_switchPadding = Vector2F( 6.0f, 2.0f );
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UIToggle_h__
