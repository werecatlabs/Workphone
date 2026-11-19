#pragma once

namespace cityCrossingLandings
{
    struct Vertex
    {
        Vector3F position;
        float station, lateral;
    };

    struct Panel
    {
        Vector3F centre, along, across;
        float halfLength, inner, outer;
    };

    struct KerbSegment
    {
        Vector3F a, b, roadward;
    };

    SourceBudgetVector<KerbSegment> kerbSegments( const CityRoadTerrainMesh &footways )
    {
        SourceBudgetVector<KerbSegment> result;
        for( size_t i = 0; i + 2 < footways.indices.size(); i += 3 )
        {
            const auto &top = footways.vertices[footways.indices[i]];
            const auto &a = footways.vertices[footways.indices[i + 1]].position;
            const auto &b = footways.vertices[footways.indices[i + 2]].position;
            if( std::abs( top.normal.y ) > .01f || top.position.y - a.y < .009f ||
                std::abs( top.position.x - a.x ) > .001f || std::abs( top.position.z - a.z ) > .001f )
            {
                continue;
            }
            const auto roadward = Vector3F( top.normal.x, 0, top.normal.z ).normaliseCopy();
            if( ( b - a ).lengthSquared() > .000004f )
            {
                result.push_back( { a, b, roadward } );
            }
        }
        return result;
    }

    void addGutter( Geometry &gutter, const SourceBudgetVector<KerbSegment> &segments,
                    const CityRoadCrossing &crossing, Vector3F across,
                    const render::TerrainData &terrain, const Transform3<real_Num> &placement )
    {
        constexpr float bias = .014f;
        const float halfDepth = crossing.depth * .5f + .39f;
        const auto sample = [&]( Vector3F &p ) {
            if( !render::sampleTerrainHeight( terrain, placement, p, p.y ) )
            {
                return false;
            }
            p.y += bias;
            return true;
        };
        for( const auto &segment : segments )
        {
            if( segment.roadward.dotProduct( across ) > -.65f )
            {
                continue;
            }
            const auto a = segment.a - crossing.centre, b = segment.b - crossing.centre;
            const float kerbOffset = ( a + b ).dotProduct( across ) * .5f - crossing.width * .5f;
            if( std::abs( kerbOffset ) > .39f )
            {
                continue;
            }
            const float start = a.dotProduct( crossing.outward );
            const float advance = b.dotProduct( crossing.outward ) - start;
            if( std::abs( advance ) < .001f )
            {
                continue;
            }
            const float t0 = ( -halfDepth - start ) / advance, t1 = ( halfDepth - start ) / advance;
            const float first = std::max( 0.f, std::min( t0, t1 ) ),
                        last = std::min( 1.f, std::max( t0, t1 ) );
            if( last - first <= .001f )
            {
                continue;
            }

            const auto step = segment.b - segment.a;
            const auto edgeA = segment.a + step * first, edgeB = segment.a + step * last;
            auto p = edgeA - segment.roadward * .009f, q = edgeB - segment.roadward * .009f;
            auto r = edgeB + segment.roadward * .088f, s = edgeA + segment.roadward * .088f;
            if( !sample( p ) || !sample( q ) || !sample( r ) || !sample( s ) )
            {
                continue;
            }
            auto middle = ( p + r ) * .5f;
            const float interpolated = middle.y;
            if( !sample( middle ) || std::abs( middle.y - interpolated ) > .009f )
            {
                continue;
            }
            auto normal = ( q - p ).crossProduct( s - p ).normaliseCopy();
            if( normal.y < 0 )
            {
                normal = -normal;
            }
            if( normal.y > .8f )
            {
                gutter.quad( p, q, r, s, normal );
            }
        }
    }

