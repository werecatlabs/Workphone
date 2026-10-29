#pragma once

#if WP_TERRAIN_GAMEPLAY_PACKAGE
#    include <Workphone/Graphics/TerrainData.hpp>
#    include <WPGraphics/Resources/TerrainResource.hpp>
#    include <Workphone/System/ResourceSystem.hpp>
#    include <filesystem>
#    include <memory>

namespace workphone::advanced::terrain_gameplay
{
    /** Representative bounded terrain. Map IDs already identify the cooked closure;
     * exporting never changes the source that was installed into the live scene. */
    render::TerrainData makeSource();

    /** Export a fresh, source-free install closure. The vehicle uses the existing
     * buildCar generation recipe; its actual generated content hash is mandatory.
     * Failed authoring evidence is retained beside the requested destination. */
    void exportPackage( const std::filesystem::path &destination, const render::TerrainData &,
                        u32 seed, int quality, u64 vehicleHash );

    struct Package
    {
        std::shared_ptr<resource::ResourceSystem> runtime;
        std::shared_ptr<const render::DecodedCookedTerrain> decoded;
        u32 seed = 0;
        int quality = 0;
        u64 vehicleHash = 0;
    };

    /** Read-only mount and CPU decode. The caller reconstructs the existing
     * procedural vehicle, then checks its content hash before starting gameplay. */
    std::unique_ptr<Package> loadPackage( const std::filesystem::path &directory );
}
#endif
