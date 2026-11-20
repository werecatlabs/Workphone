#pragma once

#include <Workphone/Procedural/CityBuildingStyles.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace workphone::scene::procedural_scene::cityBuildingWindowKit
{
    enum class Head { Lintel, Segmental, Pediment, Stepped, Timber };

    struct FacadePreset
    {
        float floorHeight;
        float bayWidth, bayWidthStep;            // Metres
        float windowRatio, windowRatioStep;      // Fraction of a bay
        float shopWindowRatio;
        Head windowHead;
    };

    // Persisted style IDs 1..9. District Mix (0) resolves to one of these
    // before placement. Steps choose among three stable parcel variations.
    inline constexpr std::array facadePresets{
        //            floor   bay    step   window  step   shop   head
        FacadePreset{ 3.2f,   4.4f,  .45f,  .48f,  .05f,  .79f,  Head::Segmental }, // Masonry
        FacadePreset{ 3.5f,   4.6f,  0.f,   .56f,  0.f,   .79f,  Head::Pediment  }, // Civic
        FacadePreset{ 3.6f,   3.8f,  0.f,   .48f,  .05f,  .79f,  Head::Lintel    }, // Modern curtain wall
        FacadePreset{ 3.2f,   4.2f,  0.f,   .48f,  .05f,  .79f,  Head::Lintel    }, // Terrace
        FacadePreset{ 3.5f,   4.2f,  0.f,   .48f,  .05f,  .79f,  Head::Pediment  }, // Parisian
        FacadePreset{ 4.1f,   4.2f,  0.f,   .74f,  0.f,   .74f,  Head::Segmental }, // Warehouse
        FacadePreset{ 3.2f,   4.4f,  .45f,  .48f,  .05f,  .79f,  Head::Lintel    }, // Mediterranean
        FacadePreset{ 3.2f,   4.2f,  0.f,   .52f,  0.f,   .52f,  Head::Stepped   }, // Art Deco
        FacadePreset{ 3.2f,   4.2f,  0.f,   .50f,  0.f,   .50f,  Head::Timber    }  // Timber
    };
    static_assert( facadePresets.size() == cityBuildingStyleMax );

    constexpr const FacadePreset &presetForStyle( int style )
    {
        return facadePresets[std::clamp( style, 1, cityBuildingStyleMax ) - 1];
    }

    struct DoorLeaf
    {
        float width, height;
    };

    // Keep the leaf centred on the entrance landing. Only split off sidelights
    // or a fanlight when there is room for the frame and an actual light.
    constexpr DoorLeaf fitHouseDoor( float openingWidth, float openingHeight )
    {
        const float clearWidth = std::max( 0.f, openingWidth - .12f );
        const float clearHeight = std::max( 0.f, openingHeight - .065f );
        return { clearWidth >= 1.35f ? .96f : clearWidth,
                 clearHeight >= 2.29f ? 2.06f : clearHeight };
    }

    // Each block has a front, soffit, top and two exposed ends, with its
    // rear against the wall. Every treatment includes one projecting sill.
    constexpr size_t vertexCost( Head head )
    {
        switch( head )
        {
        case Head::Segmental: return 6 * 20;
        case Head::Pediment:
        case Head::Stepped:
        case Head::Timber: return 4 * 20;
        default: return 2 * 20;
        }
    }

    struct WindowPlan
    {
        int bays = 0, firstStorey = 0, storeys = 0, selectedColumns = 0, selectedRows = 0;

        bool contains( int bay, int storey ) const
        {
            const auto selected = []( int index, int count, int selectedCount ) {
                if( index < 0 || index >= count || selectedCount == 0 ) return false;
                // Round to the centre of each interval. Columns line up across
                // storeys, even when only a few full rows fit the face budget.
                const int64_t offset = count / 2;
                return ( int64_t( index + 1 ) * selectedCount + offset ) / count !=
                       ( int64_t( index ) * selectedCount + offset ) / count;
            };
            return selected( bay, bays, selectedColumns ) &&
                   selected( storey - firstStorey, storeys, selectedRows );
        }
    };

    // Splitting raw vertices can leave every face just short of one surround.
    // Allocate complete elements, giving the street elevation first choice.
    inline size_t faceWindowSlots( size_t lotSlots, size_t tier, size_t tierCount, int face )
    {
        const size_t tierSlots = lotSlots / tierCount + ( tier < lotSlots % tierCount );
        constexpr std::array<int, 5> order{ 0, 2, 3, 1, 0 }; // front, flanks, rear, front
        const size_t rounds = tierSlots / order.size();
        const auto partialEnd = order.begin() + tierSlots % order.size();
        return rounds * ( face == 0 ? 2 : 1 ) + std::count( order.begin(), partialEnd, face );
    }

    inline WindowPlan planWindows( int bays, int storeys, int firstStorey, size_t slots )
    {
        WindowPlan plan;
        if( bays <= 0 || firstStorey < 0 || firstStorey >= storeys || !slots ) return plan;
        plan.bays = bays;
        plan.firstStorey = firstStorey;
        plan.storeys = storeys - firstStorey;
        plan.selectedColumns = int( std::min( size_t( bays ), slots ) );
        plan.selectedRows = int( std::min( size_t( plan.storeys ), slots / plan.selectedColumns ) );
        return plan;
    }
}  // namespace workphone::scene::procedural_scene::cityBuildingWindowKit
