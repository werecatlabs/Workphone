/**
 * @file UISliderOgreNext.hpp
 * @brief Defines the OgreNext implementation of a UI slider component.
 * @author Workphone Engine Team
 */

#ifndef UISlider_h__
#define UISlider_h__

#include <WPGraphicsOgreNext/UI/Colibri/UIElementColibri.hpp>
#include <Workphone/Interface/UI/IUISlider.hpp>
#include "Workphone/UI/UIElement.hpp"

namespace workphone
{
    namespace ui
    {
        /**
         * @class UISliderOgreNext
         * @brief OgreNext-based implementation of a UI slider control.
         *
         * This class provides a slider widget using the Colibri UI framework integrated with OgreNext.
         * It supports horizontal and vertical orientations, customizable value ranges, and drag interaction.
         * The slider internally uses an integer denominator for precision when converting between
         * floating-point values and the underlying Colibri slider's integer-based implementation.
         */
        class UISliderColibri : public UIElementColibri<UIElement<IUISlider>>
        {
        public:
            /**
             * @brief Default constructor.
             */
            UISliderColibri();

            /**
             * @brief Destructor.
             */
            ~UISliderColibri() override;

            /**
             * @brief Loads the slider component with the provided data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the slider component and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the current value of the slider.
             * @return The current slider value as a floating-point number between min and max values.
             */
            f32 getValue() const override;

            /**
             * @brief Sets the current value of the slider.
             * @param value The new slider value. Will be clamped to the [minValue, maxValue] range.
             */
            void setValue( f32 value ) override;

            /**
             * @brief Gets the minimum value of the slider range.
             * @return The minimum value the slider can represent.
             */
            f32 getMinValue() const override;

            /**
             * @brief Sets the minimum value of the slider range.
             * @param minValue The new minimum value for the slider.
             */
            void setMinValue( f32 minValue ) override;

            /**
             * @brief Gets the maximum value of the slider range.
             * @return The maximum value the slider can represent.
             */
            f32 getMaxValue() const override;

            /**
             * @brief Sets the maximum value of the slider range.
             * @param maxValue The new maximum value for the slider.
             */
            void setMaxValue( f32 maxValue ) override;

            /**
             * @brief Gets the orientation direction of the slider.
             * @return The direction (horizontal or vertical) of the slider.
             */
            Direction getDirection() const override;

            /**
             * @brief Sets the orientation direction of the slider.
             * @param direction The new direction (horizontal or vertical) for the slider.
             */
            void setDirection( Direction direction ) override;

            /**
             * @brief Checks if the slider is currently being dragged by the user.
             * @return True if the slider is being dragged, false otherwise.
             */
            bool isDragging() const override;

            /**
             * @brief Sets the dragging state of the slider.
             * @param dragging True to set the slider as being dragged, false otherwise.
             */
            void setDragging( bool dragging ) override;

            /**
             * @brief Sets the size of the slider widget.
             * @param size A 2D vector representing the width and height of the slider.
             */
            void setSize( const Vector2<real_Num> &size ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Pointer to the underlying Colibri slider widget. */
            Colibri::Slider *m_slider = nullptr;

            /** @brief The minimum value the slider can represent (default: 0.0). */
            f32 m_minValue = 0.0f;

            /** @brief The maximum value the slider can represent (default: 1.0). */
            f32 m_maxValue = 1.0f;

            /** @brief Denominator used for converting floating-point values to integer precision (default: 1000). */
            s32 m_denominator = 1000;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UISlider_h__
