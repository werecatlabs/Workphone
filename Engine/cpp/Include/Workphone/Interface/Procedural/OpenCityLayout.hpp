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
        static constexpr std::uint32_t generatorRevision = 5;
        static constexpr float roadSetback = 11.f;
        static constexpr float roadsideSetback = roadSetback - roadHalfWidth;
        static constexpr float serviceGap = 4.f;
        // Clear gap kept between neighbouring parcel envelopes for pedestrians/services.
        static constexpr float parcelSeparation = .75f;
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

        // Every data member must be listed here.
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

        WPCore_API static OpenCityLayout generate( std::uint32_t seed, int blocks = 8, int route = 0,
                                                   bool naturalRoads = false );

        // True when two parcel envelopes are closer than parcelSeparation on both axes.
        static bool parcelsOverlap( const Lot &a, const Lot &b )
        {
            return std::abs( a.parcelX - b.parcelX ) <
                       ( a.parcelWidth + b.parcelWidth ) * .5f + parcelSeparation &&
                   std::abs( a.parcelZ - b.parcelZ ) <
                       ( a.parcelDepth + b.parcelDepth ) * .5f + parcelSeparation;
        }

        // The scene uses these same occupied volumes for facade geometry and
        // collision, so a tower setback never leaves an invisible full-lot wall.
        WPCore_API static SourceBudgetVector<BuildingVolume> buildingVolumes( const Lot &lot );

        WPCore_API float roadDistance( float x, float z ) const;

        // Exact minimum distance from the entire axis-aligned footprint to road
        // centrelines. Checking only corners misses a road crossing a large parcel.
        WPCore_API float footprintRoadClearance( float x, float z, float width, float depth ) const;

        // Natural streets have different widths. Reserve the same pedestrian /
        // frontage envelope beyond their actual carriageway, not a 12m road at
        // every address. The centreline query above keeps its existing meaning.
        WPCore_API float footprintRoadEdgeClearance( float x, float z, float width, float depth ) const;

    private:
        WPCore_API float footprintClearance( float x, float z, float width, float depth, bool fromEdge,
                                             float upperBound = 1000.f ) const;
        WPCore_API void generateGridLots( std::uint32_t seed );
        WPCore_API void generateNatural( std::uint32_t seed, int routeChoice );
    };
}  // namespace workphone::procedural
