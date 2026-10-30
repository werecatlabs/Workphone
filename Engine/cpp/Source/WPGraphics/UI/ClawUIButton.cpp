#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIButton.hpp>
#include <WPGraphics/UI/ClawUILayout.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ProjectLocalization.hpp>
#include <WorkphoneCore/workphone.h>
#include "ClawUIFrameRetention.hpp"

namespace workphone::ui
{
    ClawUIButton::ClawUIButton()
    {
        m_type = "Button";
    }

    ClawUIButton::~ClawUIButton()
    {
        unload( nullptr );
    }

    void ClawUIButton::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        setLoadingState( LoadingState::Loaded );
    }

    void ClawUIButton::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        ClawUIElement<IUIButton>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    bool ClawUIButton::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return false;
    }

    void ClawUIButton::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUIButton::setTextSize( f32 textSize )
    {
        m_textSize = MathF::clamp( textSize, 0.0f, 255.0f );
    }

    f32 ClawUIButton::getTextSize() const
    {
        return m_textSize;
    }

    bool ClawUIButton::isSimpleButton() const
    {
        return m_isSimpleButton;
    }

    void ClawUIButton::setSimpleButton( bool simpleButton )
    {
        m_isSimpleButton = simpleButton;
    }

    bool ClawUIButton::handleStateChanged( SmartPtr<IState> &state )
    {
        return ClawUIElement<IUIButton>::handleStateChanged( state );
    }

    SmartPtr<Properties> ClawUIButton::getProperties() const
    {
        auto properties = ClawUIElement<IUIButton>::getProperties();
        ScopedLock lock( this, false );
        if( m_hasLocalStyle )
        {
            properties->setProperty( normalColourStr, m_normalColour );
            properties->setProperty( hoverColourStr, m_hoverColour );
            properties->setProperty( activeColourStr, m_activeColour );
            properties->setProperty( "disabledColour", m_disabledColour );
        }
        return properties;
    }

    void ClawUIButton::setProperties( SmartPtr<Properties> properties )
    {
        ClawUIElement<IUIButton>::setProperties( properties );
        if( !properties )
        {
            return;
        }
        ScopedLock lock( this );
        if( properties->hasProperty( normalColourStr ) || properties->hasProperty( hoverColourStr ) ||
            properties->hasProperty( activeColourStr ) || properties->hasProperty( "disabledColour" ) )
        {
            properties->getPropertyValue( normalColourStr, m_normalColour );
            properties->getPropertyValue( hoverColourStr, m_hoverColour );
            properties->getPropertyValue( activeColourStr, m_activeColour );
            properties->getPropertyValue( "disabledColour", m_disabledColour );
            m_hasLocalStyle = true;
        }
    }

    void ClawUIButton::draw( struct wp_context *ctx )
    {
        // Disabled controls remain visible. Actor visibility is handled by the
        // scene component; only input is suppressed here.
        if( !ctx || !ctx->current || !ctx->current->layout || !ctx->style.font || !isVisible() )
        {
            return;
        }
        const auto requestedBounds = getWorkphoneBounds();
        wp_layout_space_push( ctx, requestedBounds );
        wp_rect bounds;
        const auto widgetState = wp_widget( &bounds, ctx );
        if( widgetState == WORKPHONE_WIDGET_INVALID )
        {
            return;
        }

        auto style = ctx->style.button;
        const auto enabled = isEnabled();
        {
            ScopedLock lock( this, false );
            if( m_hasLocalStyle )
            {
                const auto normal = ClawUIWorkphoneContext::toWorkphoneColor(
                    enabled ? m_normalColour : m_disabledColour );
                style.normal = wp_style_item_color( normal );
                style.hover = wp_style_item_color(
                    enabled ? ClawUIWorkphoneContext::toWorkphoneColor( m_hoverColour ) : normal );
                style.active = wp_style_item_color(
                    enabled ? ClawUIWorkphoneContext::toWorkphoneColor( m_activeColour ) : normal );
                style.border = 0;
                style.rounding = 0;
                style.color_factor_background = 1.0f;
            }
        }

        auto textColour = enabled ? ClawUIWorkphoneContext::toWorkphoneColor( getColour() )
                                  : ctx->style.button.text_normal;
        if( !enabled )
        {
            textColour.a = static_cast<wp_byte>( textColour.a * 0.45f );
        }
        style.text_normal = style.text_hover = style.text_active = textColour;
        style.color_factor_text = 1.0f;
        frame_retention::retainFontOwner( this );
        // Commands retain their font pointer until conversion, so each button
        // owns its descriptor instead of borrowing a stack or shared style font.
        if( !m_drawFont )
        {
            m_drawFont = std::make_unique<wp_user_font>();
        }
        *m_drawFont = *ctx->style.font;
        const auto label = localizeProjectText( getLabel() );
        const auto nativeLabel = reinterpret_cast<const wp_c8 *>( label.c_str() );
        auto textHeight = MathF::min( MathF::max( getTextSize(), 1.0f ),
                                      MathF::max( bounds.h - style.padding.y * 2.0f, 1.0f ) );
        const auto width = m_drawFont->width( m_drawFont->userdata, textHeight, nativeLabel,
                                              static_cast<int>( label.size() ) );
        const auto contentWidth = MathF::max( bounds.w - style.padding.x * 2.0f, 1.0f );
        if( width > contentWidth )
        {
            textHeight *= contentWidth / width;
        }
        m_drawFont->height = textHeight;
        const auto *input = !enabled || widgetState == WORKPHONE_WIDGET_ROM ||
                                    widgetState == WORKPHONE_WIDGET_DISABLED ||
                                    ( ctx->current->layout->flags & WORKPHONE_WINDOW_ROM )
                                ? nullptr
                                : &ctx->input;
        // Use a local style and explicit font without mutating the shared context.
        auto *canvas = wp_window_get_canvas( ctx );
        wp_rect content;
        const auto clicked = wp_do_button( &ctx->last_widget_state, canvas, bounds, &style, input,
                                           ctx->button_behavior, &content );
        // Core input emits ACTIVE while its button drawing expects ACTIVED.
        auto drawState = ctx->last_widget_state;
        if( drawState & WORKPHONE_WIDGET_STATE_ACTIVE )
        {
            drawState |= WORKPHONE_WIDGET_STATE_ACTIVED;
        }

        if( style.draw_begin )
        {
            style.draw_begin( canvas, style.userdata );
        }

        wp_draw_button_text( canvas, &bounds, &content, drawState, &style, nativeLabel,
                             static_cast<int>( label.size() ), style.text_alignment, m_drawFont.get() );
        if( style.draw_end )
        {
            style.draw_end( canvas, style.userdata );
        }

        if( clicked )
        {
            handleButtonClick();
        }

        drawWorkphoneChildren( ctx );
    }

    void ClawUIButton::handleButtonClick()
    {
        if( auto layoutElement = getLayout() )
        {
            layoutElement->onActivate( this );
        }

        Array<Parameter> arguments;
        for( auto &listener : getObjectListeners() )
        {
            if( listener )
            {
                listener->handleEvent( EventType::UI, IEvent::CLICK_HASH, arguments, this, nullptr,
                                       nullptr );
            }
        }
    }
}  // namespace workphone::ui
