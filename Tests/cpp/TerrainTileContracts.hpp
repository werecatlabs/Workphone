#ifndef WPTerrainTileContracts_h__
#define WPTerrainTileContracts_h__
#include <Workphone/Graphics/TerrainTiles.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <chrono>
#include <cmath>
#include <map>
#include <stdexcept>
#include <thread>

namespace terrain_tile_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }
    inline TerrainData fixture( s32 width = 129, s32 depth = 65 )
    {
        TerrainData d;
        d.dimensions = { width, depth };
        d.origin = { 0, 0 };
        d.spacing = { 1, 1 };
        d.heights.resize( size_t( width ) * depth );
        for( s32 z = 0; z < depth; ++z )
            for( s32 x = 0; x < width; ++x )
                d.heights[size_t( z ) * width + x] = std::sin( x * .07f ) * std::cos( z * .11f ) * 4;
        return d;
    }
    inline void run()
    {
        auto data = fixture();
        String error;
        Array<TerrainTileInfo> tiles;
        require( describeTerrainTiles( data, tiles, error ) && tiles.size() == 2,
                 "129x65 has two shared-border tiles" );
        TerrainMeshData fine, coarse;
        require( buildTerrainTile( data, tiles[0], 0, fine, error ) &&
                     buildTerrainTile( data, tiles[1], 3, coarse, error ),
                 "fine and coarse tiles build" );
        std::map<int, size_t> edge;
        for( size_t i = 0; i < fine.positions.size(); ++i )
            if( fine.positions[i].x == 64 )
                edge.emplace( int( fine.positions[i].z ), i );
        size_t matched = 0;
        for( size_t i = 0; i < coarse.positions.size(); ++i )
            if( coarse.positions[i].x == 64 )
            {
                auto found = edge.find( int( coarse.positions[i].z ) );
                require( found != edge.end(), "all coarse border samples exist in fine neighbor" );
                const auto j = found->second;
                require( coarse.positions[i] == fine.positions[j] &&
                             coarse.normals[i] == fine.normals[j] && coarse.uvs[i] == fine.uvs[j],
                         "mixed LOD boundary positions, normals and UVs match exactly" );
                ++matched;
            }
        require( matched == 65 && coarse.indices.size() < fine.indices.size() / 4,
                 "border is complete while coarse interior reduces triangles" );
        double area = 0;
        for( size_t i = 0; i < coarse.indices.size(); i += 3 )
        {
            auto a = coarse.positions.at( coarse.indices[i] ),
                 b = coarse.positions.at( coarse.indices[i + 1] ),
                 c = coarse.positions.at( coarse.indices[i + 2] );
            const auto twice = ( b.z - a.z ) * ( c.x - a.x ) - ( b.x - a.x ) * ( c.z - a.z );
            require( twice > 0, "every LOD triangle retains upward winding and nonzero area" );
            area += twice * .5;
        }
        require( std::abs( area - 4096 ) < .001,
                 "LOD fans cover the tile without gaps or overlapping area" );
        data.holes.assign( 128 * 64, 0 );
        data.holes[64] = 1;
        require( describeTerrainTiles( data, tiles, error ) && tiles[1].hasHoles,
                 "hole metadata propagates to correct tile" );
        require( !buildTerrainTile( data, tiles[1], 3, coarse, error ),
                 "hole tiles cannot simplify away gameplay topology" );
        require( buildTerrainTile( data, tiles[1], 0, coarse, error ) &&
                     coarse.indices.size() == 64 * 64 * 6 - 6,
                 "hole removes both cell triangles" );
        TerrainMeshData preserved = coarse;
        require( !buildTerrainTile( data, tiles[0], 0, coarse, error, [] { return true; } ) &&
                     coarse.positions == preserved.positions,
                 "cancelled geometry preparation preserves caller output" );
        auto malformed = data;
        malformed.spacing.x = 0;
        require( !buildTerrainTile( malformed, tiles[0], 0, coarse, error ) &&
                     coarse.positions == preserved.positions,
                 "invalid source metadata leaves tile output untouched" );
        auto forged = tiles[1];
        forged.hasHoles = false;
        require( !buildTerrainTile( data, forged, 1, coarse, error ),
                 "forged tile metadata cannot hide source holes from LOD validation" );

        TerrainTileView view;
        view.cull = false;
        view.localCamera = { 32, 1000, 32 };
        view.pixelsPerMetreAtUnitDistance = 100;
        auto selected = selectTerrainTiles( tiles, { view }, {}, 256 );
        require( selected.size() == 2 && selected[0].lod > 0 && selected[1].lod == 0,
                 "LOD error selection keeps hole tile exact" );
        auto jitter = view;
        jitter.localCamera.y += .01f;
        require( selectTerrainTiles( tiles, { jitter }, selected, 256 ) == selected,
                 "smallSource camera jitter does not toggle stable LOD" );
        auto closeView = view;
        closeView.localCamera = { 32, 1, 32 };
        auto multiple = selectTerrainTiles( tiles, { view, closeView }, selected, 256 );
        require( multiple[0].lod == 0, "multi-view union selects most detailed demand" );
        require( selectTerrainTiles( tiles, { view }, {}, 1 ).size() == 1,
                 "residency selection is count bounded" );
        view.cull = true;
        view.localToClip = Matrix4F::identity();
        require( selectTerrainTiles( tiles, { view }, {}, 256 ).size() == 1,
                 "frustum rejection excludes distant tile" );
        TerrainTileInfo outside;
        outside.minimum = { 1.05f, 0, 0 };
        outside.maximum = { 1.1f, .5f, .5f };
        require( selectTerrainTiles( { outside }, { view }, {}, 1 ).empty(),
                 "unexpanded view culls tile beyond the edge" );
        view.prefetchMargin = .15f;
        require( selectTerrainTiles( { outside }, { view }, {}, 1 ).size() == 1,
                 "prefetch margin requests terrain just outside the view" );

        TerrainTileStream stream;
        auto source = std::make_shared<TerrainData>( fixture( 1025, 1025 ) );
        source->revision = 1;
        require( describeTerrainTiles( *source, tiles, error ) && tiles.size() == 256,
                 "1025 grid partitions into 256 shared-border tiles" );
        Array<TerrainTileKey> demand;
        for( const auto &t : tiles )
            demand.push_back( { t.id, 3 } );
        require( !stream.request( source, tiles, { demand.front(), demand.front() }, error ),
                 "duplicate stream demands rejected" );
        const auto streamStart = std::chrono::steady_clock::now();
        u64 peakReadyBytes = 0;
        require( stream.request( source, tiles, demand, error ),
                 "bounded large-map stream request accepted" );
        TerrainTileResult result;
        size_t received = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 15 );
        while( received < demand.size() && std::chrono::steady_clock::now() < deadline )
        {
            if( stream.take( result ) )
            {
                require( result.source == source && result.mesh && result.error.empty(),
                         "worker produces requested generation" );
                ++received;
            }
            else
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            const auto stats = stream.stats();
            peakReadyBytes = std::max( peakReadyBytes, stats.readyBytes );
            require( stats.pending <= 256 && stats.active <= 1 && stats.ready <= 8 &&
                         stats.readyBytes <= 8ull * 1024 * 1024,
                     "worker queue and ready memory remain bounded" );
        }
        require( received == 256, "complete 1025 terrain stream finishes within fixture timeout" );
        std::printf(
            "Terrain CPU stream: 1025x1025, 256 LOD3 tiles, %.3f ms, peak ready reservation %llu bytes\n",
            std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - streamStart )
                .count(),
            static_cast<unsigned long long>( peakReadyBytes ) );
        // Repeated teleport/cancel/reload cannot leak ready results across epochs.
        for( u32 i = 0; i < 1000; ++i )
        {
            require( stream.request( source, tiles, { { i % 256, 3 } }, error ),
                     "teleport request accepted" );
            if( i % 3 == 0 )
                stream.cancel();
        }
        auto replacement = std::make_shared<TerrainData>( *source );
        replacement->revision = 2;
        require( stream.request( replacement, tiles, { { 7, 2 } }, error ),
                 "new source generation replaces pending work" );
        bool done = false;
        const auto finalDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
        while( !done && std::chrono::steady_clock::now() < finalDeadline )
        {
            if( stream.take( result ) )
            {
                require(
                    result.source == replacement && result.key == TerrainTileKey{ 7, 2 } && result.mesh,
                    "stale jobs cannot resurrect prior demand" );
                done = true;
            }
            else
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
        require( done, "latest generation completes after teleport soak" );
        stream.cancel();
        require( stream.stats().readyBytes == 0 && stream.stats().pending == 0,
                 "cancel releases queued geometry" );
        stream.drain();
        result = {};
        auto scheduler = WorldStreamingScheduler::shared();
        const auto baseline = scheduler->stats();
        const auto sourceClass = size_t( StreamMemoryClass::SourceCpu );
        const auto resultClass = size_t( StreamMemoryClass::ResultCpu );
        {
            TerrainTileStream first, second;
            auto smallSource = std::make_shared<TerrainData>( fixture( 65, 65 ) );
            require( describeTerrainTiles( *smallSource, tiles, error ), "Small shared producer fixture" );
            require( first.request( smallSource, tiles, { { 0, 0 } }, error ) &&
                         second.request( smallSource, tiles, { { 0, 1 } }, error ),
                     "Independent terrain producers share the bounded worker service" );
            require( scheduler->stats().workers == baseline.workers &&
                         scheduler->stats().producers == baseline.producers + 2,
                     "Adding terrain producers does not create another worker pool" );
            TerrainTileResult a, b;
            bool gotA = false, gotB = false;
            const auto sharedDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            while( ( !gotA || !gotB ) && std::chrono::steady_clock::now() < sharedDeadline )
            {
                if( !gotA ) gotA = first.take( a );
                if( !gotB ) gotB = second.take( b );
                if( !gotA || !gotB ) std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }
            require( gotA && gotB && a.mesh && b.mesh && a.error.empty() && b.error.empty(),
                     "Both terrain producers deliver immutable meshes" );
            const auto admitted = scheduler->stats();
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, admitted.used[sourceClass], error ),
                     "Source budget can be constrained to live reservations" );
            auto other = std::make_shared<TerrainData>( *smallSource );
            other->heights[0] += 1;
            const bool rejectedSource = !first.request( other, tiles, { { 0, 0 } }, error );
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, admitted.limits[sourceClass], error ),
                     "Source admission limit restored" );
            require( rejectedSource && a.source == smallSource,
                     "Source admission failure retains existing completed terrain" );
            auto meshA = a.mesh, meshB = b.mesh;
            a = {}; b = {};
            first.drain(); second.drain();
            require( scheduler->stats().used[sourceClass] == baseline.used[sourceClass] + 2 * terrainSourceBytes( *smallSource ) &&
                         scheduler->stats().used[resultClass] == baseline.used[resultClass] + ( 2ull << 20 ),
                     "Retained mesh aliases preserve source and result reservations after producer drain" );
            meshA.reset(); meshB.reset();
            require( scheduler->stats().used[sourceClass] == baseline.used[sourceClass] &&
                         scheduler->stats().used[resultClass] == baseline.used[resultClass],
                     "Last mesh alias release returns source and result reservations" );
        }
        require( scheduler->stats().producers == baseline.producers,
                 "Terrain producer destruction unregisters from the shared service" );
        std::printf(
            "Terrain tiles: 1025x1025, 256 tiles, mixed-LOD seams, bounded worker and 1000 demand "
            "changes passed.\n" );
    }
}  // namespace terrain_tile_contracts
#endif
