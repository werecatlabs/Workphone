#pragma once

namespace cityPedestrianSurfacing
{
    constexpr float edgeWidth = .105f;
    constexpr float tileMetres = 1.20f;

    struct Batches
    {
        Geometry paving, edging, drainage, bedding;
    };

    struct Vertex
    {
        Vector3F position;
        float lateral, station, fromEdge;
    };

    struct Polygon
    {
        std::array<Vertex, 8> vertices{};
        size_t count = 0;
    };

    Polygon clip( const Polygon &polygon, float Vertex::*coordinate, float line, float side )
    {
        Polygon result;
        for( size_t i = 0; i < polygon.count; ++i )
        {
            const auto &a = polygon.vertices[i];
            const auto &b = polygon.vertices[( i + 1 ) % polygon.count];
            const float da = ( a.*coordinate - line ) * side;
            const float db = ( b.*coordinate - line ) * side;
            if( da >= 0 )
            {
                result.vertices[result.count++] = a;
            }
            if( ( da > 0 && db < 0 ) || ( da < 0 && db > 0 ) )
            {
                const float t = da / ( da - db );
                result.vertices[result.count++] = { a.position + ( b.position - a.position ) * t,
                                                    a.lateral + ( b.lateral - a.lateral ) * t,
                                                    a.station + ( b.station - a.station ) * t,
                                                    a.fromEdge + ( b.fromEdge - a.fromEdge ) * t };
            }
        }
        return result;
    }

    void emit( Geometry &geometry, const Polygon &polygon, Vector3F normal, Vector3F tangent, bool edge )
    {
        if( polygon.count < 3 )
        {
            return;
        }
        // Each subdivision is on its source triangle plane, including the
        // inlaid edge strips. Materials share the same supported walking top.
        const auto origin = polygon.vertices.front().position;
        for( size_t i = 1; i + 1 < polygon.count; ++i )
        {
            const auto cross = ( polygon.vertices[i].position - origin )
                                   .crossProduct( polygon.vertices[i + 1].position - origin );
            const float winding = cross.dotProduct( normal );
            if( std::abs( winding ) < 1e-8f )
            {
                continue;
            }
            const u32 base = u32( geometry.positions.size() );
            for( const size_t index : { size_t( 0 ), winding > 0 ? i : i + 1, winding > 0 ? i + 1 : i } )
            {
                const auto &vertex = polygon.vertices[index];
                const float u =
                    edge ? .075f + .093f * vertex.fromEdge / edgeWidth : vertex.lateral / tileMetres;
                geometry.positions.push_back( vertex.position );
                geometry.normals.push_back( normal );
                geometry.uv.push_back( { u, vertex.station / tileMetres, 0 } );
                geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
            }
            geometry.indices.push_back( base );
            geometry.indices.push_back( base + 1 );
            geometry.indices.push_back( base + 2 );
        }
    }

    void emitTop( Batches &batches, Vertex a, Vertex b, Vertex c, Vector3F across, float drain )
    {
        auto normal = ( b.position - a.position ).crossProduct( c.position - a.position );
        if( std::abs( normal.y ) < 1e-8f )
        {
            return;
        }
        normal.normalise();
        if( normal.y < 0 )
        {
            normal = -normal;
        }
        auto tangent = across - normal * across.dotProduct( normal );
        tangent.normalise();
        const Polygon triangle{ { a, b, c }, 3 };
        emit( batches.edging, clip( triangle, &Vertex::fromEdge, edgeWidth, -1.f ), normal, tangent,
              true );
        const auto paving = clip( triangle, &Vertex::fromEdge, edgeWidth, 1.f );
        constexpr float halfChannel = .023f;
        const float start = std::min( { a.station, b.station, c.station } );
        const float end = std::max( { a.station, b.station, c.station } );
        if( drain - halfChannel >= end || drain + halfChannel <= start )
        {
            emit( batches.paving, paving, normal, tangent, false );
            return;
        }
        const auto afterStart = clip( paving, &Vertex::station, drain - halfChannel, 1.f );
        emit( batches.paving, clip( paving, &Vertex::station, drain - halfChannel, -1.f ), normal,
              tangent, false );
        emit( batches.paving, clip( afterStart, &Vertex::station, drain + halfChannel, 1.f ), normal,
              tangent, false );
        emit( batches.drainage, clip( afterStart, &Vertex::station, drain + halfChannel, -1.f ), normal,
              tangent, false );
    }

