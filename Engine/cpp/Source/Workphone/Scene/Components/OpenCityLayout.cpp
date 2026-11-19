#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <algorithm>
#include <cmath>
#include <optional>

namespace workphone::procedural
{
    namespace
    {
        using Lot = OpenCityLayout::Lot;
        using District = OpenCityLayout::District;
        using Frontage = OpenCityLayout::Frontage;

        std::uint64_t hash( std::uint64_t value )
        {
            value ^= value >> 30;
            value *= 0xbf58476d1ce4e5b9ULL;
            value ^= value >> 27;
            value *= 0x94d049bb133111ebULL;
            return value ^ ( value >> 31 );
        }

        // One independent value in [0, 1) per identity and stream.
        float random( std::uint64_t identity, std::uint64_t stream )
        {
            return float( hash( identity ^ ( stream * 0x9e3779b97f4a7c15ULL ) ) >> 40 ) / 16777216.f;
        }

        // Salts and streams are part of the generated layout. Changing any of them
        // reshuffles every city, so it must come with a generatorRevision bump.
        constexpr std::uint64_t variantSalt = 0xb76a5d29ULL;
        constexpr std::uint64_t districtSalt = 0x6469737472696374ULL;  // "district"
        constexpr std::uint64_t parcelRankSalt = 0x70617263656cULL;    // "parcel"

        enum Stream : std::uint64_t
        {
            FrontageStream = 1,
            WidthStream,
            DepthStream,
            HeightStream,
            ParkStream,
            TowerWidthStream,
            TowerDepthStream
        };

        float pointSegmentDistance( float x, float z, float ax, float az, float bx, float bz )
        {
            const float dx = bx - ax, dz = bz - az, l2 = dx * dx + dz * dz;
            const float t =
                l2 > 1e-8f ? std::clamp( ( ( x - ax ) * dx + ( z - az ) * dz ) / l2, 0.f, 1.f ) : 0;
            return std::hypot( x - ax - t * dx, z - az - t * dz );
        }

        struct Ends
        {
            const OpenCityRoadNetwork::Point *a, *b;
        };

        // The road graph is a public member, so tolerate edges that name no node.
        std::optional<Ends> edgeEnds( const OpenCityRoadNetwork &roads,
                                      const OpenCityRoadNetwork::Edge &edge )
        {
            if( edge.a >= roads.nodes.size() || edge.b >= roads.nodes.size() )
            {
                return std::nullopt;
            }
            return Ends{ &roads.nodes[edge.a], &roads.nodes[edge.b] };
        }

        struct Interval
        {
            float min, max;
        };

        // Each half block runs from its street setback to the central service gap.
        Interval gridParcelSpan( int cell, bool upperHalf, float spacing )
        {
            const float origin = cell * spacing;
            const float middle = spacing * .5f;
            const float halfGap = OpenCityLayout::serviceGap * .5f;
            if( upperHalf )
            {
                return { origin + ( middle + halfGap ),
                         origin + ( spacing - OpenCityLayout::roadSetback ) };
            }
            return { origin + OpenCityLayout::roadSetback, origin + ( middle - halfGap ) };
        }

        // The town centre and an east-west commercial spine are commercial; one
        // block in eleven is civic. Compared in squared half-cell units, which are
        // exact, so the result cannot depend on a platform's hypot rounding.
        District gridDistrict( int cellX, int cellZ, std::uint64_t blockHash )
        {
            const float cx = cellX + .5f, cz = cellZ + .5f;
            if( cx * cx + cz * cz < 1.5f * 1.5f || ( std::abs( cz ) < .6f && std::abs( cx ) < 3.2f ) )
            {
                return District::Commercial;
            }
            return blockHash % 11 == 0 ? District::Civic : District::Residential;
        }

        float gridParkChance( District district )
        {
            return district == District::Commercial ? .04f
                   : district == District::Civic    ? .22f
                                                    : .16f;
        }

