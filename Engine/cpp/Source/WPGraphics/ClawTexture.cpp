#include "WPGraphics/WPClawHammerPCH.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Graphics/DdsTexture.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/JsonParser.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <FreeImage.h>
#include "ClawAssetBudget.hpp"

#include <cstring>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <cmath>
#include <type_traits>
#include <memory>

namespace
{
    struct ClawTextureData
    {
        // Declared before arrays so their storage is freed before the lease.
        workphone::StreamBudget sourceBudget;
        workphone::Array<workphone::u8> pixels;
        workphone::Array<workphone::f32> floatPixels;
        workphone::Array<workphone::render::TextureMipLevel> cookedLevels;
        std::vector<workphone::u8> ddsBytes;
        workphone::SourceBudgetString cookedProfile;
        workphone::SourceBudgetString sourcePath;
        bool decodeAttempted = false;
        workphone::u32 projectMipPolicy = 0;
        void *textureView = nullptr;
        wp_renderer_dx11 *renderer = nullptr;
        void *device = nullptr;
        workphone::render::TextureMipSettings mipSettings;
        workphone::Vector2I pixelSize{ 0, 0 };
        bool uploadPending = true;
        wp_renderer_dx11 *attemptedRenderer = nullptr;
        void *attemptedDevice = nullptr;
    };

    std::mutex g_textureMutex;
    using TextureCacheKey = const workphone::render::ClawTexture *;
    using TextureCache = std::unordered_map<TextureCacheKey, ClawTextureData,
        std::hash<TextureCacheKey>, std::equal_to<TextureCacheKey>,
        workphone::SourceBudgetAllocator<std::pair<const TextureCacheKey, ClawTextureData>>>;
    // MSVC unordered_map allocates its sentinel/buckets even when empty.
    // Construct lazily, outside DLL initialization, and retire the entire empty
    // map without creating a replacement allocation during teardown.
    class TextureCacheStorage
    {
        workphone::SourceBudgetUnique<TextureCache> m_cache;
    public:
        ClawTextureData &operator[]( TextureCacheKey key )
        {
            if( !m_cache )
            {
                auto next = workphone::makeSourceBudgetUnique<TextureCache>();
                auto &entry = (*next)[key];
                m_cache = std::move(next);
                return entry;
            }
            return (*m_cache)[key];
        }
        TextureCache::iterator find( TextureCacheKey key )
        { return m_cache ? m_cache->find(key) : TextureCache::iterator{}; }
        TextureCache::iterator end()
        { return m_cache ? m_cache->end() : TextureCache::iterator{}; }
        void erase( TextureCache::iterator entry )
        {
            m_cache->erase(entry);
            if( m_cache->empty() ) m_cache.reset();
        }
    };
    TextureCacheStorage g_textureData;

    workphone::StreamBudget reserveTextureCpu( workphone::u64 bytes, workphone::String &error )
    {
        return workphone::WorldStreamingScheduler::shared()->reserve(
            workphone::StreamMemoryClass::SourceCpu, bytes + 4096, error );
    }

    workphone::u64 generatedMipBytes( workphone::u32 width, workphone::u32 height,
                                      const workphone::render::TextureMipSettings &settings )
    {
        workphone::u64 bytes = workphone::u64( width ) * height * 4;
        while( settings.filter != workphone::render::TextureMipFilter::None && ( width > 1 || height > 1 ) )
        {
            width = std::max( width / 2, 1u );
            height = std::max( height / 2, 1u );
            if( !settings.atlasColumns || width < settings.atlasColumns || width % settings.atlasColumns ) break;
            bytes += workphone::u64( width ) * height * 4;
        }
        return bytes + 4096;
    }

    void *retainUploadedTexture( void *view, workphone::StreamBudget budget )
    {
        if( view && !workphone::render::asset_budget::retainTexture( view, std::move( budget ) ) )
        {
            wp_renderer_dx11_destroy_texture_native( view );
            return nullptr;
        }
        return view;
    }

    struct FreeImageLifetime
    {
        FreeImageLifetime()
        {
            FreeImage_Initialise( FALSE );
        }

        ~FreeImageLifetime()
        {
            FreeImage_DeInitialise();
        }
    };

    void destroyTextureView( ClawTextureData &data )
    {
        if( data.textureView )
        {
            wp_renderer_dx11_destroy_texture_native( data.textureView );
            data.textureView = nullptr;
        }
        data.renderer = nullptr;
        data.device = nullptr;
    }

    bool validateCookedMips( const workphone::Array<workphone::render::TextureMipLevel> &levels,
                             const workphone::render::TextureMipSettings &settings,
                             workphone::String &error )
    {
        using namespace workphone::render;
        const auto filter = static_cast<int>( settings.filter );
        if( levels.empty() || levels.size() > 15 || filter < 0 ||
            filter > static_cast<int>( TextureMipFilter::Roughness ) || !settings.atlasColumns ||
            !std::isfinite( settings.alphaCutoff ) || settings.alphaCutoff <= 0 ||
            settings.alphaCutoff >= 1 )
        {
            error = "Invalid cooked texture mip settings or level count";
            return false;
        }
        auto width = levels.front().width;
        auto height = levels.front().height;
        if( !width || !height || width > 16384 || height > 16384 || width % settings.atlasColumns != 0 )
        {
            error = "Invalid cooked texture dimensions or atlas columns";
            return false;
        }
        workphone::u64 totalBytes = 0;
        for( size_t index = 0; index < levels.size(); ++index )
        {
            const auto &level = levels[index];
            const auto bytes = static_cast<workphone::u64>( width ) * height * 4u;
            if( level.width != width || level.height != height || level.bgra.size() != bytes ||
                bytes > graphicsTextureByteLimit - totalBytes )
            {
                error = "Cooked texture mip dimensions, byte length or memory budget is invalid";
                return false;
            }
            totalBytes += bytes;
            const auto nextWidth = std::max( width / 2, 1u );
            const auto nextHeight = std::max( height / 2, 1u );
            const bool last = settings.filter == TextureMipFilter::None ||
                              ( width == 1 && height == 1 ) || nextWidth < settings.atlasColumns ||
                              nextWidth % settings.atlasColumns != 0;
            if( last != ( index + 1 == levels.size() ) )
            {
                error = "Cooked texture must contain its exact complete or atlas-limited mip chain";
                return false;
            }
            width = nextWidth;
            height = nextHeight;
        }
        return true;
    }

