#pragma once

#include "CookedTerrainContracts.hpp"
#include "DX11TestEvidence.hpp"

namespace terrain_package_contracts
{
    using namespace cooked_terrain_contracts;

    // This entry point initializes only graphics/application services. No compiler
    // registry, source mount, catalog, SQLite plugin or compilation DB is created.
    inline void consume( const fs::path &package )
    {
        claw_texture_mip_contracts::Fixture graphics;
        graphics.initialize();
        claw_terrain_contracts::MeshManagerScope meshes;
        auto &renderer = *graphics.renderer;
        String error;
        ResourceSystem runtime( nullptr, nullptr );
        RuntimeResourceConfig mount;
        mount.compiledRoot = path( package );
        mount.manifestPath = path( package / "terrain.wprs" );
        mount.target = "terrain-dx11-contract";
        require( runtime.initializeRuntime( mount, error ), error );
        auto loaded = runtime.load( ResourceID( "data://ground.terrain" ), error );
        require( loaded && loaded->dependencies.size() == 13,
                 "Package must contain the pinned thirteen-texture closure: " + error );
        auto decoded = decodeCookedTerrainResource( loaded, mount.target, true, error );
        require( decoded != nullptr, error );
        auto terrain = make_ptr<ClawTerrain>();
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 128, 128 } );
        auto camera = make_ptr<ClawCamera>();
        camera->setFOVy( 60 );
        camera->setNearClipDistance( .1f );
        camera->setFarClipDistance( 10 );
        camera->setAspectRatio( 1 );
        wp_camera_set_position( camera->getNativeCamera(), { 0, 3, 0 } );
        wp_camera_look_at( camera->getNativeCamera(), { 0, 0, 0 }, { 0, 0, -1 } );

        const auto draw = [&]() {
            renderer.setRenderTarget( target );
            renderer.setViewport( nullptr );
            renderer.setCamera( camera );
            renderer.disableShadows();
            renderer.setSceneLighting( ColourF::White, { 0, -1, 0 }, ColourF::White, 0 );
            renderer.setSceneFog( 0, ColourF::Black, 0, 0, 1 );
            renderer.clear( ColourF::Black );
            renderer.renderTerrain( terrain );
            return claw_foliage_contracts::pixels( wp_renderer_get_dx11( renderer.getNativeRenderer() ) );
        };
        auto candidate = stageCookedTerrainMaterial( renderer, decoded, error );
        require( candidate && terrain->applyCookedTerrain( candidate, error ), error );
        auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        require( recordDX11TestDevice( static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) ) ),
                 "Package consumer must identify its actual device" );
        const auto pixels = draw();
        require( pixels.size() == 128 * 128 * 4 && terrain->getTerrainResidency().usedCookedTileManifest,
                 "Package must render using its validated cooked tile manifest" );
        size_t colours[4]{};
        for( size_t p = 0; p < pixels.size(); p += 4 )
        {
            const int r = pixels[p], g = pixels[p + 1], b = pixels[p + 2];
            colours[0] += r > g + 35 && r > b + 35;
            colours[1] += g > r + 35 && g > b + 35;
            colours[2] += b > r + 35 && b > g + 35;
            colours[3] += r > 80 && g > 80 && b + 35 < std::min( r, g );
        }
        for( auto count : colours ) require( count > 25, "All four packaged layers must reach DX11 pixels" );
        // A machine-readable colour check and retained image accompany process evidence.
        std::ofstream imageFile( package / "terrain.ppm", std::ios::binary );
        imageFile << "P6\n128 128\n255\n";
        for( size_t p = 0; p < pixels.size(); p += 4 )
            imageFile.write( reinterpret_cast<const char *>( pixels.data() + p ), 3 );
        require( imageFile.good(), "Package capture must be writable" );
        imageFile.close();
        const auto revision = terrain->getTerrainRevision();
        auto corrupt = std::make_shared<RuntimeResource>( *loaded );
        corrupt->payload.back() ^= 1;
        require( !stageCookedTerrainMaterial( renderer, corrupt, mount.target, true, error ) &&
                     terrain->getTerrainRevision() == revision,
                 "Corrupt replacement must preserve the published package terrain" );
        require( !stageCookedTerrainMaterial( renderer, loaded, "wrong-target", true, error ),
                 "Wrong-target terrain must reject in the standalone consumer" );
        corrupt.reset();
        candidate.reset();
        runtime.clearRuntimeCache();
        loaded.reset();
        claw_foliage_contracts::equivalent( pixels, draw(), "Package terrain must outlive runtime cache handles" );
        terrain->unload( nullptr );
        renderer.setCamera( nullptr );
        renderer.setRenderTarget( nullptr );
        target = nullptr;
        renderer.unload( nullptr );
        renderer.load( graphics.window );
        require( renderer.isLoaded(), "Package consumer renderer must recreate" );
        candidate = stageCookedTerrainMaterial( renderer, decoded, error );
        require( candidate && terrain->applyCookedTerrain( candidate, error ), error );
        target = make_ptr<ClawRenderTarget>();
        target->setSize( { 128, 128 } );
        claw_foliage_contracts::equivalent( pixels, draw(), "Retained cooked data must restore package pixels after recreation" );
        terrain->unload( nullptr );
        renderer.setCamera( nullptr );
        renderer.setRenderTarget( nullptr );
        runtime.shutdown();
        std::printf( "PASS: standalone source-free terrain package; 13 pinned textures; red=%zu green=%zu blue=%zu yellow=%zu; failure retention, cache release and device recreation\n",
                     colours[0], colours[1], colours[2], colours[3] );
    }
}
