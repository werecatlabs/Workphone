#pragma once

#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <WorkphoneGraphics/workphone_graphics_camera.h>
#include <WorkphoneGraphics/workphone_graphics_mesh.h>
#include <WorkphoneGraphics/workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace claw_terrain_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using Microsoft::WRL::ComPtr;

    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }

    inline bool terrainNear( real_Num a, real_Num b )
    {
        return std::abs( a - b ) < real_Num( 1.e-5 );
    }

    struct MeshManagerScope
    {
        SmartPtr<core::IApplicationManager> application = core::IApplicationManager::instance();
        SmartPtr<IResourceManager> previous;
        SmartPtr<MeshManager> owned;
        SmartPtr<IFactoryManager> previousFactory;
        SmartPtr<FactoryManager> ownedFactory;

        MeshManagerScope()
        {
            require( application != nullptr, "terrain mesh fixture needs the application services" );
            previousFactory = application->getFactoryManager();
            if( !previousFactory )
            {
                ownedFactory = make_ptr<FactoryManager>();
                application->setFactoryManager( ownedFactory );
            }
            previous = application->getMeshManager();
            if( !previous )
            {
                owned = make_ptr<MeshManager>();
                application->setMeshManager( owned );
            }
        }
        ~MeshManagerScope()
        {
            // The test calls have already destroyed every terrain and CPU mesh.
            // Keep the manager bound until its own retained mesh resources retire.
            if( owned )
            {
                owned->unload( nullptr );
                application->setMeshManager( previous );
            }
            if( ownedFactory )
                application->setFactoryManager( previousFactory );
        }
    };

    class TerrainFixture : public ClawTerrain
    {
    public:
        wp_graphics_mesh *cachedMesh() const
        {
            return m_renderMesh;
        }
    };

    inline TerrainData rectangularData()
    {
        TerrainData data;
        data.dimensions = { 3, 5 };
        data.spacing = { 0.4f, 0.2f };
        data.origin = { -0.4f, -0.4f };
        data.heights.assign( 15, 0.0f );
        return data;
    }

    inline void testGeometry()
    {
        auto terrain = make_ptr<TerrainFixture>();
        auto data = rectangularData();
        data.heightScale = 0.25f;
        data.heights[4] = 4.0f;
        String error;
        require( terrain->applyTerrainData( data, error ), "rectangular terrain must publish" );
        auto *mesh = terrain->getNativeRenderMesh();
        require( mesh && wp_graphics_mesh_get_vertex_count( mesh ) == 15 &&
                     wp_graphics_mesh_get_index_count( mesh ) == 48 &&
                     wp_graphics_mesh_get_submesh_count( mesh ) == 1,
                 "native terrain must contain every rectangular sample and triangle" );
        require( wp_graphics_mesh_get_vertex_format( mesh ) == WORKPHONE_VERTEX_FORMAT_PNTC &&
                     wp_graphics_mesh_get_index_format( mesh ) == WORKPHONE_INDEX_FORMAT_UINT32,
                 "terrain must expose native PNTC vertices and u32 indices" );
        const auto *vertices =
            static_cast<const wp_graphics_mesh_vertex_pntc *>( wp_graphics_mesh_get_vertices( mesh ) );
        const auto *indices = static_cast<const u32 *>( wp_graphics_mesh_get_indices( mesh ) );
        require(
            terrainNear( vertices[0].position.x, -0.4 ) && terrainNear( vertices[0].position.z, -0.4 ) &&
                terrainNear( vertices[14].position.x, 0.4 ) &&
                terrainNear( vertices[14].position.z, 0.4 ) && terrainNear( vertices[4].position.y, 1 ),
            "native geometry must use exact origin, independent spacing and height scale" );
        const u32 expected[] = { 0, 3, 1, 1, 3, 4 };
        for( size_t i = 0; i < 6; ++i )
            require( indices[i] == expected[i],
                     "native terrain must use the shared top-right/bottom-left diagonal" );
        const auto cpuMesh = terrain->getMesh();
        require( cpuMesh != nullptr, "Claw terrain must provide its shared CPU mesh" );
        const auto cpuPoints = MeshUtil::getPoints( cpuMesh );
        const auto cpuIndices = MeshUtil::getIndices( cpuMesh );
        require( cpuPoints.size() == 15 && cpuIndices.size() == 48,
                 "CPU and native terrain topology must have equal dimensions" );
        for( size_t i = 0; i < cpuPoints.size(); ++i )
        {
            require( terrainNear( cpuPoints[i].x, vertices[i].position.x ) &&
                         terrainNear( cpuPoints[i].y, vertices[i].position.y ) &&
                         terrainNear( cpuPoints[i].z, vertices[i].position.z ),
                     "CPU and native terrain positions must agree" );
            const auto &normal = vertices[i].normal;
            require( terrainNear( normal.x * normal.x + normal.y * normal.y + normal.z * normal.z, 1 ),
                     "native terrain normals must be finite unit vectors" );
        }
        for( size_t i = 0; i < cpuIndices.size(); ++i )
            require( cpuIndices[i] == indices[i], "CPU and native terrain triangles must agree" );

        Transform3<real_Num> transform;
        transform.setPosition( { 1, 2, 3 } );
        transform.setScale( { 2, 3, 4 } );
        transform.setOrientation( Quaternion<real_Num>::eulerDegrees( 0, 90, 0 ) );
        terrain->setWorldTransform( transform );
        const auto surface = transform.getPosition() +
                             transform.getOrientation() * Vector3<real_Num>( -0.2f, 1.5f, -1.0f );
        require( terrainNear( terrain->getHeightAtWorldPosition( surface ), 3.5 ),
                 "terrain queries must sample rendered triangles under yaw and nonuniform scale" );
        const Ray3F ray( { f32( surface.x ), 13.5f, f32( surface.z ) }, { 0, -1, 0 } );
        const auto hit = terrain->intersects( ray );
        require( hit && hit->hasIntersected() && terrainNear( hit->getPosition().y, 3.5 ),
                 "terrain picking must hit the same rendered triangle as the height query" );
        require( terrain->getNativeRenderMesh() == mesh,
                 "world placement changes must reuse local native geometry" );

        const auto retained = terrain->getTerrainSnapshot();
        auto invalid = data;
        invalid.heights[4] = std::numeric_limits<f32>::quiet_NaN();
        require( !terrain->applyTerrainData( invalid, error ) &&
                     terrain->getTerrainSnapshot() == retained && terrain->getNativeRenderMesh() == mesh,
                 "invalid terrain edits must retain the source and last-good native mesh" );
        invalid = data;
        invalid.heights.pop_back();
        require( !terrain->applyTerrainData( invalid, error ) && terrain->getNativeRenderMesh() == mesh,
                 "mismatched rectangular sample counts must preserve native geometry" );

        terrain->setHeightScale( 0 );
        mesh = terrain->getNativeRenderMesh();
        vertices =
            static_cast<const wp_graphics_mesh_vertex_pntc *>( wp_graphics_mesh_get_vertices( mesh ) );
        require( vertices && terrainNear( vertices[4].position.y, 0 ),
                 "zero height scale must flatten native geometry without a fallback to one" );
        data.dimensions = { 259, 3 };
        data.spacing = { 1, 1 };
        data.origin = { -129, -1 };
        data.heightScale = 1;
        data.heights.assign( 259 * 3, 0.0f );
        data.heights[259 + 64] = 7;
        require( terrain->applyTerrainData( data, error ), "wide rectangular terrain must publish" );
        mesh = terrain->getNativeRenderMesh();
        require( mesh && wp_graphics_mesh_get_vertex_count( mesh ) == 259 * 3 &&
                     wp_graphics_mesh_get_index_count( mesh ) == 258 * 2 * 6,
                 "native terrain must retain source resolution beyond the old 257-sample cap" );
        vertices =
            static_cast<const wp_graphics_mesh_vertex_pntc *>( wp_graphics_mesh_get_vertices( mesh ) );
        require( terrainNear( vertices[259 + 64].position.y, 7 ),
                 "native terrain must retain a sample skipped by the old resampling path" );
        require( terrain->getNativeRenderMesh() == mesh,
                 "unchanged terrain must reuse its native mesh" );
        terrain->unload( nullptr );
        require( !terrain->cachedMesh(), "terrain unload must retire its native mesh" );
    }

    inline int greenPixel( wp_renderer_dx11 *native, int x, int y )
    {
        auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
        ComPtr<ID3D11RenderTargetView> view;
        context->OMGetRenderTargets( 1, view.GetAddressOf(), nullptr );
        require( view != nullptr, "terrain readback requires a bound render target" );
        ComPtr<ID3D11Resource> resource;
        view->GetResource( resource.GetAddressOf() );
        ComPtr<ID3D11Texture2D> source;
        require( SUCCEEDED( resource.As( &source ) ), "terrain target must be a texture" );
        D3D11_TEXTURE2D_DESC desc{};
        source->GetDesc( &desc );
        require( desc.Width > u32( x ) && desc.Height > u32( y ) &&
                     ( desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM ||
                       desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM ),
                 "terrain target must expose bounded RGBA8 pixels" );
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, staging.GetAddressOf() ) ),
                 "terrain readback allocation must succeed" );
        context->CopyResource( staging.Get(), source.Get() );
        D3D11_MAPPED_SUBRESOURCE mapped{};
        require( SUCCEEDED( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &mapped ) ),
                 "terrain pixels must be readable" );
        const auto value =
            *( static_cast<const unsigned char *>( mapped.pData ) + y * mapped.RowPitch + x * 4 + 1 );
        context->Unmap( staging.Get(), 0 );
        return value;
    }

    struct RenderScope
    {
        ClawRendererDX11 &renderer;
        ~RenderScope()
        {
            renderer.setCamera( nullptr );
            renderer.setRenderTarget( nullptr );
        }
    };

    inline void testDrawAndEdit( ClawRendererDX11 &renderer )
    {
        RenderScope scope{ renderer };
        auto terrain = make_ptr<ClawTerrain>();
        auto data = rectangularData();
        String error;
        require( terrain->applyTerrainData( data, error ) && terrain->isVisible(),
                 "standalone terrain must accept geometry and expose visibility" );
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 64, 64 } );
        renderer.setRenderTarget( target );
        renderer.setViewport( nullptr );
        auto camera = make_ptr<ClawCamera>();
        camera->setFOVy( 1.04719755f );
        camera->setNearClipDistance( 0.1f );
        camera->setFarClipDistance( 10 );
        camera->setAspectRatio( 1 );
        wp_camera_set_position( camera->getNativeCamera(), { 0, 3, 0 } );
        wp_camera_look_at( camera->getNativeCamera(), { 0, 0, 0 }, { 0, 0, -1 } );
        renderer.setCamera( camera );
        renderer.disableShadows();
        renderer.setSceneLighting( ColourF::White, Vector3F( 0, -1, 0 ), ColourF::White, 0 );
        auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        renderer.clear( ColourF::Black );
        renderer.renderTerrain( terrain );
        const auto view = camera->getViewMatrix();
        const auto projection = camera->getProjectionMatrix();
        // The standalone native look-at path currently maps negative Y toward
        // this camera. Verify the actual eye-space depths before relying on it:
        // at 64 pixels and FOV 60, half-width 0.4 covers 7.4 then 14.8 pixels.
        const f32 editedHeight = -1.5f;
        require( terrainNear( -view[2][3], 3 ) &&
                     terrainNear( -( view[2][1] * editedHeight + view[2][3] ), 1.5 ) &&
                     terrainNear( projection[0][0], 1.7320508 ),
                 "terrain fixture must place the edited surface closer to its camera" );
        require( greenPixel( native, 32, 32 ) > 30 && greenPixel( native, 44, 32 ) < 10,
                 "flat rectangular terrain must draw its expected DX11 footprint" );
        data.heights.assign( 15, editedHeight );
        require( terrain->applyTerrainData( data, error ), "a height edit must publish" );
        renderer.clear( ColourF::Black );
        renderer.renderTerrain( terrain );
        const auto editedPixel = greenPixel( native, 44, 32 );
        require( editedPixel > 30,
                 "height edits must replace cached native/GPU geometry and change visible coverage" );
        const auto revision = terrain->getTerrainRevision();
        data.heights[0] = std::numeric_limits<f32>::infinity();
        require( !terrain->applyTerrainData( data, error ) && terrain->getTerrainRevision() == revision,
                 "rejected edits must keep the published terrain revision" );
        renderer.clear( ColourF::Black );
        renderer.renderTerrain( terrain );
        require( greenPixel( native, 44, 32 ) == editedPixel,
                 "rejected terrain edits must retain the last-good DX11 result" );
        std::printf( "Claw terrain DX11 edited footprint: green=%d\n", editedPixel );
        terrain->unload( nullptr );
    }

    inline bool run( ClawRendererDX11 &renderer )
    {
        try
        {
            MeshManagerScope meshManager;
            testGeometry();
            testDrawAndEdit( renderer );
            std::puts(
                "Claw terrain rectangular geometry, query/picking, revision and DX11 edit contracts "
                "passed." );
            return true;
        }
        catch( const std::exception &error )
        {
            std::fprintf( stderr, "Claw terrain contract failed: %s\n", error.what() );
            return false;
        }
    }
}  // namespace claw_terrain_contracts
