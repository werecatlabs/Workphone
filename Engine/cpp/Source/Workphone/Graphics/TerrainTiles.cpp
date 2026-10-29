#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/TerrainTiles.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace workphone::render
{
    namespace
    {
        f64 height( const TerrainData &d, s32 x, s32 z )
        {
            return f64( d.heights[size_t( z ) * d.dimensions.x + x] ) * d.heightScale;
        }

        f64 interpolatedHeight( const TerrainData &d, f64 x, f64 z )
        {
            const auto ix = std::min( s32( x ), d.dimensions.x - 2 );
            const auto iz = std::min( s32( z ), d.dimensions.y - 2 );
            const auto fx = x - ix, fz = z - iz;
            const auto a = height( d, ix, iz ), b = height( d, ix + 1, iz );
            const auto c = height( d, ix, iz + 1 ), e = height( d, ix + 1, iz + 1 );
            return fx + fz <= 1 ? a + fx * ( b - a ) + fz * ( c - a )
                                : e + ( 1 - fx ) * ( c - e ) + ( 1 - fz ) * ( b - e );
        }

        Vector3F normal( const TerrainData &d, f64 x, f64 z )
        {
            const auto x0 = std::max( 0.0, x - 1 ), x1 = std::min( f64( d.dimensions.x - 1 ), x + 1 );
            const auto z0 = std::max( 0.0, z - 1 ), z1 = std::min( f64( d.dimensions.y - 1 ), z + 1 );
            const auto dx = ( x1 - x0 ) * d.spacing.x, dz = ( z1 - z0 ) * d.spacing.y;
            const auto dyx = interpolatedHeight( d, x1, z ) - interpolatedHeight( d, x0, z );
            const auto dyz = interpolatedHeight( d, x, z1 ) - interpolatedHeight( d, x, z0 );
            const auto nx = -dyx * dz, ny = dx * dz, nz = -dyz * dx;
            const auto length = std::hypot( nx, ny, nz );
            return { f32( nx / length ), f32( ny / length ), f32( nz / length ) };
        }

        bool visible( const TerrainTileInfo &tile, const TerrainTileView &view )
        {
            if( !view.cull )
            {
                return true;
            }
            // Homogeneous six-plane AABB test. Test the full [-w,w] depth range:
            // conservative for both existing GL-style and DX-style projections.
            u32 common = 63;
            for( auto x : { tile.minimum.x, tile.maximum.x } )
            {
                for( auto y : { tile.minimum.y, tile.maximum.y } )
                {
                    for( auto z : { tile.minimum.z, tile.maximum.z } )
                    {
                        const auto &m = view.localToClip;
                        const f64 cx =
                            f64( m[0][0] ) * x + f64( m[0][1] ) * y + f64( m[0][2] ) * z + m[0][3];
                        const f64 cy =
                            f64( m[1][0] ) * x + f64( m[1][1] ) * y + f64( m[1][2] ) * z + m[1][3];
                        const f64 cz =
                            f64( m[2][0] ) * x + f64( m[2][1] ) * y + f64( m[2][2] ) * z + m[2][3];
                        const f64 cw =
                            f64( m[3][0] ) * x + f64( m[3][1] ) * y + f64( m[3][2] ) * z + m[3][3];
                        if( !std::isfinite( cx + cy + cz + cw ) )
                        {
                            return true;
                        }
                        const auto extent = cw * ( 1.0 + std::clamp( view.prefetchMargin, 0.0f, 0.5f ) );
                        common &= ( cx < -extent ? 1u : 0u ) | ( cx > extent ? 2u : 0u ) |
                                  ( cy < -extent ? 4u : 0u ) | ( cy > extent ? 8u : 0u ) |
                                  ( cz < -cw ? 16u : 0u ) | ( cz > cw ? 32u : 0u );
                    }
                }
            }
            return common == 0;
        }

        f64 distanceTo( const TerrainTileInfo &tile, const TerrainTileView &view )
        {
            const auto &p = view.localCamera;
            const auto dx =
                std::max( { f64( tile.minimum.x ) - p.x, 0.0, f64( p.x ) - tile.maximum.x } );
            const auto dy =
                std::max( { f64( tile.minimum.y ) - p.y, 0.0, f64( p.y ) - tile.maximum.y } );
            const auto dz =
                std::max( { f64( tile.minimum.z ) - p.z, 0.0, f64( p.z ) - tile.maximum.z } );
            return std::max( 0.01,
                             std::hypot( dx, dy, dz ) * std::max( 0.000001f, view.minimumWorldScale ) );
        }
    }  // namespace

    bool describeTerrainTiles( const TerrainData &data, Array<TerrainTileInfo> &output, String &error )
    {
        if( !validateTerrainData( data, error ) )
        {
            return false;
        }
        Array<TerrainTileInfo> result;
        for( s32 z = 0; z < data.dimensions.y - 1; z += terrainTileCells )
        {
            for( s32 x = 0; x < data.dimensions.x - 1; x += terrainTileCells )
            {
                TerrainTileInfo tile;
                tile.id = u32( result.size() );
                tile.first = { x, z };
                tile.last = { std::min( x + s32( terrainTileCells ), data.dimensions.x - 1 ),
                              std::min( z + s32( terrainTileCells ), data.dimensions.y - 1 ) };
                f64 low = std::numeric_limits<f64>::infinity(), high = -low;
                for( s32 iz = z; iz <= tile.last.y; ++iz )
                {
                    for( s32 ix = x; ix <= tile.last.x; ++ix )
                    {
                        const auto h = height( data, ix, iz );
                        low = std::min( low, h );
                        high = std::max( high, h );
                        if( !data.holes.empty() && ix < tile.last.x && iz < tile.last.y &&
                            data.holes[size_t( iz ) * ( data.dimensions.x - 1 ) + ix] )
                        {
                            tile.hasHoles = true;
                        }
                    }
                }
                tile.minimum = { f32( f64( data.origin.x ) + x * f64( data.spacing.x ) ), f32( low ),
                                 f32( f64( data.origin.y ) + z * f64( data.spacing.y ) ) };
                tile.maximum = { f32( f64( data.origin.x ) + tile.last.x * f64( data.spacing.x ) ),
                                 f32( high ),
                                 f32( f64( data.origin.y ) + tile.last.y * f64( data.spacing.y ) ) };
                // Each coarse triangle and the source lie within this cell's
                // height range, so this bounds error even where diagonals cross.
                for( u32 lod = 1; lod < terrainTileLods; ++lod )
                {
                    const s32 step = 1 << lod;
                    f64 bound = 0;
                    for( s32 cz = z; cz < tile.last.y; cz += step )
                    {
                        for( s32 cx = x; cx < tile.last.x; cx += step )
                        {
                            f64 lo = std::numeric_limits<f64>::infinity(), hi = -lo;
                            for( s32 iz = cz; iz <= std::min( cz + step, tile.last.y ); ++iz )
                            {
                                for( s32 ix = cx; ix <= std::min( cx + step, tile.last.x ); ++ix )
                                {
                                    const auto h = height( data, ix, iz );
                                    lo = std::min( lo, h );
                                    hi = std::max( hi, h );
                                }
                            }
                            bound = std::max( bound, hi - lo );
                        }
                    }
                    tile.errors[lod] = f32( std::min( bound, f64( std::numeric_limits<f32>::max() ) ) );
                }
                result.push_back( tile );
            }
        }
        output = std::move( result );
        return true;
    }

    bool buildTerrainTile( const TerrainData &data, const TerrainTileInfo &tile, u32 lod,
                           TerrainMeshData &output, String &error,
                           const std::function<bool()> &cancelled )
    {
        error.clear();
        if( data.dimensions.x < 2 || data.dimensions.y < 2 ||
            data.dimensions.x > terrainMaximumDimension || data.dimensions.y > terrainMaximumDimension ||
            u64( data.dimensions.x ) * data.dimensions.y > terrainMaximumSamples ||
            !std::isfinite( data.spacing.x ) || data.spacing.x <= 0 ||
            !std::isfinite( data.spacing.y ) || data.spacing.y <= 0 || !std::isfinite( data.origin.x ) ||
            !std::isfinite( data.origin.y ) || !std::isfinite( data.heightScale ) ||
            !std::isfinite( data.origin.x + ( data.dimensions.x - 1 ) * data.spacing.x ) ||
            !std::isfinite( data.origin.y + ( data.dimensions.y - 1 ) * data.spacing.y ) ||
            lod >= terrainTileLods || tile.first.x < 0 || tile.first.y < 0 ||
            tile.last.x >= data.dimensions.x || tile.last.y >= data.dimensions.y ||
            tile.last.x <= tile.first.x || tile.last.y <= tile.first.y ||
            tile.last.x - tile.first.x > terrainTileCells ||
            tile.last.y - tile.first.y > terrainTileCells ||
            data.heights.size() != size_t( data.dimensions.x ) * data.dimensions.y ||
            ( !data.holes.empty() &&
              data.holes.size() != size_t( data.dimensions.x - 1 ) * ( data.dimensions.y - 1 ) ) ||
            ( tile.hasHoles && lod != 0 ) )
        {
            error = "Invalid terrain tile or LOD";
            return false;
        }
        // Public tile calls may not have gone through describeTerrainTiles. Check
        // just this tile and its normal halo, keeping worker cost independent of
        // total terrain size and rejecting malformed input before touching output.
        for( s32 z = std::max( 0, tile.first.y - 1 );
             z <= std::min( data.dimensions.y - 1, tile.last.y + 1 ); ++z )
        {
            if( cancelled && cancelled() )
            {
                error = "Terrain tile generation cancelled";
                return false;
            }
            for( s32 x = std::max( 0, tile.first.x - 1 );
                 x <= std::min( data.dimensions.x - 1, tile.last.x + 1 ); ++x )
            {
                if( !std::isfinite( data.heights[size_t( z ) * data.dimensions.x + x] *
                                    data.heightScale ) )
                {
                    error = "Terrain tile samples must be finite";
                    return false;
                }
            }
        }
        if( lod && !data.holes.empty() )
        {
            for( s32 z = tile.first.y; z < tile.last.y; ++z )
            {
                for( s32 x = tile.first.x; x < tile.last.x; ++x )
                {
                    if( data.holes[size_t( z ) * ( data.dimensions.x - 1 ) + x] )
                    {
                        error = "Terrain tiles containing holes require exact LOD0";
                        return false;
                    }
                }
            }
        }
        try
        {
            TerrainMeshData mesh;
            std::unordered_map<u64, u32> vertices;
            const auto vertex = [&]( s32 doubledX, s32 doubledZ ) {
                const u64 key = ( u64( u32( doubledZ ) ) << 32 ) | u32( doubledX );
                auto found = vertices.find( key );
                if( found != vertices.end() )
                {
                    return found->second;
                }
                const f64 x = doubledX * 0.5, z = doubledZ * 0.5;
                const u32 index = u32( mesh.positions.size() );
                vertices.emplace( key, index );
                mesh.positions.push_back( { f32( f64( data.origin.x ) + x * data.spacing.x ),
                                            f32( interpolatedHeight( data, x, z ) ),
                                            f32( f64( data.origin.y ) + z * data.spacing.y ) } );
                mesh.normals.push_back( normal( data, x, z ) );
                mesh.uvs.push_back(
                    { f32( x / ( data.dimensions.x - 1 ) ), f32( z / ( data.dimensions.y - 1 ) ) } );
                return index;
            };
            const s32 step = 1 << lod;
            for( s32 z = tile.first.y; z < tile.last.y; z += step )
            {
                if( cancelled && cancelled() )
                {
                    error = "Terrain tile generation cancelled";
                    return false;
                }
                for( s32 x = tile.first.x; x < tile.last.x; x += step )
                {
                    const s32 ex = std::min( x + step, tile.last.x ),
                              ez = std::min( z + step, tile.last.y );
                    if( lod == 0 )
                    {
                        if( !data.holes.empty() &&
                            data.holes[size_t( z ) * ( data.dimensions.x - 1 ) + x] )
                        {
                            continue;
                        }
                        const auto a = vertex( 2 * x, 2 * z ), b = vertex( 2 * ex, 2 * z );
                        const auto c = vertex( 2 * x, 2 * ez ), d = vertex( 2 * ex, 2 * ez );
                        for( auto i : { a, c, b, b, c, d } )
                        {
                            mesh.indices.push_back( i );
                        }
                    }
                    else
                    {
                        Array<u32> ring;
                        // Clockwise viewed from above, giving +Y winding.
                        for( s32 rz = z; rz < ez; rz += ( x == tile.first.x ? 1 : step ) )
                        {
                            ring.push_back( vertex( 2 * x, 2 * rz ) );
                        }
                        for( s32 rx = x; rx < ex; rx += ( ez == tile.last.y ? 1 : step ) )
                        {
                            ring.push_back( vertex( 2 * rx, 2 * ez ) );
                        }
                        for( s32 rz = ez; rz > z; rz -= ( ex == tile.last.x ? 1 : step ) )
                        {
                            ring.push_back( vertex( 2 * ex, 2 * rz ) );
                        }
                        for( s32 rx = ex; rx > x; rx -= ( z == tile.first.y ? 1 : step ) )
                        {
                            ring.push_back( vertex( 2 * rx, 2 * z ) );
                        }
                        const auto centre = vertex( x + ex, z + ez );
                        for( size_t i = 0; i < ring.size(); ++i )
                        {
                            for( auto index : { centre, ring[i], ring[( i + 1 ) % ring.size()] } )
                            {
                                mesh.indices.push_back( index );
                            }
                        }
                    }
                }
            }
            output = std::move( mesh );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    Array<TerrainTileKey> selectTerrainTiles( const Array<TerrainTileInfo> &tiles,
                                              const Array<TerrainTileView> &views,
                                              const Array<TerrainTileKey> &previous, u32 maximumTiles )
    {
        struct Choice
        {
            TerrainTileKey key;
            f64 distance;
        };

        Array<Choice> choices;
        for( const auto &tile : tiles )
        {
            bool wanted = false;
            u32 selected = terrainTileLods - 1;
            f64 distance = std::numeric_limits<f64>::infinity();
            const auto old = std::find_if( previous.begin(), previous.end(),
                                           [&]( auto k ) { return k.tile == tile.id; } );
            for( const auto &view : views )
            {
                if( !visible( tile, view ) )
                {
                    continue;
                }
                wanted = true;
                const auto d = distanceTo( tile, view );
                distance = std::min( distance, d );
                const f64 factor = std::abs( view.verticalScale ) *
                                   std::max( 1.0f, view.pixelsPerMetreAtUnitDistance ) / d;
                const auto threshold = std::max( 0.01f, view.maximumPixelError );
                u32 level = 0;
                if( !tile.hasHoles )
                {
                    for( u32 candidate = 1; candidate < terrainTileLods; ++candidate )
                    {
                        // Refinement at 100%; coarsening only below 75% avoids
                        // toggling around a threshold without exceeding the budget.
                        const auto hysteresis =
                            old != previous.end() && candidate <= old->lod ? 1.0 : 0.75;
                        if( tile.errors[candidate] * factor <= threshold * hysteresis )
                        {
                            level = candidate;
                        }
                    }
                }
                selected = std::min( selected, level );
            }
            if( wanted )
            {
                choices.push_back( { { tile.id, selected }, distance } );
            }
        }
        std::stable_sort( choices.begin(), choices.end(),
                          []( const auto &a, const auto &b ) { return a.distance < b.distance; } );
        Array<TerrainTileKey> result;
        for( size_t i = 0; i < std::min( size_t( maximumTiles ), choices.size() ); ++i )
        {
            result.push_back( choices[i].key );
        }
        return result;
    }

    u64 terrainMeshBytes( const TerrainMeshData &mesh )
    {
        return mesh.positions.size() * sizeof( Vector3F ) + mesh.normals.size() * sizeof( Vector3F ) +
               mesh.uvs.size() * sizeof( Vector2F ) + mesh.indices.size() * sizeof( u32 );
    }

    u64 terrainSourceBytes( const TerrainData &data )
    {
        u64 bytes = sizeof( TerrainData ) + data.heights.capacity() * sizeof( f32 ) +
                    data.layerWeights.capacity() * sizeof( Vector4F ) +
                    data.surfaceMasks.capacity() * sizeof( Vector2F ) +
                    data.holes.capacity() * sizeof( u8 ) + data.appearance.detailNormal.capacity();
        for( const auto &layer : data.layers )
        {
            bytes += layer.albedo.capacity() + layer.normal.capacity() + layer.orm.capacity();
        }
        return bytes;
    }

    struct TerrainTileStream::Impl
    {
        struct SourceOwner
        {
            StreamBudget lease;
            TerrainSnapshot source;
        };

        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        std::shared_ptr<WorldStreamProducer> producer;
        TerrainSnapshot source;
        Array<TerrainTileKey> demand;

        Impl()
        {
            String error;
            producer = scheduler->createProducer( {}, error );
            if( !producer )
            {
                throw std::runtime_error( error );
            }
        }

        ~Impl()
        {
            producer->drain();
        }
    };

    TerrainTileStream::TerrainTileStream() : m_impl( std::make_unique<Impl>() )
    {
    }

    TerrainTileStream::~TerrainTileStream() = default;

    bool TerrainTileStream::request( TerrainSnapshot data, const Array<TerrainTileInfo> &tiles,
                                     const Array<TerrainTileKey> &keys, String &error )
    {
        return requestWithSourceBudget( std::move( data ), tiles, keys, {}, error );
    }

    bool TerrainTileStream::requestWithSourceBudget(
        TerrainSnapshot data, const Array<TerrainTileInfo> &tiles,
        const Array<TerrainTileKey> &keys, std::shared_ptr<StreamBudgetLease> sourceBudget,
        String &error )
    {
        if( !data || keys.size() > 256 || tiles.size() > 2048 )
        {
            error = "Terrain stream requires a source, at most 2048 tiles and 256 requests";
            return false;
        }
        for( auto key : keys )
        {
            if( key.tile >= tiles.size() || key.lod >= terrainTileLods ||
                tiles[key.tile].id != key.tile )
            {
                error = "Invalid terrain stream tile request";
                return false;
            }
        }
        for( size_t i = 0; i < keys.size(); ++i )
        {
            if( std::find( keys.begin(), keys.begin() + i, keys[i] ) != keys.begin() + i )
            {
                error = "Duplicate terrain stream tile request";
                return false;
            }
        }
        auto &p = *m_impl;
        if( sourceBudget &&
            ( !p.scheduler->ownsBudget( sourceBudget ) ||
              sourceBudget->memoryClass() != StreamMemoryClass::SourceCpu ||
              sourceBudget->bytes() < terrainSourceBytes( *data ) ) )
        {
            error = "Terrain source lease must cover this snapshot in the shared SourceCpu ledger";
            return false;
        }
        if( data == p.source && keys == p.demand )
        {
            error.clear();
            return true;
        }
        try
        {
            TerrainSnapshot retained = p.source;
            if( data != retained )
            {
                auto owner = std::make_shared<Impl::SourceOwner>();
                owner->lease = sourceBudget ? std::move( sourceBudget ) :
                    p.scheduler->reserve( StreamMemoryClass::SourceCpu,
                                          terrainSourceBytes( *data ), error );
                if( !owner->lease )
                {
                    return false;
                }
                owner->source = data;
                retained = TerrainSnapshot( owner, data.get() );
            }
            auto demand = keys;
            Array<WorldStreamRequest> requests;
            requests.reserve( keys.size() );
            for( size_t i = 0; i < keys.size(); ++i )
            {
                const auto key = keys[i];
                const auto tile = tiles[key.tile];
                WorldStreamRequest request;
                request.key = ( u64( key.tile ) << 32 ) | key.lod;
                request.priority = s32( keys.size() - i );
                request.pendingBytes = sizeof( WorldStreamRequest ) + sizeof( TerrainTileInfo ) + 256;
                request.workingBytes = 1ull << 20;
                request.maximumResultBytes = 1ull << 20;
                request.execute = [retained, key, tile]( const StreamCancellation &cancel ) {
                    auto result = std::make_shared<TerrainTileResult>();
                    result->source = retained;
                    result->key = key;
                    auto mesh = std::make_shared<TerrainMeshData>();
                    if( buildTerrainTile( *retained, tile, key.lod, *mesh, result->error,
                                          [&] { return cancel.cancelled(); } ) )
                    {
                        result->mesh = std::move( mesh );
                    }
                    WorldStreamResult output;
                    output.payload = result;
                    if( result->mesh )
                    {
                        output.bytes = sizeof( TerrainTileResult ) + sizeof( TerrainMeshData ) +
                                       result->mesh->positions.capacity() * sizeof( Vector3F ) +
                                       result->mesh->normals.capacity() * sizeof( Vector3F ) +
                                       result->mesh->uvs.capacity() * sizeof( Vector2F ) +
                                       result->mesh->indices.capacity() * sizeof( u32 );
                    }
                    return output;
                };
                requests.push_back( std::move( request ) );
            }
            if( !p.producer->replace( requests, error ) )
            {
                return false;
            }
            p.source = std::move( retained );
            p.demand = std::move( demand );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool TerrainTileStream::take( TerrainTileResult &result )
    {
        WorldStreamResult ready;
        if( !m_impl->producer->take( ready ) )
        {
            return false;
        }
        if( !ready.payload )
        {
            result = {};
            result.source = m_impl->source;
            result.key = { u32( ready.key >> 32 ), u32( ready.key ) };
            result.error = ready.error;
            return true;
        }
        const auto tile = std::static_pointer_cast<const TerrainTileResult>( ready.payload );
        result = *tile;
        // Alias the complete scheduled payload, not just its mesh control block:
        // callers retaining a mesh also retain its shared ResultCpu reservation.
        if( tile->mesh )
        {
            result.mesh = std::shared_ptr<const TerrainMeshData>( ready.payload, tile->mesh.get() );
        }
        return true;
    }

    void TerrainTileStream::cancel()
    {
        m_impl->producer->cancel();
        m_impl->source.reset();
        m_impl->demand.clear();
    }

    void TerrainTileStream::drain()
    {
        m_impl->producer->drain();
        m_impl->source.reset();
        m_impl->demand.clear();
    }

    TerrainStreamStats TerrainTileStream::stats() const
    {
        const auto value = m_impl->producer->stats();
        TerrainStreamStats result;
        result.generation = value.generation;
        result.completed = value.completed;
        result.cancelled = value.cancelled;
        result.readyBytes = value.readyBytes;
        result.pending = value.pending;
        result.active = value.active;
        result.ready = value.ready;
        return result;
    }
}  // namespace workphone::render
