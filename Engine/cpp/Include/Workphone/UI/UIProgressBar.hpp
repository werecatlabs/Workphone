#ifndef UIProgressBar_h__
#define UIProgressBar_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIProgressBar.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @class UIProgressBar
         * @brief UI element used to visually represent the progress of a task.
         *
         * This class implements the IUIProgressBar interface and provides controls for
         * the progress value, range, colors, and visual styling such as borders and rounding.
         */
        class WPCore_API UIProgressBar : public UIElement<IUIProgressBar>
        {
        public:
            /**
             * @brief Constructs a new UIProgressBar instance.
             */
            UIProgressBar();

            /**
             * @brief Destroys the UIProgressBar instance.
             */
            ~UIProgressBar() override;

            /**
             * @brief Retrieves the current progress value.
             * @return The current value.
             */
            f32 getValue() const override;

            /**
             * @brief Sets the progress value.
             * @param value The new value to set.
             */
            void setValue( f32 value ) override;

            /**
             * @brief Retrieves the minimum value of the progress bar.
             * @return The minimum value.
             */
            f32 getMinValue() const override;

            /**
             * @brief Sets the minimum value of the progress bar.
             * @param value The new minimum value.
             */
            void setMinValue( f32 value ) override;

            /**
             * @brief Retrieves the maximum value of the progress bar.
             * @return The maximum value.
             */
            f32 getMaxValue() const override;

            /**
             * @brief Sets the maximum value of the progress bar.
             * @param value The new maximum value.
             */
            void setMaxValue( f32 value ) override;

            /**
             * @brief Checks if the progress text is visible.
             * @return True if text is shown, otherwise false.
             */
            bool getShowText() const override;

            /**
             * @brief Sets whether the progress text should be displayed.
             * @param showText True to show text, false to hide it.
             */
            void setShowText( bool showText ) override;

            /**
             * @brief Retrieves the fill color of the progress bar.
             * @return The fill color.
             */
            ColourF getFillColour() const override;

            /**
             * @brief Sets the fill color of the progress bar.
             * @param colour The new fill color.
             */
            void setFillColour( const ColourF &colour ) override;

            /**
             * @brief Retrieves the background color of the progress bar.
             * @return The background color.
             */
            ColourF getBackgroundColour() const override;

            /**
             * @brief Sets the background color of the progress bar.
             * @param colour The new background color.
             */
            void setBackgroundColour( const ColourF &colour ) override;

            /**
             * @brief Retrieves the border color of the progress bar.
             * @return The border color.
             */
            ColourF getBorderColour() const override;

            /**
             * @brief Sets the border color of the progress bar.
             * @param colour The new border color.
             */
            void setBorderColour( const ColourF &colour ) override;

            /**
             * @brief Retrieves the color of the progress text.
             * @return The text color.
             */
            ColourF getTextColour() const override;

            /**
             * @brief Sets the color of the progress text.
             * @param colour The new text color.
             */
            void setTextColour( const ColourF &colour ) override;

            /**
             * @brief Retrieves the width of the progress bar border.
             * @return The border width.
             */
            f32 getBorderWidth() const override;

            /**
             * @brief Sets the width of the progress bar border.
             * @param width The new border width.
             */
            void setBorderWidth( f32 width ) override;

            /**
             * @brief Retrieves the rounding value for the progress bar corners.
             * @return The rounding value.
             */
            f32 getRounding() const override;

            /**
             * @brief Sets the rounding value for the progress bar corners.
             * @param rounding The new rounding value.
             */
            void setRounding( f32 rounding ) override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIProgressBar_h__
