#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIText.hpp>
#include <WPGraphics/ClawFont.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ProjectLocalization.hpp>
#include <WorkphoneCore/workphone.h>
#include "ClawUIFrameRetention.hpp"

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone, ClawUIText, ClawUIElement<IUIText> );

    ClawUIText::ClawUIText()
    {
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
        {
            ScopedLock lock( this );
            m_runtimeFont = nullptr;
        }
        ClawUIElement<IUIText>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void ClawUIText::setText( const String &text )
    {
        ScopedLock lock( this );
        m_text = text;
        setLabel( text );
    }

    String ClawUIText::getText() const
    {
        ScopedLock lock( this );
        return !m_text.empty() ? m_text : getLabel();
    }

    void ClawUIText::setTextSize( f32 textSize )
    {
        ScopedLock lock( this );
        m_textSize = MathF::max( textSize, 0.0f );
    }

    f32 ClawUIText::getTextSize() const
    {
        ScopedLock lock( this );
        return m_textSize;
    }

    void ClawUIText::setVerticalAlignment( u8 alignment )
    {
        ScopedLock lock( this );
        m_verticalAlignment = alignment;
    }

    u8 ClawUIText::getVerticalAlignment() const
    {
        ScopedLock lock( this );
        return m_verticalAlignment;
    }

    void ClawUIText::setHorizontalAlignment( u8 alignment )
    {
        ScopedLock lock( this );
        m_horizontalAlignment = alignment;
    }

    u8 ClawUIText::getHorizontalAlignment() const
    {
        ScopedLock lock( this );
        return m_horizontalAlignment;
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

    bool ClawUIText::setRuntimeFont( SmartPtr<render::IFont> font )
    {
        if( font )
        {
            auto native = dynamic_pointer_cast<render::ClawFont>( font );
            if( !native || !font->isLoaded() || !native->retainRuntimeFont() ||
                !native->getNativeFont() )
            {
                return false;
            }
        }
        ScopedLock lock( this );
        m_runtimeFont = std::move( font );
        return true;
    }

    SmartPtr<render::IFont> ClawUIText::getRuntimeFont() const
    {
        ScopedLock lock( this );
        return m_runtimeFont;
    }

    void ClawUIText::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        const auto text = m_runtimeLiteralText ? getText() : localizeProjectText( getText() );
        if( text.empty() )
        {
            wp_spacing( ctx, 1 );
            return;
        }

        struct wp_rect bounds;
        if( wp_widget( &bounds, ctx ) == WORKPHONE_WIDGET_INVALID || !ctx->style.font )
        {
            return;
        }

        frame_retention::retainFontOwner( this );
        // Commands retain their font pointer until conversion. Keep a per-element
        // font descriptor alive, sharing the atlas while honoring the requested size.
        auto font = getRuntimeFont();
        auto native = font ? dynamic_pointer_cast<render::ClawFont>( font ) : nullptr;
        if( font && ( !native || !native->getNativeFont() ) )
        {
            return;
        }
        auto drawFont = std::allocate_shared<wp_user_font>( SourceBudgetAllocator<wp_user_font>{} );
        *drawFont = native ? native->getNativeFont()->handle : *ctx->style.font;
        m_drawFont = drawFont;
        frame_retention::retainSnapshot( drawFont );
        if( font )
        {
            frame_retention::retainResource( font );
            frame_retention::retainSnapshot( font->retainRuntimeFont() );
        }
        auto lines = StringUtil::split( text, "\n" );
        if( lines.empty() )
        {
            return;
        }
        auto height = MathF::min( MathF::max( m_textSize, 1.0f ), bounds.h / lines.size() );
        for( const auto &line : lines )
        {
            const auto width =
                m_drawFont->width( m_drawFont->userdata, height, (const wp_c8 *)line.c_str(),
                                   static_cast<int>( line.size() ) );
            if( width > bounds.w && width > 0.0f )
            {
                height *= bounds.w / width;
            }
        }
        m_drawFont->height = height;
        const auto blockHeight = height * lines.size();
        auto y = bounds.y;
        if( m_verticalAlignment == static_cast<u8>( VerticalAlignment::CENTER ) )
        {
            y += ( bounds.h - blockHeight ) * 0.5f;
        }
        else if( m_verticalAlignment == static_cast<u8>( VerticalAlignment::BOTTOM ) )
        {
            y += bounds.h - blockHeight;
        }
        for( const auto &line : lines )
        {
            const auto width =
                m_drawFont->width( m_drawFont->userdata, height, (const wp_c8 *)line.c_str(),
                                   static_cast<int>( line.size() ) );
            auto x = bounds.x;
            if( m_horizontalAlignment == static_cast<u8>( HorizontalAlignment::CENTER ) )
            {
                x += ( bounds.w - width ) * 0.5f;
            }
            else if( m_horizontalAlignment == static_cast<u8>( HorizontalAlignment::RIGHT ) )
            {
                x += bounds.w - width;
            }
            const struct wp_rect lineBounds = { x, y, width, height };
            wp_draw_text( wp_window_get_canvas( ctx ), lineBounds, (const c8 *)line.c_str(),
                          static_cast<int>( line.size() ), m_drawFont.get(), wp_rgba( 0, 0, 0, 0 ),
                          ClawUIWorkphoneContext::toWorkphoneColor( getColour() ) );
            y += height;
        }
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
