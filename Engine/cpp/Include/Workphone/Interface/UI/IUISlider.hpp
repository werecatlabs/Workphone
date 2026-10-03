#ifndef IUISlider_h__
#define IUISlider_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a slider. */
        class WPCore_API IUISlider : public IUIElement
        {
        public:
            IUISlider() : IUIElement( IUISlider::typeInfo() )
            {
            }

            IUISlider( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Virtual destructor. */
            ~IUISlider() override;

            /** Gets the current value of the slider. */
            virtual f32 getValue() const = 0;

            /** Sets the current value of the slider. */
            virtual void setValue( f32 value ) = 0;

            /** Gets the minimum value of the slider. */
            virtual f32 getMinValue() const = 0;

            /** Sets the minimum value of the slider. */
            virtual void setMinValue( f32 minValue ) = 0;

            /** Gets the maximum value of the slider. */
            virtual f32 getMaxValue() const = 0;

            /** Sets the maximum value of the slider. */
            virtual void setMaxValue( f32 maxValue ) = 0;

            /** Gets the direction of the slider (horizontal or vertical). */
            virtual Direction getDirection() const = 0;

            /** Sets the direction of the slider (horizontal or vertical). */
            virtual void setDirection( Direction direction ) = 0;

            /** Returns true if the slider is currently being dragged. */
            virtual bool isDragging() const = 0;

            /** Sets the dragging state of the slider. */
            virtual void setDragging( bool dragging ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUISlider_h__
