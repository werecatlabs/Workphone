#pragma once

        namespace cityFootwayAppearance
        {
            constexpr float curbTileMetres = 5.76f;

            float unit( u64 key )
            {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            }

            float ramp( float low, float high, float value )
            {
                const float t = std::clamp( ( value - low ) / ( high - low ), 0.f, 1.f );
                return t * t * ( 3.f - 2.f * t );
            }

            SmartPtr<render::IMaterial> curbMaterial( SceneAssets &assets, u32 seed )
            {
                constexpr u32 width = 512, height = 64, stones = 8;
                TextureBuffer colour( width, height ), normal( width, height );
                SourceBudgetVector<float> level( size_t( width ) * height );
                const auto byte = []( float value ) { return u8( std::clamp( value, 0.f, 255.f ) ); };
                for( u32 y = 0; y < height; ++y )
                {
                    const float v = ( y + .5f ) / height;
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float coordinate = ( x + .5f ) * stones / width;
                        const auto course = u32( coordinate );
                        const float phase = coordinate - course;
                        const auto grain = unit( ( u64( seed ) << 32 ) | ( y * width + x ) );
                        const auto flecks = unit( ( u64( seed + 71 ) << 32 ) | ( y / 3 * width + x / 3 ) );
                        const auto block = unit( seed + 1991u + course );
                        const float jointDistance = std::min( phase, 1.f - phase );
                        const float face = ramp( .007f, .027f, jointDistance - .006f * flecks );
                        const float wornEdge = ( 1.f - ramp( .06f, .23f, v ) ) * ( .68f + .32f * flecks );
                        const float dirt = ramp( .35f, .95f, v ) * ( .55f + .45f * flecks );
                        const float joints = 93.f + 14.f * flecks;
                        const float aggregate = ( grain - .5f ) * 24.f + ( flecks - .5f ) * 12.f;
                        const float granite = 145.f + 21.f * block + aggregate + 14.f * wornEdge - 31.f * dirt;
                        auto pixel = colour.pixel( x, y );
                        pixel[0] = byte( joints + ( granite + 3.f - joints ) * face );
                        pixel[1] = byte( joints - 4.f + ( granite - joints ) * face );
                        pixel[2] = byte( joints - 9.f + ( granite - 1.f - joints ) * face );
                        pixel[3] = 255;
                        level[size_t( y ) * width + x] = face * ( .0039f + .0011f * grain - .0012f * wornEdge );
                    }
                }
                const auto point = [&]( u32 x, u32 y ) { return level[size_t( y ) * width + x]; };
                for( u32 y = 0; y < height; ++y )
                {
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float dx = ( point( ( x + 1 ) % width, y ) - point( ( x + width - 1 ) % width, y ) ) *
                                         ( width / ( 2.f * curbTileMetres ) );
                        const float dy = ( point( x, std::min( y + 1, height - 1 ) ) - point( x, y ? y - 1 : 0 ) ) *
                                         ( height / .30f );
                        const auto n = Vector3F( -dx, -dy, 1 ).normaliseCopy();
                        auto pixel = normal.pixel( x, y );
                        pixel[0] = byte( 127.5f * ( 1.f + n.x ) );
                        pixel[1] = byte( 127.5f * ( 1.f + n.y ) );
                        pixel[2] = byte( 127.5f * ( 1.f + n.z ) );
                        pixel[3] = 255;
                    }
                }
                auto result = material( assets, ColourF::White, .91f );
                result->setTexture( upload( assets, colour ), 0 );
                result->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
                result->setUVProjection( 0 );
                result->setNormalStrength( .59f );
                result->setEditorUInt( "uvFilter", 3 );
                result->setEditorFloat( "uvAniso", 8.f );
                return result;
            }

            SmartPtr<render::IMaterial> topMaterial( SceneAssets &assets, SmartPtr<render::IMaterial> shared )
            {
                auto result = material( assets, shared->getDiffuse(), shared->getRoughness(), shared->getMetalness() );
                for( const u32 layer : { 0u, 1u, 2u, 3u, 22u } )
                    if( auto texture = shared->getTexture( layer ) ) result->setTexture( texture, layer );
                result->setUVProjection( shared->getUVProjection() );
                result->setTriplanarScale( shared->getTriplanarScale() );
                result->setNormalStrength( .59f );
                result->setEditorFloat( "aoStrength", .82f );
                result->setEditorUInt( "uvFilter", 3 );
                result->setEditorFloat( "uvAniso", 8.f );
                return result;
            }

            void addVertex( Geometry &geometry, const CityRoadTerrainVertex &vertex,
                            Vector2F uv, Vector3F tangent )
            {
                tangent -= vertex.normal * tangent.dotProduct( vertex.normal );
                tangent.normalise();
                geometry.positions.push_back( vertex.position );
                geometry.normals.push_back( vertex.normal );
                geometry.uv.push_back( { uv.x, uv.y, 0 } );
                geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
            }

            SmartPtr<IMesh> pack( const Geometry &geometry )
            {
                return MeshUtil::createMesh( geometry.positions, geometry.normals, geometry.tangents,
                                             geometry.uv, geometry.indices );
            }

            constexpr u32 missing = std::numeric_limits<u32>::max();
            using Node = std::pair<s64, s64>;
            Node node( Vector3F point )
            {
                return { std::llround( double( point.x ) * 512 ), std::llround( double( point.z ) * 512 ) };
            }

            struct Edge
            {
                std::array<u32, 4> vertices;
                Vector3F direction;
                float length, u0 = 0, u1 = 0;
                u32 next = missing, previous = missing;
                bool assigned = false;
            };

            void assignStations( SourceBudgetVector<Edge> &edges, const CityRoadTerrainMesh &fitted, u32 seed )
            {
                SourceBudgetMap<Node, SourceBudgetVector<u32>> outgoing;
                for( u32 i = 0; i < edges.size(); ++i )
                    outgoing[node( fitted.vertices[edges[i].vertices[0]].position )].push_back( i );
                for( u32 i = 0; i < edges.size(); ++i )
                {
                    const auto end = fitted.vertices[edges[i].vertices[3]].position;
                    const auto found = outgoing.find( node( end ) );
                    if( found == outgoing.end() ) continue;
                    u32 best = missing;
                    float alignment = -.55f;
                    for( const auto next : found->second )
                    {
                        if( next == i ) continue;
                        const auto start = fitted.vertices[edges[next].vertices[0]].position;
                        const float score = edges[i].direction.dotProduct( edges[next].direction );
                        if( ( start - end ).lengthSquared() > .000016f || score <= alignment ) continue;
                        if( edges[next].previous != missing &&
                            edges[edges[next].previous].direction.dotProduct( edges[next].direction ) >= score ) continue;
                        alignment = score;
                        best = next;
                    }
                    if( best == missing ) continue;
                    auto &next = edges[best];
                    if( next.previous != missing ) edges[next.previous].next = missing;
                    next.previous = i;
                    edges[i].next = best;
                }

                SourceBudgetVector<u32> contour;
                for( const bool open : { true, false } )
                {
                    for( u32 first = 0; first < edges.size(); ++first )
                    {
                        if( edges[first].assigned || ( edges[first].previous == missing ) != open ) continue;
                        contour.clear();
                        float length = 0;
                        u32 index = first;
                        while( index != missing && !edges[index].assigned )
                        {
                            auto &edge = edges[index];
                            edge.assigned = true;
                            length += edge.length;
                            contour.push_back( index );
                            index = edge.next;
                        }
                        const bool closed = index == first && length >= 2.8f;
                        const float tile = closed ? length / std::max( 1.f, std::round( length / curbTileMetres ) ) :
                                                   curbTileMetres;
                        const auto start = node( fitted.vertices[edges[first].vertices[0]].position );
                        const float phase = unit( ( u64( u32( start.first ) ) << 32 ) | ( u32( start.second ) ^ seed ) );
                        float station = 0;
                        for( const auto member : contour )
                        {
                            auto &edge = edges[member];
                            edge.u0 = phase + station / tile;
                            station += edge.length;
                            edge.u1 = phase + station / tile;
                        }
                    }
                }
            }

            struct Appearance
            {
                SmartPtr<IMesh> mesh;
                SmartPtr<render::IMaterial> slab, curb;
            };

            Appearance prepare( SceneAssets &assets, const CityRoadTerrainMesh &fitted,
                                SmartPtr<render::IMaterial> paving, u32 seed )
            {
                if( fitted.indices.empty() ) return {};
                auto edgeStart = fitted.indices.size();
                for( size_t i = 0; i + 2 < fitted.indices.size(); i += 3 )
                {
                    const auto &top = fitted.vertices[fitted.indices[i]];
                    const auto &bottom = fitted.vertices[fitted.indices[i + 1]];
                    if( std::abs( top.normal.y ) < .01f && top.position.y - bottom.position.y > .10f &&
                        std::abs( top.position.x - bottom.position.x ) < .001f &&
                        std::abs( top.position.z - bottom.position.z ) < .001f )
                    {
                        edgeStart = i;
                        break;
                    }
                }

                SourceBudgetVector<Edge> edges;
                size_t vertexCount = fitted.vertices.size();
                for( size_t i = edgeStart; i + 5 < fitted.indices.size(); i += 6 )
                {
                    const auto *j = fitted.indices.data() + i;
                    const auto &a = fitted.vertices[j[0]], &b = fitted.vertices[j[1]];
                    const auto &c = fitted.vertices[j[2]], &d = fitted.vertices[j[5]];
                    auto delta = d.position - a.position;
                    delta.y = 0;
                    const float length = delta.length();
                    if( j[0] != j[3] || j[2] != j[4] || length < .00001f ||
                        std::abs( a.normal.y ) > .01f || a.position.y - b.position.y < .10f ||
                        std::abs( a.position.x - b.position.x ) > .001f ||
                        std::abs( a.position.z - b.position.z ) > .001f ||
                        std::abs( d.position.x - c.position.x ) > .001f ||
                        std::abs( d.position.z - c.position.z ) > .001f ) break;
                    edges.push_back( { { j[0], j[1], j[2], j[5] }, delta / length, length } );
                    vertexCount = std::min( vertexCount, size_t( std::min( { j[0], j[1], j[2], j[5] } ) ) );
                }
                const auto topEnd = fitted.indices.begin() + edgeStart;
                const bool sectioned = !edges.empty() && edgeStart > 0 && edgeStart + edges.size() * 6 == fitted.indices.size() &&
                    std::all_of( fitted.indices.begin(), topEnd, [vertexCount]( u32 index ) { return index < vertexCount; } );

                Geometry slab;
                for( size_t i = 0; i < ( sectioned ? vertexCount : fitted.vertices.size() ); ++i )
                {
                    const auto &vertex = fitted.vertices[i];
                    const auto tangent = std::abs( vertex.normal.y ) > .9f ? Vector3F( 1, 0, 0 ) :
                                          Vector3F( -vertex.normal.z, 0, vertex.normal.x );
                    addVertex( slab, vertex, vertex.uv, tangent );
                }
                const auto slabEnd = sectioned ? topEnd : fitted.indices.end();
                slab.indices.reserve( slabEnd - fitted.indices.begin() );
                for( auto index = fitted.indices.begin(); index != slabEnd; ++index )
                    slab.indices.push_back( *index );
                auto mesh = pack( slab );
                auto top = topMaterial( assets, paving );
                if( !sectioned ) return { mesh, top, nullptr };

                assignStations( edges, fitted, seed );
                Geometry curb;
                for( const auto &edge : edges )
                {
                    const auto base = u32( curb.positions.size() );
                    for( u32 i = 0; i < edge.vertices.size(); ++i )
                        addVertex( curb, fitted.vertices[edge.vertices[i]],
                                   { i < 2 ? edge.u0 : edge.u1, i == 0 || i == 3 ? .045f : .955f },
                                   edge.direction );
                    // The original (top A, bottom A, bottom B), (top A, bottom B,
                    // top B) faces are retained once, with their exact XYZ and winding.
                    for( const auto index : { 0u, 1u, 2u, 0u, 2u, 3u } ) curb.indices.push_back( base + index );
                }
                for( auto section : pack( curb )->getSubMeshes() ) mesh->addSubMesh( section );
                return { mesh, top, curbMaterial( assets, seed ) };
            }
        }  // namespace cityFootwayAppearance
