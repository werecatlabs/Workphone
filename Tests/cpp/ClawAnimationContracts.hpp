#pragma once

#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace claw_animation_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using Microsoft::WRL::ComPtr;

    inline void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    class SkinFixture : public ClawMesh
    {
    public:
        SkinFixture()
        {
            m_mesh = wp_graphics_mesh_create();
            const wp_graphics_mesh_vertex_pnt vertices[] = {
                { { -0.2f, -0.2f, 0.5f }, { 0, 0, -1 }, { 0, 0 } },
                { { 0.2f, -0.2f, 0.5f }, { 0, 0, -1 }, { 1, 0 } },
                { { 0, 0.2f, 0.5f }, { 0, 0, -1 }, { 0.5f, 1 } } };
            const wp_u16 indices[] = { 0, 1, 2 };
            require( m_mesh && wp_graphics_mesh_set_vertices( m_mesh, WORKPHONE_VERTEX_FORMAT_PNT, vertices, 3 ) &&
                     wp_graphics_mesh_set_indices_u16( m_mesh, indices, 3 ), "clone fixture geometry allocation" );
            require( wp_graphics_mesh_add_submesh( m_mesh, 0, 3, 42 ) >= 0, "clone fixture material range" );
            Array<wp_skin_vertex> skin( 3 );
            for( unsigned i = 0; i < 3; ++i )
            {
                std::memset( &skin[i], 0, sizeof( skin[i] ) );
                skin[i].position = vertices[i].position;
                skin[i].normal = vertices[i].normal;
                skin[i].weights[0] = 1;
            }
            require( setSkinningData( skin ), "clone fixture valid bind geometry" );
            setMaterialName( "fixture-submesh", 0 );
        }
    };

    inline Array<wp_mat4f> palette( float x )
    {
        Array<wp_mat4f> value( 1 );
        std::memset( value.data(), 0, sizeof( wp_mat4f ) );
        for( unsigned i = 0; i < 4; ++i ) value[0].m[i][i] = 1;
        value[0].m[0][3] = x;
        return value;
    }

    inline float firstX( const ClawMesh &mesh )
    {
        return static_cast<const wp_graphics_mesh_vertex_pnt *>(
            wp_graphics_mesh_get_vertices( mesh.getNativeMesh() ) )[0].position.x;
    }

    inline int greenPixel( wp_renderer_dx11 *native, unsigned x, unsigned y )
    {
        auto device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
        auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
        ComPtr<ID3D11RenderTargetView> view;
        context->OMGetRenderTargets( 1, view.GetAddressOf(), nullptr );
        require( view != nullptr, "animation readback requires the render target" );
        ComPtr<ID3D11Resource> resource;
        view->GetResource( resource.GetAddressOf() );
        ComPtr<ID3D11Texture2D> source;
        require( SUCCEEDED( resource.As( &source ) ), "animation readback texture" );
        D3D11_TEXTURE2D_DESC desc{};
        source->GetDesc( &desc );
        require( x < desc.Width && y < desc.Height &&
                 ( desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM ),
                 "animation RGBA8 readback dimensions" );
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, staging.GetAddressOf() ) ), "animation readback allocation" );
        context->CopyResource( staging.Get(), source.Get() );
        D3D11_MAPPED_SUBRESOURCE mapped{};
        require( SUCCEEDED( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &mapped ) ), "animation readback map" );
        const auto result = static_cast<const unsigned char *>( mapped.pData )[y * mapped.RowPitch + x * 4 + 1];
        context->Unmap( staging.Get(), 0 );
        return result;
    }

    inline bool run( ClawRendererDX11 &renderer )
    {
        struct TargetScope
        {
            ClawRendererDX11 &renderer;
            ~TargetScope() { renderer.setCamera( nullptr ); renderer.setRenderTarget( nullptr ); }
        } scope{ renderer };
        try
        {
            auto original = make_ptr<SkinFixture>();
            require( original->applySkinningPalette( palette( 0.5f ) ), "original deformation" );
            auto clone = dynamic_pointer_cast<ClawMesh>( original->clone( "independent-character" ) );
            require( clone && clone->hasSkinningData() && clone->getNativeMesh() != original->getNativeMesh(),
                     "clone retains bind skinning and owns independent native geometry" );
            require( std::abs( firstX( *clone ) - 0.3f ) < 1e-5f &&
                     clone->getMaterialName( 0 ) == "fixture-submesh" &&
                     wp_graphics_mesh_get_index_format( clone->getNativeMesh() ) == WORKPHONE_INDEX_FORMAT_UINT16 &&
                     wp_graphics_mesh_get_submesh_count( clone->getNativeMesh() ) == 1 &&
                     wp_graphics_mesh_get_submesh( clone->getNativeMesh(), 0 )->material_id == 42,
                     "clone preserves current deformation, topology and material range" );
            require( clone->applySkinningPalette( palette( -0.5f ) ) &&
                     std::abs( firstX( *clone ) + 0.7f ) < 1e-5f &&
                     std::abs( firstX( *original ) - 0.3f ) < 1e-5f,
                     "clone samples immutable bind vertices, independently of source output" );
            auto target = make_ptr<ClawRenderTarget>();
            target->setSize( { 64, 64 } );
            renderer.setRenderTarget( target );
            renderer.setViewport( nullptr );
            renderer.setCamera( nullptr );
            renderer.disableShadows();
            renderer.setSceneLighting( ColourF::White, Vector3F( 0, -1, 0 ), ColourF::White, 0 );
            auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
            renderer.clear( ColourF::Black );
            renderer.renderMesh( original.get(), Matrix4F::identity() );
            renderer.renderMesh( clone.get(), Matrix4F::identity() );
            require( greenPixel( native, 16, 32 ) > 30 && greenPixel( native, 48, 32 ) > 30 &&
                     greenPixel( native, 32, 32 ) < 10,
                     "DX11 independently draws both deformation outputs" );
            original->unload( nullptr );
            original = nullptr;
            require( clone->applySkinningPalette( palette( 0 ) ) && std::abs( firstX( *clone ) + 0.2f ) < 1e-5f,
                     "clone retains shared immutable bind data after original unload" );
            renderer.clear( ColourF::Black );
            renderer.renderMesh( clone.get(), Matrix4F::identity() );
            require( greenPixel( native, 32, 32 ) > 30 && greenPixel( native, 16, 32 ) < 10,
                     "surviving clone refreshes its own DX11 deformation cache" );
            clone->unload( nullptr );
            std::puts( "Claw animation clone bind/output ownership and independent DX11 deformation: PASS" );
            return true;
        }
        catch( const std::exception &error )
        {
            std::fprintf( stderr, "Claw animation: FAIL: %s\n", error.what() );
            return false;
        }
    }
}