        Lot makeGridLot( std::uint32_t seed, int cellX, int cellZ, std::uint64_t quadrant,
                         float spacing, District district, float density )
        {
            const bool east = quadrant >= 2;
            const bool south = ( quadrant & 1 ) != 0;
            const auto x = gridParcelSpan( cellX, east, spacing );
            const auto z = gridParcelSpan( cellZ, south, spacing );
            // The seed, signed cell and quadrant identify each parcel across city sizes.
            const std::uint64_t id = ( ( std::uint64_t( seed ) << 18 ) |
                                       ( std::uint64_t( std::uint32_t( cellX ) & 255u ) << 10 ) |
                                       ( std::uint64_t( std::uint32_t( cellZ ) & 255u ) << 2 ) |
                                       quadrant ) +
                                     1;
            Lot lot{};
            lot.id = id;
            lot.variant = std::uint32_t( hash( id ^ variantSalt ) );
            lot.district = district;
            lot.frontage = random( id, FrontageStream ) < .5f
                               ? ( east ? Frontage::East : Frontage::West )
                               : ( south ? Frontage::South : Frontage::North );
            lot.parcelX = ( x.min + x.max ) * .5f;
            lot.parcelZ = ( z.min + z.max ) * .5f;
            lot.parcelWidth = x.max - x.min;
            lot.parcelDepth = z.max - z.min;
            lot.width = lot.parcelWidth * ( .72f + .23f * random( id, WidthStream ) );
            lot.depth = lot.parcelDepth * ( .68f + .27f * random( id, DepthStream ) );
            lot.x = lot.frontage == Frontage::West   ? x.min + lot.width * .5f
                    : lot.frontage == Frontage::East ? x.max - lot.width * .5f
                                                     : lot.parcelX;
            lot.z = lot.frontage == Frontage::North   ? z.min + lot.depth * .5f
                    : lot.frontage == Frontage::South ? z.max - lot.depth * .5f
                                                      : lot.parcelZ;
            const float nominalHeight =
                district == District::Commercial
                    ? 20 + random( id, HeightStream ) * ( 18 + density * 34 )
                : district == District::Civic ? 10 + random( id, HeightStream ) * 14
                                              : 8 + random( id, HeightStream ) * ( 11 + density * 10 );
            const float floorHeight = district == District::Commercial ? 3.6f : 3.2f;
            lot.height =
                std::max( floorHeight * 2, std::floor( nominalHeight / floorHeight ) * floorHeight );
            lot.park = random( id, ParkStream ) < gridParkChance( district );
            if( lot.park )
            {
                lot.x = lot.parcelX;
                lot.z = lot.parcelZ;
                lot.width = lot.parcelWidth;
                lot.depth = lot.parcelDepth;
            }
            return lot;
        }

        // Natural-city tuning. Everything here shapes the generated layout.
        constexpr std::size_t maximumCandidates = 8192;
        // The commercial district reaches further out than the core that is packed
        // first, so core lots win overlaps before the rest compete for the budget.
        constexpr float commercialRadius = .40f, packingCoreRadius = .34f;
        constexpr float minimumPitch = 18.f, maximumPitch = 28.f;
        // Along-street spans of a typical lot, per axis, for central and outer streets.
        constexpr float centralSpanX = 23.f, centralSpanZ = 21.f;
        constexpr float outerSpanX = 15.f, outerSpanZ = 18.f;
        // Clear gap between the street's sidewalk and a lot's near face.
        constexpr float frontageMargin = .025f;
        constexpr std::uint64_t civicOneIn = 13;
        constexpr float naturalCommercialParkChance = .04f, naturalOtherParkChance = .10f;

        struct FrontageCandidate
        {
            Lot lot;
            std::uint64_t rank;
            bool cityCentre;
        };

        // One side of one street: the unit the lot samples are laid along.
        struct StreetSide
        {
            std::size_t edgeIndex;
            OpenCityRoadNetwork::Point start;
            float dx, dz, nx, nz;
            float halfWidth;
            unsigned side;
        };

