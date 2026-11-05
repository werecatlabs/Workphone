#pragma once

#include "ClawFoliageAcceptance.hpp"
#include "CookedFoliageContracts.hpp"
#include <Workphone/Graphics/TerrainData.hpp>

namespace claw_installed_foliage_contracts
{
    using namespace claw_foliage_acceptance;
    using namespace workphone::resource;
    namespace fs = std::filesystem;

    inline void exportForest( const fs::path &source )
    {
        require( fs::create_directories( source ), "Installed forest requires a new source directory" );
        const auto leaf = leafPixels();
        std::array<u8, 18> header{};
        header[2] = 2;
        header[12] = u8( leaf.width );
        header[14] = u8( leaf.height );
        header[16] = 32;
        header[17] = 0x28;
        {
            std::ofstream image( source / "leaf.tga", std::ios::binary );
            image.write( reinterpret_cast<const char *>( header.data() ), header.size() );
            image.write( reinterpret_cast<const char *>( leaf.bgra.data() ), leaf.bgra.size() );
            require( bool( image ), "Installed forest must write the authored cutout texture" );
        }
        cooked_foliage_contracts::text( source / "leaf.texres",
            R"({"version":1,"source":"data://leaf.tga","mipFilter":"cutout","atlasColumns":1,"alphaCutoff":0.5})" );

        TerrainData ground;
        ground.dimensions = { 129, 129 };
        ground.origin = { -256, -256 };
        ground.spacing = { 4, 4 };
        ground.heights.clear();
        ground.heights.reserve( size_t( ground.dimensions.x ) * ground.dimensions.y );
        for( s32 z = -256; z <= 256; z += 4 )
            for( s32 x = -256; x <= 256; x += 4 )
                ground.heights.push_back( 2.f * std::sin( x * .018f ) * std::cos( z * .016f ) );
        const auto terrain = serializeTerrainData( ground );
        require( !terrain.empty(), "Installed forest must serialize its matching terrain" );
        cooked_foliage_contracts::text( source / "ground.terrain", terrain );

        String error;
        if( !writeFoliageSources( forest( "data://leaf.texres" ), cooked_foliage_contracts::path( source ),
                                  ResourceID( "data://forest.folworld" ), error ) )
            throw std::runtime_error( error.c_str() );
    }

    inline void consumeForest( const fs::path &package, const String &uuid, const fs::path &captures )
    {
        claw_texture_mip_contracts::Fixture graphics;
        graphics.initialize();
        auto &renderer = *graphics.renderer;
        auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        require( native && recordDX11TestDevice( static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) ) ),
                 "Installed forest consumer must identify its actual DX11 device" );
        auto runtime = std::make_shared<ResourceSystem>( nullptr, nullptr );
        RuntimeResourceConfig config;
        config.compiledRoot = cooked_foliage_contracts::path( package );
        config.manifestPath = cooked_foliage_contracts::path( package / "runtime.wprm" );
        config.target = "pc";
        String error;
        ResourceID id;
        if( !runtime->initializeRuntime( config, error ) || !runtime->resolveRuntimeUUID( uuid, id, error ) )
            throw std::runtime_error( error.c_str() );
        const auto decoded = decodeCookedFoliageWorld( runtime, id, config.target, true, error );
        if( !decoded ) throw std::runtime_error( error.c_str() );
        require( decoded->source.snapshot->instances.empty() && decoded->source.cells.size() == cells * cells &&
                     decoded->source.snapshot->species.size() == 3,
                 "Installed forest must start with a shallow 64-cell mixed-species directory" );
        u64 instances = 0;
        for( const auto count : decoded->source.pageInstanceCounts )
        {
            require( count == treesPerCell + grassPerCell, "Installed page count differs from authoring" );
            instances += count;
        }
        std::printf( "FOLIAGE_INSTALLED,%zu,%llu\n", decoded->source.cells.size(),
                     static_cast<unsigned long long>( instances ) );

        ClawFoliageWorld world;
        const auto stage = [&] {
            auto candidate = stageCookedFoliageWorld( renderer, decoded, error );
            if( !candidate || !world.setSource( candidate->source, candidate->resolver, error ) )
                throw std::runtime_error( error.c_str() );
            return candidate;
        };
        auto candidate = stage();
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { s32( width ), s32( height ) } );
        auto camera = make_ptr<Camera>();
        flight( world, renderer, camera, target, 65, 0, captures );

        // Recreate the real renderer and reload the installed GPU textures on its new device.
        world.clear();
        candidate.reset();
        renderer.setRenderTarget( nullptr );
        renderer.setCamera( nullptr );
        renderer.unload( nullptr );
        renderer.load( graphics.window );
        require( renderer.isLoaded(), "Installed forest recovery must recreate the DX11 renderer" );
        candidate = stage();
        target = make_ptr<ClawRenderTarget>();
        target->setSize( { s32( width ), s32( height ) } );
        renderer.setRenderTarget( target );
        renderer.setViewport( nullptr );
        renderer.setCamera( camera );
        renderer.setSceneLighting( ColourF::White, { 0, -1, 0 }, ColourF::White, 0 );
        native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        const auto view = camera->configure( 1, { -165.f, 3.5f, -190.f }, { -145.f, .8f, -95.f } );
        waitForPages( world, renderer, { view }, 0 );
        renderer.clear( ColourF::Black );
        require( renderPass( world, renderer, native, view ).actualDraws &&
                     capture( native, captures / "recovered.bmp" ) > 100,
                 "Recreated DX11 device must draw the installed foliage" );
        world.clear();
        candidate.reset();
        renderer.setRenderTarget( nullptr );
        renderer.setCamera( nullptr );
        runtime->shutdown();
        std::puts( "PASS: installed representative foliage, paged flight and renderer recovery" );
    }
}
