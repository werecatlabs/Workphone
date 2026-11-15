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
                        !bounds( a.position, b.position, c.position ) ) continue;

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
                                    previous.lateral + ( current.lateral - previous.lateral ) * t };
                            }
                            if( d1 >= 0 ) clipped[emitted++] = current;
                            previous = current;
                            d0 = d1;
                        }
                        polygon.swap( clipped );
                        count = emitted;
                    }
                    if( count < 3 ) continue;

                    auto normal = ( b.position - a.position ).crossProduct( c.position - a.position );
                    if( normal.lengthSquared() < 1e-10f ) continue;
                    if( normal.y < 0 ) normal = -normal;
                    normal.normalise();
                    const auto tangent = ( panel.along - normal * panel.along.dotProduct( normal ) ).normaliseCopy();
                    const float handedness = normal.crossProduct( tangent ).dotProduct( panel.across ) >= 0 ? 1.f : -1.f;
                    for( size_t corner = 1; corner + 1 < count; ++corner )
                    {
                        const auto face = ( polygon[corner].position - polygon[0].position ).crossProduct(
                            polygon[corner + 1].position - polygon[0].position );
                        if( face.lengthSquared() < 1e-10f ) continue;
                        const bool forward = face.dotProduct( normal ) > 0;
                        for( auto index : { size_t( 0 ), forward ? corner : corner + 1, forward ? corner + 1 : corner } )
                        {
                            const auto &vertex = polygon[index];
                            geometry.indices.push_back( u32( geometry.positions.size() ) );
                            geometry.positions.push_back( vertex.position + Vector3F( 0, .006f, 0 ) );
                            geometry.normals.push_back( normal );
                            geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, handedness } );
                            geometry.uv.push_back( { vertex.station / .432f, ( vertex.lateral - panel.inner ) / .432f, 0 } );
                        }
                    }
                }
            }

            SmartPtr<render::IMaterial> materialForLandings( SceneAssets &assets )
            {
                constexpr u32 size = 128;
                constexpr float tile = .432f;
                TextureBuffer colour( size, size ), normal( size, size );
                const auto profile = [size]( float x, float y ) {
                    const float u = ( x + .5f ) * 6.f / size;
                    const float v = ( y + .5f ) * 6.f / size;
                    const float radius = std::hypot( u - std::floor( u ) - .5f, v - std::floor( v ) - .5f );
                    const float shoulder = std::clamp( ( .32f - radius ) / .16f, 0.f, 1.f );
                    return .0045f * shoulder * shoulder * ( 3.f - 2.f * shoulder );
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
                        const float relief = metres / .0045f;
                        pixel[0] = byte( 172.f + .63f * grain + 8.f * relief );
                        pixel[1] = byte( 127.f + .55f * grain + 5.f * relief );
                        pixel[2] = byte( 76.f + .46f * grain + 3.f * relief );
                        pixel[3] = 255;
                        const float dx = ( profile( float( x ) + 1, float( y ) ) - profile( float( x ) - 1, float( y ) ) ) * size / ( 2 * tile );
                        const float dy = ( profile( float( x ), float( y ) + 1 ) - profile( float( x ), float( y ) - 1 ) ) * size / ( 2 * tile );
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
                surface->setNormalStrength( .82f );
                surface->setUVProjection( 0 );
                surface->setEditorUInt( "uvFilter", 3 );
                surface->setEditorFloat( "uvAniso", 8.f );
                return surface;
            }

            void build( SceneAssets &assets, const CityRoadTerrainMesh &footways )
            {
                const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                if( !terrain ) return;
                const auto &placement = assets.roadAuthoring->getTerrainPlacement();
                const auto &source = assets.roadAuthoring->getSource();
                CityRoadJunctionBake connections;
                String error;
                const auto sampleHeight = [&]( const Vector3F &point, float &height ) {
                    return render::sampleTerrainHeight( *terrain, placement, point, height );
                };
                if( !buildCityRoadJunctions( source, assets.roadAuthoring->getBake(), sampleHeight, {}, connections, error ) ) return;

                const auto crossings = cityRoadCrossings::collect( source, connections, footways );
                Geometry geometry;
                size_t panels = 0;
                for( const auto &crossing : crossings )
                {
                    const float inner = crossing.width * .5f + .13f;
                    for( const float side : { -1.f, 1.f } )
                    {
                        const auto start = geometry.indices.size();
                        project( geometry, footways, { crossing.centre, crossing.outward, crossing.across * side,
                                                      crossing.depth * .5f - .16f, inner, inner + .77f } );
                        if( geometry.indices.size() > start ) ++panels;
                    }
                }
                if( geometry.indices.empty() ) return;
                const auto actor = mesh( assets, "Terrain city tactile crossing landings", geometry, materialForLandings( assets ) );
                actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                WP_LOG( String( "Terrain city tactile landings=" ) + StringUtil::toString( panels ) +
                        " crossings=" + StringUtil::toString( crossings.size() ) +
                        " triangles=" + StringUtil::toString( geometry.indices.size() / 3 ) );
            }
        }  // namespace cityCrossingLandings
