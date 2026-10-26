#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <Workphone/Interface/Procedural/OpenCityRoadNetwork.hpp>

namespace workphone::procedural
{
    // CPU layout shared by city rendering, road queries and race routes. No engine
    // singleton is required, so tools can preview and validate the same layout.
    struct OpenCityLayout : SourceBudgetObject
    {
        struct Point
        {
            float x, z;
        };
        enum class District : std::uint8_t
        {
            Residential,
            Commercial,
            Civic
        };
        // World-space street frontage: North is -Z, East is +X.
        enum class Frontage : std::uint8_t
        {
            West,
            East,
            North,
            South
        };

        struct Lot
        {
            float x, z, width, depth, height;
            bool park;
            std::uint64_t id = 0;
            std::uint32_t variant = 0;
            District district = District::Residential;
            Frontage frontage = Frontage::North;
            float parcelX = 0, parcelZ = 0, parcelWidth = 0, parcelDepth = 0;
            float groundHeight = 0;  // Level foundation datum; legacy layouts retain zero.
            float groundMinimumHeight = 0;
        };

        struct BuildingVolume
        {
            float x, z, width, depth, bottom, height;
        };

        static constexpr float roadHalfWidth = 6.f;
        static constexpr std::uint32_t generatorRevision = 2;
        static constexpr float roadSetback = 11.f;
        static constexpr float serviceGap = 4.f;
        int blocks = 0;
        float spacing = 0, extent = 0;
        bool naturalRoads = false;
        OpenCityRoadNetwork roads;
        SourceBudgetVector<Lot> lots;
        SourceBudgetVector<Point> route;
        OpenCityLayout() = default;
        OpenCityLayout( const OpenCityLayout & ) = default;
        OpenCityLayout( OpenCityLayout && ) noexcept = default;
        OpenCityLayout &operator=( OpenCityLayout && ) noexcept = default;

        OpenCityLayout &operator=( const OpenCityLayout &other )
        {
            if( this != &other )
            {
                OpenCityLayout candidate( other );
                swap( candidate );
            }
            return *this;
        }

        void swap( OpenCityLayout &other ) noexcept
        {
            using std::swap;
            swap( blocks, other.blocks );
            swap( spacing, other.spacing );
            swap( extent, other.extent );
            swap( naturalRoads, other.naturalRoads );
            roads.swap( other.roads );
            lots.swap( other.lots );
            route.swap( other.route );
        }

        static OpenCityLayout generate( std::uint32_t seed, int blocks = 8, int route = 0,
                                        bool naturalRoads = false )
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
            city.lots.reserve( std::size_t( city.blocks * city.blocks * 4 ) );
            for( int x = 0; x < city.blocks; ++x )
            {
                for( int z = 0; z < city.blocks; ++z )
                {
                    for( int a = 0; a < 2; ++a )
                    {
                        for( int b = 0; b < 2; ++b )
                        {
                            const int cellX = x - city.blocks / 2, cellZ = z - city.blocks / 2;
                            // The bounded signed-cell domain needs 8 bits per axis. This
                            // identity is injective for every supported seed/cell/quadrant
                            // and remains stable when city bounds or route selection change.
                            const std::uint64_t id =
                                ( ( std::uint64_t( seed ) << 18 ) |
                                  ( std::uint64_t( std::uint32_t( cellX ) & 255u ) << 10 ) |
                                  ( std::uint64_t( std::uint32_t( cellZ ) & 255u ) << 2 ) |
                                  std::uint64_t( a * 2 + b ) ) +
                                1;
                            const auto block =
                                hash( ( std::uint64_t( seed ) << 32 ) ^ std::uint32_t( cellX ) ^
                                      ( std::uint64_t( std::uint32_t( cellZ ) ) << 16 ) );
                            const float cellRadius = std::hypot( cellX + .5f, cellZ + .5f );
                            const float density = std::exp( -cellRadius * .55f );
                            const auto district = cellRadius < 1.5f || ( std::abs( cellZ + .5f ) < .6f &&
                                                                         std::abs( cellX + .5f ) < 3.2f )
                                                      ? District::Commercial
                                                  : block % 11 == 0 ? District::Civic
                                                                    : District::Residential;
                            const float minX =
                                cellX * city.spacing +
                                ( a ? city.spacing * .5f + serviceGap * .5f : roadSetback );
                            const float maxX =
                                cellX * city.spacing + ( a ? city.spacing - roadSetback
                                                           : city.spacing * .5f - serviceGap * .5f );
                            const float minZ =
                                cellZ * city.spacing +
                                ( b ? city.spacing * .5f + serviceGap * .5f : roadSetback );
                            const float maxZ =
                                cellZ * city.spacing + ( b ? city.spacing - roadSetback
                                                           : city.spacing * .5f - serviceGap * .5f );
                            Lot lot{};
                            lot.id = id;
                            lot.variant = std::uint32_t( hash( id ^ 0xb76a5d29ULL ) );
                            lot.district = district;
                            lot.frontage = random( id, 1 ) < .5f
                                               ? ( a ? Frontage::East : Frontage::West )
                                               : ( b ? Frontage::South : Frontage::North );
                            lot.parcelX = ( minX + maxX ) * .5f;
                            lot.parcelZ = ( minZ + maxZ ) * .5f;
                            lot.parcelWidth = maxX - minX;
                            lot.parcelDepth = maxZ - minZ;
                            lot.width = lot.parcelWidth * ( .72f + .23f * random( id, 2 ) );
                            lot.depth = lot.parcelDepth * ( .68f + .27f * random( id, 3 ) );
                            lot.x = lot.frontage == Frontage::West   ? minX + lot.width * .5f
                                    : lot.frontage == Frontage::East ? maxX - lot.width * .5f
                                                                     : lot.parcelX;
                            lot.z = lot.frontage == Frontage::North   ? minZ + lot.depth * .5f
                                    : lot.frontage == Frontage::South ? maxZ - lot.depth * .5f
                                                                      : lot.parcelZ;
                            const float nominalHeight =
                                district == District::Commercial
                                    ? 20 + random( id, 4 ) * ( 18 + density * 34 )
                                : district == District::Civic
                                    ? 10 + random( id, 4 ) * 14
                                    : 8 + random( id, 4 ) * ( 11 + density * 10 );
                            const float floorHeight = district == District::Commercial ? 3.6f : 3.2f;
                            lot.height =
                                std::max( floorHeight * 2,
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
                            city.lots.push_back( lot );
                        }
                    }
                }
            }
            const float radius = ( city.blocks / 2 - 2 + std::clamp( route, 0, 2 ) ) * city.spacing;
            // Start at the origin facing -Z, matching the existing vehicle reset.
            const Point corners[] = { { 0, 0 },           { 0, -radius }, { radius, -radius },
                                      { radius, radius }, { 0, radius },  { 0, 0 } };
            for( int i = 0; i < 5; ++i )
            {
                const auto a = corners[i], b = corners[i + 1];
                const int steps = int( std::ceil( std::hypot( b.x - a.x, b.z - a.z ) / 2 ) );
                for( int j = 0; j < steps; ++j )
                {
                    const float t = float( j ) / steps;
                    city.route.push_back( { a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t } );
                }
            }
            return city;
        }

