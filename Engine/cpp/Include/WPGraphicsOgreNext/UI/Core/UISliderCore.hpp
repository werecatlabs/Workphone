/**
 * @file UISliderOgreNext.hpp
 * @brief Defines the OgreNext implementation of a UI slider component.
 * @author Workphone Engine Team
 */

#ifndef __UISliderCore_h__
#define __UISliderCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/Interface/UI/IUISlider.hpp>
#include "Workphone/UI/UIElement.hpp"
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @class UISliderOgreNext
         * @brief OgreNext-based implementation of a UI slider control.
         *
         * This class provides a slider widget using the Core UI framework integrated with OgreNext.
         * It supports horizontal and vertical orientations, customizable value ranges, and drag interaction.
         * The slider internally uses an integer denominator for precision when converting between
         * floating-point values and the underlying Core slider's integer-based implementation.
         */
        class UISliderCore : public UIElementCore<UIElement<IUISlider>>
        {
        public:
            /**
             * @brief Default constructor.
             */
            UISliderCore();

            /**
             * @brief Destructor.
             */
            ~UISliderCore() override;

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
             * @brief Per-frame update — submits the slider draw command and dispatches value-change events.
             */
            void update() override;

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

            s32 getDenominator() const;
            void setDenominator( s32 denominator );

            // ---------------------------------------------------------------
            // Background colours
            // ---------------------------------------------------------------

            ColourF getNormalColour() const;
            void setNormalColour( const ColourF &colour );

            ColourF getHoverColour() const;
            void setHoverColour( const ColourF &colour );

            ColourF getActiveColour() const;
            void setActiveColour( const ColourF &colour );

            // ---------------------------------------------------------------
            // Bar colours
            // ---------------------------------------------------------------

            ColourF getBarNormalColour() const;
            void setBarNormalColour( const ColourF &colour );

            ColourF getBarHoverColour() const;
            void setBarHoverColour( const ColourF &colour );

            ColourF getBarActiveColour() const;
            void setBarActiveColour( const ColourF &colour );

            ColourF getBarFilledColour() const;
            void setBarFilledColour( const ColourF &colour );

            // ---------------------------------------------------------------
            // Cursor colours
            // ---------------------------------------------------------------

            ColourF getCursorNormalColour() const;
            void setCursorNormalColour( const ColourF &colour );

            ColourF getCursorHoverColour() const;
            void setCursorHoverColour( const ColourF &colour );

            ColourF getCursorActiveColour() const;
            void setCursorActiveColour( const ColourF &colour );

            // ---------------------------------------------------------------
            // Border / shape
            // ---------------------------------------------------------------

            ColourF getBorderColour() const;
            void setBorderColour( const ColourF &colour );

            f32 getBorderWidth() const;
            void setBorderWidth( f32 width );

            f32 getRounding() const;
            void setRounding( f32 rounding );

            f32 getBarHeight() const;
            void setBarHeight( f32 height );

            // ---------------------------------------------------------------
            // Padding / spacing / cursor size
            // ---------------------------------------------------------------

            Vector2F getPadding() const;
            void setPadding( const Vector2F &padding );

            Vector2F getSpacing() const;
            void setSpacing( const Vector2F &spacing );

            Vector2F getCursorSize() const;
            void setCursorSize( const Vector2F &size );

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Push current style members into the Workphone context. */
            void applyStyle( struct wp_context *ctx ) const;

            /** @brief Pointer to the underlying Core slider widget. */
            struct wp_slider *m_slider = nullptr;

            /** @brief Cached slider value for immediate-mode state persistence. */
            f32 m_value = 0.0f;

            /** @brief Cached dragging state, updated each frame from WorkphoneCore widget state. */
            bool m_dragging = false;

            /** @brief Orientation of the slider (horizontal or vertical). */
            Direction m_direction = Direction::Horizontal;

            /** @brief The minimum value the slider can represent (default: 0.0). */
            f32 m_minValue = 0.0f;

            /** @brief The maximum value the slider can represent (default: 1.0). */
            f32 m_maxValue = 1.0f;

            /** @brief Denominator used for converting floating-point values to integer precision (default: 1000). */
            s32 m_denominator = 1000;

            // ---------------------------------------------------------------
            // Style members
            // ---------------------------------------------------------------

            // Background colours (track area)
            ColourF m_normalColour{ 0.18f, 0.18f, 0.18f, 1.0f };
            ColourF m_hoverColour{ 0.26f, 0.26f, 0.26f, 1.0f };
            ColourF m_activeColour{ 0.14f, 0.14f, 0.14f, 1.0f };

            // Bar colours
            ColourF m_barNormalColour{ 0.40f, 0.40f, 0.40f, 1.0f };
            ColourF m_barHoverColour{ 0.50f, 0.50f, 0.50f, 1.0f };
            ColourF m_barActiveColour{ 0.60f, 0.60f, 0.60f, 1.0f };
            ColourF m_barFilledColour{ 0.26f, 0.52f, 0.96f, 1.0f };

            // Cursor colours
            ColourF m_cursorNormalColour{ 0.60f, 0.60f, 0.60f, 1.0f };
            ColourF m_cursorHoverColour{ 0.78f, 0.78f, 0.78f, 1.0f };
            ColourF m_cursorActiveColour{ 1.0f, 1.0f, 1.0f, 1.0f };

            // Border / shape
            ColourF m_borderColour{ 0.50f, 0.50f, 0.50f, 1.0f };
            f32 m_borderWidth = 1.0f;
            f32 m_rounding = 4.0f;
            f32 m_barHeight = 6.0f;

            // Padding / spacing / cursor size
            Vector2F m_padding{ 4.0f, 4.0f };
            Vector2F m_spacing{ 4.0f, 4.0f };
            Vector2F m_cursorSize{ 16.0f, 16.0f };
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UISlider_h__
