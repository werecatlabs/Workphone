#pragma once

// Included by the race scene builder alongside its private mesh/material helpers.
namespace cityVerge
{
    using Cell = std::pair<s32, s32>;

    Cell cellAt( Vector3F point )
    {
        return { s32( std::floor( point.x / 8.f ) ), s32( std::floor( point.z / 8.f ) ) };
    }

    float crossXZ( Vector3F a, Vector3F b )
    {
        return a.x * b.z - a.z * b.x;
    }

    struct RoadSpan
    {
        Vector3F a, b;
        float radius, distance;
        size_t road;
    };

    struct RoadEnd
    {
        Vector3F centre, direction;
        float halfWidth, distance;
        size_t road;
    };

    struct SurfaceCell
    {
        SourceBudgetVector<u32> walkTriangles, roadSpans, roadEnds;
    };

    using Cells = SourceBudgetMap<Cell, SurfaceCell>;

    template <class Add>
    void indexBounds( Cells &cells, Vector3F a, Vector3F b, Vector3F c, float margin, const Add &add )
    {
        const auto low = cellAt(
            { std::min( { a.x, b.x, c.x } ) - margin, 0, std::min( { a.z, b.z, c.z } ) - margin } );
        const auto high = cellAt(
            { std::max( { a.x, b.x, c.x } ) + margin, 0, std::max( { a.z, b.z, c.z } ) + margin } );
        for( auto x = low.first; x <= high.first; ++x )
        {
            for( auto z = low.second; z <= high.second; ++z )
            {
                add( cells[{ x, z }] );
            }
        }
    }

    float distanceToSpanSquared( Vector3F point, const RoadSpan &span )
    {
        auto direction = span.b - span.a;
        direction.y = 0;
        auto offset = point - span.a;
        offset.y = 0;
        const auto lengthSquared = direction.lengthSquared();
        if( lengthSquared > 0 )
        {
            offset -= direction * std::clamp( offset.dotProduct( direction ) / lengthSquared, 0.f, 1.f );
        }
        return offset.lengthSquared();
    }

    float smooth( float t )
    {
        t = std::clamp( t, 0.f, 1.f );
        return t * t * ( 3.f - 2.f * t );
    }

    float unit( u64 key )
    {
        return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
    }

    float noise( u32 seed, float x, float y, int period )
    {
        const auto ix = int( std::floor( x ) ), iy = int( std::floor( y ) );
        const auto fx = smooth( x - ix ), fy = smooth( y - iy );
        const auto value = [=]( int dx, int dy ) {
            const auto column = u32( ( ix + dx ) % period );
            const auto row = u32( iy + dy );
            return unit( ( u64( row ) << 32 ) | ( column ^ seed ) );
        };
        return ( value( 0, 0 ) * ( 1.f - fx ) + value( 1, 0 ) * fx ) * ( 1.f - fy ) +
               ( value( 0, 1 ) * ( 1.f - fx ) + value( 1, 1 ) * fx ) * fy;
    }

    TextureBuffer makeTexture( u32 seed )
    {
        // U repeats along the pavement. V holds a narrow damp seam, exposed silty
        // soil, and short grass that breaks up into the existing terrain surface.
        TextureBuffer texture( 512, 64 );
        for( u32 y = 0; y < texture.height; ++y )
        {
            const auto across = float( y ) / float( texture.height - 1 );
            for( u32 x = 0; x < texture.width; ++x )
            {
                const float along = float( x ) / float( texture.width );
                const float patches = noise( seed, along * 11.f, across * 4.f, 11 );
                const float crumbs = noise( seed + 1, along * 97.f, across * 19.f, 97 );
                const float grains = noise( seed + 2, along * 233.f, across * 59.f, 233 );
                const float cover = smooth( ( patches * .72f + crumbs * .28f - .34f ) / .35f );
                const float seam =
                    ( 1.f - smooth( across / ( .065f + .14f * cover ) ) ) * ( .26f + .74f * cover );
                const float damp = seam * smooth( ( cover - .55f ) / .35f );
                const float grass = smooth( ( across + .31f * patches + .24f * crumbs - .45f ) / .47f );
                const float reach = .35f + .48f * cover + .13f * crumbs;
                const float fade = 1.f - smooth( ( across - reach * .36f ) / ( reach * .64f ) );
                const float density = .12f + .47f * cover + .28f * seam + .12f * damp;
                const float flecks = 24.f * ( grains - .5f ) + 24.f * ( crumbs - .5f );
                auto pixel = texture.pixel( x, y );
                const auto channel = [&]( float soil, float leaf ) {
                    return u8( std::clamp(
                        soil * ( 1.f - grass ) + leaf * grass + flecks - seam * 10.f - damp * 13.f, 0.f,
                        255.f ) );
                };
                pixel[0] = channel( 123.f, 109.f );
                pixel[1] = channel( 102.f, 101.f );
                pixel[2] = channel( 76.f, 64.f );
                pixel[3] = u8( 255.f * std::clamp( fade * density, 0.f, 1.f ) );
            }
        }
        return texture;
    }

