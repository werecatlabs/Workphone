#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralScene.hpp>
#include <Workphone/Scene/Components/ProceduralCircuitScene.hpp>
#include <Workphone/Scene/Components/ProceduralCityStreaming.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Workphone.hpp>
#include "Workphone/Scene/Components/CityTrafficAppearance.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <filesystem>

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
        if( Thread::getCurrentTask() != TaskId::Application || !isLoaded() || !isEnabled() )
        {
            return;
        }

        auto app = core::IApplicationManager::instancePtr();
        if( !app )
        {
            return;
        }

        if( m_activationPending )
        {
            auto tasks = app->getTaskManager();
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

        if( !m_generated )
        {
            return;
        }

        {
            std::unique_lock<std::recursive_mutex> lock( m_generationMutex, std::try_to_lock );
            if( !lock.owns_lock() || !m_generated || m_activationPending )
            {
                return;
            }
            if( auto traffic = m_assets.cityTraffic )
            {
                // This component is reachable through both its owning actor and
                // its explicit manager registration. Advance only once per task tick.
                const auto tick = app->getTimer()->getTickCount( TaskId::Application );
                if( tick != m_assets.cityTrafficLastTick )
                {
                    m_assets.cityTrafficLastTick = tick;
                    const bool sourceCurrent =
                        !m_assets.roadAuthoring ||
                        ( traffic->sourceRevision() == m_assets.roadAuthoring->getSourceRevision() &&
                          traffic->sourceRevision() == m_assets.roadAuthoring->getActiveRevision() );
                    if( !sourceCurrent )
                    {
                        // An authored edit invalidates cached lanes immediately. The city
                        // component rebuilds the graph and actors on its next refresh.
                        m_assets.cityTrafficAccumulator = 0;
                    }
                    else if( app->isPlaying() && !app->isPaused() )
                    {
                        const auto player = m_assets.body ? getActor() : nullptr;
                        if( player )
                        {
                            traffic->setExternalObstacle( player->getPosition(), 2.2f );
                        }
                        else
                        {
                            traffic->setExternalObstacle( {}, 0, false );
                        }
                        const f32 frame =
                            std::clamp( f32( app->getTimer()->getDeltaTime() ), 0.f, .12f );
                        m_assets.cityTrafficAccumulator =
                            std::min( m_assets.cityTrafficAccumulator + frame, .15f );
                        constexpr f32 fixedStep = 1.f / 30.f;
                        for( u32 step = 0; step < 4 && m_assets.cityTrafficAccumulator >= fixedStep;
                             ++step )
                        {
                            traffic->step( fixedStep );
                            m_assets.cityTrafficAccumulator -= fixedStep;
                        }
                        const auto &poses = traffic->vehicles();
                        const auto count = std::min( poses.size(), m_assets.cityTrafficActors.size() );
                        for( size_t index = 0; index < count; ++index )
                        {
                            auto &entry = m_assets.cityTrafficActors[index];
                            const auto &pose = poses[index];
                            if( !entry.actor || entry.id != pose.id ||
                                pose.forward.lengthSquared() < .0001f )
                            {
                                continue;
                            }
                            entry.actor->setPosition(
                                pose.position +
                                Vector3F( 0, race::cityTrafficAppearance::chassisLift, 0 ) );
                            entry.actor->setOrientation(
                                race::cityTrafficAppearance::roadOrientation( pose.forward ) );
                            entry.actor->updateTransform();
                        }
                    }
                }
            }
        }

        std::shared_ptr<race::CityStreaming> streaming;
        {
            std::unique_lock<std::recursive_mutex> lock( m_generationMutex, std::try_to_lock );
            if( !lock.owns_lock() || !m_generated || m_activationPending )
            {
                return;
            }

            streaming = m_assets.cityStreaming;
            if( !streaming || !streaming->stats().totalPatches )
            {
                return;
            }
        }

        // LOD edits and native render publication use the manager, then graphics owner.
        // Regeneration can already own the generation lock while removing scene objects.
        // If it does, release the publication locks and let it finish before retrying.
        auto manager = app->getGameManager();
        auto graphics = app->getGraphicsSystem();
        if( !manager || !graphics )
        {
            return;
        }

        std::unique_lock<std::recursive_mutex> lock( m_generationMutex, std::try_to_lock );
        if( !lock.owns_lock() || !m_generated || m_activationPending ||
            m_assets.cityStreaming != streaming )
        {
            return;
        }

        try
        {
            String error;
            const auto updated = m_cityStreamingObserverOverride
                                     ? streaming->update( m_cityStreamingObservers, error )
                                     : streaming->updateFromActiveCamera( error );
            if( !updated && !error.empty() )
            {
                m_generationError = "City streaming: " + error;
            }
        }
        catch( const std::bad_alloc & )
        {
            // Preserve current observer demand until the next update can obtain its snapshot.
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
            if( m_assets.openCity && m_assets.cityStreamingEnabled && !m_assets.cityRoadShowcase )
            {
                race::CityStreamingOptions options;
                options.loadRadius = m_assets.cityStreamLoadRadius;
                options.retainRadius = m_assets.cityStreamRetainRadius;
                auto streaming = std::allocate_shared<race::CityStreaming>(
                    SourceBudgetAllocator<race::CityStreaming>{}, options );
                const auto cache = ( std::filesystem::u8path( app->getProjectPath().c_str() ) /
                                     ".FBCache" / "ProceduralCityStreaming" )
                                       .generic_u8string();
                String error;
                if( !streaming->initialize( m_assets, owner, String( cache.c_str() ), error ) )
                {
                    throw std::runtime_error( ( "City streaming: " + error ).c_str() );
                }
                m_assets.cityStreaming = std::move( streaming );
            }
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
        if( retired.cityStreaming )
        {
            retired.cityStreaming->close();
        }
        retired.cityStreaming.reset();
        if( retired.cityTraffic )
        {
            retired.cityTraffic->clear();
        }
        retired.cityTraffic.reset();
        retired.cityTrafficActors.clear();
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

    bool ProceduralScene::prepareGeneration()
    {
        return true;
    }

    void ProceduralScene::readGenerationProperties( Properties & )
    {
    }

    void ProceduralScene::generationFailed()
    {
    }

    void ProceduralScene::generationCleared()
    {
    }

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

    bool ProceduralScene::isGenerated() const
    {
        return m_generated;
    }

    String ProceduralScene::getGenerationError() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        return m_generationError;
    }

    const race::SceneAssets &ProceduralScene::getAssets() const
    {
        return m_assets;
    }

    void ProceduralScene::setCityStreamingObservers( const Array<Vector3F> &observers )
    {
        if( observers.empty() || observers.size() > 16 ||
            std::any_of( observers.begin(), observers.end(),
                         []( const auto &p ) { return !p.isFinite(); } ) )
        {
            throw std::invalid_argument( "City streaming requires one to sixteen finite observers." );
        }
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        auto admitted = makeSourceBudgetSnapshot( observers );
        m_cityStreamingObservers.swap( admitted );
        m_cityStreamingObserverOverride = true;
    }

    void ProceduralScene::clearCityStreamingObservers()
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        m_cityStreamingObservers.clear();
        m_cityStreamingObserverOverride = false;
    }

    u32 ProceduralScene::getSeed() const
    {
        return m_seed;
    }

    void ProceduralScene::setSeed( u32 seed )
    {
        m_seed = seed;
    }

    s32 ProceduralScene::getQuality() const
    {
        return static_cast<s32>( m_quality );
    }

    void ProceduralScene::setQuality( s32 quality )
    {
        m_quality = static_cast<procedural::VehicleAppearanceQuality>( std::clamp( quality, 0, 3 ) );
    }

    bool ProceduralScene::getFoliageWorldEnabled() const
    {
        return m_foliageWorldEnabled;
    }

    void ProceduralScene::setFoliageWorldEnabled( bool enabled )
    {
        m_foliageWorldEnabled = enabled;
    }

    String ProceduralScene::getMaterialExportDirectory() const
    {
        return m_materialExportDirectory;
    }

    void ProceduralScene::setMaterialExportDirectory( const String &directory )
    {
        m_materialExportDirectory = directory;
    }

    u32 ProceduralScene::getCircuitSampleCount() const
    {
        return static_cast<u32>( m_assets.circuit.samples.size() );
    }

    f32 ProceduralScene::getCircuitLength() const
    {
        return m_assets.circuit.length;
    }

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
