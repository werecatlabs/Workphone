#pragma once

#include "CityTerrainProjection.hpp"

        namespace cityEntranceLandscape
        {
            struct Bed
            {
                Vector3F centre, along, away;
                float halfLength = 0.f, halfWidth = 0.f;
                u64 lot = 0, key = 0;
                size_t perennials = 0, sedges = 0, heads = 0;
                bool companion = false;
            };

            struct Batches
            {
                Geometry earth, leaf, grass, seedHeads;
                SourceBudgetVector<Bed> beds;
            };

            using Outline = std::array<Vector3F, 12>;

            Outline outline( const Bed &bed, float margin = 0.f )
            {
                Outline shape;
                for( size_t i = 0; i < shape.size(); ++i )
                {
                    const float angle = i * ( 2.f * pi / shape.size() );
                    const float bias = 1.f + .051f * std::sin( angle + .47f ) + .027f * std::cos( angle * 3.f + .82f );
                    shape[i] = bed.centre + ( bed.along * ( std::cos( angle ) * ( bed.halfLength + margin ) ) +
                                              bed.away * ( std::sin( angle ) * ( bed.halfWidth + margin ) ) ) * bias;
                }
                return shape;
            }

            template <class A, class B>
            bool separated( const A &a, const B &b, Vector3F inside )
            {
                for( size_t i = 0; i < a.size(); ++i )
                {
                    const auto p = a[i], edge = a[( i + 1 ) % a.size()] - p;
                    const float orientation = cityVerge::crossXZ( edge, inside - p ) >= 0.f ? 1.f : -1.f;
                    if( std::all_of( b.begin(), b.end(), [&]( Vector3F v ) {
                            return orientation * cityVerge::crossXZ( edge, v - p ) < -.0001f;
                        } ) ) return true;
                }
                return false;
            }

            bool drape( Geometry &out, const Bed &bed, const Outline &shape, const render::TerrainData &terrain,
                        const Transform3<real_Num> &frame )
            {
                const cityParkLandscape::TerrainProjection projection( terrain, frame );
                if( !projection.emit( out, shape, []( Vector3F ) { return .016f; } ) ) return false;
                for( size_t i = 0; i < out.positions.size(); ++i )
                {
                    const auto normal = out.normals[i];
                    if( normal.y < .85f ) return false;
                    const auto tangent = ( bed.along - normal * bed.along.dotProduct( normal ) ).normaliseCopy();
                    const auto delta = out.positions[i] - bed.centre;
                    out.tangents[i] = { tangent.x, tangent.y, tangent.z, 1 };
                    out.uv[i] = { .5f + delta.dotProduct( bed.along ) / ( 2.22f * bed.halfLength ),
                                  .5f + delta.dotProduct( bed.away ) / ( 2.22f * bed.halfWidth ), 0 };
                }
                return true;
            }

            void plant( Batches &batches, Bed &bed, const render::TerrainData &terrain,
                        const Transform3<real_Num> &frame )
            {
                using cityVerge::unit;
                const Vector3F up( 0, 1, 0 );
                const bool compactBed = bed.companion;
                const auto within = [&]( Vector3F p ) {
                    const auto q = p - bed.centre;
                    const float u = q.dotProduct( bed.along ) / bed.halfLength, v = q.dotProduct( bed.away ) / bed.halfWidth;
                    return u * u + v * v <= .82f;
                };
                const auto clump = [&]( float x, float z, u64 key, bool broad, bool flower, float vigour ) {
                    auto base = bed.centre + bed.along * ( x * bed.halfLength ) + bed.away * ( z * bed.halfWidth );
                    if( !within( base ) || !render::sampleTerrainHeight( terrain, frame, base, base.y ) ) return;
                    base.y += .014f;
                    auto &geometry = broad ? batches.leaf : batches.grass;
                    const size_t initial = geometry.indices.size();
                    const float height = vigour * ( broad ? .15f + .13f * unit( key + 2 ) : .235f + .20f * unit( key + 2 ) );
                    const u32 leaves = broad ? 8u : 7u;
                    for( u32 spray = 0; spray < leaves; ++spray )
                    {
                        const auto curl = render::foliageStableHash( key + 13u * spray + 77 );
                        const float angle = spray * 2.39996323f + 2.f * pi * unit( key + 3 ) + .42f * unit( curl );
                        const auto direction = bed.along * std::cos( angle ) + bed.away * std::sin( angle );
                        const Vector3F across( -direction.z, 0, direction.x );
                        const float lift = height * ( .62f + .43f * unit( curl + 1 ) );
                        float lean = vigour * ( broad ? .145f + .135f * unit( curl + 2 ) : .075f + .16f * unit( curl + 2 ) );
                        float width = vigour * ( broad ? .031f + .022f * unit( curl + 3 ) : .012f + .010f * unit( curl + 3 ) );
                        const auto root = base + direction * .022f;
                        if( !within( root + direction * lean ) )
                        {
                            lean *= .69f;
                            width *= .81f;
                        }
                        const auto low = root + up * ( lift * .52f ) + direction * ( lean * .17f );
                        const auto shoulder = root + up * ( lift * .91f ) + direction * ( lean * .58f );
                        const auto tip = root + up * ( lift * ( broad ? .70f : .91f ) ) + direction * lean;
                        if( within( tip ) ) cityVerge::foldedGrassBlade( geometry, std::array{ root, low, shoulder, tip },
                            std::array{ width * .21f, width * .83f, width * ( broad ? 1.f : .61f ) }, across, within );
                    }
                    if( geometry.indices.size() == initial ) return;
                    if( broad ) ++bed.perennials;
                    else ++bed.sedges;
                    if( !flower ) return;
                    for( u32 plume = 0; plume < ( compactBed ? 1u : 2u ); ++plume )
                    {
                        const float spread = plume == 0 ? -.066f : .073f;
                        const auto top = base + bed.along * spread + bed.away * ( .018f + .052f * unit( key + plume + 24 ) ) +
                                         up * ( height + .044f + .077f * unit( key + plume + 36 ) );
                        if( !within( top ) ) continue;
                        batches.grass.beam( base + up * .035f, top - up * .016f, .0075f );
                        batches.seedHeads.beam( top - up * .024f, top + up * .028f + bed.along * spread * .38f, .016f );
                        ++bed.heads;
                    }
                };

                // Broken broadleaf patches overlap at their margins, leaving some earth
                // visible between the taller, deliberately uneven sedge pockets.
                const std::array<float, 3> centres = { -.48f, -.035f, .46f };
                const std::array<float, 3> depths = { -.13f, .21f, -.16f };
                const std::array<u32, 3> leafCounts = compactBed ? std::array<u32, 3>{ 3, 2, 3 } : std::array<u32, 3>{ 4, 5, 4 };
                const std::array<u32, 3> sedgeCounts = compactBed ? std::array<u32, 3>{ 1, 2, 1 } : std::array<u32, 3>{ 3, 2, 3 };
                const float scale = compactBed ? .75f : 1.f;
                for( u32 drift = 0; drift < centres.size(); ++drift )
                {
                    const u64 key = render::foliageStableHash( bed.key + drift * 251u );
                    const float centre = centres[drift] + .075f * ( unit( key + 1 ) - .5f );
                    const float depth = depths[drift] + .12f * ( unit( key + 2 ) - .5f );
                    for( u32 i = 0; i < leafCounts[drift]; ++i )
                    {
                        const auto sample = render::foliageStableHash( key + 7u * i + 19u );
                        const float angle = i * 2.39996323f + 2.f * pi * unit( key + 3 );
                        const float reach = i == 0 ? 0.f : .60f + .38f * unit( sample + 1 );
                        clump( centre + std::cos( angle ) * reach * .255f, depth + std::sin( angle ) * reach * .40f,
                               sample, true, false, scale * ( .83f + .29f * unit( sample + 2 ) ) );
                    }
                    for( u32 i = 0; i < sedgeCounts[drift]; ++i )
                    {
                        const auto sample = render::foliageStableHash( key + 17u * i + 83u );
                        const float x = centre + ( float( i ) - float( sedgeCounts[drift] - 1u ) * .5f ) * .13f + .058f * unit( sample );
                        const float z = depth + ( drift == 1u ? -.25f : .23f ) + .20f * ( unit( sample + 1 ) - .5f );
                        const bool flower = i == 0 || ( drift == 0u && i == 2u );
                        clump( x, z, sample, false, flower, scale * ( .83f + .23f * unit( sample + 2 ) ) );
                    }
                }
            }

            template <class Clear>
            bool offer( Batches &batches, const cityEntranceRoute::Route &route, u64 lotId,
                        const render::TerrainData &terrain, const Transform3<real_Num> &frame, const Clear &clear )
            {
                if( route.rowCount < 5 || route.rowCount > route.rows.size() || route.length < 4.1f || batches.beds.size() >= 16 ) return false;
                const auto flat = []( Vector3F v ) { v.y = 0; return v; };
                const u64 key = render::foliageStableHash( lotId ^ 0x656e747279626564ull );
                const auto initial = flat( route.rows[0].right - route.rows[0].left ).normaliseCopy();
                const auto overall = flat( route.rows[route.rowCount - 1].centre - route.rows[0].centre );
                const float shift = overall.dotProduct( initial );
                const int firstSide = std::abs( shift ) > .42f ? ( shift > 0 ? -1 : 1 ) : ( key & 1u ? 1 : -1 );
                std::array<float, 96> stations{};
                for( size_t i = 1; i < route.rowCount; ++i )
                    stations[i] = stations[i - 1] + flat( route.rows[i].centre - route.rows[i - 1].centre ).length();
                const auto place = [&]( int side, bool companion ) {
                    const auto stationsToTry = companion ? std::array{ .58f, .54f, .62f, .50f } :
                                                           std::array{ .43f, .51f, .35f, .61f };
                    const auto setbacks = companion ? std::array{ .49f, .68f, .86f, 1.06f } :
                                                      std::array{ .58f, .89f, 1.18f, 1.48f };
                    const float halfWidth = companion ? .44f : .66f;
                    const float halfLength = companion ? .87f : std::clamp( route.length * .23f, .98f, 1.46f );
                    for( const float progress : stationsToTry )
                    {
                        const float station = stations[route.rowCount - 1] * progress;
                        const auto row = size_t( std::lower_bound( stations.begin() + 1, stations.begin() + route.rowCount, station ) - stations.begin() );
                        if( row == route.rowCount || stations[row] - stations[row - 1] < .02f ) continue;
                        const auto &a = route.rows[row - 1], &b = route.rows[row];
                        const auto along = flat( b.centre - a.centre ).normaliseCopy();
                        auto away = Vector3F( -along.z, 0, along.x );
                        if( away.dotProduct( initial * float( side ) ) < 0.f ) away = -away;
                        const float t = ( station - stations[row - 1] ) / ( stations[row] - stations[row - 1] );
                        const auto mix = [t]( Vector3F x, Vector3F y ) { return x + ( y - x ) * t; };
                        const auto centre = mix( a.centre, b.centre );
                        const auto edge = side > 0 ? mix( a.right, b.right ) : mix( a.left, b.left );
                        for( const float setback : setbacks )
                        {
                            auto position = centre + away * ( std::max( .70f, away.dotProduct( edge - centre ) ) + halfWidth + setback );
                            if( !render::sampleTerrainHeight( terrain, frame, position, position.y ) ) continue;
                            Bed bed{ position, along, away, halfLength, halfWidth, lotId,
                                     companion ? render::foliageStableHash( key + 37 ) : key };
                            bed.companion = companion;
                            const auto expanded = outline( bed, .26f );
                            if( std::any_of( batches.beds.begin(), batches.beds.end(), [&]( const Bed &other ) {
                                    const auto previous = outline( other, .21f );
                                    return !separated( expanded, previous, bed.centre ) && !separated( previous, expanded, other.centre );
                                } ) ) continue;
                            bool admitted = true;
                            for( size_t i = 1; admitted && i < route.rowCount; ++i )
                            {
                                const auto &p = route.rows[i - 1], &q = route.rows[i];
                                const std::array strip{ p.left, p.right, q.right, q.left };
                                if( !separated( expanded, strip, bed.centre ) && !separated( strip, expanded, ( p.centre + q.centre ) * .5f ) )
                                    admitted = false;
                            }
                            for( size_t i = 0; admitted && i < expanded.size(); ++i )
                                admitted = clear( std::array{ bed.centre, expanded[i], expanded[( i + 1 ) % expanded.size()] } );
                            if( !admitted ) continue;
                            Geometry surface;
                            const auto shape = outline( bed );
                            if( !drape( surface, bed, shape, terrain, frame ) ) continue;
                            cityParkLandscape::append( batches.earth, surface );
                            plant( batches, bed, terrain, frame );
                            batches.beds.push_back( bed );
                            return true;
                        }
                    }
                    return false;
                };
                int side = firstSide;
                if( !place( side, false ) )
                {
                    side = -side;
                    if( !place( side, false ) ) return false;
                }
                if( route.length >= 6.2f && batches.beds.size() < 16 ) place( -side, true );
                return true;
            }

            void publish( SceneAssets &assets, const Batches &batches, u32 seed )
            {
                if( batches.earth.indices.empty() ) return;
                TextureBuffer soil( 256, 128 );
                using cityVerge::noise;
                using cityVerge::smooth;
                using cityVerge::unit;
                for( u32 y = 0; y < soil.height; ++y )
                    for( u32 x = 0; x < soil.width; ++x )
                    {
                        const float u = ( x + .5f ) / soil.width, v = ( y + .5f ) / soil.height;
                        const float swale = noise( seed + 17, u * 6.f, v * 5.f, 719 );
                        const float pieces = noise( seed + 71, u * 66.f, v * 41.f, 179 );
                        const float grit = unit( ( u64( seed ) << 32 ) | ( y * soil.width + x ) );
                        const float radial = std::hypot( 2.f * u - 1.f, 2.f * v - 1.f );
                        const float ripple = std::sin( 2.f * pi * ( v * 12.5f + swale * .51f + pieces * .055f ) );
                        const float gravel = smooth( ( swale + .30f * pieces - .56f ) * 2.60f );
                        const float dust = 18.f * gravel + ( grit - .5f ) * 14.f + ( pieces - .5f ) * 22.f + 4.f * ripple;
                        const float coverage = smooth( ( .88f + .076f * swale - radial ) / .25f );
                        auto rgba = soil.pixel( x, y );
                        rgba[0] = u8( 95.f + dust ); rgba[1] = u8( 82.f + dust ); rgba[2] = u8( 64.f + .93f * dust );
                        rgba[3] = u8( 236.f * coverage * ( .88f + .12f * pieces ) );
                    }
                auto earth = material( assets, ColourF::White, .96f );
                earth->setTexture( upload( assets, soil ), 0 );
                earth->setTransparent( true );
                earth->setBlendMode( 1 );
                earth->setDepthWrite( false );
                earth->setUVProjection( 0 );
                if( auto actor = mesh( assets, "Terrain city entrance planted earth", batches.earth, earth ) )
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                const auto add = [&]( const char *name, const Geometry &geometry, ColourF tint ) {
                    if( geometry.indices.empty() ) return;
                    auto materialInstance = material( assets, tint, .89f );
                    materialInstance->setCullMode( 0 );
                    mesh( assets, name, geometry, materialInstance );
                };
                add( "Terrain city entrance broadleaf perennials", batches.leaf, { .35f, .46f, .23f, 1 } );
                add( "Terrain city entrance sedge drifts", batches.grass, { .43f, .52f, .25f, 1 } );
                add( "Terrain city entrance dry seed heads", batches.seedHeads, { .69f, .59f, .37f, 1 } );
                for( const auto &bed : batches.beds )
                    WP_LOG( String( "Terrain city entrance planted shoulder lot=" ) + StringUtil::toString( bed.lot ) +
                            ", x=" + StringUtil::toString( bed.centre.x ) + ", y=" + StringUtil::toString( bed.centre.y ) +
                            ", z=" + StringUtil::toString( bed.centre.z ) +
                            ", perennials=" + StringUtil::toString( bed.perennials ) +
                            ", sedges=" + StringUtil::toString( bed.sedges ) + ", seedHeads=" + StringUtil::toString( bed.heads ) +
                            ", companion=" + StringUtil::toString( bed.companion ? 1 : 0 ) );
                WP_LOG( String( "Terrain city entrance planted shoulders=" ) + StringUtil::toString( batches.beds.size() ) );
            }
        }  // namespace cityEntranceLandscape
