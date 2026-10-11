#pragma once
#include "ClawFoliageContracts.hpp"
#include <WPGraphics/ClawTerrainMaterial.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>

namespace claw_terrain_material_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using namespace claw_foliage_contracts;

    inline ComPtr<ID3D11ShaderResourceView> texture( wp_renderer_dx11 *native, UINT width, UINT height,
                                                     DXGI_FORMAT format, const void *pixels,
                                                     UINT stride )
    {
        auto device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = desc.ArraySize = 1;
        desc.Format = format;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = pixels;
        data.SysMemPitch = stride;
        ComPtr<ID3D11Texture2D> image;
        ComPtr<ID3D11ShaderResourceView> view;
        require( SUCCEEDED( device->CreateTexture2D( &desc, &data, &image ) ) &&
                     SUCCEEDED( device->CreateShaderResourceView( image.Get(), nullptr, &view ) ),
                 "terrain test texture upload must succeed" );
        return view;
    }
    inline bool run( ClawRendererDX11 &renderer )
    {
        struct Scope
        {
            ClawRendererDX11 &r;
            ~Scope()
            {
                wp_renderer_dx11_end_shadow_map( wp_renderer_get_dx11( r.getNativeRenderer() ) );
                ClawTerrainMaterial::unbind( r );
                r.setCamera( nullptr );
                r.setRenderTarget( nullptr );
            }
        } scope{ renderer };
        try
        {
            auto target = make_ptr<ClawRenderTarget>();
            target->setSize( { 128, 128 } );
            renderer.setRenderTarget( target );
            renderer.setViewport( nullptr );
            renderer.setCamera( nullptr );
            auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
            setup( native );
            // set_material injects the globally bound environment. Isolate map response.
            wp_renderer_dx11_set_environment( native, nullptr, 0 );
            auto projection = identity();
            projection.m[1][1] = projection.m[2][2] = 0;
            projection.m[1][2] = projection.m[2][1] = -1; // Front-facing top-down view.
            wp_renderer_dx11_set_projection_matrix( native, &projection );
            auto mesh = geometry( native,
                                  { { { -.9f, 0, -.9f }, { 0, 1, 0 }, { 0, 0 }, 0xffffffff },
                                    { { .9f, 0, -.9f }, { 0, 1, 0 }, { 1, 0 }, 0xffffffff },
                                    { { .9f, 0, .9f }, { 0, 1, 0 }, { 1, 1 }, 0xffffffff },
                                    { { -.9f, 0, .9f }, { 0, 1, 0 }, { 0, 1 }, 0xffffffff } },
                                  { 0, 2, 1, 0, 3, 2 } );
            wp_terrain_material_dx11 terrain{};
            terrain.controls = { 1, 1, 0, 0 };
            terrain.control_transform = { .5f, .5f, .5f, .5f };
            terrain.appearance = { 0, 4, 0, .2f };
            terrain.detail = { 4, 4, 1, 0 };
            terrain.macro = { .01f, 0, 0, 0 };
            terrain.macro_tint = { .5f, .5f, .5f, 1 };
            terrain.weather = { 0, .35f, .12f, 0 };
            terrain.snow = { .8f, 0, 0, 0 };
            terrain.snow_tint = { 1, 1, 1, 1 };
            for( int i = 0; i < 4; ++i )
            {
                terrain.tint[i] = { 1, 1, 1, 1 };
                terrain.surface[i] = { 1, 1, 1, 1 };
                terrain.maps[i] = { 1, 0, 0, 1 };
                terrain.height[i] = { 1, 0, 0, 0 };
            }
            const unsigned colours[4] = { 0xff0000ff, 0xff00ff00, 0xffff0000,
                                          0xff00ffff };  // RGBA byte order.
            ComPtr<ID3D11ShaderResourceView> maps[WP_TERRAIN_MATERIAL_VIEWS];
            void *views[WP_TERRAIN_MATERIAL_VIEWS]{};
            for( int i = 0; i < 4; ++i )
            {
                maps[i * 3] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &colours[i], 4 );
                views[i * 3] = maps[i * 3].Get();
            }
            auto ordinary = material( { 1, 1, 1, 1 } );
            auto draw = [&]() {
                require( wp_renderer_dx11_set_terrain_material( native, &terrain, views ),
                         "complete terrain material must bind" );
                wp_renderer_dx11_set_material( native, &ordinary );
                wp_renderer_dx11_set_clear_color( native, 0, 0, 0, 1 );
                wp_renderer_dx11_clear( native, WORKPHONE_CLEAR_FLAG_ALL );
                wp_renderer_dx11_draw_geometry_pntc( native, mesh.get(), 0, 6, 0 );
                return pixels( native );
            };
            auto weight = [&]( wp_vec4f value ) {
                maps[12] = texture( native, 1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, &value, 16 );
                views[12] = maps[12].Get();
            };
            const size_t center = ( 64 * 128 + 64 ) * 4;
            for( int layer = 0; layer < 4; ++layer )
            {
                wp_vec4f weights{};
                reinterpret_cast<float *>( &weights )[layer] = 1;
                weight( weights );
                auto image = draw();
                for( int channel = 0; channel < 3; ++channel )
                    require( std::abs( int( image[center + channel] ) -
                                       int( ( colours[layer] >> ( channel * 8 ) ) & 255 ) ) <= 2,
                             "all four actual layer albedo textures must independently reach pixels" );
            }
            weight( { 1, 0, 0, 0 } );
            const auto red = draw();
            weight( { 0, 0, 0, 0 } );
            equivalent( red, draw(), "zero-sum terrain controls must choose layer zero" );
            weight( { 1, 1, 0, 0 } );
            const auto normalized = draw();
            weight( { 8, 8, 0, 0 } );
            equivalent( normalized, draw(), "shader must normalize unnormalized positive controls" );
            require(
                normalized[center] > 175 && normalized[center + 1] > 175 && normalized[center + 2] < 3,
                "albedo blending must happen in linear colour space" );
            weight( { 1, 0, 0, 0 } );
            ordinary.uv_transform.z = 1;
            ordinary.ambient_color = { 1, 1, 1, 1 };
            ordinary.camera_position = { 0, 3, 0, 0 };
            ordinary.specular_color = { .04f, .04f, .04f, 0 };
            terrain.surface[0].w = 0;
            const auto lit = draw();
            const unsigned normalPixel =
                0xff80ffff;  // encoded tangent normal with a strong positive X/Y tilt.
            maps[1] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &normalPixel, 4 );
            views[1] = maps[1].Get();
            terrain.maps[0].y = 1;
            const auto normal = draw();
            std::printf( "Terrain normal response: base=%u mapped=%u\n", unsigned( lit[center] ),
                         unsigned( normal[center] ) );
            require( std::abs( int( lit[center] ) - int( normal[center] ) ) > 4,
                     "normal map must visibly alter terrain lighting" );
            terrain.maps[0].y = 0;
            const unsigned darkOrm = 0xff00ff00;  // occlusion=0, roughness=1, metalness=0.
            maps[2] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &darkOrm, 4 );
            views[2] = maps[2].Get();
            terrain.maps[0].z = 1;
            const auto occluded = draw();
            require( occluded[center] + 30 < lit[center],
                     "ORM occlusion must reduce ambient terrain light" );
            const unsigned metalOrm = 0xffffffff;
            maps[2] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &metalOrm, 4 );
            views[2] = maps[2].Get();
            terrain.surface[0].w = 1;
            const auto metallic = draw();
            std::printf( "Terrain isolated ambient response: lit=%u AO=%u metallic=%u\n",
                         unsigned( lit[center] ), unsigned( occluded[center] ), unsigned( metallic[center] ) );
            require( metallic[center] + 30 < lit[center],
                     "ORM metallic must suppress dielectric diffuse" );
            // Direct specular response proves the green ORM channel is roughness.
            terrain.surface[0].w = 0;
            ordinary.ambient_color = { 0, 0, 0, 1 };
            ordinary.light_color = { 1, 1, 1, 2 };
            ordinary.light_direction = { 0, -1, 0, 0 };
            const unsigned smoothOrm = 0xff001aff, roughOrm = 0xff00f0ff;
            maps[2] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &smoothOrm, 4 );
            views[2] = maps[2].Get();
            const auto smooth = draw();
            maps[2] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &roughOrm, 4 );
            views[2] = maps[2].Get();
            const auto rough = draw();
            std::printf( "Terrain ORM response: lit=%u AO=%u metal=%u smoothG=%u roughG=%u\n",
                         unsigned( lit[center] ), unsigned( occluded[center] ),
                         unsigned( metallic[center] ), unsigned( smooth[center + 1] ),
                         unsigned( rough[center + 1] ) );
            require( std::abs( int( smooth[center + 1] ) - int( rough[center + 1] ) ) > 10,
                     "ORM roughness must visibly change the dielectric highlight" );

            // Terrain uses the same actual depth-pass coverage and PCF receiving as meshes.
            require( wp_renderer_dx11_begin_shadow_map( native, &projection, 128 ),
                     "terrain shadow map must begin" );
            wp_renderer_dx11_set_depth_test_enabled( native, 1 );
            wp_renderer_dx11_set_depth_write_enabled( native, 1 );
            wp_renderer_dx11_set_depth_func( native, WORKPHONE_DEPTH_FUNC_LESS );
            wp_renderer_dx11_draw_geometry_pntc( native, mesh.get(), 0, 6, 0 );
            require( shadowDepth( native, 64, 64 ) < .51f,
                     "layered terrain must write actual shadow depth" );
            wp_renderer_dx11_end_shadow_map( native );
            wp_renderer_dx11_set_depth_test_enabled( native, 0 );
            wp_renderer_dx11_set_depth_write_enabled( native, 0 );
            auto receiver = identity();
            receiver.m[1][3] = -.3f;
            wp_renderer_dx11_set_world_matrix( native, &receiver );
            wp_renderer_dx11_enable_shadow_receiving( native, 0 );
            const auto unshadowed = draw();
            wp_renderer_dx11_enable_shadow_receiving( native, 1 );
            const auto shadowed = draw();
            std::printf( "Terrain shadow response: unshadowed=%u shadowed=%u\n",
                         unsigned( unshadowed[center] ), unsigned( shadowed[center] ) );
            require( shadowed[center] + 20 < unshadowed[center],
                     "layered terrain must receive its actual shadow map" );
            wp_renderer_dx11_enable_shadow_receiving( native, 0 );
            const auto unit = identity();
            wp_renderer_dx11_set_world_matrix( native, &unit );
            ordinary.ambient_color = { 1, 1, 1, 1 };
            ordinary.light_color = {};
            terrain.surface[0].w = 0;
            terrain.maps[0].z = 0;
            ordinary.fog_params = { 3, 0, 0, 1 };
            ordinary.fog_color = { 0, 0, 1, 1 };
            const auto fogged = draw();
            require( fogged[center + 2] > fogged[center] + 50,
                     "terrain must preserve shared fog rendering" );
            ordinary.fog_params = {};
            ordinary.uv_transform.z = 0;
            const unsigned stripePixels[] = { 0xff0000ff, 0xffff0000 };
            maps[0] = texture( native, 2, 1, DXGI_FORMAT_R8G8B8A8_UNORM, stripePixels, 8 );
            views[0] = maps[0].Get();
            terrain.surface[0].x = 1;
            const auto singleRepeat = draw();
            terrain.surface[0].x = 3;
            const auto repeated = draw();
            size_t uvChanges = 0;
            for( size_t i = 0; i < repeated.size(); i += 4 )
                uvChanges += std::abs( int( repeated[i] ) - int( singleRepeat[i] ) ) > 20;
            require( uvChanges > 1000, "local-metre UV scale must visibly change texture repetition" );
            auto left = geometry( native,
                                  { { { -.9f, 0, -.9f }, { 0, 1, 0 }, { 0, 0 }, 0xffffffff },
                                    { { 0, 0, -.9f }, { 0, 1, 0 }, { 1, 0 }, 0xffffffff },
                                    { { 0, 0, .9f }, { 0, 1, 0 }, { 1, 1 }, 0xffffffff },
                                    { { -.9f, 0, .9f }, { 0, 1, 0 }, { 0, 1 }, 0xffffffff } },
                                  { 0, 2, 1, 0, 3, 2 } );
            auto right = geometry( native,
                                   { { { 0, 0, -.9f }, { 0, 1, 0 }, { 0, 0 }, 0xffffffff },
                                     { { .9f, 0, -.9f }, { 0, 1, 0 }, { 1, 0 }, 0xffffffff },
                                     { { .9f, 0, .9f }, { 0, 1, 0 }, { 1, 1 }, 0xffffffff },
                                     { { 0, 0, .9f }, { 0, 1, 0 }, { 0, 1 }, 0xffffffff } },
                                   { 0, 2, 1, 0, 3, 2 } );
            wp_renderer_dx11_clear( native, WORKPHONE_CLEAR_FLAG_ALL );
            wp_renderer_dx11_draw_geometry_pntc( native, left.get(), 0, 6, 0 );
            wp_renderer_dx11_draw_geometry_pntc( native, right.get(), 0, 6, 0 );
            equivalent( repeated, pixels( native ),
                        "tile-local mesh UV resets must not create terrain layer seams" );
            // A vertical face has constant planar X and must gain real projected
            // texture variation from the triplanar side projection.
            auto originalMesh = std::move( mesh );
            mesh = geometry( native,
                { { { 0, -.9f, -.9f }, { 1, 0, 0 }, { 0, 0 }, 0xffffffff },
                  { { 0, -.9f, .9f }, { 1, 0, 0 }, { 1, 0 }, 0xffffffff },
                  { { 0, .9f, .9f }, { 1, 0, 0 }, { 1, 1 }, 0xffffffff },
                  { { 0, .9f, -.9f }, { 1, 0, 0 }, { 0, 1 }, 0xffffffff } }, { 0, 1, 2, 0, 2, 3 } );
            auto sideProjection = identity();
            sideProjection.m[0][0] = sideProjection.m[2][2] = 0;
            sideProjection.m[0][2] = 1;
            sideProjection.m[2][0] = -1;
            wp_renderer_dx11_set_projection_matrix( native, &sideProjection );
            const auto planarSide = draw();
            terrain.appearance.x = 1;
            const auto triplanarSide = draw();
            size_t projectionChanges = 0;
            for( size_t i = 0; i < triplanarSide.size(); i += 4 )
                projectionChanges += std::abs( int( triplanarSide[i] ) - int( planarSide[i] ) ) > 20;
            require( projectionChanges > 1000,
                     "triplanar side projection must remove planar stretching on a steep face" );
            terrain.appearance.x = 0;
            mesh = std::move( originalMesh );
            wp_renderer_dx11_set_projection_matrix( native, &projection );
            maps[0] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &colours[0], 4 );
            views[0] = maps[0].Get();
            terrain.surface[0].x = 1;
            const unsigned lowHeight = 0x0000ffff, highHeight = 0xff00ffff;
            maps[2] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &lowHeight, 4 );
            maps[5] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &highHeight, 4 );
            views[2] = maps[2].Get(); views[5] = maps[5].Get();
            terrain.maps[0].z = terrain.maps[1].z = 1;
            weight( { 1, 1, 0, 0 } );
            const auto ordinaryBlend = draw();
            terrain.appearance.z = 1;
            const auto heightBlend = draw();
            require( ordinaryBlend[center] > 175 && ordinaryBlend[center+1] > 175 &&
                         heightBlend[center] < 3 && heightBlend[center+1] > 250,
                     "ORM alpha heights must select the higher painted layer in actual pixels" );
            weight( { 0, 0, 0, 0 } );
            const auto heightFallback = draw();
            require( heightFallback[center] > 250 && heightFallback[center+1] < 3,
                     "height blending must preserve the zero-weight layer-zero fallback" );
            terrain.appearance.z = 0;
            terrain.maps[0].z = terrain.maps[1].z = 0;
            weight( { 1, 0, 0, 0 } );
            ordinary.uv_transform.z = 1;
            const auto withoutDetail = draw();
            maps[13] = texture( native, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &normalPixel, 4 );
            views[13] = maps[13].Get();
            terrain.detail.w = 1;
            const auto detailed = draw();
            require( std::abs( int( detailed[center] ) - int( withoutDetail[center] ) ) > 4,
                     "independent detail normal must change terrain lighting" );
            terrain.detail.w = 0;
            ordinary.uv_transform.z = 0;
            const auto withoutMacro = draw();
            terrain.macro = { 3, 1, 12345, 54321 };
            terrain.macro_tint = { .1f, .1f, .1f, 1 };
            const auto macro = draw();
            equivalent( macro, draw(), "macro variation must be deterministic for the same local positions and seed" );
            terrain.macro.z += 1;
            const auto otherSeed = draw();
            size_t macroChanges = 0, seedChanges = 0;
            for( size_t i = 0; i < macro.size(); i += 4 )
            {
                macroChanges += std::abs( int( macro[i] ) - int( withoutMacro[i] ) ) > 20;
                seedChanges += std::abs( int( macro[i] ) - int( otherSeed[i] ) ) > 10;
            }
            require( macroChanges > 1000 && seedChanges > 1000,
                     "macro tint and seed must produce visible low-frequency material variation" );
            terrain.macro.y = 0;
            const wp_vec2f surfaceMasks[] = { { 1, 0 }, { 0, 1 } };
            maps[14] = texture( native, 2, 1, DXGI_FORMAT_R32G32_FLOAT, surfaceMasks, sizeof(surfaceMasks) );
            views[14] = maps[14].Get();
            terrain.snow.y = 1;
            terrain.weather = { 1, .8f, .12f, 1 };
            const auto weather = draw();
            const size_t leftPixel = (64 * 128 + 24) * 4, rightPixel = (64 * 128 + 104) * 4;
            require( weather[leftPixel] < 200 && weather[leftPixel+1] < 3 &&
                         weather[rightPixel] > 250 && weather[rightPixel+1] > 250 && weather[rightPixel+2] > 250,
                     "wetness and snow sample masks must independently control dark wet ground and opaque snow" );
            terrain.weather = { 0, 0, .1f, 0 }; terrain.snow.y = 0;
            ordinary.uv_transform.z = 1;
            ordinary.ambient_color = { 0, 0, 0, 1 };
            ordinary.light_color = { 1, 1, 1, 2 };
            terrain.surface[0].z = 1;
            const auto dry = draw();
            terrain.weather.x = 1;
            const auto wet = draw();
            require( wet[center+1] > dry[center+1] + 10,
                     "wetness must visibly lower PBR roughness rather than only recolour the surface" );
            terrain.weather.x = 0;
            ordinary.uv_transform.z = 0;
            std::printf( "Terrain appearance: triplanar pixels=%zu macro=%zu seed=%zu; detail=%u->%u; wet G=%u->%u\n",
                projectionChanges,macroChanges,seedChanges,unsigned(withoutDetail[center]),unsigned(detailed[center]),
                unsigned(dry[center+1]),unsigned(wet[center+1]) );
            const auto beforeInvalid = draw();
            auto invalid = terrain;
            invalid.surface[0].x = 0;
            require( !wp_renderer_dx11_set_terrain_material( native, &invalid, views ),
                     "invalid native material must reject atomically" );
            for( int channel = 0; channel < 4; ++channel )
            {
                invalid = terrain;
                reinterpret_cast<float *>( &invalid.tint[0] )[channel] = 1.01f;
                require( !wp_renderer_dx11_validate_terrain_material( native, &invalid, views ) &&
                             !wp_renderer_dx11_set_terrain_material( native, &invalid, views ),
                         "native terrain tint must match the shared zero-to-one RGBA contract" );
            }
            for( int field = 0; field < 6; ++field )
            {
                auto invalid = terrain;
                if( field == 0 ) invalid.appearance.x = 2;
                if( field == 1 ) invalid.appearance.w = 0;
                if( field == 2 ) invalid.weather.x = 1.1f;
                if( field == 3 ) invalid.macro.y = -1;
                if( field == 4 ) invalid.snow.x = .01f;
                if( field == 5 ) invalid.height[0].x = 5;
                require( !wp_renderer_dx11_validate_terrain_material( native, &invalid, views ) &&
                             !wp_renderer_dx11_set_terrain_material( native, &invalid, views ),
                         "invalid native terrain appearance must reject transactionally" );
            }
            wp_renderer_dx11_clear( native, WORKPHONE_CLEAR_FLAG_ALL );
            wp_renderer_dx11_draw_geometry_pntc( native, mesh.get(), 0, 6, 0 );
            equivalent( beforeInvalid, pixels( native ),
                        "failed material update must retain previous complete binding" );
            ClawTerrainMaterial owner;
            String error;
            auto data = std::make_shared<TerrainData>();
            data->layers[0].tint = { 0, 1, 0, 1 };
            auto first = owner.stage( renderer, data, error );
            require( first && owner.publish( renderer, first, error ),
                     "terrain material candidate must publish" );
            auto failed = std::make_shared<TerrainData>( *data );
            failed->layers[0].albedo = "missing-layer";
            require( !owner.stage( renderer, failed, error,
                                   []( const String &, String &e ) -> SmartPtr<ITexture> {
                                       e = "missing fixture texture";
                                       return {};
                                   } ) &&
                         owner.snapshot() == data,
                     "missing layer must retain complete previous material revision" );
            failed = std::make_shared<TerrainData>( *data );
            failed->appearance.detailNormal = "missing-detail";
            require( !owner.stage( renderer, failed, error,
                                   []( const String &, String &e ) -> SmartPtr<ITexture> {
                                       e = "missing detail fixture texture"; return {};
                                   } ) && owner.snapshot() == data,
                     "missing detail normal must retain the complete previous material publication" );
            auto stale = owner.stage( renderer, data, error );
            auto newest = owner.stage( renderer, data, error );
            require(
                !owner.publish( renderer, stale, error ) && owner.publish( renderer, newest, error ),
                "stale staged terrain material must not publish" );
            require( owner.bind( renderer, error ), "retained terrain material must bind" );
            auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
            const auto boundTerrainViews = [&]() {
                ID3D11ShaderResourceView *bound[WP_TERRAIN_MATERIAL_VIEWS]{};
                context->PSGetShaderResources( 9, WP_TERRAIN_MATERIAL_VIEWS, bound );
                size_t count = 0;
                for( auto view : bound ) if( view ) { ++count; view->Release(); }
                return count;
            };
            require( boundTerrainViews() > 0, "actual terrain draw must bind views to the DX11 context" );
            ClawTerrainMaterial::unbind( renderer );
            require( boundTerrainViews() == 0,
                     "terrain unbind must immediately release all context SRVs before ownership leases retire" );
            {
                const auto scheduler = WorldStreamingScheduler::shared();
                const auto before = scheduler->stats();
                constexpr auto gpu = size_t( StreamMemoryClass::Gpu );
                struct Restore
                {
                    std::shared_ptr<WorldStreamingScheduler> scheduler;
                    u64 limit;
                    ~Restore() { String ignored; scheduler->setBudget( StreamMemoryClass::Gpu, limit, ignored ); }
                } restore{ scheduler, before.limits[gpu] };
                ClawTerrainMaterial budgetOwner;
                auto painted = std::make_shared<TerrainData>( *data );
                painted->layerWeights.assign( 4, Vector4F( 1, 0, 0, 0 ) );
                painted->surfaceMasks.assign( 4, Vector2F( 1, 1 ) );
                auto paintedCandidate = budgetOwner.stage( renderer, painted, error );
                require( paintedCandidate && budgetOwner.publish( renderer, paintedCandidate, error ) &&
                             budgetOwner.ownedGpuBytes() == 96 && scheduler->stats().used[gpu] == before.used[gpu] + 96,
                         "control and weather maps must reserve their owned GPU bytes before upload" );
                require( scheduler->setBudget( StreamMemoryClass::Gpu, before.used[gpu] + 96, error ),
                         "live material GPU budget can be capped at its retained bytes" );
                require( !budgetOwner.stage( renderer, painted, error ) && budgetOwner.snapshot() == painted &&
                             scheduler->stats().used[gpu] == before.used[gpu] + 96,
                         "GPU admission failure must retain the previous complete material and budget" );
                budgetOwner.clear();
                require( scheduler->stats().used[gpu] == before.used[gpu] + 96,
                         "retained material candidates must retain their map allocation budget" );
                paintedCandidate.reset();
                require( scheduler->stats().used[gpu] == before.used[gpu],
                         "last map candidate destruction must return its GPU budget" );
            }
            ordinary = material( { 0, 0, 1, 1 } );
            wp_renderer_dx11_set_material( native, &ordinary );
            wp_renderer_dx11_clear( native, WORKPHONE_CLEAR_FLAG_ALL );
            wp_renderer_dx11_draw_geometry_pntc( native, mesh.get(), 0, 6, 0 );
            const auto restored = pixels( native );
            require( restored[center + 2] > 250 && restored[center] < 3,
                     "ordinary meshes must not inherit terrain layer state after unbind" );
            std::puts(
                "Terrain PBR layers: 4 albedos, normalized/zero controls, linear blending, normals, ORM "
                "AO/metallic/roughness, cast/receive shadows, fog, local-metre UV/tile seams, "
                "height blend/triplanar/detail/macro/wetness/snow, stale/failure retention PASS" );
            return true;
        }
        catch( const std::exception &e )
        {
            std::fprintf( stderr, "Terrain material contract failed: %s\n", e.what() );
            return false;
        }
    }
}  // namespace claw_terrain_material_contracts
