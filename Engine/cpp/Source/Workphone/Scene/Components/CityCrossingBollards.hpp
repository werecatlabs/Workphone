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

            void bollard( Geometry &body, Geometry &reflector, Vector3F root, float height )
            {
                constexpr u32 sides = 12;
                constexpr float radius = .074f;
                const auto point = [&]( u32 i, float r, float y ) {
                    const float angle = 2.f * pi * i / sides;
                    return root + Vector3F( std::cos( angle ) * r, y, std::sin( angle ) * r );
                };
                const auto strip = [&]( Geometry &geometry, u32 i, float bottom, float top, float r0, float r1 ) {
                    const float angle = 2.f * pi * ( i + .5f ) / sides;
                    const auto normal = Vector3F( std::cos( angle ) * ( top - bottom ), r0 - r1,
                                                  std::sin( angle ) * ( top - bottom ) ).normaliseCopy();
                    geometry.quad( point( i, r0, bottom ), point( i + 1, r0, bottom ),
                                   point( i + 1, r1, top ), point( i, r1, top ), normal );
                };
                for( u32 i = 0; i < sides; ++i )
                {
                    strip( body, i, -.012f, .039f, .092f, .092f );
                    strip( body, i, .039f, .066f, .092f, radius );
                    strip( body, i, .066f, height - .202f, radius, radius );
                    strip( reflector, i, height - .202f, height - .071f, radius, radius );
                    strip( body, i, height - .071f, height - .036f, radius, radius );
                    strip( body, i, height - .036f, height, radius, .035f );
                    if( i % 2 == 0 )
                        body.quad( root + Vector3F( 0, height, 0 ), point( i, .035f, height ),
                                   point( i + 1, .035f, height ), point( i + 2, .035f, height ), { 0, 1, 0 } );
                }
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
                          material( assets, { .237f, .257f, .254f, 1 }, .66f, .32f ) );
                    auto collar = material( assets, { .90f, .83f, .64f, 1 }, .46f );
                    collar->setEmissive( { .022f, .018f, .011f, 1 } );
                    if( auto actor = mesh( assets, "Terrain city crossing bollard reflectors", reflector, collar ) )
                        actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                }
                WP_LOG( String( "Terrain city crossing bollard pairs=" ) + StringUtil::toString( pairs[0] + pairs[1] ) +
                        " ordinary=" + StringUtil::toString( pairs[0] ) +
                        " roundabouts=" + StringUtil::toString( pairs[1] ) );
            }
        }  // namespace cityCrossingBollards
