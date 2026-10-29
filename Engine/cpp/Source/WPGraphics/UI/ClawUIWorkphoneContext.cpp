#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>
#include <fstream>

namespace workphone::ui
{
    namespace
    {
        std::vector<wp_byte> readTrueTypeFont( const String &path )
        {
            if( path.empty() )
            {
                return {};
            }
            std::ifstream stream( path.c_str(), std::ios::binary | std::ios::ate );
            if( !stream )
            {
                return {};
            }
            const auto length = stream.tellg();
            if( length < 12 || length > 16 * 1024 * 1024 )
            {
                return {};
            }
            std::vector<wp_byte> bytes( static_cast<size_t>( length ) );
            stream.seekg( 0 );
            if( !stream.read( reinterpret_cast<char *>( bytes.data() ), length ) )
            {
                return {};
            }
            const auto u16 = [&]( size_t offset ) -> size_t {
                return ( static_cast<size_t>( bytes[offset] ) << 8 ) | bytes[offset + 1];
            };
            const auto u32 = [&]( size_t offset ) -> size_t {
                return ( u16( offset ) << 16 ) | u16( offset + 2 );
            };
            // stb_truetype expects valid source data and has no length argument.
            // Reject non-TTF, truncated directories and out-of-range tables
            // before handing an optional local asset to that parser.
            const auto signature = u32( 0 );
            if( signature != 0x00010000 && signature != 0x74727565 )
            {
                return {};
            }
            const auto count = u16( 4 );
            if( count == 0 || count > ( bytes.size() - 12 ) / 16 )
            {
                return {};
            }
            const auto directoryEnd = 12 + count * 16;

            struct RequiredTable
            {
                size_t tag;
                size_t minimum;
                bool found;
            };

            RequiredTable required[] = {
                { 0x636d6170, 4, false },   // cmap
                { 0x676c7966, 1, false },   // glyf
                { 0x68656164, 54, false },  // head
                { 0x68686561, 36, false },  // hhea
                { 0x686d7478, 4, false },   // hmtx
                { 0x6c6f6361, 4, false },   // loca
                { 0x6d617870, 6, false }    // maxp
            };
            for( size_t index = 0; index < count; ++index )
            {
                const auto record = 12 + index * 16;
                const auto tag = u32( record );
                const auto offset = u32( record + 8 );
                const auto size = u32( record + 12 );
                if( offset < directoryEnd || offset > bytes.size() || size > bytes.size() - offset )
                {
                    return {};
                }
                for( auto &table : required )
                {
                    if( table.tag == tag )
                    {
                        if( table.found || size < table.minimum )
                        {
                            return {};
                        }
                        table.found = true;
                    }
                }
            }
            for( const auto &table : required )
            {
                if( !table.found )
                {
                    return {};
                }
            }
            return bytes;
        }

        String projectDefaultFont()
        {
            if( auto application = core::IApplicationManager::instancePtr() )
            {
                auto project = application->getProjectPath();
                // The Editor creates its render UI before restoring the project.
                // Its remembered project is already available in preferences.
                if( project.empty() )
                {
                    if( auto settings = application->getEditorSettings() )
                    {
                        String rememberedProject;
                        if( settings->getPropertyValue( "Project Path", rememberedProject ) )
                        {
                            project = Path::getFilePath( rememberedProject );
                        }
                    }
                }
                if( !project.empty() )
                {
                    const auto path = Path::lexically_normal( project + "/Assets/UI/Fonts/Default.ttf" );
                    if( Path::isExistingFile( path ) )
                    {
                        return path;
                    }
                }
            }
            return {};
        }
    }  // namespace

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

        // The optional project face is resolved once at startup. The embedded
        // face remains available for projects without an authored UI font.
        wp_font_atlas_init_default( &m_atlas );
        wp_font_atlas_begin( &m_atlas );

        struct wp_font *defaultFont = nullptr;
        auto fontBytes = readTrueTypeFont( projectDefaultFont() );
        if( !fontBytes.empty() )
        {
            auto config = wp_font_config( 64.0f );
            config.oversample_h = 3;
            config.pixel_snap = 0;
            // The atlas copies this validated buffer, so a file replacement
            // cannot change the bytes between validation and baking.
            defaultFont = wp_font_atlas_add_from_memory( &m_atlas, fontBytes.data(), fontBytes.size(),
                                                         64.0f, &config );
        }
        const auto hasProjectFont = defaultFont != nullptr;
        if( !defaultFont )
        {
            auto config = wp_font_config( 32.0f );
            defaultFont = wp_font_atlas_add_default( &m_atlas, 32.0f, &config );
        }
        int width = 0;
        int height = 0;
        const void *pixels =
            wp_font_atlas_bake( &m_atlas, &width, &height, WORKPHONE_FONT_ATLAS_RGBA32 );

        if( hasProjectFont && ( !pixels || width <= 0 || height <= 0 ) )
        {
            // Adding a file succeeds before the TTF is parsed. A failed bake
            // must discard that atlas before retrying with the embedded face.
            wp_font_atlas_clear( &m_atlas );
            wp_font_atlas_init_default( &m_atlas );
            wp_font_atlas_begin( &m_atlas );
            auto config = wp_font_config( 32.0f );
            defaultFont = wp_font_atlas_add_default( &m_atlas, 32.0f, &config );
            width = height = 0;
            pixels = wp_font_atlas_bake( &m_atlas, &width, &height, WORKPHONE_FONT_ATLAS_RGBA32 );
            WP_LOG_WARNING( "Could not bake the project UI font; using the embedded font." );
        }

        if( !defaultFont || !pixels || width <= 0 || height <= 0 )
        {
            shutdown();
            return;
        }

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

        if( defaultFont )
        {
            defaultFont->handle.height = 13.0f;
        }
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
        {
            wp_font_atlas_clear( &m_atlas );
        }

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
