#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/RaycastHit.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include "ProceduralCityConfiguration.hpp"
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Terrain/CollisionTerrain.hpp>
#include <Workphone/Scene/Components/CityRoadAuthoring.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralRaceScene, ProceduralScene );
    ProceduralRaceScene::ProceduralRaceScene() : m_city( std::make_unique<ProceduralCityConfiguration>() )
    {
    }

    ProceduralRaceScene::~ProceduralRaceScene() = default;

    void ProceduralRaceScene::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            return;
        }

        ProceduralScene::load( data );

        auto manager = core::IApplicationManager::instance()->getGameManager();
        manager->registerComponentUpdate( TaskId::Physics, Thread::UpdateState::Update, this );
        manager->registerComponentUpdate( TaskId::Physics, Thread::UpdateState::PostUpdate, this );
        manager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Update, this );
    }

    void ProceduralRaceScene::unload( SmartPtr<ISharedObject> data )
    {
        ProceduralScene::unload( data );
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        m_city->releaseBindings();
        m_citySourceOwner = nullptr;
        m_terrainGameplaySource.reset();
    }

    bool ProceduralRaceScene::prepareGeneration()
    {
        if( m_city->usesTerrainRoads() && m_city->isRoadShowcase() )
        {
            m_generationError =
                "Choose a generated terrain city or the separate road authoring showcase.";
            return false;
        }
        if( m_terrainGameplaySource &&
            ( m_city->isOpenCity() || m_city->isRoadShowcase() || m_city->usesTerrainRoads() ||
              m_foliageWorldEnabled || !m_materialExportDirectory.empty() ) )
        {
            m_generationError =
                "Terrain gameplay requires its own ground; disable city, foliage-world and source "
                "material export modes.";
            return false;
        }
        if( m_city->usesTerrainRoads() && !m_materialExportDirectory.empty() )
        {
            m_generationError = "Terrain-city source material export is unavailable.";
            return false;
        }
        auto manager = core::IApplicationManager::instance()->getGameManager();
        if( !m_city->resolveBindings( *manager, m_generationError ) )
        {
            clearGeneratedScene();
            return false;
        }
        return true;
    }

    void ProceduralRaceScene::buildGeneratedScene()
    {
        auto app = core::IApplicationManager::instance();
        auto actor = getActor();
        ensurePhysicsScene();
        app->getPhysicsManager()->getPhysicsScene()->setGravity( Vector3F( 0, -9.81f, 0 ) );
        if( auto manager = app->getVehicleManager() )
        {
            manager->load( nullptr );
        }

        if( !actor->getComponent<CollisionBox>() )
        {
            actor->addComponent<CollisionBox>();
        }
        auto body = actor->getComponent<Rigidbody>();
        if( !body )
        {
            body = actor->addComponent<Rigidbody>();
        }
        body->setAngularDamping( 3 );
        body->setLinearDamping( .05f );
        auto car = actor->getComponent<CarController>();
        if( !car )
        {
            car = actor->addComponent<CarController>();
        }
        if( !car->getVehicleController() )
        {
            throw std::runtime_error( "ProceduralRaceScene requires WPVehiclePhysics." );
        }

        m_city->applyTo( m_assets );
        m_assets.terrainGameplaySource = m_terrainGameplaySource;
        m_assets.useFoliageWorld = m_foliageWorldEnabled;
        m_assets.exportSourceDirectory = m_materialExportDirectory;
        race::buildScene( m_assets, actor, m_seed, m_quality, m_citySourceOwner.lock() );
        m_city->adoptBuild( m_assets );

        actor->setSmoothMotion( true, false );
        for( auto child : actor->getChildren() )
        {
            if( !child->getFlag( IGameActor::ActorFlagWorldSpaceTransform ) )
            {
                child->setSmoothMotion( true, true );
            }
        }
        actor->getTransform()->setTask( TaskId::Physics );
        car->refreshWheels();
        configurePhysics();
        if( !m_assets.body )
        {
            throw std::runtime_error( m_generationError.empty() ? "Generated scene activation failed."
                                                                 : m_generationError.c_str() );
        }

        // Scene play can reconstruct the body and controller; configure again after settling.
        m_physicsConfigured = false;
        if( !m_assets.cityRaceRouteAvailable )
        {
            m_generationError = m_assets.cityRaceRouteError;
        }
    }

    void ProceduralRaceScene::generationFailed()
    {
        m_city->retainBindings( m_assets );
    }

    void ProceduralRaceScene::generationCleared()
    {
        m_vehicleAudio.unload();
        m_vehicleVisualEffects.unload();
        m_presentationInitialized = false;
        m_presentationResetRequested = false;
        m_presentationAudioClock = 0;
        setControls( 0, 0, 0 );
        m_resetRequested = false;
        m_physicsConfigured = false;
        m_surfaceGrip = 1;
    }

    void ProceduralRaceScene::configurePhysics()
    {
        if( !m_assets.circuit.samples.empty() && getCarController() &&
            getCarController()->getVehicleController() )
        {
            if( Thread::getTaskFlag( Thread::Application_Flag ) )
            {
                auto app = core::IApplicationManager::instancePtr();
                if( app && m_assets.terrainGameplaySource && m_assets.ground )
                {
                    const auto actorState = app->isPlaying() ? IGameActor::State::Play
                                                            : IGameActor::State::Edit;
                    const auto componentState = app->isPlaying() ? IComponent::State::Play
                                                                : IComponent::State::Edit;
                    const auto ground = m_assets.ground;
                    const auto terrain = ground->getComponent<TerrainSystem>();
                    const auto collision = ground->getComponent<CollisionTerrain>();
                    const auto body = ground->getComponent<Rigidbody>();
                    if( ground->getState() == actorState && terrain && collision && body &&
                        terrain->getState() == componentState &&
                        collision->getState() == componentState && body->getState() == componentState )
                    {
                        const auto renderer =
                            dynamic_cast<render::Terrain *>( terrain->getTerrain().get() );
                        const auto requested = terrain->getTerrainSnapshot();
                        render::TerrainSnapshot active;
                        Transform3<real_Num> placement;
                        u64 serial = 0;
                        bool detached = true;
                        terrain->getTerrainIntegrationState( active, placement, serial, detached );
                        if( !renderer || !requested || !serial || detached || active != requested ||
                            renderer->hasPendingTerrainPublication() ||
                            renderer->getTerrainSnapshot() != requested ||
                            renderer->getActiveTerrainSnapshot() != requested ||
                            collision->getTerrainSnapshot() != requested )
                        {
                            // Let Render/Physics advance the deferred generation.
                            // Stopping them on every application retry resets their
                            // task FSMs before either can execute its next frame.
                            m_physicsConfigured = false;
                            setControls( 0, 1, 0 );
                            return;
                        }
                    }
                }
                auto tasks = app ? app->getTaskManager() : nullptr;
                auto renderLock = tasks ? tasks->lockTask( TaskId::Render ) : TaskLock();
                auto physicsLock = tasks ? tasks->lockTask( TaskId::Physics ) : TaskLock();
                std::lock_guard<std::recursive_mutex> presentationLock( m_generationMutex );
                String error;
                if( !race::activateGeneratedScene( m_assets, getActor(), error ) )
                {
                    m_physicsConfigured = false;
                    setControls( 0, 1, 0 );
                    if( !error.empty() )
                    {
                        clearGeneratedScene();
                        m_generationError = error;
                        WP_LOG_ERROR( error );
                    }
                    return;
                }
            }
            else if( !m_physicsConfigured )
            {
                setControls( 0, 1, 0 );
                return;
            }
            race::configurePhysics( m_assets, getActor() );
            m_physicsConfigured = true;
        }
    }

    void ProceduralRaceScene::setControls( f32 throttle, f32 brake, f32 steering )
    {
        if( auto car = getCarController() )
        {
            car->setControls( throttle, brake, steering );
        }
    }

    void ProceduralRaceScene::usePlayerControls()
    {
        if( auto car = getCarController() )
        {
            car->usePlayerControls();
        }
    }

    void ProceduralRaceScene::reset()
    {
        m_resetRequested = true;
    }

    void ProceduralRaceScene::performReset()
    {
        m_presentationResetRequested = true;
        setControls( 0, 0, 0 );

        auto actor = getActor();
        if( !actor )
        {
            return;
        }

        if( auto car = getCarController() )
        {
            car->setThrottle( 0 );
            car->setBrake( 0 );
            car->setSteering( 0 );
            if( auto vehicle = car->getVehicleController() )
            {
                vehicle->reset();
                for( s32 channel = 0; channel < 3; ++channel )
                {
                    vehicle->setChannel( channel, 0 );
                }
            }
        }

        Vector3F spawnPosition( 0, .42f, 0 );
        if( ( m_assets.cityRoadShowcase || m_assets.cityTerrainRoads ) && m_assets.roadTerrain )
        {
            f32 height = 0;
            const auto snapshot = m_assets.roadTerrain->getTerrainSnapshot();
            const auto point = m_assets.cityRoadShowcase ? Vector3F( 0, 0, -60 )
                               : m_assets.circuit.samples.empty()
                                   ? Vector3F( 0, 0, 0 )
                                   : m_assets.circuit.samples.front().position;
            bool driveable = false;
            if( m_assets.roadAuthoring &&
                m_assets.roadAuthoring->getRuntimeSurface( point.x, point.z, height, driveable ) &&
                driveable )
            {
                spawnPosition = { point.x, height + .42f, point.z };
            }
            else if( snapshot &&
                     render::sampleTerrainHeight( *snapshot,
                                                  m_assets.roadAuthoring
                                                      ? m_assets.roadAuthoring->getTerrainPlacement()
                                                      : Transform3<real_Num>(),
                                                  point, height ) )
            {
                spawnPosition = { point.x, height + .445f, point.z };
            }
            else
            {
                m_generationError = "The edited city has no supported reset position.";
                return;
            }
        }
        const Transform3F spawn( spawnPosition, QuaternionF::identity() );
        if( auto rigidbody = actor->getComponent<Rigidbody>() )
        {
            rigidbody->setLinearVelocity( Vector3F::zero() );
            rigidbody->setAngularVelocity( Vector3F::zero() );
            if( auto body = rigidbody->getRigidDynamic() )
            {
                body->clearForce();
                body->clearTorque();
                body->setTransform( spawn );
            }
        }

        actor->setPosition( spawn.getPosition() );
        actor->setOrientation( spawn.getOrientation() );
        actor->updateTransform();
    }

    void ProceduralRaceScene::update()
    {
        auto app = core::IApplicationManager::instance();
        if( Thread::getCurrentTask() == TaskId::Application && isLoaded() && isEnabled() &&
            isGenerated() && !m_physicsConfigured )
        {
            configurePhysics();
        }
        if( Thread::getCurrentTask() == TaskId::Application && isLoaded() && isEnabled() &&
            m_city->usesTerrainRoads() )
        {
            if( auto manager = app->getGameManager();
                manager && m_city->needsRefresh( *manager, getActor(), m_assets ) )
            {
                regenerate();
            }
            return;
        }
        if( Thread::getCurrentTask() == TaskId::Render )
        {
            // Generation may hold scene/graphics locks on the application task.
            // Skip presentation rather than wait with render locks held.
            std::unique_lock<std::recursive_mutex> presentationLock( m_generationMutex,
                                                                     std::try_to_lock );
            if( !presentationLock.owns_lock() || !isGenerated() || !getActor() )
            {
                return;
            }

            getActor()->updateTransform();
            if( m_presentationResetRequested.exchange( false ) )
            {
                m_vehicleVisualEffects.reset();
            }

            if( m_automaticPresentation )
            {
                initializePresentation();
                const auto dt = float( app->getTimer()->getDeltaTime() );
                if( std::isfinite( dt ) && dt > 0 )
                {
                    m_presentationAudioClock += std::min( dt, .05f );
                    if( m_presentationAudioClock >= 1.f / 30.f )
                    {
                        m_vehicleAudio.update( sampleVehicleAudio(), m_presentationAudioClock );
                        m_presentationAudioClock = 0;
                    }
                    m_vehicleVisualEffects.update( sampleVehicleEffects(), dt );
                }
            }
            return;
        }

        if( !isGenerated() || !isEnabled() || Thread::getCurrentTask() != TaskId::Physics ||
            !app->isPlaying() || app->isPaused() )
        {
            return;
        }

        auto car = getCarController();
        // Play rebuilds the wheel setup. Apply tuning after that transition.
        if( !m_physicsConfigured && car && car->getState() == State::Play &&
            app->getTimer()->getTimeSinceSceneLoad() > 3 )
        {
            configurePhysics();
        }
        if( !m_physicsConfigured )
        {
            setControls( 0, 1, 0 );
            return;
        }

        if( m_resetRequested.exchange( false ) )
        {
            performReset();
        }

        if( !car )
        {
            return;
        }

        auto lamp =
            m_assets.vehicleMaterials[static_cast<size_t>( procedural::VehicleMaterialSlot::RainLight )];
        const ColourF emissive( car->getBrake() > 0 ? 1.f : .15f, .003f, .001f, 1 );
        if( lamp && lamp->getEmissive() != emissive )
        {
            lamp->setEmissive( emissive );
        }

        if( m_physicsConfigured )
        {
            updateSurfaceGrip();
            applyAerodynamics();
        }
    }

    void ProceduralRaceScene::postUpdate()
    {
        if( Thread::getCurrentTask() == TaskId::Physics && isGenerated() && isEnabled() )
        {
            updateWheelVisuals();
            updateShadow();
        }
    }

    // todo move
    void ProceduralRaceScene::applyAerodynamics()
    {
        auto actor = getActor();
        auto body = actor->getComponent<Rigidbody>();
        auto velocity = body->getLinearVelocity();
        velocity.y = real_Num( 0.0 );

        const auto speed = velocity.length();
        const auto &aero = m_assets.vehicle.physics.aero;
        auto force =
            -velocity * static_cast<real_Num>( .5 * aero.airDensityKgPerM3 * aero.referenceAreaM2 *
                                               aero.dragCoefficient * speed );
        force.y = -static_cast<real_Num>( .5 * aero.airDensityKgPerM3 * aero.referenceAreaM2 *
                                          aero.downforceCoefficient * speed * speed );
        body->addForce( force );

        if( auto carController = getCarController() )
        {
            if( auto vehicle = carController->getVehicleController() )
            {
                const auto acceleration =
                    real_Num( 9.81 ) -
                    force.y / static_cast<real_Num>( m_assets.vehicle.physics.massProperties.massKg );
                for( u32 i = 0; i < 4; ++i )
                {
                    auto wheel = vehicle->getWheelController( i );
                    wheel->setContactAcceleration( acceleration );
                }
            }
        }
    }

    void ProceduralRaceScene::updateSurfaceGrip()
    {
        auto actor = getActor();
        auto p = actor->getPosition();

        float roadDistance;

        if( ( m_assets.cityRoadShowcase || m_assets.cityTerrainRoads ) && m_assets.roadAuthoring )
        {
            f32 height = 0;
            bool driveable = false;
            roadDistance =
                m_assets.roadAuthoring->getRuntimeSurface( p.x, p.z, height, driveable ) && driveable
                    ? 0.f
                    : 100.f;
        }
        else if( m_assets.openCity )
        {
            roadDistance = m_assets.cityLayout.roadDistance( p.x, p.z );
        }
        else
        {
            auto offset = p - getCircuitPosition( nearestCircuitSample( p ) );
            offset.y = 0;
            roadDistance = offset.length();
        }

        const auto grip = roadDistance < 6.85f ? 1.f : roadDistance < 11.f ? .55f : .35f;
        if( grip == m_surfaceGrip )
        {
            return;
        }

        m_surfaceGrip = grip;

        if( auto carController = getCarController() )
        {
            if( auto vehicle = carController->getVehicleController() )
            {
                for( u32 i = 0; i < 4; ++i )
                {
                    auto wheel = vehicle->getWheelController( i );
                    // Preserve the authored tyre multiplier when changing surface; the old
                    // path silently replaced 1.25 with 1.0 on the first physics tick.
                    wheel->setGrip( grip * 1.25f );
                }
            }
        }
    }

    void ProceduralRaceScene::updateWheelVisuals()
    {
        auto car = getCarController();
        auto vehicle = car ? car->getVehicleController() : nullptr;
        if( !vehicle )
        {
            return;
        }

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheel = vehicle->getWheelController( i );
            if( !wheel || !m_assets.wheels[i] )
            {
                continue;
            }

            // Reuse the contact solver's raycast result instead of casting four
            // more rays and allocating four hit objects every physics step.
            const auto extension =
                wheel->getSuspensionTravel() * ( 1 - std::clamp( wheel->getCompression(), 0.f, 1.f ) );
            auto position = wheel->getLocalTransform().getPosition();
            position.y -= extension;
            m_assets.wheels[i]->setLocalPosition( position );
        }
    }

    void ProceduralRaceScene::updateShadow()
    {
        if( !m_assets.shadow || !getActor() )
        {
            return;
        }

        auto position = getActor()->getPosition();
        auto forward = getActor()->getWorldTransform().forward();
        auto yaw = std::atan2( -forward.x, -forward.z );
        f32 surfaceHeight = 0;
        if( ( m_assets.cityRoadShowcase || m_assets.cityTerrainRoads ) && m_assets.roadTerrain )
        {
            const auto snapshot = m_assets.roadTerrain->getTerrainSnapshot();
            const auto placement = m_assets.roadAuthoring ? m_assets.roadAuthoring->getTerrainPlacement()
                                                          : Transform3<real_Num>();
            if( !snapshot ||
                !render::sampleTerrainHeight( *snapshot, placement, position, surfaceHeight ) )
            {
                m_assets.shadow->setVisible( false );
                return;
            }
        }
        position.y =
            surfaceHeight + ( ( m_assets.cityRoadShowcase || m_assets.cityTerrainRoads ) ? .025f : 0.f );
        m_assets.shadow->setPosition( position );
        m_assets.shadow->setOrientation( QuaternionF::eulerDegrees( 0, yaw * 180.f / 3.14159265f, 0 ) );
        m_assets.shadow->setVisible( getActor()->getPosition().y - surfaceHeight < .8f );
        m_assets.shadow->updateTransform();
    }

    void ProceduralRaceScene::setView( const Vector3F &position, f32 fovRadians, f32 nearClip,
                                       f32 lodBias )
    {
        if( !m_assets.vehicleLOD )
        {
            return;
        }

        auto system = dynamic_pointer_cast<LODSystem>( m_assets.vehicleLOD->getComponentSystem() );
        if( system )
        {
            LODSystem::View view;
            view.position = position;
            view.verticalFovRadians = fovRadians;
            view.nearClipDistance = nearClip;
            view.lodBias = lodBias;
            system->setViewOverride( view );
        }
    }

    f32 ProceduralRaceScene::getWheelbase() const
    {
        return static_cast<float>( m_assets.vehicle.physics.wheelbaseM );
    }

    f32 ProceduralRaceScene::getMaxSteeringAngle() const
    {
        return static_cast<float>( m_assets.vehicle.physics.wheels[0].maxSteerRad );
    }

    SmartPtr<IGameActor> ProceduralRaceScene::getWheelActor( u32 index ) const
    {
        return m_assets.wheels.at( index );
    }

    SmartPtr<CarController> ProceduralRaceScene::getCarController() const
    {
        auto actor = getActor();
        return actor ? actor->getComponent<CarController>() : nullptr;
    }

    bool ProceduralRaceScene::validateReflection() const
    {
        return race::validateReflection( m_assets );
    }

    SmartPtr<Properties> ProceduralRaceScene::getProperties() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        auto properties = ProceduralScene::getProperties();
        properties->setProperty( "Audio Enabled", getAudioEnabled() );
        properties->setProperty( "Effects Enabled", getEffectsEnabled() );
        m_city->writeProperties( *properties, m_assets );
        return properties;
    }

    void ProceduralRaceScene::readGenerationProperties( Properties &properties )
    {
        bool audio = getAudioEnabled(), effects = getEffectsEnabled();
        properties.getPropertyValue( "Audio Enabled", audio );
        properties.getPropertyValue( "Effects Enabled", effects );
        setAudioEnabled( audio );
        setEffectsEnabled( effects );
        m_city->readProperties( properties, m_generationError );
    }

    SmartPtr<IGameActor> ProceduralRaceScene::getBodyActor() const
    {
        return m_assets.body;
    }

    bool ProceduralRaceScene::isPhysicsConfigured() const
    {
        return m_physicsConfigured;
    }

    bool ProceduralRaceScene::isGenerated() const
    {
        return ProceduralScene::isGenerated() && !m_assets.circuit.samples.empty();
    }

    void ProceduralRaceScene::setTerrainGameplaySource( render::TerrainSnapshot source )
    {
        String error;
        if( source && !render::validateTerrainData( *source, error ) )
        {
            throw std::invalid_argument( error.c_str() );
        }
        std::lock_guard<std::recursive_mutex> presentationLock( m_generationMutex );
        m_terrainGameplaySource = std::move( source );
    }

    void ProceduralRaceScene::setCitySourceOwner( SmartPtr<IGameActor> owner )
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        m_citySourceOwner = owner.get();
    }

    void ProceduralRaceScene::initializePresentation()
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        if( !isGenerated() )
        {
            return;
        }

        auto app = core::IApplicationManager::instancePtr();
        if( !app )
        {
            return;
        }

        const bool audio = m_audioEnabled, effects = m_effectsEnabled;
        if( !m_presentationInitialized || audio != m_appliedAudioEnabled )
        {
            m_vehicleAudio.unload();

            if( audio && !m_vehicleAudio.load( app->getSoundManager() ) )
            {
                WP_LOG_WARNING(
                    "Race vehicle audio unavailable: check the output device and bundled WAV "
                    "files." );
            }

            m_appliedAudioEnabled = audio;
        }

        if( !m_presentationInitialized || effects != m_appliedEffectsEnabled )
        {
            m_vehicleVisualEffects.unload();

            if( effects && !m_vehicleVisualEffects.load( getQuality() == 0   ? 0u
                                                         : getQuality() == 1 ? 1u
                                                                             : 2u,
                                                         m_seed ) )
            {
                WP_LOG_WARNING(
                    "Race vehicle visual effects unavailable in the active graphics backend." );
            }

            m_appliedEffectsEnabled = effects;
        }

        m_presentationInitialized = true;
    }

    void ProceduralRaceScene::setAudioEnabled( bool enabled )
    {
        m_audioEnabled = enabled;
    }

    bool ProceduralRaceScene::getAudioEnabled() const
    {
        return m_audioEnabled;
    }

    void ProceduralRaceScene::setEffectsEnabled( bool enabled )
    {
        m_effectsEnabled = enabled;
    }

    bool ProceduralRaceScene::getEffectsEnabled() const
    {
        return m_effectsEnabled;
    }

    void ProceduralRaceScene::setAutomaticPresentation( bool enabled )
    {
        m_automaticPresentation = enabled;
    }

    bool ProceduralRaceScene::isAudioAvailable() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        return m_vehicleAudio.isPlaying();
    }

    bool ProceduralRaceScene::isEffectsAvailable() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        return m_vehicleVisualEffects.uploaded();
    }

    u32 ProceduralRaceScene::getParticleCount() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        return u32( m_vehicleVisualEffects.particles() );
    }

    u32 ProceduralRaceScene::getSkidDecalCount() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_generationMutex );
        return u32( m_vehicleVisualEffects.decals() );
    }

    advanced::VehicleAudio &ProceduralRaceScene::getVehicleAudio()
    {
        return m_vehicleAudio;
    }

    advanced::VehicleVisualEffects &ProceduralRaceScene::getVehicleVisualEffects()
    {
        return m_vehicleVisualEffects;
    }

    bool ProceduralRaceScene::isRoadSurface( const Vector3F &position ) const
    {
        if( !isGenerated() )
        {
            return false;
        }

        if( ( m_assets.cityRoadShowcase || m_assets.cityTerrainRoads ) && m_assets.roadAuthoring )
        {
            f32 height = 0;
            bool driveable = false;
            return m_assets.roadAuthoring->getRuntimeSurface( position.x, position.z, height,
                                                              driveable ) &&
                   driveable && std::abs( position.y - height ) < 1.f;
        }

        if( m_assets.openCity )
        {
            return m_assets.cityLayout.roadDistance( position.x, position.z ) < 6.85f;
        }

        auto offset = position - getCircuitPosition( nearestCircuitSample( position ) );
        offset.y = 0;
        return offset.length() < 6.85f;
    }

    advanced::VehicleEffectsFrame ProceduralRaceScene::sampleVehicleEffects() const
    {
        auto app = core::IApplicationManager::instancePtr();
        if( !app || !getActor() || !isGenerated() )
        {
            advanced::VehicleEffectsFrame empty;
            empty.playing = false;
            return empty;
        }

        advanced::VehicleEffectsFrame frame;
        frame.playing = app->isPlaying() && !app->isPaused();
        frame.position = getActor()->getPosition();

        auto body = getActor()->getComponent<scene::Rigidbody>();
        auto car = getActor()->getComponent<scene::CarController>();
        auto vehicle = car ? car->getVehicleController() : nullptr;

        if( !body || !vehicle || !vehicle->getBody() )
        {
            frame.playing = false;
            return frame;
        }

        frame.velocity = body->getLinearVelocity();

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheel = vehicle->getWheelController( i );
            if( !wheel )
            {
                continue;
            }

            auto properties = wheel->getProperties();
            bool grounded = false;
            double slip = 0;
            properties->getPropertyValue( "Is On Ground", grounded );
            properties->getPropertyValue( "Slip Velocity", slip );
            frame.slip[i] = float( slip );
            frame.width[i] = float( m_assets.vehicle.physics.wheels[i].tire.widthM * .82 );

            if( !grounded )
            {
                continue;
            }

            const auto hub = wheel->getWorldTransform().getPosition();
            SmartPtr<physics::IRaycastHit> hit = make_ptr<physics::RaycastHit>();
            // Reuse the vehicle callback, which excludes its own chassis.
            if( vehicle->getBody()->castWorldRay(
                    Ray3<real_Num>( hub + Vector3F( 0, .25f, 0 ), Vector3F( 0, -1, 0 ) ), hit ) &&
                hit->getDistance() < 1.5f && hit->getNormal().y > .5f )
            {
                frame.grounded[i] = true;
                frame.contact[i] = hit->getPoint();
                frame.normal[i] = hit->getNormal();
                frame.onRoad[i] = isRoadSurface( frame.contact[i] );
            }
        }

        frame.playing = frame.playing && isEnabled();
        return frame;
    }

    advanced::VehicleAudioInput ProceduralRaceScene::sampleVehicleAudio() const
    {
        auto app = core::IApplicationManager::instancePtr();
        if( !app || !getActor() || !isGenerated() )
        {
            advanced::VehicleAudioInput empty;
            empty.playing = false;
            return empty;
        }

        advanced::VehicleAudioInput input;
        const auto &drivetrain = m_assets.vehicle.physics.drivetrain;
        input.idleRpm = float( drivetrain.idleRpm );
        input.redlineRpm = float( drivetrain.redlineRpm );
        input.playing = app->isPlaying() && !app->isPaused();

        if( auto car = getActor()->getComponent<scene::CarController>() )
        {
            if( auto vehicle = car->getVehicleController() )
            {
                input.throttle = vehicle->getChannel( 0 );
                if( auto drive = vehicle->getDriveTrain() )
                {
                    double rpm = 0;
                    drive->getProperties()->getPropertyValue( "RPM", rpm );
                    input.rpm = float( rpm );
                }
                for( u32 i = 0; i < 4; ++i )
                {
                    if( auto wheel = vehicle->getWheelController( i ) )
                    {
                        auto properties = wheel->getProperties();
                        bool grounded = false;
                        double slip = 0;
                        properties->getPropertyValue( "Is On Ground", grounded );
                        properties->getPropertyValue( "Slip Velocity", slip );
                        if( grounded )
                        {
                            input.grounded = true;
                            input.slipSpeed = std::max( input.slipSpeed, float( slip ) );
                        }
                    }
                }
            }
        }

        if( auto body = getActor()->getComponent<scene::Rigidbody>() )
        {
            input.speed = float( body->getLinearVelocity().length() );
        }

        input.playing = input.playing && isEnabled();
        return input;
    }

}  // namespace workphone::scene