        void packFrontages( SourceBudgetVector<FrontageCandidate> &candidates, std::size_t limit,
                            SourceBudgetVector<Lot> &lots )
        {
            std::sort( candidates.begin(), candidates.end(), []( const auto &a, const auto &b ) {
                // Fill the core first, then distribute the remaining budget by seeded rank.
                if( a.cityCentre != b.cityCentre )
                {
                    return a.cityCentre;
                }
                return a.rank != b.rank ? a.rank < b.rank : a.lot.id < b.lot.id;
            } );
            lots.reserve( limit );
            for( const auto &candidate : candidates )
            {
                const auto &lot = candidate.lot;
                const bool overlaps =
                    std::any_of( lots.begin(), lots.end(), [&]( const auto &occupied ) {
                        return OpenCityLayout::parcelsOverlap( lot, occupied );
                    } );
                if( !overlaps )
                {
                    lots.push_back( lot );
                    if( lots.size() == limit )
                    {
                        break;
                    }
                }
            }
        }

        // The route leaves the origin facing -Z around the loop. The final return to
        // the origin is implicit: consumers wrap the polyline, and repeating the first
        // point would add a zero-length step.
        void appendGridRoute( SourceBudgetVector<OpenCityLayout::Point> &route, float radius )
        {
            using Point = OpenCityLayout::Point;
            const Point corners[] = { { 0, 0 },           { 0, -radius }, { radius, -radius },
                                      { radius, radius }, { 0, radius },  { 0, 0 } };
            for( int i = 0; i < 5; ++i )
            {
                const auto a = corners[i], b = corners[i + 1];
                const int steps = int( std::ceil( std::hypot( b.x - a.x, b.z - a.z ) / 2 ) );
                for( int j = 0; j < steps; ++j )
                {
                    const float t = float( j ) / steps;
                    route.push_back( { a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t } );
                }
            }
        }
    }  // namespace

    OpenCityLayout OpenCityLayout::generate( std::uint32_t seed, int blocks, int route,
                                             bool naturalRoads )
    {
        OpenCityLayout city;
        city.blocks = std::clamp( blocks / 2 * 2, 6, 10 );
        city.spacing = 88.f + float( seed % 17 );
        city.extent = city.blocks * city.spacing * .5f;
        city.naturalRoads = naturalRoads;
        if( naturalRoads )
        {
            city.generateNatural( seed, route );
            return city;
        }
        city.generateGridLots( seed );
        appendGridRoute( city.route,
                         ( city.blocks / 2 - 2 + std::clamp( route, 0, 2 ) ) * city.spacing );
        return city;
    }

    SourceBudgetVector<OpenCityLayout::BuildingVolume> OpenCityLayout::buildingVolumes( const Lot &lot )
    {
        SourceBudgetVector<BuildingVolume> result;
        if( lot.park || !std::isfinite( lot.x ) || !std::isfinite( lot.z ) ||
            !std::isfinite( lot.width ) || !std::isfinite( lot.depth ) || !std::isfinite( lot.height ) ||
            lot.width <= 0 || lot.depth <= 0 || lot.height <= 0 || !std::isfinite( lot.groundHeight ) )
        {
            return result;
        }
        result.reserve( 3 );
        if( lot.district == District::Residential || lot.height < 12 )
        {
            result.push_back( { lot.x, lot.z, lot.width, lot.depth, lot.groundHeight, lot.height } );
            return result;
        }
        const float baseHeight =
            std::min( lot.height * .35f, lot.district == District::Commercial ? 10.8f : 6.4f );
        result.push_back( { lot.x, lot.z, lot.width, lot.depth, lot.groundHeight, baseHeight } );
        const float width = lot.width * ( .68f + .10f * random( lot.variant, TowerWidthStream ) );
        const float depth = lot.depth * ( .66f + .12f * random( lot.variant, TowerDepthStream ) );
        const float x = lot.x + ( lot.frontage == Frontage::West   ? 1.f
                                  : lot.frontage == Frontage::East ? -1.f
                                                                   : 0 ) *
                                    ( lot.width - width ) * .20f;
        const float z = lot.z + ( lot.frontage == Frontage::North   ? 1.f
                                  : lot.frontage == Frontage::South ? -1.f
                                                                    : 0 ) *
                                    ( lot.depth - depth ) * .20f;
        const float crownHeight =
            lot.district == District::Commercial ? std::min( lot.height * .12f, 5.4f ) : 0;
        result.push_back( { x, z, width, depth, lot.groundHeight + baseHeight,
                            lot.height - baseHeight - crownHeight } );
        if( crownHeight > 0 )
        {
            result.push_back( { x, z, width * .84f, depth * .84f,
                                lot.groundHeight + lot.height - crownHeight, crownHeight } );
        }
        return result;
    }

