#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralScene.hpp>
#include <Workphone/Scene/Components/ProceduralCircuitScene.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <stdexcept>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralScene, Component );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralCircuitScene, ProceduralScene );

    ProceduralScene::ProceduralScene() = default;
    ProceduralScene::~ProceduralScene() = default;

    void ProceduralScene::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            return;
        }
        Component::load( data );
        if( auto manager = core::IApplicationManager::instance()->getGameManager() )
        {
            manager->registerComponentUpdate( TaskId::Application, Thread::UpdateState::Update, this );
        }
        setLoadingState( LoadingState::Loaded );
    }

    void ProceduralScene::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }
        if( auto app = core::IApplicationManager::instancePtr() )
        {
            if( auto manager = app->getGameManager() )
            {
                manager->unregisterAllComponent( this );
            }
        }
        clearGeneratedScene();
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void ProceduralScene::update()
    {
        if( Thread::getCurrentTask() != TaskId::Application || !isLoaded() || !isEnabled() ||
            !m_activationPending )
        {
            return;
        }
        auto tasks = core::IApplicationManager::instance()->getTaskManager();
        auto renderLock = tasks ? tasks->lockTask( TaskId::Render ) : TaskLock();
        auto physicsLock = tasks ? tasks->lockTask( TaskId::Physics ) : TaskLock();
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        if( m_activationPending )
        {
            try
            {
                activateGeneratedAssets();
            }
            catch( const std::exception &failure )
            {
                generationFailed();
                clearGeneratedScene();
                m_generationError = failure.what();
            }
        }
    }

    bool ProceduralScene::regenerate()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto tasks = app ? app->getTaskManager() : nullptr;
        auto renderLock = tasks ? tasks->lockTask( TaskId::Render ) : TaskLock();
        auto physicsLock = tasks ? tasks->lockTask( TaskId::Physics ) : TaskLock();
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        auto manager = app ? app->getGameManager() : nullptr;
        auto owner = getActor();
        auto scene = manager ? manager->getCurrentScene() : nullptr;
        if( !owner || !scene )
        {
            m_generationError = "Attach the procedural component to an actor before generating.";
            return false;
        }
        if( owner->getScene() && owner->getScene() != scene )
        {
            m_generationError = "The procedural component belongs to a different scene.";
            return false;
        }
        try
        {
            if( !prepareGeneration() )
            {
                return false;
            }
            clearGeneratedScene();
            m_generationError.clear();
            buildGeneratedScene();
            m_generated = true;
            return true;
        }
        catch( const std::exception &failure )
        {
            generationFailed();
            clearGeneratedScene();
            m_generationError = failure.what();
            return false;
        }
    }

    void ProceduralScene::clearGeneratedScene()
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        m_generated = false;
        m_activationPending = false;
        race::SceneAssets retired;
        m_assets.swap( retired );
        // Release producer storage before callbacks can allocate during retirement.
        retired.vehicle = {};
        retired.circuit = {};
        retired.cityLayout = {};
        generationCleared();
        race::clearScene( retired, getActor() );
    }

    SmartPtr<Properties> ProceduralScene::getProperties() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        auto properties = Component::getProperties();
        properties->setProperty( "Seed", m_seed );
        properties->setProperty( "Appearance Quality", getQuality() );
        properties->setProperty( "Foliage World", m_foliageWorldEnabled );
        properties->setProperty( "Generation Error", m_generationError );
        properties->setButtonPressed( "Regenerate", false );
        return properties;
    }

    void ProceduralScene::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }
        Component::setProperties( properties );
        {
            std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
            properties->getPropertyValue( "Seed", m_seed );
            s32 quality = getQuality();
            properties->getPropertyValue( "Appearance Quality", quality );
            setQuality( quality );
            properties->getPropertyValue( "Foliage World", m_foliageWorldEnabled );
            readGenerationProperties( *properties );
        }
        if( properties->isButtonPressed( "Regenerate" ) )
        {
            regenerate();
        }
    }

    bool ProceduralScene::prepareGeneration() { return true; }
    void ProceduralScene::readGenerationProperties( Properties & ) {}
    void ProceduralScene::generationFailed() {}
    void ProceduralScene::generationCleared() {}

    void ProceduralScene::ensurePhysicsScene()
    {
        auto physics = core::IApplicationManager::instance()->getPhysicsManager();
        if( !physics )
        {
            throw std::runtime_error( "Procedural scene generation requires a physics manager." );
        }
        if( !physics->getPhysicsScene() )
        {
            physics->setPhysicsScene( physics->addScene() );
        }
        if( !physics->getPhysicsScene() )
        {
            throw std::runtime_error( "Cannot create procedural scene physics." );
        }
    }

    void ProceduralScene::activateGeneratedAssets()
    {
        String error;
        m_activationPending = !race::activateGeneratedScene( m_assets, getActor(), error );
        if( !error.empty() )
        {
            throw std::runtime_error( error.c_str() );
        }
    }

    bool ProceduralScene::isGenerated() const { return m_generated; }

    String ProceduralScene::getGenerationError() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        return m_generationError;
    }

    const race::SceneAssets &ProceduralScene::getAssets() const { return m_assets; }
    u32 ProceduralScene::getSeed() const { return m_seed; }
    void ProceduralScene::setSeed( u32 seed ) { m_seed = seed; }
    s32 ProceduralScene::getQuality() const { return static_cast<s32>( m_quality ); }
    void ProceduralScene::setQuality( s32 quality )
    {
        m_quality = static_cast<procedural::VehicleAppearanceQuality>( std::clamp( quality, 0, 3 ) );
    }
    bool ProceduralScene::getFoliageWorldEnabled() const { return m_foliageWorldEnabled; }
    void ProceduralScene::setFoliageWorldEnabled( bool enabled ) { m_foliageWorldEnabled = enabled; }
    String ProceduralScene::getMaterialExportDirectory() const { return m_materialExportDirectory; }
    void ProceduralScene::setMaterialExportDirectory( const String &directory )
    {
        m_materialExportDirectory = directory;
    }

    u32 ProceduralScene::getCircuitSampleCount() const
    {
        return static_cast<u32>( m_assets.circuit.samples.size() );
    }
    f32 ProceduralScene::getCircuitLength() const { return m_assets.circuit.length; }
    u32 ProceduralScene::nearestCircuitSample( const Vector3F &position ) const
    {
        return static_cast<u32>( m_assets.circuit.nearest( position ) );
    }
    Vector3F ProceduralScene::getCircuitPosition( u32 index ) const
    {
        return m_assets.circuit.samples.at( index ).position;
    }
    Vector3F ProceduralScene::getCircuitRight( u32 index ) const
    {
        return m_assets.circuit.samples.at( index ).right;
    }

    void ProceduralCircuitScene::buildGeneratedScene()
    {
        ensurePhysicsScene();
        m_assets.useFoliageWorld = m_foliageWorldEnabled;
        m_assets.exportSourceDirectory = m_materialExportDirectory;
        race::buildCircuitScene( m_assets, getActor(), m_seed, m_quality );
        activateGeneratedAssets();
    }
}  // namespace workphone::scene
