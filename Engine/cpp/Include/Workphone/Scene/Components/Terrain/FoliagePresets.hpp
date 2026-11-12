#pragma once
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>

namespace workphone::scene
{
    // Values are shared with serialized standalone ProceduralFoliage presets.
    enum class FoliagePreset
    {
        Oak,
        Birch,
        Pine,
        Bush,
        Grass,
        TallGrass,
        Palm
    };
    inline constexpr u32 foliageCutoutSize = 128;

    WPCore_API FoliagePlantRecipe foliagePresetRecipe( FoliagePreset preset );
    WPCore_API bool generateFoliagePreset( FoliagePreset preset, const FoliagePlantRecipe &recipe,
                                           render::FoliageSpecies &output, String &error );
    // BGRA pixels for the existing runtime texture upload API.
    WPCore_API Array<u8> foliagePresetCutout( FoliagePreset preset );
    WPCore_API bool saveFoliagePresetCutout( FoliagePreset preset, const String &projectRoot,
                                             const String &assetDirectory, String &textureId,
                                             String &error );
}  // namespace workphone::scene
