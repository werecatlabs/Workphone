#pragma once

        namespace cityFootwayAppearance
        {
            constexpr float curbTileMetres = 5.76f;
            constexpr u32 pavingPairs = 6, pavingCourses = 10;
            constexpr float pavingPairMetres = .80f, pavingCourseMetres = .40f;
            constexpr float pavingTileU = pavingPairs * pavingPairMetres;
            constexpr float pavingTileV = pavingCourses * pavingCourseMetres;

            float unit( u64 key )
            {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            }

            float ramp( float low, float high, float value )
            {
                const float t = std::clamp( ( value - low ) / ( high - low ), 0.f, 1.f );
                return t * t * ( 3.f - 2.f * t );
            }

            u8 asByte( float value )
            {
                return u8( std::clamp( value + .5f, 0.f, 255.f ) );
            }

            float aggregateNoise( u32 seed, float u, float v, u32 columns, u32 rows )
            {
                const float x = u * columns, y = v * rows;
                const auto column = u32( x ), row = u32( y );
                const auto noise = [&]( u32 ix, u32 iy ) {
                    return unit( ( u64( seed ) << 32 ) | ( iy % rows * columns + ix % columns ) );
                };
                const float fx = ramp( 0, 1, x - column ), fy = ramp( 0, 1, y - row );
                const float upper = noise( column, row ) * ( 1.f - fx ) + noise( column + 1, row ) * fx;
                const float lower = noise( column, row + 1 ) * ( 1.f - fx ) + noise( column + 1, row + 1 ) * fx;
                return upper * ( 1.f - fy ) + lower * fy;
            }

            void stonePixels( TextureBuffer &colour, TextureBuffer &roughness, u32 x, u32 y,
                              float tone, float jointTone, float face, float wear, float aggregate, float warmth = 0.f )
            {
                auto pixel = colour.pixel( x, y );
                pixel[0] = asByte( jointTone + 3.f + ( tone - jointTone + warmth ) * face );
                pixel[1] = asByte( jointTone + 1.f + ( tone - jointTone + 1.f + .5f * warmth ) * face );
                pixel[2] = asByte( jointTone - 3.f + ( tone - jointTone + 1.f ) * face );
                pixel[3] = 255;
                auto rough = roughness.pixel( x, y );
                rough[0] = rough[1] = rough[2] = asByte( 245.f - face * ( 17.f + 13.f * wear - 7.f * aggregate ) );
                rough[3] = 255;
            }

            SmartPtr<render::IMaterial> stoneMaterial( SceneAssets &assets, const TextureBuffer &colour,
                                                       const TextureBuffer &roughness,
                                                       const SourceBudgetVector<float> &level,
                                                       float tileU, float tileV, bool wrapV, float strength )
            {
                const auto width = colour.width, height = colour.height;
                TextureBuffer normal( width, height );
                const auto point = [&]( u32 x, u32 y ) { return level[size_t( y ) * width + x]; };
                for( u32 y = 0; y < height; ++y )
                {
                    const auto next = wrapV ? ( y + 1 ) % height : std::min( y + 1, height - 1 );
                    const auto previous = y ? y - 1 : ( wrapV ? height - 1 : 0 );
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float dx = ( point( ( x + 1 ) % width, y ) - point( ( x + width - 1 ) % width, y ) ) *
                                         ( width / ( 2.f * tileU ) );
                        const float dy = ( point( x, next ) - point( x, previous ) ) * ( height / ( 2.f * tileV ) );
                        const auto n = Vector3F( -dx, -dy, 1 ).normaliseCopy();
                        auto pixel = normal.pixel( x, y );
                        pixel[0] = asByte( 127.5f * ( 1.f + n.x ) );
                        pixel[1] = asByte( 127.5f * ( 1.f + n.y ) );
                        pixel[2] = asByte( 127.5f * ( 1.f + n.z ) );
                        pixel[3] = 255;
                    }
                }
                auto result = material( assets, ColourF::White, 1.f );
                result->setTexture( upload( assets, colour ), 0 );
                result->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
                result->setTexture( upload( assets, roughness, render::TextureMipFilter::Roughness ), 3 );
                result->setUVProjection( 0 );
                result->setNormalStrength( strength );
                result->setEditorUInt( "uvFilter", 3 );
                result->setEditorFloat( "uvAniso", 8.f );
                return result;
            }

            SmartPtr<render::IMaterial> curbMaterial( SceneAssets &assets, u32 seed )
            {
                constexpr u32 width = 512, height = 64;
                constexpr std::array<float, 9> joints{ 0.f, .70f, 1.43f, 2.09f, 2.87f, 3.57f, 4.33f, 5.02f,
                                                       curbTileMetres };
                TextureBuffer colour( width, height ), roughness( width, height );
                SourceBudgetVector<float> level( size_t( width ) * height );
                for( u32 y = 0; y < height; ++y )
                {
                    const float v = ( y + .5f ) / height;
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float u = ( x + .5f ) / width;
                        const float station = u * curbTileMetres;
                        const auto next = std::upper_bound( joints.begin(), joints.end(), station );
                        const auto course = u32( next - joints.begin() - 1 );
                        const auto block = unit( seed + 1991u + course );
                        const float fine = aggregateNoise( seed + 71u, u, v, 196, 13 );
                        const float mineral = aggregateNoise( seed + 101u, u, v, 66, 5 );
                        const float weather = aggregateNoise( seed + 181u, u, v, 23, 7 );
                        const float streak = aggregateNoise( seed + 187u, u, v, 72, 3 );
                        const float jointDistance = std::min( station - joints[course], *next - station );
                        const float face = ramp( .004f, .020f, jointDistance - .004f * mineral );
                        const float wornEdge = 1.f - ramp( .05f, .37f, v );
                        const float dirt = ramp( .40f, .96f, v ) * ( .25f + .75f * mineral );
                        // Road spray and runoff darken the lower kerb irregularly;
                        // the cap remains lighter where feet and tyres polish it.
                        const float waterline = ramp( .52f, .90f, v ) *
                                                ( .35f + .65f * ramp( .55f, .84f, weather ) * streak );
                        const float capChips = ( 1.f - ramp( .03f, .12f, v ) ) * ramp( .78f, .93f, fine );
                        const float granite = 140.f + 16.f * block + 13.f * ( fine - .5f ) +
                                              10.f * wornEdge - 18.f * dirt - 11.f * waterline + 5.f * capChips;
                        stonePixels( colour, roughness, x, y, granite, 108.f + 9.f * mineral, face, wornEdge, fine );
                        level[size_t( y ) * width + x] = face *
                            ( .0034f + .0012f * fine - .0011f * wornEdge - .0003f * waterline );
                    }
                }
                return stoneMaterial( assets, colour, roughness, level, curbTileMetres, .15f, false, .57f );
            }

            SmartPtr<render::IMaterial> pavingMaterial( SceneAssets &assets, u32 seed )
            {
                // Keep the slab dimensions fixed while resolving their aggregate
                // and narrow joints at street-view distances.
                constexpr u32 width = 1024, height = 1024;
                constexpr float longSlab = .52f;
                TextureBuffer colour( width, height ), roughness( width, height );
                SourceBudgetVector<float> level( size_t( width ) * height );
                for( u32 y = 0; y < height; ++y )
                {
                    const float v = ( y + .5f ) / height;
                    const float across = v * pavingCourses;
                    const auto row = u32( across );
                    const float withinRow = ( across - row ) * pavingCourseMetres;
                    const float rowDistance = std::min( withinRow, pavingCourseMetres - withinRow );
                    const float stagger = row % 2 ? .40f : .13f;
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float u = ( x + .5f ) / width;
                        const float along = u * pavingTileU + stagger;
                        const auto pair = u32( along / pavingPairMetres );
                        const float withinPair = along - pair * pavingPairMetres;
                        const bool shortSlab = withinPair > longSlab;
                        const float slabDistance = shortSlab ? std::min( withinPair - longSlab,
                                                                       pavingPairMetres - withinPair ) :
                                                              std::min( withinPair, longSlab - withinPair );
                        const float fine = aggregateNoise( seed + 221u, u, v, 768, 640 );
                        const float mineral = aggregateNoise( seed + 467u, u, v, 167, 139 );
                        const float bloom = aggregateNoise( seed + 463u, u, v, 49, 39 );
                        const float weather = aggregateNoise( seed + 727u, u, v, 12, 10 );
                        const float runoff = aggregateNoise( seed + 733u, u, v, 52, 26 );
                        const auto slab = ( row * pavingPairs + pair % pavingPairs ) * 2u + u32( shortSlab );
                        const u64 slabKey = ( u64( seed ) << 32 ) | slab;
                        const float variation = unit( slabKey );
                        const float wear = unit( slabKey ^ 0x9829a1u );
                        const float distance = std::min( rowDistance, slabDistance );
                        const float face = ramp( .0025f, .0105f, distance - .0015f * fine );
                        const float patina = ramp( .55f, .88f, bloom ) * ( 1.f - wear );
                        const float rainStain = ramp( .61f, .83f, weather ) *
                                                ( .35f + .65f * runoff ) * ( .65f + .35f * ( 1.f - wear ) );
                        const float grain = unit( ( u64( seed + 529u ) << 32 ) | ( y * width + x ) );
                        const float lightMineral = ramp( .67f, .86f, fine );
                        const float pore = 1.f - ramp( .045f, .17f, fine );
                        const float edgeDirt = ( 1.f - ramp( .010f, .048f, distance ) ) *
                                               ( .35f + .65f * mineral ) * ( .30f + .70f * ( 1.f - wear ) );
                        const float concrete = 142.f + 16.f * variation + 11.f * ( fine - .5f ) +
                                               5.f * ( mineral - .5f ) + 4.f * ( grain - .5f ) +
                                               5.f * lightMineral - 8.f * pore - 9.f * patina -
                                               11.f * edgeDirt - 12.f * rainStain;
                        stonePixels( colour, roughness, x, y, concrete, 108.f + 7.f * bloom,
                                     face, wear, fine, 3.f + 5.f * variation );
                        auto rough = roughness.pixel( x, y );
                        rough[0] = rough[1] = rough[2] = asByte(
                            233.f - face * ( 8.f + 11.f * wear ) + 4.f * pore -
                            7.f * lightMineral + 5.f * rainStain );
                        level[size_t( y ) * width + x] =
                            face * ( .0026f + .0011f * variation + .00042f * fine + .00018f * grain -
                                     .00040f * pore + .00012f * lightMineral - .0003f * rainStain );
                    }
                }
                return stoneMaterial( assets, colour, roughness, level, pavingTileU, pavingTileV, true, .64f );
            }

            SmartPtr<render::IMaterial> sharedTopMaterial( SceneAssets &assets, SmartPtr<render::IMaterial> shared )
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
                            Vector2F uv, Vector3F tangent, float handedness = 1.f )
            {
                tangent -= vertex.normal * tangent.dotProduct( vertex.normal );
                tangent.normalise();
                geometry.positions.push_back( vertex.position );
                geometry.normals.push_back( vertex.normal );
                geometry.uv.push_back( { uv.x, uv.y, 0 } );
                geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, handedness } );
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
                    if( std::abs( top.normal.y ) < .01f && top.position.y - bottom.position.y > .012f &&
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
                        std::abs( a.normal.y ) > .01f || a.position.y - b.position.y < .012f ||
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
                    if( sectioned )
                    {
                        const auto tangent = std::abs( vertex.normal.y ) > .001f ?
                            Vector3F( vertex.normal.y, -vertex.normal.x, 0.f ) :
                            Vector3F( -vertex.normal.z, 0.f, vertex.normal.x );
                        addVertex( slab, vertex, { vertex.position.x / pavingTileU, vertex.position.z / pavingTileV },
                                   tangent, -1.f );
                    }
                    else
                    {
                        const auto tangent = std::abs( vertex.normal.y ) > .9f ? Vector3F( 1, 0, 0 ) :
                                             Vector3F( -vertex.normal.z, 0, vertex.normal.x );
                        addVertex( slab, vertex, vertex.uv, tangent );
                    }
                }
                const auto slabEnd = sectioned ? topEnd : fitted.indices.end();
                slab.indices.reserve( slabEnd - fitted.indices.begin() );
                for( auto index = fitted.indices.begin(); index != slabEnd; ++index )
                    slab.indices.push_back( *index );
                auto mesh = pack( slab );
                if( !sectioned ) return { mesh, sharedTopMaterial( assets, paving ), nullptr };

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
                return { mesh, pavingMaterial( assets, seed ), curbMaterial( assets, seed ) };
            }
        }  // namespace cityFootwayAppearance