    float OpenCityLayout::roadDistance( float x, float z ) const
    {
        if( !std::isfinite( x ) || !std::isfinite( z ) || !std::isfinite( spacing ) ||
            !std::isfinite( extent ) || spacing <= 0 || extent <= 0 )
        {
            return 1000;
        }
        if( std::abs( x ) > extent + 6 || std::abs( z ) > extent + 6 )
        {
            return 1000;
        }
        if( naturalRoads )
        {
            float distance = 1000;
            for( const auto &edge : roads.edges )
            {
                const auto ends = edgeEnds( roads, edge );
                if( !ends )
                {
                    continue;
                }
                const auto &a = *ends->a, &b = *ends->b;
                // The gap to the segment's bounding box is a lower bound on its distance.
                if( std::min( a.x, b.x ) - x > distance || x - std::max( a.x, b.x ) > distance ||
                    std::min( a.z, b.z ) - z > distance || z - std::max( a.z, b.z ) > distance )
                {
                    continue;
                }
                distance = std::min( distance, pointSegmentDistance( x, z, a.x, a.z, b.x, b.z ) );
            }
            return distance;
        }
        return std::min( std::abs( x - std::round( x / spacing ) * spacing ),
                         std::abs( z - std::round( z / spacing ) * spacing ) );
    }

    float OpenCityLayout::footprintRoadClearance( float x, float z, float width, float depth ) const
    {
        return footprintClearance( x, z, width, depth, false );
    }

    float OpenCityLayout::footprintRoadEdgeClearance( float x, float z, float width, float depth ) const
    {
        return footprintClearance( x, z, width, depth, true );
    }

    float OpenCityLayout::footprintClearance( float x, float z, float width, float depth,
                                              bool fromEdge, float upperBound ) const
    {
        if( !std::isfinite( x ) || !std::isfinite( z ) || !std::isfinite( width ) ||
            !std::isfinite( depth ) || width < 0 || depth < 0 || spacing <= 0 )
        {
            return 0;
        }
        const float x0 = x - width * .5f, x1 = x + width * .5f;
        const float z0 = z - depth * .5f, z1 = z + depth * .5f;
        float distance = upperBound;
        const auto segment = [&]( float ax, float az, float bx, float bz, float halfWidth ) {
            // An axis gap bounds the closest possible centreline distance.
            // Lot placement only needs distances inside the required setback.
            const float reach = distance + ( fromEdge ? halfWidth : 0.f );
            if( std::min( ax, bx ) - x1 > reach || x0 - std::max( ax, bx ) > reach ||
                std::min( az, bz ) - z1 > reach || z0 - std::max( az, bz ) > reach )
            {
                return;
            }
            float t0 = 0, t1 = 1;
            const auto clip = [&]( float p, float q ) {
                if( std::abs( p ) < 1e-7f )
                {
                    return q >= 0;
                }
                const float t = q / p;
                if( p < 0 )
                {
                    t0 = std::max( t0, t );
                }
                else
                {
                    t1 = std::min( t1, t );
                }
                return t0 <= t1;
            };
            if( clip( ax - bx, ax - x0 ) && clip( bx - ax, x1 - ax ) && clip( az - bz, az - z0 ) &&
                clip( bz - az, z1 - az ) )
            {
                distance = std::min( distance, fromEdge ? -halfWidth : 0.f );
                return;
            }
            const auto toBox = [&]( float px, float pz ) {
                return std::hypot( std::max( { x0 - px, 0.f, px - x1 } ),
                                   std::max( { z0 - pz, 0.f, pz - z1 } ) );
            };
            const float nearest = std::min( { toBox( ax, az ), toBox( bx, bz ),
                                              pointSegmentDistance( x0, z0, ax, az, bx, bz ),
                                              pointSegmentDistance( x1, z0, ax, az, bx, bz ),
                                              pointSegmentDistance( x0, z1, ax, az, bx, bz ),
                                              pointSegmentDistance( x1, z1, ax, az, bx, bz ) } );
            distance = std::min( distance, nearest - ( fromEdge ? halfWidth : 0.f ) );
        };
        if( naturalRoads )
        {
            for( const auto &edge : roads.edges )
            {
                if( const auto ends = edgeEnds( roads, edge ) )
                {
                    segment( ends->a->x, ends->a->z, ends->b->x, ends->b->z,
                             OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f );
                }
            }
        }
        else
        {
            for( int i = -blocks / 2; i <= blocks / 2; ++i )
            {
                segment( i * spacing, -extent, i * spacing, extent, roadHalfWidth );
                segment( -extent, i * spacing, extent, i * spacing, roadHalfWidth );
            }
        }
        return distance;
    }

