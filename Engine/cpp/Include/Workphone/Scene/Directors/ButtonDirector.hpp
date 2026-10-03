#ifndef ButtonDirector_h__
#define ButtonDirector_h__

#include <Workphone/Scene/Directors/UiElementDirector.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class ButtonDirector
         * @brief Director responsible for configuring and styling button UI elements.
         *
         * ButtonDirector provides properties and helpers to control button appearance
         * such as text size and per-state colours (normal, highlighted, pressed, disabled).
         * It serializes/deserializes these properties through the Properties API and
         * is used by the UI system when instancing button widgets.
         */
        class WPCore_API ButtonDirector : public UiElementDirector
        {
        public:
            /**
             * @brief Construct a ButtonDirector with default style values.
             */
            ButtonDirector();

            /**
             * @brief Destructor.
             */
            ~ButtonDirector() override;

            /**
             * @brief Retrieve serializable properties for this director.
             * @return SmartPtr<Properties> containing director configuration (text size, colours, ...).
             * @copydoc UiElementDirector::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties to configure this director.
             * @param properties Properties object containing configuration values.
             * @copydoc UiElementDirector::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the configured text size for button labels.
             * @return Text size in pixels (or logical units depending on UI system).
             */
            u32 getTextSize() const;

            /**
             * @brief Set the text size used for button labels.
             * @param textSize Desired text size in pixels (or logical units).
             */
            void setTextSize( u32 textSize );

            /**
             * @brief Get the colour used for the button in its normal (idle) state.
             * @return ColourF representing the normal text colour.
             */
            ColourF getNormalColour() const;

            /**
             * @brief Set the normal (idle) text colour for the button.
             * @param normalColour Colour to use for normal state.
             */
            void setNormalColour( const ColourF &normalColour );

            /**
             * @brief Get the colour used when the button is highlighted (hover/focus).
             * @return ColourF representing the highlighted colour.
             */
            ColourF getHighlightedColour() const;

            /**
             * @brief Set the highlighted text colour for the button.
             * @param highlightedColour Colour to use when highlighted.
             */
            void setHighlightedColour( const ColourF &highlightedColour );

            /**
             * @brief Get the colour used when the button is pressed (active click state).
             * @return ColourF representing the pressed colour.
             */
            ColourF getPressedColour() const;

            /**
             * @brief Set the pressed text colour for the button.
             * @param pressedColour Colour to use when pressed.
             */
            void setPressedColour( const ColourF &pressedColour );

            /**
             * @brief Get the colour used when the button is disabled.
             * @return ColourF representing the disabled colour.
             */
            ColourF getDisabledColour() const;

            /**
             * @brief Set the disabled text colour for the button.
             * @param disabledColour Colour to use when the button is disabled.
             */
            void setDisabledColour( const ColourF &disabledColour );

            WP_CLASS_REGISTER_DECL;

            /** Property key for text size in properties objects. */
            static const String textSizeStr;

            /** Property key for the normal state colour in properties objects. */
            static const String normalColourStr;

            /** Property key for the highlighted state colour in properties objects. */
            static const String highlightedColourStr;

            /** Property key for the pressed state colour in properties objects. */
            static const String pressedColourStr;

            /** Property key for the disabled state colour in properties objects. */
            static const String disabledColourStr;

        protected:
            /**
             * @brief Text size used for button labels. Default: 12.
             */
            u32 m_textSize = 12;

            /**
             * @brief Colour used for the button's normal (idle) text. Default: White.
             */
            ColourF m_normalColour = ColourF::White;

            /**
             * @brief Colour used when the button is highlighted (hover/focus). Default: White.
             */
            ColourF m_highlightedColour = ColourF::White;

            /**
             * @brief Colour used when the button is pressed (active click). Default: White.
             */
            ColourF m_pressedColour = ColourF::White;

            /**
             * @brief Colour used when the button is disabled. Default: White.
             */
            ColourF m_disabledColour = ColourF::White;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ButtonDirector_h__
