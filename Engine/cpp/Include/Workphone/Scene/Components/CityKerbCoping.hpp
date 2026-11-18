#pragma once

        namespace cityKerbCoping
        {
            constexpr float width = .17f, lift = .003f;
            constexpr size_t maximumTriangles = 60000;

            // Only top triangles take part: the finish cannot span a hole, a ramp
            // crease or the carriageway. Its collision remains the fitted footway.
            struct TopSurface
            {
                SourceBudgetVector<cityRoadEdges::Face> faces;
                SourceBudgetMap<std::pair<s32, s32>, SourceBudgetVector<u32>> cells;

                explicit TopSurface( const CityRoadTerrainMesh &walks )
                {
                    for( size_t i = 0; i + 2 < walks.indices.size(); i += 3 )
                    {
                        const auto &a = walks.vertices[walks.indices[i]];
                        if( a.normal.y < .7f ) continue;
                        const auto b = walks.vertices[walks.indices[i + 1]].position;
                        const auto c = walks.vertices[walks.indices[i + 2]].position;
                        if( std::abs( cityRoadEdges::cross( b - a.position, c - a.position ) ) < 1e-8f ) continue;
                        const auto id = u32( faces.size() );
                        faces.emplace_back( a.position, b, c );
                        faces.back().bounds.cells( [&]( auto cell ) { cells[cell].push_back( id ); return false; } );
                    }
                }
            };

            size_t clip( std::array<Vector3F, 8> &polygon, const cityRoadEdges::Face &face,
                         const std::array<Vector3F, 4> &footprint )
            {
                std::copy( face.points.begin(), face.points.end(), polygon.begin() );
                size_t count = 3;
                const auto sign = cityRoadEdges::cross( footprint[1] - footprint[0], footprint[3] - footprint[0] ) >= 0 ? 1.f : -1.f;
                std::array<Vector3F, 8> scratch;
                for( size_t i = 0; i < 4 && count >= 3; ++i )
                {
                    const auto start = footprint[i], edge = footprint[( i + 1 ) % 4] - start;
                    size_t output = 0;
                    for( size_t j = 0; j < count; ++j )
                    {
                        const auto p = polygon[j], q = polygon[( j + 1 ) % count];
                        const auto dp = sign * cityRoadEdges::cross( edge, p - start );
                        const auto dq = sign * cityRoadEdges::cross( edge, q - start );
                        if( dp >= 0 ) scratch[output++] = p;
                        if( ( dp > 0 && dq < 0 ) || ( dp < 0 && dq > 0 ) )
                            scratch[output++] = p + ( q - p ) * ( dp / ( dp - dq ) );
                    }
                    count = output;
                    polygon.swap( scratch );
                }
                return count;
            }

            Vector3F inset( Vector3F current, Vector3F adjacent )
            {
                const auto bisector = -( current + adjacent );
                const auto length = bisector.length();
                if( length < .0001f ) return Vector3F( 0, 0, 0 );
                const auto reach = std::min( width / ( .5f * length ), 2.f * width );
                return bisector * ( reach / length );
            }

            void append( Geometry &target, const Geometry &part )
            {
                const auto base = u32( target.positions.size() );
                target.positions.insert( target.positions.end(), part.positions.begin(), part.positions.end() );
                target.normals.insert( target.normals.end(), part.normals.begin(), part.normals.end() );
                target.uv.insert( target.uv.end(), part.uv.begin(), part.uv.end() );
                target.tangents.insert( target.tangents.end(), part.tangents.begin(), part.tangents.end() );
                for( const auto i : part.indices ) target.indices.push_back( base + i );
            }

            struct SectionVertices
            {
                static constexpr float positionTolerance = .000065f;
                static constexpr float attributeTolerance = .000012f;
                SourceBudgetMap<std::array<s64, 3>, u32> buckets;
                SourceBudgetVector<u32> next;

                u32 index( Geometry &geometry, Vector3F p, Vector3F n, Vector3F uv, Vector4F tangent )
                {
                    const auto coordinate = []( float v ) { return std::llround( double( v ) * 4096.0 ); };
                    const std::array<s64, 3> key{ coordinate( p.x ), coordinate( p.y ), coordinate( p.z ) };
                    auto &first = buckets.try_emplace( key, cityFootwayAppearance::missing ).first->second;
                    const auto close = []( Vector3F a, Vector3F b, float tolerance ) {
                        return ( a - b ).lengthSquared() <= tolerance * tolerance;
                    };
                    const auto direction = Vector3F( tangent.x, tangent.y, tangent.z );
                    for( u32 i = first; i != cityFootwayAppearance::missing; i = next[i] )
                    {
                        const auto t = geometry.tangents[i];
                        if( t.w == tangent.w && close( geometry.positions[i], p, positionTolerance ) &&
                            close( geometry.normals[i], n, attributeTolerance ) &&
                            close( geometry.uv[i], uv, attributeTolerance ) &&
                            close( Vector3F( t.x, t.y, t.z ), direction, attributeTolerance ) ) return i;
                    }
                    const auto result = u32( geometry.positions.size() );
                    geometry.positions.push_back( p );
                    geometry.normals.push_back( n );
                    geometry.uv.push_back( uv );
                    geometry.tangents.push_back( tangent );
                    next.push_back( first );
                    first = result;
                    return result;
                }
            };

            Geometry generate( const CityRoadTerrainMesh &walks, const cityRoadEdges::Surface &road, u32 seed,
                               u32 &sections )
            {
                SourceBudgetVector<cityFootwayAppearance::Edge> edges;
                for( size_t i = 0; i + 5 < walks.indices.size(); i += 3 )
                {
                    const auto *j = walks.indices.data() + i;
                    const auto &a = walks.vertices[j[0]], &b = walks.vertices[j[1]];
                    const auto &c = walks.vertices[j[2]], &d = walks.vertices[j[5]];
                    if( j[0] != j[3] || j[2] != j[4] || std::abs( a.normal.y ) > .01f ||
                        a.position.y - b.position.y < .012f ||
                        std::hypot( a.position.x - b.position.x, a.position.z - b.position.z ) > .001f ||
                        std::hypot( d.position.x - c.position.x, d.position.z - c.position.z ) > .001f ) continue;
                    auto delta = d.position - a.position; delta.y = 0;
                    const auto length = delta.length();
                    if( length < .00001f ) continue;
                    edges.push_back( { { j[0], j[1], j[2], j[5] }, delta / length, length } );
                    i += 3;
                }
                cityFootwayAppearance::assignStations( edges, walks, seed );
                // An unlinked span can be a genuine end or a corner that could
                // not share a miter with its neighbor. Only true ends get a
                // square finish. Check geometric proximity across cell boundaries.
                constexpr float endpointTolerance = .004f;
                const auto endpointCell = [endpointTolerance]( Vector3F p ) -> cityFootwayAppearance::Node {
                    return { s64( std::floor( p.x / endpointTolerance ) ),
                             s64( std::floor( p.z / endpointTolerance ) ) };
                };
                SourceBudgetVector<u32> endpoints;
                endpoints.reserve( edges.size() * 2 );
                for( u32 i = 0; i < edges.size(); ++i )
                {
                    endpoints.push_back( i * 2 );
                    endpoints.push_back( i * 2 + 1 );
                }
                const auto endpointPoint = [&]( u32 endpoint ) {
                    return walks.vertices[edges[endpoint / 2].vertices[endpoint % 2 ? 3 : 0]].position;
                };
                const auto cellOf = [&]( u32 endpoint ) { return endpointCell( endpointPoint( endpoint ) ); };
                std::sort( endpoints.begin(), endpoints.end(), [&]( u32 a, u32 b ) { return cellOf( a ) < cellOf( b ); } );
                const auto touchesAnother = [&]( u32 index, Vector3F point ) {
                    const auto cell = endpointCell( point );
                    for( const int dx : { -1, 0, 1 } )
                    for( const int dz : { -1, 0, 1 } )
                    {
                        const cityFootwayAppearance::Node neighbor( cell.first + dx, cell.second + dz );
                        const auto first = std::lower_bound( endpoints.begin(), endpoints.end(), neighbor,
                            [&]( u32 endpoint, const auto &key ) { return cellOf( endpoint ) < key; } );
                        for( auto found = first; found != endpoints.end() && cellOf( *found ) == neighbor; ++found )
                        {
                            if( *found / 2 != index &&
                                ( endpointPoint( *found ) - point ).lengthSquared() <= endpointTolerance * endpointTolerance )
                                return true;
                        }
                    }
                    return false;
                };
                const TopSurface top( walks );
                Geometry result;
                sections = 0;
                SourceBudgetVector<u32> candidates;
                for( u32 i = 0; i < edges.size(); ++i )
                {
                    const auto remaining = maximumTriangles - result.indices.size() / 3;
                    if( remaining == 0 ) break;
                    const auto &edge = edges[i];
                    const auto a = walks.vertices[edge.vertices[0]].position;
                    const auto b = walks.vertices[edge.vertices[3]].position;
                    const auto outward = walks.vertices[edge.vertices[0]].normal;
                    const auto mid = ( a + b ) * .5f;
                    if( !road.contains( mid + outward * .04f ) && !road.contains( mid + outward * .12f ) ) continue;
                    if( ( edge.previous == cityFootwayAppearance::missing && touchesAnother( i, a ) ) ||
                        ( edge.next == cityFootwayAppearance::missing && touchesAnother( i, b ) ) ) continue;
                    const auto prior = edge.previous == cityFootwayAppearance::missing ? outward :
                        walks.vertices[edges[edge.previous].vertices[0]].normal;
                    const auto next = edge.next == cityFootwayAppearance::missing ? outward :
                        walks.vertices[edges[edge.next].vertices[0]].normal;
                    const auto startInset = inset( outward, prior ), endInset = inset( outward, next );
                    const auto startProjection = cityRoadEdges::cross( startInset, edge.direction );
                    const auto endProjection = cityRoadEdges::cross( endInset, edge.direction );
                    if( std::abs( startProjection ) < 1e-6f || std::abs( endProjection ) < 1e-6f ) continue;
                    const std::array footprint{ a, b, b + endInset, a + startInset };
                    const auto orientation = cityRoadEdges::cross( footprint[1] - footprint[0], footprint[2] - footprint[1] );
                    bool convex = std::abs( orientation ) > 1e-8f;
                    for( size_t corner = 1; corner < 4; ++corner )
                        convex = convex && orientation * cityRoadEdges::cross(
                            footprint[( corner + 1 ) % 4] - footprint[corner],
                            footprint[( corner + 2 ) % 4] - footprint[( corner + 1 ) % 4] ) > 0;
                    if( !convex ) continue;
                    const cityRoadEdges::Bounds bounds( footprint );
                    candidates.clear();
                    bounds.cells( [&]( auto cell ) {
                        const auto found = top.cells.find( cell );
                        if( found != top.cells.end() ) candidates.insert( candidates.end(), found->second.begin(), found->second.end() );
                        return false;
                    } );
                    std::sort( candidates.begin(), candidates.end() );
                    candidates.erase( std::unique( candidates.begin(), candidates.end() ), candidates.end() );
                    Geometry section;
                    SectionVertices vertices;
                    for( const auto id : candidates )
                    {
                        const auto &face = top.faces[id];
                        if( !bounds.intersects( face.bounds ) ) continue;
                        std::array<Vector3F, 8> polygon;
                        const auto count = clip( polygon, face, footprint );
                        auto normal = ( face.points[1] - face.points[0] ).crossProduct( face.points[2] - face.points[0] ).normaliseCopy();
                        if( normal.y < 0 ) normal = -normal;
                        const auto tangent = ( edge.direction - normal * normal.dotProduct( edge.direction ) ).normaliseCopy();
                        const auto handedness = normal.crossProduct( tangent ).dotProduct( -outward ) >= 0 ? 1.f : -1.f;
                        for( size_t k = 1; k + 1 < count; ++k )
                        {
                            const auto area = ( polygon[k] - polygon[0] ).crossProduct( polygon[k + 1] - polygon[0] ).dotProduct( normal );
                            if( std::abs( area ) < 1e-8f ) continue;
                            // Commit only complete strips. Reject the next
                            // triangle before it can grow uncommitted storage.
                            if( section.indices.size() / 3 >= remaining ) return result;
                            std::array<u32, 3> triangle;
                            size_t vertexCount = 0;
                            for( const auto index : { size_t( 0 ), area > 0 ? k : k + 1, area > 0 ? k + 1 : k } )
                            {
                                const auto p = polygon[index], delta = p - a;
                                // A miter cross-section has one station across
                                // its width. The road-facing edge still uses
                                // exactly the skirt's linear station mapping.
                                const auto d0 = cityRoadEdges::cross( startInset, delta ) / startProjection;
                                const auto d1 = cityRoadEdges::cross( endInset, b - p ) / endProjection;
                                const auto fraction = std::clamp( d0 / ( d0 + d1 ), 0.f, 1.f );
                                const auto u = edge.u0 + fraction * ( edge.u1 - edge.u0 );
                                const auto v = .045f + .17f * std::clamp( -delta.dotProduct( outward ) / width, 0.f, 1.f );
                                triangle[vertexCount++] = vertices.index( section, p + Vector3F( 0, lift, 0 ), normal,
                                    { u, v, 0 }, { tangent.x, tangent.y, tangent.z, handedness } );
                            }
                            const auto p = section.positions[triangle[0]];
                            const auto q = section.positions[triangle[1]];
                            const auto r = section.positions[triangle[2]];
                            if( ( q - p ).crossProduct( r - p ).dotProduct( normal ) <= 1e-8f ) continue;
                            for( const auto index : triangle ) section.indices.push_back( index );
                        }
                    }
                    if( section.indices.empty() ) continue;
                    append( result, section );
                    ++sections;
                }
                return result;
            }

            void build( SceneAssets &assets, const CityRoadTerrainMesh &walks, const cityRoadEdges::Surface &road,
                        SmartPtr<render::IMaterial> stone, u32 seed )
            {
                if( !stone ) return;
                u32 sections = 0;
                const auto coping = generate( walks, road, seed, sections );
                if( auto actor = mesh( assets, "Terrain city kerb coping", coping, stone ) )
                {
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                    WP_LOG( String( "Terrain city kerb coping sections=" ) + StringUtil::toString( sections ) +
                        " triangles=" + StringUtil::toString( coping.indices.size() / 3 ) +
                        " vertices=" + StringUtil::toString( coping.positions.size() ) );
                }
            }
        }
