#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawOverlayElementText.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawOverlayElementText, IOverlayElementText );

    ClawOverlayElementText::ClawOverlayElementText() = default;
    ClawOverlayElementText::~ClawOverlayElementText() = default;

    void ClawOverlayElementText::setFontName( const String &fontName )
    {
        m_fontName = fontName;
    }

    void ClawOverlayElementText::setCharHeight( f32 charHeight )
    {
        m_charHeight = charHeight;
    }

    void ClawOverlayElementText::setVerticalAlignment( u8 alignment )
    {
        m_verticalAlignment = alignment;
    }

    u8 ClawOverlayElementText::getVerticalAlignment() const
    {
        return m_verticalAlignment;
    }

    void ClawOverlayElementText::setHorizontalAlignment( u8 alignment )
    {
        m_horizontalAlignment = alignment;
    }

    u8 ClawOverlayElementText::getHorizontalAlignment() const
    {
        return m_horizontalAlignment;
    }

    void ClawOverlayElementText::setSpaceWidth( f32 width )
    {
        m_spaceWidth = width;
    }

    f32 ClawOverlayElementText::getSpaceWidth() const
    {
        return m_spaceWidth;
    }

    void ClawOverlayElementText::update()
    {
        // if( !ctx || !isVisible() ) return;

        // struct wp_rect rect;
        // rect.x = static_cast<short>( m_position.X() );
        // rect.y = static_cast<short>( m_position.Y() );
        // rect.w = static_cast<unsigned short>( m_size.X() );
        // rect.h = static_cast<unsigned short>( m_size.Y() );

        // struct wp_color colour = ClawUIWorkphoneContext::toWorkphoneColor( m_colour );
        // struct wp_color shadow = { 0, 0, 0, 0 };

        //// Note: In a full implementation, we would resolve m_fontName to a wp_user_font.
        //// For now, we use the default font from the context.
        // const struct wp_user_font *font = ctx->stacks.fonts.data;

        // wp_draw_text( &ctx->overlay, rect, m_caption.c_str(), static_cast<int>( m_caption.length() ),
        // font, colour, shadow );
    }
}  // namespace workphone::render