    float distance( Vector3F a, Vector3F b )
    {
        return std::hypot( b.x - a.x, b.z - a.z );
    }

    template <class Surface>
    bool append( Batches &batches, const cityEntranceRoute::Route &route, const Surface &surface )
    {
        if( route.rowCount < 2 || route.rowCount > route.rows.size() )
        {
            return false;
        }
        std::array<float, 96> stations{};
        for( size_t i = 1; i < route.rowCount; ++i )
        {
            stations[i] = stations[i - 1] + distance( route.rows[i - 1].centre, route.rows[i].centre );
        }
        float drain = -1.f;
        for( size_t i = 2; i + 2 < route.rowCount; ++i )
        {
            // A low point can use a flush linear channel. Stay clear of
            // the existing apron and sidewalk seams at both ends.
            if( route.rows[i - 1].centre.y - route.rows[i].centre.y >= .017f &&
                route.rows[i + 1].centre.y - route.rows[i].centre.y >= .017f && stations[i] > 1.2f &&
                stations[route.rowCount - 1] - stations[i] > .85f )
            {
                drain = stations[i];
                break;
            }
        }
        const auto section = []( const auto &row, float station, bool right ) {
            const float half = distance( row.left, row.centre );
            const float width = right ? distance( row.centre, row.right ) : half;
            if( right )
            {
                return std::array<Vertex, 2>{ Vertex{ row.centre, 0.f, station, width },
                                              Vertex{ row.right, width, station, 0.f } };
            }
            return std::array<Vertex, 2>{ Vertex{ row.left, -width, station, 0.f },
                                          Vertex{ row.centre, 0.f, station, width } };
        };
        const size_t first = batches.paving.indices.size();
        for( size_t i = 1; i < route.rowCount; ++i )
        {
            const auto &from = route.rows[i - 1];
            const auto &to = route.rows[i];
            const auto across = ( from.right - from.left ) + ( to.right - to.left );
            for( const bool right : { false, true } )
            {
                const auto low = section( from, stations[i - 1], right );
                const auto high = section( to, stations[i], right );
                emitTop( batches, low[0], low[1], high[1], across, drain );
                emitTop( batches, low[0], high[1], high[0], across, drain );

                const auto a = right ? from.right : from.left;
                const auto b = right ? to.right : to.left;
                auto baseA = a, baseB = b;
                if( !surface( baseA ) || !surface( baseB ) )
                {
                    continue;
                }
                // Sink the supported edge into the bank. Leave the two
                // shared end rows open where they meet the existing walks.
                baseA.y = std::min( a.y, baseA.y - .054f );
                baseB.y = std::min( b.y, baseB.y - .054f );
                const auto normal =
                    cityEntranceRoute::horizontal( right ? across : -across ).normaliseCopy();
                batches.bedding.quad( a, b, baseB, baseA, normal, stations[i - 1] / tileMetres,
                                      stations[i] / tileMetres, .28f, .35f );
            }
        }
        return batches.paving.indices.size() > first;
    }

    struct Pavers
    {
        SmartPtr<render::ITexture> colour, normal;
    };

