#pragma once

#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>

#include <array>

namespace workphone::scene
{
    class IGameManager;

    namespace race
    {
        struct SceneAssets;
    }

    // City authoring settings and the persistent road source shared by both scene components.
    class ProceduralCityConfiguration
    {
    public:
        explicit ProceduralCityConfiguration( bool alwaysOpen = false );

        bool isOpenCity() const;
        bool isRoadShowcase() const;
        bool usesTerrainRoads() const;

        bool resolveBindings( IGameManager &manager, String &error );
        void applyTo( race::SceneAssets &assets ) const;
        void adoptBuild( const race::SceneAssets &assets );
        void retainBindings( const race::SceneAssets &assets );
        void releaseBindings();
        bool needsRefresh( IGameManager &manager, SmartPtr<IGameActor> owner,
                           const race::SceneAssets &assets );

        void writeProperties( Properties &properties, const race::SceneAssets &assets ) const;
        void readProperties( Properties &properties, String &error );

    private:
        const bool m_alwaysOpen;
        bool m_openCity = false;
        bool m_roadShowcase = false;
        bool m_terrainRoads = false;
        bool m_naturalRoads = false;
        s32 m_blocks = 8;
        s32 m_route = 0;
        s32 m_streetDetail = 100;
        s32 m_buildingStyle = 0;
        std::array<String, 5> m_buildingMeshes;

        String m_roadAuthoringUUID;
        SmartPtr<CityRoadAuthoring> m_roadAuthoring;
        SmartPtr<TerrainSystem> m_terrain;
        u64 m_lastRefreshAttemptRevision = 0;
        render::TerrainSnapshot m_builtTerrain, m_lastAttemptTerrain;
        Transform3<real_Num> m_builtTerrainPlacement, m_lastAttemptPlacement;
    };
}  // namespace workphone::scene
