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

    constexpr float settStripWidth = .58f;
    constexpr u32 settsPerRepeat = 6;
    constexpr float settLength = .34f;

    SmartPtr<render::IMaterial> settMaterial( SceneAssets &assets, u32 seed )
    {
        constexpr u32 width = 512, height = 128;
        TextureBuffer colour( width, height ), normals( width, height );
        const u64 key = ( u64( seed ) << 32 ) | 0x95a25u;
        for( u32 y = 0; y < height; ++y )
        {
            const u32 row = y / ( height / 2 );
            const float v = ( y % ( height / 2 ) + .5f ) / ( height / 2 );
            for( u32 x = 0; x < width; ++x )
            {
                const float station = ( x + .5f ) * settsPerRepeat / width + row * .5f;
                const u32 stone = u32( station ) % settsPerRepeat + row * settsPerRepeat;
                const float u = station - std::floor( station );
                const float across = std::min( u, 1 - u ) * settLength;
                const float down = std::min( v, 1 - v ) * settStripWidth * .5f;
                const u64 texel = u64( y ) * width + x;
                const float grain = variation( key + 31 + texel );
                const float joint = std::min( across, down ) - .0025f - grain * .002f;
                const float edge = std::clamp( joint / .006f, 0.f, 1.f );
                const float face = edge * edge * ( 3.f - 2.f * edge );
                const float warmth = 12.f * ( variation( key + stone + 19 ) - .5f );
                const float granite = 131.f + 31.f * variation( key + stone ) + 19.f * ( grain - .5f );
                const float tone = 95.f + ( granite - 95.f ) * face;
                auto *pixel = colour.pixel( x, y );
                pixel[0] = u8( tone + 4.f + warmth * face );
                pixel[1] = u8( tone + 1.f );
                pixel[2] = u8( tone - 3.f - warmth * face );
                pixel[3] = 255;

                // The eased edge rises from a shallow mortar joint into the flat stone face.
                const float bevel = -.42f * 4.f * edge * ( 1.f - edge );
                const auto normal = Vector3F( across < down ? ( u < .5f ? bevel : -bevel ) : 0.f,
                                              down <= across ? ( v < .5f ? bevel : -bevel ) : 0.f,
                                              1.f ).normaliseCopy();
                auto *bump = normals.pixel( x, y );
                bump[0] = u8( normal.x * 127.5f + 127.5f );
                bump[1] = u8( normal.y * 127.5f + 127.5f );
                bump[2] = u8( normal.z * 127.5f + 127.5f );
                bump[3] = 255;
            }
        }
        auto stone = material( assets, ColourF::White, .9f );
        stone->setTexture( upload( assets, colour ), 0 );
        stone->setTexture( upload( assets, normals, render::TextureMipFilter::Normal ), 1 );
        stone->setUVProjection( 0 );
        stone->setEditorUInt( "uvFilter", 3 );
        stone->setEditorFloat( "uvAniso", 8.f );
        return stone;
    }

    TextureBuffer plantedSoil( u32 seed )
    {
        constexpr u32 size = 128;
        TextureBuffer texture( size, size );
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float u = ( x + .5f ) * 2.f / size - 1.f;
                const float v = ( y + .5f ) * 2.f / size - 1.f;
                const float r = std::hypot( u, v );
                const float coarse = variation( ( u64( seed ) << 32 ) ^ ( ( y / 4 ) * size + x / 4 ) );
                const float fine = variation( ( u64( seed ^ 0x5c117u ) << 32 ) ^ ( y * size + x ) );
                const float grit = 14.f * fine + 12.f * coarse;
                const float edge = std::clamp( ( .99f - r - .05f * coarse ) / .22f, 0.f, 1.f );
                auto *pixel = texture.pixel( x, y );
                pixel[0] = u8( 91.f + grit );
                pixel[1] = u8( 79.f + grit );
                pixel[2] = u8( 57.f + grit );
                pixel[3] = u8( std::lround( 255.f * edge * edge * ( 3.f - 2.f * edge ) ) );
            }
        }
        return texture;
    }

    SmartPtr<render::IMaterial> perennialFlowers( SceneAssets &assets )
    {
        // The left tile is a side view of three salvia spikes. The right tile looks
        // down on the branching, nearly flat flower head of a yarrow stem.
        constexpr u32 tile = 128;
        TextureBuffer atlas( tile * 2, tile );
        const std::array spikes{ Vector2F( -.31f, .81f ), Vector2F( .02f, .97f ),
                                 Vector2F( .32f, .72f ) };
        std::array<Vector2F, 17> umbels;
        for( u32 i = 0; i < umbels.size(); ++i )
        {
            const float r = .335f * std::sqrt( float( i ) / 16.f );
            const float angle = i * 2.39996323f;
            umbels[i] = { r * std::cos( angle ), r * std::sin( angle ) };
        }
        const auto write = []( u8 *pixel, Vector3F colour, bool covered ) {
            pixel[0] = u8( colour.x );
            pixel[1] = u8( colour.y );
            pixel[2] = u8( colour.z );
            pixel[3] = covered ? 255 : 0;
        };
        for( u32 y = 0; y < tile; ++y )
        {
            for( u32 x = 0; x < tile; ++x )
            {
                const float u = ( x + .5f ) / tile - .5f, v = ( y + .5f ) / tile;
                const float grain = variation( u64( y ) * tile + x );
                Vector3F colour = v > .45f ? Vector3F( 144, 112, 164 ) : Vector3F( 92, 116, 70 );
                bool covered = false;
                for( const auto tip : spikes )
                {
                    const float t = v / tip.y;
                    if( t >= 1.f )
                    {
                        continue;
                    }
                    const float side = u - tip.x * ( .07f + .93f * t );
                    const float distance = std::abs( side );
                    if( distance < .009f )
                    {
                        covered = true;
                        colour = { 92, 118, 69 };
                    }
                    if( t < .61f )
                    {
                        const float leaf = distance / .17f;
                        const float halfWidth = std::max( 0.f, .05f * 4.f * leaf * ( 1.f - leaf ) );
                        const float stripe = t - leaf * .17f;
                        if( std::min( std::abs( stripe - .16f ), std::abs( stripe - .36f ) ) < halfWidth )
                        {
                            covered = true;
                            colour = side < 0 ? Vector3F( 88, 113, 65 ) : Vector3F( 105, 129, 77 );
                        }
                    }
                    else
                    {
                        const float bud = ( t - .61f ) / .39f;
                        const float whorl = .5f + .5f * std::cos( 12.f * pi * bud + 5.f * side );
                        if( distance < ( .080f - .060f * bud ) * ( .61f + .39f * whorl ) )
                        {
                            covered = true;
                            const float light = .82f + .15f * whorl + .08f * grain;
                            colour = Vector3F( 165, 133, 186 ) * light;
                        }
                    }
                }
                write( atlas.pixel( x, y ), colour, covered );

                float nearest = 1.f;
                const Vector2F point( u, v - .5f );
                for( const auto &umbel : umbels )
                {
                    nearest = std::min( nearest, ( point - umbel ).lengthSquared() );
                }
                const float cap = std::clamp( 1.f - nearest / .012f, 0.f, 1.f );
                colour = Vector3F( 211, 192, 132 ) * ( .81f + .16f * cap + .09f * grain );
                write( atlas.pixel( x + tile, y ), colour, nearest < .011f + .002f * grain );
            }
        }
        auto flowers = material( assets, ColourF::White, .91f );
        flowers->setTexture( upload( assets, atlas, render::TextureMipFilter::Cutout, 2 ), 0 );
        flowers->setUVProjection( 0 );
        flowers->setCutout( true );
        flowers->setAlphaClip( .43f );
        flowers->setDepthWrite( true );
        flowers->setCullMode( 0 );
        flowers->setEditorUInt( "uvFilter", 3 );
        flowers->setEditorFloat( "uvAniso", 8.f );
        return flowers;
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
                {
                    shape.indices.push_back( index );
                }
                a += sides;
                b += sides;
            }
            for( const auto index : { a, crown, b } )
            {
                shape.indices.push_back( index );
            }
        }
    }

    void build( SceneAssets &assets, u32 seed )
    {
        const int detail = std::clamp( assets.cityStreetDetail, 0, 100 );
        if( !detail )
        {
            return;
        }

        Geometry mulch, shrubs, leafSprays, flowers, mowingStrips;
        std::array<Geometry, 3> grasses;
        u32 islands = 0, beds = 0, tufts = 0, shrubGroups = 0, floralGroups = 0;
        constexpr u32 maxIslands = 16;
        const auto &source = assets.roadAuthoring->getSource();
        for( const auto &island : source.roundabouts )
        {
            // Reserve the outer shoulder for the mowing strip and its planting setback.
            const float radius = island.islandRadius - island.kerbWidth - .8f;
            if( radius < 1.8f )
            {
                continue;
            }
            if( islands >= maxIslands )
            {
                break;
            }
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

            // The island bake adds port endpoints between these regular angles. Chords
            // between the regular samples therefore stay entirely on its grass polygon.
            const u32 segments = island.ringSamples - u32( 2 * island.ports.size() );
            const float outer = island.islandRadius - island.kerbWidth - .012f;
            const float inner = outer - settStripWidth;
            const float repeats = std::max( 1.f, std::round( pi * ( outer + inner ) /
                                                              ( settLength * settsPerRepeat ) ) );
            const float phase = variation( key + 53 );
            for( u32 i = 0; i < segments; ++i )
            {
                const float u = float( i ) / segments, next = float( i + 1 ) / segments;
                const auto a = bearing( 2.f * pi * u );
                const auto b = bearing( i + 1 == segments ? 0.f : 2.f * pi * next );
                mowingStrips.quad( lay( island.center + a * inner, .006f ),
                                   lay( island.center + b * inner, .006f ),
                                   lay( island.center + b * outer, .006f ),
                                   lay( island.center + a * outer, .006f ), normal,
                                   phase + u * repeats, phase + next * repeats );
            }

            const u32 bedCount = radius > 3.5f ? 2 : 1;
            for( u32 bed = 0; bed < bedCount; ++bed )
            {
                const auto bedKey = render::foliageStableHash( key + bed );
                const float side = bed ? 1.f : -1.f;
                const auto centre = island.center +
                                    across * ( bedCount == 2 ? side * radius * .29f : 0 ) +
                                    along * ( bedCount == 2 ? side * radius * .10f : 0 );
                const float length = radius * ( bed ? .47f : .60f );
                const float width = radius * ( bed ? .23f : .29f );
                const auto bedPoint = [&]( float angle, float span ) {
                    const float ripple =
                        1 + .10f * std::cos( 3 * angle + side ) + .04f * std::sin( 5 * angle );
                    return centre + along * ( span * length * std::cos( angle ) * ripple ) +
                           across * ( span * width * std::sin( angle ) * ripple );
                };
                // One triangulated surface on the same grade as the island's own mesh.
                constexpr u32 sectors = 20;
                const auto first = u32( mulch.positions.size() );
                const auto bedVertex = [&]( Vector3F p, Vector2F uv ) {
                    mulch.positions.push_back( lay( p, .012f ) );
                    mulch.normals.push_back( normal );
                    mulch.tangents.push_back(
                        { along.x, -normal.dotProduct( along ) / normal.y, along.z, 1 } );
                    mulch.uv.push_back( { uv.x, uv.y, 0 } );
                };
                bedVertex( centre, { .5f, .5f } );
                for( u32 i = 0; i < sectors; ++i )
                {
                    const float angle = i * 2.f * pi / sectors;
                    const auto map = bearing( angle + 2.f * pi * variation( bedKey ) );
                    bedVertex( bedPoint( angle, 1 ), { .5f + map.x * .5f, .5f + map.z * .5f } );
                }
                for( u32 i = 0; i < sectors; ++i )
                {
                    for( const auto index : { first, first + 1 + ( i + 1 ) % sectors, first + 1 + i } )
                    {
                        mulch.indices.push_back( index );
                    }
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
                            grasses[tint],
                            std::array{ root,
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
                    const float size =
                        std::min( width * .48f, .49f ) * ( .82f + .18f * variation( shrubKey ) );
                    const float phase = variation( shrubKey + 1 ) * 2 * pi;
                    const auto axis = bearing( phase );
                    const float height = .44f + .09f * variation( shrubKey + 2 );
                    // A pair of low, off-centre cores supplies shade between the leafy shoots.
                    for( u32 lobe = 0; lobe < 2; ++lobe )
                    {
                        const auto base = lay( position + axis * ( size * ( lobe ? -.31f : .20f ) ), 0 );
                        shrubCrown( shrubs, base, size * ( lobe ? .53f : .64f ),
                                    height * ( lobe ? .49f : .60f ), shrubKey + lobe * 37 );
                    }
                    const u32 shoots = detail < 50 ? 11 : 22;
                    for( u32 i = 0; i < shoots; ++i )
                    {
                        const u64 shoot = render::foliageStableHash( shrubKey + i + 61 );
                        const auto direction = bearing( phase + i * 2.39996323f +
                                                        .35f * variation( shoot ) );
                        const float spread = .30f + .70f * variation( shoot + 1 );
                        const auto root = lay( position + direction * ( size * spread * .27f ),
                                               height * ( .21f + .09f * variation( shoot + 2 ) ) );
                        const auto tip = lay( position + direction * ( size * spread * 1.10f ),
                                              height * ( 1.17f - .46f * spread +
                                                         .13f * variation( shoot + 3 ) ) );
                        const auto half = Vector3F( -direction.z, 0, direction.x ) *
                                          ( size * ( .27f + .10f * variation( shoot + 4 ) ) );
                        if( clear( root - half ) && clear( root + half ) &&
                            clear( tip + half ) && clear( tip - half ) )
                        {
                            const auto face = half.crossProduct( tip - root ).normaliseCopy();
                            leafSprays.quad( root - half, root + half, tip + half, tip - half, face );
                        }
                    }
                    ++shrubGroups;
                }
                const auto onSoil = [&]( Vector3F p ) {
                    const auto d = p - centre;
                    const float x = d.dotProduct( along ) / length, z = d.dotProduct( across ) / width;
                    // All lobes of the mulched bed enclose this ellipse.
                    return x * x + z * z < .82f * .82f && clear( p );
                };
                const u32 drifts = bed ? 2 : 3;
                const float phase = variation( bedKey + 182 ) * 2.f * pi;
                const float spread = std::min( width * .26f, .35f );
                for( u32 drift = 0; drift < drifts; ++drift )
                {
                    const bool salvia = ( drift + bed ) % 2 == 0;
                    const auto patch = centre +
                        along * ( length * ( ( drift + .5f ) / drifts - .5f ) * 1.53f ) +
                        across * ( width * ( drift % 2 ? -.38f : .38f ) );
                    const u32 clumps = 2 + u32( detail ) * 5 / 100;
                    for( u32 clump = 0; clump < clumps; ++clump )
                    {
                        const u64 clumpKey = render::foliageStableHash( bedKey + 181 + 11 * drift + clump );
                        const float angle = phase + clump * 2.39996323f;
                        const float span = std::sqrt( ( clump + .2f ) / clumps );
                        const auto root = patch + along * ( std::min( length * .26f, .93f ) * span *
                                                            std::cos( angle ) ) +
                                          across * ( spread * span * std::sin( angle ) );
                        const u32 shoots = detail >= 50 ? 8 : 4;
                        bool planted = false;
                        for( u32 shoot = 0; shoot < shoots; ++shoot )
                        {
                            const u64 k = render::foliageStableHash( clumpKey + shoot );
                            const auto out = bearing( angle + shoot * 2.39996323f + .4f * variation( k ) );
                            const Vector3F side( -out.z, 0, out.x );
                            const float reach = spread * ( .41f + .54f * variation( k + 1 ) );
                            const auto foot = lay( root + out * ( .26f * reach ), .012f );
                            const auto tip = lay( root + out * reach, ( salvia ? .35f : .26f ) +
                                                                     .13f * variation( k + 2 ) );
                            const auto half = side * std::min( spread * .42f, salvia ? .13f : .073f );
                            if( !onSoil( foot - half ) || !onSoil( foot + half ) ||
                                !onSoil( tip - half ) || !onSoil( tip + half ) )
                            {
                                continue;
                            }
                            const auto face = half.crossProduct( tip - foot ).normaliseCopy();
                            if( salvia )
                            {
                                flowers.quad( foot - half, foot + half, tip + half, tip - half,
                                              face, 0, .5f );
                            }
                            else
                            {
                                // A leafy stem carries one shallow, outward-tilted corymb.
                                const float radius = .057f + .026f * variation( k + 3 );
                                const auto wide = side * radius;
                                const auto tilt = ( out - Vector3F( 0, .40f, 0 ) ) * radius;
                                if( !onSoil( tip - wide - tilt ) || !onSoil( tip + wide - tilt ) ||
                                    !onSoil( tip + wide + tilt ) || !onSoil( tip - wide + tilt ) )
                                {
                                    continue;
                                }
                                leafSprays.quad( foot - half, foot + half, tip + half, tip - half, face );
                                flowers.quad( tip - wide - tilt, tip + wide - tilt, tip + wide + tilt,
                                              tip - wide + tilt, wide.crossProduct( tilt ).normaliseCopy(),
                                              .5f, 1.f );
                            }
                            planted = true;
                        }
                        floralGroups += planted;
                    }
                }
            }
        }
        if( !islands )
        {
            return;
        }

        if( auto actor = mesh( assets, "Terrain city island granite mowing strips", mowingStrips,
                               settMaterial( assets, seed ) ) )
        {
            actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                scene::Renderer::CastShadows::Off );
        }
        auto earth = material( assets, ColourF::White, .99f );
        earth->setTexture( upload( assets, plantedSoil( seed ) ), 0 );
        earth->setTransparent( true );
        earth->setBlendMode( 1 );
        earth->setDepthWrite( false );
        earth->setUVProjection( 0 );
        earth->setEditorUInt( "uvFilter", 3 );
        earth->setEditorFloat( "uvAniso", 8.f );
        if( auto actor = mesh( assets, "Terrain city island planting soil", mulch, earth ) )
        {
            actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                scene::Renderer::CastShadows::Off );
        }
        mesh( assets, "Terrain city island clipped shrubs", shrubs,
              material( assets, { .20f, .29f, .16f, 1 }, .93f ) );
        if( !leafSprays.indices.empty() )
        {
            auto leaves = cityLeafSprig::surface( assets );
            leaves->setCullMode( 0 );
            mesh( assets, "Terrain city island leaf sprays", leafSprays, leaves );
        }
        const ColourF palette[] = { { .41f, .48f, .28f, 1 },
                                    { .35f, .43f, .32f, 1 },
                                    { .57f, .52f, .33f, 1 } };
        for( u32 tint = 0; tint < grasses.size(); ++tint )
        {
            if( grasses[tint].indices.empty() )
            {
                continue;
            }
            auto grass = material( assets, palette[tint], .93f );
            grass->setCullMode( 0 );
            mesh( assets, "Terrain city island ornamental grass " + StringUtil::toString( tint ),
                  grasses[tint], grass );
        }
        if( !flowers.indices.empty() )
        {
            mesh( assets, "Terrain city island perennial flowers", flowers, perennialFlowers( assets ) );
        }
        WP_LOG( String( "Terrain city roundabout landscaping: islands=" ) +
                StringUtil::toString( islands ) + " beds=" + StringUtil::toString( beds ) +
                " grassTufts=" + StringUtil::toString( tufts ) +
                " shrubGroups=" + StringUtil::toString( shrubGroups ) +
                " flowerClusters=" + StringUtil::toString( floralGroups ) );
    }
}  // namespace cityRoundaboutLandscape
