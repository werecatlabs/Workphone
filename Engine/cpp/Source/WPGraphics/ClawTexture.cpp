#include "WPGraphics/WPClawHammerPCH.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Graphics/DdsTexture.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <FreeImage.h>
#include "ClawAssetBudget.hpp"

#include <cstring>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <cmath>

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
        workphone::String cookedProfile;
        workphone::String sourcePath;
        bool decodeAttempted = false;
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
    std::unordered_map<const workphone::render::ClawTexture *, ClawTextureData> g_textureData;

    workphone::StreamBudget reserveTextureCpu( workphone::u64 bytes, workphone::String &error )
    {
        return workphone::WorldStreamingScheduler::shared()->reserve(
            workphone::StreamMemoryClass::SourceCpu, bytes + sizeof( ClawTextureData ) + 4096, error );
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
        workphone::u64 bytes = 0;
        for( const auto &level : levels ) bytes += level.bgra.size();
        workphone::String error;
        auto budget = workphone::WorldStreamingScheduler::shared()->reserve(
            workphone::StreamMemoryClass::Gpu, bytes, error );
        if( !budget ) { if( budgetDenied ) *budgetDenied = true; return nullptr; }
        std::vector<wp_texture_mip_dx11> nativeLevels;
        nativeLevels.reserve( levels.size() );
        for( const auto &level : levels )
        {
            nativeLevels.push_back( { level.bgra.data(), level.width, level.height } );
        }
        return retainUploadedTexture( wp_renderer_dx11_create_texture_mips_native(
            renderer, nativeLevels.data(), static_cast<wp_u32>( nativeLevels.size() ) ), std::move( budget ) );
    }

    void *uploadDds( wp_renderer_dx11 *renderer, const std::vector<workphone::u8> &bytes,
                     bool *budgetDenied = nullptr )
    {
        const auto image = workphone::dds::inspect( bytes );
        workphone::u64 residentBytes = 0;
        for( const auto &m : image.mips ) residentBytes += m.size;
        workphone::String error;
        auto budget = workphone::WorldStreamingScheduler::shared()->reserve(
            workphone::StreamMemoryClass::Gpu, residentBytes, error );
        if( !budget ) { if( budgetDenied ) *budgetDenied = true; return nullptr; }
        std::vector<wp_texture_encoded_mip_dx11> levels;
        for( const auto &m : image.mips )
        {
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

        {
            std::scoped_lock lock( g_textureMutex );
            auto &data = g_textureData[texture];
            if( data.decodeAttempted && data.sourcePath == filePath )
            {
                return !data.pixels.empty();
            }
            data.sourcePath = filePath;
            data.decodeAttempted = true;
        }

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

        // The decoder's plugin registry must be released before this DLL unloads.
        static FreeImageLifetime freeImageLifetime;
        auto memory = FreeImage_OpenMemory( encoded.data(), static_cast<DWORD>( encoded.size() ) );
        if( !memory )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: failed to allocate decoder stream for '" + filePath +
                          "'." );
            return false;
        }

        auto format = FreeImage_GetFileTypeFromMemory( memory, 0 );
        if( format == FIF_UNKNOWN )
        {
            format = FreeImage_GetFIFFromFilename( filePath.c_str() );
        }

        auto bitmap = format != FIF_UNKNOWN ? FreeImage_LoadFromMemory( format, memory, 0 ) : nullptr;
        auto converted = bitmap ? FreeImage_ConvertTo32Bits( bitmap ) : nullptr;
        if( bitmap )
        {
            FreeImage_Unload( bitmap );
        }
        FreeImage_CloseMemory( memory );

        if( !converted )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: failed to decode '" + filePath +
                          "'; file format is unsupported or image data is invalid." );
            return false;
        }

        const auto width = FreeImage_GetWidth( converted );
        const auto height = FreeImage_GetHeight( converted );
        if( width == 0 || height == 0 || width > 16384 || height > 16384 ||
            u64( width ) * height * 4u > render::graphicsTextureByteLimit ||
            static_cast<size_t>( width ) >
                std::numeric_limits<size_t>::max() / ( static_cast<size_t>( height ) * 4u ) )
        {
            FreeImage_Unload( converted );
            WP_LOG_ERROR( "WPGraphics/Texture: invalid or oversized image dimensions in '" + filePath +
                          "'." );
            return false;
        }

        auto sourceBudget = reserveTextureCpu( u64( width ) * height * 4u, budgetError );
        if( !sourceBudget )
        {
            FreeImage_Unload( converted );
            std::scoped_lock lock( g_textureMutex );
            g_textureData[texture].decodeAttempted = false;
            WP_LOG_WARNING( budgetError );
            return false;
        }
        Array<u8> pixels( static_cast<size_t>( width ) * static_cast<size_t>( height ) * 4u );
        const auto rowBytes = static_cast<size_t>( width ) * 4u;
        for( u32 y = 0; y < height; ++y )
        {
            // FreeImage scan lines are bottom-up; GPU uploads are top-down.
            const auto source = FreeImage_GetScanLine( converted, height - 1u - y );
            std::memcpy( pixels.data() + static_cast<size_t>( y ) * rowBytes, source, rowBytes );
        }
        FreeImage_Unload( converted );

        texture->setSize( Vector2I( static_cast<s32>( width ), static_cast<s32>( height ) ) );
        std::scoped_lock lock( g_textureMutex );
        auto &data = g_textureData[texture];
        auto retiredBudget = std::move( data.sourceBudget );
        data.sourceBudget = std::move( sourceBudget );
        data.pixels = std::move( pixels );
        std::decay_t<decltype( data.cookedLevels )>{}.swap( data.cookedLevels );
        data.pixelSize = { static_cast<s32>( width ), static_cast<s32>( height ) };
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

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
        auto renderer = graphicsSystem
                            ? dynamic_pointer_cast<ClawRendererDX11>( graphicsSystem->getRenderer() )
                            : nullptr;
        auto dx11 = explicitRenderer
                        ? explicitRenderer
                        : ( renderer ? wp_renderer_get_dx11( renderer->getNativeRenderer() ) : nullptr );
        auto device = dx11 ? wp_renderer_dx11_get_device( dx11 ) : nullptr;

        std::scoped_lock lock( g_textureMutex );
        auto found = g_textureData.find( texture );
        if( found == g_textureData.end() || !device )
        {
            return nullptr;
        }

        auto &data = found->second;
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
                // A complete mip chain is smaller than twice its base level.
                auto working = WorldStreamingScheduler::shared()->reserve(
                    StreamMemoryClass::WorkingCpu, u64( data.pixels.size() ) * 2 + 4096, error );
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
            }

            setLoadingState( LoadingState::Loaded );
        }

        void ClawTexture::unload( SmartPtr<ISharedObject> data )
        {
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );
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
            setLoadingState( LoadingState::Unloaded );
        }

        void ClawTexture::copyToTexture( SmartPtr<ITexture> &target )
        {
        }

        void ClawTexture::copyData( void *data, const Vector2I &size )
        {
            if( data && size.x > 0 && size.y > 0 )
            {
                if( size.x > 16384 || size.y > 16384 ||
                    u64( size.x ) * size.y * 4u > graphicsTextureByteLimit ) return;
                const auto byteCount =
                    static_cast<size_t>( size.x ) * static_cast<size_t>( size.y ) * 4u;
                String error;
                auto sourceBudget = reserveTextureCpu( byteCount, error );
                if( !sourceBudget ) { WP_LOG_WARNING( error ); return; }
                std::scoped_lock lock( g_textureMutex );
                auto &textureData = g_textureData[this];
                Array<u8> pixels( byteCount );
                std::memcpy( pixels.data(), data, byteCount );
                auto retiredBudget = std::move( textureData.sourceBudget );
                textureData.sourceBudget = std::move( sourceBudget );
                textureData.pixels = std::move( pixels );
                std::decay_t<decltype( textureData.floatPixels )>{}.swap( textureData.floatPixels );
                std::decay_t<decltype( textureData.cookedLevels )>{}.swap( textureData.cookedLevels );
                std::decay_t<decltype( textureData.ddsBytes )>{}.swap( textureData.ddsBytes );
                textureData.cookedProfile.clear();
                textureData.pixelSize = size;
                textureData.uploadPending = true;
            }

            m_size = size;

            if( auto renderTarget = getRenderTarget() )
            {
                renderTarget->setSize( size );
            }
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
                auto sourceBudget = reserveTextureCpu( rgba.size() * sizeof( f32 ), error );
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
                u64 bytes = levels.size() * sizeof( TextureMipLevel );
                for( const auto &level : levels ) bytes += level.bgra.size();
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
                auto sourceBudget = reserveTextureCpu( input.ddsBytes.size() + input.target.size(), error );
                if( !sourceBudget ) return false;
                auto retained = input.ddsBytes;
                auto profile = input.target;
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
                properties->setProperty( "cookedProfile", found->second.cookedProfile );
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
