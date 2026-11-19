#pragma once

namespace cityStormDrains
{
    constexpr float grateLength = .83f, grateWidth = .105f;

    float cross( Vector3F a, Vector3F b )
    {
        return a.x * b.z - a.z * b.x;
    }

    struct KerbEdges
    {
        using Cell = std::pair<s32, s32>;

        struct Edge
        {
            Vector3F a, b, towardsRoad;
        };

        struct Hit
        {
            Vector3F point, towardsRoad;
        };

        SourceBudgetMap<Cell, SourceBudgetVector<Edge>> index;

        static Cell cell( Vector3F p )
        {
            return { s32( std::floor( p.x / 8.f ) ), s32( std::floor( p.z / 8.f ) ) };
        }

        explicit KerbEdges( const CityRoadTerrainMesh &footways )
        {
            for( size_t i = 0; i + 2 < footways.indices.size(); i += 3 )
            {
                const auto &top = footways.vertices[footways.indices[i]];
                const auto &a = footways.vertices[footways.indices[i + 1]];
                const auto &b = footways.vertices[footways.indices[i + 2]];
                if( std::abs( top.normal.y ) > .01f || top.position.y - a.position.y < .012f ||
                    std::abs( top.position.x - a.position.x ) > .001f ||
                    std::abs( top.position.z - a.position.z ) > .001f )
                {
                    continue;
                }
                const auto low = cell( { std::min( a.position.x, b.position.x ), 0,
                                         std::min( a.position.z, b.position.z ) } );
                const auto high = cell( { std::max( a.position.x, b.position.x ), 0,
                                          std::max( a.position.z, b.position.z ) } );
                for( auto x = low.first; x <= high.first; ++x )
                {
                    for( auto z = low.second; z <= high.second; ++z )
                    {
                        index[{ x, z }].push_back( { a.position, b.position, top.normal } );
                    }
                }
            }
        }

        bool find( Vector3F origin, Vector3F side, float expected, Hit &result ) const
        {
            const auto a = origin + side * ( expected - .42f );
            const auto b = origin + side * ( expected + .42f );
            const auto low = cell( { std::min( a.x, b.x ), 0, std::min( a.z, b.z ) } );
            const auto high = cell( { std::max( a.x, b.x ), 0, std::max( a.z, b.z ) } );
            float nearest = expected + .42f;
            bool hit = false;
            for( auto x = low.first; x <= high.first; ++x )
            {
                for( auto z = low.second; z <= high.second; ++z )
                {
                    const auto found = index.find( { x, z } );
                    if( found == index.end() )
                    {
                        continue;
                    }
                    for( const auto &edge : found->second )
                    {
                        const auto delta = edge.b - edge.a;
                        const float denominator = cross( side, delta );
                        if( std::abs( denominator ) < .0001f ||
                            edge.towardsRoad.dotProduct( side ) > -.87f )
                        {
                            continue;
                        }
                        const auto offset = edge.a - origin;
                        const float distance = cross( offset, delta ) / denominator;
                        const float fraction = cross( offset, side ) / denominator;
                        if( distance >= expected - .42f && distance < nearest && fraction >= -.0001f &&
                            fraction <= 1.0001f )
                        {
                            nearest = distance;
                            const auto point = edge.a + delta * std::clamp( fraction, 0.f, 1.f );
                            result = { point, edge.towardsRoad };
                            hit = true;
                        }
                    }
                }
            }
            return hit;
        }
    };

    float ramp( float low, float high, float value )
    {
        const auto t = std::clamp( ( value - low ) / ( high - low ), 0.f, 1.f );
        return t * t * ( 3.f - 2.f * t );
    }

