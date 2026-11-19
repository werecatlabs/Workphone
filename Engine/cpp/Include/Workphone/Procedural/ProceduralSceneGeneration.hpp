#pragma once
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainFit.hpp>
#include <Workphone/Scene/Components/CityTrafficSimulation.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <Workphone\Procedural\Circuit.hpp>
#include <Workphone\Procedural\SceneAssets.hpp>
#include <array>
#include <memory>

namespace workphone::scene::race
{

    // City generation uses the process-wide SourceCpu ledger. A lower request
    // does not shrink an externally configured or previously raised limit.
    WPCore_API bool prepareCitySourceMemoryBudget( int minimumMiB, String &error );

    /**
     * Validate that reflection rendering resources are correctly configured for
     * the provided scene assets (e.g. textures and actors present).
     *
     * @param assets Scene assets to validate.
     * @return true if reflection resources are valid and usable.
     */
    WPCore_API bool validateReflection( const SceneAssets &assets );
    /// Stable lot selection for 0..100 density; lower values retain central lots first.
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
