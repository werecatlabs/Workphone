#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIWorkphoneContext::ClawUIWorkphoneContext()
    {
        m_context = new struct wp_context
        {
        };
        m_memory = new wp_byte[kMemorySize];

        if( !m_context || !m_memory )
        {
            shutdown();
            return;
        }

        // Initialise the font atlas.  The default font is baked into WorkphoneCore.
        wp_font_atlas_init_default( &m_atlas );
        wp_font_atlas_begin( &m_atlas );

        struct wp_font_config cfg = wp_font_config( 13.0f );
        struct wp_font *defaultFont = wp_font_atlas_add_default( &m_atlas, 13.0f, &cfg );
        int width = 0;
        int height = 0;
        const void *pixels =
            wp_font_atlas_bake( &m_atlas, &width, &height, WORKPHONE_FONT_ATLAS_RGBA32 );

        if( pixels && width > 0 && height > 0 )
        {
            m_fontWidth = width;
            m_fontHeight = height;
            const auto byteCount = static_cast<size_t>( width ) * static_cast<size_t>( height ) * 4;
            const auto *begin = static_cast<const wp_byte *>( pixels );
            m_fontPixels.assign( begin, begin + byteCount );
            m_fontTexture = wp_handle_ptr( m_fontPixels.data() );
        }

        m_nullTexture.texture = m_fontTexture;
        m_nullTexture.uv.x = 0.0f;
        m_nullTexture.uv.y = 0.0f;
        wp_font_atlas_end( &m_atlas, m_fontTexture, &m_nullTexture );

        const struct wp_user_font *userFont = defaultFont ? &defaultFont->handle : nullptr;

        // Initialise fixed-size Nuklear context.
        if( wp_init_fixed( m_context, m_memory, kMemorySize, userFont ) == wp_false )
        {
            shutdown();
            return;
        }

        m_valid = true;
    }

    ClawUIWorkphoneContext::~ClawUIWorkphoneContext()
    {
        shutdown();
    }

    void ClawUIWorkphoneContext::shutdown()
    {
        if( m_context )
        {
            wp_free( m_context );
            delete m_context;
            m_context = nullptr;
        }

        // cleanup only releases font source data; clear also frees the glyphs,
        // font objects and configuration allocated for the atlas.
        if( m_atlas.permanent.alloc && m_atlas.permanent.free )
            wp_font_atlas_clear( &m_atlas );

        delete[] static_cast<wp_byte *>( m_memory );
        m_memory = nullptr;

        m_fontPixels.clear();
        m_fontWidth = 0;
        m_fontHeight = 0;
        m_fontTexture = {};
        m_nullTexture = {};

        m_valid = false;
    }

    bool ClawUIWorkphoneContext::isValid() const
    {
        return m_valid;
    }

    struct wp_context *ClawUIWorkphoneContext::getContext() const
    {
        return m_context;
    }

    void *ClawUIWorkphoneContext::getMemory() const
    {
        return m_memory;
    }

    const void *ClawUIWorkphoneContext::getFontPixels() const
    {
        return m_fontPixels.empty() ? nullptr : m_fontPixels.data();
    }

    wp_s32 ClawUIWorkphoneContext::getFontWidth() const
    {
        return m_fontWidth;
    }

    wp_s32 ClawUIWorkphoneContext::getFontHeight() const
    {
        return m_fontHeight;
    }

    wp_handle ClawUIWorkphoneContext::getFontTexture() const
    {
        return m_fontTexture;
    }

    const struct wp_draw_null_texture &ClawUIWorkphoneContext::getNullTexture() const
    {
        return m_nullTexture;
    }

    struct wp_color ClawUIWorkphoneContext::toWorkphoneColor( const ColourF &colour )
    {
        struct wp_color result;
        result.r = static_cast<wp_u8>( MathF::clamp( colour.r, 0.0f, 1.0f ) * 255.0f );
        result.g = static_cast<wp_u8>( MathF::clamp( colour.g, 0.0f, 1.0f ) * 255.0f );
        result.b = static_cast<wp_u8>( MathF::clamp( colour.b, 0.0f, 1.0f ) * 255.0f );
        result.a = static_cast<wp_u8>( MathF::clamp( colour.a, 0.0f, 1.0f ) * 255.0f );
        return result;
    }

    wp_vec2f ClawUIWorkphoneContext::toWorkphoneVec2( const Vector2F &vec )
    {
        wp_vec2f result;
        result.x = vec.X();
        result.y = vec.Y();
        return result;
    }

    struct wp_rect ClawUIWorkphoneContext::toWorkphoneRect( const Vector2F &position,
                                                            const Vector2F &size )
    {
        struct wp_rect result;
        result.x = position.X();
        result.y = position.Y();
        result.w = size.X();
        result.h = size.Y();
        return result;
    }
}  // namespace workphone::ui
