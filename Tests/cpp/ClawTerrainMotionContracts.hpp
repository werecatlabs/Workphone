#pragma once
#include "ClawTerrainContracts.hpp"
#include <WorkphoneGraphics/workphone_graphics_scenenode.h>
#include <algorithm>
#include <chrono>
#include <set>
#include <thread>

namespace claw_terrain_motion_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }
    inline void bounded( const ClawTerrainResidency &state, u32 limit )
    {
        require( state.residentTiles <= limit && state.stagedTiles <= limit &&
                     state.residentBytes <= 8 * 1024 * 1024 && state.stagedBytes <= 8 * 1024 * 1024,
                 "moving view must retain bounded active and staged GPU generations" );
        require( state.worker.pending <= 256 && state.worker.active <= 1 && state.worker.ready <= 8 &&
                     state.worker.readyBytes <= 8 * 1024 * 1024,
                 "moving view must retain bounded CPU work and ready geometry" );
    }
    inline std::set<int> columns( const SmartPtr<ClawTerrain> &terrain )
    {
        std::set<int> result;
        for( auto *mesh : terrain->getNativeRenderTiles() )
        {
            const auto *vertices = static_cast<const wp_graphics_mesh_vertex_pntc *>(
                wp_graphics_mesh_get_vertices( mesh ) );
            float x = std::numeric_limits<float>::max();
            for( u32 i = 0; i < wp_graphics_mesh_get_vertex_count( mesh ); ++i )
                x = std::min( x, vertices[i].position.x );
            result.insert( int( std::lround( x / terrainTileCells ) ) );
        }
        return result;
    }
    inline bool run( ClawRendererDX11 &renderer )
    {
        struct Scope
        {
            ClawRendererDX11 &renderer;
            bool frame = false;
            ~Scope()
            {
                if( frame )
                    renderer.endRender();
                renderer.setCamera( nullptr );
                renderer.setRenderTarget( nullptr );
            }
        } scope{ renderer };
        try
        {
            auto target = make_ptr<ClawRenderTarget>();
            target->setSize( { 64, 64 } );
            renderer.setRenderTarget( target );
            renderer.setViewport( nullptr );
            auto terrain = make_ptr<ClawTerrain>();
            String error;
            require( terrain->setTerrainResidencyLimits( 16, 8 * 1024 * 1024, 1, error ),
                     "moving view budget must initialize" );
            TerrainData data;
            data.dimensions = { 257, 257 };
            data.origin = { 0, 0 };
            data.heights.assign( 257 * 257, 0 );
            require( terrain->applyTerrainData( data, error ), "moving view source must initialize" );
            TerrainTileView left;
            left.cull = false;
            left.localCamera = { 1, 1000, 1 };
            auto right = left;
            right.localCamera = { 255, 1000, 255 };
            renderer.beginRender();
            scope.frame = true;
            terrain->prepareRenderTiles( renderer, { left }, error );
            require( error.empty(), error.c_str() );
            const auto generation = terrain->getTerrainResidency().worker.generation;
            const auto firstDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            unsigned calls = 0;
            do
            {
                terrain->prepareRenderTiles( renderer, { ( ++calls & 1 ) ? right : left }, error );
                require( error.empty(), error.c_str() );
                const auto state = terrain->getTerrainResidency();
                bounded( state, 16 );
                require( state.worker.generation == generation,
                         "priority order changes must not cancel identical tile membership" );
                require( state.stagedTiles <= 1,
                         "multiple preparations in one renderer frame share one upload allowance" );
                if( state.stagedTiles == 1 )
                    break;
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            } while( std::chrono::steady_clock::now() < firstDeadline );
            require( terrain->getTerrainResidency().stagedTiles == 1,
                     "first moving view tile must upload within timeout" );
            const auto firstBytes = terrain->getTerrainResidency().uploadedBytes;
            for( unsigned i = 0; i < 20; ++i )
            {
                terrain->prepareRenderTiles( renderer, { ( i & 1 ) ? right : left }, error );
                require( error.empty() && terrain->getTerrainResidency().uploadedBytes == firstBytes,
                         "additional views cannot multiply a frame's GPU upload budget" );
            }
            renderer.endRender();
            scope.frame = false;
            const auto completeDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            bool complete = false;
            while( !complete && std::chrono::steady_clock::now() < completeDeadline )
            {
                complete =
                    terrain->prepareRenderTiles( renderer, { ( ++calls & 1 ) ? right : left }, error );
                require( error.empty(), error.c_str() );
                const auto state = terrain->getTerrainResidency();
                bounded( state, 16 );
                require( state.worker.generation == generation,
                         "continuous camera motion must not starve a stable demand" );
                if( !complete )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }
            auto resident = terrain->getTerrainResidency();
            require( complete && resident.residentTiles == 16 &&
                         resident.uploadedBytes == resident.residentBytes,
                     "stable membership under camera motion uploads each tile exactly once" );
            auto camera = make_ptr<ClawCamera>();
            auto cameraNode = std::unique_ptr<wp_scenenode, decltype( &wp_scenenode_destroy )>(
                wp_scenenode_create(), &wp_scenenode_destroy );
            require( cameraNode != nullptr, "moving camera scene node must initialize" );
            wp_camera_attach_to_node( camera->getNativeCamera(), cameraNode.get() );
            camera->setFOVy( 1.04719755f );
            camera->setAspectRatio( 1 );
            camera->setNearClipDistance( .1f );
            camera->setFarClipDistance( 1000 );
            wp_camera_set_position( camera->getNativeCamera(), { 128, 350, 128 } );
            wp_camera_look_at( camera->getNativeCamera(), { 128, 0, 128 }, { 0, 0, -1 } );
            renderer.setCamera( camera );
            renderer.disableShadows();
            renderer.setSceneLighting( ColourF::White, { 0, -1, 0 }, ColourF::White, 0 );
            renderer.clear( ColourF::Black );
            renderer.renderTerrain( terrain, false );
            const auto visibleGreen = claw_terrain_contracts::greenPixel(
                wp_renderer_get_dx11( renderer.getNativeRenderer() ), 32, 32 );
            const auto centreInView = camera->getViewMatrix().transformAffine( Vector3<real_Num>( 128, 0, 128 ) );
            std::printf( "Terrain moving camera: centre view=(%.4f,%.4f,%.4f), green=%d, tiles=%u\n",
                double( centreInView.x ), double( centreInView.y ), double( centreInView.z ), visibleGreen, resident.residentTiles );
            require( std::abs( centreInView.x ) < .001 && std::abs( centreInView.y ) < .001 && centreInView.z < -349,
                     "attached moving camera must centre its requested terrain point" );
            require( visibleGreen > 30,
                     "moving-view generation must produce visible GPU terrain" );
            wp_camera_attach_to_node( camera->getNativeCamera(), nullptr );
            terrain->unload( nullptr );

            auto strip = make_ptr<ClawTerrain>();
            require( strip->setTerrainResidencyLimits( 3, 8 * 1024 * 1024, 1, error ),
                     "overlap budget must initialize" );
            data.dimensions = { 257, 65 };
            data.heights.assign( 257 * 65, 0 );
            require( strip->applyTerrainData( data, error ), "overlap source must initialize" );
            left.localCamera = { 96, 1000, 32 };
            right.localCamera = { 224, 1000, 32 };
            const auto finish = [&]( const TerrainTileView &view ) {
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
                bool done = false;
                while( !done && std::chrono::steady_clock::now() < deadline )
                {
                    done = strip->prepareRenderTiles( renderer, { view }, error );
                    require( error.empty(), error.c_str() );
                    bounded( strip->getTerrainResidency(), 3 );
                    if( !done )
                        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
                require( done, "changed view membership must finish within timeout" );
            };
            finish( left );
            const auto previous = strip->getRenderedTerrainSnapshot();
            require( columns( strip ) == std::set<int>{ 0, 1, 2 },
                     "left camera must publish the three nearest strip columns" );
            const auto beforeEdit = strip->getTerrainResidency().uploadedBytes;
            data.heights.assign( 257 * 65, 1 );
            require( strip->applyTerrainData( data, error ),
                     "replacement strip source must initialize" );
            const auto overlapDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            do
            {
                strip->prepareRenderTiles( renderer, { left }, error );
                require( error.empty(), error.c_str() );
                if( strip->getTerrainResidency().stagedTiles )
                    break;
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            } while( std::chrono::steady_clock::now() < overlapDeadline );
            require( strip->getTerrainResidency().stagedTiles == 1 &&
                         strip->getRenderedTerrainSnapshot() == previous,
                     "partial replacement must retain the complete old publication" );
            strip->prepareRenderTiles( renderer, { right }, error );
            require( error.empty() && strip->getTerrainResidency().stagedTiles >= 1 &&
                         strip->getRenderedTerrainSnapshot() == previous,
                     "changed demand preserves already uploaded overlapping tiles and the old active "
                     "source" );
            finish( right );
            resident = strip->getTerrainResidency();
            require( columns( strip ) == std::set<int>{ 1, 2, 3 } &&
                         strip->getRenderedTerrainSnapshot() != previous &&
                         resident.uploadedBytes - beforeEdit == resident.residentBytes,
                     "overlapping replacement tiles publish once without redundant GPU upload" );
            // A view-only, equal-count change must compare resident keys, not just counts.
            finish( left );
            require( columns( strip ) == std::set<int>{ 0, 1, 2 },
                     "same-size changed membership must replace actual resident keys" );
            require( !strip->prepareRenderTiles( renderer, {}, error ) &&
                         strip->getTerrainResidencyError() == error && !error.empty(),
                     "invalid preparation must expose its retained diagnostic" );
            finish( left );
            require( strip->getTerrainResidencyError().empty(),
                     "successful preparation clears prior diagnostic" );
            strip->unload( nullptr );
            std::puts(
                "Claw terrain camera motion, overlapping demand, per-frame upload bounds and visible "
                "publication passed." );
            return true;
        }
        catch( const std::exception &e )
        {
            std::fprintf( stderr, "Claw terrain camera motion failed: %s\n", e.what() );
            return false;
        }
    }
}  // namespace claw_terrain_motion_contracts
