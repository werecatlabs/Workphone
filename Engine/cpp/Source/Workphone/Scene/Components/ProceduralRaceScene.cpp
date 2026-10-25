#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Physics/RaycastHit.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
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
    namespace race
    {
        // Module-private declaration; no public/PCH API or layout change.
        bool activateGeneratedScene( const SceneAssets &, SmartPtr<IGameActor>, String & );
    }

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralRaceScene, Component );
    ProceduralRaceScene::ProceduralRaceScene() = default;
    ProceduralRaceScene::~ProceduralRaceScene() = default;

    void ProceduralRaceScene::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            return;
        }

        Component::load( data );

        auto manager = core::IApplicationManager::instance()->getGameManager();
        manager->registerComponentUpdate( TaskId::Physics, Thread::UpdateState::Update, this );
        manager->registerComponentUpdate( TaskId::Physics, Thread::UpdateState::PostUpdate, this );
        manager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Update, this );
        manager->registerComponentUpdate( TaskId::Application, Thread::UpdateState::Update, this );

        setLoadingState( LoadingState::Loaded );
    }

    void ProceduralRaceScene::unload( SmartPtr<ISharedObject> data )
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
        m_cityRoadAuthoring=nullptr;m_cityTerrain=nullptr;
        Component::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }

    bool ProceduralRaceScene::regenerate()
    {
        auto tasks = core::IApplicationManager::instance()->getTaskManager();
        // Lua can generate during Play. Synchronize resource/body replacement
        // without changing the Editor's global play flag (which requests Stop).
        auto renderLock = tasks ? tasks->lockTask( TaskId::Render ) : TaskLock();
        auto physicsLock = tasks ? tasks->lockTask( TaskId::Physics ) : TaskLock();
        std::lock_guard<std::recursive_mutex> presentationLock( m_presentationMutex );

        auto actor = getActor();
        if( !actor )
        {
            m_generationError = "Attach ProceduralRaceScene to a vehicle actor before generating.";
            return false;
        }
        if(m_cityTerrainRoads && m_cityRoadShowcase)
        {m_generationError="Choose a generated terrain city or the separate road authoring showcase.";return false;}
        if(m_cityTerrainRoads && !m_materialExportDirectory.empty())
        {m_generationError="Terrain-city source material export is unavailable.";return false;}

        auto currentScene=core::IApplicationManager::instance()->getGameManager()->getCurrentScene();
        if(m_cityTerrainRoads)
        {
            if(m_cityTerrain && (!m_cityTerrain->getActor() || m_cityTerrain->getActor()->getScene()!=currentScene))m_cityTerrain=nullptr;
            if(m_cityRoadAuthoring && (!m_cityRoadAuthoring->getActor() ||
                m_cityRoadAuthoring->getActor()->getScene()!=currentScene ||
                !m_cityRoadAuthoring->getActor()->getFlag(IGameActor::ActorFlagInScene)))
            {m_cityRoadAuthoring=nullptr;m_cityTerrain=nullptr;}
            if(!m_cityRoadAuthoring && !m_cityRoadAuthoringUUID.empty())
            {
                for(auto candidate:core::IApplicationManager::instance()->getGameManager()->getActors())
                    if(candidate && candidate->getScene()==currentScene && candidate->getFlag(IGameActor::ActorFlagInScene))
                        if(auto source=candidate->getComponent<CityRoadAuthoring>())
                            if(source->getHandle() && source->getHandle()->getUUIDAsString()==m_cityRoadAuthoringUUID)
                            {m_cityRoadAuthoring=source;break;}
                if(!m_cityRoadAuthoring){clearGeneratedScene();m_generationError="The saved city road source is unavailable in this scene.";return false;}
            }
            if(m_cityRoadAuthoring)
            {
                // The canonical source owns this binding; cached scenery must
                // never reverse a user's rebind to another terrain.
                m_cityTerrain=m_cityRoadAuthoring->getTerrain();
                if(!m_cityTerrain)
                    for(auto candidate:core::IApplicationManager::instance()->getGameManager()->getActors())
                        if(candidate && candidate->getScene()==currentScene && candidate->getFlag(IGameActor::ActorFlagInScene))
                            if(auto terrain=candidate->getComponent<TerrainSystem>())
                                if(terrain->getHandle() && terrain->getHandle()->getUUIDAsString()==m_cityRoadAuthoring->getTerrainBindingUUID())
                                {m_cityTerrain=terrain;break;}
                if(!m_cityTerrain){clearGeneratedScene();m_generationError="The saved city terrain binding is unavailable.";return false;}
            }
        }
        // Saved source/terrain anchors are scene-owned. Only their derived city
        // presentation is replaced, so regeneration cannot erase manual roads.
        clearGeneratedScene();

        try
        {
            auto app = core::IApplicationManager::instance();
            auto physics = app->getPhysicsManager();
            if( !physics )
            {
                throw std::runtime_error( "ProceduralRaceScene requires a physics manager." );
            }

            if( !physics->getPhysicsScene() )
            {
                physics->setPhysicsScene( physics->addScene() );
            }

            physics->getPhysicsScene()->setGravity( Vector3F( 0, -9.81f, 0 ) );

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

            // Admit the owner slot before publishing a new manager-owned actor.
            if( !m_cityRoadShowcase && !m_cityTerrainRoads )
            {
            m_assets.actors.reserve(m_assets.actors.size()+1);
            auto ground = app->getGameManager()->createActor();
            if(!ground) throw std::runtime_error("Cannot create procedural collision actor");
            m_assets.actors.push_back( ground );
            m_assets.ground = ground;
            ground->setName( "Circuit collision plane" );
            ground->setStatic( true );
            ground->setPosition( Vector3F( 0, -.5f, 0 ) );
            ground->addComponent<CollisionBox>()->setExtents( Vector3F( 1100, 1, 1100 ) );
            ground->addComponent<Rigidbody>();
            app->getGameManager()->getCurrentScene()->addActor( ground );
            }

            if( m_openCity && !m_cityRoadShowcase )
            {
                m_cityLayout = procedural::OpenCityLayout::generate( m_seed, m_cityBlocks, m_cityRoute, m_cityNaturalRoads );
            }

            m_assets.openCity = m_openCity;
            m_assets.cityNaturalRoads = m_cityNaturalRoads;
            m_assets.cityRoadShowcase = m_cityRoadShowcase;
            m_assets.cityTerrainRoads = m_cityTerrainRoads;
            if(m_cityTerrainRoads){m_assets.roadAuthoring=m_cityRoadAuthoring;m_assets.roadTerrain=m_cityTerrain;}
            m_assets.useFoliageWorld = m_foliageWorldEnabled;
            m_assets.exportSourceDirectory = m_materialExportDirectory;
            m_assets.cityBlocks = m_cityBlocks;
            m_assets.cityRoute = m_cityRoute;

            race::buildScene( m_assets, actor, m_seed, m_quality );
            if(m_cityTerrainRoads)
            {
                m_cityRoadAuthoring=m_assets.roadAuthoring;m_cityTerrain=m_assets.roadTerrain;
                m_cityRoadAuthoringUUID=m_cityRoadAuthoring->getHandle()->getUUIDAsString();
                m_cityLayout=m_assets.cityLayout;m_cityLastRefreshAttemptRevision=m_assets.cityAppliedSourceRevision;
                m_cityBuiltTerrain=m_cityRoadAuthoring->getTerrainSnapshot();m_cityBuiltTerrainPlacement=m_cityRoadAuthoring->getTerrainPlacement();
                m_cityLastAttemptTerrain=m_cityBuiltTerrain;m_cityLastAttemptPlacement=m_cityBuiltTerrainPlacement;
            }

            // Render the chassis and attached meshes from the same sampled parent pose.
            actor->setSmoothMotion( true, true );
            actor->getTransform()->setTask( TaskId::Physics );
            car->refreshWheels();
            configurePhysics();
            if(!m_assets.body)
                throw std::runtime_error(m_generationError.empty()?"Generated scene activation failed.":m_generationError.c_str());

            // Scene play can reconstruct the body and controller; configure again after settling.
            m_physicsConfigured = false;
            m_generationError=m_assets.cityRaceRouteAvailable?String():m_assets.cityRaceRouteError;
            return true;
        }
        catch( const std::exception &e )
        {
            if(m_cityTerrainRoads && m_assets.roadTerrain)m_cityTerrain=m_assets.roadTerrain;
            if(m_cityTerrainRoads && m_assets.roadAuthoring)
            {
                m_cityRoadAuthoring=m_assets.roadAuthoring;m_cityTerrain=m_assets.roadTerrain;
                if(auto handle=m_cityRoadAuthoring->getHandle())m_cityRoadAuthoringUUID=handle->getUUIDAsString();
            }
            // Retire partial producers before admitting the diagnostic string.
            clearGeneratedScene();
            m_generationError = e.what();
            return false;
        }

        return false;
    }

    void ProceduralRaceScene::clearGeneratedScene()
    {
        std::lock_guard<std::recursive_mutex> presentationLock( m_presentationMutex );
        race::SceneAssets retired;
        m_assets.swap(retired);
        // Drop producer pixels/path scratch before allocation-prone callbacks.
        // Resource inventories remain owners through the entire retirement.
        retired.vehicle = {};
        retired.circuit = {};
        m_cityLayout = {};
        m_vehicleAudio.unload();
        m_vehicleVisualEffects.unload();
        m_presentationInitialized = false;
        m_presentationResetRequested = false;
        m_presentationAudioClock = 0;

        setControls( 0, 0, 0 );

        m_resetRequested = false;
        m_physicsConfigured = false;
        m_surfaceGrip = 1;

        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;

        if( retired.vehicleLOD )
        {
            if( auto system =
                    dynamic_pointer_cast<LODSystem>( retired.vehicleLOD->getComponentSystem() ) )
            {
                system->clearViewOverride();
            }
        }

        if( retired.vehicleLOD )
        {
            if( auto actor = getActor() )
            {
                actor->removeComponentInstance( retired.vehicleLOD );
            }
        }

        if( manager )
        {
            for( auto it = retired.actors.rbegin(); it != retired.actors.rend(); ++it )
            {
                try { manager->destroyActor(*it); } catch(const std::bad_alloc &) {}
            }
        }

        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        if( graphics )
        {
            auto scene = graphics->getGraphicsScene();
            if( retired.foliageTerrain )
            {
                String error;
                retired.foliageTerrain->setFoliageSnapshot( {}, error );
                if( scene && (!retired.cityTerrainRoads || !retired.roadTerrain ||
                    retired.roadTerrain->getTerrain()!=retired.foliageTerrain) )
                {
                    scene->removeGraphicsObject( retired.foliageTerrain );
                }
            }

            if( scene && retired.sky )
            {
                scene->removeGraphicsObject( retired.sky );
            }

            if( scene && retired.sun )
            {
                scene->removeGraphicsObject( retired.sun );
            }

            auto materials = graphics->getMaterialManager();
            if( materials )
            {
                for( auto material : retired.materials )
                {
                    try { materials->destroyResource(material); } catch(const std::bad_alloc &) {}
                }
            }

            auto textures = graphics->getTextureManager();
            if( textures )
            {
                for( auto texture : retired.textures )
                {
                    // TextureManager detaches the resource before notifying it.
                    try { textures->destroyResource(texture); } catch(const std::bad_alloc &) {}
                }
            }
        }

        auto meshes = app ? dynamic_pointer_cast<MeshManager>( app->getMeshManager() ) : nullptr;
        if( meshes )
        {
            for( auto mesh : retired.meshes )
            {
                try { meshes->removeMeshResource(mesh); } catch(const std::bad_alloc &) {}
            }
        }
    }

    void ProceduralRaceScene::configurePhysics()
    {
        if( isGenerated() && getCarController() && getCarController()->getVehicleController() )
        {
            if(Thread::getTaskFlag(Thread::Application_Flag))
            {
                auto app=core::IApplicationManager::instancePtr();auto tasks=app?app->getTaskManager():nullptr;
                auto renderLock=tasks?tasks->lockTask(TaskId::Render):TaskLock();
                auto physicsLock=tasks?tasks->lockTask(TaskId::Physics):TaskLock();
                std::lock_guard<std::recursive_mutex> presentationLock(m_presentationMutex);
                String error;
                if(!race::activateGeneratedScene(m_assets,getActor(),error))
                {
                    m_physicsConfigured=false;setControls(0,1,0);
                    if(!error.empty()){clearGeneratedScene();m_generationError=error;WP_LOG_ERROR(error);}
                    return;
                }
            }
            else if(!m_physicsConfigured){setControls(0,1,0);return;}
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

        Vector3F spawnPosition(0,.42f,0);
        if((m_assets.cityRoadShowcase || m_assets.cityTerrainRoads) && m_assets.roadTerrain)
        {
            f32 height=0;const auto snapshot=m_assets.roadTerrain->getTerrainSnapshot();
            const auto point=m_assets.cityRoadShowcase?Vector3F(0,0,-60):
                m_assets.circuit.samples.empty()?Vector3F(0,0,0):m_assets.circuit.samples.front().position;
            bool driveable=false;
            if(m_assets.roadAuthoring && m_assets.roadAuthoring->getRuntimeSurface(point.x,point.z,height,driveable) && driveable)
                spawnPosition={point.x,height+.42f,point.z};
            else if(snapshot && render::sampleTerrainHeight(*snapshot,m_assets.roadAuthoring?m_assets.roadAuthoring->getTerrainPlacement():Transform3<real_Num>(),point,height))
                spawnPosition={point.x,height+.445f,point.z};
            else {m_generationError="The edited city has no supported reset position.";return;}
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
        if(Thread::getCurrentTask()==TaskId::Application && isLoaded() && isEnabled() && isGenerated() && !m_physicsConfigured)
            configurePhysics();
        if(Thread::getCurrentTask()==TaskId::Application && isLoaded() && isEnabled() && m_cityTerrainRoads)
        {
            const auto currentScene=app->getGameManager()->getCurrentScene();
            if(m_cityRoadAuthoring && (!m_cityRoadAuthoring->getActor() ||
                m_cityRoadAuthoring->getActor()->getScene()!=currentScene ||
                !m_cityRoadAuthoring->getActor()->getFlag(IGameActor::ActorFlagInScene)))
            {
                // A retained component pointer must not keep scenery/collision
                // alive after its canonical source actor leaves the scene.
                if(auto owner=m_cityRoadAuthoring->getActor())
                    if(!owner->getFlag(IGameActor::ActorFlagInScene))m_cityRoadAuthoring->update();
                regenerate();return;
            }
            if(!m_cityRoadAuthoring && !m_cityRoadAuthoringUUID.empty())
            {
                const auto scene=app->getGameManager()->getCurrentScene();
                if(getActor() && getActor()->getScene()==scene)
                    for(auto candidate:app->getGameManager()->getActors())
                        if(candidate && candidate->getScene()==scene && candidate->getFlag(IGameActor::ActorFlagInScene))
                            if(auto source=candidate->getComponent<CityRoadAuthoring>())
                                if(source->getHandle() && source->getHandle()->getUUIDAsString()==m_cityRoadAuthoringUUID)
                                {m_cityRoadAuthoring=source;break;}
            }
            if(!m_cityRoadAuthoring)
            {
                if(m_assets.roadAuthoring || !m_assets.actors.empty())regenerate();
                return;
            }
            const auto revision=m_cityRoadAuthoring->getActiveRevision();
            const auto snapshot=m_cityRoadAuthoring->getTerrainSnapshot();const auto placement=m_cityRoadAuthoring->getTerrainPlacement();
            const bool changed=m_assets.roadAuthoring!=m_cityRoadAuthoring || revision!=m_assets.cityAppliedSourceRevision || snapshot!=m_cityBuiltTerrain || placement!=m_cityBuiltTerrainPlacement;
            const bool attempted=revision==m_cityLastRefreshAttemptRevision && snapshot==m_cityLastAttemptTerrain && placement==m_cityLastAttemptPlacement;
            if(changed && !attempted)
            {
                m_cityLastRefreshAttemptRevision=revision;m_cityLastAttemptTerrain=snapshot;m_cityLastAttemptPlacement=placement;
                regenerate();
            }
            return;
        }
        if( Thread::getCurrentTask() == TaskId::Render )
        {
            // Generation may hold scene/graphics locks on the application task.
            // Skip presentation rather than wait with render locks held.
            std::unique_lock<std::recursive_mutex> presentationLock( m_presentationMutex,
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
        if(!m_physicsConfigured){setControls(0,1,0);return;}

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
        velocity.y = real_Num(0.0);

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

        if((m_assets.cityRoadShowcase || m_assets.cityTerrainRoads) && m_assets.roadAuthoring)
        {
            f32 height=0;bool driveable=false;
            roadDistance=m_assets.roadAuthoring->getRuntimeSurface(p.x,p.z,height,driveable) && driveable?0.f:100.f;
        }
        else if( m_assets.openCity )
        {
            roadDistance = m_cityLayout.roadDistance( p.x, p.z );
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
        f32 surfaceHeight=0;
        if((m_assets.cityRoadShowcase || m_assets.cityTerrainRoads) && m_assets.roadTerrain)
        {
            const auto snapshot=m_assets.roadTerrain->getTerrainSnapshot();
            const auto placement=m_assets.roadAuthoring?m_assets.roadAuthoring->getTerrainPlacement():Transform3<real_Num>();
            if(!snapshot || !render::sampleTerrainHeight(*snapshot,placement,position,surfaceHeight))
            {m_assets.shadow->setVisible(false);return;}
        }
        position.y = surfaceHeight+((m_assets.cityRoadShowcase || m_assets.cityTerrainRoads)?.025f:0.f);
        m_assets.shadow->setPosition( position );
        m_assets.shadow->setOrientation( QuaternionF::eulerDegrees( 0, yaw * 180.f / 3.14159265f, 0 ) );
        m_assets.shadow->setVisible( getActor()->getPosition().y-surfaceHeight < .8f );
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

    void ProceduralRaceScene::setQuality( s32 quality )
    {
        m_quality = static_cast<procedural::VehicleAppearanceQuality>( std::clamp( quality, 0, 3 ) );
    }

    u32 ProceduralRaceScene::nearestCircuitSample( const Vector3F &position ) const
    {
        return static_cast<u32>( m_assets.circuit.nearest( position ) );
    }

    Vector3F ProceduralRaceScene::getCircuitPosition( u32 index ) const
    {
        return m_assets.circuit.samples.at( index ).position;
    }

    Vector3F ProceduralRaceScene::getCircuitRight( u32 index ) const
    {
        return m_assets.circuit.samples.at( index ).right;
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
        auto p = Component::getProperties();
        p->setProperty( "Seed", m_seed );
        p->setProperty( "Appearance Quality", getQuality() );
        p->setProperty( "Audio Enabled", getAudioEnabled() );
        p->setProperty( "Effects Enabled", getEffectsEnabled() );
        p->setProperty( "Open City", m_openCity );
        p->setProperty( "Foliage World", m_foliageWorldEnabled );
        p->setProperty( "City Blocks", m_cityBlocks );
        p->setProperty( "City Route", m_cityRoute );
        p->setProperty( "City Natural Roads", m_cityNaturalRoads );
        p->setProperty( "Road Authoring Showcase", m_cityRoadShowcase );
        p->setProperty( "City Terrain Roads", m_cityTerrainRoads );
        p->setProperty( "City Road Authoring UUID", m_cityRoadAuthoringUUID );
        p->setProperty( "Generation Error", m_generationError );
        p->setButtonPressed( "Regenerate", false );
        return p;
    }

    void ProceduralRaceScene::setProperties( SmartPtr<Properties> p )
    {
        if( !p )
        {
            return;
        }

        Component::setProperties( p );

        p->getPropertyValue( "Seed", m_seed );
        s32 quality = getQuality();
        p->getPropertyValue( "Appearance Quality", quality );
        setQuality( quality );

        bool audio = getAudioEnabled(), effects = getEffectsEnabled();
        p->getPropertyValue( "Audio Enabled", audio );
        p->getPropertyValue( "Effects Enabled", effects );
        setAudioEnabled( audio );
        setEffectsEnabled( effects );
        p->getPropertyValue( "Open City", m_openCity );
        p->getPropertyValue( "Foliage World", m_foliageWorldEnabled );
        p->getPropertyValue( "City Blocks", m_cityBlocks );
        p->getPropertyValue( "City Route", m_cityRoute );
        p->getPropertyValue( "City Natural Roads", m_cityNaturalRoads );
        p->getPropertyValue( "Road Authoring Showcase", m_cityRoadShowcase );
        bool terrainMode=m_cityTerrainRoads;p->getPropertyValue( "City Terrain Roads", terrainMode );
        if(!terrainMode && m_cityTerrainRoads && (m_cityRoadAuthoring || !m_cityRoadAuthoringUUID.empty()))
            m_generationError="Leaving terrain-city mode while its saved road/terrain anchors are active is unavailable; use a separate scene for the flat city.";
        else m_cityTerrainRoads=terrainMode;
        String binding=m_cityRoadAuthoringUUID;p->getPropertyValue( "City Road Authoring UUID", binding );
        if(binding!=m_cityRoadAuthoringUUID)
        {m_cityRoadAuthoringUUID=binding;m_cityRoadAuthoring=nullptr;m_cityTerrain=nullptr;m_cityLastAttemptTerrain.reset();}
        if(m_cityTerrainRoads)m_openCity=true;
        m_cityBlocks = std::clamp( m_cityBlocks / 2 * 2, 6, 10 );
        m_cityRoute = std::clamp( m_cityRoute, 0, 2 );

        if( p->isButtonPressed( "Regenerate" ) )
        {
            regenerate();
        }
    }

    SmartPtr<IGameActor> ProceduralRaceScene::getBodyActor() const
    {
        return m_assets.body;
    }

    f32 ProceduralRaceScene::getCircuitLength() const
    {
        return m_assets.circuit.length;
    }

    u32 ProceduralRaceScene::getCircuitSampleCount() const
    {
        return static_cast<u32>( m_assets.circuit.samples.size() );
    }

    const race::SceneAssets &ProceduralRaceScene::getAssets() const
    {
        return m_assets;
    }

    String ProceduralRaceScene::getGenerationError() const
    {
        return m_generationError;
    }

    bool ProceduralRaceScene::isPhysicsConfigured() const
    {
        return m_physicsConfigured;
    }

    void ProceduralRaceScene::setFoliageWorldEnabled( bool enabled )
    {
        m_foliageWorldEnabled = enabled;
    }

    bool ProceduralRaceScene::getFoliageWorldEnabled() const
    {
        return m_foliageWorldEnabled;
    }

    void ProceduralRaceScene::setMaterialExportDirectory( const String &directory )
    {
        m_materialExportDirectory = directory;
    }

    String ProceduralRaceScene::getMaterialExportDirectory() const
    {
        return m_materialExportDirectory;
    }

    bool ProceduralRaceScene::isGenerated() const
    {
        return !m_assets.circuit.samples.empty();
    }

    s32 ProceduralRaceScene::getQuality() const
    {
        return static_cast<s32>( m_quality );
    }

    void ProceduralRaceScene::setSeed( u32 seed )
    {
        m_seed = seed;
    }

    u32 ProceduralRaceScene::getSeed() const
    {
        return m_seed;
    }

    void ProceduralRaceScene::initializePresentation()
    {
        std::lock_guard<std::recursive_mutex> lock( m_presentationMutex );
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
        std::lock_guard<std::recursive_mutex> lock( m_presentationMutex );
        return m_vehicleAudio.isPlaying();
    }

    bool ProceduralRaceScene::isEffectsAvailable() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_presentationMutex );
        return m_vehicleVisualEffects.uploaded();
    }

    u32 ProceduralRaceScene::getParticleCount() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_presentationMutex );
        return u32( m_vehicleVisualEffects.particles() );
    }

    u32 ProceduralRaceScene::getSkidDecalCount() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_presentationMutex );
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

        if((m_assets.cityRoadShowcase || m_assets.cityTerrainRoads) && m_assets.roadAuthoring)
        {
            f32 height=0;bool driveable=false;
            return m_assets.roadAuthoring->getRuntimeSurface(position.x,position.z,height,driveable) && driveable && std::abs(position.y-height)<1.f;
        }

        if( m_assets.openCity )
        {
            return m_cityLayout.roadDistance( position.x, position.z ) < 6.85f;
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
