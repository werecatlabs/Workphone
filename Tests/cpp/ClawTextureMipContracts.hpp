#pragma once

#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace claw_texture_mip_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using Microsoft::WRL::ComPtr;

    inline void require( bool condition, const char *message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    class Window : public GraphicsWindow
    {
    public:
        HWND handle = nullptr;
        Vector2I getSize() const override
        {
            return { 32, 32 };
        }
        void getWindowHandle( void *data ) override
        {
            *static_cast<HWND *>( data ) = handle;
        }
        void _getObject( void **data ) const override
        {
            *data = nullptr;
        }
    };

    class Graphics : public ClawHammerSystem
    {
    public:
        void attachRenderer( SmartPtr<IRenderer> renderer )
        {
            m_renderer = renderer;
        }
    };

    struct Fixture
    {
        TypeManager types;
        SmartPtr<core::ApplicationManager> application;
        SmartPtr<Graphics> graphics;
        SmartPtr<ClawRendererDX11> renderer;
        SmartPtr<Window> window;

        Fixture()
        {
            types.load();
            TypeManager::setInstance( &types );
        }

        void initialize()
        {
            application = make_ptr<core::ApplicationManager>();
            core::IApplicationManager::setInstance( application );
            // ISharedObject::getProperties creates Properties through this service.
            application->setFactoryManager( make_ptr<FactoryManager>() );
            graphics = make_ptr<Graphics>();
            application->setGraphicsSystem( graphics );
            window = make_ptr<Window>();
            window->handle =
                CreateWindowExW( 0, L"STATIC", L"Texture mip contracts", WS_OVERLAPPED, 0, 0, 32, 32,
                                 nullptr, nullptr, GetModuleHandleW( nullptr ), nullptr );
            require( window->handle != nullptr, "texture contract window must initialize" );
            renderer = make_ptr<ClawRendererDX11>();
            renderer->load( window );
            require( renderer->isLoaded(), "texture contracts require a DX11 device" );
            graphics->attachRenderer( renderer );
        }

        ~Fixture()
        {
            if( graphics )
                graphics->attachRenderer( nullptr );
            if( renderer )
                renderer->unload( nullptr );
            renderer = nullptr;
            if( window && window->handle )
                DestroyWindow( window->handle );
            window = nullptr;
            if( application )
            {
                application->setGraphicsSystem( nullptr );
                application->setFactoryManager( nullptr );
            }
            graphics = nullptr;
            core::IApplicationManager::setInstance( nullptr );
            application = nullptr;
            TypeManager::setInstance( nullptr );
            types.unload();
        }
    };

    inline ID3D11ShaderResourceView *view( const ClawTexture &texture )
    {
        void *native = nullptr;
        texture.getTextureFinal( &native );
        return static_cast<ID3D11ShaderResourceView *>( native );
    }

    inline D3D11_TEXTURE2D_DESC description( ID3D11ShaderResourceView *view )
    {
        require( view != nullptr, "texture mip view must exist" );
        ComPtr<ID3D11Resource> resource;
        view->GetResource( resource.GetAddressOf() );
        ComPtr<ID3D11Texture2D> texture;
        require( SUCCEEDED( resource.As( &texture ) ), "mip view must expose a 2D texture" );
        D3D11_TEXTURE2D_DESC result{};
        texture->GetDesc( &result );
        return result;
    }

    inline unsigned greenPixel( ID3D11ShaderResourceView *view )
    {
        ComPtr<ID3D11Resource> resource;
        view->GetResource( resource.GetAddressOf() );
        ComPtr<ID3D11Texture2D> texture;
        require( SUCCEEDED( resource.As( &texture ) ), "pixel source must be a 2D texture" );
        ComPtr<ID3D11Device> device;
        texture->GetDevice( device.GetAddressOf() );
        ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext( context.GetAddressOf() );
        auto desc = description( view );
        desc.MipLevels = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, staging.GetAddressOf() ) ),
                 "texture mip readback must allocate" );
        context->CopySubresourceRegion( staging.Get(), 0, 0, 0, 0, texture.Get(), 0, nullptr );
        D3D11_MAPPED_SUBRESOURCE mapped{};
        require( SUCCEEDED( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &mapped ) ),
                 "texture mip readback must map" );
        const auto result = static_cast<const unsigned char *>( mapped.pData )[1];
        context->Unmap( staging.Get(), 0 );
        return result;
    }

    inline void checkSettings( const ClawTexture &texture, TextureMipFilter filter, u32 columns,
                               float cutoff )
    {
        const auto properties = texture.getProperties();
        s32 actualFilter = -1;
        u32 actualColumns = 0;
        float actualCutoff = 0;
        require( properties->getPropertyValue( "mipmapFilter", actualFilter ) &&
                     properties->getPropertyValue( "mipmapAtlasColumns", actualColumns ) &&
                     properties->getPropertyValue( "mipmapAlphaCutoff", actualCutoff ) &&
                     actualFilter == static_cast<s32>( filter ) && actualColumns == columns &&
                     std::abs( actualCutoff - cutoff ) < .00001f,
                 "authored mip settings must survive rejected edits and reload" );
    }

    inline void run()
    {
        Fixture fixture;
        fixture.initialize();
        ClawTexture texture;
        auto properties = make_ptr<Properties>();
        properties->setProperty( "mipmapFilter", static_cast<s32>( TextureMipFilter::Cutout ) );
        properties->setProperty( "mipmapAtlasColumns", 2u );
        properties->setProperty( "mipmapAlphaCutoff", .3f );
        texture.setProperties( properties );
        std::array<unsigned char, 4 * 2 * 4> pixels{};
        for( size_t i = 0; i < pixels.size(); i += 4 )
        {
            pixels[i + 1] = 40;
            pixels[i + 3] = 255;
        }
        texture.copyData( pixels.data(), { 4, 2 } );
        texture.load( nullptr );
        ComPtr<ID3D11ShaderResourceView> initial = view( texture );
        require( description( initial.Get() ).MipLevels == 2 && greenPixel( initial.Get() ) == 40,
                 "wrapper must publish the authored atlas mip chain" );

        properties->setProperty( "mipmapFilter", static_cast<s32>( TextureMipFilter::Data ) );
        properties->setProperty( "mipmapAtlasColumns", 3u );
        texture.setProperties( properties );
        checkSettings( texture, TextureMipFilter::Cutout, 2, .3f );
        require( view( texture ) == initial.Get(), "invalid atlas edit must retain the last good view" );

        properties->setProperty( "mipmapAtlasColumns", 1u );
        texture.setProperties( properties );
        ComPtr<ID3D11ShaderResourceView> complete = view( texture );
        require( complete.Get() != initial.Get() && description( complete.Get() ).MipLevels == 3,
                 "valid edit must publish a replacement with its complete mip chain" );
        require( view( texture ) == complete.Get(), "unchanged metadata must reuse the replacement" );

        // The atlas is valid for the existing pixels, but invalid for this new source size.
        properties->setProperty( "mipmapAtlasColumns", 2u );
        texture.setProperties( properties );
        std::array<unsigned char, 3 * 2 * 4> replacement{};
        for( size_t i = 0; i < replacement.size(); i += 4 )
        {
            replacement[i + 1] = 180;
            replacement[i + 3] = 255;
        }
        texture.copyData( replacement.data(), { 3, 2 } );
        require( view( texture ) == complete.Get() && view( texture ) == complete.Get() &&
                     greenPixel( view( texture ) ) == 40,
                 "invalid pending pixels must preserve the previous GPU pixels without throwing" );
        properties->setProperty( "mipmapAtlasColumns", 1u );
        texture.setProperties( properties );
        require( description( view( texture ) ).Width == 3 && greenPixel( view( texture ) ) == 180,
                 "repairing pending metadata must publish the replacement pixels" );

        properties->setProperty( "mipmapFilter", static_cast<s32>( TextureMipFilter::Cutout ) );
        properties->setProperty( "mipmapAtlasColumns", 3u );
        properties->setProperty( "mipmapAlphaCutoff", .27f );
        texture.setProperties( properties );
        texture.reload( nullptr );
        checkSettings( texture, TextureMipFilter::Cutout, 3, .27f );
        require( view( texture ) == nullptr, "reload without source pixels must release GPU residency" );
        texture.copyData( replacement.data(), { 3, 2 } );
        require( description( view( texture ) ).MipLevels == 1 && greenPixel( view( texture ) ) == 180,
                 "reuploaded pixels must use the retained atlas authoring" );
        texture.unload( nullptr );
        checkSettings( texture, TextureMipFilter::Cutout, 3, .27f );
        require( view( texture ) == nullptr, "explicit unload must release the published view" );
        std::puts( "ClawTexture mip metadata, reload and last-good publication contracts passed." );
    }
}  // namespace claw_texture_mip_contracts
