#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Physics/RaycastHit.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED(workphone::scene, ProceduralRaceScene, Component);
    ProceduralRaceScene::ProceduralRaceScene() = default;
    ProceduralRaceScene::~ProceduralRaceScene() = default;

    void ProceduralRaceScene::load(SmartPtr<ISharedObject> data)
    {
        if(isLoaded())
            return;
        Component::load(data);
        auto manager = core::IApplicationManager::instance()->getGameManager();
        manager->registerComponentUpdate(TaskId::Physics, Thread::UpdateState::Update, this);
        manager->registerComponentUpdate(TaskId::Physics, Thread::UpdateState::PostUpdate, this);
        manager->registerComponentUpdate(TaskId::Render, Thread::UpdateState::Update, this);
        setLoadingState(LoadingState::Loaded);
    }

    void ProceduralRaceScene::unload(SmartPtr<ISharedObject> data)
    {
        if(getLoadingState() == LoadingState::Unloaded)
            return;
        if(auto app = core::IApplicationManager::instancePtr())
            if(auto manager = app->getGameManager())
                manager->unregisterAllComponent(this);
        clearGeneratedScene();
        Component::unload(data);
        setLoadingState(LoadingState::Unloaded);
    }

    bool ProceduralRaceScene::regenerate()
    {
        auto tasks = core::IApplicationManager::instance()->getTaskManager();
        // Lua can generate during Play. Synchronize resource/body replacement
        // without changing the Editor's global play flag (which requests Stop).
        auto renderLock = tasks ? tasks->lockTask(TaskId::Render) : TaskLock();
        auto physicsLock = tasks ? tasks->lockTask(TaskId::Physics) : TaskLock();
        std::lock_guard<std::recursive_mutex> presentationLock(m_presentationMutex);
        auto actor = getActor();
        if(!actor)
        {
            m_generationError = "Attach ProceduralRaceScene to a vehicle actor before generating.";
            return false;
        }
        // Rendering and physics are stopped while replacing the hierarchy.
        clearGeneratedScene();
        try
        {
            auto app = core::IApplicationManager::instance();
            auto physics = app->getPhysicsManager();
            if(!physics)
                throw std::runtime_error("ProceduralRaceScene requires a physics manager.");
            if(!physics->getPhysicsScene())
                physics->setPhysicsScene(physics->addScene());
            physics->getPhysicsScene()->setGravity(Vector3F(0, -9.81f, 0));
            if(auto manager = app->getVehicleManager())
                manager->load(nullptr);

            if(!actor->getComponent<CollisionBox>())
                actor->addComponent<CollisionBox>();
            auto body = actor->getComponent<Rigidbody>();
            if(!body)
                body = actor->addComponent<Rigidbody>();
            body->setAngularDamping(3);
            body->setLinearDamping(.05f);
            auto car = actor->getComponent<CarController>();
            if(!car)
                car = actor->addComponent<CarController>();
            if(!car->getVehicleController())
                throw std::runtime_error("ProceduralRaceScene requires WPVehiclePhysics.");

            auto ground = app->getGameManager()->createActor();
            m_assets.actors.push_back(ground);
            m_assets.ground = ground;
            ground->setName("Circuit collision plane");
            ground->setStatic(true);
            ground->setPosition(Vector3F(0, -.5f, 0));
            ground->addComponent<CollisionBox>()->setExtents(Vector3F(1100, 1, 1100));
            ground->addComponent<Rigidbody>();
            app->getGameManager()->getCurrentScene()->addActor(ground);
            if(m_openCity) m_cityLayout = procedural::OpenCityLayout::generate(m_seed, m_cityBlocks, m_cityRoute);
            m_assets.openCity = m_openCity;
            m_assets.cityBlocks = m_cityBlocks;
            m_assets.cityRoute = m_cityRoute;
            race::buildScene(m_assets, actor, m_seed, m_quality);
            // Render the chassis and attached meshes from the same sampled parent pose.
            actor->setSmoothMotion(true, true);
            actor->getTransform()->setTask(TaskId::Physics);
            car->refreshWheels();
            configurePhysics();
            // Scene play can reconstruct the body and controller; configure again after settling.
            m_physicsConfigured = false;
            m_generationError.clear();
            return true;
        }
        catch(const std::exception &e)
        {
            m_generationError = e.what();
            clearGeneratedScene();
            return false;
        }
    }

    void ProceduralRaceScene::clearGeneratedScene()
    {
        std::lock_guard<std::recursive_mutex> presentationLock( m_presentationMutex );
        m_vehicleAudio.unload();
        m_vehicleVisualEffects.unload();
        m_presentationInitialized = false;
        m_presentationResetRequested = false;
        m_presentationAudioClock = 0;
        setControls(0, 0, 0);
        m_resetRequested = false;
        m_physicsConfigured = false;
        m_surfaceGrip = 1;
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        if(m_assets.vehicleLOD)
            if(auto system =
                dynamic_pointer_cast<LODSystem>(m_assets.vehicleLOD->getComponentSystem()))
                system->clearViewOverride();
        if( m_assets.vehicleLOD )
            if( auto actor = getActor() )
                actor->removeComponentInstance( m_assets.vehicleLOD );
        if(manager)
            for(auto it = m_assets.actors.rbegin(); it != m_assets.actors.rend(); ++it)
                manager->destroyActor(*it);
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        if(graphics)
        {
            auto scene = graphics->getGraphicsScene();
            if(scene && m_assets.sky)
                scene->removeGraphicsObject(m_assets.sky);
            if(scene && m_assets.sun)
                scene->removeGraphicsObject(m_assets.sun);
            auto materials = graphics->getMaterialManager();
            if(materials)
                for(auto material : m_assets.materials)
                    materials->destroyResource(material);
            auto textures = graphics->getTextureManager();
            if(textures)
                for(auto texture : m_assets.textures)
                    textures->destroyResource(texture);
        }
        auto meshes = app ? dynamic_pointer_cast<MeshManager>(app->getMeshManager()) : nullptr;
        if(meshes)
            for(auto mesh : m_assets.meshes)
                meshes->removeMeshResource(mesh);
        m_assets = {};
    }

    void ProceduralRaceScene::configurePhysics()
    {
        if(isGenerated() && getCarController() && getCarController()->getVehicleController())
        {
            race::configurePhysics(m_assets, getActor());
            m_physicsConfigured = true;
        }
    }

    void ProceduralRaceScene::setControls(f32 throttle, f32 brake, f32 steering)
    {
        if(auto car = getCarController())
            car->setControls(throttle, brake, steering);
    }

    void ProceduralRaceScene::usePlayerControls()
    {
        if(auto car = getCarController())
            car->usePlayerControls();
    }

    void ProceduralRaceScene::reset()
    {
        m_resetRequested = true;
    }

    void ProceduralRaceScene::performReset()
    {
        m_presentationResetRequested = true;
        setControls(0, 0, 0);
        auto actor = getActor();
        if(!actor)
            return;
        if(auto car = getCarController())
        {
            car->setThrottle(0);
            car->setBrake(0);
            car->setSteering(0);
            if(auto vehicle = car->getVehicleController())
            {
                vehicle->reset();
                for(s32 channel = 0; channel < 3; ++channel)
                    vehicle->setChannel(channel, 0);
            }
        }
        const Transform3F spawn(Vector3F(0, .42f, 0), QuaternionF::identity());
        if(auto rigidbody = actor->getComponent<Rigidbody>())
        {
            rigidbody->setLinearVelocity(Vector3F::zero());
            rigidbody->setAngularVelocity(Vector3F::zero());
            if(auto body = rigidbody->getRigidDynamic())
            {
                body->clearForce();
                body->clearTorque();
                body->setTransform(spawn);
            }
        }
        actor->setPosition(spawn.getPosition());
        actor->setOrientation(spawn.getOrientation());
        actor->updateTransform();
    }

    void ProceduralRaceScene::update()
    {
        auto app = core::IApplicationManager::instance();
        if(Thread::getCurrentTask() == TaskId::Render)
        {
            // Generation may hold scene/graphics locks on the application task.
            // Skip presentation rather than wait with render locks held.
            std::unique_lock<std::recursive_mutex> presentationLock(m_presentationMutex, std::try_to_lock);
            if (!presentationLock.owns_lock() || !isGenerated() || !getActor()) return;
            getActor()->updateTransform();
            if ( m_presentationResetRequested.exchange( false ) ) m_vehicleVisualEffects.reset();
            if ( m_automaticPresentation )
            {
                initializePresentation();
                const auto dt = float( app->getTimer()->getDeltaTime() );
                if ( std::isfinite( dt ) && dt > 0 )
                {
                    m_presentationAudioClock += std::min( dt, .05f );
                    if ( m_presentationAudioClock >= 1.f / 30.f )
                    {
                        m_vehicleAudio.update( sampleVehicleAudio(), m_presentationAudioClock );
                        m_presentationAudioClock = 0;
                    }
                    m_vehicleVisualEffects.update( sampleVehicleEffects(), dt );
                }
            }
            return;
        }
        if ( !isGenerated() || !isEnabled() || Thread::getCurrentTask() != TaskId::Physics || !app->isPlaying() ||
             app->isPaused() )
            return;
        auto car = getCarController();
        // Play rebuilds the wheel setup. Apply tuning after that transition.
        if(!m_physicsConfigured && car && car->getState() == State::Play &&
           app->getTimer()->getTimeSinceSceneLoad() > 3)
            configurePhysics();
        if(m_resetRequested.exchange(false))
            performReset();
        if(!car)
            return;
        auto lamp = m_assets.vehicleMaterials[static_cast<size_t>(
            procedural::VehicleMaterialSlot::RainLight)];
        const ColourF emissive(car->getBrake() > 0 ? 1.f : .15f, .003f, .001f, 1);
        if(lamp && lamp->getEmissive() != emissive)
            lamp->setEmissive(emissive);

        if(m_physicsConfigured)
        {
            updateSurfaceGrip();
            applyAerodynamics();
        }
    }

    void ProceduralRaceScene::postUpdate()
    {
        if(Thread::getCurrentTask() == TaskId::Physics && isGenerated() && isEnabled())
        {
            updateWheelVisuals();
            updateShadow();
        }
    }

    void ProceduralRaceScene::applyAerodynamics()
    {
        auto body = getActor()->getComponent<Rigidbody>();
        auto velocity = body->getLinearVelocity();
        velocity.y = 0;
        const auto speed = velocity.length();
        const auto &aero = m_assets.vehicle.physics.aero;
        auto force = -velocity * static_cast<float>(.5 * aero.airDensityKgPerM3 * aero.referenceAreaM2 *
                                                    aero.dragCoefficient * speed);
        force.y = -static_cast<float>(.5 * aero.airDensityKgPerM3 * aero.referenceAreaM2 *
                                      aero.downforceCoefficient * speed * speed);
        body->addForce(force);
        auto vehicle = getCarController()->getVehicleController();
        const auto acceleration =
            9.81f - force.y / static_cast<float>(m_assets.vehicle.physics.massProperties.massKg);
        for(u32 i = 0; i < 4; ++i)
        {
            auto wheel = vehicle->getWheelController(i);
            wheel->setContactAcceleration(acceleration);
        }
    }

    void ProceduralRaceScene::updateSurfaceGrip()
    {
        auto p = getActor()->getPosition();
        float roadDistance;
        if(m_assets.openCity)
            roadDistance = m_cityLayout.roadDistance(p.x, p.z);
        else
        {
            auto offset = p - getCircuitPosition(nearestCircuitSample(p));
            offset.y = 0;
            roadDistance = offset.length();
        }
        const auto grip = roadDistance < 6.85f ? 1.f : roadDistance < 11.f ? .55f : .35f;
        if(grip == m_surfaceGrip)
            return;
        m_surfaceGrip = grip;
        auto vehicle = getCarController()->getVehicleController();
        for(u32 i = 0; i < 4; ++i)
        {
            auto wheel = vehicle->getWheelController(i);
            // Preserve the authored tyre multiplier when changing surface; the old
            // path silently replaced 1.25 with 1.0 on the first physics tick.
            wheel->setGrip(grip * 1.25f);
        }
    }

    void ProceduralRaceScene::updateWheelVisuals()
    {
        auto car = getCarController();
        auto vehicle = car ? car->getVehicleController() : nullptr;
        if(!vehicle)
            return;
        for(u32 i = 0; i < 4; ++i)
        {
            auto wheel = vehicle->getWheelController(i);
            if(!wheel || !m_assets.wheels[i])
                continue;
            // Reuse the contact solver's raycast result instead of casting four
            // more rays and allocating four hit objects every physics step.
            const auto extension =
                wheel->getSuspensionTravel() * (1 - std::clamp(wheel->getCompression(), 0.f, 1.f));
            auto position = wheel->getLocalTransform().getPosition();
            position.y -= extension;
            m_assets.wheels[i]->setLocalPosition(position);
        }
    }

    void ProceduralRaceScene::updateShadow()
    {
        if(!m_assets.shadow || !getActor())
            return;
        auto position = getActor()->getPosition();
        auto forward = getActor()->getWorldTransform().forward();
        auto yaw = std::atan2(-forward.x, -forward.z);
        position.y = 0;
        m_assets.shadow->setPosition(position);
        m_assets.shadow->setOrientation(QuaternionF::eulerDegrees(0, yaw * 180.f / 3.14159265f, 0));
        m_assets.shadow->setVisible(getActor()->getPosition().y < .8f);
        m_assets.shadow->updateTransform();
    }

    void ProceduralRaceScene::setView(const Vector3F &position, f32 fovRadians, f32 nearClip,
                                      f32 lodBias)
    {
        if(!m_assets.vehicleLOD)
            return;
        auto system = dynamic_pointer_cast<LODSystem>(m_assets.vehicleLOD->getComponentSystem());
        if(system)
        {
            LODSystem::View view;
            view.position = position;
            view.verticalFovRadians = fovRadians;
            view.nearClipDistance = nearClip;
            view.lodBias = lodBias;
            system->setViewOverride(view);
        }
    }

    void ProceduralRaceScene::setQuality(s32 quality)
    {
        m_quality = static_cast<procedural::VehicleAppearanceQuality>(std::clamp(quality, 0, 3));
    }

    u32 ProceduralRaceScene::nearestCircuitSample(const Vector3F &position) const
    {
        return static_cast<u32>(m_assets.circuit.nearest(position));
    }

    Vector3F ProceduralRaceScene::getCircuitPosition(u32 index) const
    {
        return m_assets.circuit.samples.at(index).position;
    }

    Vector3F ProceduralRaceScene::getCircuitRight(u32 index) const
    {
        return m_assets.circuit.samples.at(index).right;
    }

    f32 ProceduralRaceScene::getWheelbase() const
    {
        return static_cast<float>(m_assets.vehicle.physics.wheelbaseM);
    }

    f32 ProceduralRaceScene::getMaxSteeringAngle() const
    {
        return static_cast<float>(m_assets.vehicle.physics.wheels[0].maxSteerRad);
    }

    SmartPtr<IGameActor> ProceduralRaceScene::getWheelActor(u32 index) const
    {
        return m_assets.wheels.at(index);
    }

    SmartPtr<CarController> ProceduralRaceScene::getCarController() const
    {
        auto actor = getActor();
        return actor ? actor->getComponent<CarController>() : nullptr;
    }

    bool ProceduralRaceScene::validateReflection() const
    {
        return race::validateReflection(m_assets);
    }

    SmartPtr<Properties> ProceduralRaceScene::getProperties() const
    {
        auto p = Component::getProperties();
        p->setProperty("Seed", m_seed);
        p->setProperty("Appearance Quality", getQuality());
        p->setProperty( "Audio Enabled", getAudioEnabled() );
        p->setProperty( "Effects Enabled", getEffectsEnabled() );
        p->setProperty("Open City", m_openCity);
        p->setProperty("City Blocks", m_cityBlocks);
        p->setProperty("City Route", m_cityRoute);
        p->setProperty("Generation Error", m_generationError);
        p->setButtonPressed("Regenerate", false);
        return p;
    }

    void ProceduralRaceScene::setProperties(SmartPtr<Properties> p)
    {
        if(!p)
            return;
        Component::setProperties(p);
        p->getPropertyValue("Seed", m_seed);
        s32 quality = getQuality();
        p->getPropertyValue("Appearance Quality", quality);
        setQuality(quality);
        bool audio = getAudioEnabled(), effects = getEffectsEnabled();
        p->getPropertyValue( "Audio Enabled", audio );
        p->getPropertyValue( "Effects Enabled", effects );
        setAudioEnabled( audio );
        setEffectsEnabled( effects );
        p->getPropertyValue("Open City", m_openCity);
        p->getPropertyValue("City Blocks", m_cityBlocks);
        p->getPropertyValue("City Route", m_cityRoute);
        m_cityBlocks = std::clamp(m_cityBlocks / 2 * 2, 6, 10);
        m_cityRoute = std::clamp(m_cityRoute, 0, 2);
        if(p->isButtonPressed("Regenerate"))
            regenerate();
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
        return static_cast<u32>(m_assets.circuit.samples.size());
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

    bool ProceduralRaceScene::isGenerated() const
    {
        return !m_assets.circuit.samples.empty();
    }

    s32 ProceduralRaceScene::getQuality() const
    {
        return static_cast<s32>(m_quality);
    }

    void ProceduralRaceScene::setSeed(u32 seed)
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
        if ( !isGenerated() ) return;
        auto app = core::IApplicationManager::instancePtr();
        if ( !app ) return;
        const bool audio = m_audioEnabled, effects = m_effectsEnabled;
        if ( !m_presentationInitialized || audio != m_appliedAudioEnabled )
        {
            m_vehicleAudio.unload();
            if ( audio && !m_vehicleAudio.load( app->getSoundManager() ) )
                WP_LOG_WARNING(
                    "Race vehicle audio unavailable: check the output device and bundled WAV "
                    "files." );
            m_appliedAudioEnabled = audio;
        }
        if ( !m_presentationInitialized || effects != m_appliedEffectsEnabled )
        {
            m_vehicleVisualEffects.unload();
            if ( effects && !m_vehicleVisualEffects.load( getQuality() == 0   ? 0u
                                                          : getQuality() == 1 ? 1u
                                                                              : 2u,
                                                          m_seed ) )
                WP_LOG_WARNING(
                    "Race vehicle visual effects unavailable in the active graphics backend." );
            m_appliedEffectsEnabled = effects;
        }
        m_presentationInitialized = true;
    }
    void ProceduralRaceScene::setAudioEnabled( bool enabled ) { m_audioEnabled = enabled; }
    bool ProceduralRaceScene::getAudioEnabled() const { return m_audioEnabled; }
    void ProceduralRaceScene::setEffectsEnabled( bool enabled ) { m_effectsEnabled = enabled; }
    bool ProceduralRaceScene::getEffectsEnabled() const { return m_effectsEnabled; }
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
    advanced::VehicleAudio& ProceduralRaceScene::getVehicleAudio() { return m_vehicleAudio; }
    advanced::VehicleVisualEffects& ProceduralRaceScene::getVehicleVisualEffects()
    {
        return m_vehicleVisualEffects;
    }
    bool ProceduralRaceScene::isRoadSurface( const Vector3F& position ) const
    {
        if ( !isGenerated() ) return false;
        if ( m_assets.openCity ) return m_cityLayout.roadDistance( position.x, position.z ) < 6.85f;
        auto offset = position - getCircuitPosition( nearestCircuitSample( position ) );
        offset.y = 0;
        return offset.length() < 6.85f;
    }
    advanced::VehicleEffectsFrame ProceduralRaceScene::sampleVehicleEffects() const
    {
        auto app = core::IApplicationManager::instancePtr();
        if ( !app || !getActor() || !isGenerated() )
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
        if ( !body || !vehicle || !vehicle->getBody() )
        {
            frame.playing = false;
            return frame;
        }
        frame.velocity = body->getLinearVelocity();
        for ( u32 i = 0; i < 4; ++i )
        {
            auto wheel = vehicle->getWheelController( i );
            if ( !wheel ) continue;
            auto properties = wheel->getProperties();
            bool grounded = false;
            double slip = 0;
            properties->getPropertyValue( "Is On Ground", grounded );
            properties->getPropertyValue( "Slip Velocity", slip );
            frame.slip[i] = float( slip );
            frame.width[i] = float( m_assets.vehicle.physics.wheels[i].tire.widthM * .82 );
            if ( !grounded ) continue;
            const auto hub = wheel->getWorldTransform().getPosition();
            SmartPtr<physics::IRaycastHit> hit = make_ptr<physics::RaycastHit>();
            // Reuse the vehicle callback, which excludes its own chassis.
            if ( vehicle->getBody()->castWorldRay(
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
        if ( !app || !getActor() || !isGenerated() )
        {
            advanced::VehicleAudioInput empty;
            empty.playing = false;
            return empty;
        }
        advanced::VehicleAudioInput input;
        const auto& drivetrain = m_assets.vehicle.physics.drivetrain;
        input.idleRpm = float( drivetrain.idleRpm );
        input.redlineRpm = float( drivetrain.redlineRpm );
        input.playing = app->isPlaying() && !app->isPaused();
        if ( auto car = getActor()->getComponent<scene::CarController>() )
        {
            if ( auto vehicle = car->getVehicleController() )
            {
                input.throttle = vehicle->getChannel( 0 );
                if ( auto drive = vehicle->getDriveTrain() )
                {
                    double rpm = 0;
                    drive->getProperties()->getPropertyValue( "RPM", rpm );
                    input.rpm = float( rpm );
                }
                for ( u32 i = 0; i < 4; ++i )
                    if ( auto wheel = vehicle->getWheelController( i ) )
                    {
                        auto properties = wheel->getProperties();
                        bool grounded = false;
                        double slip = 0;
                        properties->getPropertyValue( "Is On Ground", grounded );
                        properties->getPropertyValue( "Slip Velocity", slip );
                        if ( grounded )
                        {
                            input.grounded = true;
                            input.slipSpeed = std::max( input.slipSpeed, float( slip ) );
                        }
                    }
            }
        }
        if ( auto body = getActor()->getComponent<scene::Rigidbody>() )
            input.speed = float( body->getLinearVelocity().length() );

        input.playing = input.playing && isEnabled();
        return input;
    }
} // namespace workphone::scene