        // The scene uses these same occupied volumes for facade geometry and
        // collision, so a tower setback never leaves an invisible full-lot wall.
        static SourceBudgetVector<BuildingVolume> buildingVolumes( const Lot &lot )
        {
            SourceBudgetVector<BuildingVolume> result;
            if( lot.park || !std::isfinite( lot.x ) || !std::isfinite( lot.z ) ||
                !std::isfinite( lot.width ) || !std::isfinite( lot.depth ) ||
                !std::isfinite( lot.height ) || lot.width <= 0 || lot.depth <= 0 || lot.height <= 0 ||
                !std::isfinite( lot.groundHeight ) )
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
            const float width = lot.width * ( .68f + .10f * random( lot.variant, 6 ) );
            const float depth = lot.depth * ( .66f + .12f * random( lot.variant, 7 ) );
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

        float roadDistance( float x, float z ) const
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
                    const auto &a = roads.nodes[edge.a], &b = roads.nodes[edge.b];
                    distance = std::min( distance, pointSegmentDistance( x, z, a.x, a.z, b.x, b.z ) );
                }
                return distance;
            }
            return std::min( std::abs( x - std::round( x / spacing ) * spacing ),
                             std::abs( z - std::round( z / spacing ) * spacing ) );
        }

        // Exact minimum distance from the entire axis-aligned footprint to road
        // centrelines. Checking only corners misses a road crossing a large parcel.
        float footprintRoadClearance( float x, float z, float width, float depth ) const
        {
            if( !std::isfinite( x ) || !std::isfinite( z ) || !std::isfinite( width ) ||
                !std::isfinite( depth ) || width < 0 || depth < 0 || spacing <= 0 )
            {
                return 0;
            }
            const float x0 = x - width * .5f, x1 = x + width * .5f;
            const float z0 = z - depth * .5f, z1 = z + depth * .5f;
            float distance = 1000;
            const auto segment = [&]( float ax, float az, float bx, float bz ) {
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
                    distance = 0;
                    return;
                }
                const auto toBox = [&]( float px, float pz ) {
                    return std::hypot( std::max( { x0 - px, 0.f, px - x1 } ),
                                       std::max( { z0 - pz, 0.f, pz - z1 } ) );
                };
                distance = std::min( { distance, toBox( ax, az ), toBox( bx, bz ),
                                       pointSegmentDistance( x0, z0, ax, az, bx, bz ),
                                       pointSegmentDistance( x1, z0, ax, az, bx, bz ),
                                       pointSegmentDistance( x0, z1, ax, az, bx, bz ),
                                       pointSegmentDistance( x1, z1, ax, az, bx, bz ) } );
            };
            if( naturalRoads )
            {
                for( const auto &edge : roads.edges )
                {
                    const auto &a = roads.nodes[edge.a], &b = roads.nodes[edge.b];
                    segment( a.x, a.z, b.x, b.z );
                }
            }
            else
            {
                for( int i = -blocks / 2; i <= blocks / 2; ++i )
                {
                    segment( i * spacing, -extent, i * spacing, extent );
                    segment( -extent, i * spacing, extent, i * spacing );
                }
            }
            return distance;
        }

    private:
        static float pointSegmentDistance( float x, float z, float ax, float az, float bx, float bz )
        {
            const float dx = bx - ax, dz = bz - az, l2 = dx * dx + dz * dz;
            const float t =
                l2 > 1e-8f ? std::clamp( ( ( x - ax ) * dx + ( z - az ) * dz ) / l2, 0.f, 1.f ) : 0;
            return std::hypot( x - ax - t * dx, z - az - t * dz );
        }

        WPCore_API void generateNatural( std::uint32_t seed, int routeChoice );

        static std::uint64_t hash( std::uint64_t value )
        {
            value ^= value >> 30;
            value *= 0xbf58476d1ce4e5b9ULL;
            value ^= value >> 27;
            value *= 0x94d049bb133111ebULL;
            return value ^ ( value >> 31 );
        }

        static float random( std::uint64_t identity, std::uint64_t stream )
        {
            return float( hash( identity ^ ( stream * 0x9e3779b97f4a7c15ULL ) ) >> 40 ) / 16777216.f;
        }
    };
}  // namespace workphone::procedural
