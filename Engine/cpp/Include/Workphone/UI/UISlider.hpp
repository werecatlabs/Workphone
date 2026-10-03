#ifndef UISlider_h__
#define UISlider_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUISlider.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @class UISlider
         * @brief UI element that provides a slider for selecting a value within a range.
         *
         * This class implements the IUISlider interface, allowing the UI to render
         * and interact with a slider that can be moved horizontally or vertically.
         */
        class WPCore_API UISlider : public UIElement<IUISlider>
        {
        public:
            /**
             * @brief Constructs a new UISlider instance.
             */
            UISlider();

            /**
             * @brief Destroys the UISlider instance.
             */
            ~UISlider() override;

            /**
             * @brief Retrieves the current value of the slider.
             * @return The current slider value.
             */
            f32 getValue() const override;

            /**
             * @brief Sets the value of the slider.
             * @param value The new value to set.
             */
            void setValue( f32 value ) override;

            /**
             * @brief Retrieves the minimum allowed value for the slider.
             * @return The minimum value.
             */
            f32 getMinValue() const override;

            /**
             * @brief Sets the minimum allowed value for the slider.
             * @param minValue The minimum value.
             */
            void setMinValue( f32 minValue ) override;

            /**
             * @brief Retrieves the maximum allowed value for the slider.
             * @return The maximum value.
             */
            f32 getMaxValue() const override;

            /**
             * @brief Sets the maximum allowed value for the slider.
             * @param maxValue The maximum value.
             */
            void setMaxValue( f32 maxValue ) override;

            /**
             * @brief Retrieves the orientation of the slider.
             * @return The current direction (e.g., Horizontal or Vertical).
             */
            Direction getDirection() const override;

            /**
             * @brief Sets the orientation of the slider.
             * @param direction The direction to set.
             */
            void setDirection( Direction direction ) override;

            /**
             * @brief Checks if the slider is currently being dragged by the user.
             * @return True if the slider is being dragged, otherwise false.
             */
            bool isDragging() const override;

            /**
             * @brief Sets the dragging state of the slider.
             * @param dragging True to set the slider as dragging, false otherwise.
             */
            void setDragging( bool dragging ) override;

            /**
             * @brief Marks the slider as invalid and triggers a redraw or update.
             */
            void invalidate() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UISlider_h__
