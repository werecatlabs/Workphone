#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CityRoadFootways.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>

namespace workphone::scene
{
    namespace
    {
        constexpr double gap = .05, width = 2.4, lift = .15, cellSize = 32;
        constexpr size_t maximumPieces = 12000, maximumVertices = 300000;
        constexpr u64 maximumWork = 32000000;
        struct Point { double x = 0, z = 0; };
        struct Bounds { double x0, z0, x1, z1; };
        struct Quad
        {
            std::array<Point, 4> points;
            Bounds bounds;
            bool valid = true;
            u32 stamp = 0;
        };
        void require( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
        void charge( u64 &work, u64 amount = 1 )
        {
            require( amount <= maximumWork - work, "Footway geometry work budget exceeded." ); work += amount;
        }
        bool finite( Vector3F p ) { return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z ); }
        Point point( Vector3F p ) { return { p.x, p.z }; }
        Point add( Point a, Point b, double scale ) { return { a.x + b.x * scale, a.z + b.z * scale }; }
        Point interpolate( Point a, Point b, double t ) { return { a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t }; }
        double cross( Point a, Point b, Point c ) { return ( b.x - a.x ) * ( c.z - a.z ) - ( b.z - a.z ) * ( c.x - a.x ); }
        Quad quad( Point a, Point b, Point c, Point d, bool unsupportedFold = false )
        {
            Quad q{ { a, b, c, d }, { a.x, a.z, a.x, a.z } };
            double winding = 0;
            for( size_t i = 0; i < 4; ++i )
            {
                const auto p = q.points[i];
                require( std::isfinite( p.x ) && std::isfinite( p.z ) && std::abs( p.x ) <= 1000000 && std::abs( p.z ) <= 1000000,
                         "Footway footprint has invalid or unbounded coordinates." );
                q.bounds.x0 = std::min( q.bounds.x0, p.x ); q.bounds.x1 = std::max( q.bounds.x1, p.x );
                q.bounds.z0 = std::min( q.bounds.z0, p.z ); q.bounds.z1 = std::max( q.bounds.z1, p.z );
                const double turn = cross( p, q.points[( i + 1 ) % 4], q.points[( i + 2 ) % 4] );
                q.valid &= std::abs( turn ) > 1e-8 && ( !winding || turn * winding > 0 );
                winding = turn;
            }
            require( q.valid || unsupportedFold, "Footway or road ribbon is folded or degenerate." );
            return q;
        }
        bool overlaps( const Quad &a, const Quad &b )
        {
            // Positive-area intersection only: contiguous four-metre pieces must
            // share their boundary without rejecting every second sidewalk tile.
            if( a.bounds.x1 <= b.bounds.x0 + 1e-6 || b.bounds.x1 <= a.bounds.x0 + 1e-6 ||
                a.bounds.z1 <= b.bounds.z0 + 1e-6 || b.bounds.z1 <= a.bounds.z0 + 1e-6 ) return false;
            for( const auto *q : { &a, &b } ) for( size_t i = 0; i < 4; ++i )
            {
                const auto p = q->points[i], n = q->points[( i + 1 ) % 4];
                const double nx = p.z - n.z, nz = n.x - p.x, tolerance = std::hypot( nx, nz ) * 1e-6;
                double amin = std::numeric_limits<double>::max(), amax = -amin, bmin = amin, bmax = -amin;
                for( auto v : a.points ) { const double d = v.x * nx + v.z * nz; amin = std::min( amin, d ); amax = std::max( amax, d ); }
                for( auto v : b.points ) { const double d = v.x * nx + v.z * nz; bmin = std::min( bmin, d ); bmax = std::max( bmax, d ); }
                if( amax <= bmin + tolerance || bmax <= amin + tolerance ) return false;
            }
            return true;
        }
        double segmentSquared( Point p, Point a, Point b )
        {
            const double x = b.x - a.x, z = b.z - a.z, size = x * x + z * z;
            const double t = size > 0 ? std::clamp( ( ( p.x - a.x ) * x + ( p.z - a.z ) * z ) / size, 0., 1. ) : 0;
            return std::pow( p.x - a.x - x * t, 2 ) + std::pow( p.z - a.z - z * t, 2 );
        }
        bool circleOverlap( const Quad &q, Point center, double radius )
        {
            if( center.x + radius < q.bounds.x0 || center.x - radius > q.bounds.x1 ||
                center.z + radius < q.bounds.z0 || center.z - radius > q.bounds.z1 ) return false;
            bool positive = false, negative = false;
            for( size_t i = 0; i < 4; ++i )
            {
                const auto a = q.points[i], b = q.points[( i + 1 ) % 4];
                const double turn = cross( a, b, center ); positive |= turn > 0; negative |= turn < 0;
                if( segmentSquared( center, a, b ) < radius * radius ) return true;
            }
            return !( positive && negative );
        }
        class Index
        {
        public:
            void insert( Quad q, u64 &work )
            {
                require( quads.size() < 65536, "Footway spatial obstacle budget exceeded." );
                const auto id = static_cast<u32>( quads.size() ); quads.push_back( q );
                cellsFor( q.bounds, [&]( auto key ) {
                    charge( work ); require( ++entries <= 500000, "Footway spatial index budget exceeded." );
                    cells[key].push_back( id );
                } );
            }
            bool intersects( const Quad &q, u64 &work )
            {
                ++stamp; bool hit = false;
                cellsFor( q.bounds, [&]( auto key ) {
                    if( hit ) return;
                    const auto found = cells.find( key ); if( found == cells.end() ) return;
                    for( auto id : found->second )
                    {
                        auto &other = quads[id]; if( other.stamp == stamp ) continue; other.stamp = stamp;
                        charge( work ); if( overlaps( q, other ) ) { hit = true; break; }
                    }
                } );
                return hit;
            }
        private:
            template<class F> static void cellsFor( const Bounds &b, F f )
            {
                const auto x0 = static_cast<s32>( std::floor( b.x0 / cellSize ) ), x1 = static_cast<s32>( std::floor( b.x1 / cellSize ) );
                const auto z0 = static_cast<s32>( std::floor( b.z0 / cellSize ) ), z1 = static_cast<s32>( std::floor( b.z1 / cellSize ) );
                require( s64( x1 - x0 + 1 ) * ( z1 - z0 + 1 ) <= 4096, "Footway spatial footprint exceeds its bounded index." );
                for( auto z = z0; z <= z1; ++z ) for( auto x = x0; x <= x1; ++x ) f( std::make_pair( x, z ) );
            }
            SourceBudgetVector<Quad> quads;
            SourceBudgetMap<std::pair<s32, s32>, SourceBudgetVector<u32>> cells;
            u32 stamp = 0; size_t entries = 0;
        };
        size_t straightEnd( const SourceBudgetVector<procedural::CityRoadSplineSample> &p, size_t first )
        {
            const auto a = p[first].position, b = p[first + 1].position;
            const double length = std::hypot( double( b.x ) - a.x, double( b.z ) - a.z );
            require( length > 1e-6, "Footway road bake has a zero horizontal segment." );
            const Point direction{ ( b.x - a.x ) / length, ( b.z - a.z ) / length };
            size_t last = first; std::array<double, 3> previous{};
            for( size_t i = first + 1; i < p.size(); ++i )
            {
                const std::array<Vector3F, 3> begin = { p[first].position, p[first].left, p[first].right };
                const std::array<Vector3F, 3> end = { p[i].position, p[i].left, p[i].right };
                if( std::hypot( double( end[0].x ) - begin[0].x, double( end[0].z ) - begin[0].z ) > 64.0001 ) break;
                std::array<double, 3> progress{}; bool valid = true;
                for( size_t j = 0; j < 3; ++j )
                {
                    const double x = double( end[j].x ) - begin[j].x, z = double( end[j].z ) - begin[j].z;
                    progress[j] = x * direction.x + z * direction.z;
                    valid &= progress[j] > previous[j] && std::abs( x * direction.z - z * direction.x ) <= .0002;
                }
                if( !valid ) break; previous = progress; last = i;
            }
            return std::max( first + 1, last );
        }
        bool unsupportedTerrain( const String &error, u64 denials )
        {
            if( sourceBudgetDenialCount() != denials || error == std::bad_alloc().what() ) return false;
            const std::string_view message( error.data(), error.size() );
            return message == "Terrain sample is outside, a hole or unavailable." ||
                message == "Road profile exceeds grade or has zero horizontal span." ||
                message == "A road or junction footprint intersects a terrain hole." ||
                message == "Road or junction footprint extends outside its bound terrain." ||
                message == "Road footprint is not completely supported by terrain cells." ||
                message == "Covered terrain or junction carriageway exceeds the road grade limit.";
        }
        using PositionKey = std::array<s64, 3>;
        PositionKey positionKey( Vector3F p )
        { return { std::llround( double( p.x ) * 1000000 ), std::llround( double( p.y ) * 1000000 ), std::llround( double( p.z ) * 1000000 ) }; }
        struct Boundary { u32 a = 0, b = 0, count = 0; };
        class BoundaryTopology
        {
        public:
            u32 node( Vector3F p, u64 &work )
            {
                const auto key = positionKey( p ); const auto cached = exact.find( key );
                if( cached != exact.end() ) return cached->second;
                // A city-scale F32 coordinate has an ULP much larger than1um.
                // Search neighbouring bins, rather than rounding to a coarser key
                // whose own bucket boundary would merely move the same defect.
                constexpr double tolerance = .0002;
                const s64 x = s64( std::floor( double( p.x ) / tolerance ) ), z = s64( std::floor( double( p.z ) / tolerance ) );
                u32 selected = std::numeric_limits<u32>::max();
                for( s64 dz = -1; dz <= 1; ++dz ) for( s64 dx = -1; dx <= 1; ++dx )
                {
                    charge( work ); const auto found = cells.find( { x + dx, z + dz } );
                    if( found == cells.end() ) continue;
                    for( const auto index : found->second )
                    {
                        charge( work ); const auto q = positions[index];
                        const double ox = double( p.x ) - q.x, oy = double( p.y ) - q.y, oz = double( p.z ) - q.z;
                        if( ox * ox + oy * oy + oz * oz <= tolerance * tolerance ) selected = std::min( selected, index );
                    }
                }
                if( selected == std::numeric_limits<u32>::max() )
                {
                    require( positions.size() < maximumVertices, "Footway boundary topology budget exceeded." );
                    selected = u32( positions.size() ); positions.push_back( p ); cells[{ x, z }].push_back( selected );
                }
                exact.emplace( key, selected ); return selected;
            }
        private:
            SourceBudgetVector<Vector3F> positions;
            SourceBudgetMap<PositionKey, u32> exact;
            SourceBudgetMap<std::pair<s64, s64>, SourceBudgetVector<u32>> cells;
        };
        class Builder
        {
        public:
            Builder( const procedural::CityRoadSplineSource &source, render::TerrainSnapshot terrain,
                     const Transform3<real_Num> &placement ) : terrain( terrain ), placement( placement )
            {
                mesh.sourceRevision = source.revision; mesh.terrainRevision = terrain->revision;
                pending.reserve( 128 );
            }
            void piece( const Quad &q, float grade, const procedural::CityRoadSplineSource &source )
            {
                require( ++pieces <= maximumPieces, "Footway candidate budget exceeds12000 pieces." );
                if( !q.valid ) return; // A valid tight road can have an unsupported inner sidewalk radius.
                if( obstacles.intersects( q, work ) || accepted.intersects( q, work ) ) return;
                for( const auto &junction : source.roundabouts )
                {
                    charge( work ); if( circleOverlap( q, point( junction.center ), junction.outerRadius ) ) return;
                }
                const auto start = interpolate( q.points[0], q.points[1], .5 );
                const auto end = interpolate( q.points[3], q.points[2], .5 );
                // The canonical codec requires distinct knots by more than1cm.
                // Tiny curved sample slivers cannot form an independent sweep.
                if( std::hypot( end.x - start.x, end.z - start.z ) <= .011 ) return;
                // Estimate the exact helper's local cell visits before entering
                // it, so repeated unsupported pieces cannot consume unbounded CPU.
                Bounds local{ std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
                              -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max() };
                const auto inverse = placement.getOrientation().inverse(); const auto scale = placement.getScale();
                for( auto p : q.points )
                {
                    const auto v = inverse * ( Vector3F( float( p.x ), 0, float( p.z ) ) - placement.getPosition() );
                    const double x = v.x / scale.x, z = v.z / scale.z;
                    local.x0 = std::min( local.x0, x ); local.x1 = std::max( local.x1, x );
                    local.z0 = std::min( local.z0, z ); local.z1 = std::max( local.z1, z );
                }
                const double cells = ( std::ceil( ( local.x1 - local.x0 ) / terrain->spacing.x ) + 2 ) *
                                     ( std::ceil( ( local.z1 - local.z0 ) / terrain->spacing.y ) + 2 );
                require( std::isfinite( cells ) && cells <= maximumWork, "Footway terrain-fit work budget exceeded." );
                pending.push_back( { q, grade, u64( cells ) * 2 } );
                // Reserve geometry before terrain fitting so two neighbouring
                // candidates never produce coincident tops. An unsupported piece
                // can conservatively reserve its local corner, but emits nothing.
                accepted.insert( q, work );
                if( pending.size() == 128 ) flush();
            }
            void finish()
            {
                flush();
                const size_t topVertices = mesh.vertices.size();
                SourceBudgetMap<std::array<s64, 5>, u32> skirts;
                const auto skirtVertex = [&]( Vector3F position, Vector3F normal, u32 node, u32 level ) {
                    const std::array<s64, 5> key{ node, level, std::llround( normal.x * 10000. ),
                        std::llround( normal.y * 10000. ), std::llround( normal.z * 10000. ) };
                    auto found = skirts.find( key );
                    if( found == skirts.end() )
                    {
                        if( mesh.vertices.size() >= maximumVertices )
                            throw std::runtime_error( "Footway skirts exceed300000 vertices; top=" + std::to_string( topVertices ) +
                                " skirt=" + std::to_string( mesh.vertices.size() - topVertices ) );
                        const auto index = u32( mesh.vertices.size() );
                        // The tangent follows the directed boundary. World UVs
                        // permit sharing coplanar skirt endpoints across cells,
                        // while a normal change still retains a hard kerb corner.
                        const float u = ( position.x * normal.z - position.z * normal.x ) * .25f;
                        mesh.vertices.push_back( { position, normal, { u, position.y * .25f } } );
                        found = skirts.emplace( key, index ).first;
                    }
                    return found->second;
                };
                for( const auto &[key, edge] : boundary ) if( edge.count == 1 )
                {
                    const auto a = mesh.vertices[edge.a].position, b = mesh.vertices[edge.b].position;
                    const auto delta = b - a; const double length = std::hypot( delta.x, delta.z );
                    if( length < .0001 ) continue;
                    const Vector3F normal{ float( -delta.z / length ), 0, float( delta.x / length ) };
                    const auto na = topology.node( a, work ), nb = topology.node( b, work );
                    const auto ta = skirtVertex( a, normal, na, 1 ), ba = skirtVertex( a - Vector3F( 0, float( lift ), 0 ), normal, na, 0 );
                    const auto bb = skirtVertex( b - Vector3F( 0, float( lift ), 0 ), normal, nb, 0 ), tb = skirtVertex( b, normal, nb, 1 );
                    mesh.indices.insert( mesh.indices.end(), { ta, ba, bb, ta, bb, tb } );
                    mesh.triangleKinds.insert( mesh.triangleKinds.end(), 2, CityRoadTerrainTriangleKind::Kerb ); charge( work );
                }
            }
            Index obstacles, accepted; CityRoadTerrainMesh mesh; u64 work = 0;
        private:
            struct Pending { Quad quad; float grade; u64 cellVisits; };
            static SourceBudgetText temporaryId( u32 value )
            {
                SourceBudgetText id = "b1000000-0000-4000-8000-000000000000";
                constexpr char hex[] = "0123456789abcdef";
                for( size_t i = 0; i < 8; ++i ) { id[id.size() - 1 - i] = hex[value & 15]; value >>= 4; }
                return id;
            }
            void flush()
            {
                if( pending.empty() ) return;
                fitBatch( 0, pending.size() ); pending.clear();
            }
            size_t coalescedEnd( size_t first, size_t limit, Quad &footprint )
            {
                footprint = pending[first].quad;
                const auto inner = footprint.points[0], outer = footprint.points[1];
                const auto end = footprint.points[3]; const double length = std::hypot( end.x - inner.x, end.z - inner.z );
                const Point axis{ ( end.x - inner.x ) / length, ( end.z - inner.z ) / length };
                const auto progress = [&]( Point a, Point origin ) { return ( a.x - origin.x ) * axis.x + ( a.z - origin.z ) * axis.z; };
                const auto onLine = [&]( Point a, Point origin ) { return std::abs( ( a.x - origin.x ) * axis.z - ( a.z - origin.z ) * axis.x ) <= .0002; };
                if( !onLine( footprint.points[2], outer ) ) return first + 1;
                size_t next = first + 1;
                for( ; next < limit; ++next )
                {
                    charge( work ); const auto &candidate = pending[next]; const auto &q = candidate.quad;
                    if( std::abs( candidate.grade - pending[first].grade ) > 1e-7f ||
                        std::hypot( q.points[0].x - footprint.points[3].x, q.points[0].z - footprint.points[3].z ) > .0002 ||
                        std::hypot( q.points[1].x - footprint.points[2].x, q.points[1].z - footprint.points[2].z ) > .0002 ||
                        std::hypot( q.points[3].x - inner.x, q.points[3].z - inner.z ) > 64.0001 ||
                        std::hypot( q.points[2].x - outer.x, q.points[2].z - outer.z ) > 64.0001 ||
                        !onLine( q.points[0], inner ) || !onLine( q.points[3], inner ) ||
                        !onLine( q.points[1], outer ) || !onLine( q.points[2], outer ) ||
                        progress( q.points[3], inner ) <= progress( footprint.points[3], inner ) ||
                        progress( q.points[2], outer ) <= progress( footprint.points[2], outer ) ) break;
                    footprint = quad( inner, outer, q.points[2], q.points[3] );
                }
                return next;
            }
            void fitBatch( size_t first, size_t count )
            {
                require( count && count <= 128, "Footway terrain batch exceeds128 pieces." );
                // validateTerrainPlacement scans retained heights and their
                // extrema. Charge both complete passes on every retry as well;
                // hostile all-hole inputs cannot hide billions of validation visits.
                charge( work, u64( terrain->heights.size() ) * 2 );
                for( size_t i = first; i < first + count; ++i ) charge( work, pending[i].cellVisits );
                CityRoadTerrainMesh surface; bool supported = false;
                {
                    procedural::CityRoadSplineSource strip; procedural::CityRoadSplineBake stripBake;
                    strip.version = 2; strip.id = temporaryId( 1 ); strip.terrainResourceUuid = temporaryId( 2 );
                    strip.revision = stripBake.sourceRevision = mesh.sourceRevision;
                    strip.roads.reserve( count ); stripBake.roads.reserve( count );
                    for( size_t i = first; i < first + count; )
                    {
                        const auto &piece = pending[i]; Quad footprint;
                        const size_t next = coalescedEnd( i, first + count, footprint );
                        const auto ordinal = u32( strip.roads.size() );
                        procedural::CityRoadSpline road; procedural::CityRoadSplineSamples samples;
                        road.id = temporaryId( ordinal * 3 + 3 ); road.width = float( width ); road.maxGrade = piece.grade;
                        samples.roadId = road.id; samples.points.resize( 2 ); road.knots.resize( 2 );
                        for( size_t end = 0; end < 2; ++end )
                        {
                            const auto a = footprint.points[end ? 3 : 0], b = footprint.points[end ? 2 : 1];
                            auto &sample = samples.points[end];
                            sample.left = { float( a.x ), 0, float( a.z ) }; sample.right = { float( b.x ), 0, float( b.z ) };
                            sample.position = ( sample.left + sample.right ) * .5f;
                            road.knots[end].position = sample.position;
                            road.knots[end].id = temporaryId( ordinal * 3 + 4 + u32( end ) );
                        }
                        const float span = ( samples.points[1].position - samples.points[0].position ).length();
                        require( span > .01f, "Footway piece has insufficient horizontal span." );
                        samples.points[1].distance = span;
                        strip.roads.push_back( std::move( road ) ); stripBake.roads.push_back( std::move( samples ) );
                        i = next;
                    }
                    String error; const auto denials = sourceBudgetDenialCount();
                    supported = buildCityRoadTerrainMesh( strip, stripBake, terrain, placement, surface, error );
                    if( !supported && !unsupportedTerrain( error, denials ) ) throw std::runtime_error( error.c_str() );
                }
                if( supported ) { append( surface ); return; }
                if( count == 1 ) return;
                const size_t half = count / 2;
                fitBatch( first, half ); fitBatch( first + half, count - half );
            }
            void append( const CityRoadTerrainMesh &surface )
            {
                require( !surface.indices.empty(), "Footway terrain fitting returned an empty supported batch." );
                // Weld each batch's repeated cell-triangle vertices. UVs use the
                // world surface so adjoining pieces have continuous concrete scale.
                SourceBudgetMap<std::array<s64, 6>, u32> welded;
                SourceBudgetVector<u32> remap; remap.reserve( surface.vertices.size() );
                for( auto vertex : surface.vertices )
                {
                    const auto p = positionKey( vertex.position );
                    const std::array<s64, 6> key{ p[0], p[1], p[2], std::llround( vertex.normal.x * 10000. ),
                        std::llround( vertex.normal.y * 10000. ), std::llround( vertex.normal.z * 10000. ) };
                    auto found = welded.find( key );
                    if( found == welded.end() )
                    {
                        require( mesh.vertices.size() < maximumVertices, "Footway output exceeds300000 vertices." );
                        const auto index = u32( mesh.vertices.size() );
                        vertex.position.y += float( lift ); vertex.uv = { vertex.position.x * .25f, vertex.position.z * .25f };
                        mesh.vertices.push_back( vertex ); found = welded.emplace( key, index ).first;
                    }
                    remap.push_back( found->second );
                }
                for( size_t i = 0; i < surface.indices.size(); i += 3 )
                {
                    std::array<u32, 3> triangle;
                    for( size_t j = 0; j < 3; ++j ) triangle[j] = remap.at( surface.indices[i + j] );
                    if( triangle[0] == triangle[1] || triangle[1] == triangle[2] || triangle[0] == triangle[2] ) continue;
                    mesh.indices.insert( mesh.indices.end(), triangle.begin(), triangle.end() );
                    mesh.triangleKinds.push_back( CityRoadTerrainTriangleKind::Kerb );
                    const std::array<u32, 3> nodes{ topology.node( mesh.vertices[triangle[0]].position, work ),
                        topology.node( mesh.vertices[triangle[1]].position, work ), topology.node( mesh.vertices[triangle[2]].position, work ) };
                    // Keep exact fitted triangles, but an epsilon-thin sliver
                    // contributes no separate boundary after endpoint matching.
                    if( nodes[0] == nodes[1] || nodes[1] == nodes[2] || nodes[0] == nodes[2] ) continue;
                    for( size_t j = 0; j < 3; ++j )
                    {
                        const auto a = triangle[j], b = triangle[( j + 1 ) % 3];
                        const auto ka = nodes[j], kb = nodes[( j + 1 ) % 3];
                        if( ka == kb ) continue;
                        const auto key = ka < kb ? std::make_pair( ka, kb ) : std::make_pair( kb, ka );
                        auto &edge = boundary[key]; if( !edge.count ) { edge.a = a; edge.b = b; } ++edge.count;
                        require( edge.count <= 2, "Footway supported pieces overlap or have nonmanifold edges." );
                        charge( work );
                    }
                }
            }
            render::TerrainSnapshot terrain; const Transform3<real_Num> &placement;
            SourceBudgetVector<Pending> pending;
            BoundaryTopology topology;
            SourceBudgetMap<std::pair<u32, u32>, Boundary> boundary;
            size_t pieces = 0;
        };
    }

