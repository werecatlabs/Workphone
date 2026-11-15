#pragma once

#include "CityParkLayout.hpp"
#include "CityTerrainProjection.hpp"

        namespace cityParkLandscape
        {
            using Patch = std::array<Vector3F, 4>;

            Vector3F frontage( OpenCityLayout::Frontage side )
            {
                switch( side )
                {
                case OpenCityLayout::Frontage::West: return { -1, 0, 0 };
                case OpenCityLayout::Frontage::East: return { 1, 0, 0 };
                case OpenCityLayout::Frontage::South: return { 0, 0, 1 };
                default: return { 0, 0, -1 };
                }
            }

            bool blocked( const Patch &patch, const cityRoadEdges::Surface &surface )
            {
                return surface.overlaps( { patch[0], patch[1], patch[2] } ) ||
                       surface.overlaps( { patch[0], patch[2], patch[3] } );
            }

            SmartPtr<render::IMaterial> packedGrit( SceneAssets &assets, u32 seed )
            {
                constexpr u32 size = 128;
                TextureBuffer map( size, size );
                for( u32 y = 0; y < size; ++y )
                {
                    for( u32 x = 0; x < size; ++x )
                    {
                        const auto grain = [&]( u32 cx, u32 cy ) {
                            return int( render::foliageStableHash( ( u64( seed ) << 32 ) | ( cy * size + cx ) ) >> 59 ) - 15;
                        };
                        const int weather = grain( x, y ) + grain( x / 3 + 48, y / 3 + 48 ) / 2;
                        auto *p = map.pixel( x, y );
                        p[0] = u8( 164 + weather ); p[1] = u8( 149 + weather );
                        p[2] = u8( 125 + weather ); p[3] = 255;
                    }
                }
                auto result = material( assets, ColourF::White, .96f );
                result->setTexture( upload( assets, map ), 0 );
                result->setUVProjection( 3 );
                result->setTriplanarScale( .46f );
                return result;
            }

            template <size_t Sides>
            void fitSeatBay( Layout &layout, Geometry &path, const std::array<Vector3F, Sides> &lawn,
                             const TerrainProjection &project, const Transform3<real_Num> &frame,
                             const cityRoadEdges::Surface &road, const cityRoadEdges::Surface &footways )
            {
                // The loop samples short corners densely. Span adjacent edges and cut the bay
                // mouth against each one to keep a readable path and preserve the lawn outside it.
                constexpr float halfSpan = 1.17f, depth = 1.30f, pathHeight = .037f;
                const std::array<size_t, 12> sectors{ 6, 11, 7, 10, 5, 12, 8, 9, 4, 13, 3, 14 };
                const auto inside = [&]( Vector3F p ) {
                    for( size_t i = 0; i < Sides; ++i )
                    {
                        const auto a = lawn[i], edge = lawn[( i + 1 ) % Sides] - a;
                        if( cityRoadEdges::cross( edge, layout.centre - a ) *
                            cityRoadEdges::cross( edge, p - a ) < -.0001f ) return false;
                    }
                    return true;
                };
                for( size_t order = 0; order < sectors.size(); ++order )
                {
                    const auto sector = sectors[order ^ size_t( ( layout.lot->variant >> 8 ) & 1u )] * Sides / 16;
                    constexpr size_t halfEdges = 5;
                    std::array<Vector3F, 2 * halfEdges + 1> rim;
                    for( size_t i = 0; i < rim.size(); ++i ) rim[i] = lawn[( sector + Sides - halfEdges + i ) % Sides];
                    const auto base = rim[halfEdges];
                    const auto along = ( rim.back() - rim.front() ).normaliseCopy();
                    Vector3F facing( along.z, 0, -along.x );
                    if( facing.dotProduct( base - layout.centre ) < 0 ) facing = -facing;
                    const auto across = [&]( Vector3F p ) { return ( p - base ).dotProduct( along ); };
                    if( across( rim.front() ) > -halfSpan || across( rim.back() ) < halfSpan ) continue;
                    const auto back = base - facing * depth;
                    if( !inside( back - along * halfSpan ) || !inside( back + along * halfSpan ) ) continue;
                    auto seat = base - facing * .72f;
                    float low = 0, high = 0;
                    if( !render::sampleTerrainHeight( project.terrain, frame, seat, seat.y ) ) continue;
                    low = high = seat.y;
                    const auto support = [&]( Vector3F p, float &height ) {
                        if( !render::sampleTerrainHeight( project.terrain, frame, p, height ) ) return false;
                        low = std::min( low, height ); high = std::max( high, height );
                        return true;
                    };
                    const Vector3F seatAcross( -facing.z, 0, facing.x );
                    std::array<float, 4> feet{};
                    bool valid = true;
                    for( size_t i = 0; i < feet.size(); ++i )
                    {
                        const auto p = seat + seatAcross * ( i < 2 ? -.69f : .69f ) +
                                       facing * ( i % 2 ? -.170f : .158f );
                        valid = valid && support( p, feet[i] );
                    }
                    if( !valid || high - low > .18f ) continue;
                    const float seatDatum = high;
                    Geometry bay;
                    for( size_t i = 1; i < rim.size() && valid; ++i )
                    {
                        const auto a = rim[i - 1], b = rim[i];
                        const auto from = across( a ), to = across( b );
                        if( to <= from ) { valid = false; break; }
                        const float start = std::max( -halfSpan, from ), end = std::min( halfSpan, to );
                        if( end - start < .0001f ) continue;
                        const Patch patch{ a + ( b - a ) * ( ( start - from ) / ( to - from ) ),
                                           a + ( b - a ) * ( ( end - from ) / ( to - from ) ),
                                           back + along * end, back + along * start };
                        float height;
                        for( const auto p : patch ) valid = valid && support( p, height );
                        valid = valid && high - low < .28f && !blocked( patch, road ) && !blocked( patch, footways ) &&
                                project.emit( bay, patch, [pathHeight]( Vector3F ) { return pathHeight; } );
                    }
                    if( !valid || bay.indices.empty() ) continue;
                    append( path, bay );
                    layout.hasSeat = true;
                    seat.y = seatDatum + pathHeight;
                    layout.seat = seat;
                    layout.seatForward = facing;
                    for( size_t i = 0; i < feet.size(); ++i ) layout.seatFootOffsets[i] = feet[i] - seatDatum;
                    return;
                }
            }

            SourceBudgetVector<Layout> build( SceneAssets &assets, const OpenCityLayout &city,
                const CityRoadTerrainMesh &fit, SmartPtr<IMesh> walkMesh, const cityRoadEdges::Surface &road,
                const SmartPtr<render::IMaterial> &paving, u32 seed, u32 textureSize,
                VehicleAppearanceQuality quality )
            {
                SourceBudgetVector<Layout> layouts;
                if( !walkMesh || std::none_of( city.lots.begin(), city.lots.end(), []( const auto &lot ) { return lot.park; } ) )
                    return layouts;
                const auto ground = assets.roadAuthoring->getTerrainSnapshot();
                if( !ground || ground->revision != fit.terrainRevision ) return layouts;
                const auto frame = assets.roadAuthoring->getTerrainPlacement();
                const TerrainProjection project( *ground, frame );
                const cityGroundContact::WalkEdges cuts( fit );
                const cityRoadEdges::Surface footways( walkMesh );
                Geometry grass, grit, kerbs;
                constexpr size_t sides = 32;
                constexpr float entryHalfAngle = pi / 16.f;
                const size_t limit = quality == VehicleAppearanceQuality::Preview ? 20 : 40;
                const auto vergeHeight = []( Vector3F ) { return .018f; };
                const auto pathHeight = []( Vector3F ) { return .037f; };
                const auto kerbHeight = []( Vector3F ) { return .041f; };
                for( const auto &lot : city.lots )
                {
                    if( !lot.park || std::min( lot.parcelWidth, lot.parcelDepth ) < 7.8f ) continue;
                    if( layouts.size() >= limit ) break;
                    Layout layout;
                    layout.lot = &lot;
                    const Vector3F centre( lot.parcelX, 0, lot.parcelZ );
                    const auto preferred = frontage( lot.frontage );
                    const Vector3F turn( -preferred.z, 0, preferred.x );
                    auto out = preferred;
                    float best = 15.f;
                    for( const auto &direction : { preferred, turn, -turn, -preferred } )
                    {
                        const float depth = ( std::abs( direction.x ) > .5f ? lot.parcelWidth : lot.parcelDepth ) * .5f;
                        const auto lip = centre + direction * ( depth - .95f );
                        const float distance = cuts.reach( lip, direction );
                        const float score = distance + ( direction.dotProduct( preferred ) < .5f ? 2.4f : 0.f );
                        if( distance >= .5f && score < best ) { out = direction; best = score; }
                    }
                    const Vector3F across( -out.z, 0, out.x );
                    const float width = std::abs( out.x ) > .5f ? lot.parcelDepth : lot.parcelWidth;
                    const float depth = std::abs( out.x ) > .5f ? lot.parcelWidth : lot.parcelDepth;
                    const float drift = ( float( lot.variant & 255u ) / 255.f - .5f ) * .62f;
                    const auto focus = centre + across * drift - out * .32f;
                    const float halfAcross = width * .5f - 1.15f, halfDepth = depth * .5f - 1.15f;
                    const float pathWidth = std::clamp( std::min( width, depth ) * .15f, 1.25f, 1.86f );
                    std::array<Vector3F, sides> outer{}, inner{};
                    for( size_t i = 0; i < sides; ++i )
                    {
                        const auto angle = i == 0 ? -entryHalfAngle : entryHalfAngle +
                            ( 2.f * pi - 2.f * entryHalfAngle ) * float( i - 1 ) / float( sides - 1 );
                        const auto u = std::sin( angle ), v = std::cos( angle );
                        const auto spread = 1.f + .025f * std::sin( angle * 3.f ) + .027f * std::cos( angle * 2.f );
                        outer[i] = focus + ( across * ( u * halfAcross ) + out * ( v * halfDepth ) ) * spread;
                        inner[i] = focus + ( across * ( u * ( halfAcross - pathWidth ) ) +
                                             out * ( v * ( halfDepth - pathWidth ) ) ) * spread;
                    }
                    const auto a = outer[0], b = outer[1], mouth = ( a + b ) * .5f;
                    const float mouthHalf = std::min( .96f, ( b - a ).length() * .5f );
                    const auto lipA = mouth - across * mouthHalf, lipB = mouth + across * mouthHalf;
                    const float reachA = cuts.reach( lipA, out ) - .035f, reachB = cuts.reach( lipB, out ) - .035f;
                    const auto c = lipB + out * reachB, d = lipA + out * reachA;
                    const Patch entrance{ a, b, c, d };
                    const cityRoadEdges::Bounds entranceBounds( entrance );
                    const bool overlapsParcel = std::any_of( city.lots.begin(), city.lots.end(), [&]( const auto &other ) {
                        if( other.id == lot.id ) return false;
                        const Vector3F center( other.parcelX, 0, other.parcelZ );
                        const Vector3F half( other.parcelWidth * .5f + .1f, 0, other.parcelDepth * .5f + .1f );
                        return entranceBounds.intersects( cityRoadEdges::Bounds( std::array{ center - half, center + half } ) );
                    } );
                    const bool canConnect = std::min( reachA, reachB ) >= .65f && std::max( reachA, reachB ) <= 13.f &&
                        std::abs( reachA - reachB ) <= 2.15f && !overlapsParcel &&
                        !blocked( entrance, road ) && !blocked( entrance, footways );
                    // Use the exact source diagonal for the clearances and drape. Sites that cross a
                    // fitted road, overlap a sidewalk, or span missing ground leave the meadow alone.
                    bool clear = true;
                    for( size_t i = 0; i < sides; ++i )
                    {
                        const auto j = ( i + 1 ) % sides;
                        const std::array foot{ centre, outer[i], outer[j] };
                        if( road.overlaps( foot ) || footways.overlaps( foot ) ) { clear = false; break; }
                    }
                    if( !clear ) continue;
                    Geometry lawnBatch, pathBatch, edging;
                    if( !project.emit( lawnBatch, inner, vergeHeight ) ) continue;
                    for( size_t i = 0; clear && i < sides; ++i )
                    {
                        const auto j = ( i + 1 ) % sides;
                        const auto p = outer[i], q = outer[j], r = inner[j], s = inner[i];
                        const bool atEntrance = i == 0 && canConnect;
                        const auto pTrim = atEntrance ? p : p + ( s - p ) * .065f;
                        const auto qTrim = atEntrance ? q : q + ( r - q ) * .065f;
                        const auto rTrim = r + ( q - r ) * .065f, sTrim = s + ( p - s ) * .065f;
                        clear = project.emit( pathBatch, Patch{ pTrim, qTrim, rTrim, sTrim }, pathHeight ) &&
                                project.emit( edging, Patch{ sTrim, rTrim, r, s }, kerbHeight ) &&
                                ( atEntrance || project.emit( edging, Patch{ p, q, qTrim, pTrim }, kerbHeight ) );
                        layout.paths.push_back( { ( p + s ) * .5f, ( q + r ) * .5f, pathWidth * .57f } );
                    }
                    if( !clear ) continue;
                    layout.entry = mouth;
                    layout.centre = focus;
                    if( !render::sampleTerrainHeight( *ground, frame, layout.centre, layout.centre.y ) ||
                        !render::sampleTerrainHeight( *ground, frame, layout.entry, layout.entry.y ) ) continue;
                    layout.entry.y += pathHeight( layout.entry );
                    if( canConnect )
                    {
                        const auto ramp = std::min( 1.55f, std::min( reachA, reachB ) - .13f );
                        const auto rampC = c - out * ramp, rampD = d - out * ramp;
                        const auto entryHeight = [&]( Vector3F point ) {
                            const float u = std::clamp( ( point - lipA ).dotProduct( across ) / ( 2.f * mouthHalf ), 0.f, 1.f );
                            const float run = reachA + ( reachB - reachA ) * u;
                            return .037f + .109f * std::clamp( ( ( point - lipA ).dotProduct( out ) - run + ramp ) / ramp, 0.f, 1.f );
                        };
                        auto edge = ( c + d ) * .5f;
                        Geometry spur;
                        if( project.emit( spur, Patch{ a, b, rampC, rampD }, pathHeight ) &&
                            project.emit( spur, Patch{ rampD, rampC, c, d }, entryHeight ) &&
                            render::sampleTerrainHeight( *ground, frame, edge, edge.y ) )
                        {
                            append( pathBatch, spur );
                            layout.entry = edge + Vector3F( 0, .146f, 0 );
                            layout.hasEntry = true;
                            layout.paths.push_back( { mouth, edge, ( b - a ).length() * .52f } );
                        }
                    }
                    fitSeatBay( layout, pathBatch, inner, project, frame, road, footways );
                    append( grass, lawnBatch ); append( grit, pathBatch ); append( kerbs, edging );
                    WP_LOG( String( "Terrain city park lot=" ) + StringUtil::toString( lot.id ) +
                            ", sidewalk entry=" + StringUtil::toString( layout.hasEntry ? 1 : 0 ) +
                            ", bench bay=" + StringUtil::toString( layout.hasSeat ? 1 : 0 ) );
                    layouts.push_back( std::move( layout ) );
                }
                if( layouts.empty() ) return layouts;
                auto turf = material( assets, { .49f, .63f, .36f, 1 }, .98f );
                applyCitySurface( assets, turf, CitySurface::Ground, textureSize, .31f, 3 );
                turf->setDiffuse( { .59f, .75f, .48f, 1 } );
                const auto publish = [&]( const String &name, const Geometry &geometry,
                                          const SmartPtr<render::IMaterial> &surface ) {
                    if( auto actor = mesh( assets, name, geometry, surface ) )
                        actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                };
                publish( "Terrain city park clipped lawn", grass, turf );
                publish( "Terrain city park gravel walks", grit, packedGrit( assets, seed ) );
                publish( "Terrain city park stone path edging", kerbs, paving );
                WP_LOG( String( "Terrain city landscaped parks=" ) + StringUtil::toString( layouts.size() ) +
                        ", path faces=" + StringUtil::toString( grit.indices.size() / 3 ) );
                return layouts;
            }
        }
