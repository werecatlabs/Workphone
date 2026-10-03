#ifndef UIRadialProgressCore_h__
#define UIRadialProgressCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/UI/UIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Radial progress widget for cooldowns, charge meters, and circular timers. */
        class UIRadialProgressCore : public UIElementCore<UIElement<IUIElement>>
        {
        public:
            UIRadialProgressCore();
            ~UIRadialProgressCore() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            f32 getValue() const;
            void setValue( f32 value );

            bool getClockwise() const;
            void setClockwise( bool clockwise );

            f32 getStartAngle() const;
            void setStartAngle( f32 startAngleRadians );

            ColourF getFillColour() const;
            void setFillColour( const ColourF &colour );

            ColourF getBackgroundColour() const;
            void setBackgroundColour( const ColourF &colour );

            ColourF getBorderColour() const;
            void setBorderColour( const ColourF &colour );

            f32 getBorderWidth() const;
            void setBorderWidth( f32 width );

            WP_CLASS_REGISTER_DECL;

        protected:
            f32 m_value = 1.0f;
            bool m_clockwise = true;
            f32 m_startAngle = -1.57079632679f;
            ColourF m_fillColour = ColourF( 0.15f, 0.55f, 1.0f, 1.0f );
            ColourF m_backgroundColour = ColourF( 0.0f, 0.0f, 0.0f, 0.55f );
            ColourF m_borderColour = ColourF( 0.0f, 0.0f, 0.0f, 0.75f );
            f32 m_borderWidth = 1.0f;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIRadialProgressCore_h__
