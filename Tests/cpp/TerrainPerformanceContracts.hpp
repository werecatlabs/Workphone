#ifndef WPTerrainPerformanceContracts_h__
#define WPTerrainPerformanceContracts_h__

// Representative terrain performance and soak fixtures for the acceptance
// scenarios documented in docs/WPGRAPHICS_TERRAIN_PROCEDURAL_REVIEW.md:
// cap-adjacent erosion latency and peak memory, 1,000-style
// regeneration/load/unload/resize cycles, and a fixed flythrough plus teleport
// stream at representative scale. These are measurements, not release
// certification; results are recorded in the implementation status.

#include <Workphone/Graphics/TerrainTiles.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainEditing.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>
#if defined( _WIN32 )
#include <windows.h>
#include <psapi.h>
#endif

namespace terrain_performance_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using workphone::scene::TerrainErosionMode;
    using workphone::scene::TerrainErosionRecipe;
    using workphone::scene::TerrainSystem;

    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }

    inline s32 peakWorkingSetMiB()
    {
#if defined( _WIN32 )
        PROCESS_MEMORY_COUNTERS counters{};
        if( GetProcessMemoryInfo( GetCurrentProcess(), &counters, sizeof( counters ) ) )
            return s32( counters.PeakWorkingSetSize >> 20 );
#endif
        return -1;
    }

    inline double percentile( std::vector<double> sorted, double p )
    {
        require( !sorted.empty(), "percentile fixture collected samples" );
        std::sort( sorted.begin(), sorted.end() );
        const auto index =
            std::min( sorted.size() - 1,
                      static_cast<size_t>( std::ceil( p / 100.0 * sorted.size() ) ) - 1 );
        return sorted[index];
    }

    // The documented erosion ceiling is 262,144 simulation samples including
    // halo and 16,777,216 simulation sample iterations. A 128 m-radius write
    // region at 1 m spacing is exactly 256 samples wide, so 64 hydraulic
    // iterations put the simulation at exactly 512x512 samples and exactly the
    // sample-iteration cap; 64 thermal iterations use a 384-sample halo box.
    inline void representativeErosionMeasurement()
    {
        TerrainData field;
        field.dimensions = { 1025, 1025 };
        field.origin = { 0, 0 };
        field.spacing = { 1, 1 };
        field.heights.resize( size_t( 1025 ) * 1025 );
        for( s32 z = 0; z < 1025; ++z )
            for( s32 x = 0; x < 1025; ++x )
                field.heights[size_t( z ) * 1025 + x] =
                    std::sin( x * .031f ) * std::cos( z * .047f ) * 6 +
                    std::sin( ( x + z ) * .013f ) * 3;
        String error;
        const std::pair<const char *, TerrainErosionMode> modes[] = {
            { "thermal", TerrainErosionMode::Thermal },
            { "hydraulic", TerrainErosionMode::Hydraulic },
        };
        for( const auto &entry : modes )
        {
            TerrainErosionRecipe recipe;
            recipe.mode = entry.second;
            recipe.region.centre = { 512.5f, 512.5f };
            recipe.region.radius = 128;
            recipe.iterations = 64;
            recipe.seed = 97;
            TerrainData result;
            const auto start = std::chrono::steady_clock::now();
            require( prepareTerrainErosion( field, recipe, result, nullptr, error ),
                     error.c_str() );
            const auto milliseconds =
                std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - start )
                    .count();
            require( validateTerrainData( result, error ) && result.heights != field.heights,
                     "representative erosion publishes a changed valid field" );
            // Determinism must hold at the cap-adjacent scale too.
            TerrainData repeat;
            require( prepareTerrainErosion( field, recipe, repeat, nullptr, error ),
                     error.c_str() );
            require( repeat.heights == result.heights && repeat.revision == result.revision,
                     "representative erosion is deterministic" );
            // Samples outside the write region plus halo must be untouched.
            size_t changedOutside = 0;
            for( s32 z = 0; z < 1025; ++z )
            {
                for( s32 x = 0; x < 1025; ++x )
                {
                    const double dx = x - 512.5, dz = z - 512.5;
                    const bool writable = dx * dx + dz * dz < 128.0 * 128.0;
                    if( !writable && result.heights[size_t( z ) * 1025 + x] !=
                                         field.heights[size_t( z ) * 1025 + x] )
                        ++changedOutside;
                }
            }
            require( changedOutside == 0, "representative erosion writes only the selected region" );
            std::printf(
                "Terrain %s erosion representative: 1025x1025 source, radius 128 write region, %u "
                "iterations, seed 97, %.3f ms, process peak working set %d MiB\n",
                entry.first, recipe.iterations, milliseconds, peakWorkingSetMiB() );
        }
        // Cancellation must abort promptly from inside the bounded simulation.
        TerrainErosionRecipe recipe;
        recipe.mode = TerrainErosionMode::Hydraulic;
        recipe.region.centre = { 512.5f, 512.5f };
        recipe.region.radius = 128;
        recipe.iterations = 64;
        recipe.seed = 97;
        std::atomic_bool cancelled{ false };
        TerrainData output = field;
        std::atomic<bool> failed{ false };
        std::thread worker( [&] {
            String workerError;
            failed = !prepareTerrainErosion( field, recipe, output, &cancelled, workerError );
        } );
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
        const auto cancelStart = std::chrono::steady_clock::now();
        cancelled = true;
        worker.join();
        const auto cancelMs = std::chrono::duration<double, std::milli>(
                                  std::chrono::steady_clock::now() - cancelStart )
                                  .count();
        require( failed && output.heights == field.heights,
                 "cancelled representative erosion leaves output untouched" );
        std::printf( "Terrain erosion cancellation latency: %.3f ms\n", cancelMs );
        require( cancelMs < 15000, "cancellation terminates within one bounded iteration window" );
    }

    inline void regenerationSoak( u32 cycles )
    {
        auto scheduler = WorldStreamingScheduler::shared();
        const auto baseline = scheduler->stats();
        const auto sourceClass = size_t( StreamMemoryClass::SourceCpu );
        String error;
        std::vector<double> cycleMs;
        cycleMs.reserve( cycles );
        for( u32 i = 0; i < cycles; ++i )
        {
            const auto start = std::chrono::steady_clock::now();
            auto terrain = make_ptr<TerrainSystem>();
            const u32 width = 65 + ( i % 4 ) * 32, depth = 65 + ( ( i / 4 ) % 4 ) * 32;
            terrain->setGeneratedHeightMapWidth( width );
            terrain->setGeneratedHeightMapHeight( depth );
            terrain->generateHeightMap();
            auto snapshot = terrain->getTerrainSnapshot();
            require( snapshot && snapshot->dimensions == Vector2I( s32( width ), s32( depth ) ) &&
                         snapshot->heights.size() == size_t( width ) * depth,
                     "regeneration produces the requested grid every cycle" );
            const auto json = terrain->exportTerrainData();
            require( !json.empty(), "cycled terrain exports" );
            auto reopened = make_ptr<TerrainSystem>();
            require( reopened->importTerrainData( json, error ), error.c_str() );
            require( reopened->exportTerrainData() == json, "cycled export roundtrips exactly" );
            terrain = nullptr;
            reopened = nullptr;
            cycleMs.push_back( std::chrono::duration<double, std::milli>(
                                   std::chrono::steady_clock::now() - start )
                                   .count() );
        }
        const auto after = scheduler->stats();
        require( after.used[sourceClass] == baseline.used[sourceClass] &&
                     after.pending == baseline.pending && after.ready == baseline.ready &&
                     after.producers == baseline.producers,
                 "regeneration soak leaves no scheduler reservation or work behind" );
        std::vector<double> first( cycleMs.begin(), cycleMs.begin() + std::min<u32>( 16, cycles ) ),
            last( cycleMs.end() - std::min<u32>( 16, cycles ), cycleMs.end() );
        const auto firstMean = std::accumulate( first.begin(), first.end(), 0.0 ) / first.size();
        const auto lastMean = std::accumulate( last.begin(), last.end(), 0.0 ) / last.size();
        const auto p95 = percentile( cycleMs, 95 );
        std::printf(
            "Terrain regeneration soak: %u cycles, mean first16 %.3f ms, mean last16 %.3f ms, "
            "p95 %.3f ms, scheduler SourceCpu unchanged\n",
            cycles, firstMean, lastMean, p95 );
        require( lastMean < firstMean * 4 + 25.0 && p95 < firstMean * 8 + 50.0,
                 "no unexplained per-cycle drift across the soak" );
    }

    // Fixed flythrough over a 1025x1025 source with cross-map teleports,
    // measuring cold stream, per-stop and teleport completion latency on the
    // CPU streaming path. GPU residency evidence is covered by the Claw
    // streaming/motion contracts.
    inline void flythroughTeleportSoak()
    {
        String error;
        TerrainData field;
        field.dimensions = { 1025, 1025 };
        field.origin = { 0, 0 };
        field.spacing = { 1, 1 };
        field.heights.resize( size_t( 1025 ) * 1025 );
        for( s32 z = 0; z < 1025; ++z )
            for( s32 x = 0; x < 1025; ++x )
                field.heights[size_t( z ) * 1025 + x] =
                    std::sin( x * .043f ) * std::cos( z * .067f ) * 5;
        Array<TerrainTileInfo> tiles;
        require( describeTerrainTiles( field, tiles, error ) && tiles.size() == 256,
                 "representative source partitions into 256 tiles" );
        Array<TerrainTileKey> keys;
        for( const auto &tile : tiles )
            keys.push_back( { tile.id, 3 } );
        const auto schedulerStats = [] {
            return WorldStreamingScheduler::shared()->stats();
        };
        const auto baseline = schedulerStats();
        TerrainTileStream stream;
        auto source = std::make_shared<TerrainData>( field );
        source->revision = 1;
        u64 peakReadyBytes = 0;
        TerrainTileResult result;
        const auto coldStart = std::chrono::steady_clock::now();
        require( stream.request( source, tiles, keys, error ), error.c_str() );
        size_t received = 0;
        const auto coldDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 120 );
        while( received < keys.size() && std::chrono::steady_clock::now() < coldDeadline )
        {
            if( stream.take( result ) )
            {
                require( result.source == source && result.mesh && result.error.empty(),
                         "flythrough workers deliver immutable results" );
                ++received;
            }
            else
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            peakReadyBytes = std::max( peakReadyBytes, stream.stats().readyBytes );
        }
        require( received == keys.size(), "cold representative stream completes" );
        const auto coldMs = std::chrono::duration<double, std::milli>(
                                std::chrono::steady_clock::now() - coldStart )
                                .count();
        // Rotary flythrough: 32 stops, each requesting a moving 16-tile window.
        std::vector<double> stopMs;
        for( u32 stop = 0; stop < 32; ++stop )
        {
            Array<TerrainTileKey> demand;
            for( u32 t = 0; t < 16; ++t )
                demand.push_back( { ( stop * 8 + t ) % 256, 3 } );
            const auto stopStart = std::chrono::steady_clock::now();
            require( stream.request( source, tiles, demand, error ), error.c_str() );
            size_t got = 0;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 15 );
            while( got < demand.size() && std::chrono::steady_clock::now() < deadline )
            {
                if( stream.take( result ) )
                    ++got;
                else
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }
            require( got == demand.size(), "flythrough stop completes" );
            stopMs.push_back( std::chrono::duration<double, std::milli>(
                                  std::chrono::steady_clock::now() - stopStart )
                                  .count() );
        }
        // Teleport phase: replace the generation and re-stream the whole map.
        std::vector<double> teleportMs;
        for( u32 teleport = 0; teleport < 4; ++teleport )
        {
            auto generation = std::make_shared<TerrainData>( field );
            generation->revision = 2 + teleport;
            const auto start = std::chrono::steady_clock::now();
            require( stream.request( generation, tiles, keys, error ), error.c_str() );
            size_t got = 0;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 120 );
            while( got < keys.size() && std::chrono::steady_clock::now() < deadline )
            {
                if( stream.take( result ) )
                    ++got;
                else
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                peakReadyBytes = std::max( peakReadyBytes, stream.stats().readyBytes );
            }
            require( got == keys.size(), "teleport stream completes" );
            teleportMs.push_back( std::chrono::duration<double, std::milli>(
                                      std::chrono::steady_clock::now() - start )
                                      .count() );
        }
        stream.drain();
        const auto after = schedulerStats();
        require( after.pending == baseline.pending && after.active == baseline.active &&
                     after.ready == baseline.ready,
                 "flythrough/teleport drain leaves no queued work" );
        std::printf(
            "Terrain flythrough/teleport: cold 256-tile stream %.3f ms, stop p50 %.3f ms, p95 "
            "%.3f ms, teleport mean %.3f ms, peak ready reservation %llu bytes\n",
            coldMs, percentile( stopMs, 50 ), percentile( stopMs, 95 ),
            std::accumulate( teleportMs.begin(), teleportMs.end(), 0.0 ) / teleportMs.size(),
            static_cast<unsigned long long>( peakReadyBytes ) );
    }
}  // namespace terrain_performance_contracts
#endif