    SmartPtr<render::IMaterial> castIron( SceneAssets &assets, u32 seed )
    {
        constexpr u32 width = 128, height = 48;
        TextureBuffer albedo( width, height ), normal( width, height );
        const auto profile = []( float u, float v ) {
            // A cast perimeter frame and alternating openings rebated into the grate.
            // Slanted bars remain recognizable when seen along the gutter at street height.
            const float x = ( u - .035f ) * 8.6f + ( v - .5f ) * .29f;
            const float notch = std::abs( x - std::round( x ) );
            const float radial = std::min( { u * grateLength, ( 1.f - u ) * grateLength, v * grateWidth,
                                             ( 1.f - v ) * grateWidth } );
            const float opening = ramp( .011f, .019f, radial ) * ( 1.f - ramp( .30f, .38f, notch ) );
            const float casting = ramp( .001f, .006f, radial );
            return Vector2F( std::max( 0.f, opening ), .0016f * casting - opening * .0037f );
        };
        const auto byte = []( float value ) { return u8( std::clamp( value, 0.f, 255.f ) ); };
        for( u32 y = 0; y < height; ++y )
        {
            for( u32 x = 0; x < width; ++x )
            {
                const float u = ( x + .5f ) / width, v = ( y + .5f ) / height;
                const auto detail = profile( u, v );
                const auto key = render::foliageStableHash( ( u64( seed ) << 32 ) | ( y * width + x ) );
                const float grain = float( ( key >> 37 ) & 31u ) - 15.f;
                const float burnish = ( 1.f - std::abs( v * 2.f - 1 ) ) * 6.f;
                const float iron = ( 88.f + burnish + .44f * grain ) * ( 1 - detail.x ) +
                                   ( 24.f + .14f * grain ) * detail.x;
                auto *pixel = albedo.pixel( x, y );
                pixel[0] = byte( iron + 2.f );
                pixel[1] = byte( iron );
                pixel[2] = byte( iron - 3.f );
                pixel[3] = 255;
                const float dx = ( profile( u + 1.f / width, v ).y - profile( u - 1.f / width, v ).y ) *
                                 width / ( 2.f * grateLength );
                const float dy =
                    ( profile( u, v + 1.f / height ).y - profile( u, v - 1.f / height ).y ) * height /
                    ( 2.f * grateWidth );
                const auto n = Vector3F( -dx, -dy, 1 ).normaliseCopy();
                auto *encoded = normal.pixel( x, y );
                encoded[0] = byte( 127.5f * ( n.x + 1.f ) );
                encoded[1] = byte( 127.5f * ( n.y + 1.f ) );
                encoded[2] = byte( 127.5f * ( n.z + 1.f ) );
                encoded[3] = 255;
            }
        }
        auto surface = material( assets, ColourF::White, .66f, .43f );
        surface->setTexture( upload( assets, albedo ), 0 );
        surface->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
        surface->setUVProjection( 0 );
        surface->setNormalStrength( .65f );
        surface->setEditorUInt( "uvFilter", 3 );
        surface->setEditorFloat( "uvAniso", 8.f );
        return surface;
    }

    struct Bounds
    {
        float x0, x1, z0, z1;

        bool intersects( const Bounds &b ) const
        {
            return x0 <= b.x1 && x1 >= b.x0 && z0 <= b.z1 && z1 >= b.z0;
        }
    };

    Bounds bounds( Vector3F a, Vector3F b, Vector3F c )
    {
        return { std::min( { a.x, b.x, c.x } ), std::max( { a.x, b.x, c.x } ),
                 std::min( { a.z, b.z, c.z } ), std::max( { a.z, b.z, c.z } ) };
    }

    struct Candidate
    {
        std::array<Vector3F, 4> outline;
        Bounds footprint;
        Vector3F kerb, along, intoRoad;
        float area = 0, covered = 0;
        size_t crossing = 0, side = 0;
        u32 stamp = 0;
        bool blocked = false;
        SourceBudgetVector<std::array<Vector3F, 3>> faces;
    };

    bool makeCandidate( const KerbEdges &edges, const CityRoadCrossing &crossing, float side,
                        float station, Candidate &candidate )
    {
        const auto across = crossing.across * side;
        const auto centre = crossing.centre + crossing.outward * station;
        const auto run = crossing.outward * ( grateLength * .5f );
        const float half = crossing.width * .5f;
        KerbEdges::Hit first{}, middle{}, last{};
        if( !edges.find( centre - run, across, half + .05f, first ) ||
            !edges.find( centre, across, half + .05f, middle ) ||
            !edges.find( centre + run, across, half + .05f, last ) ||
            first.towardsRoad.dotProduct( last.towardsRoad ) < .994f ||
            std::abs( ( middle.point - ( first.point + last.point ) * .5f ).dotProduct( across ) ) >
                .015f )
        {
            return false;
        }
        // The walk begins 5 cm beyond the asphalt. This band lies in the
        // outer 12.5 cm of the road, ahead of the existing 14 cm paint inset.
        constexpr float border = .068f;
        auto &p = candidate.outline;
        p = { first.point + first.towardsRoad * border, last.point + last.towardsRoad * border,
              last.point + last.towardsRoad * ( border + grateWidth ),
              first.point + first.towardsRoad * ( border + grateWidth ) };
        candidate.footprint = bounds( p[0], p[1], p[2] );
        auto &bbox = candidate.footprint;
        bbox.x0 = std::min( bbox.x0, p[3].x );
        bbox.x1 = std::max( bbox.x1, p[3].x );
        bbox.z0 = std::min( bbox.z0, p[3].z );
        bbox.z1 = std::max( bbox.z1, p[3].z );
        candidate.kerb = middle.point;
        candidate.intoRoad = middle.towardsRoad;
        candidate.along = last.point - first.point;
        candidate.along.y = 0;
        if( candidate.along.lengthSquared() < .65f * .65f )
        {
            return false;
        }
        candidate.along.normalise();
        candidate.area =
            .5f * std::abs( cross( p[1] - p[0], p[2] - p[0] ) + cross( p[2] - p[0], p[3] - p[0] ) );
        return candidate.area > .065f && candidate.area < .11f;
    }

