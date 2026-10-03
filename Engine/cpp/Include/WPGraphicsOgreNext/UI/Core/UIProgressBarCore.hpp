#ifndef UIProgressBarCore_h__
#define UIProgressBarCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/UI/UIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Simple Core-backed progress bar useful for health, stamina, timers, and loading UI. */
        class UIProgressBarCore : public UIElementCore<UIElement<IUIElement>>
        {
        public:
            UIProgressBarCore();
            ~UIProgressBarCore() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            f32 getValue() const;
            void setValue( f32 value );

            f32 getMinValue() const;
            void setMinValue( f32 value );

            f32 getMaxValue() const;
            void setMaxValue( f32 value );

            bool getShowText() const;
            void setShowText( bool showText );

            ColourF getFillColour() const;
            void setFillColour( const ColourF &colour );

            ColourF getBackgroundColour() const;
            void setBackgroundColour( const ColourF &colour );

            ColourF getBorderColour() const;
            void setBorderColour( const ColourF &colour );

            ColourF getTextColour() const;
            void setTextColour( const ColourF &colour );

            f32 getBorderWidth() const;
            void setBorderWidth( f32 width );

            f32 getRounding() const;
            void setRounding( f32 rounding );

            WP_CLASS_REGISTER_DECL;

        protected:
            f32 getNormalizedValue() const;
            String getDisplayText() const;

            f32 m_value = 1.0f;
            f32 m_minValue = 0.0f;
            f32 m_maxValue = 1.0f;
            bool m_showText = false;

            ColourF m_fillColour = ColourF( 0.2f, 0.75f, 0.35f, 1.0f );
            ColourF m_backgroundColour = ColourF( 0.05f, 0.06f, 0.07f, 0.85f );
            ColourF m_borderColour = ColourF( 0.0f, 0.0f, 0.0f, 0.75f );
            ColourF m_textColour = ColourF( 1.0f, 1.0f, 1.0f, 1.0f );

            f32 m_borderWidth = 1.0f;
            f32 m_rounding = 4.0f;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIProgressBarCore_h__
