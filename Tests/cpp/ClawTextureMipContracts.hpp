#pragma once

#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <chrono>
#include <thread>

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

    inline void completeGpuWork( ID3D11DeviceContext *context )
    {
        ComPtr<ID3D11Device> device;
        context->GetDevice( device.GetAddressOf() );
        const D3D11_QUERY_DESC description{ D3D11_QUERY_EVENT, 0 };
        ComPtr<ID3D11Query> completion;
        require( SUCCEEDED( device->CreateQuery( &description, completion.GetAddressOf() ) ),
                 "GPU retirement validation must create a completion event" );
        context->End( completion.Get() );
        context->Flush();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
        for( ;; )
        {
            BOOL completed = FALSE;
            const auto status = context->GetData( completion.Get(), &completed, sizeof( completed ),
                                                  D3D11_ASYNC_GETDATA_DONOTFLUSH );
            require( SUCCEEDED( status ), "GPU retirement completion query must remain valid" );
            if( status == S_OK && completed ) break;
            require( std::chrono::steady_clock::now() < deadline,
                     "GPU retirement completion must finish within five seconds" );
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
        // Final external releases may queue destruction after prior draw work.
        context->Flush();
    }

    class Window : public GraphicsWindow
    {
    public:
        HWND handle = nullptr;
        bool captureSucceeds = false;
        u8 captureValue = 0;
        mutable unsigned captureCalls = 0;
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
        bool tryCopyContentsToMemory( void *buffer, u32 size,
                                      FrameBuffer bufferId = FrameBuffer::Auto ) const override
        {
            ++captureCalls;
            if( !captureSucceeds || !buffer || size < 32u * 32u * 4u ||
                bufferId == FrameBuffer::Front )
                return false;
            std::fill_n( static_cast<u8 *>( buffer ), 32u * 32u * 4u, captureValue );
            return true;
        }
    };

    class Graphics : public ClawHammerSystem
    {
    public:
        SmartPtr<IGraphicsWindow> captureWindow;
        SmartPtr<IGraphicsWindow> getDefaultWindow() const override
        {
            return captureWindow ? captureWindow : ClawHammerSystem::getDefaultWindow();
        }
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
            {
                graphics->captureWindow = nullptr;
                graphics->attachRenderer( nullptr );
            }
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

    inline void assetBudgetContracts( Fixture &fixture )
    {
        auto scheduler = WorldStreamingScheduler::shared();
        const auto source = size_t( StreamMemoryClass::SourceCpu );
        const auto gpu = size_t( StreamMemoryClass::Gpu );
        const auto baseline = scheduler->stats();
        auto native = wp_renderer_get_dx11( fixture.renderer->getNativeRenderer() );
        String error;
        Array<TextureMipLevel> levels( 1 );
        levels[0].width = levels[0].height = 2;
        levels[0].bgra.resize( 16, 255 );
        TextureMipSettings settings;
        settings.filter = TextureMipFilter::None;
        ComPtr<ID3D11ShaderResourceView> retainedView;
        ComPtr<ID3D11Resource> retainedResource;
        {
            ClawTexture texture;
            require( texture.uploadCookedMips( levels, settings, native, error ),
                     "ordinary cooked texture upload must admit its own CPU/GPU allocations" );
            retainedView = view( texture );
            retainedView->GetResource( retainedResource.GetAddressOf() );
            const auto resident = scheduler->stats();
            require( resident.used[gpu] == baseline.used[gpu] + 16 &&
                         resident.used[source] >= baseline.used[source] + 16 &&
                         view( texture ) == retainedView.Get() &&
                         scheduler->stats().used[gpu] == resident.used[gpu],
                     "texture allocation must charge exact GPU mip bytes once and retain CPU pixels" );
            require( scheduler->setBudget( StreamMemoryClass::Gpu, resident.used[gpu], error ),
                     "texture rejection test must constrain available GPU capacity" );
            require( !texture.uploadCookedMips( levels, settings, native, error ) &&
                         view( texture ) == retainedView.Get() &&
                         scheduler->stats().used[source] == resident.used[source] &&
                         scheduler->stats().used[gpu] == resident.used[gpu],
                     "GPU budget rejection must preserve previous texture and release candidate CPU bytes" );
            require( scheduler->setBudget( StreamMemoryClass::Gpu, baseline.limits[gpu], error ) &&
                         scheduler->setBudget( StreamMemoryClass::SourceCpu, resident.used[source], error ),
                     "texture rejection test must constrain available source capacity" );
            auto replacement = levels[0].bgra;
            replacement[1] = 20;
            texture.copyData( replacement.data(), { 2, 2 } );
            require( view( texture ) == retainedView.Get() && greenPixel( retainedView.Get() ) == 255 &&
                         scheduler->stats().used[source] == resident.used[source],
                     "source budget rejection must preserve retained pixels and the published view" );
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, baseline.limits[source], error ),
                     "texture source capacity must restore" );
            texture.unload( nullptr );
            require( scheduler->stats().used[gpu] == resident.used[gpu],
                     "wrapper unload must keep GPU budget while a native view/resource survives" );
        }
        require( scheduler->stats().used[source] == baseline.used[source],
                 "texture destruction must release retained source storage" );
        retainedView.Reset();
        require( scheduler->stats().used[gpu] == baseline.used[gpu] + 16,
                 "a retained native texture resource must retain the GPU lease after its view dies" );
        retainedResource.Reset();
        require( scheduler->stats().used[gpu] == baseline.used[gpu],
                 "the last native resource release must return its GPU capacity" );

        class BudgetMesh final : public ClawMesh
        {
        public:
            BudgetMesh()
            {
                m_mesh = wp_graphics_mesh_create();
                const wp_graphics_mesh_vertex_pnt vertices[] = {
                    { { -.2f, -.2f, .5f }, { 0, 0, -1 }, { 0, 0 } },
                    { { .2f, -.2f, .5f }, { 0, 0, -1 }, { 1, 0 } },
                    { { 0, .2f, .5f }, { 0, 0, -1 }, { .5f, 1 } }
                };
                const wp_u32 indices[] = { 0, 1, 2 };
                require( m_mesh && wp_graphics_mesh_set_vertices( m_mesh, WORKPHONE_VERTEX_FORMAT_PNT,
                             vertices, 3 ) && wp_graphics_mesh_set_indices_u32( m_mesh, indices, 3 ),
                         "mesh budget fixture must create a native triangle" );
            }
        } mesh;
        auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
        context->ClearState();
        wp_renderer_dx11_invalidate_state( native );
        const auto meshBaseline = scheduler->stats().used[gpu];
        require( scheduler->setBudget( StreamMemoryClass::Gpu, meshBaseline, error ),
                 "mesh admission test must exhaust GPU capacity" );
        fixture.renderer->renderMesh( &mesh, Matrix4F::identity() );
        require( scheduler->stats().used[gpu] == meshBaseline,
                 "denied ordinary mesh geometry must not allocate unbudgeted fallback buffers" );
        mesh.load( nullptr );
        require( !mesh.isLoaded() && scheduler->stats().used[gpu] == meshBaseline,
                 "active-renderer mesh load must remain Unloaded until GPU geometry is admitted" );
        require( scheduler->setBudget( StreamMemoryClass::Gpu, baseline.limits[gpu], error ),
                 "mesh capacity must restore" );
        mesh.load( nullptr );
        require( mesh.isLoaded(), "mesh load must retry admission after GPU capacity returns" );
        fixture.renderer->renderMesh( &mesh, Matrix4F::identity() );
        const auto expected = 3 * sizeof( wp_vertex_pntc ) + 3 * sizeof( wp_u32 );
        require( scheduler->stats().used[gpu] == meshBaseline + expected,
                 "ordinary Claw mesh must charge normalized GPU vertices and u32 indices" );
        fixture.renderer->renderMesh( &mesh, Matrix4F::identity() );
        require( scheduler->stats().used[gpu] == meshBaseline + expected,
                 "cached mesh draw must share its existing allocation lease" );
        const auto previousMesh = mesh.getNativeMesh();
        Array<wp_graphics_mesh_vertex_pntc> replacementVertices( 3 );
        replacementVertices[0] = { { -.3f, -.3f, .5f }, { 0, 0, -1 }, { 0, 0 }, 0xffffffffu };
        replacementVertices[1] = { { .3f, -.3f, .5f }, { 0, 0, -1 }, { 1, 0 }, 0xffffffffu };
        replacementVertices[2] = { { 0, .3f, .5f }, { 0, 0, -1 }, { .5f, 1 }, 0xffffffffu };
        const Array<u32> replacementIndices{ 0, 1, 2 };
        require( scheduler->setBudget( StreamMemoryClass::Gpu, meshBaseline + expected, error ),
                 "mesh replacement test must exhaust candidate capacity" );
        require( !mesh.updateGeometry( replacementVertices, replacementIndices ) && mesh.isLoaded() &&
                     mesh.getNativeMesh() == previousMesh && scheduler->stats().used[gpu] == meshBaseline + expected,
                 "denied geometry replacement must retain the last loaded native mesh and its GPU allocation" );
        context->ClearState();
        wp_renderer_dx11_invalidate_state( native );
        const wp_u32 changedIndices[] = { 0, 2, 1 };
        require( wp_graphics_mesh_set_indices_u32( mesh.getNativeMesh(), changedIndices, 3 ) != 0,
                 "native generation test must mutate indices without wrapper retirement" );
        fixture.renderer->renderMesh( &mesh, Matrix4F::identity() );
        require( scheduler->stats().used[gpu] == meshBaseline + expected,
                 "denied native generation replacement must keep the old cached physical buffers" );
        require( scheduler->setBudget( StreamMemoryClass::Gpu, baseline.limits[gpu], error ),
                 "native replacement candidate capacity must restore" );
        fixture.renderer->renderMesh( &mesh, Matrix4F::identity() );
        // D3D may defer resource destruction while earlier draws remain in its
        // command buffer. Submit those commands before checking retired owners.
        context->Flush();
        require( scheduler->stats().used[gpu] == meshBaseline + expected,
                 "successful generation replacement must retire old cache storage after publication" );
        ComPtr<ID3D11Buffer> retainedVertices;
        UINT stride = 0, offset = 0;
        context->IAGetVertexBuffers( 0, 1, retainedVertices.GetAddressOf(), &stride, &offset );
        require( retainedVertices != nullptr, "mesh budget test must retain a submitted native buffer" );
        mesh.unload( nullptr );
        context->ClearState();
        wp_renderer_dx11_invalidate_state( native );
        context->Flush();
        require( scheduler->stats().used[gpu] == meshBaseline + expected,
                 "retained native buffer must keep the mesh allocation lease after cache retirement" );
        const auto heldMeshBytes = scheduler->stats().used[gpu];
        retainedVertices.Reset();
        const auto queuedMeshBytes = scheduler->stats().used[gpu];
        completeGpuWork( context );
        std::printf( "Mesh physical GPU retirement: held=%llu afterRelease=%llu completed=%llu baseline=%llu\n",
            static_cast<unsigned long long>( heldMeshBytes ), static_cast<unsigned long long>( queuedMeshBytes ),
            static_cast<unsigned long long>( scheduler->stats().used[gpu] ),
            static_cast<unsigned long long>( meshBaseline ) );
        require( scheduler->stats().used[gpu] == meshBaseline,
                 "mesh GPU reservation must retire with its last physical buffer reference" );

        // Decode the ordinary raw-file path without disk or GPU allocations.
        class BitmapFiles final : public FileSystem
        {
        public:
            std::array<u8, 58> bitmap{};
            bool available = true;
            BitmapFiles()
            {
                bitmap[0] = 'B'; bitmap[1] = 'M'; bitmap[2] = 58;
                bitmap[10] = 54; bitmap[14] = 40;
                bitmap[18] = bitmap[22] = bitmap[26] = 1;
                bitmap[28] = 24; bitmap[34] = 4;
                bitmap[54] = 11; bitmap[55] = 37; bitmap[56] = 59;
            }
            SmartPtr<workphone::IStream> open( const String &name, bool, bool, bool, bool, bool ) override
            {
                if( !available ) return nullptr;
                return make_ptr<MemoryFile>( bitmap.data(), long( bitmap.size() ), name, false );
            }
        };
        const auto working = size_t( StreamMemoryClass::WorkingCpu );
        const auto decodeBaseline = scheduler->stats();
        auto previousFiles = fixture.application->getFileSystem();
        auto bitmapFiles = make_ptr<BitmapFiles>();
        fixture.application->setFileSystem( bitmapFiles );
        {
            ClawTexture encodedDenied;
            encodedDenied.setFilePath( "retry-working.bmp" );
            require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, decodeBaseline.used[working], error ),
                     "raw decode test must exhaust encoded working capacity" );
            require( !encodedDenied.getCpuBaseMip( error ),
                     "raw texture decode must reject its encoded buffer when working capacity is exhausted" );
            require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, decodeBaseline.limits[working], error ),
                     "raw decode working capacity must restore" );
            auto pixels = encodedDenied.getCpuBaseMip( error );
            require( pixels && pixels->width == 1 && pixels->bgra[1] == 37,
                     "raw decode must retry after encoded working-budget rejection without a source change" );
        }
        {
            ClawTexture sourceDenied;
            sourceDenied.setFilePath( "retry-source.bmp" );
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, decodeBaseline.used[source], error ),
                     "raw decode test must exhaust retained-source capacity" );
            require( !sourceDenied.getCpuBaseMip( error ),
                     "raw texture decode must reject retained pixels when source capacity is exhausted" );
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, decodeBaseline.limits[source], error ),
                     "raw decode source capacity must restore" );
            auto pixels = sourceDenied.getCpuBaseMip( error );
            require( pixels && pixels->width == 1 && pixels->bgra[1] == 37,
                     "raw decode must retry after source-budget rejection without a source change" );
        }
        {
            ClawTexture decoderDenied;
            decoderDenied.setFilePath( "retry-decoder-working.bmp" );
            const auto beforeDenied = scheduler->stats();
            require( scheduler->setBudget( StreamMemoryClass::WorkingCpu,
                         decodeBaseline.used[working] + bitmapFiles->bitmap.size() + 64u * 1024u, error ),
                     "decode test must admit encoded bytes/header while exhausting full bitmap capacity" );
            decoderDenied.load( nullptr );
            require( !decoderDenied.isLoaded() &&
                         scheduler->stats().used[source] == beforeDenied.used[source] &&
                         scheduler->stats().used[working] == beforeDenied.used[working],
                     "bitmap/converted payload denial must publish no false residency or leaked reservation" );
            require( scheduler->setBudget( StreamMemoryClass::WorkingCpu, decodeBaseline.limits[working], error ),
                     "decoder bitmap capacity must restore" );
            decoderDenied.load( nullptr );
            require( decoderDenied.isLoaded() && greenPixel( view( decoderDenied ) ) == 37,
                     "full decode denial must retry through ordinary load without a source change" );
        }
        {
            ClawTextureManager manager;
            manager.load( nullptr );
            completeGpuWork( context );
            const auto cacheBaseline = scheduler->stats();
            const String cacheUuid = "7d59fda4-9d40-4cd0-8e0e-59a73f0d9dbb";
            auto resource = manager.create( cacheUuid, "cache-texture.bmp" );
            auto *cached = dynamic_cast<ClawTexture *>( resource.get() );
            require( cached != nullptr, "cache contract must create a Claw texture" );
            require( cached->getHandle()->getUUIDAsString() == cacheUuid,
                     "cache identity fixture must install a valid round-trippable UUID" );
            resource->load( nullptr );
            ComPtr<ID3D11ShaderResourceView> retained = view( *cached );
            require( greenPixel( retained.Get() ) == 37, "cache source must upload expected pixels" );
            const auto resident = scheduler->stats();
            require( resident.used[source] > cacheBaseline.used[source] &&
                         resident.used[gpu] > cacheBaseline.used[gpu],
                     "decoded cache residency must participate in source and GPU budgets" );
            require( manager.releaseUnusedResources() == 0,
                     "external strong texture owners must prevent eviction" );
            WeakPtr<IResource> weak = resource;
            resource = nullptr;
            require( manager.releaseUnusedResources() == 0,
                     "weak texture clients must remain reload-safe and prevent eviction" );
            weak = nullptr;
            {
                ISharedObject::ScopedLoadLock loadLock( cached );
                require( manager.releaseUnusedResources() == 0,
                         "explicit texture load locks must prevent eviction" );
            }
            bitmapFiles->available = false;
            require( manager.releaseUnusedResources() == 0,
                     "a missing reload source must preserve the sole resident texture data" );
            bitmapFiles->available = true;
            require( manager.releaseUnusedResources( 0 ) == 0 &&
                         manager.releaseUnusedResources( 1 ) == 1 &&
                         scheduler->stats().used[source] == cacheBaseline.used[source],
                     "bounded eviction must free only unpinned reloadable source pixels" );
            require( scheduler->stats().used[gpu] == resident.used[gpu] &&
                         greenPixel( retained.Get() ) == 37,
                     "external native views must retain their physical GPU lease and pixels" );
            resource = manager.getById( cacheUuid );
            require( resource.get() == cached && !resource->isLoaded() &&
                         cached->getHandle()->getUUIDAsString() == cacheUuid &&
                         cached->getName() == "cache-texture.bmp" &&
                         cached->getFilePath() == "cache-texture.bmp" &&
                         manager.getByName( "cache-texture.bmp" ).get() == cached &&
                         manager.getTextures().size() == 1,
                     "eviction must retain the same UUID/path wrapper without creating duplicates" );
            resource->load( nullptr );
            require( view( *cached ) != retained.Get() && greenPixel( view( *cached ) ) == 37,
                     "evicted textures must lazily decode and upload through the existing wrapper" );
            std::array<u8, 4> authored{ 0, 99, 0, 255 };
            cached->copyData( authored.data(), { 1, 1 } );
            resource = nullptr;
            require( manager.releaseUnusedResources() == 0,
                     "authoring replacement pixels must remain pinned despite the original file path" );
            require( greenPixel( view( *cached ) ) == 99,
                     "a cache sweep must preserve in-memory authoring edits" );
            retained.Reset();
            manager.unload( nullptr );
            completeGpuWork( context );
            require( scheduler->stats().used[source] == cacheBaseline.used[source] &&
                         scheduler->stats().used[gpu] == cacheBaseline.used[gpu],
                     "cache fixture shutdown must release all physical texture storage" );
        }
        {
            ClawTextureManager manager;
            manager.load( nullptr );
            for( const char *path : { "bounded-a.bmp", "bounded-b.bmp", "bounded-c.bmp" } )
                manager.loadResource( path );
            require( manager.releaseUnusedResources( 1 ) == 1 &&
                         manager.releaseUnusedResources( 1 ) == 1 &&
                         manager.releaseUnusedResources( 1 ) == 1 &&
                         manager.releaseUnusedResources( 1 ) == 0,
                     "rotating bounded sweeps must eventually visit each city texture exactly once" );
            auto manual = manager.createManual( "manual.bmp", "", 0, 1, 1, 1, 0, 0 );
            auto *manualIdentity = dynamic_cast<ClawTexture *>( manual.get() );
            require( manualIdentity != nullptr, "manual texture contract must create its resource" );
            std::array<u8, 4> authored{ 0, 91, 0, 255 };
            manual->copyData( authored.data(), { 1, 1 } );
            ComPtr<ID3D11ShaderResourceView> previousManualView = view( *manualIdentity );
            const auto manualResident = scheduler->stats();
            const auto previousUsage = manual->getUsageFlags();
            const auto previousType = manualIdentity->getTextureType();
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, manualResident.used[source], error ),
                     "manual replacement test must exhaust retained source capacity" );
            require( !manager.createManual( "manual.bmp", "", u8( TextureType::TEX_TYPE_2D ),
                         2, 2, 1, 0, 0, s32( TextureUsage::TU_DYNAMIC ) ) &&
                         manual->getSize() == Vector2I( 1, 1 ) &&
                         manual->getUsageFlags() == previousUsage && manualIdentity->getTextureType() == previousType &&
                         view( *manualIdentity ) == previousManualView.Get() &&
                         greenPixel( previousManualView.Get() ) == 91 &&
                         scheduler->stats().used[source] == manualResident.used[source],
                     "denied manual source admission must preserve dimensions, metadata, pixels and native view" );
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, decodeBaseline.limits[source], error ),
                     "manual replacement source capacity must restore" );
            auto replacement = manager.createManual( "manual.bmp", "", u8( TextureType::TEX_TYPE_2D ),
                2, 2, 1, 0, 0, s32( TextureUsage::TU_DYNAMIC ) );
            require( replacement.get() == manualIdentity && replacement->getSize() == Vector2I( 2, 2 ) &&
                         replacement->getUsageFlags() == u32( TextureUsage::TU_DYNAMIC ) &&
                         description( view( *manualIdentity ) ).Width == 2 && greenPixel( view( *manualIdentity ) ) == 0,
                     "manual replacement must retry on the same identity after source capacity returns" );
            replacement = nullptr;
            previousManualView.Reset();
            manual = nullptr;
            require( manager.releaseUnusedResources() == 0,
                     "procedural textures with file-like names must never become eviction candidates" );
            manager.unload( nullptr );
        }
        fixture.application->setFileSystem( previousFiles );
        require( scheduler->stats().used[source] == decodeBaseline.used[source] &&
                     scheduler->stats().used[working] == decodeBaseline.used[working],
                 "raw decode retry fixtures must release all retained and working reservations" );
        std::puts( "Shared texture/mesh asset budget and native allocation lifetime contracts passed." );
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
        assetBudgetContracts( fixture );
        std::puts( "ClawTexture mip metadata, reload and last-good publication contracts passed." );
    }
}  // namespace claw_texture_mip_contracts
