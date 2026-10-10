#ifndef WPTerrainEditing_h__
#define WPTerrainEditing_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone::scene
{
    class TerrainSystem;

    enum class TerrainBrushMode
    {
        Raise,
        Lower,
        Smooth,
        Flatten
    };

    /** One completed action/stroke in terrain-local metres. Strength is a local
     * height delta for raise/lower and a [0,1] blend for smooth/flatten. */
    struct TerrainBrush
    {
        TerrainBrushMode mode = TerrainBrushMode::Raise;
        Vector2F centre = Vector2F( 0, 0 );
        f32 radius = 10;
        f32 strength = 1;
        f32 targetHeight = 0;
    };

    inline constexpr u32 terrainMaximumEditSamples = 65536;

    /** Prepares a bounded before/after patch and executes one undoable command.
     * Undo/redo requires unchanged grid metadata and matching affected values;
     * unrelated edits outside the patch are preserved. Owner-thread operation. */
    WPCore_API bool applyTerrainBrush( SmartPtr<TerrainSystem> terrain, const TerrainBrush &brush,
                                       SmartPtr<ICommandManager> commands, String &error );
    WPCore_API bool saveTerrainDataFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                         String &error );
    WPCore_API bool loadTerrainDataFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                         String &error );
}  // namespace workphone::scene

#endif