    void OpenCityLayout::generateGridLots( std::uint32_t seed )
    {
        lots.reserve( std::size_t( blocks * blocks * 4 ) );
        for( int cellX = -blocks / 2; cellX < blocks / 2; ++cellX )
        {
            for( int cellZ = -blocks / 2; cellZ < blocks / 2; ++cellZ )
            {
                const auto block = hash( ( std::uint64_t( seed ) << 32 ) ^ std::uint32_t( cellX ) ^
                                         ( std::uint64_t( std::uint32_t( cellZ ) ) << 16 ) );
                // Taller towers cluster towards the centre; this is the only place the
                // distance falloff is used, so it never feeds a district decision.
                const float density = std::exp( -std::hypot( cellX + .5f, cellZ + .5f ) * .55f );
                const auto district = gridDistrict( cellX, cellZ, block );
                for( std::uint64_t quadrant = 0; quadrant < 4; ++quadrant )
                {
                    lots.push_back( makeGridLot( seed, cellX, cellZ, quadrant, spacing, district, density ) );
                }
            }
        }
    }

    void OpenCityLayout::generateNatural( std::uint32_t seed, int routeChoice )
    {
        roads = OpenCityRoadNetwork::generate( seed, blocks, routeChoice );
        route.reserve( roads.route.size() );
        for( const auto &p : roads.route )
        {
            route.push_back( { p.x, p.z } );
        }

        const auto within = [this]( float x, float z, float fraction ) {
            const float radius = fraction * extent;
            return x * x + z * z < radius * radius;
        };

        // The candidate key uses disjoint fields over the bounded graph domain.
        // Route selection never enters its identity.
        const auto makeCandidate = [&]( const StreetSide &street, float along, unsigned phase,
                                        int sample ) -> std::optional<FrontageCandidate> {
            const std::uint64_t identity = ( std::uint64_t( seed ) << 32 ) |
                                           ( std::uint64_t( street.edgeIndex ) << 17 ) |
                                           ( std::uint64_t( street.side ) << 16 ) |
                                           ( std::uint64_t( phase ) << 14 ) | std::uint64_t( sample + 1 );
            Lot lot{};
            lot.id = identity + 1;
            lot.variant = std::uint32_t( hash( lot.id ^ variantSalt ) );
            const float streetX = street.start.x + street.dx * along;
            const float streetZ = street.start.z + street.dz * along;
            lot.district = within( streetX, streetZ, commercialRadius ) ? District::Commercial
                           : hash( lot.id ^ districtSalt ) % civicOneIn == 0
                               ? District::Civic
                               : District::Residential;
            const bool commercial = lot.district == District::Commercial;
            lot.width = lot.parcelWidth =
                commercial ? 20.f + random( lot.id, WidthStream ) * 6.f
                           : 12.f + random( lot.id, WidthStream ) * 6.f;
            lot.depth = lot.parcelDepth =
                commercial ? 18.f + random( lot.id, DepthStream ) * 6.f
                           : 14.f + random( lot.id, DepthStream ) * 8.f;
            // Projection of the whole AABB onto the road normal gives
            // an exact supporting-line setback, including diagonal roads.
            const float offset = street.halfWidth + roadsideSetback + frontageMargin +
                                 ( std::abs( street.nx ) * lot.width + std::abs( street.nz ) * lot.depth ) *
                                     .5f;
            lot.x = lot.parcelX = streetX + street.nx * offset;
            lot.z = lot.parcelZ = streetZ + street.nz * offset;
            if( std::abs( lot.x ) + lot.width * .5f > extent - roadSetback ||
                std::abs( lot.z ) + lot.depth * .5f > extent - roadSetback ||
                footprintClearance( lot.x, lot.z, lot.width, lot.depth, true, roadsideSetback ) <
                    roadsideSetback )
            {
                return std::nullopt;
            }
            lot.frontage = std::abs( street.nx ) > std::abs( street.nz )
                               ? ( street.nx > 0 ? Frontage::West : Frontage::East )
                               : ( street.nz > 0 ? Frontage::North : Frontage::South );
            const float floorHeight = commercial ? 3.6f : 3.2f;
            const float nominalHeight = commercial ? 22.f + random( lot.id, HeightStream ) * 35.f
                                        : lot.district == District::Civic
                                            ? 10.f + random( lot.id, HeightStream ) * 10.f
                                            : 8.f + random( lot.id, HeightStream ) * 14.f;
            lot.height = floorHeight * std::floor( nominalHeight / floorHeight );
            lot.park = random( lot.id, ParkStream ) <
                       ( commercial ? naturalCommercialParkChance : naturalOtherParkChance );
            return FrontageCandidate{ lot, hash( lot.id ^ parcelRankSalt ),
                                      within( streetX, streetZ, packingCoreRadius ) };
        };

        // Sample both frontages of each actual street, rather than rejecting a
        // sparse world grid. Staggered samples fill the spaces left by junctions
        // while retaining the existing axis-aligned building/collision contract.
        SourceBudgetVector<FrontageCandidate> candidates;
        candidates.reserve( std::min( maximumCandidates, roads.edges.size() * 24 ) );
        const auto collect = [&] {
            for( std::size_t edgeIndex = 0; edgeIndex < roads.edges.size(); ++edgeIndex )
            {
                const auto &edge = roads.edges[edgeIndex];
                const auto ends = edgeEnds( roads, edge );
                if( !ends )
                {
                    continue;
                }
                const auto &a = *ends->a, &b = *ends->b;
                const float length = std::hypot( b.x - a.x, b.z - a.z );
                if( length < OpenCityRoadNetwork::minimumEdgeLength )
                {
                    continue;
                }
                const float dx = ( b.x - a.x ) / length, dz = ( b.z - a.z ) / length;
                const bool central = within( ( a.x + b.x ) * .5f, ( a.z + b.z ) * .5f, commercialRadius );
                const float projectedSpan =
                    std::abs( dx ) * ( central ? centralSpanX : outerSpanX ) +
                    std::abs( dz ) * ( central ? centralSpanZ : outerSpanZ );
                const float pitch = std::clamp( projectedSpan + parcelSeparation, minimumPitch, maximumPitch );
                const int steps = std::max( 1, int( std::ceil( length / pitch ) ) );
                for( unsigned side = 0; side < 2; ++side )
                {
                    const StreetSide street{ edgeIndex,
                                             a,
                                             dx,
                                             dz,
                                             side ? dz : -dz,
                                             side ? -dx : dx,
                                             OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f,
                                             side };
                    for( unsigned phase = 0; phase < 3; ++phase )
                    {
                        for( int sample = 0; sample <= steps; ++sample )
                        {
                            if( candidates.size() == maximumCandidates )
                            {
                                return;
                            }
                            // Staggered phases clamp onto the street end, which phase 0
                            // already sampled; repeating it only wastes candidates.
                            if( phase && sample == steps )
                            {
                                continue;
                            }
                            const float along =
                                std::min( length, ( sample + phase / 3.f ) * length / steps );
                            if( auto candidate = makeCandidate( street, along, phase, sample ) )
                            {
                                candidates.push_back( *candidate );
                            }
                        }
                    }
                }
            }
        };
        collect();
        packFrontages( candidates, std::size_t( blocks * blocks * 4 ), lots );
    }
}  // namespace workphone::procedural
