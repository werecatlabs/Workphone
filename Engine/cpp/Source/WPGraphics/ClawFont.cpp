//================================================================================
// Copyright (c) 2024 Workphone Dev
//================================================================================

#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawFont.hpp>
#include <WPGraphics/Resources/FontResourceCompiler.hpp>
#include <Workphone/System/ResourcePayloadStream.hpp>
#include <Workphone/Workphone.hpp>

#include <WorkphoneCore/workphone_font.h>
#include <WorkphoneCore/workphone_memory.h>
#include <WorkphoneCore/workphone_draw.h>
#include <WorkphoneCore/workphone_util.h>
extern "C" {
#include <WorkphoneCore/workphone_utf8.h>
}

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawFont, Font );

    namespace
    {
        const wp_font_glyph *runtimeLookup( const wp_font *font, u32 codepoint )
        {
            const auto end = font->glyphs + font->info.glyph_count;
            const auto found = std::lower_bound(
                font->glyphs, end, codepoint,
                []( const wp_font_glyph &glyph, u32 value ) { return glyph.codepoint < value; } );
            return found != end && found->codepoint == codepoint ? found : font->fallback;
        }

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
            FontGlyphInfo info{};
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
        struct Runtime
        {
            StreamBudget admission;
            std::function<StreamBudget( u64, String & )> storage;
            SmartPtr<ITexture> texture;
            std::vector<wp_font_glyph> glyphs;
            std::vector<wp_rune> ranges;
            struct wp_font_config config{};
            wp_font font{};
            Vector2F whiteUV;
        };

        std::shared_ptr<Runtime> runtime;
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
        if( m_runtimeBound )
        {
            setLoadingState( m_impl && m_impl->runtime ? LoadingState::Loaded : LoadingState::Error );
            return;
        }
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
        return m_impl ? ( m_impl->runtime ? &m_impl->runtime->font : m_impl->defaultFont ) : nullptr;
    }

    namespace
    {
        wp_f32 runtimeWidth( wp_handle handle, wp_f32 height, const wp_c8 *text, wp_s32 length )
        {
            const auto *font = static_cast<const wp_font *>( handle.ptr );
            if( !font || !text || length <= 0 || !std::isfinite( height ) || height <= 0 )
            {
                return 0;
            }
            f32 width = 0;
            wp_s32 at = 0;
            while( at < length )
            {
                wp_rune rune = 0;
                const auto count =
                    wp_utf_decode( reinterpret_cast<const char *>( text + at ), &rune, length - at );
                if( count <= 0 )
                {
                    break;
                }
                at += count;
                const auto glyph = runtimeLookup( font, rune );
                if( glyph )
                {
                    width += glyph->xadvance * ( height / font->info.height );
                }
            }
            return width;
        }

        void runtimeGlyph( wp_handle handle, wp_f32 height, wp_user_font_glyph *out, wp_rune rune,
                           wp_rune )
        {
            if( !out )
            {
                return;
            }
            *out = {};
            const auto *font = static_cast<const wp_font *>( handle.ptr );
            if( !font || !std::isfinite( height ) || height <= 0 )
            {
                return;
            }
            const auto *glyph = runtimeLookup( font, rune );
            if( !glyph )
            {
                return;
            }
            const auto scale = height / font->info.height;
            out->width = ( glyph->x1 - glyph->x0 ) * scale;
            out->height = ( glyph->y1 - glyph->y0 ) * scale;
            out->offset = wp_make_vec2f( glyph->x0 * scale, glyph->y0 * scale );
            out->xadvance = glyph->xadvance * scale;
            out->uv[0] = wp_make_vec2f( glyph->u0, glyph->v0 );
            out->uv[1] = wp_make_vec2f( glyph->u1, glyph->v1 );
        }
    }  // namespace

    bool ClawFont::loadRuntimeFont( const CookedFontData &data, SmartPtr<ITexture> texture,
                                    String &error,
                                    std::shared_ptr<resource::ResourcePayloadMount> mount )
    {
        error.clear();
        try
        {
#if WP_FOLIAGE_SOURCE_RESOURCES
            if( !validateCookedFontData( data, error ) )
            {
                return false;
            }
            if( !texture || !texture->isLoaded() ||
                texture->getSize() != Vector2I( data.atlasWidth, data.atlasHeight ) )
            {
                error = "Runtime font requires its loaded native atlas dimensions";
                return false;
            }
            const u64 bytes =
                16384 + u64( data.glyphs.size() ) * ( sizeof( wp_font_glyph ) + 2 * sizeof( wp_rune ) );
            auto admission = mount ? mount->reserveOwner( StreamMemoryClass::SourceCpu, bytes, error )
                                   : WorldStreamingScheduler::shared()->reserve(
                                         StreamMemoryClass::SourceCpu, bytes, error );
            if( !admission )
            {
                return false;
            }
            auto value = std::allocate_shared<Impl::Runtime>( SourceBudgetAllocator<Impl::Runtime>{} );
            value->admission = std::move( admission );
            value->texture = std::move( texture );
            value->whiteUV = Vector2F( ( data.whiteX + .5f ) / data.atlasWidth,
                                       ( data.whiteY + .5f ) / data.atlasHeight );
            auto allocator = mount ? mount->budgetAllocator()
                                   : std::function<StreamBudget( StreamMemoryClass, u64, String & )>(
                                         []( StreamMemoryClass category, u64 bytes, String &error ) {
                                             return WorldStreamingScheduler::shared()->reserve(
                                                 category, bytes, error );
                                         } );
            value->storage = [allocator = std::move( allocator )]( u64 bytes, String &error ) {
                return allocator( StreamMemoryClass::SourceCpu, bytes, error );
            };
            value->glyphs.reserve( data.glyphs.size() );
            value->ranges.reserve( 2 * data.glyphs.size() + 1 );
            for( const auto &g : data.glyphs )
            {
                wp_font_glyph glyph{};
                glyph.codepoint = g.codepoint;
                glyph.xadvance = g.xadvance;
                glyph.x0 = g.x0;
                glyph.y0 = g.y0;
                glyph.x1 = g.x1;
                glyph.y1 = g.y1;
                glyph.w = g.x1 - g.x0;
                glyph.h = g.y1 - g.y0;
                glyph.u0 = g.u0;
                glyph.v0 = g.v0;
                glyph.u1 = g.u1;
                glyph.v1 = g.v1;
                value->glyphs.push_back( glyph );
                if( !value->ranges.empty() && value->ranges.back() + 1 == g.codepoint )
                {
                    value->ranges.back() = g.codepoint;
                }
                else
                {
                    value->ranges.push_back( g.codepoint );
                    value->ranges.push_back( g.codepoint );
                }
            }
            value->ranges.push_back( 0 );
            value->config.range = value->ranges.data();
            value->config.n = &value->config;
            value->config.p = &value->config;
            value->font.config = &value->config;
            auto &font = value->font;
            font.info.height = data.height;
            font.info.ascent = data.ascent;
            font.info.descent = data.descent;
            font.info.glyph_count = u32( data.glyphs.size() );
            font.info.ranges = value->ranges.data();
            font.glyphs = value->glyphs.data();
            font.scale = 1;
            font.fallback_codepoint = data.fallback;
            font.texture = wp_handle_ptr( value->texture.get() );
            font.handle.texture = font.texture;
            font.handle.height = data.height;
            font.handle.userdata = wp_handle_ptr( &font );
            font.handle.width = runtimeWidth;
            font.handle.query = runtimeGlyph;
            for( const auto &glyph : value->glyphs )
            {
                if( glyph.codepoint == data.fallback )
                {
                    font.fallback = &glyph;
                    break;
                }
            }
            auto impl = std::make_unique<Impl>();
            impl->runtime = std::move( value );
            m_impl.swap( impl );
            m_runtimeBound = true;
            setLoadingState( LoadingState::Loaded );
            return true;
#else
            error = "Typed runtime fonts are unavailable in this build";
            return false;
#endif
        }
        catch( const std::exception &failure )
        {
            try
            {
                error = failure.what();
            }
            catch( ... )
            {
                error.clear();
            }
            return false;
        }
    }

    bool ClawFont::getRuntimeMetrics( BakedFontInfo &out, u32 &fallback ) const
    {
        if( !m_impl || !m_impl->runtime )
        {
            return false;
        }
        out = cBakedFontToCpp( &m_impl->runtime->font.info );
        fallback = m_impl->runtime->font.fallback_codepoint;
        return true;
    }

    u32 ClawFont::getRuntimeGlyphCount() const
    {
        return m_impl && m_impl->runtime ? u32( m_impl->runtime->glyphs.size() ) : 0;
    }

    bool ClawFont::getRuntimeGlyphAt( u32 index, FontGlyphInfo &out ) const
    {
        if( !m_impl || !m_impl->runtime || index >= m_impl->runtime->glyphs.size() )
        {
            return false;
        }
        out = cGlyphToCpp( &m_impl->runtime->glyphs[index] );
        return true;
    }

    SmartPtr<ITexture> ClawFont::getRuntimeAtlas() const
    {
        return m_impl && m_impl->runtime ? m_impl->runtime->texture : nullptr;
    }

    std::shared_ptr<const void> ClawFont::retainRuntimeFont() const
    {
        return m_impl ? m_impl->runtime : std::shared_ptr<const void>{};
    }

    std::shared_ptr<StreamBudgetLease> ClawFont::reserveRuntimeFontStorage( u64 bytes,
                                                                            String &error ) const
    {
        if( !m_impl || !m_impl->runtime )
        {
            error = "Captured bitmap font is unloaded";
            return {};
        }
        return m_impl->runtime->storage( bytes, error );
    }

    bool ClawFont::getRuntimeWhitePixelUV( Vector2F &output ) const
    {
        if( !m_impl || !m_impl->runtime )
        {
            return false;
        }
        output = m_impl->runtime->whiteUV;
        return true;
    }

    struct wp_font_atlas *ClawFont::getNativeAtlas()
    {
        return m_impl && !m_impl->runtime ? &m_impl->atlas : nullptr;
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
        auto glyph = getNativeFont()
                         ? ( m_impl->runtime ? runtimeLookup( getNativeFont(), codepoint )
                                             : wp_font_find_glyph( getNativeFont(), codepoint ) )
                         : nullptr;
        return cGlyphToCpp( glyph );
    }

    FontGlyphInfo ClawFont::findGlyphFromFont( struct wp_font *font, wp_rune codepoint ) const
    {
        auto glyph = wp_font_find_glyph( font, codepoint );
        return cGlyphToCpp( glyph );
    }

    BakedFontInfo ClawFont::getBakedFontInfo() const
    {
        if( !getNativeFont() )
        {
            return BakedFontInfo{};
        }
        return cBakedFontToCpp( &getNativeFont()->info );
    }

    const struct wp_font_glyph *ClawFont::getGlyphs() const
    {
        return m_impl ? ( m_impl->runtime ? m_impl->runtime->glyphs.data() : m_impl->atlas.glyphs )
                      : nullptr;
    }

    s32 ClawFont::getGlyphCount() const
    {
        return m_impl ? ( m_impl->runtime ? s32( m_impl->runtime->glyphs.size() )
                                          : m_impl->atlas.glyph_count )
                      : 0;
    }

    struct wp_font *ClawFont::getFonts() const
    {
        return m_impl ? ( m_impl->runtime ? &m_impl->runtime->font : m_impl->atlas.fonts ) : nullptr;
    }

    s32 ClawFont::getFontCount() const
    {
        return m_impl ? ( m_impl->runtime ? 1 : m_impl->atlas.font_num ) : 0;
    }

    void *ClawFont::getAtlasPixelData() const
    {
        return m_impl && !m_impl->runtime ? m_impl->atlas.pixel : nullptr;
    }

    s32 ClawFont::getAtlasWidth() const
    {
        return m_impl ? ( m_impl->runtime ? m_impl->runtime->texture->getSize().x
                                          : m_impl->atlas.tex_width )
                      : 0;
    }

    s32 ClawFont::getAtlasHeight() const
    {
        return m_impl ? ( m_impl->runtime ? m_impl->runtime->texture->getSize().y
                                          : m_impl->atlas.tex_height )
                      : 0;
    }

    bool ClawFont::isAtlasBaked() const
    {
        return m_impl && ( m_impl->runtime || m_impl->atlasBaked );
    }

}  // namespace workphone::render
