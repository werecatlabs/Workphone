//================================================================================
// Copyright (c) 2024 Workphone Dev
//================================================================================

#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawFont.hpp>
#include <Workphone/Workphone.hpp>

#include <WorkphoneCore/workphone_font.h>
#include <WorkphoneCore/workphone_memory.h>
#include <WorkphoneCore/workphone_draw.h>
#include <WorkphoneCore/workphone_util.h>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawFont, Font );

    namespace
    {
        // Default allocator wrappers for the font system
        void *CFontAlloc( wp_handle userdata, void *ptr, wp_size size )
        {
            (void)userdata;
            if( size == 0 )
            {
                if( ptr != nullptr )
                {
                    free( ptr );
                }
                return nullptr;
            }
            if( ptr == nullptr )
            {
                return malloc( size );
            }
            return realloc( ptr, size );
        }

        void CFontFree( wp_handle userdata, void *ptr )
        {
            (void)userdata;
            if( ptr != nullptr )
            {
                free( ptr );
            }
        }

        // Convert C wp_font_glyph to C++ glyph info
        FontGlyphInfo cGlyphToCpp( const struct wp_font_glyph *cGlyph )
        {
            FontGlyphInfo info;
            if( cGlyph )
            {
                info.codepoint = cGlyph->codepoint;
                info.xadvance = cGlyph->xadvance;
                info.x0 = cGlyph->x0;
                info.y0 = cGlyph->y0;
                info.x1 = cGlyph->x1;
                info.y1 = cGlyph->y1;
                info.width = cGlyph->w;
                info.height = cGlyph->h;
                info.u0 = cGlyph->u0;
                info.v0 = cGlyph->v0;
                info.u1 = cGlyph->u1;
                info.v1 = cGlyph->v1;
            }
            return info;
        }

        // Convert C wp_baked_font to C++ baked font info
        BakedFontInfo cBakedFontToCpp( const struct wp_baked_font *cBaked )
        {
            BakedFontInfo info;
            if( cBaked )
            {
                info.height = cBaked->height;
                info.ascent = cBaked->ascent;
                info.descent = cBaked->descent;
                info.glyphOffset = cBaked->glyph_offset;
                info.glyphCount = cBaked->glyph_count;
                // Note: ranges pointer is owned by the atlas, we just store it
                info.ranges = cBaked->ranges;
            }
            return info;
        }
    }  // namespace

    struct ClawFont::Impl
    {
        wp_font_atlas atlas;
        wp_allocator persistentAlloc;
        wp_allocator transientAlloc;
        struct wp_font *defaultFont;
        bool atlasBaked;
        s32 bakedWidth;
        s32 bakedHeight;

        Impl()
        {
            persistentAlloc.userdata = wp_handle_ptr( nullptr );
            persistentAlloc.alloc = CFontAlloc;
            persistentAlloc.free = CFontFree;

            transientAlloc.userdata = wp_handle_ptr( nullptr );
            transientAlloc.alloc = CFontAlloc;
            transientAlloc.free = CFontFree;

            wp_font_atlas_init( &atlas, &persistentAlloc );
            wp_font_atlas_init_custom( &atlas, &persistentAlloc, &transientAlloc );

            defaultFont = nullptr;
            atlasBaked = false;
            bakedWidth = 0;
            bakedHeight = 0;
        }

        ~Impl()
        {
            if( atlasBaked )
            {
                wp_font_atlas_end( &atlas, wp_handle_ptr( nullptr ), nullptr );
            }

            wp_font_atlas_clear( &atlas );
        }
    };

    u32 ClawFont::m_nameExt = 0;

    ClawFont::ClawFont( IResourceManager *resourceManager )
    {
        static const auto FontStr = String( "Font" );
        auto name = FontStr + StringUtil::toString( m_nameExt++ );

        setName( name );
        setResourceManager( resourceManager );

        m_impl = std::make_unique<Impl>();
    }

    ClawFont::ClawFont() = default;

    ClawFont::~ClawFont()
    {
        const auto &loadingState = getLoadingState();
        if( loadingState != LoadingState::Unloaded )
        {
            unload( nullptr );
        }
    }

    void ClawFont::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Font::load( data );

            // If font source is set, try to load it
            const auto &source = getFontSource();
            if( !StringUtil::isNullOrEmpty( source ) )
            {
                auto height = static_cast<wp_f32>( getFontSize() );
                if( height <= 0.0f )
                {
                    height = 16.0f;
                }

                auto config = wp_font_config( height );

                // Try to add from file first
                auto font =
                    wp_font_atlas_add_from_file( &m_impl->atlas, source.c_str(), height, &config );
                if( !font )
                {
                    // If file loading fails, try default font as fallback
                    config = wp_font_config( height );
                    font = wp_font_atlas_add_default( &m_impl->atlas, height, &config );
                }

                if( !m_impl->defaultFont )
                {
                    m_impl->defaultFont = font;
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ClawFont::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            if( m_impl )
            {
                // End any baked atlas
                if( m_impl->atlasBaked )
                {
                    //wp_handle handle;
                    wp_font_atlas_end( &m_impl->atlas, wp_handle{}, nullptr );
                    m_impl->atlasBaked = false;
                }

                m_impl.reset();
            }

            Font::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    struct wp_font *ClawFont::getNativeFont() const
    {
        return m_impl->defaultFont;
    }

    struct wp_font_atlas *ClawFont::getNativeAtlas()
    {
        return &m_impl->atlas;
    }

    void ClawFont::addFontFromMemory( void *memory, size_t size, f32 height, bool mergeMode )
    {
        if( !memory || size == 0 )
        {
            return;
        }

        auto config = wp_font_config( height );
        config.merge_mode = mergeMode ? 1 : 0;

        auto font = wp_font_atlas_add_from_memory( &m_impl->atlas, memory, static_cast<wp_size>( size ),
                                                   height, &config );
        if( font && !m_impl->defaultFont )
        {
            m_impl->defaultFont = font;
        }
    }

    void ClawFont::addFontFromCompressed( void *memory, size_t size, f32 height, bool mergeMode )
    {
        if( !memory || size == 0 )
        {
            return;
        }

        auto config = wp_font_config( height );
        config.merge_mode = mergeMode ? 1 : 0;

        auto font = wp_font_atlas_add_compressed( &m_impl->atlas, memory, static_cast<wp_size>( size ),
                                                  height, &config );
        if( font && !m_impl->defaultFont )
        {
            m_impl->defaultFont = font;
        }
    }

    void ClawFont::addFontFromCompressedBase85( const char *data, f32 height, bool mergeMode )
    {
        if( !data || *data == '\0' )
        {
            return;
        }

        auto config = wp_font_config( height );
        config.merge_mode = mergeMode ? 1 : 0;

        auto font = wp_font_atlas_add_compressed_base85( &m_impl->atlas, data, height, &config );
        if( font && !m_impl->defaultFont )
        {
            m_impl->defaultFont = font;
        }
    }

    void ClawFont::addFontFromFile( const String &filePath, f32 height, bool mergeMode )
    {
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            return;
        }

        auto config = wp_font_config( height );
        config.merge_mode = mergeMode ? 1 : 0;

        auto font = wp_font_atlas_add_from_file( &m_impl->atlas, filePath.c_str(), height, &config );
        if( font && !m_impl->defaultFont )
        {
            m_impl->defaultFont = font;
        }
    }

    void ClawFont::setDefaultGlyphRanges( const wp_rune *ranges )
    {
        auto config = wp_font_config( 0 );
        config.range = ranges;
        // This would need to be called before adding fonts
        // For now just store the ranges reference
        (void)config;
    }

    void ClawFont::setDefaultGlyphRangesChinese()
    {
        auto ranges = wp_font_chinese_glyph_ranges();
        setDefaultGlyphRanges( ranges );
    }

    void ClawFont::setDefaultGlyphRangesCyrillic()
    {
        auto ranges = wp_font_cyrillic_glyph_ranges();
        setDefaultGlyphRanges( ranges );
    }

    void ClawFont::setDefaultGlyphRangesKorean()
    {
        auto ranges = wp_font_korean_glyph_ranges();
        setDefaultGlyphRanges( ranges );
    }

    const wp_rune *ClawFont::getDefaultGlyphRanges() const
    {
        return wp_font_default_glyph_ranges();
    }

    void ClawFont::beginAtlas()
    {
        wp_font_atlas_begin( &m_impl->atlas );
    }

    const void *ClawFont::bakeAtlas( FontAtlasFormat format, s32 *outWidth, s32 *outHeight )
    {
        auto cFormat = ( format == FontAtlasFormat::RGBA32 ) ? WORKPHONE_FONT_ATLAS_RGBA32
                                                             : WORKPHONE_FONT_ATLAS_ALPHA8;
        return wp_font_atlas_bake( &m_impl->atlas, outWidth, outHeight, cFormat );
    }

    void ClawFont::endAtlas( void *textureHandle )
    {
        struct wp_draw_null_texture nullTex = {};
        nullTex.texture.ptr = nullptr;
        nullTex.uv.x = 0.0f;
        nullTex.uv.y = 0.0f;

        wp_handle handle;
        if( textureHandle )
        {
            handle = wp_handle_ptr( textureHandle );
        }

        wp_font_atlas_end( &m_impl->atlas, wp_handle{}, &nullTex );
        m_impl->atlasBaked = true;
    }

    FontGlyphInfo ClawFont::findGlyph( wp_rune codepoint ) const
    {
        auto glyph = wp_font_find_glyph( m_impl->defaultFont, codepoint );
        return cGlyphToCpp( glyph );
    }

    FontGlyphInfo ClawFont::findGlyphFromFont( struct wp_font *font, wp_rune codepoint ) const
    {
        auto glyph = wp_font_find_glyph( font, codepoint );
        return cGlyphToCpp( glyph );
    }

    BakedFontInfo ClawFont::getBakedFontInfo() const
    {
        if( !m_impl->defaultFont )
        {
            return BakedFontInfo{};
        }
        return cBakedFontToCpp( &m_impl->defaultFont->info );
    }

    const struct wp_font_glyph *ClawFont::getGlyphs() const
    {
        return m_impl->atlas.glyphs;
    }

    s32 ClawFont::getGlyphCount() const
    {
        return m_impl->atlas.glyph_count;
    }

    struct wp_font *ClawFont::getFonts() const
    {
        return m_impl->atlas.fonts;
    }

    s32 ClawFont::getFontCount() const
    {
        return m_impl->atlas.font_num;
    }

    void *ClawFont::getAtlasPixelData() const
    {
        return m_impl->atlas.pixel;
    }

    s32 ClawFont::getAtlasWidth() const
    {
        return m_impl->atlas.tex_width;
    }

    s32 ClawFont::getAtlasHeight() const
    {
        return m_impl->atlas.tex_height;
    }

    bool ClawFont::isAtlasBaked() const
    {
        return m_impl->atlasBaked;
    }

}  // namespace workphone::render