    bool buildCityRoadFootways( const procedural::CityRoadSplineSource &source,
        const procedural::CityRoadSplineBake &bake, render::TerrainSnapshot terrain,
        const Transform3<real_Num> &placement, CityRoadTerrainMesh &output, String &error )
    {
        try
        {
            if( !procedural::validateCityRoadSplineSource( source, error ) ) throw std::runtime_error( error.c_str() );
            require( terrain && bake.sourceRevision == source.revision && bake.roads.size() == source.roads.size(),
                     "Footways require retained terrain and a current canonical road bake." );
            if( !render::validateTerrainPlacement( *terrain, placement, error ) ) throw std::runtime_error( error.c_str() );
            Builder builder( source, terrain, placement );
            struct Run { size_t road, first, last; }; SourceBudgetVector<Run> runs;
            size_t sampleCount = 0;
            for( size_t r = 0; r < source.roads.size(); ++r )
            {
                const auto &road = source.roads[r]; const auto &samples = bake.roads[r];
                require( road.heightMode != procedural::CityRoadHeightMode::Bridge, "Bridge footways are unavailable." );
                require( samples.roadId == road.id && samples.points.size() >= 2, "Footway road samples are missing or mismatched." );
                sampleCount += samples.points.size(); require( sampleCount <= 65536, "Footway canonical sample budget exceeded." );
                for( size_t i = 0; i < samples.points.size(); ++i )
                {
                    const auto &p = samples.points[i];
                    require( finite( p.position ) && finite( p.left ) && finite( p.right ) && std::isfinite( p.distance ) &&
                        p.distance >= 0 && ( !i || p.distance > samples.points[i - 1].distance ), "Footway road bake has nonfinite or unordered samples." );
                    require( std::abs( std::hypot( double( p.left.x ) - p.right.x, double( p.left.z ) - p.right.z ) - road.width ) < .02,
                             "Footway road cross-section does not match its authored width." );
                }
                for( size_t first = 0; first + 1 < samples.points.size(); )
                {
                    const auto last = straightEnd( samples.points, first ); const auto &a = samples.points[first], &b = samples.points[last];
                    builder.obstacles.insert( quad( point( a.left ), point( a.right ), point( b.right ), point( b.left ) ), builder.work );
                    runs.push_back( { r, first, last } ); first = last;
                }
            }
            for( const auto &junction : source.roundabouts ) for( const auto &port : junction.ports )
            {
                const double run = std::hypot( port.outward.x, port.outward.z );
                require( std::isfinite( run ) && run > .001, "Footway junction has an invalid approach direction." );
                const Point direction{ port.outward.x / run, port.outward.z / run }, across{ -direction.z, direction.x };
                const auto a = add( point( junction.center ), direction, std::sqrt( junction.outerRadius * junction.outerRadius - port.width * port.width * .25 ) );
                const auto b = add( point( junction.center ), direction, junction.outerRadius + port.length );
                builder.obstacles.insert( quad( add( a, across, -port.width * .5 ), add( a, across, port.width * .5 ),
                    add( b, across, port.width * .5 ), add( b, across, -port.width * .5 ) ), builder.work );
            }
            const auto strip = [&]( Point ai, Point ao, Point bi, Point bo, float grade ) {
                const double length = std::max( std::hypot( bi.x - ai.x, bi.z - ai.z ), std::hypot( bo.x - ao.x, bo.z - ao.z ) );
                require( std::isfinite( length ) && length < 1000000, "Footway strip length is invalid." );
                const size_t count = std::max<size_t>( 1, size_t( std::ceil( length / 4. ) ) );
                require( count <= maximumPieces, "Footway strip exceeds its candidate budget." );
                for( size_t i = 0; i < count; ++i )
                {
                    const double t0 = double( i ) / count, t1 = double( i + 1 ) / count;
                    builder.piece( quad( interpolate( ai, bi, t0 ), interpolate( ao, bo, t0 ),
                        interpolate( ao, bo, t1 ), interpolate( ai, bi, t1 ), true ), grade, source );
                }
            };
            for( const auto &run : runs )
            {
                const auto &road = source.roads[run.road]; const auto &points = bake.roads[run.road].points;
                const auto &a = points[run.first], &b = points[run.last];
                const auto across = []( const procedural::CityRoadSplineSample &p ) {
                    const double span = std::hypot( double( p.right.x ) - p.left.x, double( p.right.z ) - p.left.z );
                    return Point{ ( p.right.x - p.left.x ) / span, ( p.right.z - p.left.z ) / span };
                };
                const auto na = across( a ), nb = across( b );
                for( int side : { -1, 1 } )
                {
                    const auto ea = point( side < 0 ? a.left : a.right ), eb = point( side < 0 ? b.left : b.right );
                    strip( add( ea, na, side * gap ), add( ea, na, side * ( gap + width ) ),
                           add( eb, nb, side * gap ), add( eb, nb, side * ( gap + width ) ), road.maxGrade );
                }
            }
            for( const auto &junction : source.roundabouts )
            {
                struct PortWalk
                {
                    Point direction, across;
                    double angle, innerAngle, outerAngle;
                    std::array<Point, 2> inner, outer; // Negative/positive side of the approach.
                };
                std::array<PortWalk, 6> ports{};
                constexpr double circle = 6.283185307179586;
                const double innerRadius = junction.outerRadius + gap, outerRadius = innerRadius + width;
                const auto center = point( junction.center );
                for( size_t i = 0; i < junction.ports.size(); ++i )
                {
                    const auto &port = junction.ports[i]; auto &walk = ports[i];
                    const double run = std::hypot( port.outward.x, port.outward.z );
                    walk.direction = { port.outward.x / run, port.outward.z / run }; walk.across = { -walk.direction.z, walk.direction.x };
                    walk.angle = std::atan2( walk.direction.z, walk.direction.x ); if( walk.angle < 0 ) walk.angle += circle;
                    const double innerAcross = port.width * .5 + gap, outerAcross = innerAcross + width;
                    walk.innerAngle = std::asin( innerAcross / innerRadius ); walk.outerAngle = std::asin( outerAcross / outerRadius );
                    const auto innerBase = add( center, walk.direction, std::sqrt( innerRadius * innerRadius - innerAcross * innerAcross ) );
                    const auto outerBase = add( center, walk.direction, std::sqrt( outerRadius * outerRadius - outerAcross * outerAcross ) );
                    for( int side : { -1, 1 } )
                    {
                        const size_t index = side < 0 ? 0 : 1;
                        walk.inner[index] = add( innerBase, walk.across, side * innerAcross );
                        walk.outer[index] = add( outerBase, walk.across, side * outerAcross );
                        const auto end = add( center, walk.direction, junction.outerRadius + port.length );
                        // Both circular and straight domains share this exact
                        // skew cross-section; whole-quad mouth culling previously
                        // left an unbridgeable grass gap between these surfaces.
                        strip( walk.inner[index], walk.outer[index], add( end, walk.across, side * innerAcross ),
                               add( end, walk.across, side * outerAcross ), junction.maxGrade );
                    }
                }
                const size_t portCount = junction.ports.size();
                std::sort( ports.begin(), ports.begin() + portCount,
                    []( const PortWalk &a, const PortWalk &b ) { return a.angle < b.angle; } );
                for( size_t i = 0; i < portCount; ++i )
                {
                    const auto &a = ports[i], &b = ports[( i + 1 ) % portCount];
                    const double nextAngle = b.angle + ( i + 1 == portCount ? circle : 0 );
                    const double innerStart = a.angle + a.innerAngle, innerEnd = nextAngle - b.innerAngle;
                    const double outerStart = a.angle + a.outerAngle, outerEnd = nextAngle - b.outerAngle;
                    // Acute authored approaches may not admit two full-width
                    // walks. Keep that sector unsupported rather than cross a road.
                    if( innerEnd <= innerStart || outerEnd <= outerStart ) continue;
                    const size_t count = std::max<size_t>( 1, size_t( std::ceil( std::max( ( innerEnd - innerStart ) * innerRadius,
                        ( outerEnd - outerStart ) * outerRadius ) / 3.5 ) ) );
                    require( count <= maximumPieces, "Roundabout footway sector exceeds its candidate budget." );
                    const double step = ( innerEnd - innerStart ) / count;
                    Point previousInner = a.inner[1], previousOuter = a.outer[1];
                    for( size_t segment = 0; segment <= count; ++segment )
                    {
                        Point nextInner = b.inner[0], nextOuter = b.outer[0];
                        if( segment < count )
                        {
                            const double t = ( segment + .5 ) / count, innerAngle = innerStart + ( segment + .5 ) * step;
                            const double outerAngle = outerStart + t * ( outerEnd - outerStart );
                            // Tangent intersections circumscribe the inner offset
                            // circle, including exact endpoint tangent contacts.
                            const double inner = innerRadius / std::cos( step * .5 );
                            nextInner = add( center, { std::cos( innerAngle ), std::sin( innerAngle ) }, inner );
                            nextOuter = add( center, { std::cos( outerAngle ), std::sin( outerAngle ) }, outerRadius );
                        }
                        builder.piece( quad( previousInner, previousOuter, nextOuter, nextInner, true ), junction.maxGrade, source );
                        previousInner = nextInner; previousOuter = nextOuter;
                    }
                }
            }
            builder.finish(); output.swap( builder.mesh ); error.clear(); return true;
        }
        catch( const std::exception &exception ) { error = exception.what(); return false; }
    }
}
