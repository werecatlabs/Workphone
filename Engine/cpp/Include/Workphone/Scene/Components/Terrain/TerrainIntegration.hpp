#ifndef WPTerrainIntegration_h__
#define WPTerrainIntegration_h__

#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainEditing.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone::scene
{
    class TerrainSystem;

    struct TerrainGroundSeed
    {
        u64 id = 0;  // stable caller-owned identity; unique within a population
        Vector2F localPosition = Vector2F( 0, 0 );
        f32 heightOffset = 0, yaw = 0;  // terrain-local metres/radians
        Vector3F scale = Vector3F( 1, 1, 1 );
        u32 allowedLayers = 15;      // low four bits; combined interpolated layer weight
        f32 minimumLayerWeight = 0;  // zero disables weight filtering
    };

    struct TerrainGroundMasks
    {
        Vector2I dimensions = Vector2I( 0, 0 );
        Array<f32> density;    // optional per-sample [0,1], stable seed-ID acceptance
        Array<u8> exclusions;  // optional per-cell 0/1; one excludes the entire cell
    };

    struct TerrainGroundPlacement
    {
        u64 id = 0;
        Transform3<real_Num> worldTransform;
        Vector3F normal = Vector3F( 0, 1, 0 );
        bool supported = false;  // false outside the finite terrain or over a hole
        u64 sampledRevision = 0;
    };

    /** Terrain-owned placement inputs for existing foliage/props. Seed identities and
     * unaffected placements survive edits. Only cells touching changed height samples
     * or hole cells are sampled again; topology/placement changes resample all seeds.
     * Preparation is transactional and must be published by the caller on its owner thread.
     */
    class WPCore_API TerrainGroundPopulation
    {
    public:
        static constexpr u32 maximumInstances = 65536;
        bool setSeeds( const Array<TerrainGroundSeed> &seeds, String &error );
        bool setMasks( const TerrainGroundMasks &masks, String &error );
        bool update( render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
                     String &error );
        const Array<TerrainGroundPlacement> &getPlacements() const;
        render::TerrainSnapshot getTerrainSnapshot() const;
        u32 getLastResampledCount() const;
        TerrainDirtyRect getLastDirtyCells() const;

    private:
        Array<TerrainGroundSeed> m_seeds;
        Array<TerrainGroundPlacement> m_placements;
        render::TerrainSnapshot m_terrain;
        Transform3<real_Num> m_transform;
        TerrainDirtyRect m_dirty;
        u32 m_resampled = 0;
        std::shared_ptr<const TerrainGroundMasks> m_masks, m_sampledMasks;
    };

    struct TerrainNavigationInput
    {
        render::TerrainSnapshot source;
        Transform3<real_Num> placement;
        Array<Vector3F> positions;
        Array<u32> indices;  // exact full-resolution triangles; holes omitted
        AABB3F worldBounds;
    };

    WPCore_API bool buildTerrainNavigationInput( render::TerrainSnapshot terrain,
                                                 const Transform3<real_Num> &placement,
                                                 TerrainNavigationInput &output, String &error );

    struct TerrainShorelineSegment
    {
        Vector3F start, end;
    };

    struct TerrainShorelineInput
    {
        render::TerrainSnapshot source;
        f32 waterLevel = 0;
        Array<TerrainShorelineSegment> segments;  // world-space intersections, not a water mesh
    };

    WPCore_API bool buildTerrainShorelineInput( render::TerrainSnapshot terrain,
                                                const Transform3<real_Num> &placement,
                                                f32 worldWaterLevel, TerrainShorelineInput &output,
                                                String &error, u32 maximumSegments = 262144 );

    struct TerrainPathPoint
    {
        Vector3F position, normal;
        bool supported = false;
    };

    struct TerrainPathInput
    {
        render::TerrainSnapshot source;
        Array<TerrainPathPoint> points;
    };

    /** Ground-project caller's world-XZ road/river/prop anchors. Unsupported points are
     * explicitly marked; callers must split/reject spans over holes or outside terrain.
     */
    WPCore_API bool sampleTerrainPathInput( render::TerrainSnapshot terrain,
                                            const Transform3<real_Num> &placement,
                                            const Array<Vector3F> &worldPoints, TerrainPathInput &output,
                                            String &error );

    struct TerrainIntegrationOptions
    {
        bool ground = false, navigation = false, shoreline = false, path = false;
        Array<TerrainGroundSeed> seeds;
        TerrainGroundMasks masks;
        Array<Vector3F> pathAnchors;
        f32 waterLevel = 0;
        u32 maximumNavigationTriangles = 524288, maximumShorelineSegments = 262144;
    };

    struct TerrainIntegrationData
    {
        render::TerrainSnapshot source;
        Transform3<real_Num> placement;
        TerrainGroundPopulation ground;
        TerrainNavigationInput navigation;
        TerrainShorelineInput shoreline;
        TerrainPathInput path;
    };

    using TerrainIntegrationSnapshot = std::shared_ptr<const TerrainIntegrationData>;

    /** Owner-thread attachment/configuration; publication automatically refreshes enabled
     * inputs after render/source/physics locks are released. Failed builds retain the last
     * complete bundle. Runtime detach clears it; owner unload ends the subscription.
     * Returned immutable bundles may be retained across edits and binding destruction.
     */
    class WPCore_API TerrainIntegrationBinding : public ISharedObject
    {
    public:
        TerrainIntegrationBinding();
        ~TerrainIntegrationBinding() override;
        bool bind( SmartPtr<TerrainSystem> source, const TerrainIntegrationOptions &options,
                   String &error );
        bool configure( const TerrainIntegrationOptions &options, String &error );
        void unbind();
        TerrainIntegrationSnapshot getSnapshot() const;
        String getLastError() const;
        WP_CLASS_REGISTER_DECL;

    private:
        class Listener;
        struct State;
        std::unique_ptr<State> m_state;
        void published( Listener *listener, render::TerrainSnapshot source,
                        const Transform3<real_Num> &placement, u64 serial, bool detached );
    };

    // Concrete component publication bridge. Called only after every publication lock.
    WPCore_API void notifyTerrainIntegration( TerrainSystem *owner, render::TerrainSnapshot source,
                                              const Transform3<real_Num> &placement, u64 serial,
                                              bool detached = false ) noexcept;
    WPCore_API bool terrainIntegrationIsNotifying( const TerrainSystem *owner );

    WPCore_API bool exportTerrainNavigationFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                                 String &error );
    WPCore_API bool exportTerrainShorelineFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                                f32 worldWaterLevel, String &error );
}  // namespace workphone::scene
#endif
