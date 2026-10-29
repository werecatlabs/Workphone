#ifndef WPClawTerrainStreamingContracts_h__
#define WPClawTerrainStreamingContracts_h__
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Interface/Graphics/ITerrainRayResult.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <workphone_graphics_renderer.h>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <chrono>
#include <thread>
#include <stdexcept>

namespace claw_terrain_streaming_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }
    inline bool run( ClawRendererDX11 &renderer )
    {
        try
        {
            const auto scheduler = WorldStreamingScheduler::shared();
            constexpr auto gpuClass = size_t( StreamMemoryClass::Gpu );
            const auto initialBudget = scheduler->stats();
            const auto initialStrings = CharacterPoolAllocator<char>::stats();
            struct RestoreBudget
            {
                std::shared_ptr<WorldStreamingScheduler> scheduler;
                StreamMemoryClass category;
                u64 limit;
                ~RestoreBudget()
                {
                    String ignored;
                    scheduler->setBudget( category, limit, ignored );
                }
            } restoreGpuBudget{ scheduler, StreamMemoryClass::Gpu, initialBudget.limits[gpuClass] },
              restoreSourceBudget{ scheduler, StreamMemoryClass::SourceCpu,
                                   initialBudget.limits[size_t( StreamMemoryClass::SourceCpu )] };
            auto terrain = make_ptr<ClawTerrain>();
            const Ray3F queryRay( Vector3F( 0, 100, 0 ), Vector3F( 0, -1, 0 ) );
            require( !terrain->getActiveTerrainSnapshot() &&
                         !terrain->intersects( queryRay )->hasIntersected() && !terrain->getMesh() &&
                         !terrain->getNativeRenderMesh() &&
                         terrain->getTerrainSpacePosition( Vector3<real_Num>( 1, 2, 3 ) ) == Vector3<real_Num>::zero(),
                     "new renderer exposes no phantom query or mesh surface before its first GPU publication" );
            String error;
            require( terrain->setTerrainResidencyLimits( 4, 8 * 1024 * 1024, 1, error ),
                     "terrain streaming budgets accepted" );
            TerrainData data;
            data.dimensions = { 129, 129 };
            data.origin = { 0, 0 };
            data.heights.assign( 129 * 129, 0 );
            require( terrain->applyTerrainData( data, error ), "stream source accepted" );
            TerrainTileView view;
            view.cull = false;
            view.localCamera = { 64, 10000, 64 };
            const auto prepare = [&] {
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
                bool ready = false;
                while( !ready && std::chrono::steady_clock::now() < deadline )
                {
                    const auto before = terrain->getTerrainResidency();
                    ready = terrain->prepareRenderTiles( renderer, { view }, error );
                    const auto after = terrain->getTerrainResidency();
                    require( error.empty(), error.c_str() );
                    require( after.residentTiles <= 4 && after.stagedTiles <= 4 &&
                                 after.residentBytes <= 8 * 1024 * 1024 &&
                                 after.stagedBytes <= 8 * 1024 * 1024,
                             "GPU tile residency stays inside active and staged budgets" );
                    require( after.uploadedBytes - before.uploadedBytes < 512 * 1024,
                             "one tile per update bounds GPU upload work" );
                    if( !ready )
                        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
                require( ready, "terrain tiles become resident within timeout" );
            };
            const auto coldStart=std::chrono::steady_clock::now();
            prepare();
            const auto coldMs=std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-coldStart).count();
            auto first = terrain->getRenderedTerrainSnapshot();
            require( first && terrain->getNativeRenderTiles().size() == 4,
                     "complete four-tile generation is published" );
            require( terrain->getActiveTerrainSnapshot() == first,
                     "runtime queries use the complete rendered generation" );
            require( scheduler->stats().used[gpuClass] - initialBudget.used[gpuClass] ==
                         terrain->getTerrainResidency().residentBytes +
                         terrain->terrainMaterial().ownedGpuBytes(),
                     "shared GPU accounting follows actual terrain buffers and owned maps once" );
            const auto warmBytes = terrain->getTerrainResidency().uploadedBytes;
            const auto warmStart=std::chrono::steady_clock::now();
            for( u32 i = 0; i < 10; ++i )
                require( terrain->prepareRenderTiles( renderer, { view }, error ),
                         "warm terrain remains ready" );
            require( terrain->getTerrainResidency().uploadedBytes == warmBytes,
                     "unchanged terrain does not reupload geometry" );
            const auto warmMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - warmStart ).count();
            auto *nativeRenderer = wp_renderer_get_dx11( renderer.getNativeRenderer() );
            wp_render_statistics_dx11 beforeDraw{}, firstDraw{}, afterPeer{};
            wp_renderer_dx11_get_statistics( nativeRenderer, &beforeDraw );
            terrain->setVisible( true );
            renderer.renderTerrain( terrain, false );
            wp_renderer_dx11_get_statistics( nativeRenderer, &firstDraw );
            auto *context = static_cast<ID3D11DeviceContext *>(
                wp_renderer_dx11_get_context( nativeRenderer ) );
            ID3D11Buffer *boundVertex = nullptr, *boundIndex = nullptr;
            UINT stride = 0, offset = 0;
            DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
            context->IAGetVertexBuffers( 0, 1, &boundVertex, &stride, &offset );
            context->IAGetIndexBuffer( &boundIndex, &indexFormat, &offset );
            const bool terrainBuffersUnbound = !boundVertex && !boundIndex;
            if( boundVertex ) boundVertex->Release();
            if( boundIndex ) boundIndex->Release();
            require( terrainBuffersUnbound,
                     "completed terrain draw releases context buffer references before retirement" );
            require( firstDraw.draws > beforeDraw.draws &&
                         firstDraw.geometry_creations == beforeDraw.geometry_creations,
                     "published tiles draw from their prepared GPU allocations" );
            {
                ClawRendererDX11 independentRenderer;
            }
            renderer.renderTerrain( terrain, false );
            wp_renderer_dx11_get_statistics( nativeRenderer, &afterPeer );
            require( afterPeer.draws > firstDraw.draws &&
                         afterPeer.geometry_creations == firstDraw.geometry_creations,
                     "destroying another renderer cannot discard live terrain buffers or force unbudgeted uploads" );
            std::printf("Terrain GPU residency: 129x129, four tiles, cold %.3f ms, ten warm prepares %.3f ms, resident %llu bytes, warm upload 0 bytes\n",
                coldMs,warmMs,
                static_cast<unsigned long long>(terrain->getTerrainResidency().residentBytes));
            data.heights[65 * 129 + 65] = 10;
            require( scheduler->setBudget( StreamMemoryClass::Gpu,
                                          scheduler->stats().used[gpuClass], error ),
                     "shared GPU budget can be fixed at the currently retained generation" );
            auto otherTerrain = make_ptr<ClawTerrain>();
            TerrainData competingData;
            competingData.dimensions = { 2, 2 };
            competingData.heights.assign( 4, 0 );
            require( otherTerrain->applyTerrainData( competingData, error ),
                     "single-tile competing terrain source accepted" );
            {
                const auto budgets = scheduler->stats();
                const auto working = size_t( StreamMemoryClass::WorkingCpu );
                auto heldWork = scheduler->reserve( StreamMemoryClass::WorkingCpu,
                    budgets.limits[working] - budgets.used[working], error );
                require( heldWork && !otherTerrain->prepareRenderTiles( renderer, { view }, error ) &&
                             error.find( "CPU working storage" ) != String::npos &&
                             !otherTerrain->getActiveTerrainSnapshot(),
                         "synchronous single-tile preparation respects shared CPU admission" );
            }
            require( !otherTerrain->prepareRenderTiles( renderer, { view }, error ) &&
                         !error.empty() && !otherTerrain->getActiveTerrainSnapshot(),
                     "another terrain cannot bypass the shared GPU admission budget" );
            otherTerrain->unload( nullptr );
            require( terrain->applyTerrainData( data, error ), "streamed terrain edit accepted" );
            const auto admissionDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            do
            {
                terrain->prepareRenderTiles( renderer, { view }, error );
                if( !error.empty() )
                    break;
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            } while( std::chrono::steady_clock::now() < admissionDeadline );
            require( !error.empty() && terrain->getRenderedTerrainSnapshot() == first &&
                         terrain->getActiveTerrainSnapshot() == first,
                     "shared GPU exhaustion preserves complete active query/render terrain" );
            require( scheduler->setBudget( StreamMemoryClass::Gpu,
                                          initialBudget.limits[gpuClass], error ),
                     "shared GPU budget restored for replacement admission" );
            terrain->prepareRenderTiles( renderer, { view }, error );
            require( terrain->getRenderedTerrainSnapshot() == first &&
                         terrain->getActiveTerrainSnapshot() == first,
                     "queries and GPU retain the same old generation while replacement is partial" );
            prepare();
            auto second = terrain->getRenderedTerrainSnapshot();
            require( second != first && second->heights[65 * 129 + 65] == 10,
                     "all replacement tiles publish the current edited source" );
            require( terrain->getNativeRenderMesh() != nullptr,
                     "solid active generation provides a cached reference mesh" );
            data.holes.assign( 128 * 128, 1 );
            require( terrain->applyTerrainData( data, error ), "all-hole terrain edit accepted" );
            prepare();
            second = terrain->getRenderedTerrainSnapshot();
            require( terrain->getNativeRenderTiles().empty() &&
                         !terrain->getNativeRenderMesh() && !terrain->getNativeRenderMesh() &&
                         !terrain->intersects( queryRay )->hasIntersected(),
                     "valid empty generation retires cached solid geometry across repeated queries" );
            require( terrain->getTerrainResidency().residentBytes == 0 &&
                         scheduler->stats().used[gpuClass] == initialBudget.used[gpuClass] +
                             terrain->terrainMaterial().ownedGpuBytes(),
                     "all-hole generation retains no phantom geometry reservation" );
            data.holes.clear();
            require( terrain->applyTerrainData( data, error ), "solid terrain restoration accepted" );
            prepare();
            second = terrain->getRenderedTerrainSnapshot();
            require( terrain->getNativeRenderMesh() && terrain->getNativeRenderTiles().size() == 4,
                     "a later solid generation rebuilds reference and tiled geometry" );
            data.layers[0].albedo = "missing-terrain-texture-for-retention-contract";
            require( terrain->applyTerrainData( data, error ),
                     "valid descriptor can stage unresolved dependency" );
            const auto failureDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            do
            {
                terrain->prepareRenderTiles( renderer, { view }, error );
                if( !error.empty() )
                    break;
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            } while( std::chrono::steady_clock::now() < failureDeadline );
            require( !error.empty() && terrain->getRenderedTerrainSnapshot() == second &&
                         terrain->terrainMaterial().snapshot() == second &&
                         terrain->getActiveTerrainSnapshot() == second,
                     "failed material upload leaves old complete tile and material generation" );
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu,
                                          scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )],
                                          error ),
                     "source budget fixed at active plus pending retained data" );
            data.layers[0].albedo.clear();
            data.heights[3] += 1;
            require( terrain->applyTerrainData( data, error ), "new edit supersedes failed staged terrain" );
            const auto supersededDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            bool supersededReady = false;
            while( !supersededReady && std::chrono::steady_clock::now() < supersededDeadline )
            {
                supersededReady = terrain->prepareRenderTiles( renderer, { view }, error );
                if( !supersededReady )
                {
                    require( terrain->getRenderedTerrainSnapshot() == second &&
                                 terrain->getActiveTerrainSnapshot() == second,
                             "source admission retries preserve the complete active terrain" );
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                }
            }
            if( !supersededReady )
            {
                const auto sourceClass = size_t( StreamMemoryClass::SourceCpu );
                const auto state = scheduler->stats();
                const auto residency = terrain->getTerrainResidency();
                std::fprintf( stderr,
                    "Terrain superseded admission: error='%s' sourceUsed=%llu sourceLimit=%llu "
                    "sourceRejected=%llu requestedSourceBytes=%llu activeRevision=%llu "
                    "requestedRevision=%llu staged=%u demand=%u pending=%u active=%u ready=%u\n",
                    error.c_str(), static_cast<unsigned long long>( state.used[sourceClass] ),
                    static_cast<unsigned long long>( state.limits[sourceClass] ),
                    static_cast<unsigned long long>( state.rejectedReservations[sourceClass] ),
                    static_cast<unsigned long long>( terrainSourceBytes( *terrain->getTerrainSnapshot() ) ),
                    static_cast<unsigned long long>( residency.activeRevision ),
                    static_cast<unsigned long long>( residency.requestedRevision ),
                    residency.stagedTiles, residency.requestedTiles, residency.worker.pending,
                    residency.worker.active, residency.worker.ready );
            }
            require( supersededReady && error.empty() &&
                         terrain->getRenderedTerrainSnapshot()->heights[3] == data.heights[3],
                     "superseded pending source releases budget so the newest edit can publish" );
            terrain->unload( nullptr );
            require( !terrain->getRenderedTerrainSnapshot() &&
                         !terrain->getActiveTerrainSnapshot() &&
                         !terrain->intersects( queryRay )->hasIntersected() && !terrain->getMesh() &&
                         !terrain->getNativeRenderMesh() &&
                         terrain->getTerrainResidency().residentBytes == 0,
                     "unload releases GPU generations and removes their active query surface" );
            require( scheduler->stats().used[gpuClass] == initialBudget.used[gpuClass],
                     "unload releases all active and staged shared GPU reservations" );
            // An unloaded stream still owns its admitted producer/control storage.
            // End that owner's lifetime before comparing total physical SourceCpu
            // usage to the baseline taken before constructing the terrain.
            terrain = nullptr;
            otherTerrain = nullptr;
            if( scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )] !=
                initialBudget.used[size_t( StreamMemoryClass::SourceCpu )] )
            {
                const auto strings = CharacterPoolAllocator<char>::stats();
                std::fprintf( stderr, "Terrain source retirement: bytes=%llu->%llu stringBytes=%zu->%zu stringBuckets=%zu->%zu\n",
                    static_cast<unsigned long long>( initialBudget.used[size_t( StreamMemoryClass::SourceCpu )] ),
                    static_cast<unsigned long long>( scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )] ),
                    initialStrings.committedBytes, strings.committedBytes, initialStrings.bucketCount, strings.bucketCount );
            }
            require( scheduler->stats().used[size_t( StreamMemoryClass::SourceCpu )] ==
                         initialBudget.used[size_t( StreamMemoryClass::SourceCpu )],
                     "terrain destruction drains jobs and retires all source and producer reservations" );
            std::puts( "Claw terrain bounded GPU residency and complete generation retention passed." );
            return true;
        }
        catch( const std::exception &e )
        {
            std::fprintf( stderr, "Claw terrain streaming failed: %s\n", e.what() );
            return false;
        }
    }
}  // namespace claw_terrain_streaming_contracts
#endif