    void project( Geometry &geometry, const CityRoadTerrainMesh &footways, const Panel &panel )
    {
        const auto farCorner = std::hypot( panel.outer, panel.halfLength ) + .005f;
        const auto bounds = [&]( Vector3F a, Vector3F b, Vector3F c ) {
            return std::min( { a.x, b.x, c.x } ) <= panel.centre.x + farCorner &&
                   std::max( { a.x, b.x, c.x } ) >= panel.centre.x - farCorner &&
                   std::min( { a.z, b.z, c.z } ) <= panel.centre.z + farCorner &&
                   std::max( { a.z, b.z, c.z } ) >= panel.centre.z - farCorner;
        };
        for( size_t i = 0; i + 2 < footways.indices.size(); i += 3 )
        {
            const auto &a = footways.vertices[footways.indices[i]];
            const auto &b = footways.vertices[footways.indices[i + 1]];
            const auto &c = footways.vertices[footways.indices[i + 2]];
            if( a.normal.y < .4f || b.normal.y < .4f || c.normal.y < .4f ||
                !bounds( a.position, b.position, c.position ) )
            {
                continue;
            }

            std::array<Vertex, 8> polygon{}, clipped{};
            size_t count = 0;
            for( const auto position : { a.position, b.position, c.position } )
            {
                const auto relative = position - panel.centre;
                polygon[count++] = { position, relative.dotProduct( panel.along ),
                                     relative.dotProduct( panel.across ) };
            }
            const float limits[] = { panel.halfLength, panel.halfLength, -panel.inner, panel.outer };
            for( u32 plane = 0; plane < 4 && count >= 3; ++plane )
            {
                const auto distance = [&]( const Vertex &vertex ) {
                    const float coordinate = plane < 2 ? vertex.station : vertex.lateral;
                    return limits[plane] + ( plane % 2 ? -coordinate : coordinate );
                };
                size_t emitted = 0;
                auto previous = polygon[count - 1];
                float d0 = distance( previous );
                for( size_t corner = 0; corner < count; ++corner )
                {
                    const auto &current = polygon[corner];
                    const float d1 = distance( current );
                    if( ( d0 < 0 && d1 > 0 ) || ( d0 > 0 && d1 < 0 ) )
                    {
                        const float t = d0 / ( d0 - d1 );
                        clipped[emitted++] = {
                            previous.position + ( current.position - previous.position ) * t,
                            previous.station + ( current.station - previous.station ) * t,
                            previous.lateral + ( current.lateral - previous.lateral ) * t
                        };
                    }
                    if( d1 >= 0 )
                    {
                        clipped[emitted++] = current;
                    }
                    previous = current;
                    d0 = d1;
                }
                polygon.swap( clipped );
                count = emitted;
            }
            if( count < 3 )
            {
                continue;
            }

            auto normal = ( b.position - a.position ).crossProduct( c.position - a.position );
            if( normal.lengthSquared() < 1e-10f )
            {
                continue;
            }
            if( normal.y < 0 )
            {
                normal = -normal;
            }
            normal.normalise();
            const auto tangent =
                ( panel.along - normal * panel.along.dotProduct( normal ) ).normaliseCopy();
            const float handedness =
                normal.crossProduct( tangent ).dotProduct( panel.across ) >= 0 ? 1.f : -1.f;
            for( size_t corner = 1; corner + 1 < count; ++corner )
            {
                const auto face =
                    ( polygon[corner].position - polygon[0].position )
                        .crossProduct( polygon[corner + 1].position - polygon[0].position );
                if( face.lengthSquared() < 1e-10f )
                {
                    continue;
                }
                const bool forward = face.dotProduct( normal ) > 0;
                for( auto index :
                     { size_t( 0 ), forward ? corner : corner + 1, forward ? corner + 1 : corner } )
                {
                    const auto &vertex = polygon[index];
                    geometry.indices.push_back( u32( geometry.positions.size() ) );
                    geometry.positions.push_back( vertex.position + Vector3F( 0, .006f, 0 ) );
                    geometry.normals.push_back( normal );
                    geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, handedness } );
                    geometry.uv.push_back(
                        { vertex.station / .432f, ( vertex.lateral - panel.inner ) / .432f, 0 } );
                }
            }
        }
    }

    SmartPtr<render::IMaterial> materialForLandings( SceneAssets &assets )
    {
        constexpr u32 size = 128;
        constexpr float tile = .432f;
        TextureBuffer colour( size, size ), normal( size, size );
        const auto smooth = []( float low, float high, float value ) {
            const float t = std::clamp( ( value - low ) / ( high - low ), 0.f, 1.f );
            return t * t * ( 3.f - 2.f * t );
        };
        const auto wrapped = []( s32 value, s32 count ) {
            const s32 coordinate = value % count;
            return u32( coordinate < 0 ? coordinate + count : coordinate );
        };
        const auto unit = [&]( s32 x, s32 y, s32 count, u64 salt ) {
            const u64 key = ( u64( wrapped( x, count ) ) << 32 ) | wrapped( y, count );
            return float( render::foliageStableHash( key ^ salt ) >> 40 ) / 16777216.f;
        };
        const auto aggregate = [&]( float x, float y, s32 cells, u64 salt ) {
            const float u = ( x + .5f ) * cells / size, v = ( y + .5f ) * cells / size;
            const auto ix = s32( std::floor( u ) ), iy = s32( std::floor( v ) );
            const float fu = smooth( 0.f, 1.f, u - ix ), fv = smooth( 0.f, 1.f, v - iy );
            const float a =
                unit( ix, iy, cells, salt ) * ( 1.f - fu ) + unit( ix + 1, iy, cells, salt ) * fu;
            const float b = unit( ix, iy + 1, cells, salt ) * ( 1.f - fu ) +
                            unit( ix + 1, iy + 1, cells, salt ) * fu;
            return a * ( 1.f - fv ) + b * fv;
        };
        const auto joint = [size, tile, &aggregate, &smooth]( float x, float y ) {
            const float u = ( x + .5f ) / size, v = ( y + .5f ) / size;
            const float localU = u - std::floor( u ), localV = v - std::floor( v );
            const float distance = tile * std::min( { localU, 1.f - localU, localV, 1.f - localV } );
            const float edgeWear = aggregate( x, y, 24, 0x74616374696c6531ull );
            return smooth( .0025f + .0014f * ( edgeWear - .5f ), .0095f + .0014f * ( edgeWear - .5f ),
                           distance );
        };
        const auto profile = [&]( float x, float y ) {
            const float u = ( x + .5f ) * 6.f / size, v = ( y + .5f ) * 6.f / size;
            const float radius = std::hypot( u - std::floor( u ) - .5f, v - std::floor( v ) - .5f );
            const auto blisterX = s32( std::floor( u ) ), blisterY = s32( std::floor( v ) );
            const float height =
                .0058f * ( .90f + .20f * unit( blisterX, blisterY, 6, 0x626c697374657231ull ) );
            return height * ( 1.f - smooth( .16f, .33f, radius ) ) - .0035f * ( 1.f - joint( x, y ) );
        };
        const auto byte = []( float v ) { return u8( std::clamp( v, 0.f, 255.f ) ); };
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float metres = profile( float( x ), float( y ) );
                const auto key = render::foliageStableHash( ( u64( x ) << 32 ) | y );
                const float grain = float( ( key >> 28 ) & 31u ) - 15.f;
                auto *pixel = colour.pixel( x, y );
                const float relief = std::max( 0.f, metres / .0058f );
                const float tileFace = joint( float( x ), float( y ) );
                const float u = ( x + .5f ) * 6.f / size;
                const float v = ( y + .5f ) * 6.f / size;
                const float radius = std::hypot( u - std::floor( u ) - .5f, v - std::floor( v ) - .5f );
                const float rim = smooth( .23f, .29f, radius ) * ( 1.f - smooth( .34f, .40f, radius ) );
                const float mineral = aggregate( float( x ), float( y ), 28, 0x6d696e6572616c32ull );
                const float weather =
                    smooth( .55f, .83f, aggregate( float( x ), float( y ), 8, 0x7765617468657232ull ) );
                const float dust = rim * ( .55f + .45f * mineral );
                const float fleck = .46f * grain + 7.f * ( mineral - .5f ) - 6.f * weather;
                // Dust accumulates at blister feet and imperfect mortar;
                // the exposed crowns remain lighter and readable.
                pixel[0] = byte( 125.f + 46.f * tileFace + fleck + 8.f * relief - 11.f * dust );
                pixel[1] = byte( 105.f + 34.f * tileFace + .88f * fleck + 6.f * relief - 10.f * dust );
                pixel[2] = byte( 85.f + 20.f * tileFace + .65f * fleck + 4.f * relief - 8.f * dust );
                pixel[3] = 255;
                const float dx =
                    ( profile( float( x ) + 1, float( y ) ) - profile( float( x ) - 1, float( y ) ) ) *
                    size / ( 2 * tile );
                const float dy =
                    ( profile( float( x ), float( y ) + 1 ) - profile( float( x ), float( y ) - 1 ) ) *
                    size / ( 2 * tile );
                const auto n = Vector3F( -dx, -dy, 1 ).normaliseCopy();
                auto *encoded = normal.pixel( x, y );
                encoded[0] = byte( 127.5f * ( n.x + 1 ) );
                encoded[1] = byte( 127.5f * ( n.y + 1 ) );
                encoded[2] = byte( 127.5f * ( n.z + 1 ) );
                encoded[3] = 255;
            }
        }
        auto surface = material( assets, ColourF::White, .94f );
        surface->setTexture( upload( assets, colour ), 0 );
        surface->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
        surface->setNormalStrength( .97f );
        surface->setUVProjection( 0 );
        surface->setEditorUInt( "uvFilter", 3 );
        surface->setEditorFloat( "uvAniso", 8.f );
        return surface;
    }

    void build( SceneAssets &assets, const CityRoadTerrainMesh &footways )
    {
        const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
        if( footways.crossings.empty() || !terrain )
        {
            return;
        }
        const auto &placement = assets.roadAuthoring->getTerrainPlacement();
        const auto edges = kerbSegments( footways );
        Geometry geometry, gutters;
        size_t panels = 0, finishedGutters = 0;
        for( const auto &crossing : footways.crossings )
        {
            const float inner = crossing.width * .5f + .13f;
            for( const float side : { -1.f, 1.f } )
            {
                const auto start = geometry.indices.size();
                project( geometry, footways,
                         { crossing.centre, crossing.outward, crossing.across * side,
                           crossing.depth * .5f - .16f, inner, inner + .77f } );
                if( geometry.indices.size() > start )
                {
                    ++panels;
                }
                if( crossing.droppedKerbs )
                {
                    const auto gutterStart = gutters.indices.size();
                    addGutter( gutters, edges, crossing, crossing.across * side, *terrain, placement );
                    if( gutters.indices.size() > gutterStart )
                    {
                        ++finishedGutters;
                    }
                }
            }
        }
        if( geometry.indices.empty() )
        {
            return;
        }
        const auto actor = mesh( assets, "Terrain city tactile crossing landings", geometry,
                                 materialForLandings( assets ) );
        actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
        WP_LOG( String( "Terrain city tactile landings=" ) + StringUtil::toString( panels ) +
                " crossings=" + StringUtil::toString( footways.crossings.size() ) +
                " triangles=" + StringUtil::toString( geometry.indices.size() / 3 ) );
        if( !gutters.indices.empty() )
        {
            auto surface = material( assets, { .28f, .29f, .28f, 1 }, .96f );
            const auto channel = mesh( assets, "Terrain city crossing gutter insets", gutters, surface );
            channel->getComponent<scene::MeshRenderer>()->setCastShadows(
                scene::Renderer::CastShadows::Off );
            WP_LOG( String( "Terrain city crossing gutter insets=" ) +
                    StringUtil::toString( finishedGutters ) );
        }
    }
}  // namespace cityCrossingLandings
