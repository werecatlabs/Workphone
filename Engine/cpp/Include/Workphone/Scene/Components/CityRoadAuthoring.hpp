#pragma once
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Procedural/CityRoadSplineSource.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Math/Ray3.hpp>
#include <functional>
#include <memory>

namespace workphone::scene
{
    class TerrainSystem;
    struct CityRoadRuntimeState;

    /** Scene-owned road source. All editing/publication runs on the owner thread.
     * The first tier stores controls and road widths in world metres; actor pose
     * is not a second source transform. Terrain queries retain one snapshot/pose.
     */
    class WPCore_API CityRoadAuthoring : public Component
    {
    public:
        using PublishCallback = std::function<bool(
            const procedural::CityRoadSplineSource &, const procedural::CityRoadSplineBake &,
            render::TerrainSnapshot, const Transform3<real_Num> &, String & )>;

        CityRoadAuthoring();
        ~CityRoadAuthoring() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        String getSourceJSON() const;
        bool setSourceJSON( const String &json, String &error );
        bool setSourceJSON( const String &json, u64 expectedRevision, String &error );
        const procedural::CityRoadSplineSource &getSource() const;
        const procedural::CityRoadSplineBake &getBake() const;
        u64 getSourceRevision() const;
        u64 getActiveRevision() const;
        const String &getError() const;

        bool bindTerrain( SmartPtr<TerrainSystem> terrain, String &error );
        SmartPtr<TerrainSystem> getTerrain() const;
        String getTerrainBindingUUID() const;
        render::TerrainSnapshot getTerrainSnapshot() const;
        const Transform3<real_Num> &getTerrainPlacement() const;
        bool intersectTerrain( const Ray3F &ray, Vector3F &sourceHit, String &error ) const;
        Vector3F localToWorld( Vector3F sourcePoint ) const;

        bool beginDraft( String &error, u64 expectedRevision = 0 );
        bool hasDraft() const;
        const procedural::CityRoadSplineSource &getDraftSource() const;
        bool addDraftRoad( const procedural::CityRoadSpline &road, String &error );
        bool addDraftKnot( const String &roadId, const procedural::CityRoadSplineKnot &knot,
                           String &error );
        bool moveDraftKnot( const String &roadId, const String &knotId, Vector3F position,
                            Vector3F inHandle, Vector3F outHandle, String &error );
        bool commitDraft( String &error );
        void cancelDraft();

        // Hooks support deterministic headless publication/failure tests. Attached
        // components use the native runtime by default, never a silent CPU-only path.
        void setPublishCallback( PublishCallback callback );
        bool rebuild( String &error );
        bool publishRuntime( const procedural::CityRoadSplineSource &source,
                             const procedural::CityRoadSplineBake &bake,
                             render::TerrainSnapshot terrain,
                             const Transform3<real_Num> &placement, String &error );
        void clearRuntime();
        size_t getRuntimeTriangleCount() const;
        bool getRuntimeSurface( f32 x, f32 z, f32 &height, bool &driveable ) const;
        WP_CLASS_REGISTER_DECL;

    private:
        bool captureTerrain( render::TerrainSnapshot &snapshot,
                             Transform3<real_Num> &placement, u64 &serial, String &error ) const;
        bool applySource( procedural::CityRoadSplineSource candidate, u64 expectedRevision,
                          bool preserveRevision, String &error );
        bool resolveTerrainBinding();

        procedural::CityRoadSplineSource m_source, m_draft;
        procedural::CityRoadSplineBake m_bake;
        SourceBudgetText m_sourceJSON;
        AtomicWeakPtr<TerrainSystem> m_terrain;
        String m_terrainBindingUUID, m_error;
        render::TerrainSnapshot m_terrainSnapshot;
        Transform3<real_Num> m_terrainPlacement;
        std::weak_ptr<const render::TerrainData> m_lastTerrainAttempt;
        Transform3<real_Num> m_lastAttemptPlacement;
        u64 m_lastAttemptSerial = 0, m_lastAttemptSourceRevision = 0;
        u64 m_terrainSerial = 0, m_activeRevision = 0, m_draftRevision = 0;
        bool m_hasDraft = false, m_hasAcceptedSource = false, m_pendingPublication = false;
        PublishCallback m_publishCallback;
        std::shared_ptr<CityRoadRuntimeState> m_runtime;
    };
}