    template <class Levels>
    void *uploadMipLevels( wp_renderer_dx11 *renderer, const Levels &levels, bool *budgetDenied = nullptr )
    {
        size_t first = 0;
        workphone::s32 quality = 2, maximum = 0;
        if( auto app = workphone::core::IApplicationManager::instancePtr() )
            if(auto settings = app->getPlayerSettings())
            { settings->getPropertyValue("quality.texture",quality); settings->getPropertyValue("quality.maxTextureSize",maximum); }
        const size_t skip = quality == 0 ? 2u : quality == 1 ? 1u : 0u;
        while(first + 1 < levels.size() && (first < skip ||
              (maximum > 0 && (levels[first].width > static_cast<workphone::u32>(maximum) || levels[first].height > static_cast<workphone::u32>(maximum))))) ++first;
        workphone::u64 bytes = 0;
        for(size_t i = first; i < levels.size(); ++i) bytes += levels[i].bgra.size();
        workphone::String error;
        auto budget = workphone::WorldStreamingScheduler::shared()->reserve(
            workphone::StreamMemoryClass::Gpu, bytes, error );
        if( !budget ) { if( budgetDenied ) *budgetDenied = true; return nullptr; }
        std::vector<wp_texture_mip_dx11> nativeLevels;
        nativeLevels.reserve( levels.size() );
        for(size_t i = first; i < levels.size(); ++i)
        {
            const auto &level = levels[i];
            nativeLevels.push_back( { level.bgra.data(), level.width, level.height } );
        }
        return retainUploadedTexture( wp_renderer_dx11_create_texture_mips_native(
            renderer, nativeLevels.data(), static_cast<wp_u32>( nativeLevels.size() ) ), std::move( budget ) );
    }

    void *uploadDds( wp_renderer_dx11 *renderer, const std::vector<workphone::u8> &bytes,
                     bool *budgetDenied = nullptr )
    {
        const auto image = workphone::dds::inspect( bytes );
        size_t first = 0;
        workphone::s32 quality = 2, maximum = 0;
        if(auto app = workphone::core::IApplicationManager::instancePtr())
            if(auto settings = app->getPlayerSettings())
            { settings->getPropertyValue("quality.texture",quality); settings->getPropertyValue("quality.maxTextureSize",maximum); }
        const size_t skip = quality == 0 ? 2u : quality == 1 ? 1u : 0u;
        while(first + 1 < image.mips.size() && (first < skip || (maximum > 0 &&
              (image.mips[first].width > static_cast<workphone::u32>(maximum) || image.mips[first].height > static_cast<workphone::u32>(maximum))))) ++first;
        workphone::u64 residentBytes = 0;
        for(size_t i=first; i<image.mips.size(); ++i) residentBytes += image.mips[i].size;
        workphone::String error;
        auto budget = workphone::WorldStreamingScheduler::shared()->reserve(
            workphone::StreamMemoryClass::Gpu, residentBytes, error );
        if( !budget ) { if( budgetDenied ) *budgetDenied = true; return nullptr; }
        std::vector<wp_texture_encoded_mip_dx11> levels;
        for(size_t i=first; i<image.mips.size(); ++i)
        {
            const auto &m = image.mips[i];
            levels.push_back( { bytes.data() + m.offset, m.width, m.height, m.size } );
        }
        return retainUploadedTexture( wp_renderer_dx11_create_texture_encoded_mips_native(
            renderer, levels.data(), static_cast<wp_u32>( levels.size() ), image.format ), std::move( budget ) );
    }

    bool decodeTexture( workphone::render::ClawTexture *texture )
    {
        using namespace workphone;

        if( !texture )
        {
            return false;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto fileSystem = applicationManager ? applicationManager->getFileSystemPtr() : nullptr;
        auto filePath = texture->getFilePath();
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            filePath = texture->getName();
        }
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            return false;
        }

        try
        {
            std::scoped_lock lock( g_textureMutex );
            auto &data = g_textureData[texture];
            if( data.decodeAttempted && data.sourcePath == filePath )
            {
                return !data.pixels.empty();
            }
            if( data.sourcePath != filePath ) data.sourcePath = filePath;
            data.decodeAttempted = true;
        }
        catch( const std::bad_alloc & ) { return false; }

        auto stream =
            fileSystem ? fileSystem->open( filePath, true, true, false, false, false ) : nullptr;
        if( !stream && fileSystem )
        {
            stream = fileSystem->open( filePath, true, true, false, true, true );
        }

