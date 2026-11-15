#pragma once

#include "CityReserveLayout.hpp"

        namespace cityReserveLandscape
        {
            enum class Priority : u8 { InnerApproach, MidApproach, RoundaboutEdge, Crossing, Street };

            struct Candidate
            {
                Vector3F edge, along, outward;
                u64 key;
                Priority priority;
            };

            std::array<Vector3F, 20> outline( const Site &site )
            {
                std::array<Vector3F, 20> shape;
                const float phase = cityVerge::unit( site.key ) * 2.f * pi;
                for( size_t i = 0; i < shape.size(); ++i )
                {
                    const float angle = 2.f * pi * ( float( i ) / shape.size() );
                    const float extent = .95f + .035f * std::sin( angle * 3.f + phase ) +
                                         .015f * std::cos( angle * 4.f - phase );
                    shape[i] = site.centre + ( site.along * ( std::cos( angle ) * site.halfLength ) +
                        site.outward * ( std::sin( angle ) * site.halfDepth ) ) * extent;
                }
                return shape;
            }

            bool meets( const Site &site, const std::array<Vector3F, 20> &shape, const cityRoadEdges::Bounds &bounds )
            {
                const Vector3F centre( ( bounds.x0 + bounds.x1 ) * .5f, 0, ( bounds.z0 + bounds.z1 ) * .5f );
                const auto d = centre - site.centre;
                const float x = ( bounds.x1 - bounds.x0 ) * .5f, z = ( bounds.z1 - bounds.z0 ) * .5f;
                const auto extent = [&]( Vector3F direction ) {
                    return std::abs( direction.x ) * x + std::abs( direction.z ) * z;
                };
                if( std::abs( d.dotProduct( site.along ) ) > site.halfLength + extent( site.along ) ||
                    std::abs( d.dotProduct( site.outward ) ) > site.halfDepth + extent( site.outward ) ||
                    !cityRoadEdges::Bounds( shape ).intersects( bounds ) ) return false;
                const std::array parcel{ Vector3F( bounds.x0, 0, bounds.z0 ), Vector3F( bounds.x1, 0, bounds.z0 ),
                                         Vector3F( bounds.x1, 0, bounds.z1 ), Vector3F( bounds.x0, 0, bounds.z1 ) };
                for( size_t i = 0; i < shape.size(); ++i )
                {
                    const auto a = shape[i], edge = shape[( i + 1 ) % shape.size()] - a;
                    const auto inside = cityRoadEdges::cross( edge, site.centre - a ) >= 0 ? 1.f : -1.f;
                    if( std::all_of( parcel.begin(), parcel.end(), [&]( Vector3F p ) {
                            return inside * cityRoadEdges::cross( edge, p - a ) < -.0001f;
                        } ) ) return false;
                }
                return true;
            }

            SmartPtr<render::IMaterial> meadowBlend( SceneAssets &assets, u32 seed )
            {
                TextureBuffer texture( 256, 128 );
                for( u32 y = 0; y < texture.height; ++y )
                {
                    for( u32 x = 0; x < texture.width; ++x )
                    {
                        const float u = ( x + .5f ) / texture.width;
                        const float v = ( y + .5f ) / texture.height;
                        const float radial = std::hypot( u * 2.f - 1.f, v * 2.f - 1.f );
                        const float field = cityVerge::noise( seed, u * 7.f, v * 6.f, 101 );
                        const float tussock = cityVerge::noise( seed + 31, u * 32.f + 9.f, v * 26.f, 101 );
                        const float damp = cityVerge::smooth( ( field - .26f ) / .46f );
                        const float grain = cityVerge::unit( u64( y ) * texture.width + x + ( u64( seed ) << 32 ) );
                        const float edge = cityVerge::smooth( ( std::min( .85f, .73f + .18f * field + .04f * tussock ) - radial ) / .38f );
                        const float textureMix = .59f + .26f * tussock + .15f * grain;
                        const float light = 14.f * ( grain - .5f ) + 18.f * ( tussock - .5f );
                        auto *rgba = texture.pixel( x, y );
                        rgba[0] = u8( 123.f - 13.f * damp + light );
                        rgba[1] = u8( 125.f + 13.f * damp + light );
                        rgba[2] = u8( 78.f + 1.f * damp + light );
                        rgba[3] = u8( 152.f * edge * textureMix );
                    }
                }
                auto result = material( assets, ColourF::White, .96f );
                result->setTexture( upload( assets, texture ), 0 );
                result->setTransparent( true );
                result->setBlendMode( 1 );
                result->setDepthWrite( false );
                return result;
            }

            SourceBudgetVector<Site> build( SceneAssets &assets, const OpenCityLayout &city,
                const CityRoadTerrainMesh &fitted, SmartPtr<IMesh> walkMesh, const cityRoadEdges::Surface &road,
                const SourceBudgetVector<cityGroundContact::Entry> &buildings,
                const SourceBudgetVector<cityParkLandscape::Layout> &parks, u32 seed, VehicleAppearanceQuality quality )
            {
                SourceBudgetVector<Site> sites;
                const auto detail = size_t( std::clamp( assets.cityStreetDetail, 0, 100 ) );
                if( !walkMesh || !detail ) return sites;
                const auto ground = assets.roadAuthoring->getTerrainSnapshot();
                if( !ground || ground->revision != fitted.terrainRevision ) return sites;
                const auto frame = assets.roadAuthoring->getTerrainPlacement();
                const cityParkLandscape::TerrainProjection project( *ground, frame );
                const cityGroundContact::WalkEdges edges( fitted );
                const cityRoadEdges::Surface footways( walkMesh );

                SourceBudgetVector<cityRoadEdges::Bounds> occupied;
                for( const auto &lot : city.lots )
                {
                    const Vector3F p( lot.parcelX, 0, lot.parcelZ ), h( lot.parcelWidth * .5f + .35f, 0,
                                                                     lot.parcelDepth * .5f + .35f );
                    occupied.emplace_back( std::array{ p - h, p + h } );
                }
                const auto reservePassage = [&]( Vector3F a, Vector3F b, float width ) {
                    const Vector3F h( width, 0, width );
                    occupied.emplace_back( std::array{ a - h, a + h, b - h, b + h } );
                };
                for( const auto &entry : buildings )
                {
                    const float reach = edges.reach( entry.door, entry.outward );
                    if( reach >= .15f ) reservePassage( entry.door, entry.door + entry.outward * reach,
                                                       std::max( .85f, entry.halfWidth + .22f ) );
                }
                for( const auto &park : parks )
                    if( park.hasEntry && !park.paths.empty() )
                        reservePassage( park.paths.back().a, park.paths.back().b, park.paths.back().halfWidth + .33f );

                using Cell = std::array<s32, 4>;
                SourceBudgetMap<Cell, Candidate> opportunities;
                const auto offer = [&]( Vector3F edge, Vector3F along, Vector3F outward, Priority priority ) {
                    if( !footways.contains( edge - outward * .27f ) ||
                        footways.contains( edge + outward * .25f ) || road.contains( edge + outward * .25f ) ) return;
                    const auto azimuth = std::atan2( outward.z, outward.x );
                    const Cell cell{ s32( std::floor( edge.x / 5.f ) ), s32( std::floor( edge.z / 5.f ) ),
                                     s32( std::floor( ( azimuth + pi ) * ( 4.f / pi ) ) ) & 7, s32( priority ) };
                    const u64 coordinates = ( u64( u32( s32( std::floor( edge.x * 3.f ) ) ) ) << 32 ) |
                                            u32( s32( std::floor( edge.z * 3.f ) ) );
                    const u64 key = render::foliageStableHash( coordinates ^ ( u64( seed ) * 0x9e3779b97f4a7c15ull ) );
                    const auto previous = opportunities.find( cell );
                    if( previous == opportunities.end() || key < previous->second.key )
                        opportunities.insert_or_assign( cell, Candidate{ edge, along, outward, key, priority } );
                };

                const auto &roundabouts = assets.roadAuthoring->getSource().roundabouts;
                const auto offerApproach = [&]( Vector3F point, Vector3F direction, float halfWidth, Priority priority ) {
                    const Vector3F across( direction.z, 0, -direction.x );
                    for( const float side : { -1.f, 1.f } )
                    {
                        const auto outward = across * side;
                        const auto reach = edges.reach( point, outward, true );
                        if( reach > halfWidth + .9f && reach < halfWidth + 5.f )
                            offer( point + outward * reach, direction, outward, priority );
                    }
                };
                // Offer both public verges on each actual approach before more distant
                // crossing beds take their spacing. The inner station meets the curve.
                for( const auto &junction : roundabouts )
                {
                    for( const auto &port : junction.ports )
                    {
                        auto direction = port.outward;
                        direction.y = 0;
                        direction.normalise();
                        for( const auto &[fraction, priority] : { std::pair{ .34f, Priority::InnerApproach },
                                                                  std::pair{ .55f, Priority::MidApproach } } )
                            offerApproach( junction.center + direction * ( junction.outerRadius + fraction * port.length ),
                                           direction, port.width * .5f, priority );
                    }
                }

                // Actual pedestrian approaches provide useful civic pockets even when
                // the sidewalk ends were shortened or rounded by terrain fitting.
                for( const auto &crossing : fitted.crossings )
                {
                    for( const float shift : { -6.0f, 5.1f } )
                        offerApproach( crossing.centre + crossing.outward * shift, crossing.outward,
                                       crossing.width * .5f, Priority::Crossing );
                }
                for( size_t i = 0; i + 2 < fitted.indices.size(); i += 3 )
                {
                    const auto &top = fitted.vertices[fitted.indices[i]];
                    const auto a = fitted.vertices[fitted.indices[i + 1]].position;
                    const auto b = fitted.vertices[fitted.indices[i + 2]].position;
                    if( std::abs( top.normal.y ) > .01f || top.position.y - a.y < .11f ||
                        std::abs( top.position.x - a.x ) > .001f || std::abs( top.position.z - a.z ) > .001f ||
                        std::hypot( b.x - a.x, b.z - a.z ) < .14f ) continue;
                    const Vector3F along( -top.normal.z, 0, top.normal.x );
                    const auto mid = ( a + b ) * .5f;
                    const bool radial = std::any_of( roundabouts.begin(), roundabouts.end(), [&]( const auto &junction ) {
                        const auto d = mid - junction.center;
                        const float radius = std::hypot( d.x, d.z );
                        return radius > junction.outerRadius - .4f && radius < junction.outerRadius + 8.5f &&
                               d.dotProduct( top.normal ) > radius * .28f;
                    } );
                    offer( mid, along, top.normal, radial ? Priority::RoundaboutEdge : Priority::Street );
                }

                SourceBudgetVector<Candidate> ordered;
                ordered.reserve( opportunities.size() );
                for( const auto &[cell, candidate] : opportunities ) ordered.push_back( candidate );
                std::sort( ordered.begin(), ordered.end(), []( const Candidate &a, const Candidate &b ) {
                    return a.priority != b.priority ? a.priority < b.priority : a.key < b.key;
                } );
                const bool preview = quality == VehicleAppearanceQuality::Preview;
                const size_t limit = ( ( preview ? 42 : 86 ) * detail + 99 ) / 100;
                const size_t cornerLimit = ( ( preview ? 32 : 66 ) * detail + 99 ) / 100;
                size_t corners = 0;
                Geometry meadow;
                for( const auto &candidate : ordered )
                {
                    if( sites.size() >= limit ) break;
                    const bool corner = candidate.priority != Priority::Street;
                    if( corner && corners >= cornerLimit ) continue;
                    for( const auto &[lengthScale, depthScale] : { std::pair{ 1.f, 1.f }, std::pair{ .76f, .76f },
                            std::pair{ .62f, .62f }, std::pair{ .75f, .54f }, std::pair{ .60f, .48f } } )
                    {
                        Site site;
                        site.along = candidate.along;
                        site.outward = candidate.outward;
                        site.halfLength = lengthScale * ( 2.9f + 1.3f * cityVerge::unit( candidate.key + 5 ) );
                        site.halfDepth = depthScale * ( 1.45f + .70f * cityVerge::unit( candidate.key + 7 ) );
                        site.centre = candidate.edge + site.outward *
                            ( site.halfDepth + std::max( .33f, .44f * depthScale ) +
                              .43f * depthScale * cityVerge::unit( candidate.key + 11 ) );
                        site.key = candidate.key;
                        site.corner = corner;
                        const auto shape = outline( site );
                        if( std::any_of( occupied.begin(), occupied.end(), [&]( const auto &b ) { return meets( site, shape, b ); } ) ||
                            std::any_of( sites.begin(), sites.end(), [&]( const auto &other ) {
                                const float spacing = 2.3f + .5f * ( site.halfLength + other.halfLength );
                                return std::hypot( site.centre.x - other.centre.x, site.centre.z - other.centre.z ) < spacing;
                            } ) || !render::sampleTerrainHeight( *ground, frame, site.centre, site.centre.y ) ) continue;

                        bool clear = true;
                        for( size_t i = 0; i < shape.size() && clear; ++i )
                        {
                            float y = 0;
                            const auto p = shape[i];
                            const std::array wedge{ site.centre, p, shape[( i + 1 ) % shape.size()] };
                            clear = !road.overlaps( wedge ) && !footways.overlaps( wedge ) &&
                                render::sampleTerrainHeight( *ground, frame, p, y ) &&
                                std::abs( y - site.centre.y ) < .13f + .26f * std::hypot( p.x - site.centre.x, p.z - site.centre.z );
                        }
                        if( !clear ) continue;
                        Geometry bed;
                        if( !project.emit( bed, shape, []( Vector3F ) { return .025f; } ) ) continue;
                        for( size_t i = 0; i < bed.positions.size(); ++i )
                        {
                            const auto d = bed.positions[i] - site.centre;
                            const float side = ( site.key & 1u ) ? 1.f : -1.f;
                            bed.uv[i] = { .5f + side * d.dotProduct( site.along ) / ( 2.09f * site.halfLength ),
                                          .5f + d.dotProduct( site.outward ) / ( 2.09f * site.halfDepth ), 0 };
                        }
                        cityParkLandscape::append( meadow, bed );
                        sites.push_back( site );
                        if( site.corner ) ++corners;
                        WP_LOG( String( "Terrain city public meadow site=" ) + StringUtil::toString( site.key ) +
                                ", x=" + StringUtil::toString( site.centre.x ) + ", z=" +
                                StringUtil::toString( site.centre.z ) + ", crossing=" +
                                StringUtil::toString( site.corner ? 1 : 0 ) + ", approach=" +
                                StringUtil::toString( candidate.priority <= Priority::MidApproach ? 1 : 0 ) );
                        break;
                    }
                }
                if( sites.empty() ) return sites;
                if( auto actor = mesh( assets, "Terrain city public meadow beds", meadow, meadowBlend( assets, seed ) ) )
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                WP_LOG( String( "Terrain city public meadow beds=" ) + StringUtil::toString( sites.size() ) +
                        ", crossing pockets=" + StringUtil::toString( corners ) +
                        ", terrain faces=" + StringUtil::toString( meadow.indices.size() / 3 ) );
                return sites;
            }
        }
