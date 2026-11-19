#ifndef SceneAssets_h__
#define SceneAssets_h__

#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Procedural/CityRoadTerrainFit.hpp>
#include <Workphone/Procedural/CityTrafficSimulation.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <array>
#include <memory>

namespace workphone::scene::race
{

    inline constexpr float defaultCityStreamLoadRadius = 300.f;
    inline constexpr float defaultCityStreamRetainRadius = 420.f;
    inline constexpr int minimumCitySourceMemoryMiB = 128;
    inline constexpr int defaultCitySourceMemoryMiB = 1024;
    inline constexpr int maximumCitySourceMemoryMiB = 4096;

    class CityStreaming;

    struct CityTrafficActor
    {
        u32 id = 0;
        SmartPtr<IGameActor> actor;
        u32 respawns = 0;
    };

    /**
     * Collection of assets that make up a generated race scene.
     *
     * This structure holds the generated vehicle data, the track circuit and all
     * render / physics actors, materials, textures and supporting scene resources
     * required for rendering and simulation. It is intended to be populated by
     * buildScene() and consumed by the scene component and physics configuration
     * code.
     */
    struct WPCore_API SceneAssets : SourceBudgetObject
    {
        bool openCity = false, useFoliageWorld = false, cityNaturalRoads = false,
             cityRoadShowcase = false, cityTerrainRoads = false;
        bool cityStreamingEnabled = false;
        float cityStreamLoadRadius = defaultCityStreamLoadRadius;
        float cityStreamRetainRadius = defaultCityStreamRetainRadius;
        int citySourceMemoryMiB = defaultCitySourceMemoryMiB;
        std::shared_ptr<CityStreaming> cityStreaming;
        SmartPtr<CityRoadAuthoring> roadAuthoring;
        SmartPtr<TerrainSystem> roadTerrain;
        render::TerrainSnapshot terrainGameplaySource;
        procedural::OpenCityLayout cityLayout;
        u32 cityBuildingSupportChecks = 0, cityCulledParcels = 0;
        bool cityBuildingSupportValid = true, citySourceReused = false;
        float cityTerrainRelief = 1.f;
        bool cityTerrainFitted = false;
        CityRoadTerrainFitReport cityTerrainFitReport;
        bool cityRaceRouteAvailable = true;
        String cityRaceRouteError;
        u64 cityAppliedSourceRevision = 0;
        int cityStreetDetail = 100;
        int cityStreetPropDensity = 150;
        int cityBuildingDensity = 150;
        u32 cityTrafficDensity = 35;
        bool cityTrafficDriveOnLeft = false;
        std::shared_ptr<scene::CityTrafficSimulation> cityTraffic;
        SourceBudgetVector<CityTrafficActor> cityTrafficActors;
        f32 cityTrafficAccumulator = 0;
        u32 cityTrafficLastTick = ~0u;
        bool cityDebugBlocks = false, cityDebugLots = false;
        u32 cityDebugBlockCount = 0, cityDebugLotCount = 0;
        // LOD0->1 and LOD1->2 distances for trees, shrubs and grass (metres).
        std::array<f32, 6> cityFoliageLODDistances = { 65, 260, 20, 100, 8, 35 };
        // Zero follows the appearance-quality default; otherwise pixels per view.
        u32 cityFoliageImpostorResolution = 0;
        u32 cityBuildingImpostorResolution = 0;
        u32 cityStreetDetailCount = 0;
        int cityBuildingStyle = 0;
        std::array<String, 5> cityBuildingMeshes;
        u32 cityBuildingModuleCount = 0;
        std::array<u32, 5> cityBuildingModulePlacements{};
        String cityBuildingKitDiagnostic;
        SmartPtr<render::Terrain> foliageTerrain;
        int cityBlocks = 8, cityRoute = 0;
        procedural::GeneratedVehicle vehicle;
        Circuit circuit;
        std::array<SmartPtr<IGameActor>, 4> wheels;
        SmartPtr<IGameActor> body, shadow, reflectionActor, ground, sceneryRoot;
        SmartPtr<render::ITexture> reflectionTexture;
        SourceBudgetVector<SmartPtr<render::IMaterial>> vehicleMaterials, materials;
        SourceBudgetVector<SmartPtr<render::ITexture>> textures;
        SourceBudgetVector<SmartPtr<IMeshResource>> meshes;
        SourceBudgetVector<SmartPtr<IGameActor>> actors;
        Array<SmartPtr<LODGroup>> treeLODs{ Allocator<SmartPtr<LODGroup>>( sourceBudget ) };
        SmartPtr<LODGroup> vehicleLOD;
        SmartPtr<render::ISky> sky;
        SmartPtr<render::IGraphicsLight> sun;
        String resourcePrefix;
        String exportSourceDirectory;
        u64 triangles = 0, textureBytes = 0;
        SceneAssets();
        SceneAssets( const SceneAssets &other );
        SceneAssets &operator=( const SceneAssets &other );
        SceneAssets( SceneAssets && ) noexcept;
        SceneAssets &operator=( SceneAssets && ) noexcept;
        void swap( SceneAssets &other ) noexcept;
    };

}  // namespace workphone::scene::race

#endif  // SceneAssets_h__
