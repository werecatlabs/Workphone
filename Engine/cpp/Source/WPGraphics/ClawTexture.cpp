#include "WPGraphics/WPClawHammerPCH.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <FreeImage.h>

#include <cstring>
#include <limits>
#include <mutex>
#include <unordered_map>

namespace
{
    struct ClawTextureData
    {
        workphone::Array<workphone::u8> pixels;
        workphone::Array<workphone::render::TextureMipLevel> cookedLevels;
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
    void *uploadMipLevels( wp_renderer_dx11 *renderer, const Levels &levels )
    {
        std::vector<wp_texture_mip_dx11> nativeLevels;
        nativeLevels.reserve( levels.size() );
        for( const auto &level : levels )
            nativeLevels.push_back( { level.bgra.data(), level.width, level.height } );
        return wp_renderer_dx11_create_texture_mips_native( renderer, nativeLevels.data(),
                                                            static_cast<wp_u32>( nativeLevels.size() ) );
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
            data.pixels.clear();
            data.cookedLevels.clear();
            data.pixelSize = { 0, 0 };
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
        if( width == 0 || height == 0 ||
            static_cast<size_t>( width ) >
                std::numeric_limits<size_t>::max() / ( static_cast<size_t>( height ) * 4u ) )
        {
            FreeImage_Unload( converted );
            WP_LOG_ERROR( "WPGraphics/Texture: invalid or oversized image dimensions in '" + filePath +
                          "'." );
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
        data.pixels = std::move( pixels );
        data.cookedLevels.clear();
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
                ( !found->second.pixels.empty() || !found->second.cookedLevels.empty() ) )
            {
                return true;
            }
        }

        return decodeTexture( texture );
    }

    void *ensureTextureView( const workphone::render::ClawTexture *texture )
    {
        using namespace workphone;
        using namespace render;

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
        auto renderer = graphicsSystem
                            ? dynamic_pointer_cast<ClawRendererDX11>( graphicsSystem->getRenderer() )
                            : nullptr;
        auto dx11 = renderer ? wp_renderer_get_dx11( renderer->getNativeRenderer() ) : nullptr;
        auto device = dx11 ? wp_renderer_dx11_get_device( dx11 ) : nullptr;

        std::scoped_lock lock( g_textureMutex );
        auto found = g_textureData.find( texture );
        if( found == g_textureData.end() || !device )
        {
            return nullptr;
        }

        auto &data = found->second;
        const auto previousView = data.device == device ? data.textureView : nullptr;
        if( ( data.pixels.empty() && data.cookedLevels.empty() ) ||
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
            if( !data.cookedLevels.empty() )
                candidate = uploadMipLevels( dx11, data.cookedLevels );
            else
            {
                const auto levels = generateTextureMips( data.pixels.data(), data.pixelSize.x,
                                                         data.pixelSize.y, data.mipSettings );
                candidate = uploadMipLevels( dx11, levels );
            }
            if( !candidate )
            {
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
                    found->second = ClawTextureData{};
                    found->second.mipSettings = settings;
                    found->second.cookedLevels = std::move( cookedLevels );
                    if( !found->second.cookedLevels.empty() )
                        found->second.pixelSize = {
                            static_cast<s32>( found->second.cookedLevels.front().width ),
                            static_cast<s32>( found->second.cookedLevels.front().height )
                        };
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
            m_size = size;

            if( data && size.x > 0 && size.y > 0 )
            {
                const auto byteCount =
                    static_cast<size_t>( size.x ) * static_cast<size_t>( size.y ) * 4u;
                std::scoped_lock lock( g_textureMutex );
                auto &textureData = g_textureData[this];
                Array<u8> pixels( byteCount );
                std::memcpy( pixels.data(), data, byteCount );
                textureData.pixels = std::move( pixels );
                textureData.cookedLevels.clear();
                textureData.pixelSize = size;
                textureData.uploadPending = true;
            }

            if( auto renderTarget = getRenderTarget() )
            {
                renderTarget->setSize( size );
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
                return false;
            try
            {
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
                    data.cookedLevels.swap( retainedLevels );
                    data.pixels.clear();
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
            return properties;
        }

        void ClawTexture::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
                return;
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
                // An explicit authoring edit opts back into legacy mip generation.
                if( !data.cookedLevels.empty() )
                {
                    const auto &base = data.cookedLevels.front().bgra;
                    Array<u8> pixels( base.begin(), base.end() );
                    data.pixels = std::move( pixels );
                    data.cookedLevels.clear();
                }
                data.uploadPending = true;
            }
            data.mipSettings = settings;
        }
    }  // namespace render
}  // namespace workphone
