#ifndef ProgressiveMeshOptions_h__
#define ProgressiveMeshOptions_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    enum class MeshLodGenerationMode : u8
    {
        Disabled,
        Automatic,
        Custom,
        Manual
    };

    enum class MeshLodPreset : u8
    {
        Performance,
        Balanced,
        Quality,
        Custom
    };

    enum class MeshLodTransitionMode : u8
    {
        ScreenHeight,
        ScreenArea,
        Distance
    };

    enum class MeshLodReductionMode : u8
    {
        Percentage,
        FixedVertexReduction,
        ErrorThreshold
    };

    enum class MeshSimplificationMetric : u8
    {
        Curvature,
        QuadricError
    };

    /**
     * @brief One generated or artist-authored progressive-mesh level.
     *
     * `remainingGeometry` is the retained fraction. For example, 0.25 keeps approximately
     * 25 percent of the source geometry.
     */
    struct WPCore_API ProgressiveMeshLodLevel
    {
        bool enabled = true;
        f32 screenHeight = 0.5f;
        f32 screenArea = 0.2f;
        f32 distance = 10.0f;
        MeshLodReductionMode reductionMode = MeshLodReductionMode::Percentage;
        f32 remainingGeometry = 0.5f;
        u32 verticesToRemove = 0;
        f32 maximumError = 0.0f;
        String manualMesh;

        bool operator==( const ProgressiveMeshLodLevel &other ) const;
        bool operator!=( const ProgressiveMeshLodLevel &other ) const;
    };

    /**
     * @brief Backend-neutral progressive-mesh import and runtime options.
     *
     * Asset import directors provide defaults and each Mesh component stores its own copy, allowing
     * an imported instance to be tuned without modifying other users of the source asset.
     */
    struct WPCore_API ProgressiveMeshOptions
    {
        MeshLodGenerationMode generationMode = MeshLodGenerationMode::Disabled;
        MeshLodPreset preset = MeshLodPreset::Balanced;
        MeshLodTransitionMode transitionMode = MeshLodTransitionMode::Distance;
        MeshSimplificationMetric simplificationMetric = MeshSimplificationMetric::QuadricError;

        Array<ProgressiveMeshLodLevel> levels;

        u32 minimumSourceTriangleCount = 500;
        u32 minimumLodTriangleCount = 64;

        bool preserveBorders = true;
        bool preserveUvSeams = true;
        bool preserveHardEdges = true;
        bool preserveMaterialBoundaries = true;
        bool preserveSkinning = true;

        f32 borderImportance = 1.0f;
        f32 uvImportance = 1.0f;
        f32 normalImportance = 1.0f;
        f32 skinningImportance = 1.0f;
        f32 hardEdgeAngleDegrees = 60.0f;

        bool useVertexNormals = true;
        bool compressIndexBuffers = true;

        bool optimiseHiddenInterior = false;
        f32 outsideImportance = 0.0f;
        f32 outsideWalkAngleDegrees = 90.0f;

        bool generateShadowLods = true;
        s32 shadowLodOffset = 1;
        bool recalculateBounds = true;

        bool halfPosition = false;
        bool halfTexCoords = false;
        bool qTangents = true;
        bool halfPoseData = true;

        bool isEnabled() const;
        Array<ProgressiveMeshLodLevel> getResolvedLevels() const;
        bool validate( String *errorMessage = nullptr ) const;
        u64 getHash() const;

        SmartPtr<Properties> toProperties( const String &name ) const;
        void fromProperties( SmartPtr<Properties> properties );

        bool operator==( const ProgressiveMeshOptions &other ) const;
        bool operator!=( const ProgressiveMeshOptions &other ) const;

        static Array<ProgressiveMeshLodLevel> getPresetLevels( MeshLodPreset preset );
    };
}  // namespace workphone

#endif  // ProgressiveMeshOptions_h__
