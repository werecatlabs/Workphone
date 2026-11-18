#pragma once

        namespace cityCrossingBollards
        {
            bool overlaps( const CityStreetFixture &a, const CityStreetFixture &b )
            {
                const auto reach = []( const CityStreetFixture &shape, Vector3F axis ) {
                    return std::abs( axis.dotProduct( shape.along ) ) * shape.halfLength +
                           std::abs( axis.dotProduct( shape.outward ) ) * shape.halfDepth;
                };
                const auto offset = b.position - a.position;
                for( const auto axis : { a.along, a.outward, b.along, b.outward } )
                    if( std::abs( offset.dotProduct( axis ) ) >= reach( a, axis ) + reach( b, axis ) + .12f )
                        return false;
                return true;
            }

            bool nearLamp( const CityStreetFixture &area, Vector3F lamp )
            {
                const auto offset = lamp - area.position;
                const float x = std::max( 0.f, std::abs( offset.dotProduct( area.along ) ) - area.halfLength );
                const float z = std::max( 0.f, std::abs( offset.dotProduct( area.outward ) ) - area.halfDepth );
                return x * x + z * z < .38f * .38f;
            }

            bool covers( const CityStreetFixture &area, float maximumRise )
            {
                return std::abs( area.area - 4.0 * area.halfLength * area.halfDepth ) < .00015 &&
                       area.highest - area.lowest <= maximumRise;
            }

            constexpr u32 bollardSides = 24;

            Vector3F radialDirection( u32 side )
            {
                const float angle = 2.f * pi * ( side % bollardSides ) / bollardSides;
                return { std::cos( angle ), 0, std::sin( angle ) };
            }

            struct BollardRing
            {
                float height, radius, normalY, v;
            };

            void appendBollardProfile( Geometry &geometry, Vector3F root,
                                       std::initializer_list<BollardRing> profile )
            {
                constexpr u32 pitch = bollardSides + 1;
                auto row = u32( geometry.positions.size() );
                for( const auto &ring : profile )
                {
                    const float radialNormal = std::sqrt( 1.f - ring.normalY * ring.normalY );
                    for( u32 side = 0; side <= bollardSides; ++side )
                    {
                        const auto radial = radialDirection( side );
                        geometry.positions.push_back( root + radial * ring.radius + Vector3F( 0, ring.height, 0 ) );
                        geometry.normals.push_back( radial * radialNormal + Vector3F( 0, ring.normalY, 0 ) );
                        geometry.tangents.push_back( { -radial.z, 0, radial.x, -1 } );
                        geometry.uv.push_back( { float( side ) / bollardSides, ring.v, 0 } );
                    }
                    if( &ring != profile.begin() )
                        for( u32 side = 0; side < bollardSides; ++side )
                        {
                            const auto top = row + side, bottom = top - pitch;
                            for( const u32 index : { bottom, top, top + 1, bottom, top + 1, bottom + 1 } )
                                geometry.indices.push_back( index );
                        }
                    row += pitch;
                }
            }

            void bollard( Geometry &body, Geometry &reflector, Vector3F root, float height )
            {
                constexpr float shaft = .073f, sleeve = .074f, crown = .035f;
                const float lower = height - .201f, upper = height - .072f;
                appendBollardProfile( body, root, {
                    { -.012f, .092f, 0, 0 }, { .027f, .092f, 0, 0 },
                    { .036f, .090f, .62f, 0 }, { .049f, .077f, .55f, 0 },
                    { .060f, shaft, 0, 0 }, { lower, shaft, 0, 0 }
                } );
                appendBollardProfile( reflector, root, {
                    { lower, shaft, -.45f, 0 }, { lower + .004f, sleeve, 0, .03f },
                    { upper - .004f, sleeve, 0, .97f }, { upper, shaft, .45f, 1 }
                } );
                appendBollardProfile( body, root, {
                    { upper, shaft, 0, 0 }, { height - .044f, shaft, 0, 0 },
                    { height - .031f, .070f, .46f, 0 }, { height - .012f, .055f, .66f, 0 },
                    { height, crown, 1, 0 }
                } );
                const auto top = root + Vector3F( 0, height, 0 );
                for( u32 side = 0; side < bollardSides; side += 2 )
                    body.quad( top, top + radialDirection( side ) * crown,
                               top + radialDirection( side + 1 ) * crown,
                               top + radialDirection( side + 2 ) * crown, { 0, 1, 0 } );
            }

            SmartPtr<render::IMaterial> reflectorMaterial( SceneAssets &assets )
            {
                constexpr u32 width = 384, height = 96;
                TextureBuffer colour( width, height ), normal( width, height );
                const auto prism = []( float u, float v ) {
                    const float columns = u * 72.f, rows = v * 20.f;
                    const auto ridge = []( float x ) { return 1.f - 2.f * std::abs( x - std::floor( x ) - .5f ); };
                    return std::min( { ridge( columns ), ridge( rows + columns * .5f ),
                                       ridge( rows - columns * .5f ) } );
                };
                const auto byte = []( float value ) { return u8( std::clamp( value, 0.f, 255.f ) ); };
                for( u32 y = 0; y < height; ++y )
                {
                    for( u32 x = 0; x < width; ++x )
                    {
                        const float u = ( x + .5f ) / width, v = ( y + .5f ) / height;
                        const float seal = std::clamp( std::min( v, 1.f - v ) * 32.f, 0.f, 1.f );
                        const float foil = 213.f + 15.f * seal + 13.f * prism( u, v );
                        auto *albedo = colour.pixel( x, y );
                        albedo[0] = byte( foil ); albedo[1] = byte( foil - 6.f );
                        albedo[2] = byte( foil - 21.f ); albedo[3] = 255;
                        const float du = ( prism( u + 1.f / width, v ) - prism( u - 1.f / width, v ) ) * .42f;
                        const float dv = ( prism( u, v + 1.f / height ) - prism( u, v - 1.f / height ) ) * .42f;
                        const auto n = Vector3F( -du, -dv, 1 ).normaliseCopy();
                        auto *pixel = normal.pixel( x, y );
                        pixel[0] = byte( 127.5f * ( 1.f + n.x ) ); pixel[1] = byte( 127.5f * ( 1.f + n.y ) );
                        pixel[2] = byte( 127.5f * ( 1.f + n.z ) ); pixel[3] = 255;
                    }
                }
                auto materialInstance = material( assets, ColourF::White, .39f );
                materialInstance->setTexture( upload( assets, colour ), 0 );
                materialInstance->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
                materialInstance->setUVProjection( 0 );
                materialInstance->setNormalStrength( .28f );
                materialInstance->setEditorUInt( "uvFilter", 3 );
                materialInstance->setEditorFloat( "uvAniso", 8.f );
                return materialInstance;
            }

            void build( SceneAssets &assets, const CityRoadTerrainMesh &footways,
                        const SourceBudgetVector<CityStreetFixture> &furniture,
                        const SourceBudgetVector<Vector3F> &lamps, VehicleAppearanceQuality quality )
            {
                const int detail = std::clamp( assets.cityStreetDetail, 0, 100 );
                if( !detail || footways.crossings.empty() ) return;
                struct Landing
                {
                    std::array<CityStreetFixture, 2> post;
                    CityStreetFixture passage;
                    size_t crossing;
                    u32 side;
                };
                SourceBudgetVector<Landing> candidates;
                const auto reserveClear = [&]( const CityStreetFixture &area ) {
                    return std::none_of( furniture.begin(), furniture.end(), [&]( const auto &item ) {
                        return overlaps( area, item );
                    } ) && std::none_of( lamps.begin(), lamps.end(), [&]( Vector3F lamp ) {
                        return nearLamp( area, lamp );
                    } );
                };
                constexpr float baseRadius = .094f;
                constexpr float clearWidth = 1.90f;
                for( size_t crossing = 0; crossing < footways.crossings.size(); ++crossing )
                {
                    const auto &shape = footways.crossings[crossing];
                    for( u32 side = 0; side < 2; ++side )
                    {
                        const auto outward = shape.across * ( side == 0 ? -1.f : 1.f );
                        for( const float inset : { .269f, .335f } )
                            for( const float gap : { .63f, 1.01f } )
                            {
                                const auto centre = shape.centre + outward * ( shape.width * .5f + inset );
                                const float flank = shape.depth * .5f + gap;
                                const auto post = [&]( Vector3F position ) {
                                    return CityStreetFixture{ position, shape.outward, outward, CityStreetFixture::Bin,
                                                              baseRadius, baseRadius };
                                };
                                const CityStreetFixture passage = {
                                    centre + outward * ( baseRadius + .035f + clearWidth * .5f ), shape.outward,
                                    outward, CityStreetFixture::Bin, flank + .25f, clearWidth * .5f };
                                const auto a = post( centre - shape.outward * flank );
                                const auto b = post( centre + shape.outward * flank );
                                if( reserveClear( a ) && reserveClear( b ) && reserveClear( passage ) )
                                    candidates.push_back( { { a, b }, passage, crossing, side } );
                            }
                    }
                }
                // Stream the fitted top once. Whole footprint coverage keeps bases off
                // sloped kerb faces and ensures there is a walk behind the pair.
                SourceBudgetMap<cityHillside::Cell, SourceBudgetVector<size_t>> cells;
                for( size_t i = 0; i < candidates.size(); ++i )
                    cells[cityHillside::cellFor( candidates[i].passage.position )].push_back( i );
                constexpr float sweep = 4.6f;
                for( size_t i = 0; !cells.empty() && i + 2 < footways.indices.size(); i += 3 )
                {
                    const auto &a = footways.vertices[footways.indices[i]];
                    const auto &b = footways.vertices[footways.indices[i + 1]];
                    const auto &c = footways.vertices[footways.indices[i + 2]];
                    if( std::min( { a.normal.y, b.normal.y, c.normal.y } ) < .91f ) continue;
                    const Vector3F low( std::min( { a.position.x, b.position.x, c.position.x } ) - sweep, 0,
                                        std::min( { a.position.z, b.position.z, c.position.z } ) - sweep );
                    const Vector3F high( std::max( { a.position.x, b.position.x, c.position.x } ) + sweep, 0,
                                         std::max( { a.position.z, b.position.z, c.position.z } ) + sweep );
                    cityHillside::visitCells( low, high, [&]( auto cell ) {
                        const auto found = cells.find( cell );
                        if( found == cells.end() ) return true;
                        for( const auto choice : found->second )
                        {
                            auto &candidate = candidates[choice];
                            const auto &p = candidate.passage.position;
                            if( p.x < low.x || p.x > high.x || p.z < low.z || p.z > high.z ) continue;
                            measureStreetFixtureSupport( candidate.passage, a.position, b.position, c.position );
                            for( auto &post : candidate.post )
                                measureStreetFixtureSupport( post, a.position, b.position, c.position );
                        }
                        return true;
                    } );
                }

                Geometry body, reflector;
                SourceBudgetVector<std::array<bool, 2>> used( footways.crossings.size() );
                SourceBudgetVector<CityStreetFixture> installed, passages;
                const bool preview = quality == VehicleAppearanceQuality::Preview;
                const size_t budget = ( ( preview ? 8u : 16u ) * detail + 99 ) / 100;
                size_t pairs[2] = {};
                for( const auto &candidate : candidates )
                {
                    const auto &crossing = footways.crossings[candidate.crossing];
                    const size_t family = crossing.roundabout ? 1 : 0;
                    if( used[candidate.crossing][candidate.side] || pairs[family] >= budget ||
                        !covers( candidate.passage, .58f ) || !covers( candidate.post[0], .038f ) ||
                        !covers( candidate.post[1], .038f ) ) continue;
                    if( std::any_of( installed.begin(), installed.end(), [&]( const auto &previous ) {
                            return overlaps( candidate.post[0], previous ) || overlaps( candidate.post[1], previous ) ||
                                   overlaps( candidate.passage, previous );
                        } ) || std::any_of( passages.begin(), passages.end(), [&]( const auto &previous ) {
                            return overlaps( candidate.post[0], previous ) || overlaps( candidate.post[1], previous );
                        } ) ) continue;
                    bool offRoad = true;
                    for( const auto &post : candidate.post )
                    {
                        float height = 0;
                        bool drivable = false;
                        if( assets.roadAuthoring->getRuntimeSurface( post.position.x, post.position.z, height, drivable ) &&
                            drivable ) offRoad = false;
                    }
                    if( !offRoad ) continue;
                    for( const auto &post : candidate.post )
                    {
                        auto root = post.position;
                        root.y = post.lowest;
                        bollard( body, reflector, root, crossing.roundabout ? 1.04f : 1.01f );
                        installed.push_back( post );
                    }
                    used[candidate.crossing][candidate.side] = true;
                    passages.push_back( candidate.passage );
                    ++pairs[family];
                }

                if( !body.indices.empty() )
                {
                    mesh( assets, "Terrain city crossing bollards", body,
                          material( assets, { .237f, .257f, .254f, 1 }, .53f, .08f ) );
                    if( auto actor = mesh( assets, "Terrain city crossing bollard reflectors", reflector,
                                           reflectorMaterial( assets ) ) )
                        actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                }
                WP_LOG( String( "Terrain city crossing bollard pairs=" ) + StringUtil::toString( pairs[0] + pairs[1] ) +
                        " ordinary=" + StringUtil::toString( pairs[0] ) +
                        " roundabouts=" + StringUtil::toString( pairs[1] ) );
            }
        }  // namespace cityCrossingBollards
