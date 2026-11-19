#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Scene/Components/ProceduralCityConfiguration.hpp"
#include "Workphone/Scene/RuntimeSceneMembership.hpp"
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Scene/Components/CityBuildingStyles.hpp>
#include <Workphone/Procedural/ProceduralSceneGeneration.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        constexpr std::array cityBuildingMeshProperties{ "City Wall Mesh", "City Window Mesh",
                                                         "City Entrance Mesh", "City Corner Mesh",
                                                         "City Roof Mesh" };
        constexpr std::array cityFoliageDistanceProperties{
            "City Tree LOD0 Distance",  "City Tree LOD1 Distance",  "City Shrub LOD0 Distance",
            "City Shrub LOD1 Distance", "City Grass LOD0 Distance", "City Grass LOD1 Distance"
        };
        constexpr std::array<u32, 4> cityImpostorResolutions{ 0, 64, 128, 256 };
        constexpr char cityImpostorResolutionLabels[] = "Auto;64 px;128 px;256 px;";

        bool inCurrentScene( SmartPtr<IGameActor> actor, const IGameScene *scene )
        {
            return scene && actor && actor->getScenePtr() == scene &&
                   detail::runtimeActorPublished( actor );
        }

        template <class T>
        SmartPtr<T> findSceneAnchor( IGameManager &manager, const String &uuid )
        {
            if( uuid.empty() )
            {
                return nullptr;
            }
            const auto scene = manager.getCurrentScene();
            for( const auto &actor : manager.getActors() )
            {
                if( !inCurrentScene( actor, scene.get() ) )
                {
                    continue;
                }
                if( auto component = actor->getComponent<T>() )
                {
                    if( auto handle = component->getHandle();
                        handle && handle->getUUIDAsString() == uuid )
                    {
                        return component;
                    }
                }
            }
            return nullptr;
        }
    }  // namespace

    bool race::prepareCitySourceMemoryBudget( int minimumMiB, String &error )
    {
        if( minimumMiB < minimumCitySourceMemoryMiB || minimumMiB > maximumCitySourceMemoryMiB )
        {
            error = "City Source Memory MiB must be between 128 and 4096.";
            return false;
        }
        try
        {
            WorldStreamingScheduler::raiseSharedSourceBudget( u64( minimumMiB ) << 20 );
            error.clear();
            return true;
        }
        catch( const std::exception &failure )
        {
            error = String( "Cannot raise the shared source memory budget for this city: " ) +
                    failure.what();
            return false;
        }
    }

    ProceduralCityConfiguration::ProceduralCityConfiguration( bool alwaysOpen ) :
        m_alwaysOpen( alwaysOpen ),
        m_streamLoadRadius( race::defaultCityStreamLoadRadius ),
        m_streamRetainRadius( race::defaultCityStreamRetainRadius ),
        m_sourceMemoryMiB( race::defaultCitySourceMemoryMiB )
    {
    }

    bool ProceduralCityConfiguration::isOpenCity() const
    {
        return m_alwaysOpen || m_openCity;
    }

    bool ProceduralCityConfiguration::isRoadShowcase() const
    {
        return m_roadShowcase;
    }

    bool ProceduralCityConfiguration::usesTerrainRoads() const
    {
        return m_terrainRoads;
    }

    bool ProceduralCityConfiguration::resolveBindings( IGameManager &manager, String &error )
    {
        if( isOpenCity() && !race::prepareCitySourceMemoryBudget( m_sourceMemoryMiB, error ) )
        {
            return false;
        }
        if( !m_terrainRoads )
        {
            return true;
        }

        const auto scene = manager.getCurrentScene();
        if( m_terrain && !inCurrentScene( m_terrain->getActor(), scene.get() ) )
        {
            m_terrain = nullptr;
        }
        if( m_roadAuthoring && !inCurrentScene( m_roadAuthoring->getActor(), scene.get() ) )
        {
            m_roadAuthoring = nullptr;
            m_terrain = nullptr;
        }
        if( !m_roadAuthoring && !m_roadAuthoringUUID.empty() )
        {
            m_roadAuthoring = findSceneAnchor<CityRoadAuthoring>( manager, m_roadAuthoringUUID );
            if( !m_roadAuthoring )
            {
                error = "The saved city road source is unavailable in this scene.";
                return false;
            }
        }
        if( m_roadAuthoring )
        {
            // The road source is canonical. The previous build cannot reverse a user's rebind.
            m_terrain = m_roadAuthoring->getTerrain();
            if( !m_terrain || !inCurrentScene( m_terrain->getActor(), scene.get() ) )
            {
                m_terrain =
                    findSceneAnchor<TerrainSystem>( manager, m_roadAuthoring->getTerrainBindingUUID() );
            }
            if( !m_terrain )
            {
                error = "The saved city terrain binding is unavailable.";
                return false;
            }
        }
        return true;
    }

    void ProceduralCityConfiguration::applyTo( race::SceneAssets &assets ) const
    {
        assets.openCity = isOpenCity();
        assets.cityStreamingEnabled = assets.openCity && m_streamingEnabled;
        assets.cityStreamLoadRadius = m_streamLoadRadius;
        assets.cityStreamRetainRadius = m_streamRetainRadius;
        assets.citySourceMemoryMiB = m_sourceMemoryMiB;
        assets.cityNaturalRoads = m_naturalRoads;
        assets.cityRoadShowcase = m_roadShowcase;
        assets.cityTerrainRoads = m_terrainRoads;
        assets.cityBlocks = m_blocks;
        assets.cityRoute = m_route;
        assets.cityStreetDetail = m_streetDetail;
        assets.cityStreetPropDensity = m_streetPropDensity;
        assets.cityBuildingDensity = m_buildingDensity;
        assets.cityTrafficDensity = u32( m_trafficDensity );
        assets.cityTrafficDriveOnLeft = m_trafficDriveOnLeft;
        assets.cityDebugBlocks = m_debugBlocks;
        assets.cityDebugLots = m_debugLots;
        assets.cityFoliageLODDistances = m_foliageLODDistances;
        assets.cityFoliageImpostorResolution =
            cityImpostorResolutions[size_t( m_foliageImpostorResolution )];
        assets.cityBuildingImpostorResolution =
            cityImpostorResolutions[size_t( m_buildingImpostorResolution )];
        assets.cityTerrainRelief = m_terrainRelief;
        assets.cityBuildingStyle = m_buildingStyle;
        assets.cityBuildingMeshes = m_buildingMeshes;
        if( m_terrainRoads )
        {
            assets.roadAuthoring = m_roadAuthoring;
            assets.roadTerrain = m_terrain;
        }
    }

    void ProceduralCityConfiguration::adoptBuild( const race::SceneAssets &assets )
    {
        if( !m_terrainRoads )
        {
            return;
        }
        retainBindings( assets );
        if( !assets.roadAuthoring || !assets.roadTerrain || m_roadAuthoringUUID.empty() )
        {
            throw std::runtime_error(
                "The terrain city build did not publish its road and terrain anchors." );
        }
        m_lastRefreshAttemptRevision = assets.cityAppliedSourceRevision;
        m_builtTerrain = m_roadAuthoring->getTerrainSnapshot();
        m_builtTerrainPlacement = m_roadAuthoring->getTerrainPlacement();
        m_lastAttemptTerrain = m_builtTerrain;
        m_lastAttemptPlacement = m_builtTerrainPlacement;
    }

    void ProceduralCityConfiguration::retainBindings( const race::SceneAssets &assets )
    {
        if( !m_terrainRoads )
        {
            return;
        }
        if( assets.roadTerrain )
        {
            m_terrain = assets.roadTerrain;
        }
        if( assets.roadAuthoring )
        {
            m_roadAuthoring = assets.roadAuthoring;
            m_terrain = assets.roadTerrain;
            if( auto handle = m_roadAuthoring->getHandle() )
            {
                m_roadAuthoringUUID = handle->getUUIDAsString();
            }
        }
    }

    void ProceduralCityConfiguration::releaseBindings()
    {
        m_roadAuthoring = nullptr;
        m_terrain = nullptr;
        m_builtTerrain.reset();
        m_lastAttemptTerrain.reset();
        m_lastRefreshAttemptRevision = 0;
    }

    bool ProceduralCityConfiguration::needsRefresh( IGameManager &manager, SmartPtr<IGameActor> owner,
                                                    const race::SceneAssets &assets )
    {
        if( !m_terrainRoads )
        {
            return false;
        }
        const auto scene = manager.getCurrentScene();
        if( m_roadAuthoring && !inCurrentScene( m_roadAuthoring->getActor(), scene.get() ) )
        {
            // Let detached source actors remove their runtime collision before retiring our scenery.
            if( auto sourceOwner = m_roadAuthoring->getActor();
                sourceOwner && !detail::runtimeActorPublished( sourceOwner ) )
            {
                m_roadAuthoring->update();
            }
            return true;
        }
        if( !m_roadAuthoring && owner && owner->getScene() == scene )
        {
            m_roadAuthoring = findSceneAnchor<CityRoadAuthoring>( manager, m_roadAuthoringUUID );
        }
        if( !m_roadAuthoring )
        {
            return assets.roadAuthoring || !assets.actors.empty();
        }

        const auto revision = m_roadAuthoring->getActiveRevision();
        const auto snapshot = m_roadAuthoring->getTerrainSnapshot();
        const auto placement = m_roadAuthoring->getTerrainPlacement();
        const bool changed = assets.roadAuthoring != m_roadAuthoring ||
                             revision != assets.cityAppliedSourceRevision ||
                             snapshot != m_builtTerrain || placement != m_builtTerrainPlacement;
        const bool attempted = revision == m_lastRefreshAttemptRevision &&
                               snapshot == m_lastAttemptTerrain && placement == m_lastAttemptPlacement;
        if( changed && !attempted )
        {
            m_lastRefreshAttemptRevision = revision;
            m_lastAttemptTerrain = snapshot;
            m_lastAttemptPlacement = placement;
            return true;
        }
        return false;
    }

    void ProceduralCityConfiguration::writeProperties( Properties &properties,
                                                       const race::SceneAssets &assets ) const
    {
        if( !m_alwaysOpen )
        {
            properties.setProperty( "Open City", m_openCity );
            properties.setProperty( "Road Authoring Showcase", m_roadShowcase );
        }
        properties.setProperty( "City Blocks", m_blocks );
        properties.setProperty( "City Route", m_route );
        properties.setProperty( "City Streaming", m_streamingEnabled );
        properties.setProperty( "City Stream Load Radius", m_streamLoadRadius );
        properties.setProperty( "City Stream Retain Radius", m_streamRetainRadius );
        properties.getPropertyObject( "City Stream Load Radius" )
            .setAttribute( "description", "Load detailed city patches within this distance in metres." );
        properties.getPropertyObject( "City Stream Retain Radius" )
            .setAttribute(
                "description",
                "Keep detailed patches until this distance in metres. Use at least the load radius." );
        properties.setProperty( "City Source Memory MiB", m_sourceMemoryMiB );
        auto &sourceMemory = properties.getPropertyObject( "City Source Memory MiB" );
        sourceMemory.setAttribute( "min", StringUtil::toString( race::minimumCitySourceMemoryMiB ) );
        sourceMemory.setAttribute( "max", StringUtil::toString( race::maximumCitySourceMemoryMiB ) );
        sourceMemory.setAttribute( "description",
                                   "Minimum shared CPU source memory available to the city." );
        properties.setProperty( "City Natural Roads", m_naturalRoads );
        properties.setProperty( "City Terrain Roads", m_terrainRoads );
        properties.setProperty( "City Terrain Relief", m_terrainRelief );
        properties.getPropertyObject( "City Terrain Relief" )
            .setAttribute(
                "description",
                "Initial terrain hills (0 to 2). Edit existing ground with the Terrain tools." );
        properties.setProperty( "City Street Detail", m_streetDetail );
        properties.setProperty( "City Street Prop Density", m_streetPropDensity );
        auto &propDensity = properties.getPropertyObject( "City Street Prop Density" );
        propDensity.setAttribute( "min", "0" );
        propDensity.setAttribute( "max", "200" );
        propDensity.setAttribute(
            "description",
            "0 removes optional street furniture; 100 keeps the previous spacing and count; 200 doubles "
            "the target count where the pavement has safe space." );
        properties.setProperty( "City Building Density", m_buildingDensity );
        auto &density = properties.getPropertyObject( "City Building Density" );
        density.setAttribute( "min", "0" );
        density.setAttribute( "max", "200" );
        density.setAttribute( "description",
                              "100 keeps the existing city. Lower values landscape vacant lots; higher "
                              "values split suitable street-facing parcels for extra buildings." );
        properties.setProperty( "City Traffic Density", m_trafficDensity );
        auto &trafficDensity = properties.getPropertyObject( "City Traffic Density" );
        trafficDensity.setAttribute( "min", "0" );
        trafficDensity.setAttribute( "max", "100" );
        trafficDensity.setAttribute( "description",
                                     "Share of the bounded city traffic fleet to spawn. Zero disables "
                                     "traffic. Press Regenerate to apply." );
        properties.setProperty( "Traffic Drive On Left", m_trafficDriveOnLeft );
        properties.getPropertyObject( "Traffic Drive On Left" )
            .setAttribute( "description",
                           "Use the left lane for city traffic. Press Regenerate to apply." );
        properties.setProperty( "City Debug Blocks", m_debugBlocks );
        properties.getPropertyObject( "City Debug Blocks" )
            .setAttribute( "description",
                           "Show generated road-enclosed blocks, or planning cells on a grid city. "
                           "Press Regenerate." );
        properties.setProperty( "City Debug Lots", m_debugLots );
        properties.getPropertyObject( "City Debug Lots" )
            .setAttribute(
                "description",
                "Show parcel boundaries and occupied building footprints. Press Regenerate." );
        properties.setProperty( "City Debug Block Count", assets.cityDebugBlockCount, true );
        properties.setProperty( "City Debug Lot Count", assets.cityDebugLotCount, true );
        for( size_t i = 0; i < cityFoliageDistanceProperties.size(); ++i )
        {
            const auto name = cityFoliageDistanceProperties[i];
            properties.setProperty( name, m_foliageLODDistances[i] );
            auto &property = properties.getPropertyObject( name );
            property.setAttribute( "min", "1" );
            property.setAttribute( "max", "2000" );
            property.setAttribute( "description",
                                   i % 2 == 0
                                       ? "Metres from the active camera where detailed foliage starts "
                                         "blending to reduced geometry."
                                       : "Metres from the active camera where reduced geometry starts "
                                         "blending to the far LOD. Must exceed LOD0 distance." );
        }
        const auto resolutionProperty = [&]( const char *name, s32 value, const char *description ) {
            properties.setProperty( name, value );
            auto &property = properties.getPropertyObject( name );
            property.setTypeName( Properties::enumTypeStr );
            property.setAttribute( Properties::enumTypeStr, cityImpostorResolutionLabels );
            property.setAttribute( "description", description );
        };
        resolutionProperty( "City Foliage Impostor Resolution", m_foliageImpostorResolution,
                            "Pixels per directional foliage sprite. Auto uses 64 px in Preview or 128 "
                            "px otherwise. Higher sizes require more shared CPU and GPU memory." );
        resolutionProperty( "City Building Impostor Resolution", m_buildingImpostorResolution,
                            "Pixels per directional building sprite. Auto uses 64 px in Preview or 128 "
                            "px otherwise. 256 px may require a larger City Source Memory MiB budget." );
        properties.setProperty( "City Building Style", m_buildingStyle );
        auto &buildingStyle = properties.getPropertyObject( "City Building Style" );
        buildingStyle.setTypeName( Properties::enumTypeStr );
        buildingStyle.setAttribute( Properties::enumTypeStr, race::cityBuildingStyleLabels );
        for( size_t i = 0; i < cityBuildingMeshProperties.size(); ++i )
        {
            const auto name = cityBuildingMeshProperties[i];
            properties.setProperty( name, m_buildingMeshes[i], Properties::resourceStr );
            auto &property = properties.getPropertyObject( name );
            property.setAttribute( Properties::resourceTypeStr, "Mesh" );
            property.setAttribute( "allowResourcePath", "true" );
            property.setAttribute(
                "description",
                "Choose a mesh asset, or enter a file path. Clear to use the built-in module." );
        }
        properties.setProperty( "City Building Kit Status", assets.cityBuildingKitDiagnostic, true );
        properties.setProperty( "City Road Authoring UUID", m_roadAuthoringUUID );
    }

    void ProceduralCityConfiguration::readProperties( Properties &properties, String &error )
    {
        if( !m_alwaysOpen )
        {
            properties.getPropertyValue( "Open City", m_openCity );
            properties.getPropertyValue( "Road Authoring Showcase", m_roadShowcase );
        }
        properties.getPropertyValue( "City Blocks", m_blocks );
        properties.getPropertyValue( "City Route", m_route );
        properties.getPropertyValue( "City Streaming", m_streamingEnabled );
        float loadRadius = m_streamLoadRadius, retainRadius = m_streamRetainRadius;
        const bool hasLoadRadius = properties.getPropertyValue( "City Stream Load Radius", loadRadius );
        const bool hasRetainRadius =
            properties.getPropertyValue( "City Stream Retain Radius", retainRadius );
        if( hasLoadRadius || hasRetainRadius )
        {
            if( std::isfinite( loadRadius ) && std::isfinite( retainRadius ) && loadRadius > 0 &&
                retainRadius >= loadRadius )
            {
                m_streamLoadRadius = loadRadius;
                m_streamRetainRadius = retainRadius;
            }
            else if( error.empty() )
            {
                error =
                    "City streaming requires a positive load radius and a finite retain radius at least "
                    "as large.";
            }
        }
        s32 sourceMemory = m_sourceMemoryMiB;
        if( properties.getPropertyValue( "City Source Memory MiB", sourceMemory ) )
        {
            m_sourceMemoryMiB = std::clamp( sourceMemory, race::minimumCitySourceMemoryMiB,
                                            race::maximumCitySourceMemoryMiB );
        }
        properties.getPropertyValue( "City Natural Roads", m_naturalRoads );
        f32 terrainRelief = m_terrainRelief;
        if( properties.getPropertyValue( "City Terrain Relief", terrainRelief ) &&
            std::isfinite( terrainRelief ) )
        {
            m_terrainRelief = std::clamp( terrainRelief, 0.f, 2.f );
        }
        bool terrainMode = m_terrainRoads;
        properties.getPropertyValue( "City Terrain Roads", terrainMode );
        if( !terrainMode && m_terrainRoads && ( m_roadAuthoring || !m_roadAuthoringUUID.empty() ) )
        {
            error =
                "Leaving terrain-city mode while its saved road/terrain anchors are active is "
                "unavailable; use a separate scene for the flat city.";
        }
        else
        {
            m_terrainRoads = terrainMode;
        }
        String binding = m_roadAuthoringUUID;
        properties.getPropertyValue( "City Road Authoring UUID", binding );
        if( binding != m_roadAuthoringUUID )
        {
            m_roadAuthoringUUID = binding;
            releaseBindings();
        }
        if( m_terrainRoads )
        {
            m_openCity = true;
        }
        m_blocks = std::clamp( m_blocks / 2 * 2, 6, 10 );
        m_route = std::clamp( m_route, 0, 2 );
        s32 streetDetail = m_streetDetail;
        if( properties.getPropertyValue( "City Street Detail", streetDetail ) )
        {
            m_streetDetail = std::clamp( streetDetail, 0, 100 );
        }
        s32 streetPropDensity = m_streetPropDensity;
        if( properties.getPropertyValue( "City Street Prop Density", streetPropDensity ) )
        {
            if( streetPropDensity >= 0 && streetPropDensity <= 200 )
            {
                m_streetPropDensity = streetPropDensity;
            }
            else if( error.empty() )
            {
                error = "City Street Prop Density must be between 0 and 200.";
            }
        }
        s32 buildingDensity = m_buildingDensity;
        if( properties.getPropertyValue( "City Building Density", buildingDensity ) )
        {
            if( buildingDensity >= 0 && buildingDensity <= 200 )
            {
                m_buildingDensity = buildingDensity;
            }
            else if( error.empty() )
            {
                error = "City Building Density must be between 0 and 200.";
            }
        }
        s32 trafficDensity = m_trafficDensity;
        if( properties.getPropertyValue( "City Traffic Density", trafficDensity ) )
        {
            if( trafficDensity >= 0 && trafficDensity <= 100 )
            {
                m_trafficDensity = trafficDensity;
            }
            else if( error.empty() )
            {
                error = "City Traffic Density must be between 0 and 100.";
            }
        }
        properties.getPropertyValue( "Traffic Drive On Left", m_trafficDriveOnLeft );
        properties.getPropertyValue( "City Debug Blocks", m_debugBlocks );
        properties.getPropertyValue( "City Debug Lots", m_debugLots );
        auto distances = m_foliageLODDistances;
        bool changedDistances = false;
        for( size_t i = 0; i < cityFoliageDistanceProperties.size(); ++i )
        {
            changedDistances |=
                properties.getPropertyValue( cityFoliageDistanceProperties[i], distances[i] );
        }
        if( changedDistances )
        {
            bool valid = true;
            for( size_t i = 0; i < distances.size(); i += 2 )
            {
                valid &= std::isfinite( distances[i] ) && std::isfinite( distances[i + 1] ) &&
                         distances[i] >= 1 && distances[i + 1] > distances[i] &&
                         distances[i + 1] <= 2000;
            }
            if( valid )
            {
                m_foliageLODDistances = distances;
            }
            else if( error.empty() )
            {
                error =
                    "City foliage LOD distances must be finite metres between 1 and 2000, with LOD1 "
                    "farther than LOD0.";
            }
        }
        const auto readResolution = [&]( const char *name, s32 &current ) {
            s32 selected = current;
            if( !properties.getPropertyValue( name, selected ) )
            {
                return;
            }
            if( selected >= 0 && selected < s32( cityImpostorResolutions.size() ) )
            {
                current = selected;
            }
            else if( error.empty() )
            {
                error = String( name ) + " must be Auto, 64, 128 or 256 px.";
            }
        };
        readResolution( "City Foliage Impostor Resolution", m_foliageImpostorResolution );
        readResolution( "City Building Impostor Resolution", m_buildingImpostorResolution );
        s32 buildingStyle = m_buildingStyle;
        if( properties.getPropertyValue( "City Building Style", buildingStyle ) )
        {
            m_buildingStyle = std::clamp( buildingStyle, 0, race::cityBuildingStyleMax );
        }
        for( size_t i = 0; i < cityBuildingMeshProperties.size(); ++i )
        {
            String path;
            if( properties.getPropertyValue( cityBuildingMeshProperties[i], path ) )
            {
                m_buildingMeshes[i] = StringUtil::trim( path );
            }
        }
    }
}  // namespace workphone::scene