    template <class Emit>
    float clip( Vector3F a, Vector3F b, Vector3F c, const Candidate &candidate, const Emit &emit )
    {
        const auto &outline = candidate.outline;
        const float sign = cross( outline[1] - outline[0], outline[3] - outline[0] ) >= 0 ? 1.f : -1.f;
        std::array polygon{ a, b, c, Vector3F(), Vector3F(), Vector3F(), Vector3F(), Vector3F() };
        std::array<Vector3F, 8> result;
        size_t count = 3;
        for( size_t i = 0; i < outline.size(); ++i )
        {
            const auto p = outline[i], axis = outline[( i + 1 ) % outline.size()] - p;
            const auto distance = [&]( Vector3F point ) { return sign * cross( axis, point - p ); };
            size_t emitted = 0;
            auto previous = polygon[count - 1];
            auto before = distance( previous );
            for( size_t j = 0; j < count; ++j )
            {
                const auto current = polygon[j];
                const auto after = distance( current );
                if( ( before < 0 && after > 0 ) || ( before > 0 && after < 0 ) )
                {
                    result[emitted++] =
                        previous + ( current - previous ) * ( before / ( before - after ) );
                }
                if( after >= 0 )
                {
                    result[emitted++] = current;
                }
                previous = current;
                before = after;
            }
            if( emitted < 3 )
            {
                return 0;
            }
            polygon.swap( result );
            count = emitted;
        }
        float clippedArea = 0;
        for( size_t i = 1; i + 1 < count; ++i )
        {
            const float area =
                std::abs( cross( polygon[i] - polygon[0], polygon[i + 1] - polygon[0] ) ) * .5f;
            if( area <= 1e-7f )
            {
                continue;
            }
            clippedArea += area;
            emit( polygon[0], polygon[i], polygon[i + 1] );
        }
        return clippedArea;
    }

