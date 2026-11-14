#include <Workphone/WorkphonePCH.hpp>

#include "ProceduralCityConfiguration.hpp"
#include "../RuntimeSceneMembership.hpp"

#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Scene/Components/CityBuildingStyles.hpp>
#include <Workphone/Scene/Components/ProceduralSceneGeneration.hpp>

#include <algorithm>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        constexpr std::array cityBuildingMeshProperties{
            "City Wall Mesh", "City Window Mesh", "City Entrance Mesh", "City Corner Mesh",
            "City Roof Mesh"
        };

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
                    if( auto handle = component->getHandle(); handle && handle->getUUIDAsString() == uuid )
                    {
                        return component;
                    }
                }
            }
            return nullptr;
        }
    }  // namespace

    ProceduralCityConfiguration::ProceduralCityConfiguration( bool alwaysOpen ) :
        m_alwaysOpen( alwaysOpen )
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
                m_terrain = findSceneAnchor<TerrainSystem>( manager,
                                                          m_roadAuthoring->getTerrainBindingUUID() );
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
        assets.cityNaturalRoads = m_naturalRoads;
        assets.cityRoadShowcase = m_roadShowcase;
        assets.cityTerrainRoads = m_terrainRoads;
        assets.cityBlocks = m_blocks;
        assets.cityRoute = m_route;
        assets.cityStreetDetail = m_streetDetail;
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
            throw std::runtime_error( "The terrain city build did not publish its road and terrain anchors." );
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
                             revision != assets.cityAppliedSourceRevision || snapshot != m_builtTerrain ||
                             placement != m_builtTerrainPlacement;
        const bool attempted = revision == m_lastRefreshAttemptRevision && snapshot == m_lastAttemptTerrain &&
                               placement == m_lastAttemptPlacement;
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
        properties.setProperty( "City Natural Roads", m_naturalRoads );
        properties.setProperty( "City Terrain Roads", m_terrainRoads );
        properties.setProperty( "City Street Detail", m_streetDetail );
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
            property.setAttribute( "description", "Choose a mesh asset, or enter a file path. Clear to use the built-in module." );
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
        properties.getPropertyValue( "City Natural Roads", m_naturalRoads );
        bool terrainMode = m_terrainRoads;
        properties.getPropertyValue( "City Terrain Roads", terrainMode );
        if( !terrainMode && m_terrainRoads && ( m_roadAuthoring || !m_roadAuthoringUUID.empty() ) )
        {
            error = "Leaving terrain-city mode while its saved road/terrain anchors are active is "
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
