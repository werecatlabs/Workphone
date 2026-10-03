#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIProgressBarCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIUtilCore.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>
#include <workphone_command_buffer.h>
#include <workphone_layout.h>
#include <workphone_text.h>
#include <workphone_window.h>
#include <cstdio>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, UIProgressBarCore,
                               UIElementCore<UIElement<IUIElement>> );

    namespace
    {
        const String valueStr = "value";
        const String minValueStr = "minValue";
        const String maxValueStr = "maxValue";
        const String showTextStr = "showText";
        const String fillColourStr = "fillColour";
        const String backgroundColourStr = "backgroundColour";
        const String borderColourStr = "borderColour";
        const String textColourStr = "textColour";
        const String borderWidthStr = "borderWidth";
        const String roundingStr = "rounding";
    }  // namespace

    UIProgressBarCore::UIProgressBarCore()
    {
        createStateContext();
    }

    UIProgressBarCore::~UIProgressBarCore()
    {
        unload( nullptr );
        destroyStateContext();
    }

    void UIProgressBarCore::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        setLoadingState( LoadingState::Loaded );
    }

    void UIProgressBarCore::unload( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            setLoadingState( LoadingState::Unloading );
            UIElementCore<UIElement<IUIElement>>::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void UIProgressBarCore::update()
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

        wp_fill_rect( canvas, bounds, m_rounding, UIUtilCore::toWpColor( m_backgroundColour ) );

        auto fillBounds = bounds;
        fillBounds.w *= getNormalizedValue();
        if( fillBounds.w > 0.0f )
        {
            wp_fill_rect( canvas, fillBounds, m_rounding, UIUtilCore::toWpColor( m_fillColour ) );
        }

        if( m_borderWidth > 0.0f )
        {
            wp_stroke_rect( canvas, bounds, m_rounding, m_borderWidth,
                            UIUtilCore::toWpColor( m_borderColour ) );
        }

        if( m_showText )
        {
            wp_layout_space_push( ctx, bounds );
            auto text = getDisplayText();
            wp_label_colored( ctx, reinterpret_cast<const wp_c8 *>( text.c_str() ),
                              WORKPHONE_TEXT_ALIGN_CENTERED | WORKPHONE_TEXT_ALIGN_MIDDLE,
                              UIUtilCore::toWpColor( m_textColour ) );
        }

        UIElementCore<UIElement<IUIElement>>::update();
    }

    SmartPtr<Properties> UIProgressBarCore::getProperties() const
    {
        auto properties = UIElementCore<UIElement<IUIElement>>::getProperties();
        properties->setProperty( valueStr, m_value );
        properties->setProperty( minValueStr, m_minValue );
        properties->setProperty( maxValueStr, m_maxValue );
        properties->setProperty( showTextStr, m_showText );
        properties->setProperty( fillColourStr, m_fillColour );
        properties->setProperty( backgroundColourStr, m_backgroundColour );
        properties->setProperty( borderColourStr, m_borderColour );
        properties->setProperty( textColourStr, m_textColour );
        properties->setProperty( borderWidthStr, m_borderWidth );
        properties->setProperty( roundingStr, m_rounding );
        return properties;
    }

    void UIProgressBarCore::setProperties( SmartPtr<Properties> properties )
    {
        UIElementCore<UIElement<IUIElement>>::setProperties( properties );
        properties->getPropertyValue( valueStr, m_value );
        properties->getPropertyValue( minValueStr, m_minValue );
        properties->getPropertyValue( maxValueStr, m_maxValue );
        properties->getPropertyValue( showTextStr, m_showText );
        properties->getPropertyValue( fillColourStr, m_fillColour );
        properties->getPropertyValue( backgroundColourStr, m_backgroundColour );
        properties->getPropertyValue( borderColourStr, m_borderColour );
        properties->getPropertyValue( textColourStr, m_textColour );
        properties->getPropertyValue( borderWidthStr, m_borderWidth );
        properties->getPropertyValue( roundingStr, m_rounding );

        if( m_minValue > m_maxValue )
        {
            std::swap( m_minValue, m_maxValue );
        }

        m_value = MathF::clamp( m_value, m_minValue, m_maxValue );
        m_borderWidth = MathF::max( m_borderWidth, 0.0f );
        m_rounding = MathF::max( m_rounding, 0.0f );
    }

    f32 UIProgressBarCore::getValue() const
    {
        return m_value;
    }

    void UIProgressBarCore::setValue( f32 value )
    {
        m_value = MathF::clamp( value, m_minValue, m_maxValue );
    }

    f32 UIProgressBarCore::getMinValue() const
    {
        return m_minValue;
    }

    void UIProgressBarCore::setMinValue( f32 value )
    {
        m_minValue = value;
        if( m_minValue > m_maxValue )
        {
            std::swap( m_minValue, m_maxValue );
        }
        setValue( m_value );
    }

    f32 UIProgressBarCore::getMaxValue() const
    {
        return m_maxValue;
    }

    void UIProgressBarCore::setMaxValue( f32 value )
    {
        m_maxValue = value;
        if( m_minValue > m_maxValue )
        {
            std::swap( m_minValue, m_maxValue );
        }
        setValue( m_value );
    }

    bool UIProgressBarCore::getShowText() const
    {
        return m_showText;
    }

    void UIProgressBarCore::setShowText( bool showText )
    {
        m_showText = showText;
    }

    ColourF UIProgressBarCore::getFillColour() const
    {
        return m_fillColour;
    }

    void UIProgressBarCore::setFillColour( const ColourF &colour )
    {
        m_fillColour = colour;
    }

    ColourF UIProgressBarCore::getBackgroundColour() const
    {
        return m_backgroundColour;
    }

    void UIProgressBarCore::setBackgroundColour( const ColourF &colour )
    {
        m_backgroundColour = colour;
    }

    ColourF UIProgressBarCore::getBorderColour() const
    {
        return m_borderColour;
    }

    void UIProgressBarCore::setBorderColour( const ColourF &colour )
    {
        m_borderColour = colour;
    }

    ColourF UIProgressBarCore::getTextColour() const
    {
        return m_textColour;
    }

    void UIProgressBarCore::setTextColour( const ColourF &colour )
    {
        m_textColour = colour;
    }

    f32 UIProgressBarCore::getBorderWidth() const
    {
        return m_borderWidth;
    }

    void UIProgressBarCore::setBorderWidth( f32 width )
    {
        m_borderWidth = MathF::max( width, 0.0f );
    }

    f32 UIProgressBarCore::getRounding() const
    {
        return m_rounding;
    }

    void UIProgressBarCore::setRounding( f32 rounding )
    {
        m_rounding = MathF::max( rounding, 0.0f );
    }

    f32 UIProgressBarCore::getNormalizedValue() const
    {
        auto range = m_maxValue - m_minValue;
        if( range <= 0.0f )
        {
            return 0.0f;
        }

        return MathF::clamp01( ( m_value - m_minValue ) / range );
    }

    String UIProgressBarCore::getDisplayText() const
    {
        char buffer[32];
        const auto percent = static_cast<int>( getNormalizedValue() * 100.0f + 0.5f );
        std::snprintf( buffer, sizeof( buffer ), "%d%%", percent );
        return String( buffer );
    }
}  // namespace workphone::ui
