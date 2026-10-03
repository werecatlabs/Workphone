#ifndef IUILabelSliderPair_h__
#define IUILabelSliderPair_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a label-slider pair. */
        class WPCore_API IUILabelSliderPair : public IUIElement
        {
        public:
            IUILabelSliderPair();

            IUILabelSliderPair( u32 poolTypeId );

            /** Destructor. */
            ~IUILabelSliderPair() override;

            String getLabel() const override = 0;

            void setLabel( const String &label ) override = 0;

            virtual f32 getValue() const = 0;

            virtual void setValue( f32 value ) = 0;

            virtual f32 getMinValue() const = 0;

            virtual void setMinValue( f32 minValue ) = 0;

            virtual f32 getMaxValue() const = 0;

            virtual void setMaxValue( f32 maxValue ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IUILabelSliderPair_h__
