#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::procedural
{
    namespace
    {
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

        struct FrontageCandidate
        {
            OpenCityLayout::Lot lot;
            std::uint64_t rank;
            bool cityCentre;
        };

        void packFrontages( SourceBudgetVector<FrontageCandidate> &candidates, std::size_t limit,
                            SourceBudgetVector<OpenCityLayout::Lot> &lots )
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
                        return std::abs( lot.parcelX - occupied.parcelX ) <
                                   ( lot.parcelWidth + occupied.parcelWidth ) * .5f + .75f &&
                               std::abs( lot.parcelZ - occupied.parcelZ ) <
                                   ( lot.parcelDepth + occupied.parcelDepth ) * .5f + .75f;
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
    }  // namespace

    void OpenCityLayout::generateGridLots( std::uint32_t seed )
    {
        lots.reserve( std::size_t( blocks * blocks * 4 ) );
        for( int cellX = -blocks / 2; cellX < blocks / 2; ++cellX )
        {
            for( int cellZ = -blocks / 2; cellZ < blocks / 2; ++cellZ )
            {
                const auto block = hash( ( std::uint64_t( seed ) << 32 ) ^ std::uint32_t( cellX ) ^
                                         ( std::uint64_t( std::uint32_t( cellZ ) ) << 16 ) );
                const float cellRadius = std::hypot( cellX + .5f, cellZ + .5f );
                const float density = std::exp( -cellRadius * .55f );
                const auto district = cellRadius < 1.5f || ( std::abs( cellZ + .5f ) < .6f &&
                                                             std::abs( cellX + .5f ) < 3.2f )
                                          ? District::Commercial
                                      : block % 11 == 0 ? District::Civic
                                                        : District::Residential;
                for( std::uint64_t quadrant = 0; quadrant < 4; ++quadrant )
                {
                    const bool east = quadrant >= 2;
                    const bool south = ( quadrant & 1 ) != 0;
                    const auto x = gridParcelSpan( cellX, east, spacing );
                    const auto z = gridParcelSpan( cellZ, south, spacing );
                    // The seed, signed cell and quadrant identify each parcel across city sizes.
                    const std::uint64_t id =
                        ( ( std::uint64_t( seed ) << 18 ) |
                          ( std::uint64_t( std::uint32_t( cellX ) & 255u ) << 10 ) |
                          ( std::uint64_t( std::uint32_t( cellZ ) & 255u ) << 2 ) | quadrant ) +
                        1;
                    Lot lot{};
                    lot.id = id;
                    lot.variant = std::uint32_t( hash( id ^ 0xb76a5d29ULL ) );
                    lot.district = district;
                    lot.frontage = random( id, 1 ) < .5f ? ( east ? Frontage::East : Frontage::West )
                                                         : ( south ? Frontage::South : Frontage::North );
                    lot.parcelX = ( x.min + x.max ) * .5f;
                    lot.parcelZ = ( z.min + z.max ) * .5f;
                    lot.parcelWidth = x.max - x.min;
                    lot.parcelDepth = z.max - z.min;
                    lot.width = lot.parcelWidth * ( .72f + .23f * random( id, 2 ) );
                    lot.depth = lot.parcelDepth * ( .68f + .27f * random( id, 3 ) );
                    lot.x = lot.frontage == Frontage::West   ? x.min + lot.width * .5f
                            : lot.frontage == Frontage::East ? x.max - lot.width * .5f
                                                             : lot.parcelX;
                    lot.z = lot.frontage == Frontage::North   ? z.min + lot.depth * .5f
                            : lot.frontage == Frontage::South ? z.max - lot.depth * .5f
                                                              : lot.parcelZ;
                    const float nominalHeight =
                        district == District::Commercial ? 20 + random( id, 4 ) * ( 18 + density * 34 )
                        : district == District::Civic    ? 10 + random( id, 4 ) * 14
                                                         : 8 + random( id, 4 ) * ( 11 + density * 10 );
                    const float floorHeight = district == District::Commercial ? 3.6f : 3.2f;
                    lot.height = std::max( floorHeight * 2,
                                           std::floor( nominalHeight / floorHeight ) * floorHeight );
                    lot.park = random( id, 5 ) < ( district == District::Commercial ? .04f
                                                   : district == District::Civic    ? .22f
                                                                                    : .16f );
                    if( lot.park )
                    {
                        lot.x = lot.parcelX;
                        lot.z = lot.parcelZ;
                        lot.width = lot.parcelWidth;
                        lot.depth = lot.parcelDepth;
                    }
                    lots.push_back( lot );
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

        // Sample both frontages of each actual street, rather than rejecting a
        // sparse world grid. Staggered samples fill the spaces left by junctions
        // while retaining the existing axis-aligned building/collision contract.
        SourceBudgetVector<FrontageCandidate> candidates;
        constexpr std::size_t maximumCandidates = 8192;
        candidates.reserve( std::min( maximumCandidates, roads.edges.size() * 24 ) );
        for( std::size_t edgeIndex = 0; edgeIndex < roads.edges.size(); ++edgeIndex )
        {
            const auto &edge = roads.edges[edgeIndex];
            const auto &a = roads.nodes[edge.a], &b = roads.nodes[edge.b];
            const float length = std::hypot( b.x - a.x, b.z - a.z );
            if( length < OpenCityRoadNetwork::minimumEdgeLength )
            {
                continue;
            }
            const float dx = ( b.x - a.x ) / length, dz = ( b.z - a.z ) / length;
            for( unsigned side = 0; side < 2; ++side )
            {
                const float nx = side ? dz : -dz, nz = side ? -dx : dx;
                const bool central =
                    std::hypot( ( a.x + b.x ) * .5f, ( a.z + b.z ) * .5f ) < extent * .40f;
                const float projectedSpan = std::abs( dx ) * ( central ? 23.f : 15.f ) +
                                            std::abs( dz ) * ( central ? 21.f : 18.f );
                const float pitch = std::clamp( projectedSpan + .75f, 18.f, 28.f );
                const int steps = std::max( 1, int( std::ceil( length / pitch ) ) );
                for( unsigned phase = 0; phase < 3; ++phase )
                {
                    for( int sample = 0; sample <= steps; ++sample )
                    {
                        if( candidates.size() == maximumCandidates )
                        {
                            break;
                        }
                        const float along =
                            std::min( length, ( sample + phase / 3.f ) * length / steps );
                        // The candidate key uses disjoint fields over the bounded
                        // graph domain. Route selection never enters its identity.
                        const std::uint64_t identity =
                            ( std::uint64_t( seed ) << 32 ) | ( std::uint64_t( edgeIndex ) << 17 ) |
                            ( std::uint64_t( side ) << 16 ) | ( std::uint64_t( phase ) << 14 ) |
                            std::uint64_t( sample + 1 );
                        Lot lot{};
                        lot.id = identity + 1;
                        lot.variant = std::uint32_t( hash( lot.id ^ 0xb76a5d29ULL ) );
                        const float streetX = a.x + dx * along, streetZ = a.z + dz * along;
                        const float radius = std::hypot( streetX, streetZ ) / extent;
                        lot.district = radius < .40f ? District::Commercial
                                       : hash( lot.id ^ 0x6469737472696374ULL ) % 13 == 0
                                           ? District::Civic
                                           : District::Residential;
                        const bool commercial = lot.district == District::Commercial;
                        lot.width = lot.parcelWidth = commercial ? 20.f + random( lot.id, 2 ) * 6.f
                                                                 : 12.f + random( lot.id, 2 ) * 6.f;
                        lot.depth = lot.parcelDepth = commercial ? 18.f + random( lot.id, 3 ) * 6.f
                                                                 : 14.f + random( lot.id, 3 ) * 8.f;
                        // Projection of the whole AABB onto the road normal gives
                        // an exact supporting-line setback, including diagonal roads.
                        const float offset =
                            OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f + roadsideSetback +
                            .025f + ( std::abs( nx ) * lot.width + std::abs( nz ) * lot.depth ) * .5f;
                        lot.x = lot.parcelX = streetX + nx * offset;
                        lot.z = lot.parcelZ = streetZ + nz * offset;
                        if( std::abs( lot.x ) + lot.width * .5f > extent - roadSetback ||
                            std::abs( lot.z ) + lot.depth * .5f > extent - roadSetback ||
                            footprintClearance( lot.x, lot.z, lot.width, lot.depth, true,
                                                roadsideSetback ) < roadsideSetback )
                        {
                            continue;
                        }
                        lot.frontage = std::abs( nx ) > std::abs( nz )
                                           ? ( nx > 0 ? Frontage::West : Frontage::East )
                                           : ( nz > 0 ? Frontage::North : Frontage::South );
                        const float floorHeight = commercial ? 3.6f : 3.2f;
                        const float nominalHeight = commercial ? 22.f + random( lot.id, 4 ) * 35.f
                                                    : lot.district == District::Civic
                                                        ? 10.f + random( lot.id, 4 ) * 10.f
                                                        : 8.f + random( lot.id, 4 ) * 14.f;
                        lot.height = floorHeight * std::floor( nominalHeight / floorHeight );
                        lot.park = random( lot.id, 5 ) < ( commercial ? .04f : .10f );
                        candidates.push_back(
                            { lot, hash( lot.id ^ 0x70617263656cULL ), radius < .34f } );
                    }
                }
            }
        }
        packFrontages( candidates, std::size_t( blocks * blocks * 4 ), lots );
    }
}  // namespace workphone::procedural
