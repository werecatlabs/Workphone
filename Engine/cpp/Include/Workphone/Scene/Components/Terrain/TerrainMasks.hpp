#ifndef WPTerrainMasks_h__
#define WPTerrainMasks_h__
#include <Workphone/Scene/Components/Terrain/TerrainEditing.hpp>

namespace workphone::scene
{
    /** Normalized PGM16 masks. Rule masks encode height/slope/radius coverage;
     * erosion masks encode absolute predicted displacement divided by its maximum.
     * Exports do not modify the authoring terrain or record recipe-only success.
     */
    WPCore_API bool exportTerrainRuleMask( SmartPtr<TerrainSystem> terrain, const String &path,
                                           const TerrainBrush &rule, String &error );
    WPCore_API bool exportTerrainErosionMask( SmartPtr<TerrainSystem> terrain, const String &path,
                                              const TerrainErosionRecipe &recipe, String &error );
}  // namespace workphone::scene
#endif