    struct Edge
    {
        Vector3F a, b, outward;
        u32 terminal = std::numeric_limits<u32>::max();
    };

    Cell joinKey( Vector3F point )
    {
        return { s32( std::lround( point.x * 100.f ) ), s32( std::lround( point.z * 100.f ) ) };
    }

    template <size_t Count, class Clear>
    void foldedGrassBlade( Geometry &geometry, const std::array<Vector3F, Count> &spine,
                           const std::array<float, Count - 1> &widths, Vector3F across,
                           const Clear &isClear )
    {
        std::array<std::array<Vector3F, 3>, Count - 1> sections;
        const auto facing = across.crossProduct( spine.back() - spine.front() ).normaliseCopy();
        // The fold gives the blade a visible cross section on grazing views.
        // Shared ridge vertices keep lighting soft across the fold.
        for( size_t i = 0; i < sections.size(); ++i )
        {
            const auto width = across * widths[i];
            auto &section = sections[i];
            section = { spine[i] - width, spine[i] + facing * ( widths[i] * .57f ), spine[i] + width };
            if( !isClear( section[0] ) || !isClear( section[1] ) || !isClear( section[2] ) )
            {
                return;
            }
        }
        const auto base = u32( geometry.positions.size() );
        const auto vertex = [&]( Vector3F point, Vector3F normal ) {
            geometry.positions.push_back( point );
            geometry.normals.push_back( normal.normaliseCopy() );
            geometry.uv.push_back( { 0, 0, 0 } );
            geometry.tangents.push_back( { across.x, across.y, across.z, 1 } );
        };
        const Vector3F up( 0, 1, 0 );
        for( size_t i = 0; i < sections.size(); ++i )
        {
            const auto along = ( spine[i + 1] - spine[i > 0 ? i - 1 : 0] ).normaliseCopy();
            const auto normal = up + across.crossProduct( along ) * .22f;
            vertex( sections[i][0], normal - across * .13f );
            vertex( sections[i][1], normal );
            vertex( sections[i][2], normal + across * .13f );
        }
        const auto tip = u32( geometry.positions.size() );
        vertex( spine.back(), up + facing * .15f );

        const auto triangle = [&]( u32 a, u32 b, u32 c ) {
            const auto ab = geometry.positions[b] - geometry.positions[a];
            const auto ac = geometry.positions[c] - geometry.positions[a];
            // The shading normals need not follow the fold direction.
            const auto outward = geometry.normals[a] + geometry.normals[b] + geometry.normals[c];
            const bool forward = ab.crossProduct( ac ).dotProduct( outward ) >= 0;
            geometry.indices.push_back( a );
            geometry.indices.push_back( forward ? b : c );
            geometry.indices.push_back( forward ? c : b );
        };
        for( size_t i = 1; i < sections.size(); ++i )
        {
            const u32 a = base + u32( 3 * ( i - 1 ) ), b = a + 3;
            triangle( a, a + 1, b + 1 );
            triangle( a, b + 1, b );
            triangle( a + 1, a + 2, b + 2 );
            triangle( a + 1, b + 2, b + 1 );
        }
        triangle( tip - 3, tip - 2, tip );
        triangle( tip - 2, tip - 1, tip );
    }
}  // namespace cityVerge

