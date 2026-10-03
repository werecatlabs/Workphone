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

        wp_flags alignment = WORKPHONE_TEXT_ALIGN_LEFT;
        if( m_horizontalAlignment == 1 )
        {
            alignment = WORKPHONE_TEXT_ALIGN_CENTERED;
        }
        else if( m_horizontalAlignment >= 2 )
        {
            alignment = WORKPHONE_TEXT_ALIGN_RIGHT;
        }

        if( m_verticalAlignment == 1 )
        {
            alignment |= WORKPHONE_TEXT_ALIGN_MIDDLE;
        }
        else if( m_verticalAlignment >= 2 )
        {
            alignment |= WORKPHONE_TEXT_ALIGN_BOTTOM;
        }
        else
        {
            alignment |= WORKPHONE_TEXT_ALIGN_TOP;
        }

        wp_label_colored( ctx, reinterpret_cast<const wp_c8 *>( text.c_str() ), alignment,
                          ClawUIWorkphoneContext::toWorkphoneColor( getColour() ) );
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
