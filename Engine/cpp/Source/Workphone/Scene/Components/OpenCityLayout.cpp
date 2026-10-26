#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::procedural
{
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
        struct Candidate
        {
            Lot lot;
            std::uint64_t rank;
        };

        SourceBudgetVector<Candidate> candidates;
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
                            roadSetback + .025f +
                            ( std::abs( nx ) * lot.width + std::abs( nz ) * lot.depth ) * .5f;
                        lot.x = lot.parcelX = streetX + nx * offset;
                        lot.z = lot.parcelZ = streetZ + nz * offset;
                        if( std::abs( lot.x ) + lot.width * .5f > extent - roadSetback ||
                            std::abs( lot.z ) + lot.depth * .5f > extent - roadSetback ||
                            footprintRoadClearance( lot.x, lot.z, lot.width, lot.depth ) < roadSetback )
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
                        candidates.push_back( { lot, hash( lot.id ^ 0x70617263656cULL ) } );
                    }
                }
            }
        }
        std::sort( candidates.begin(), candidates.end(), []( const Candidate &a, const Candidate &b ) {
            return a.rank != b.rank ? a.rank < b.rank : a.lot.id < b.lot.id;
        } );
        const std::size_t limit = std::size_t( blocks * blocks * 4 );
        lots.reserve( limit );
        for( const auto &candidate : candidates )
        {
            const auto &lot = candidate.lot;
            bool overlaps = false;
            for( const auto &occupied : lots )
            {
                if( std::abs( lot.x - occupied.x ) < ( lot.width + occupied.width ) * .5f + .75f &&
                    std::abs( lot.z - occupied.z ) < ( lot.depth + occupied.depth ) * .5f + .75f )
                {
                    overlaps = true;
                    break;
                }
            }
            if( overlaps )
            {
                continue;
            }
            lots.push_back( lot );
            if( lots.size() == limit )
            {
                break;
            }
        }
    }
}  // namespace workphone::procedural
