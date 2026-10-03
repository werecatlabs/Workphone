#ifndef IUIProgressBar_h__
#define IUIProgressBar_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class WPCore_API IUIProgressBar : public IUIElement
        {
        public:
            IUIProgressBar() : IUIElement( IUIProgressBar::typeInfo() )
            {
            }

            IUIProgressBar( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            ~IUIProgressBar() override;

            virtual f32 getValue() const = 0;
            virtual void setValue( f32 value ) = 0;

            virtual f32 getMinValue() const = 0;
            virtual void setMinValue( f32 value ) = 0;

            virtual f32 getMaxValue() const = 0;
            virtual void setMaxValue( f32 value ) = 0;

            virtual bool getShowText() const = 0;
            virtual void setShowText( bool showText ) = 0;

            virtual ColourF getFillColour() const = 0;
            virtual void setFillColour( const ColourF &colour ) = 0;

            virtual ColourF getBackgroundColour() const = 0;
            virtual void setBackgroundColour( const ColourF &colour ) = 0;

            virtual ColourF getBorderColour() const = 0;
            virtual void setBorderColour( const ColourF &colour ) = 0;

            virtual ColourF getTextColour() const = 0;
            virtual void setTextColour( const ColourF &colour ) = 0;

            virtual f32 getBorderWidth() const = 0;
            virtual void setBorderWidth( f32 width ) = 0;

            virtual f32 getRounding() const = 0;
            virtual void setRounding( f32 rounding ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIProgressBar_h__
