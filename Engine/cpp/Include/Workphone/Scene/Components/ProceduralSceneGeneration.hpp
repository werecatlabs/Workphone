#pragma once
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainFit.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <array>
#include <memory>

namespace workphone::scene::race
{
    class CityStreaming;

    inline constexpr float defaultCityStreamLoadRadius = 300.f;
    inline constexpr float defaultCityStreamRetainRadius = 420.f;
    inline constexpr int minimumCitySourceMemoryMiB = 128;
    inline constexpr int defaultCitySourceMemoryMiB = 768;
    inline constexpr int maximumCitySourceMemoryMiB = 4096;

    // City generation uses the process-wide SourceCpu ledger. A lower request
    // does not shrink an externally configured or previously raised limit.
    WPCore_API bool prepareCitySourceMemoryBudget( int minimumMiB, String &error );

    /**
     * A single sample point along a race circuit.
     *
     * position: world-space position of the sample.
     * right: world-space right direction at the sample (used for vehicle orientation).
     * distance: accumulated distance along the circuit up to this sample.
     */
    struct CircuitSample
    {
        Vector3F position;
        Vector3F right;
        f32 distance = 0.0f;
    };

    /**
     * Represents a procedural race circuit composed of ordered samples.
     *
     * The samples vector contains the track samples in order; length is the total
     * path length computed from the samples. nearest() returns the index of the
     * sample closest to a given world-space position.
     */
    struct WPCore_API Circuit : SourceBudgetObject
    {
        SourceBudgetVector<CircuitSample> samples;
        float length = 0;
        Circuit() = default;
        Circuit( const Circuit & ) = default;
        Circuit( Circuit && ) noexcept = default;
        Circuit &operator=( Circuit && ) noexcept = default;

        Circuit &operator=( const Circuit &other )
        {
            if( this != &other )
            {
                Circuit candidate( other );
                swap( candidate );
            }
            return *this;
        }

        void swap( Circuit &other ) noexcept
        {
            samples.swap( other.samples );
            std::swap( length, other.length );
        }

        /**
         * Find the index of the nearest circuit sample to the provided position.
         *
         * @param position World-space position to query.
         * @return Index into the samples vector of the nearest sample.
         */
        size_t nearest( const Vector3F &position ) const;
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
        int cityBuildingDensity = 100;
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
        SceneAssets() = default;
        SceneAssets( const SceneAssets &other );
        SceneAssets &operator=( const SceneAssets &other );
        SceneAssets( SceneAssets && ) noexcept = default;
        SceneAssets &operator=( SceneAssets && ) noexcept = default;
        void swap( SceneAssets &other ) noexcept;
    };

    /**
     * Generate a deterministic circuit using the provided seed.
     *
     * @param seed Random seed for procedural generation.
     * @return Generated Circuit instance containing samples and length.
     */
    WPCore_API Circuit generateCircuit( u32 seed );

    /**
     * Run any internal validation checks for circuit generation. Intended for
     * debug/assertions; does not return a value.
     */
    WPCore_API void validateCircuit();

    /**
     * Validate that reflection rendering resources are correctly configured for
     * the provided scene assets (e.g. textures and actors present).
     *
     * @param assets Scene assets to validate.
     * @return true if reflection resources are valid and usable.
     */
    WPCore_API bool validateReflection( const SceneAssets &assets );
    /// Stable lot occupancy for 0..100 density; lower values retain central lots first.
    WPCore_API bool selectCityBuilding( u64 lotId, u32 seed, f32 centreDistanceFraction,
                                       s32 densityPercent );
    WPCore_API void applyCityBuildingDensity( procedural::OpenCityLayout &city, u32 seed,
                                              s32 densityPercent );

    /**
     * Build scene assets for a procedural race scene and attach them to the
     * provided vehicle actor. This creates renderable actors, materials,
     * textures and computes derived statistics such as triangle and texture
     * memory usage.
     *
     * @param assets Output container for created scene assets.
     * @param vehicle Vehicle actor to attach generated parts to.
     * @param seed Seed used for deterministic generation.
     * @param quality Visual quality level for generated vehicle appearance.
     * @param citySourceOwner Optional persistent owner of newly authored terrain/roads.
     */
    WPCore_API void buildScene( SceneAssets &assets, SmartPtr<IGameActor> vehicle, u32 seed,
                                procedural::VehicleAppearanceQuality quality,
                                SmartPtr<IGameActor> citySourceOwner = nullptr );

    /**
     * Configure physics actors, colliders and joints for the generated scene
     * and associated vehicle. This function should be called after buildScene()
     * and before stepping physics simulation.
     *
     * @param assets Scene assets containing actors and meshes.
     * @param vehicle Vehicle actor to configure physics for.
     */
    WPCore_API void configurePhysics( const SceneAssets &assets, SmartPtr<IGameActor> vehicle );

    // Independent builders use the owning actor's scene lighting. The race
    // facade composes the same generation stages with its vehicle presentation.
    WPCore_API void buildCircuitScene( SceneAssets &assets, SmartPtr<IGameActor> owner, u32 seed,
                                      procedural::VehicleAppearanceQuality quality );
    WPCore_API void buildCityScene( SceneAssets &assets, SmartPtr<IGameActor> owner, u32 seed,
                                   procedural::VehicleAppearanceQuality quality );

    // May defer while the owner is detached or scene loading is in progress.
    // A nonempty error distinguishes failure from deferred activation.
    WPCore_API bool activateGeneratedScene( const SceneAssets &assets, SmartPtr<IGameActor> owner,
                                           String &error );

    // Release the runtime inventory. Editable city road and terrain anchors
    // belong to their actor hierarchy independently of regeneration.
    WPCore_API void clearScene( SceneAssets &assets, SmartPtr<IGameActor> owner );
}  // namespace workphone::scene::race
