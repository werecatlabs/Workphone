#pragma once
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace workphone::procedural
{
    // Shared render/collision descriptors for the flat arterial grid showcase.
    // General road topology remains the responsibility of IRoadSystem.
    struct OpenCityStreetDetails : SourceBudgetObject
    {
        static constexpr float roadHalfWidth = OpenCityLayout::roadHalfWidth;
        static constexpr float sidewalkWidth = 2.5f;
        static constexpr float sidewalkTop = .15f;
        static constexpr float cornerExtent = roadHalfWidth + sidewalkWidth;

        struct Corner
        {
            float x = 0, z = 0, width = 0, depth = 0, bottom = 0, top = sidewalkTop;
        };
        enum class FurnitureKind : std::uint8_t
        {
            Lamp,
            Bollard
        };

        struct Furniture
        {
            FurnitureKind kind = FurnitureKind::Lamp;
            // yaw points the lamp arm toward the road as (sin(yaw), cos(yaw)).
            float x = 0, z = 0, yaw = 0, baseY = sidewalkTop;
            std::uint32_t seed = 0;
            std::uint64_t id = 0;
        };

        SourceBudgetVector<Corner> corners, sidewalks;
        SourceBudgetVector<Furniture> furniture;

        OpenCityStreetDetails() = default;
        OpenCityStreetDetails( const OpenCityStreetDetails & ) = default;
        OpenCityStreetDetails( OpenCityStreetDetails && ) noexcept = default;
        OpenCityStreetDetails &operator=( OpenCityStreetDetails && ) noexcept = default;

        OpenCityStreetDetails &operator=( const OpenCityStreetDetails &other )
        {
            if( this != &other )
            {
                OpenCityStreetDetails candidate( other );
                swap( candidate );
            }
            return *this;
        }

        void swap( OpenCityStreetDetails &other ) noexcept
        {
            corners.swap( other.corners );
            sidewalks.swap( other.sidewalks );
            furniture.swap( other.furniture );
        }

        static void validateGrid( const OpenCityLayout &city )
        {
            if( city.blocks < 6 || city.blocks > 10 || city.blocks % 2 != 0 ||
                !std::isfinite( city.spacing ) || city.spacing < 64 || city.spacing > 128 ||
                !std::isfinite( city.extent ) ||
                std::abs( city.extent - city.blocks * city.spacing * .5f ) > .001f )
            {
                throw std::invalid_argument( "Street details require a finite bounded open-city grid." );
            }
        }

        // Interior walks, corners and the block plaza form one solid surface.
        // Merge that union without spanning a carriageway. Outside the city,
        // retain the original pieces so dead-end road mouths remain open.
        SourceBudgetVector<Corner> collisionSurfaces( const OpenCityLayout &city ) const
        {
            validateGrid( city );
            if( city.naturalRoads )
            {
                return {};
            }
            const size_t n = size_t( city.blocks );
            if( corners.size() != ( n + 1 ) * ( n + 1 ) * 4 || sidewalks.size() != 4 * n * ( n + 1 ) )
            {
                throw std::invalid_argument(
                    "Collision surfaces require the complete matching street grid." );
            }
            SourceBudgetVector<Corner> result;
            result.reserve( n * n + 12 * n + 4 );
            const int half = city.blocks / 2;
            const float width = city.spacing - 2 * roadHalfWidth;
            for( int x = -half; x < half; ++x )
            {
                for( int z = -half; z < half; ++z )
                {
                    result.push_back( { ( x + .5f ) * city.spacing, ( z + .5f ) * city.spacing, width,
                                        width, 0, sidewalkTop } );
                }
            }
            const auto appendOutside = [&]( const auto &pieces ) {
                for( const auto &piece : pieces )
                {
                    if( std::abs( piece.x ) > city.extent || std::abs( piece.z ) > city.extent )
                    {
                        result.push_back( piece );
                    }
                }
            };
            appendOutside( sidewalks );
            appendOutside( corners );
            return result;
        }

        static OpenCityStreetDetails generate( const OpenCityLayout &city, std::uint32_t seed,
                                               bool fineDetails = true )
        {
            validateGrid( city );
            if( city.naturalRoads )
            {
                return {};
            }
            OpenCityStreetDetails result;
            const int half = city.blocks / 2;
            const auto junctions = size_t( city.blocks + 1 ) * size_t( city.blocks + 1 );
            const auto segments = size_t( 2 ) * city.blocks * ( city.blocks + 1 );
            result.corners.reserve( junctions * 4 );
            result.sidewalks.reserve( segments * 2 );
            result.furniture.reserve( segments * ( fineDetails ? 4 : 2 ) );
            for( int row = -half; row <= half; ++row )
            {
                for( int col = -half; col <= half; ++col )
                {
                    for( int sx : { -1, 1 } )
                    {
                        for( int sz : { -1, 1 } )
                        {
                            result.corners.push_back(
                                { row * city.spacing + sx * ( roadHalfWidth + sidewalkWidth * .5f ),
                                  col * city.spacing + sz * ( roadHalfWidth + sidewalkWidth * .5f ),
                                  sidewalkWidth, sidewalkWidth, 0, sidewalkTop } );
                        }
                    }
                }
            }
            const auto hash = []( std::uint32_t value ) {
                value ^= value >> 16;
                value *= 0x7feb352du;
                value ^= value >> 15;
                value *= 0x846ca68bu;
                return value ^ ( value >> 16 );
            };
            constexpr float pi = 3.14159265358979323846f;
            for( int axis = 0; axis < 2; ++axis )
            {
                for( int row = -half; row <= half; ++row )
                {
                    for( int col = -half; col < half; ++col )
                    {
                        const float along = ( col + .5f ) * city.spacing;
                        const float line = row * city.spacing;
                        for( int sideIndex = 0; sideIndex < 2; ++sideIndex )
                        {
                            const int side = sideIndex ? 1 : -1;
                            const float pavementOffset = side * ( roadHalfWidth + sidewalkWidth * .5f );
                            // Corners own the 2.5m square at each junction. Trimming to
                            // 8.5m prevents overlapping perpendicular sidewalk slabs.
                            result.sidewalks.push_back(
                                axis ? Corner{ line + pavementOffset, along, sidewalkWidth,
                                               city.spacing - 2 * cornerExtent, 0, sidewalkTop }
                                     : Corner{ along, line + pavementOffset,
                                               city.spacing - 2 * cornerExtent, sidewalkWidth, 0,
                                               sidewalkTop } );
                            // Signed grid coordinates, axis and side form stable keys;
                            // detail tiers do not reshuffle the retained lamps.
                            const auto key = std::uint32_t( row + 16 ) << 10 |
                                             std::uint32_t( col + 16 ) << 4 |
                                             std::uint32_t( axis ) << 3 | std::uint32_t( sideIndex );
                            const auto lampSeed = hash( seed ^ key ^ 0xa341316cu );
                            const float jitter = ( float( lampSeed >> 8 ) / 16777216.f - .5f ) * 8;
                            const float lampAlong =
                                ( col + ( sideIndex ? .68f : .32f ) ) * city.spacing + jitter;
                            const float offset = side * 7.7f;
                            const float yaw =
                                axis ? ( side > 0 ? -pi * .5f : pi * .5f ) : ( side > 0 ? pi : 0 );
                            result.furniture.push_back(
                                { FurnitureKind::Lamp, axis ? line + offset : lampAlong,
                                  axis ? lampAlong : line + offset, yaw, sidewalkTop, lampSeed,
                                  ( std::uint64_t( seed ) << 32 ) | key } );
                            if( fineDetails )
                            {
                                const float bollardAlong =
                                    col * city.spacing + ( sideIndex ? city.spacing - 16 : 16 );
                                const auto bollardKey = key | 4u;
                                result.furniture.push_back(
                                    { FurnitureKind::Bollard, axis ? line + offset : bollardAlong,
                                      axis ? bollardAlong : line + offset, yaw, sidewalkTop,
                                      hash( seed ^ bollardKey ^ 0xc8013ea4u ),
                                      ( std::uint64_t( seed ) << 32 ) | bollardKey } );
                            }
                        }
                    }
                }
            }
            return result;
        }
    };
}  // namespace workphone::procedural