    Pavers pavers( SceneAssets &assets, u32 seed )
    {
        constexpr u32 size = 128;
        TextureBuffer colour( size, size ), normal( size, size );
        SourceBudgetVector<float> relief( size * size );
        const auto random = []( u64 key ) {
            return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
        };
        const auto channel = []( float value ) { return u8( std::clamp( value, 0.f, 255.f ) + .5f ); };
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float u = ( x + .5f ) * 4.f / size;
                const u32 course = u32( u );
                const float v = ( y + .5f ) * 2.f / size + .5f * float( course & 1u );
                const u32 stone = u32( v ) & 1u;
                const float across = u - course, along = v - std::floor( v );
                const float gap = std::min( .30f * std::min( across, 1.f - across ),
                                            .60f * std::min( along, 1.f - along ) );
                const u64 key = ( u64( seed ) << 32 ) | u64( y * size + x );
                const float grain = random( key );
                const float finish = random( seed + course * 117u + stone * 37u );
                const float bevel = std::clamp( ( gap - .0015f ) / .006f, 0.f, 1.f );
                const float face = bevel * bevel * ( 3.f - 2.f * bevel );
                const float dust = 110.f + grain * 9.f;
                const float top = 144.f + 15.f * finish + 13.f * ( grain - .5f );
                const float base = dust + ( top - dust ) * face;
                const auto pixel = colour.pixel( x, y );
                pixel[0] = channel( base + 8.f );
                pixel[1] = channel( base );
                pixel[2] = channel( base - 9.f );
                pixel[3] = 255;
                relief[size_t( y ) * size + x] = face * ( .0023f + .00065f * grain );
            }
        }
        const auto height = [&]( u32 x, u32 y ) { return relief[size_t( y ) * size + x]; };
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float dx = height( ( x + 1 ) % size, y ) - height( ( x + size - 1 ) % size, y );
                const float dy = height( x, ( y + 1 ) % size ) - height( x, ( y + size - 1 ) % size );
                const float scale = size / ( 2.f * tileMetres );
                const auto n = Vector3F( -dx * scale, -dy * scale, 1.f ).normaliseCopy();
                const auto pixel = normal.pixel( x, y );
                pixel[0] = channel( 127.5f * ( 1.f + n.x ) );
                pixel[1] = channel( 127.5f * ( 1.f + n.y ) );
                pixel[2] = channel( 127.5f * ( 1.f + n.z ) );
                pixel[3] = 255;
            }
        }
        return { upload( assets, colour ), upload( assets, normal, render::TextureMipFilter::Normal ) };
    }

    std::array<SmartPtr<scene::IGameActor>, 3> publish( SceneAssets &assets, const Batches &batches,
                                                        u32 seed )
    {
        std::array<SmartPtr<scene::IGameActor>, 3> actors{};
        if( batches.paving.indices.empty() )
        {
            return actors;
        }
        const auto maps = pavers( assets, seed );
        const auto stone = [&]( ColourF tint, float normalStrength ) {
            auto result = material( assets, tint, .91f );
            result->setTexture( maps.colour, 0 );
            result->setTexture( maps.normal, 1 );
            result->setUVProjection( 0 );
            result->setNormalStrength( normalStrength );
            result->setEditorUInt( "uvFilter", 3 );
            result->setEditorFloat( "uvAniso", 8.f );
            return result;
        };
        const auto add = [&]( const char *name, const Geometry &geometry, const auto &surface ) {
            auto actor = mesh( assets, name, geometry, surface );
            if( actor )
            {
                actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                    scene::Renderer::CastShadows::Off );
            }
            return actor;
        };
        actors[0] = add( "Terrain city pedestrian connector paving", batches.paving,
                         stone( ColourF::White, .44f ) );
        if( !batches.edging.indices.empty() )
        {
            actors[1] = add( "Terrain city pedestrian connector stone edging", batches.edging,
                             stone( { .83f, .82f, .78f, 1 }, .28f ) );
        }
        if( !batches.drainage.indices.empty() )
        {
            actors[2] = add( "Terrain city pedestrian connector drainage", batches.drainage,
                             material( assets, { .245f, .250f, .235f, 1 }, .67f, .54f ) );
        }
        if( !batches.bedding.indices.empty() )
        {
            add( "Terrain city pedestrian connector stone bedding", batches.bedding,
                 stone( { .58f, .57f, .52f, 1 }, .18f ) );
        }
        return actors;
    }
}  // namespace cityPedestrianSurfacing