template <class RoadSurface>
void buildTerrainCityVergeTransitions( SceneAssets &assets, const CityRoadTerrainMesh &walks, u32 seed,
                                       VehicleAppearanceQuality quality, const RoadSurface &roadSurface )
{
    using namespace cityVerge;
    const auto ground = assets.roadAuthoring->getTerrainSnapshot();
    if( !ground || walks.indices.empty() )
    {
        return;
    }
    const auto &placement = assets.roadAuthoring->getTerrainPlacement();
    const auto &source = assets.roadAuthoring->getSource();
    const auto &roads = assets.roadAuthoring->getBake().roads;
    if( source.roads.size() != roads.size() )
    {
        return;
    }

    Cells cells;
    SourceBudgetVector<RoadSpan> spans;
    for( size_t i = 0; i < roads.size(); ++i )
    {
        const auto &points = roads[i].points;
        for( size_t j = 1; j < points.size(); ++j )
        {
            const RoadSpan span{ points[j - 1].position, points[j].position,
                                 source.roads[i].width * .5f + .08f,
                                 ( points[j - 1].distance + points[j].distance ) * .5f, i };
            const auto index = u32( spans.size() );
            spans.push_back( span );
            indexBounds( cells, span.a, span.b, span.b, span.radius,
                         [index]( auto &cell ) { cell.roadSpans.push_back( index ); } );
        }
    }
    constexpr u32 noTerminal = std::numeric_limits<u32>::max();
    SourceBudgetVector<RoadEnd> ends;
    for( size_t i = 0; i < roads.size(); ++i )
    {
        const auto &road = source.roads[i];
        const auto &positions = roads[i].points;
        if( road.closed || road.heightMode != CityRoadHeightMode::Drape || positions.size() < 2 ||
            road.width < 4.f || positions.back().distance - positions.front().distance < road.width )
        {
            continue;
        }
        for( const bool start : { true, false } )
        {
            const auto &sample = start ? positions.front() : positions.back();
            const auto found = cells.find( cellAt( sample.position ) );
            if( found == cells.end() ||
                std::any_of(
                    found->second.roadSpans.begin(), found->second.roadSpans.end(), [&]( u32 index ) {
                        const auto &span = spans[index];
                        return span.road != i && distanceToSpanSquared( sample.position, span ) <
                                                     span.radius * span.radius;
                    } ) )
            {
                continue;
            }
            auto direction = start ? -sample.tangent : sample.tangent;
            direction.y = 0;
            direction.normalise();
            const auto index = u32( ends.size() );
            ends.push_back( { sample.position, direction, road.width * .5f, sample.distance, i } );
            indexBounds( cells, sample.position, sample.position, sample.position,
                         road.width * .5f + 6.f,
                         [index]( auto &cell ) { cell.roadEnds.push_back( index ); } );
        }
    }
    SourceBudgetVector<Edge> edges;
    for( u32 i = 0; i + 2 < walks.indices.size(); i += 3 )
    {
        const auto &a = walks.vertices[walks.indices[i]];
        const auto &b = walks.vertices[walks.indices[i + 1]];
        const auto &c = walks.vertices[walks.indices[i + 2]];
        if( a.normal.y > .1f && b.normal.y > .1f && c.normal.y > .1f )
        {
            indexBounds( cells, a.position, b.position, c.position, 0,
                         [i]( auto &cell ) { cell.walkTriangles.push_back( i ); } );
            continue;
        }
        // Each exposed skirt starts with (top A, bottom A, bottom B).
        // Those bottom vertices match the surveyed pavement boundary,
        // including the arcs and clipped ends after junction union.
        if( std::abs( a.normal.y ) < .01f && std::abs( b.normal.y ) < .01f &&
            std::abs( c.normal.y ) < .01f && std::abs( a.position.x - b.position.x ) < .001f &&
            std::abs( a.position.z - b.position.z ) < .001f && a.position.y - b.position.y > .1f &&
            ( b.position - c.position ).lengthSquared() > .000025f )
        {
            edges.push_back( { b.position, c.position, a.normal } );
        }
    }

    size_t overlapPoints = 0, overlapQuads = 0;
    const auto clearOfCarriageway = [&]( Vector3F point ) {
        if( !roadSurface.contains( point ) )
        {
            return true;
        }
        ++overlapPoints;
        return false;
    };
    const auto isClear = [&]( Vector3F point ) {
        for( const auto &junction : source.roundabouts )
        {
            auto offset = point - junction.center;
            offset.y = 0;
            const float radius = junction.outerRadius + .08f;
            if( offset.lengthSquared() < radius * radius )
            {
                return false;
            }
        }
        const auto found = cells.find( cellAt( point ) );
        if( found == cells.end() )
        {
            return clearOfCarriageway( point );
        }
        for( const auto index : found->second.roadSpans )
        {
            const auto &span = spans[index];
            if( distanceToSpanSquared( point, span ) >= span.radius * span.radius )
            {
                continue;
            }
            bool beyondOval = false;
            for( const auto endIndex : found->second.roadEnds )
            {
                const auto &end = ends[endIndex];
                if( end.road != span.road ||
                    std::abs( span.distance - end.distance ) > 2.5f * end.halfWidth )
                {
                    continue;
                }
                const auto offset = point - end.centre;
                const auto ahead = offset.dotProduct( end.direction );
                const auto spanAhead = std::max( ( span.a - end.centre ).dotProduct( end.direction ),
                                                 ( span.b - end.centre ).dotProduct( end.direction ) );
                if( ahead <= std::max( 0.f, spanAhead ) )
                {
                    continue;
                }
                // A circular lane capsule would overhang the outer kerb on wide ends.
                // Preserve the shallow asphalt terminal oval instead of that overhang.
                const auto x = crossXZ( end.direction, offset ) / span.radius;
                const auto z = ahead / ( std::min( 3.f, end.halfWidth ) + .08f );
                if( x * x + z * z >= 1.f )
                {
                    beyondOval = true;
                    break;
                }
            }
            if( !beyondOval )
            {
                return false;
            }
        }
        for( const auto index : found->second.walkTriangles )
        {
            const auto &a = walks.vertices[walks.indices[index]].position;
            const auto &b = walks.vertices[walks.indices[index + 1]].position;
            const auto &c = walks.vertices[walks.indices[index + 2]].position;
            const float ab = crossXZ( b - a, point - a );
            const float bc = crossXZ( c - b, point - b );
            const float ca = crossXZ( a - c, point - c );
            if( ( ab > 0 && bc > 0 && ca > 0 ) || ( ab < 0 && bc < 0 && ca < 0 ) )
            {
                return false;
            }
        }
        // Ordinary junction fans and curves may extend beyond the source spans.
        return clearOfCarriageway( point );
    };

    SourceBudgetMap<Cell, Vector3F> joinedNormals;
    SourceBudgetVector<Edge> vergeEdges;
    for( auto edge : edges )
    {
        const auto centre = ( edge.a + edge.b ) * .5f;
        if( !isClear( centre + edge.outward * .32f ) )
        {
            continue;
        }
        const auto found = cells.find( cellAt( centre ) );
        if( found != cells.end() )
        {
            for( const auto index : found->second.roadEnds )
            {
                const auto &end = ends[index];
                const auto offset = centre - end.centre;
                const auto ahead = offset.dotProduct( end.direction );
                const auto across = std::abs( crossXZ( end.direction, offset ) );
                // The free-end oval projects 0–3 m ahead of the road spline, plus the
                // pavement. Require the actual welded skirt to face into that sector.
                if( ahead > .55f && ahead < 5.65f && across < end.halfWidth + 2.60f &&
                    edge.outward.dotProduct( end.direction ) > .18f )
                {
                    edge.terminal = index;
                    break;
                }
            }
        }
        vergeEdges.push_back( edge );
        for( const auto &point : { edge.a, edge.b } )
        {
            auto inserted = joinedNormals.try_emplace( joinKey( point ), Vector3F( 0, 0, 0 ) );
            inserted.first->second += edge.outward;
        }
    }

    Geometry geometry;
    Geometry fringe[2];
    const bool preview = quality == VehicleAppearanceQuality::Preview;
    const size_t maxQuads = preview ? 5200 : 14000;
    const size_t maxClumps = preview ? 435 : 1120;
    size_t clumps = 0;
    const float phase = float( seed % 1024u ) / 1024.f;
    const float outerRadiusSquared = assets.cityLayout.extent * assets.cityLayout.extent * .48f;
    const auto pointNormal = [&]( Vector3F point, Vector3F fallback ) {
        auto normal = joinedNormals.find( joinKey( point ) )->second;
        normal.normalise();
        return normal.dotProduct( fallback ) > .55f ? normal : fallback;
    };
    const auto widthAt = [phase]( Vector3F point ) {
        const auto swell = std::sin( point.x * .29f + point.z * .21f + phase * 6.28f );
        const auto ripple = std::sin( point.x * .76f - point.z * .62f - phase * 6.28f );
        return 1.24f + .38f * swell + .20f * ripple;
    };
    const auto supported = [&]( Vector3F &point ) {
        return isClear( point ) && render::sampleTerrainHeight( *ground, placement, point, point.y );
    };
    const auto fringeCover = [seed]( Vector3F point ) {
        return smooth(
            ( noise( seed + 17, ( point.x + 1024.f ) * .37f, ( point.z + 1024.f ) * .37f, 4093 ) -
              .32f ) /
            .42f );
    };
    const auto emitFringe = [&]( Vector3F centre, Vector3F spread, Vector3F outward, u64 key,
                                 float cover, bool terminal, size_t limit ) {
        if( clumps >= limit )
        {
            return;
        }
        const bool dry = unit( key + 1 ) > .58f;
        auto &batch = fringe[dry ? 1 : 0];
        const float drift = terminal ? .23f + .50f * unit( key + 2 ) : .27f + .60f * unit( key + 2 );
        const unsigned members = 2 + unsigned( cover * ( terminal ? 4.f : 3.f ) );
        for( unsigned i = 0; i < members && clumps < limit; ++i )
        {
            const auto plant = render::foliageStableHash( key + 3 + i );
            auto root = centre + spread * ( unit( plant ) - .5f ) +
                        outward * ( drift + .26f * ( unit( plant + 1 ) - .5f ) );
            if( !supported( root ) )
            {
                continue;
            }
            const auto firstVertex = batch.positions.size();
            const bool rosette = unit( plant + 2 ) < .25f;
            const unsigned bladeCount = rosette ? 4u : 4u + unsigned( 2.f * unit( plant + 3 ) );
            const float phaseAngle = 2.f * pi * unit( plant + 4 );
            const float scale = ( terminal ? 1.22f : 1.f ) * ( .73f + .54f * unit( plant + 5 ) );
            for( unsigned blade = 0; blade < bladeCount; ++blade )
            {
                const auto sample = render::foliageStableHash( plant + 6 + blade );
                const float angle =
                    phaseAngle + 2.f * pi * ( blade + .32f * unit( sample ) ) / bladeCount;
                const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                const Vector3F across( direction.z, 0, -direction.x );
                const bool shortBlade = !rosette && blade >= ( bladeCount + 1 ) / 2;
                const float tier = shortBlade ? .52f + .25f * unit( sample + 6 ) : 1.f;
                const float height = scale * tier *
                                     ( rosette ? .047f + .050f * unit( sample + 1 )
                                               : .067f + .140f * unit( sample + 1 ) );
                const float reach = scale * ( rosette ? .09f + .05f * unit( sample + 2 )
                                                      : .044f + .09f * unit( sample + 2 ) );
                auto foot = root + direction * ( .023f * unit( sample + 3 ) );
                const auto lean = across * ( reach * .32f * ( unit( sample + 5 ) - .5f ) );
                auto curve = foot + direction * ( reach * ( rosette ? .52f : .64f ) ) + lean * .45f;
                auto tip = foot + direction * reach + lean;
                if( !supported( foot ) || !supported( curve ) || !supported( tip ) )
                {
                    continue;
                }
                const float soil = foot.y;
                foot.y -= .014f;
                curve.y = std::max( curve.y + .018f, soil + height * .82f );
                tip.y = std::max( tip.y + .018f, soil + height * ( rosette ? .64f : .96f ) );
                const float width = scale * ( rosette ? .018f + .004f * unit( sample + 4 )
                                                      : .012f + .005f * unit( sample + 4 ) );
                foldedGrassBlade( batch, std::array{ foot, curve, tip }, { width * .76f, width }, across,
                                  isClear );
            }
            if( batch.positions.size() != firstVertex )
            {
                ++clumps;
            }
        }
    };

    const auto keyAt = [seed]( Vector3F point ) {
        const auto cell = joinKey( point );
        return render::foliageStableHash( ( u64( u32( cell.first ) ) << 32 ) | u32( cell.second ) ) ^
               seed;
    };
    // Station seeds belong to the road end. Intersect each station's
    // ray with the exposed skirt, so terrain clipping or an extra arc
    // subdivision cannot reroll all of the nearby grass clumps.
    SourceBudgetVector<SourceBudgetVector<size_t>> terminalEdges( ends.size() );
    for( size_t i = 0; i < vergeEdges.size(); ++i )
    {
        if( vergeEdges[i].terminal != noTerminal )
        {
            terminalEdges[vergeEdges[i].terminal].push_back( i );
        }
    }
    const size_t activeEnds = std::count_if( terminalEdges.begin(), terminalEdges.end(),
                                             []( const auto &edges ) { return !edges.empty(); } );
    if( activeEnds )
    {
        const size_t terminalBudget = maxClumps * 2 / 3;
        const size_t perEnd = std::min<size_t>( preview ? 28 : 44, terminalBudget / activeEnds );
        for( size_t endIndex = 0; endIndex < terminalEdges.size(); ++endIndex )
        {
            const auto &arc = terminalEdges[endIndex];
            if( arc.empty() )
            {
                continue;
            }
            const auto &end = ends[endIndex];
            const auto endKey = keyAt( end.centre );
            const Vector3F side( -end.direction.z, 0, end.direction.x );
            const unsigned stations = end.halfWidth >= 7.5f ? 24u : end.halfWidth >= 4.f ? 16u : 12u;
            const auto limit = clumps + perEnd;
            for( unsigned i = 0; i < stations && clumps < limit; ++i )
            {
                // Visit well-spaced sectors first if this end uses its reserve.
                const auto station = ( i * 7u ) % stations;
                const auto key = render::foliageStableHash( endKey + station );
                const float angle = pi * ( station + .18f + .64f * unit( key + 11 ) ) / stations;
                const auto ray =
                    side * ( std::cos( angle ) * ( end.halfWidth + 2.45f ) ) +
                    end.direction * ( std::sin( angle ) * ( std::min( 3.f, end.halfWidth ) + 2.45f ) );
                float nearestOutside = 0;
                Vector3F point, outward;
                for( const auto edgeIndex : arc )
                {
                    const auto &edge = vergeEdges[edgeIndex];
                    const auto delta = edge.b - edge.a;
                    const float denominator = crossXZ( ray, delta );
                    if( std::abs( denominator ) < .00001f )
                    {
                        continue;
                    }
                    const float t = crossXZ( edge.a - end.centre, ray ) / denominator;
                    if( t < 0 || t > 1 )
                    {
                        continue;
                    }
                    const auto hit = edge.a + delta * t;
                    const float distance = ( hit - end.centre ).dotProduct( ray );
                    if( distance <= nearestOutside )
                    {
                        continue;
                    }
                    nearestOutside = distance;
                    point = hit;
                    outward = pointNormal( edge.a, edge.outward ) * ( 1 - t ) +
                              pointNormal( edge.b, edge.outward ) * t;
                }
                if( nearestOutside <= 0 )
                {
                    continue;
                }
                const float cover = fringeCover( point );
                if( unit( key ) >= .69f + .26f * cover )
                {
                    continue;
                }
                outward.normalise();
                const Vector3F tangent( -outward.z, 0, outward.x );
                emitFringe( point, tangent * .65f, outward, key, cover, true, limit );
            }
        }
    }

    SourceBudgetMap<Cell, size_t> fringeCounts;
    for( const auto &edge : vergeEdges )
    {
        if( clumps >= maxClumps )
        {
            break;
        }
        if( edge.terminal != noTerminal )
        {
            continue;
        }
        const size_t areaLimit = preview ? 5 : 8;
        auto &count = fringeCounts[cellAt( ( edge.a + edge.b ) * .5f )];
        if( count >= areaLimit )
        {
            continue;
        }
        const auto delta = edge.b - edge.a;
        const auto pieces = std::max( 1, int( std::ceil( delta.length() / 1.1f ) ) );
        const auto before = clumps;
        const auto limit = std::min( maxClumps, clumps + areaLimit - count );
        for( int i = 0; i < pieces && clumps < limit; ++i )
        {
            const auto a = edge.a + delta * ( float( i ) / pieces );
            const auto b = edge.a + delta * ( float( i + 1 ) / pieces );
            const auto point = ( a + b ) * .5f;
            const auto spread = b - a;
            const auto key = keyAt( point );
            const float cover = fringeCover( point );
            if( unit( key ) < spread.length() * ( .045f + .70f * cover ) )
            {
                emitFringe( point, spread, edge.outward, key, cover, false, limit );
            }
        }
        count += clumps - before;
    }

    // Keep the terminal blend and exposed approaches in the preview quota.
    for( int pass = 0; pass < 3; ++pass )
    {
        for( const auto &edge : vergeEdges )
        {
            if( geometry.indices.size() / 6 >= maxQuads )
            {
                break;
            }
            const auto centre = ( edge.a + edge.b ) * .5f;
            const bool outskirts = centre.x * centre.x + centre.z * centre.z > outerRadiusSquared;
            const int priority = edge.terminal != noTerminal ? 0 : ( outskirts ? 1 : 2 );
            if( priority != pass )
            {
                continue;
            }
            const auto delta = edge.b - edge.a;
            const auto normalA = pointNormal( edge.a, edge.outward );
            const auto normalB = pointNormal( edge.b, edge.outward );
            const auto count = std::max( 1, int( std::ceil( delta.length() / 1.1f ) ) );
            for( int step = 0; step < count && geometry.indices.size() / 6 + 2 <= maxQuads; ++step )
            {
                const float t0 = float( step ) / count, t1 = float( step + 1 ) / count;
                const auto a = edge.a + delta * t0, b = edge.a + delta * t1;
                const auto outA = normalA + ( normalB - normalA ) * t0;
                const auto outB = normalA + ( normalB - normalA ) * t1;
                const float widthA = widthAt( a ), widthB = widthAt( b );
                const auto u = [phase]( Vector3F p ) { return .119f * p.x + .087f * p.z + phase; };
                constexpr float rows[] = { 0.f, .37f, 1.f };
                for( int row = 0; row < 2; ++row )
                {
                    const float inner = rows[row], outer = rows[row + 1];
                    auto p = a + outA * ( .045f + inner * widthA );
                    auto q = b + outB * ( .045f + inner * widthB );
                    auto r = b + outB * ( .045f + outer * widthB );
                    auto s = a + outA * ( .045f + outer * widthA );
                    if( std::abs( crossXZ( q - p, r - p ) ) < .00001f ||
                        crossXZ( q - p, r - p ) * crossXZ( r - p, s - p ) <= 0 )
                    {
                        break;
                    }
                    if( !supported( p ) || !supported( q ) || !supported( r ) || !supported( s ) )
                    {
                        break;
                    }
                    if( roadSurface.overlaps( { p, q, r } ) || roadSurface.overlaps( { p, r, s } ) )
                    {
                        ++overlapQuads;
                        break;
                    }
                    bool matchesTerrain = true;
                    for( auto middle : { ( p + q ) * .5f, ( r + s ) * .5f, ( p + r ) * .5f,
                                         ( p + q + r ) / 3.f, ( p + r + s ) / 3.f } )
                    {
                        const float interpolated = middle.y;
                        matchesTerrain = matchesTerrain && supported( middle ) &&
                                         std::abs( middle.y - interpolated ) < .035f;
                    }
                    if( !matchesTerrain )
                    {
                        break;
                    }
                    auto normal = ( q - p ).crossProduct( r - p );
                    normal.normalise();
                    if( normal.y < 0 )
                    {
                        normal = -normal;
                    }
                    if( normal.y < .83f )
                    {
                        break;
                    }
                    const Vector3F bias( 0, .039f, 0 );
                    // Keep the vertical UV range inside the image so wrap filtering
                    // samples the transparent border at the terrain-facing edge.
                    geometry.quad( p + bias, q + bias, r + bias, s + bias, normal, u( a ), u( b ),
                                   .012f + .97f * inner, .012f + .97f * outer );
                }
            }
        }
    }
    if( !geometry.indices.empty() )
    {
        auto transition = material( assets, ColourF::White, .96f );
        transition->setTexture( upload( assets, makeTexture( seed ) ), 0 );
        transition->setTransparent( true );
        transition->setBlendMode( 1 );
        transition->setDepthWrite( false );
        if( auto actor =
                mesh( assets, "Terrain city verge transition soil and grass", geometry, transition ) )
        {
            actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                scene::Renderer::CastShadows::Off );
        }
    }
    const auto publishFringe = [&]( const char *name, const Geometry &batch, ColourF tint ) {
        if( batch.indices.empty() )
        {
            return;
        }
        if( auto actor = mesh( assets, name, batch, material( assets, tint, .95f ) ) )
        {
            actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                scene::Renderer::CastShadows::Off );
        }
    };
    publishFringe( "Terrain city verge pavement fringe grass", fringe[0], { .334f, .378f, .192f, 1 } );
    publishFringe( "Terrain city verge pavement fringe dry tips", fringe[1],
                   { .412f, .380f, .217f, 1 } );
    WP_LOG( String( "Terrain city verge exact carriageway rejects: points=" ) +
            StringUtil::toString( overlapPoints ) +
            ", transition quads=" + StringUtil::toString( overlapQuads ) );
}
