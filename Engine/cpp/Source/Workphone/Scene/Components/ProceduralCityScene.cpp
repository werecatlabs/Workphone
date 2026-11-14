#include <Workphone/WorkphonePCH.hpp>

#include <Workphone/Scene/Components/ProceduralCityScene.hpp>

#include "ProceduralCityConfiguration.hpp"

#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralCityScene, ProceduralScene );

    ProceduralCityScene::ProceduralCityScene() :
        m_city( std::make_unique<ProceduralCityConfiguration>( true ) )
    {
    }

    ProceduralCityScene::~ProceduralCityScene() = default;

    void ProceduralCityScene::unload( SmartPtr<ISharedObject> data )
    {
        ProceduralScene::unload( data );
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        m_city->releaseBindings();
    }

    void ProceduralCityScene::update()
    {
        ProceduralScene::update();
        if( Thread::getCurrentTask() != TaskId::Application || !isLoaded() || !isEnabled() )
        {
            return;
        }

        // Road publication can acquire task locks. Refresh on the application
        // task before taking the generation mutex in regenerate().
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        if( manager && m_city->needsRefresh( *manager, getActor(), m_assets ) )
        {
            regenerate();
        }
    }

    SmartPtr<Properties> ProceduralCityScene::getProperties() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        auto properties = ProceduralScene::getProperties();
        m_city->writeProperties( *properties, m_assets );
        return properties;
    }

    void ProceduralCityScene::readGenerationProperties( Properties &properties )
    {
        m_city->readProperties( properties, m_generationError );
    }

    bool ProceduralCityScene::prepareGeneration()
    {
        if( m_city->usesTerrainRoads() && !m_materialExportDirectory.empty() )
        {
            m_generationError = "Terrain-city source material export is unavailable.";
            return false;
        }
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        if( !manager )
        {
            m_generationError = "Procedural city generation requires a scene manager.";
            return false;
        }
        if( !m_city->resolveBindings( *manager, m_generationError ) )
        {
            clearGeneratedScene();
            return false;
        }
        return true;
    }

    void ProceduralCityScene::buildGeneratedScene()
    {
        ensurePhysicsScene();
        m_city->applyTo( m_assets );
        m_assets.useFoliageWorld = m_foliageWorldEnabled;
        m_assets.exportSourceDirectory = m_materialExportDirectory;
        race::buildCityScene( m_assets, getActor(), m_seed, m_quality );
        m_city->adoptBuild( m_assets );
        activateGeneratedAssets();
        if( !m_assets.cityRaceRouteAvailable )
        {
            m_generationError = m_assets.cityRaceRouteError;
        }
    }

    void ProceduralCityScene::generationFailed()
    {
        m_city->retainBindings( m_assets );
    }
}  // namespace workphone::scene
