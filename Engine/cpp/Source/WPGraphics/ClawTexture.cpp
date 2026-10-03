#include "WPGraphics/WPClawHammerPCH.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
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
        workphone::String sourcePath;
        bool decodeAttempted = false;
        void *textureView = nullptr;
        wp_renderer_dx11 *renderer = nullptr;
    };

    std::mutex g_textureMutex;
    std::unordered_map<const workphone::render::ClawTexture *, ClawTextureData> g_textureData;
    struct FreeImageLifetime
    {
        FreeImageLifetime() { FreeImage_Initialise( FALSE ); }
        ~FreeImageLifetime() { FreeImage_DeInitialise(); }
    };

    void destroyTextureView( ClawTextureData &data )
    {
        if( data.textureView )
        {
            wp_renderer_dx11_destroy_texture_native( data.textureView );
            data.textureView = nullptr;
        }
        data.renderer = nullptr;
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
            destroyTextureView( data );
            data.pixels.clear();
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
            WP_LOG_ERROR( "WPGraphics/Texture: cannot read '" + filePath + "'; stream is missing, empty or exceeds the decoder size limit." );
            return false;
        }

        Array<u8> encoded( streamSize );
        if( stream->read( encoded.data(), streamSize ) != streamSize )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: incomplete read of '" + filePath + "'; expected " + std::to_string( streamSize ) + " bytes." );
            return false;
        }

        // The decoder's plugin registry must be released before this DLL unloads.
        static FreeImageLifetime freeImageLifetime;
        auto memory = FreeImage_OpenMemory( encoded.data(), static_cast<DWORD>( encoded.size() ) );
        if( !memory )
        {
            WP_LOG_ERROR( "WPGraphics/Texture: failed to allocate decoder stream for '" + filePath + "'." );
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
            WP_LOG_ERROR( "WPGraphics/Texture: failed to decode '" + filePath + "'; file format is unsupported or image data is invalid." );
            return false;
        }

        const auto width = FreeImage_GetWidth( converted );
        const auto height = FreeImage_GetHeight( converted );
        if( width == 0 || height == 0 ||
            static_cast<size_t>( width ) >
                std::numeric_limits<size_t>::max() / ( static_cast<size_t>( height ) * 4u ) )
        {
            FreeImage_Unload( converted );
            WP_LOG_ERROR( "WPGraphics/Texture: invalid or oversized image dimensions in '" + filePath + "'." );
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
        destroyTextureView( data );
        data.pixels = std::move( pixels );
        return true;
    }

    bool ensureTextureDecoded( workphone::render::ClawTexture *texture )
    {
        {
            std::scoped_lock lock( g_textureMutex );
            auto found = g_textureData.find( texture );
            if( found != g_textureData.end() && !found->second.pixels.empty() )
            {
                return true;
            }
        }

        return decodeTexture( texture );
    }

    void ensureTextureView( const workphone::render::ClawTexture *texture )
    {
        using namespace workphone;
        using namespace render;

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
        auto renderer = graphicsSystem
                            ? dynamic_pointer_cast<ClawRendererDX11>( graphicsSystem->getRenderer() )
                            : nullptr;
        auto dx11 = renderer ? wp_renderer_get_dx11( renderer->getNativeRenderer() ) : nullptr;

        std::scoped_lock lock( g_textureMutex );
        auto found = g_textureData.find( texture );
        if( found == g_textureData.end() || found->second.pixels.empty() || !dx11 )
        {
            return;
        }

        auto &data = found->second;
        if( data.textureView && data.renderer == dx11 )
        {
            return;
        }

        destroyTextureView( data );
        const auto size = texture->getSize();
        data.textureView = wp_renderer_dx11_create_texture_native(
            dx11, data.pixels.data(), size.x, size.y, WORKPHONE_PIXEL_FORMAT_BGRA8 );
        data.renderer = data.textureView ? dx11 : nullptr;
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
                    g_textureData.erase( found );
                }
            }
            Texture::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        SmartPtr<IRenderTarget> ClawTexture::getRenderTarget() const
        {
            return Texture::getRenderTarget();
        }

        void ClawTexture::setRenderTarget( SmartPtr<IRenderTarget> rt )
        {
            Texture::setRenderTarget( rt );
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
                destroyTextureView( textureData );
                textureData.pixels.resize( byteCount );
                std::memcpy( textureData.pixels.data(), data, byteCount );
            }

            if( auto renderTarget = getRenderTarget() )
            {
                renderTarget->setSize( size );
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
                    ensureTextureView( this );
                    std::scoped_lock lock( g_textureMutex );
                    auto found = g_textureData.find( this );
                    *ppTexture = found != g_textureData.end() ? found->second.textureView : nullptr;
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
                    ensureTextureView( this );
                    std::scoped_lock lock( g_textureMutex );
                    auto found = g_textureData.find( this );
                    *ppTexture = found != g_textureData.end() ? found->second.textureView : nullptr;
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
    }  // namespace render
}  // namespace workphone
