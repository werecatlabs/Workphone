#ifndef WPTerrainEditing_h__
#define WPTerrainEditing_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <memory>
#include <atomic>
#include <limits>

namespace workphone::scene
{
    class TerrainSystem;

    enum class TerrainBrushMode
    {
        Raise,
        Lower,
        Smooth,
        Flatten,
        Stamp,
        Paint,
        ClearPaint,
        Normalize,
        Hole,
        FillHole,
        Terrace,
        Noise,
        PaintWetness,
        ClearWetness,
        PaintSnow,
        ClearSnow
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
        u32 layer = 0;
        u32 seed = 0;
        f32 terraceStep = 1;
        // Optional normalized grayscale stamp, bilinearly sampled over the brush diameter.
        Vector2I stampDimensions = Vector2I( 0, 0 );
        Array<f32> stamp;
        f32 minHeight = -std::numeric_limits<f32>::max();
        f32 maxHeight = std::numeric_limits<f32>::max();
        f32 minSlope = 0, maxSlope = 90;
    };

    inline constexpr u32 terrainMaximumEditSamples = 65536;

    struct TerrainDirtyRect
    {
        s32 minX = 0, minZ = 0, maxX = -1, maxZ = -1;  // inclusive samples/cells touched
    };

    /** Owner-thread preview stroke. Updates publish real geometry, cancellation restores
     * only the stroke's values, and commit registers exactly one bounded undo item. */
    class WPCore_API TerrainEditStroke
    {
    public:
        TerrainEditStroke( SmartPtr<TerrainSystem> terrain, SmartPtr<ICommandManager> commands );
        ~TerrainEditStroke();
        TerrainEditStroke( const TerrainEditStroke & ) = delete;
        TerrainEditStroke &operator=( const TerrainEditStroke & ) = delete;
        bool update( const TerrainBrush &brush, String &error );
        bool commit( String &error );
        bool cancel( String &error );
        TerrainDirtyRect getDirtyRect() const;

    private:
        struct State;
        std::unique_ptr<State> m_state;
    };

    enum class TerrainHeightEncoding
    {
        Pgm16,
        RawU16LE,
        RawF32LE
    };

    struct TerrainHeightFileOptions
    {
        TerrainHeightEncoding encoding = TerrainHeightEncoding::Pgm16;
        Vector2I dimensions = Vector2I( 0, 0 );    // required for raw input
        f32 minimumHeight = 0, maximumHeight = 1;  // unscaled U16 range, no silent clipping
    };

    WPCore_API bool importTerrainHeightFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                             const TerrainHeightFileOptions &options, String &error );
    WPCore_API bool exportTerrainHeightFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                             const TerrainHeightFileOptions &options, String &error );
    WPCore_API bool exportTerrainMeshFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                           String &error );

    enum class TerrainErosionMode
    {
        Thermal,
        Hydraulic
    };

    struct TerrainErosionRecipe
    {
        TerrainErosionMode mode = TerrainErosionMode::Thermal;
        TerrainBrush region;
        u32 iterations = 16;
        f32 talusAngle = 30, rain = 0.01f, sedimentCapacity = 0.25f;
        u32 seed = 0;
    };

    // Pure bounded preparation may run on a worker. Cancellation/failure leaves output untouched.
    WPCore_API bool prepareTerrainErosion( const render::TerrainData &source,
                                           const TerrainErosionRecipe &recipe,
                                           render::TerrainData &output,
                                           const std::atomic_bool *cancelled, String &error );
    WPCore_API bool applyTerrainErosion( SmartPtr<TerrainSystem> terrain,
                                         const TerrainErosionRecipe &recipe,
                                         SmartPtr<ICommandManager> commands, String &error );

    struct TerrainGradeRecipe
    {
        Array<Vector3F> points;  // local metres; piecewise linear centreline elevation
        f32 width = 4, shoulder = 2, strength = 1, depth = 0;
        bool river = false;  // parabolic cross-section, bed lower by depth
    };

    WPCore_API bool applyTerrainGrade( SmartPtr<TerrainSystem> terrain, const TerrainGradeRecipe &recipe,
                                       SmartPtr<ICommandManager> commands, String &error );

    /** Prepares a bounded before/after patch and executes one undoable command.
     * Undo/redo requires unchanged grid metadata and matching affected values;
     * unrelated edits outside the patch are preserved. Owner-thread operation. */
    WPCore_API bool applyTerrainBrush( SmartPtr<TerrainSystem> terrain, const TerrainBrush &brush,
                                       SmartPtr<ICommandManager> commands, String &error );
    WPCore_API bool applyTerrainAppearance( SmartPtr<TerrainSystem> terrain,
                                            const render::TerrainAppearance &appearance,
                                            SmartPtr<ICommandManager> commands, String &error );
    WPCore_API bool applyTerrainLayer( SmartPtr<TerrainSystem> terrain, u32 index,
                                       const render::TerrainLayer &layer,
                                       SmartPtr<ICommandManager> commands, String &error );
    WPCore_API bool saveTerrainDataFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                         String &error );
    WPCore_API bool loadTerrainDataFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                         String &error );
}  // namespace workphone::scene

#endif
