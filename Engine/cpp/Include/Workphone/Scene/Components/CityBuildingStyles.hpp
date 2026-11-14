#pragma once

#include <array>

namespace workphone::scene::race
{
    // Persisted IDs: append new styles so existing scenes retain their appearance.
    inline constexpr std::array<const char *, 10> cityBuildingStyleKeys{
        "district", "masonry", "civic", "modern", "terrace", "parisian", "warehouse",
        "mediterranean", "artdeco", "timber"
    };
    inline constexpr int cityBuildingStyleMax = int( cityBuildingStyleKeys.size() ) - 1;
    // The property enum parser requires a trailing delimiter.
    inline constexpr char cityBuildingStyleLabels[] =
        "District Mix;Masonry Residential;Civic Stone;Modern Office;European Terrace;"
        "Parisian Mansard;Warehouse Loft;Mediterranean;Art Deco;Timber Gabled;";
}
