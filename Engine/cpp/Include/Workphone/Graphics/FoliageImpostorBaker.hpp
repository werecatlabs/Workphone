#ifndef WPFoliageImpostorBaker_h__
#define WPFoliageImpostorBaker_h__
#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <functional>
#include <atomic>
namespace workphone::render
{
    using FoliageBakeTextureResolver =
        std::function<std::shared_ptr<const TextureMipLevel>( const String &, String & )>;
    WPCore_API void setFoliageImpostorTextureResolver( FoliageBakeTextureResolver resolver );
    WPCore_API FoliageBakeTextureResolver getFoliageImpostorTextureResolver();
    struct FoliageImpostorBakeOptions
    {
        u32 tileSize = 128, gutter = 4;
        Array<Vector3F> directions = { { 0, 0, 1 }, { 1, 0, 0 }, { 0, 0, -1 }, { -1, 0, 0 } };
        String resourcePrefix = "data://foliage/impostor";
        u64 maximumRasterSamples = 128ull * 1024 * 1024;
    };
    struct FoliageImpostorBake
    {
        FoliageImpostor metadata;
        // BGRA bytes; albedo sRGB cutout, normal capture-tangent, linear ORM and depth.
        // Projection is a common sphere square centred at the species bounds midpoint.
        std::vector<TextureMipLevel> albedo, normal, orm, depth;
        Vector3F centre;
        f32 radius = 0;
        u64 rasterSamples = 0;
    };
    WPCore_API bool bakeFoliageImpostor( const FoliageSpecies &, const FoliageBakeTextureResolver &,
                                         const FoliageImpostorBakeOptions &, FoliageImpostorBake &output,
                                         String &error, const std::atomic<bool> *cancel = nullptr );
    /** Writes four TGA sources and matching .texres descriptors, then publishes the
     * .folspec last. destinationDirectory must be a fresh directory, avoiding partial
     * replacement of existing catalog assets. Texture IDs use options.resourcePrefix.
     */
    WPCore_API bool saveFoliageImpostorSource( const FoliageSpecies &, const FoliageImpostorBake &,
                                               const String &destinationDirectory, String &speciesPath,
                                               String &error );
}  // namespace workphone::render
#endif