    void build( SceneAssets &assets, const CityRoadTerrainMesh &footways, u32 seed,
                VehicleAppearanceQuality quality )
    {
        const int detail = std::clamp( assets.cityStreetDetail, 0, 100 );
        if( !detail || footways.crossings.empty() )
        {
            return;
        }
        const auto layers = cityCarriagewayUtilities::activeLayers( assets );
        if( !layers.road || !layers.paint )
        {
            return;
        }
        const KerbEdges edges( footways );
        SourceBudgetVector<Candidate> candidates;
        for( size_t index = 0; index < footways.crossings.size(); ++index )
        {
            const auto &crossing = footways.crossings[index];
            if( !crossing.droppedKerbs )
            {
                continue;
            }
            for( size_t side = 0; side < 2; ++side )
            {
                const auto key =
                    render::foliageStableHash( ( u64( seed ) << 32 ) | ( index * 2 + side ) );
                const float offset = crossing.depth * .5f + .65f + .12f * float( key & 7u ) / 7.f;
                for( const float sign : { -1.f, 1.f } )
                {
                    Candidate candidate;
                    if( !makeCandidate( edges, crossing, side ? 1.f : -1.f, sign * offset, candidate ) )
                    {
                        continue;
                    }
                    candidate.crossing = index;
                    candidate.side = side;
                    candidates.push_back( std::move( candidate ) );
                }
            }
        }
        if( candidates.empty() )
        {
            return;
        }

        SourceBudgetMap<KerbEdges::Cell, SourceBudgetVector<size_t>> cells;
        for( size_t i = 0; i < candidates.size(); ++i )
        {
            const auto &b = candidates[i].footprint;
            const auto low = KerbEdges::cell( { b.x0, 0, b.z0 } ),
                       high = KerbEdges::cell( { b.x1, 0, b.z1 } );
            for( auto x = low.first; x <= high.first; ++x )
            {
                for( auto z = low.second; z <= high.second; ++z )
                {
                    cells[{ x, z }].push_back( i );
                }
            }
        }
        u32 stamp = 0;
        const auto inspect = [&]( const SmartPtr<IMesh> &mesh, bool obstacle ) {
            return !mesh || cityCarriagewayUtilities::visitTriangles( mesh, [&]( Vector3F a, Vector3F b,
                                                                                 Vector3F c ) {
                const auto box = bounds( a, b, c );
                const auto low = KerbEdges::cell( { box.x0, 0, box.z0 } ),
                           high = KerbEdges::cell( { box.x1, 0, box.z1 } );
                ++stamp;
                const auto test = [&]( Candidate &candidate ) {
                    if( candidate.blocked || candidate.stamp == stamp ||
                        !candidate.footprint.intersects( box ) )
                    {
                        return;
                    }
                    candidate.stamp = stamp;
                    if( obstacle )
                    {
                        if( clip( a, b, c, candidate, []( auto, auto, auto ) {} ) > .00002f )
                        {
                            candidate.blocked = true;
                        }
                    }
                    else
                    {
                        candidate.covered += clip( a, b, c, candidate, [&]( auto p, auto q, auto r ) {
                            if( candidate.faces.size() < 60 )
                            {
                                candidate.faces.push_back( { p, q, r } );
                            }
                            else
                            {
                                candidate.blocked = true;
                            }
                        } );
                    }
                };
                // A large source triangle can span many cells. Prefilter its actual
                // footprint instead of walking an unbounded integer range.
                if( u64( s64( high.first ) - low.first + 1 ) *
                        u64( s64( high.second ) - low.second + 1 ) >
                    128 )
                {
                    for( auto &candidate : candidates )
                    {
                        test( candidate );
                    }
                    return;
                }
                for( auto x = low.first; x <= high.first; ++x )
                {
                    for( auto z = low.second; z <= high.second; ++z )
                    {
                        if( const auto found = cells.find( { x, z } ); found != cells.end() )
                        {
                            for( const auto index : found->second )
                            {
                                test( candidates[index] );
                            }
                        }
                    }
                }
            } );
        };
        if( !inspect( layers.paint, true ) || !inspect( layers.island, true ) ||
            !inspect( layers.kerb, true ) || !inspect( layers.road, false ) )
        {
            return;
        }
        std::stable_sort( candidates.begin(), candidates.end(),
                          []( const Candidate &a, const Candidate &b ) {
                              return a.crossing == b.crossing
                                         ? ( a.side == b.side ? a.kerb.y > b.kerb.y : a.side < b.side )
                                         : a.crossing < b.crossing;
                          } );

        Geometry grates;
        SourceBudgetVector<std::array<bool, 2>> used( footways.crossings.size() );
        const bool preview = quality == VehicleAppearanceQuality::Preview;
        const size_t limit = ( ( preview ? 8u : 20u ) * u32( detail ) + 99 ) / 100;
        size_t counts[2] = {};
        for( const auto &candidate : candidates )
        {
            const auto family = footways.crossings[candidate.crossing].roundabout ? 1u : 0u;
            if( used[candidate.crossing][candidate.side] || counts[family] >= limit ||
                candidate.blocked || std::abs( candidate.covered - candidate.area ) > .00038f ||
                candidate.faces.empty() )
            {
                continue;
            }
            bool grounded = true;
            for( const auto &face : candidate.faces )
            {
                if( std::any_of( face.begin(), face.end(), [&]( Vector3F p ) {
                        return std::abs( p.y - candidate.kerb.y ) > .12f;
                    } ) )
                {
                    grounded = false;
                    break;
                }
            }
            if( !grounded )
            {
                continue;
            }
            for( const auto &face : candidate.faces )
            {
                auto n = ( face[1] - face[0] ).crossProduct( face[2] - face[0] );
                const bool forward = n.y > 0;
                n = ( forward ? n : -n ).normaliseCopy();
                auto tangent = candidate.along - n * candidate.along.dotProduct( n );
                tangent.normalise();
                const float handedness =
                    n.crossProduct( tangent ).dotProduct( candidate.intoRoad ) >= 0 ? 1.f : -1.f;
                for( const auto vertex :
                     { face[0], forward ? face[1] : face[2], forward ? face[2] : face[1] } )
                {
                    const auto local = vertex - candidate.outline[0];
                    grates.indices.push_back( u32( grates.positions.size() ) );
                    grates.positions.push_back( vertex + Vector3F( 0, .005f, 0 ) );
                    grates.normals.push_back( n );
                    grates.uv.push_back(
                        { std::clamp( local.dotProduct( candidate.along ) / grateLength, 0.f, 1.f ),
                          std::clamp( local.dotProduct( candidate.intoRoad ) / grateWidth, 0.f, 1.f ),
                          0 } );
                    grates.tangents.push_back( { tangent.x, tangent.y, tangent.z, handedness } );
                }
            }
            used[candidate.crossing][candidate.side] = true;
            ++counts[family];
        }
        if( grates.indices.empty() )
        {
            return;
        }
        if( auto actor =
                mesh( assets, "Terrain city cast iron storm grates", grates, castIron( assets, seed ) ) )
        {
            actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                scene::Renderer::CastShadows::Off );
        }
        WP_LOG( String( "Terrain city storm grates=" ) + StringUtil::toString( counts[0] + counts[1] ) +
                " ordinary=" + StringUtil::toString( counts[0] ) +
                " roundabouts=" + StringUtil::toString( counts[1] ) );
    }
}  // namespace cityStormDrains
