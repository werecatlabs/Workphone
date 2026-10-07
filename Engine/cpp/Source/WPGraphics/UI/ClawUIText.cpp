#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIText.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone, ClawUIText, ClawUIElement<IUIText> );

    ClawUIText::ClawUIText()
    {
        setType( "Text" );
    }

    ClawUIText::~ClawUIText() = default;

    void ClawUIText::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        setLoadingState( LoadingState::Loaded );
    }

    void ClawUIText::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        ClawUIElement<IUIText>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void ClawUIText::setText( const String &text )
    {
        m_text = text;
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateData<UIElementStateData>() )
            {
                state->caption = text;
                return;
            }
        }
        setLabel( text );
    }

    String ClawUIText::getText() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateData<UIElementStateData>() )
            {
                return state->caption;
            }
        }
        return !m_text.empty() ? m_text : getLabel();
    }

    void ClawUIText::setTextSize( f32 textSize )
    {
        m_textSize = MathF::max( textSize, 0.0f );
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateData<UIElementStateData>() )
            {
                state->textSize = static_cast<u8>( MathF::min( m_textSize, 255.0f ) );
            }
        }
    }

    f32 ClawUIText::getTextSize() const
    {
        return m_textSize;
    }

    void ClawUIText::setVerticalAlignment( u8 alignment )
    {
        m_verticalAlignment = alignment;
    }

    u8 ClawUIText::getVerticalAlignment() const
    {
        return m_verticalAlignment;
    }

    void ClawUIText::setHorizontalAlignment( u8 alignment )
    {
        m_horizontalAlignment = alignment;
    }

    u8 ClawUIText::getHorizontalAlignment() const
    {
        return m_horizontalAlignment;
    }

    void ClawUIText::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUIText::setSize( const Vector2F &size )
    {
        ClawUIElement::setSize( size );
    }

    SmartPtr<render::IOverlayElementText> ClawUIText::getOverlayText() const
    {
        return nullptr;
    }

    void ClawUIText::setOverlayText( SmartPtr<render::IOverlayElementText> overlayText )
    {
        // Compatibility no-op: WorkphoneCore text has no retained overlay object.
    }

    void ClawUIText::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = nullptr;
        }
    }

    bool ClawUIText::handleStateChanged( SmartPtr<IState> &state )
    {
        return ClawUIElement<IUIText>::handleStateChanged( state );
    }

    void ClawUIText::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        const auto text = getText();
        if( text.empty() )
        {
            wp_spacing( ctx, 1 );
            return;
        }

        struct wp_rect bounds;
        if( wp_widget( &bounds, ctx ) == WORKPHONE_WIDGET_INVALID || !ctx->style.font )
            return;

        // Commands retain their font pointer until conversion. Keep a per-element
        // font descriptor alive, sharing the atlas while honoring the requested size.
        if( !m_drawFont )
            m_drawFont = std::make_unique<wp_user_font>();
        *m_drawFont = *ctx->style.font;
        auto lines = StringUtil::split( text, "\n" );
        if( lines.empty() )
            return;
        auto height = MathF::min( MathF::max( m_textSize, 1.0f ), bounds.h / lines.size() );
        for( const auto &line : lines )
        {
            const auto width = m_drawFont->width( m_drawFont->userdata, height,
                                                  line.c_str(), static_cast<int>( line.size() ) );
            if( width > bounds.w && width > 0.0f )
                height *= bounds.w / width;
        }
        m_drawFont->height = height;
        const auto blockHeight = height * lines.size();
        auto y = bounds.y;
        if( m_verticalAlignment == static_cast<u8>( VerticalAlignment::CENTER ) )
            y += ( bounds.h - blockHeight ) * 0.5f;
        else if( m_verticalAlignment == static_cast<u8>( VerticalAlignment::BOTTOM ) )
            y += bounds.h - blockHeight;
        for( const auto &line : lines )
        {
            const auto width = m_drawFont->width( m_drawFont->userdata, height,
                                                  line.c_str(), static_cast<int>( line.size() ) );
            auto x = bounds.x;
            if( m_horizontalAlignment == static_cast<u8>( HorizontalAlignment::CENTER ) )
                x += ( bounds.w - width ) * 0.5f;
            else if( m_horizontalAlignment == static_cast<u8>( HorizontalAlignment::RIGHT ) )
                x += bounds.w - width;
            const struct wp_rect lineBounds = { x, y, width, height };
            wp_draw_text( wp_window_get_canvas( ctx ), lineBounds, line.c_str(),
                          static_cast<int>( line.size() ), m_drawFont.get(), wp_rgba( 0, 0, 0, 0 ),
                          ClawUIWorkphoneContext::toWorkphoneColor( getColour() ) );
            y += height;
        }
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
