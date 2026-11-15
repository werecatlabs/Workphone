        namespace cityRoundaboutLandscape
        {
            float variation( u64 key )
            {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            }

            Vector3F bearing( float angle )
            {
                return { std::cos( angle ), 0, std::sin( angle ) };
            }

            void shrubCrown( Geometry &shape, Vector3F base, float radius, float height, u64 key )
            {
                constexpr u32 sides = 8, rings = 3;
                const auto first = u32( shape.positions.size() );
                const auto vertex = [&]( Vector3F p, Vector3F normal, Vector3F tangent ) {
                    shape.positions.push_back( p );
                    shape.normals.push_back( normal.normaliseCopy() );
                    shape.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                    shape.uv.push_back( { p.x * 2, p.z * 2, 0 } );
                };
                for( u32 ring = 0; ring < rings; ++ring )
                {
                    const float latitude = float( ring ) * pi / ( 2 * rings );
                    for( u32 side = 0; side < sides; ++side )
                    {
                        const auto direction = bearing( ( float( side ) + .27f * ring ) * 2 * pi / sides );
                        const float ripple = .82f + .18f * variation( key + ring * sides + side );
                        const auto horizontal = direction * std::cos( latitude );
                        const float up = std::sin( latitude );
                        vertex( base + horizontal * ( ripple * radius ) + Vector3F( 0, height * up, 0 ),
                                horizontal * height + Vector3F( 0, up * radius, 0 ),
                                { -direction.z, 0, direction.x } );
                    }
                }
                const auto crown = u32( shape.positions.size() );
                vertex( base + Vector3F( 0, height, 0 ), { 0, 1, 0 }, { 1, 0, 0 } );
                for( u32 side = 0; side < sides; ++side )
                {
                    auto a = first + side;
                    auto b = first + ( side + 1 ) % sides;
                    for( u32 ring = 1; ring < rings; ++ring )
                    {
                        for( const auto index : { a, a + sides, b, b, a + sides, b + sides } )
                            shape.indices.push_back( index );
                        a += sides;
                        b += sides;
                    }
                    for( const auto index : { a, crown, b } ) shape.indices.push_back( index );
                }
            }

            void build( SceneAssets &assets, u32 seed )
            {
                const int detail = std::clamp( assets.cityStreetDetail, 0, 100 );
                if( !detail ) return;

                Geometry mulch, shrubs;
                std::array<Geometry, 3> grasses;
                u32 islands = 0, beds = 0, tufts = 0, shrubGroups = 0;
                constexpr u32 maxIslands = 16;
                const auto &source = assets.roadAuthoring->getSource();
                for( const auto &island : source.roundabouts )
                {
                    // Stay inside the grass polygon and leave a continuous mown shoulder at the kerb.
                    const float radius = island.islandRadius - island.kerbWidth - .8f;
                    if( radius < 1.8f ) continue;
                    if( islands >= maxIslands ) break;
                    ++islands;
                    const auto x = u32( std::lround( island.center.x * 10 ) );
                    const auto z = u32( std::lround( island.center.z * 10 ) );
                    const auto key = render::foliageStableHash( ( u64( x ) << 32 ) | z ) ^ seed;
                    const auto along = bearing( variation( key ) * 2 * pi );
                    const Vector3F across( -along.z, 0, along.x );
                    const auto normal = Vector3F( -island.gradeX, 1, -island.gradeZ ).normaliseCopy();
                    const auto lay = [&]( Vector3F p, float height ) {
                        p.y = island.center.y + island.islandHeight + height +
                              island.gradeX * ( p.x - island.center.x ) +
                              island.gradeZ * ( p.z - island.center.z );
                        return p;
                    };
                    const auto clear = [&]( Vector3F p ) {
                        p -= island.center;
                        return p.x * p.x + p.z * p.z <= radius * radius;
                    };

                    const u32 bedCount = radius > 3.5f ? 2 : 1;
                    for( u32 bed = 0; bed < bedCount; ++bed )
                    {
                        const auto bedKey = render::foliageStableHash( key + bed );
                        const float side = bed ? 1.f : -1.f;
                        const auto centre = island.center + across * ( bedCount == 2 ? side * radius * .29f : 0 ) +
                                            along * ( bedCount == 2 ? side * radius * .10f : 0 );
                        const float length = radius * ( bed ? .47f : .60f );
                        const float width = radius * ( bed ? .23f : .29f );
                        const auto bedPoint = [&]( float angle, float span ) {
                            const float ripple = 1 + .10f * std::cos( 3 * angle + side ) + .04f * std::sin( 5 * angle );
                            return centre + along * ( span * length * std::cos( angle ) * ripple ) +
                                   across * ( span * width * std::sin( angle ) * ripple );
                        };
                        // One triangulated surface on the same grade as the island's own mesh.
                        constexpr u32 sectors = 20;
                        const auto first = u32( mulch.positions.size() );
                        const auto bedVertex = [&]( Vector3F p ) {
                            mulch.positions.push_back( lay( p, .012f ) );
                            mulch.normals.push_back( normal );
                            mulch.tangents.push_back( { along.x, -normal.dotProduct( along ) / normal.y, along.z, 1 } );
                            mulch.uv.push_back( { p.x * .54f, p.z * .54f, 0 } );
                        };
                        bedVertex( centre );
                        for( u32 i = 0; i < sectors; ++i )
                            bedVertex( bedPoint( i * 2 * pi / sectors, 1 ) );
                        for( u32 i = 0; i < sectors; ++i )
                        {
                            for( const auto index : { first, first + 1 + ( i + 1 ) % sectors, first + 1 + i } )
                                mulch.indices.push_back( index );
                        }
                        ++beds;

                        const u32 grassCount = 4 + u32( detail ) * ( bed ? 21 : 29 ) / 100;
                        for( u32 tuft = 0; tuft < grassCount; ++tuft )
                        {
                            const auto tuftKey = render::foliageStableHash( bedKey + tuft + 19 );
                            const float angle = tuft * 2.39996323f + variation( tuftKey ) * .52f;
                            const float span = .80f * std::sqrt( ( tuft + .5f ) / grassCount );
                            const auto position = bedPoint( angle, span );
                            const u32 tint = variation( tuftKey + 1 ) > .66f ? 2 : bed;
                            const float height = ( tint == 2 ? .42f : .30f ) + .22f * variation( tuftKey + 2 );
                            for( u32 blade = 0; blade < 7; ++blade )
                            {
                                const auto heading = bearing( angle + blade * 2.39996323f );
                                const auto root = lay( position + heading * .055f, .006f );
                                const float rise = height * ( .56f + .44f * variation( tuftKey + blade + 3 ) );
                                const float lean = .10f + .20f * variation( tuftKey + blade + 11 );
                                const float halfWidth = .017f + .012f * variation( tuftKey + blade + 27 );
                                cityVerge::foldedGrassBlade(
                                    grasses[tint], std::array{ root,
                                        root + heading * ( lean * .24f ) + Vector3F( 0, rise * .48f, 0 ),
                                        root + heading * ( lean * .63f ) + Vector3F( 0, rise, 0 ),
                                        root + heading * lean + Vector3F( 0, rise * .84f, 0 ) },
                                    { halfWidth * .62f, halfWidth, halfWidth * .63f },
                                    { -heading.z, 0, heading.x }, clear );
                            }
                            ++tufts;
                        }
                        const u32 shrubCount = detail >= 50 ? ( bed ? 2 : 3 ) : 1;
                        for( u32 shrub = 0; shrub < shrubCount; ++shrub )
                        {
                            const auto shrubKey = render::foliageStableHash( bedKey + 513 + shrub );
                            const float offset = ( float( shrub ) - ( shrubCount - 1 ) * .5f ) * .48f;
                            const auto position = centre + along * ( length * offset );
                            const float size = std::min( width * .48f, .49f ) * ( .82f + .18f * variation( shrubKey ) );
                            for( u32 lobe = 0; lobe < 4; ++lobe )
                            {
                                const auto direction = bearing( variation( shrubKey + 1 ) * 2 * pi + lobe * 2 * pi / 3 );
                                const auto base = lay( position + direction * ( lobe ? size * .51f : 0 ), 0 );
                                const float spread = size * ( lobe ? .67f : 1 );
                                const float height = ( lobe ? .29f : .48f ) + .10f * variation( shrubKey + lobe + 2 );
                                shrubCrown( shrubs, base, spread, height, shrubKey + lobe * 37 );
                            }
                            ++shrubGroups;
                        }
                    }
                }
                if( !islands ) return;

                constexpr u32 textureSize = 64;
                TextureBuffer loam( textureSize, textureSize );
                for( u32 i = 0; i < textureSize * textureSize; ++i )
                {
                    const float noise = variation( i + 217 ) * .74f +
                                        variation( ( i / textureSize / 4 ) * textureSize + i % textureSize / 4 ) * .26f;
                    loam.pixels[i * 4] = u8( 82 + 34 * noise );
                    loam.pixels[i * 4 + 1] = u8( 66 + 27 * noise );
                    loam.pixels[i * 4 + 2] = u8( 49 + 23 * noise );
                    loam.pixels[i * 4 + 3] = 255;
                }
                auto earth = material( assets, { .78f, .76f, .70f, 1 }, .99f );
                earth->setTexture( upload( assets, loam ), 0 );
                mesh( assets, "Terrain city island planting soil", mulch, earth );
                mesh( assets, "Terrain city island clipped shrubs", shrubs,
                      material( assets, { .27f, .38f, .20f, 1 }, .93f ) );
                const ColourF palette[] = { { .41f, .48f, .28f, 1 }, { .35f, .43f, .32f, 1 },
                                            { .57f, .52f, .33f, 1 } };
                for( u32 tint = 0; tint < grasses.size(); ++tint )
                {
                    if( grasses[tint].indices.empty() ) continue;
                    auto grass = material( assets, palette[tint], .93f );
                    grass->setCullMode( 0 );
                    mesh( assets, "Terrain city island ornamental grass " + StringUtil::toString( tint ),
                          grasses[tint], grass );
                }
                WP_LOG( String( "Terrain city roundabout landscaping: islands=" ) + StringUtil::toString( islands ) +
                        " beds=" + StringUtil::toString( beds ) + " grassTufts=" + StringUtil::toString( tufts ) +
                        " shrubGroups=" + StringUtil::toString( shrubGroups ) );
            }
        }
