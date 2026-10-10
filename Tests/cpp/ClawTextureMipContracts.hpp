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

    inline unsigned greenPixel( ID3D11ShaderResourceView *view, UINT mip = 0 )
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
        require( mip < desc.MipLevels, "texture readback mip must exist" );
        desc.Width = std::max( desc.Width >> mip, 1u );
        desc.Height = std::max( desc.Height >> mip, 1u );
        desc.MipLevels = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, staging.GetAddressOf() ) ),
                 "texture mip readback must allocate" );
        context->CopySubresourceRegion( staging.Get(), 0, 0, 0, 0, texture.Get(), mip, nullptr );
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

    inline void cookedContracts( Fixture &fixture )
    {
        ClawTexture texture;
        // These deliberately differ from the base average, proving cooked mips are
        // uploaded verbatim instead of passing through the CPU mip generator again.
        Array<TextureMipLevel> levels( 3 );
        const u32 widths[] = { 4, 2, 1 };
        const u32 heights[] = { 2, 1, 1 };
        const unsigned char green[] = { 20, 117, 231 };
        for( size_t mip = 0; mip < levels.size(); ++mip )
        {
            auto &level = levels[mip];
            level.width = widths[mip];
            level.height = heights[mip];
            level.bgra.resize( size_t( level.width ) * level.height * 4u );
            for( size_t offset = 0; offset < level.bgra.size(); offset += 4 )
            {
                level.bgra[offset + 1] = green[mip];
                level.bgra[offset + 3] = 255;
            }
        }
        TextureMipSettings settings;
        settings.filter = TextureMipFilter::Data;
        String error;
        auto native = wp_renderer_get_dx11( fixture.renderer->getNativeRenderer() );
        require( texture.uploadCookedMips( levels, settings, native, error ) && error.empty(),
                 "valid cooked mips must upload" );
        ComPtr<ID3D11ShaderResourceView> initial = view( texture );
        require(
            description( initial.Get() ).MipLevels == 3 && greenPixel( initial.Get(), 0 ) == green[0] &&
                greenPixel( initial.Get(), 1 ) == green[1] && greenPixel( initial.Get(), 2 ) == green[2],
            "cooked mip bytes must reach the matching native subresources exactly" );

        auto invalid = levels;
        invalid.pop_back();
        require( !texture.uploadCookedMips( invalid, settings, native, error ) && !error.empty(),
                 "incomplete cooked chains must fail before publication" );
        invalid = levels;
        invalid[1].bgra.pop_back();
        require( !texture.uploadCookedMips( invalid, settings, native, error ),
                 "truncated cooked mip bytes must be rejected" );
        auto badSettings = settings;
        badSettings.atlasColumns = 3;
        require( !texture.uploadCookedMips( levels, badSettings, native, error ),
                 "cooked atlas settings must divide the source width" );
        badSettings = settings;
        badSettings.filter = TextureMipFilter::None;
        require( !texture.uploadCookedMips( levels, badSettings, native, error ),
                 "disabled mip filtering must reject surplus cooked levels" );
        require( !texture.uploadCookedMips( levels, settings, nullptr, error ) &&
                     view( texture ) == initial.Get() && greenPixel( initial.Get(), 2 ) == green[2],
                 "invalid cooked uploads must preserve the complete last-good view" );
        checkSettings( texture, settings.filter, 1, .5f );

        // Neither reload nor a different device may fall back to raw file decoding.
        texture.reload( nullptr );
        ComPtr<ID3D11ShaderResourceView> reloaded = view( texture );
        require(
            reloaded && reloaded.Get() != initial.Get() && greenPixel( reloaded.Get(), 2 ) == green[2],
            "reload must retain exact cooked mip data without source IO" );
        ComPtr<ID3D11Device> oldDevice;
        reloaded->GetDevice( oldDevice.GetAddressOf() );
        fixture.renderer->unload( nullptr );
        require( view( texture ) == nullptr, "an unloaded renderer cannot expose a stale GPU view" );
        fixture.renderer->load( fixture.window );
        require( fixture.renderer->isLoaded(), "cooked mip fixture renderer must recreate" );
        native = wp_renderer_get_dx11( fixture.renderer->getNativeRenderer() );
        auto newDevice = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
        ComPtr<ID3D11ShaderResourceView> recreated = view( texture );
        ComPtr<ID3D11Device> viewDevice;
        if( recreated )
            recreated->GetDevice( viewDevice.GetAddressOf() );
        require( newDevice && newDevice != oldDevice.Get() && viewDevice.Get() == newDevice &&
                     greenPixel( recreated.Get(), 1 ) == green[1] &&
                     greenPixel( recreated.Get(), 2 ) == green[2],
                 "renderer recreation must upload exact cooked levels to the new device" );

        auto atlasLevels = levels;
        atlasLevels.pop_back();
        settings.atlasColumns = 2;
        require( texture.uploadCookedMips( atlasLevels, settings, native, error ) &&
                     description( view( texture ) ).MipLevels == 2 &&
                     greenPixel( view( texture ), 1 ) == green[1],
                 "valid atlas-limited cooked chains must retain their authored lower levels" );
        auto properties = make_ptr<Properties>();
        properties->setProperty( "mipmapAtlasColumns", 1u );
        texture.setProperties( properties );
        require( description( view( texture ) ).MipLevels == 3 &&
                     greenPixel( view( texture ), 1 ) == green[0],
                 "explicit authoring edits must leave cooked mode and regenerate from the base" );
        require( texture.uploadCookedMips( levels, TextureMipSettings{ TextureMipFilter::Data }, native,
                                           error ),
                 "cooked upload must replace an authored texture" );
        auto replacement = levels.front().bgra;
        for( size_t offset = 0; offset < replacement.size(); offset += 4 )
            replacement[offset + 1] = 74;
        texture.copyData( replacement.data(), { 4, 2 } );
        require( greenPixel( view( texture ), 2 ) == 74,
                 "legacy copyData must supersede retained cooked levels" );
        texture.unload( nullptr );
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
        cookedContracts( fixture );
        std::puts( "ClawTexture mip metadata, reload and last-good publication contracts passed." );
    }
}  // namespace claw_texture_mip_contracts