        const auto streamSize = stream ? stream->size() : 0;
        if( !stream || streamSize == 0 ||
            streamSize > static_cast<size_Num>( std::numeric_limits<DWORD>::max() ) )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: cannot read '" + filePath +
                          "'; stream is missing, empty or exceeds the decoder size limit." );
            return false;
        }

        String budgetError;
        auto encodedBudget = WorldStreamingScheduler::shared()->reserve(
            StreamMemoryClass::WorkingCpu, streamSize, budgetError );
        if( !encodedBudget )
        {
            std::scoped_lock lock( g_textureMutex );
            g_textureData[texture].decodeAttempted = false;
            WP_LOG_WARNING( budgetError );
            return false;
        }
        Array<u8> encoded( streamSize );
        if( stream->read( encoded.data(), streamSize ) != streamSize )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: incomplete read of '" + filePath + "'; expected " +
                          std::to_string( streamSize ) + " bytes." );
            return false;
        }

        // Reserve header/codec metadata before asking a plugin to inspect input.
        // Full bitmap and converted BGRA payloads are admitted after the header.
        auto headerBudget = WorldStreamingScheduler::shared()->reserve(
            StreamMemoryClass::WorkingCpu, 64u * 1024u, budgetError );
        if( !headerBudget )
        {
            std::scoped_lock lock( g_textureMutex );
            g_textureData[texture].decodeAttempted = false;
            WP_LOG_WARNING( budgetError );
            return false;
        }
        // The decoder's plugin registry must be released before this DLL unloads.
        static FreeImageLifetime freeImageLifetime;
        using MemoryOwner = std::unique_ptr<FIMEMORY, decltype( &FreeImage_CloseMemory )>;
        using BitmapOwner = std::unique_ptr<FIBITMAP, decltype( &FreeImage_Unload )>;
        MemoryOwner memory( FreeImage_OpenMemory( encoded.data(), static_cast<DWORD>( encoded.size() ) ),
                            &FreeImage_CloseMemory );
        if( !memory )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: failed to allocate decoder stream for '" + filePath +
                          "'." );
            return false;
        }

        auto format = FreeImage_GetFileTypeFromMemory( memory.get(), 0 );
        if( format == FIF_UNKNOWN )
        {
            format = FreeImage_GetFIFFromFilename( filePath.c_str() );
        }

        if( format == FIF_UNKNOWN || !FreeImage_FIFSupportsNoPixels( format ) )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: format cannot provide a bounded header preflight for '" +
                          filePath + "'; use a cooked texture or convert to PNG/JPEG/BMP/TGA/TIFF/WebP." );
            return false;
        }
        FreeImage_SeekMemory( memory.get(), 0, SEEK_SET );
        BitmapOwner header( FreeImage_LoadFromMemory( format, memory.get(), FIF_LOAD_NOPIXELS ),
                            &FreeImage_Unload );
        auto width = header ? FreeImage_GetWidth( header.get() ) : 0u;
        auto height = header ? FreeImage_GetHeight( header.get() ) : 0u;
        auto pitch = header ? FreeImage_GetPitch( header.get() ) : 0u;
        const auto originalWidth = width;
        const auto originalHeight = height;
        u32 maximumDimension = 0;
        s32 recipeMipFilter = -1;
        try
        {
            // Match the Inspector's exact settings identity, but read an immutable
            // saved recipe. Cached directors contain mutable, unsaved UI fields.
            const auto folder = applicationManager ? applicationManager->getSettingsPath() : String();
            const auto recipePath = folder + StringUtil::toString( StringUtil::getUUID( filePath ) ) +
                                    ".resourcedata";
            if( !folder.empty() && fileSystem && fileSystem->isExistingFile( recipePath ) )
            {
                auto recipeStream = fileSystem->open( recipePath, true, true, false, false, false );
                const auto bytes = recipeStream ? recipeStream->size() : 0;
                if( !recipeStream || !bytes || bytes > ( 64u << 10 ) )
                    throw std::runtime_error( "Saved texture recipe is unreadable, empty or exceeds 64 KiB" );
                auto recipeBudget = WorldStreamingScheduler::shared()->reserve(
                    StreamMemoryClass::WorkingCpu, bytes + 1u, budgetError );
                if( !recipeBudget ) throw std::runtime_error( budgetError.c_str() );
                auto text = std::make_unique<char[]>( static_cast<size_t>( bytes ) + 1u );
                size_t offset = 0;
                while( offset < bytes )
                {
                    const auto read = recipeStream->read( text.get() + offset, bytes - offset );
                    if( !read ) throw std::runtime_error( "Saved texture recipe changed during read" );
                    offset += read;
                }
                char extra;
                if( recipeStream->read( &extra, 1 ) )
                    throw std::runtime_error( "Saved texture recipe grew during read" );
                const auto denials = sourceBudgetDenialCount();
                auto parsed = JsonParser( text.get(), static_cast<u32>( bytes ) ).parse();
                const auto object = std::get_if<JsonObject>( &parsed );
                if( !object ) throw std::runtime_error( "Saved texture recipe must be a valid JSON object" );
                auto settings = make_ptr<Properties>();
                JsonParser( text.get(), static_cast<u32>( bytes ) ).parseToProperties( settings.get() );
                if( sourceBudgetDenialCount() != denials ) throw std::bad_alloc();
                s32 size = 0;
                // A saved TextureResourceDirector always records size. The legacy
                // JSON parser can return an empty object after a syntax error;
                // never turn an unreadable saved policy into an unbounded decode.
                if( !settings->getPropertyValue( "size", size ) ||
                    size < 32 || size > 8192 || ( size & ( size - 1 ) ) )
                    throw std::runtime_error( "Saved texture recipe requires a power-of-two size from 32 to 8192" );
                maximumDimension = static_cast<u32>( size );
                if( settings->getPropertyValue( "mipmapFilter", recipeMipFilter ) &&
                    ( recipeMipFilter < 0 || recipeMipFilter > 5 ) )
                {
                    throw std::runtime_error( "Saved texture mip filter must be in the range 0 to 5" );
                }
            }
        }
        catch( const std::exception &exception )
        {
            std::scoped_lock lock( g_textureMutex );
            g_textureData[texture].decodeAttempted = false;
            WP_LOG_WARNING( String( "Texture decode settings unavailable: " ) + exception.what() );
            return false;
        }
        u32 outputWidth = width, outputHeight = height;
        int decodeFlags = 0;
        if( maximumDimension && std::max( width, height ) > maximumDimension )
        {
            if( format != FIF_JPEG )
            {
                WP_LOG_WARNING( "Bounded raw texture resizing requires JPEG scaled decoding; cook or resize '" +
                                filePath + "' before loading this format." );
                return false;
            }
            const auto longest = std::max( width, height );
            outputWidth = std::max( 1u, static_cast<u32>( u64( width ) * maximumDimension / longest ) );
            outputHeight = std::max( 1u, static_cast<u32>( u64( height ) * maximumDimension / longest ) );
            // FreeImage's JPEG codec reads the requested size from the upper
            // sixteen flag bits and decompresses at 1/2, 1/4 or 1/8 resolution.
            // Ask the header for those exact dimensions before reserving payloads.
            decodeFlags = static_cast<int>( maximumDimension << 16 );
            header.reset();
            FreeImage_SeekMemory( memory.get(), 0, SEEK_SET );
            header.reset( FreeImage_LoadFromMemory( format, memory.get(),
                                                   decodeFlags | FIF_LOAD_NOPIXELS ) );
            width = header ? FreeImage_GetWidth( header.get() ) : 0u;
            height = header ? FreeImage_GetHeight( header.get() ) : 0u;
            pitch = header ? FreeImage_GetPitch( header.get() ) : 0u;
        }
        const auto bgraBytes = u64( width ) * height * 4u;
        const auto outputBytes = u64( outputWidth ) * outputHeight * 4u;
        // ConvertTo32Bits clones an already standard 32-bit bitmap. Reuse that
        // decoder allocation instead: a 4K RGBA image otherwise requires two
        // 64 MiB working bitmaps before its separately charged retained pixels.
        const bool reuseBitmap = header && FreeImage_GetImageType( header.get() ) == FIT_BITMAP &&
                                 FreeImage_GetBPP( header.get() ) == 32;
        if( !width || !height || !outputWidth || !outputHeight ||
            originalWidth > 16384 || originalHeight > 16384 ||
            width > originalWidth || height > originalHeight || !pitch ||
            bgraBytes > render::graphicsTextureByteLimit ||
            outputBytes > render::graphicsTextureByteLimit ||
            u64( pitch ) * height > render::graphicsTextureByteLimit * 4u )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: invalid or oversized image header in '" + filePath + "'." );
            return false;
        }
        auto sourceBudget = reserveTextureCpu( outputBytes, budgetError );
        auto decoderBudget = sourceBudget ? WorldStreamingScheduler::shared()->reserve(
            StreamMemoryClass::WorkingCpu,
            u64( pitch ) * height + ( reuseBitmap ? 0u : bgraBytes ) + 64u * 1024u,
            budgetError ) : StreamBudget{};
        if( !sourceBudget || !decoderBudget )
        {
            std::scoped_lock lock( g_textureMutex );
            g_textureData[texture].decodeAttempted = false;
            WP_LOG_WARNING( budgetError );
            return false;
        }
        header.reset();
        FreeImage_SeekMemory( memory.get(), 0, SEEK_SET );
        BitmapOwner bitmap( FreeImage_LoadFromMemory( format, memory.get(), decodeFlags ), &FreeImage_Unload );
        if( !bitmap || FreeImage_GetWidth( bitmap.get() ) != width ||
            FreeImage_GetHeight( bitmap.get() ) != height || FreeImage_GetPitch( bitmap.get() ) > pitch ||
            ( reuseBitmap && ( FreeImage_GetImageType( bitmap.get() ) != FIT_BITMAP ||
                               FreeImage_GetBPP( bitmap.get() ) != 32 ) ) )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: decoded bitmap does not match its admitted header in '" +
                          filePath + "'." );
            return false;
        }
        BitmapOwner converted( reuseBitmap ? bitmap.release() :
                                            FreeImage_ConvertTo32Bits( bitmap.get() ), &FreeImage_Unload );
        bitmap.reset();
        memory.reset();
        if( !converted || FreeImage_GetWidth( converted.get() ) != width ||
            FreeImage_GetHeight( converted.get() ) != height || FreeImage_GetPitch( converted.get() ) > u64( width ) * 4u )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: failed bounded BGRA conversion for '" + filePath + "'." );
            return false;
        }
        Array<u8> pixels( static_cast<size_t>( outputBytes ) );
        const auto rowBytes = static_cast<size_t>( outputWidth ) * 4u;
        for( u32 y = 0; y < outputHeight; ++y )
        {
            if( width == outputWidth && height == outputHeight )
            {
                // FreeImage scan lines are bottom-up; GPU uploads are top-down.
                const auto source = FreeImage_GetScanLine( converted.get(), height - 1u - y );
                std::memcpy( pixels.data() + static_cast<size_t>( y ) * rowBytes, source, rowBytes );
                continue;
            }
            // JPEG scaling is limited to powers of two and a denominator of eight.
            // Finish an NPOT/large reduction directly into the admitted retained
            // buffer with an area filter; no full-size RGBA copy or rescale bitmap.
            const double top = double( y ) * height / outputHeight;
            const double bottom = double( y + 1u ) * height / outputHeight;
            for( u32 x = 0; x < outputWidth; ++x )
            {
                const double left = double( x ) * width / outputWidth;
                const double right = double( x + 1u ) * width / outputWidth;
                double sum[4]{};
                for( u32 sy = static_cast<u32>( top ); sy < height && sy < std::ceil( bottom ); ++sy )
                {
                    const double wy = std::min( bottom, double( sy + 1u ) ) - std::max( top, double( sy ) );
                    const auto row = FreeImage_GetScanLine( converted.get(), height - 1u - sy );
                    for( u32 sx = static_cast<u32>( left ); sx < width && sx < std::ceil( right ); ++sx )
                    {
                        const double weight = wy * ( std::min( right, double( sx + 1u ) ) -
                                                     std::max( left, double( sx ) ) );
                        for( u32 channel = 0; channel < 4; ++channel )
                            sum[channel] += row[size_t( sx ) * 4u + channel] * weight;
                    }
                }
                const auto area = ( right - left ) * ( bottom - top );
                for( u32 channel = 0; channel < 4; ++channel )
                    pixels[size_t( y ) * rowBytes + size_t( x ) * 4u + channel] =
                        static_cast<u8>( std::min( 255.0, std::floor( sum[channel] / area + 0.5 ) ) );
            }
        }
        converted.reset();

        texture->setSize( Vector2I( static_cast<s32>( outputWidth ), static_cast<s32>( outputHeight ) ) );
        std::scoped_lock lock( g_textureMutex );
        auto &data = g_textureData[texture];
        auto retiredBudget = std::move( data.sourceBudget );
        data.sourceBudget = std::move( sourceBudget );
        data.pixels = std::move( pixels );
        std::decay_t<decltype( data.cookedLevels )>{}.swap( data.cookedLevels );
        data.pixelSize = { static_cast<s32>( outputWidth ), static_cast<s32>( outputHeight ) };
        // Publish the shared texture's saved recipe only with its complete pixels.
        // Per-material sampling edits do not replace another user's physical mip chain.
        if( recipeMipFilter >= 0 )
            data.mipSettings.filter = static_cast<render::TextureMipFilter>( recipeMipFilter );
        data.uploadPending = true;
        return true;
    }

    bool ensureTextureDecoded( workphone::render::ClawTexture *texture )
    {
        {
            std::scoped_lock lock( g_textureMutex );
            auto found = g_textureData.find( texture );
            if( found != g_textureData.end() &&
                ( !found->second.pixels.empty() || !found->second.floatPixels.empty() || !found->second.cookedLevels.empty() ||
                  !found->second.ddsBytes.empty() ) )
            {
                return true;
            }
        }

        return decodeTexture( texture );
    }

    void *ensureTextureView( const workphone::render::ClawTexture *texture,
                             wp_renderer_dx11 *explicitRenderer = nullptr )
    {
        using namespace workphone;
        using namespace render;

        // A caller that already owns the renderer need not acquire the
        // application, graphics system and renderer again on every lookup.
        // Keep the fallback owner alive until texture resolution completes.
        SmartPtr<ClawRendererDX11> renderer;
        auto dx11 = explicitRenderer;
        if( !dx11 )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
            renderer = graphicsSystem
                           ? dynamic_pointer_cast<ClawRendererDX11>( graphicsSystem->getRenderer() )
                           : nullptr;
            dx11 = renderer ? wp_renderer_get_dx11( renderer->getNativeRenderer() ) : nullptr;
        }
        auto device = dx11 ? wp_renderer_dx11_get_device( dx11 ) : nullptr;

        std::scoped_lock lock( g_textureMutex );
        auto found = g_textureData.find( texture );
        if( found == g_textureData.end() || !device )
        {
            return nullptr;
        }

        auto &data = found->second;
        u32 policy = 0;
        if(auto app = core::IApplicationManager::instancePtr())
            if(auto settings = app->getPlayerSettings())
                policy = static_cast<u32>(settings->getPropertyAsInt("quality.texture") + 1) * 65536u +
                         static_cast<u32>(settings->getPropertyAsInt("quality.maxTextureSize"));
        if(data.projectMipPolicy != policy)
        { data.projectMipPolicy = policy; data.uploadPending = true; }
        const auto previousView = data.device == device ? data.textureView : nullptr;
        if( ( data.pixels.empty() && data.floatPixels.empty() && data.cookedLevels.empty() && data.ddsBytes.empty() ) ||
            ( !data.uploadPending && data.attemptedRenderer == dx11 && data.attemptedDevice == device ) )
        {
            return previousView;
        }

        // Retry a failed candidate only when its pixels/settings or device change.
        data.uploadPending = false;
        data.attemptedRenderer = dx11;
        data.attemptedDevice = device;
        try
        {
            void *candidate = nullptr;
            bool budgetDenied = false;
            if( !data.floatPixels.empty() )
            {
                String error;
                auto budget = WorldStreamingScheduler::shared()->reserve(
                    StreamMemoryClass::Gpu, data.floatPixels.size() * sizeof( f32 ), error );
                if( !budget ) { data.uploadPending = true; WP_LOG_WARNING( error ); return previousView; }
                candidate = retainUploadedTexture( wp_renderer_dx11_create_texture_rgba32f_native(
                    dx11, data.floatPixels.data(), static_cast<wp_u32>( data.pixelSize.x ),
                    static_cast<wp_u32>( data.pixelSize.y ),
                    static_cast<wp_u64>( data.floatPixels.size() ) ), std::move( budget ) );
            }
            else if( !data.ddsBytes.empty() )
            {
                candidate = uploadDds( dx11, data.ddsBytes, &budgetDenied );
            }
            else if( !data.cookedLevels.empty() )
            {
                candidate = uploadMipLevels( dx11, data.cookedLevels, &budgetDenied );
            }
            else
            {
                String error;
                auto working = WorldStreamingScheduler::shared()->reserve(
                    StreamMemoryClass::WorkingCpu,
                    generatedMipBytes( data.pixelSize.x, data.pixelSize.y, data.mipSettings ), error );
                if( !working ) { data.uploadPending = true; WP_LOG_WARNING( error ); return previousView; }
                const auto levels = generateTextureMips( data.pixels.data(), data.pixelSize.x,
                                                         data.pixelSize.y, data.mipSettings );
                candidate = uploadMipLevels( dx11, levels, &budgetDenied );
            }
            if( !candidate )
            {
                data.uploadPending = budgetDenied;
                WP_LOG_WARNING(
                    "WPGraphics/Texture: GPU mip upload failed; retaining the previous texture." );
                return previousView;
            }
            destroyTextureView( data );
            data.textureView = candidate;
            data.renderer = dx11;
            data.device = device;
            return candidate;
        }
        catch( const std::exception &e )
        {
            WP_LOG_WARNING(
                String( "WPGraphics/Texture: mip upload rejected; retaining the previous texture: " ) +
                e.what() );
            return previousView;
        }
    }
}  // namespace

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawTexture, Texture );

        // Backend-private observable admission path shared by copyData and
        // createManual. A null input initializes the retained candidate to zero
        // directly; it needs no second temporary image or unchecked void call.
        bool replaceClawTexturePixels( ClawTexture *texture, const void *input,
                                       const Vector2I &size, String &error )
        {
            error.clear();
            if( !texture || size.x <= 0 || size.y <= 0 || size.x > 16384 || size.y > 16384 ||
                u64( size.x ) * size.y * 4u > graphicsTextureByteLimit )
            {
                error = "Texture pixel replacement requires bounded positive dimensions.";
                return false;
            }
            try
            {
                // The old candidate remains charged until its actual array dies.
                StreamBudget retiredBudget;
                const auto byteCount = static_cast<size_t>( size.x ) * size.y * 4u;
                auto sourceBudget = reserveTextureCpu( byteCount, error );
                if( !sourceBudget ) return false;
                Array<u8> pixels( byteCount, 0 );
                if( input ) std::memcpy( pixels.data(), input, byteCount );
                {
                    std::scoped_lock lock( g_textureMutex );
                    auto &data = g_textureData[texture];
                    retiredBudget = std::move( data.sourceBudget );
                    data.sourceBudget = std::move( sourceBudget );
                    data.pixels.swap( pixels );
                    std::decay_t<decltype( data.floatPixels )>{}.swap( data.floatPixels );
                    std::decay_t<decltype( data.cookedLevels )>{}.swap( data.cookedLevels );
                    std::decay_t<decltype( data.ddsBytes )>{}.swap( data.ddsBytes );
                    data.cookedProfile.clear();
                    // Pixel replacements are authored in memory and cannot be
                    // evicted through a wrapper's earlier source file recipe.
                    data.sourcePath.clear();
                    data.decodeAttempted = true;
                    data.pixelSize = size;
                    data.uploadPending = true;
                }
                texture->setSize( size );
                return true;
            }
            catch( const std::exception &exception )
            {
                error = String( "Texture pixel staging failed: " ) + exception.what();
                return false;
            }
        }

        ClawTexture::ClawTexture() : m_size( 0, 0 ), m_usageFlags( 0 )
        {
        }

        ClawTexture::~ClawTexture()
        {
            std::scoped_lock lock( g_textureMutex );
            auto found = g_textureData.find( this );
            if( found != g_textureData.end() )
            {
                destroyTextureView( found->second );
                g_textureData.erase( found );
            }
        }

        void ClawTexture::load( SmartPtr<ISharedObject> data )
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Texture::load( data );

            if( !( getUsageFlags() & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) &&
                !ensureTextureDecoded( this ) )
            {
                WP_LOG_WARNING( "ClawTexture::load: could not decode texture: " + getFilePath() );
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            setLoadingState( LoadingState::Loaded );
        }

        void ClawTexture::unload( SmartPtr<ISharedObject> data )
        {
            // Lazy CPU/native access can acquire physical residency after a
            // rejected load left the logical state Unloaded. Retire that data
            // on every unload; the state alone does not prove cache emptiness.
            // Notifications require fresh Source storage. Their denial cannot
            // strand physical residency that was admitted by an earlier load.
            try { setLoadingState( LoadingState::Unloading ); }
            catch( const std::bad_alloc & ) {}
            {
                std::scoped_lock lock( g_textureMutex );
                auto found = g_textureData.find( this );
                if( found != g_textureData.end() )
                {
                    destroyTextureView( found->second );
                    // Authored properties outlive decoded pixels and GPU residency.
                    const auto settings = found->second.mipSettings;
                    auto cookedLevels = std::move( found->second.cookedLevels );
                    auto floatPixels = std::move( found->second.floatPixels );
                    auto ddsBytes = std::move( found->second.ddsBytes );
                    auto sourceBudget = std::move( found->second.sourceBudget );
                    auto profile = std::move( found->second.cookedProfile );
                    const auto dimensions = found->second.pixelSize;
                    found->second = ClawTextureData{};
                    found->second.mipSettings = settings;
                    found->second.cookedLevels = std::move( cookedLevels );
                    found->second.floatPixels = std::move( floatPixels );
                    found->second.ddsBytes = std::move( ddsBytes );
                    if( !found->second.cookedLevels.empty() || !found->second.floatPixels.empty() ||
                        !found->second.ddsBytes.empty() )
                        found->second.sourceBudget = std::move( sourceBudget );
                    found->second.cookedProfile = std::move( profile );
                    if( !found->second.ddsBytes.empty() || !found->second.floatPixels.empty() )
                    {
                        found->second.pixelSize = dimensions;
                    }
                    if( !found->second.cookedLevels.empty() )
                    {
                        found->second.pixelSize = {
                            static_cast<s32>( found->second.cookedLevels.front().width ),
                            static_cast<s32>( found->second.cookedLevels.front().height )
                        };
                    }
                }
            }

            Texture::unload( data );
            try { setLoadingState( LoadingState::Unloaded ); }
            catch( const std::bad_alloc & ) {}
        }

        void ClawTexture::copyToTexture( SmartPtr<ITexture> &target )
        {
        }

        bool ClawTexture::releaseUnusedCacheData()
        {
            if( isLoadLocked() || getRenderTarget() ||
                ( getUsageFlags() & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) ) return false;
            std::scoped_lock lock( g_textureMutex );
            const auto found = g_textureData.find( this );
            if( found == g_textureData.end() ) return false;
            auto &data = found->second;
            // Source decoding is the only known reload recipe. Cooked/procedural
            // uploads and authoring replacements intentionally retain their data.
            auto filePath = getFilePath();
            if( filePath.empty() ) filePath = getName();
            if( data.sourcePath.empty() || data.sourcePath != filePath ||
                !data.decodeAttempted || data.pixels.empty() || !data.floatPixels.empty() ||
                !data.cookedLevels.empty() || !data.ddsBytes.empty() ) return false;
            auto application = core::IApplicationManager::instancePtr();
            auto files = application ? application->getFileSystemPtr() : nullptr;
            auto source = files ? files->open( filePath, true, true, false, false, false ) : nullptr;
            if( !source && files ) source = files->open( filePath, true, true, false, true, true );
            if( !source || !source->size() ) return false;
            destroyTextureView( data );
            std::decay_t<decltype( data.pixels )>{}.swap( data.pixels );
            data.sourceBudget.reset();
            data.decodeAttempted = false;
            data.uploadPending = true;
            data.attemptedRenderer = nullptr;
            data.attemptedDevice = nullptr;
            setLoadingState( LoadingState::Unloaded );
            return true;
        }

        void ClawTexture::copyData( void *data, const Vector2I &size )
        {
            if( data )
            {
                String error;
                if( !replaceClawTexturePixels( this, data, size, error ) ) WP_LOG_WARNING( error );
                return;
            }
            setSize( size );
        }

        bool ClawTexture::copyByteData( const Array<u8> &bgra, const Vector2I &size, String &error )
        {
            error.clear();
            if( size.x <= 0 || size.y <= 0 || size.x > 16384 || size.y > 16384 ||
                u64( size.x ) * u64( size.y ) * 4 != bgra.size() || getRenderTarget() ||
                ( getUsageFlags() & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) )
            {
                error = "Byte texture staging requires a sampled texture, bounded dimensions and exact BGRA pixels.";
                return false;
            }
            return replaceClawTexturePixels( this, bgra.data(), size, error );
        }

        bool ClawTexture::copyLinearFloatData( const Array<f32> &rgba, const Vector2I &size,
                                               String &error )
        {
            error.clear();
            if( size.x <= 0 || size.y <= 0 || size.x > 16384 || size.y > 16384 ||
                u64( size.x ) * u64( size.y ) * 16 > graphicsTextureByteLimit ||
                u64( size.x ) * u64( size.y ) * 4 != rgba.size() || getRenderTarget() ||
                ( getUsageFlags() & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) )
            {
                error = "Linear float texture requires bounded sampled RGBA dimensions and exact pixel count.";
                return false;
            }
            for( size_t i = 0; i < rgba.size(); ++i )
                if( !std::isfinite( rgba[i] ) || rgba[i] < 0 || ( i % 4 == 3 && rgba[i] > 1 ) )
                {
                    error = "Linear float texture requires finite nonnegative RGB and alpha in [0,1].";
                    return false;
                }
            try
            {
                StreamBudget retiredBudget;
                auto sourceBudget = reserveTextureCpu( rgba.capacity() * sizeof( f32 ), error );
                if( !sourceBudget ) return false;
                auto candidate = rgba;
                std::scoped_lock lock( g_textureMutex );
                auto &data = g_textureData[this];
                retiredBudget = std::move( data.sourceBudget );
                data.sourceBudget = std::move( sourceBudget );
                data.floatPixels.swap( candidate );
                std::decay_t<decltype( data.pixels )>{}.swap( data.pixels );
                std::decay_t<decltype( data.cookedLevels )>{}.swap( data.cookedLevels );
                std::decay_t<decltype( data.ddsBytes )>{}.swap( data.ddsBytes );
                data.cookedProfile.clear();
                data.sourcePath.clear();
                data.decodeAttempted = true;
                data.pixelSize = size;
                data.uploadPending = true;
                m_size = size;
                return true;
            }
            catch( const std::exception &e )
            {
                error = String( "Linear float texture staging failed: " ) + e.what();
                return false;
            }
        }

        std::shared_ptr<const TextureMipLevel> ClawTexture::getCpuBaseMip( String &error )
        {
            error.clear();
            if( getRenderTarget() ||
                ( getUsageFlags() & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) )
            {
                error =
                    "Foliage baking cannot read render-target textures; retained CPU pixels are "
                    "required.";
                return {};
            }
            try
            {
                if( !ensureTextureDecoded( this ) )
                {
                    error = "Foliage baking could not decode texture source: " + getFilePath();
                    return {};
                }
                std::scoped_lock lock( g_textureMutex );
                const auto found = g_textureData.find( this );
                if( found == g_textureData.end() )
                {
                    error = "Texture has no retained CPU data.";
                    return {};
                }
                const auto &data = found->second;
                if( !data.floatPixels.empty() )
                {
                    error = "BGRA8 CPU snapshots cannot represent retained linear HDR pixels.";
                    return {};
                }
                if( !data.ddsBytes.empty() )
                {
                    error =
                        "Foliage baking requires decoded BGRA source pixels; compressed DDS-only "
                        "textures are unavailable.";
                    return {};
                }
                const auto width = data.cookedLevels.empty() ? u32( data.pixelSize.x )
                                                             : data.cookedLevels.front().width;
                const auto height = data.cookedLevels.empty() ? u32( data.pixelSize.y )
                                                              : data.cookedLevels.front().height;
                const auto bytes = u64( width ) * height * 4;
                if( !width || !height || bytes > graphicsTextureByteLimit ||
                    ( data.cookedLevels.empty() ? data.pixels.size()
                                                : data.cookedLevels.front().bgra.size() ) != bytes )
                {
                    error = "Retained texture base mip has invalid dimensions or byte count.";
                    return {};
                }

                struct Snapshot
                {
                    StreamBudget budget;
                    TextureMipLevel mip;
                };

                auto budget = WorldStreamingScheduler::shared()->reserve(
                    StreamMemoryClass::SourceCpu, bytes + sizeof( Snapshot ) + 256, error );
                if( !budget )
                {
                    return {};
                }
                auto snapshot = std::make_shared<Snapshot>();
                snapshot->budget = std::move( budget );
                snapshot->mip.width = width;
                snapshot->mip.height = height;
                if( data.cookedLevels.empty() )
                {
                    snapshot->mip.bgra.assign( data.pixels.begin(), data.pixels.end() );
                }
                else
                {
                    snapshot->mip.bgra = data.cookedLevels.front().bgra;
                }
                return std::shared_ptr<const TextureMipLevel>( snapshot, &snapshot->mip );
            }
            catch( const std::exception &exception )
            {
                error = exception.what();
                return {};
            }
        }

        bool ClawTexture::uploadCookedMips( const Array<TextureMipLevel> &levels,
                                            const TextureMipSettings &settings,
                                            wp_renderer_dx11 *renderer, String &error )
        {
            error.clear();
            auto *device = renderer ? wp_renderer_dx11_get_device( renderer ) : nullptr;
            if( !device || getRenderTarget() ||
                ( getUsageFlags() & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) )
            {
                error = "Cooked mip upload requires a DX11 device and a sampled texture";
                return false;
            }
            if( !validateCookedMips( levels, settings, error ) )
            {
                return false;
            }
            try
            {
                StreamBudget retiredBudget;
                u64 bytes = levels.capacity() * sizeof( TextureMipLevel );
                for( const auto &level : levels ) bytes += level.bgra.capacity();
                auto sourceBudget = reserveTextureCpu( bytes, error );
                if( !sourceBudget ) return false;
                auto retainedLevels = levels;
                {
                    std::scoped_lock lock( g_textureMutex );
                    auto &data = g_textureData[this];
                    auto *candidate = uploadMipLevels( renderer, retainedLevels );
                    if( !candidate )
                    {
                        error = "DX11 rejected the cooked texture mip upload";
                        return false;
                    }
                    // All allocating work has succeeded; publish the complete replacement.
                    destroyTextureView( data );
                    retiredBudget = std::move( data.sourceBudget );
                    data.sourceBudget = std::move( sourceBudget );
                    data.cookedLevels.swap( retainedLevels );
                    std::decay_t<decltype( data.ddsBytes )>{}.swap( data.ddsBytes );
                    data.cookedProfile.clear();
                    std::decay_t<decltype( data.pixels )>{}.swap( data.pixels );
                    std::decay_t<decltype( data.floatPixels )>{}.swap( data.floatPixels );
                    data.sourcePath.clear();
                    data.decodeAttempted = true;
                    data.mipSettings = settings;
                    data.pixelSize = { static_cast<s32>( levels.front().width ),
                                       static_cast<s32>( levels.front().height ) };
                    data.textureView = candidate;
                    data.renderer = renderer;
                    data.device = device;
                    data.attemptedRenderer = renderer;
                    data.attemptedDevice = device;
                    data.uploadPending = false;
                    m_size = data.pixelSize;
                }
                setLoadingState( LoadingState::Loaded );
                return true;
            }
            catch( const std::exception &exception )
            {
                error = String( "Cooked texture upload failed: " ) + exception.what();
                return false;
            }
        }

        bool uploadCookedDds( ClawTexture *texture, const CookedTextureData &input,
                              wp_renderer_dx11 *renderer, String &error )
        {
            error.clear();
            auto device = renderer ? wp_renderer_dx11_get_device( renderer ) : nullptr;
            if( !texture || !device || texture->getRenderTarget() ||
                ( texture->getUsageFlags() & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) )
            {
                error = "Native DDS upload requires a sampled texture and active DX11 device";
                return false;
            }
            if( input.ddsBytes.empty() )
            {
                error = "Native DDS payload is missing";
                return false;
            }
            if( !validateCookedTextureData( input, error ) )
            {
                return false;
            }
            try
            {
                StreamBudget retiredBudget;
                auto sourceBudget = reserveTextureCpu( input.ddsBytes.size(), error );
                if( !sourceBudget ) return false;
                auto retained = input.ddsBytes;
                SourceBudgetString profile( input.target );
                const auto image = dds::inspect( retained );
                std::scoped_lock lock( g_textureMutex );
                auto &data = g_textureData[texture];
                auto view = uploadDds( renderer, retained );
                if( !view )
                {
                    error = "DX11 rejected the DDS format or mip payload; previous texture retained";
                    return false;
                }
                destroyTextureView( data );
                retiredBudget = std::move( data.sourceBudget );
                data.sourceBudget = std::move( sourceBudget );
                data.ddsBytes.swap( retained );
                std::decay_t<decltype( data.cookedLevels )>{}.swap( data.cookedLevels );
                std::decay_t<decltype( data.pixels )>{}.swap( data.pixels );
                std::decay_t<decltype( data.floatPixels )>{}.swap( data.floatPixels );
                data.sourcePath.clear();
                data.cookedProfile.swap( profile );
                data.mipSettings = input.mipSettings;
                data.decodeAttempted = true;
                data.pixelSize = { static_cast<s32>( image.mips[0].width ),
                                   static_cast<s32>( image.mips[0].height ) };
                data.textureView = view;
                data.renderer = renderer;
                data.device = device;
                data.attemptedRenderer = renderer;
                data.attemptedDevice = device;
                data.uploadPending = false;
                texture->setSize( data.pixelSize );
            }
            catch( const std::exception &e )
            {
                error = e.what();
                return false;
            }
            texture->setLoadingState( LoadingState::Loaded );
            return true;
        }

        Vector2I ClawTexture::getSize() const
        {
            return m_size;
        }

        void ClawTexture::setSize( const Vector2I &size )
        {
            m_size = size;

            if( auto renderTarget = getRenderTarget() )
            {
                renderTarget->setSize( size );
            }
        }

        Vector2I ClawTexture::getActualSize() const
        {
            return m_size;
        }

        void ClawTexture::getTextureGPU( void **ppTexture ) const
        {
            if( ppTexture )
            {
                auto renderTarget = dynamic_pointer_cast<ClawRenderTarget>( getRenderTarget() );
                if( renderTarget )
                {
                    *ppTexture = renderTarget->getNativeTextureResource();
                }
                else
                {
                    ensureTextureDecoded( const_cast<ClawTexture *>( this ) );
                    *ppTexture = ensureTextureView( this );
                }
            }
        }

        void ClawTexture::getTextureFinal( void **ppTexture ) const
        {
            if( ppTexture )
            {
                auto renderTarget = dynamic_pointer_cast<ClawRenderTarget>( getRenderTarget() );
                if( renderTarget )
                {
                    *ppTexture = renderTarget->getNativeTextureView();
                }
                else
                {
                    ensureTextureDecoded( const_cast<ClawTexture *>( this ) );
                    *ppTexture = ensureTextureView( this );
                }
            }
        }

        void *ClawTexture::getNativeTextureView( wp_renderer_dx11 *renderer ) const
        {
            if( !renderer || getRenderTarget() ||
                ( getUsageFlags() & static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) )
            {
                return nullptr;
            }
            if( !ensureTextureDecoded( const_cast<ClawTexture *>( this ) ) )
            {
                return nullptr;
            }
            return ensureTextureView( this, renderer );
        }

        size_t ClawTexture::getTextureHandle() const
        {
            void *textureView = nullptr;
            getTextureFinal( &textureView );
            return reinterpret_cast<size_t>( textureView );
        }

        u32 ClawTexture::getUsageFlags() const
        {
            return m_usageFlags;
        }

        void ClawTexture::setUsageFlags( u32 usageFlags )
        {
            m_usageFlags = usageFlags;
        }

        void ClawTexture::_getObject( void **ppObject ) const
        {
            // _getObject is consumed as a native backend texture by ImGui and
            // render APIs. A C++ wrapper is not a valid ID3D11ShaderResourceView.
            getTextureFinal( ppObject );
        }

        SmartPtr<Properties> ClawTexture::getProperties() const
        {
            auto properties = Texture::getProperties();
            std::scoped_lock lock( g_textureMutex );
            const auto found = g_textureData.find( this );
            const auto settings =
                found == g_textureData.end() ? TextureMipSettings{} : found->second.mipSettings;
            properties->setProperty( "mipmapFilter", static_cast<s32>( settings.filter ) );
            properties->setProperty( "mipmapAtlasColumns", settings.atlasColumns );
            properties->setProperty( "mipmapAlphaCutoff", settings.alphaCutoff );
            if( found != g_textureData.end() && !found->second.ddsBytes.empty() )
            {
                const auto image = dds::inspect( found->second.ddsBytes );
                properties->setProperty( "cookedContainer", "DDS DX10" );
                properties->setProperty( "cookedProfile", String(found->second.cookedProfile) );
                properties->setProperty( "cookedDxgiFormat", image.format );
                properties->setProperty( "cookedMipCount", static_cast<u32>( image.mips.size() ) );
                properties->setProperty( "cookedEncodedBytes",
                                         static_cast<u32>( found->second.ddsBytes.size() ) );
                u32 resident = 0;
                for( const auto &m : image.mips )
                {
                    resident += m.size;
                }
                properties->setProperty( "cookedResidentBytes", resident );
                for( const char *name :
                     { "cookedContainer", "cookedProfile", "cookedDxgiFormat", "cookedMipCount",
                       "cookedEncodedBytes", "cookedResidentBytes" } )
                {
                    properties->getPropertyObject( name ).setReadOnly( true );
                }
            }
            return properties;
        }

        void ClawTexture::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                return;
            }
            // Admit cache metadata before base properties can publish edits.
            {
                std::scoped_lock lock( g_textureMutex );
                (void)g_textureData[this];
            }
            Texture::setProperties( properties );
            std::scoped_lock lock( g_textureMutex );
            auto &data = g_textureData[this];
            auto settings = data.mipSettings;
            s32 filter = static_cast<s32>( settings.filter );
            properties->getPropertyValue( "mipmapFilter", filter );
            properties->getPropertyValue( "mipmapAtlasColumns", settings.atlasColumns );
            properties->getPropertyValue( "mipmapAlphaCutoff", settings.alphaCutoff );
            settings.filter = static_cast<TextureMipFilter>( std::clamp( filter, 0, 5 ) );
            settings.atlasColumns = std::max( settings.atlasColumns, 1u );
            settings.alphaCutoff = std::isfinite( settings.alphaCutoff )
                                       ? std::clamp( settings.alphaCutoff, .001f, .999f )
                                       : .5f;
            if( settings.atlasColumns > 16384 ||
                ( data.pixelSize.x > 0 &&
                  static_cast<u32>( data.pixelSize.x ) % settings.atlasColumns != 0 ) )
            {
                WP_LOG_WARNING(
                    "WPGraphics/Texture: mip atlas columns must divide the texture width; preserving "
                    "the previous settings." );
                return;
            }
            if( settings.filter != data.mipSettings.filter ||
                settings.atlasColumns != data.mipSettings.atlasColumns ||
                settings.alphaCutoff != data.mipSettings.alphaCutoff )
            {
                if( !data.ddsBytes.empty() )
                {
                    WP_LOG_WARNING(
                        "Native DDS mip settings require descriptor reimport; retaining cooked "
                        "settings." );
                    return;
                }
                // An explicit authoring edit opts back into legacy mip generation.
                if( !data.cookedLevels.empty() )
                {
                    const auto &base = data.cookedLevels.front().bgra;
                    String error;
                    auto working = WorldStreamingScheduler::shared()->reserve(
                        StreamMemoryClass::WorkingCpu, base.size(), error );
                    if( !working ) { WP_LOG_WARNING( error ); return; }
                    Array<u8> pixels( base.begin(), base.end() );
                    data.pixels = std::move( pixels );
                    std::decay_t<decltype( data.cookedLevels )>{}.swap( data.cookedLevels );
                }
                data.uploadPending = true;
            }
            data.mipSettings = settings;
        }
    }  // namespace render
}  // namespace workphone
