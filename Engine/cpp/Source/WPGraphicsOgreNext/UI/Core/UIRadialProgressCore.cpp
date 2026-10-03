#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIRadialProgressCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>
#include <workphone_command_buffer.h>
#include <workphone_window.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, UIRadialProgressCore,
                               UIElementCore<UIElement<IUIElement>> );

    namespace
    {
        const String valueStr = "value";
        const String clockwiseStr = "clockwise";
        const String startAngleStr = "startAngle";
        const String fillColourStr = "fillColour";
        const String backgroundColourStr = "backgroundColour";
        const String borderColourStr = "borderColour";
        const String borderWidthStr = "borderWidth";
        constexpr f32 twoPi = 6.28318530718f;
    }  // namespace

    UIRadialProgressCore::UIRadialProgressCore()
    {
        createStateContext();
    }

    UIRadialProgressCore::~UIRadialProgressCore()
    {
        unload( nullptr );
        destroyStateContext();
    }

    void UIRadialProgressCore::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        setLoadingState( LoadingState::Loaded );
    }

    void UIRadialProgressCore::unload( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            setLoadingState( LoadingState::Unloading );
            UIElementCore<UIElement<IUIElement>>::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void UIRadialProgressCore::update()
    {
        if( !isLoaded() || !isVisible() || !isEnabled() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return;
        }

        auto ui = workphone::static_pointer_cast<UIManagerCore>( applicationManager->getRenderUI() );
        if( !ui )
        {
            return;
        }

        auto *ctx = ui->getContext();
        if( !ctx )
        {
            return;
        }

        auto *canvas = wp_window_get_canvas( ctx );
        if( !canvas )
        {
            return;
        }

        struct wp_rect bounds;
        UIUtilCore::calculateBounds( getPosition(), getSize(), &bounds );

        wp_fill_circle( canvas, bounds, UIUtilCore::toWpColor( m_backgroundColour ) );

        const auto value = MathF::clamp01( m_value );
        if( value > 0.0f )
        {
            const auto cx = bounds.x + bounds.w * 0.5f;
            const auto cy = bounds.y + bounds.h * 0.5f;
            const auto radius = MathF::min( bounds.w, bounds.h ) * 0.5f;
            const auto sweep = value * twoPi;
            const auto endAngle = m_clockwise ? m_startAngle + sweep : m_startAngle - sweep;
            wp_fill_arc( canvas, cx, cy, radius, m_startAngle, endAngle,
                         UIUtilCore::toWpColor( m_fillColour ) );
        }

        if( m_borderWidth > 0.0f )
        {
            wp_stroke_circle( canvas, bounds, m_borderWidth, UIUtilCore::toWpColor( m_borderColour ) );
        }

        UIElementCore<UIElement<IUIElement>>::update();
    }

    SmartPtr<Properties> UIRadialProgressCore::getProperties() const
    {
        auto properties = UIElementCore<UIElement<IUIElement>>::getProperties();
        properties->setProperty( valueStr, m_value );
        properties->setProperty( clockwiseStr, m_clockwise );
        properties->setProperty( startAngleStr, m_startAngle );
        properties->setProperty( fillColourStr, m_fillColour );
        properties->setProperty( backgroundColourStr, m_backgroundColour );
        properties->setProperty( borderColourStr, m_borderColour );
        properties->setProperty( borderWidthStr, m_borderWidth );
        return properties;
    }

    void UIRadialProgressCore::setProperties( SmartPtr<Properties> properties )
    {
        UIElementCore<UIElement<IUIElement>>::setProperties( properties );
        properties->getPropertyValue( valueStr, m_value );
        properties->getPropertyValue( clockwiseStr, m_clockwise );
        properties->getPropertyValue( startAngleStr, m_startAngle );
        properties->getPropertyValue( fillColourStr, m_fillColour );
        properties->getPropertyValue( backgroundColourStr, m_backgroundColour );
        properties->getPropertyValue( borderColourStr, m_borderColour );
        properties->getPropertyValue( borderWidthStr, m_borderWidth );
        m_value = MathF::clamp01( m_value );
        m_borderWidth = MathF::max( m_borderWidth, 0.0f );
    }

    f32 UIRadialProgressCore::getValue() const
    {
        return m_value;
    }

    void UIRadialProgressCore::setValue( f32 value )
    {
        m_value = MathF::clamp01( value );
    }

    bool UIRadialProgressCore::getClockwise() const
    {
        return m_clockwise;
    }

    void UIRadialProgressCore::setClockwise( bool clockwise )
    {
        m_clockwise = clockwise;
    }

    f32 UIRadialProgressCore::getStartAngle() const
    {
        return m_startAngle;
    }

    void UIRadialProgressCore::setStartAngle( f32 startAngleRadians )
    {
        m_startAngle = startAngleRadians;
    }

    ColourF UIRadialProgressCore::getFillColour() const
    {
        return m_fillColour;
    }

    void UIRadialProgressCore::setFillColour( const ColourF &colour )
    {
        m_fillColour = colour;
    }

    ColourF UIRadialProgressCore::getBackgroundColour() const
    {
        return m_backgroundColour;
    }

    void UIRadialProgressCore::setBackgroundColour( const ColourF &colour )
    {
        m_backgroundColour = colour;
    }

    ColourF UIRadialProgressCore::getBorderColour() const
    {
        return m_borderColour;
    }

    void UIRadialProgressCore::setBorderColour( const ColourF &colour )
    {
        m_borderColour = colour;
    }

    f32 UIRadialProgressCore::getBorderWidth() const
    {
        return m_borderWidth;
    }

    void UIRadialProgressCore::setBorderWidth( f32 width )
    {
        m_borderWidth = MathF::max( width, 0.0f );
    }
}  // namespace workphone::ui
