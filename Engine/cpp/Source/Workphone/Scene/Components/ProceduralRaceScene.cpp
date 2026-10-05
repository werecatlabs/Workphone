#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ProceduralRaceScene, Component );
    ProceduralRaceScene::ProceduralRaceScene() = default;
    ProceduralRaceScene::~ProceduralRaceScene() = default;

    void ProceduralRaceScene::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
            return;
        Component::load( data );
        auto manager = core::IApplicationManager::instance()->getGameManager();
        manager->registerComponentUpdate( TaskId::Physics, Thread::UpdateState::Update, this );
        manager->registerComponentUpdate( TaskId::Physics, Thread::UpdateState::PostUpdate, this );
        manager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Update, this );
        setLoadingState( LoadingState::Loaded );
    }

    void ProceduralRaceScene::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
            return;
        if( auto app = core::IApplicationManager::instancePtr() )
            if( auto manager = app->getGameManager() )
                manager->unregisterAllComponent( this );
        clearGeneratedScene();
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    bool ProceduralRaceScene::regenerate()
    {
        auto actor = getActor();
        if( !actor )
        {
            m_generationError = "Attach ProceduralRaceScene to a vehicle actor before generating.";
            return false;
        }
        // Callers generate while stopped, so replacing the hierarchy cannot race a physics step.
        clearGeneratedScene();
        try
        {
            auto app = core::IApplicationManager::instance();
            auto physics = app->getPhysicsManager();
            if( !physics )
                throw std::runtime_error( "ProceduralRaceScene requires a physics manager." );
            if( !physics->getPhysicsScene() )
                physics->setPhysicsScene( physics->addScene() );
            physics->getPhysicsScene()->setGravity( Vector3F( 0, -9.81f, 0 ) );
            if( auto manager = app->getVehicleManager() )
                manager->load( nullptr );

            actor->setSmoothMotion( false );
            if( !actor->getComponent<CollisionBox>() )
                actor->addComponent<CollisionBox>();
            auto body = actor->getComponent<Rigidbody>();
            if( !body )
                body = actor->addComponent<Rigidbody>();
            body->setAngularDamping( 3 );
            body->setLinearDamping( .05f );
            auto car = actor->getComponent<CarController>();
            if( !car )
                car = actor->addComponent<CarController>();
            if( !car->getVehicleController() )
                throw std::runtime_error( "ProceduralRaceScene requires WPVehiclePhysics." );

            auto ground = app->getGameManager()->createActor();
            m_assets.actors.push_back( ground );
            m_assets.ground = ground;
            ground->setName( "Circuit collision plane" );
            ground->setStatic( true );
            ground->setPosition( Vector3F( 0, -.5f, 0 ) );
            ground->addComponent<CollisionBox>()->setExtents( Vector3F( 1100, 1, 1100 ) );
            ground->addComponent<Rigidbody>();
            app->getGameManager()->getCurrentScene()->addActor( ground );
            race::buildScene( m_assets, actor, m_seed, m_quality );
            car->refreshWheels();
            configurePhysics();
            // Scene play can reconstruct the body and controller; configure again after settling.
            m_physicsConfigured = false;
            m_generationError.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            m_generationError = e.what();
            clearGeneratedScene();
            return false;
        }
    }

    void ProceduralRaceScene::clearGeneratedScene()
    {
        setControls( 0, 0, 0 );
        m_resetRequested = false;
        m_physicsConfigured = false;
        m_surfaceGrip = 1;
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        if( !m_assets.treeLODs.empty() )
            if( auto system = dynamic_pointer_cast<LODSystem>( m_assets.treeLODs.front()->getComponentSystem() ) )
                system->clearViewOverride();
        if( manager )
            for( auto it = m_assets.actors.rbegin(); it != m_assets.actors.rend(); ++it )
                manager->destroyActor( *it );
        auto graphics = app ? app->getGraphicsSystem() : nullptr;
        if( graphics )
        {
            auto scene = graphics->getGraphicsScene();
            if( scene && m_assets.sky )
                scene->removeGraphicsObject( m_assets.sky );
            if( scene && m_assets.sun )
                scene->removeGraphicsObject( m_assets.sun );
            auto materials = graphics->getMaterialManager();
            if( materials )
                for( auto material : m_assets.materials )
                    materials->destroyResource( material );
            auto textures = graphics->getTextureManager();
            if( textures )
                for( auto texture : m_assets.textures )
                    textures->destroyResource( texture );
        }
        auto meshes = app ? dynamic_pointer_cast<MeshManager>( app->getMeshManager() ) : nullptr;
        if( meshes )
            for( auto mesh : m_assets.meshes )
                meshes->removeMeshResource( mesh );
        m_assets = {};
    }

    void ProceduralRaceScene::configurePhysics()
    {
        if( isGenerated() && getCarController() && getCarController()->getVehicleController() )
        {
            race::configurePhysics( m_assets, getActor() );
            m_physicsConfigured = true;
        }
    }

    void ProceduralRaceScene::setControls( f32 throttle, f32 brake, f32 steering )
    {
        m_throttle = std::clamp( throttle, 0.f, 1.f );
        m_brake = std::clamp( brake, 0.f, 1.f );
        m_steering = std::clamp( steering, -1.f, 1.f );
    }

    void ProceduralRaceScene::reset() { m_resetRequested = true; }

    void ProceduralRaceScene::performReset()
    {
        setControls( 0, 0, 0 );
        auto actor = getActor();
        if( !actor )
            return;
        if( auto car = getCarController() )
        {
            car->setThrottle( 0 );
            car->setBrake( 0 );
            car->setSteering( 0 );
            if( auto vehicle = car->getVehicleController() )
            {
                vehicle->reset();
                for( s32 channel = 0; channel < 3; ++channel )
                    vehicle->setChannel( channel, 0 );
            }
        }
        const Transform3F spawn( Vector3F( 0, .42f, 0 ), QuaternionF::identity() );
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
        if( !isEnabled() || !isGenerated() )
            return;
        auto app = core::IApplicationManager::instance();
        if( Thread::getCurrentTask() == TaskId::Render )
        {
            getActor()->updateTransform();
            return;
        }
        if( Thread::getCurrentTask() != TaskId::Physics || !app->isPlaying() || app->isPaused() )
            return;
        if( !m_physicsConfigured && app->getTimer()->getTimeSinceSceneLoad() > 3 )
            configurePhysics();
        if( m_resetRequested.exchange( false ) )
            performReset();
        auto car = getCarController();
        if( !car )
            return;
        const auto throttle = m_throttle.load(), brake = m_brake.load(), steering = m_steering.load();
        car->setThrottle( throttle );
        car->setBrake( brake );
        car->setSteering( steering );
        if( auto vehicle = car->getVehicleController() )
        {
            vehicle->setChannel( 0, throttle );
            vehicle->setChannel( 1, brake );
            vehicle->setChannel( 2, steering );
        }
        auto lamp = m_assets.vehicleMaterials[size_t( procedural::VehicleMaterialSlot::RainLight )];
        lamp->setEmissive( ColourF( brake > 0 ? 1.f : .15f, .003f, .001f, 1 ) );
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

    void ProceduralRaceScene::applyAerodynamics()
    {
        auto body = getActor()->getComponent<Rigidbody>();
        auto velocity = body->getLinearVelocity();
        velocity.y = 0;
        const auto speed = velocity.length();
        const auto &aero = m_assets.vehicle.physics.aero;
        auto force = -velocity * float( .5 * aero.airDensityKgPerM3 * aero.referenceAreaM2 *
                                       aero.dragCoefficient * speed );
        force.y = -float( .5 * aero.airDensityKgPerM3 * aero.referenceAreaM2 *
                          aero.downforceCoefficient * speed * speed );
        body->addForce( force );
        auto vehicle = getCarController()->getVehicleController();
        const auto acceleration = 9.81f - force.y / float( m_assets.vehicle.physics.massProperties.massKg );
        for( u32 i = 0; i < 4; ++i )
        {
            auto wheel = vehicle->getWheelController( i );
            auto props = wheel->getProperties();
            props->setProperty( "Contact Acceleration", acceleration );
            wheel->setProperties( props );
        }
    }

    void ProceduralRaceScene::updateSurfaceGrip()
    {
        auto p = getActor()->getPosition();
        auto offset = p - getCircuitPosition( nearestCircuitSample( p ) );
        offset.y = 0;
        const auto grip = offset.length() < 6.85f ? 1.f : offset.length() < 11.f ? .55f : .35f;
        if( grip == m_surfaceGrip )
            return;
        m_surfaceGrip = grip;
        auto vehicle = getCarController()->getVehicleController();
        for( u32 i = 0; i < 4; ++i )
        {
            auto wheel = vehicle->getWheelController( i );
            auto props = wheel->getProperties();
            props->setProperty( "Grip", grip );
            wheel->setProperties( props );
        }
    }

    void ProceduralRaceScene::updateWheelVisuals()
    {
        auto car = getCarController();
        auto vehicle = car ? car->getVehicleController() : nullptr;
        auto physics = core::IApplicationManager::instance()->getPhysicsManager();
        auto scene = physics ? physics->getPhysicsScene() : nullptr;
        if( !vehicle || !scene )
            return;
        for( u32 i = 0; i < 4; ++i )
        {
            auto wheel = vehicle->getWheelController( i );
            if( !wheel || !m_assets.wheels[i] )
                continue;
            auto extension = wheel->getSuspensionTravel();
            auto hit = make_ptr<physics::RaycastHit>();
            hit->setCheckDynamic( false );
            hit->setCheckStatic( true );
            auto mount = wheel->getWorldTransform().getPosition();
            auto up = vehicle->getWorldTransform().up();
            if( scene->castRay( Ray3<real_Num>( mount, -up ), hit ) )
                extension = std::clamp( hit->getDistance() - wheel->getRadius(), 0.f, wheel->getSuspensionTravel() );
            auto position = wheel->getLocalTransform().getPosition();
            position.y -= extension;
            m_assets.wheels[i]->setLocalPosition( position );
        }
    }

    void ProceduralRaceScene::updateShadow()
    {
        if( !m_assets.shadow || !getActor() )
            return;
        auto position = getActor()->getPosition();
        auto forward = getActor()->getWorldTransform().forward();
        auto yaw = std::atan2( -forward.x, -forward.z );
        position.y = 0;
        m_assets.shadow->setPosition( position );
        m_assets.shadow->setOrientation( QuaternionF::eulerDegrees( 0, yaw * 180.f / 3.14159265f, 0 ) );
        m_assets.shadow->setVisible( getActor()->getPosition().y < .8f );
        m_assets.shadow->updateTransform();
    }

    void ProceduralRaceScene::setView( const Vector3F &position, f32 fovRadians, f32 nearClip, f32 lodBias )
    {
        if( m_assets.treeLODs.empty() )
            return;
        auto system = dynamic_pointer_cast<LODSystem>( m_assets.treeLODs.front()->getComponentSystem() );
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
    { m_quality = static_cast<procedural::VehicleAppearanceQuality>( std::clamp( quality, 0, 3 ) ); }
    u32 ProceduralRaceScene::nearestCircuitSample( const Vector3F &position ) const
    { return static_cast<u32>( m_assets.circuit.nearest( position ) ); }
    Vector3F ProceduralRaceScene::getCircuitPosition( u32 index ) const
    { return m_assets.circuit.samples.at( index ).position; }
    Vector3F ProceduralRaceScene::getCircuitRight( u32 index ) const
    { return m_assets.circuit.samples.at( index ).right; }
    f32 ProceduralRaceScene::getWheelbase() const { return float( m_assets.vehicle.physics.wheelbaseM ); }
    f32 ProceduralRaceScene::getMaxSteeringAngle() const { return float( m_assets.vehicle.physics.wheels[0].maxSteerRad ); }
    SmartPtr<IGameActor> ProceduralRaceScene::getWheelActor( u32 index ) const { return m_assets.wheels.at( index ); }
    SmartPtr<CarController> ProceduralRaceScene::getCarController() const
    { auto actor = getActor(); return actor ? actor->getComponent<CarController>() : nullptr; }
    bool ProceduralRaceScene::validateReflection() const { return race::validateReflection( m_assets ); }

    SmartPtr<Properties> ProceduralRaceScene::getProperties() const
    {
        auto p = Component::getProperties();
        p->setProperty( "Seed", m_seed );
        p->setProperty( "Appearance Quality", getQuality() );
        p->setProperty( "Generation Error", m_generationError );
        p->setButtonPressed( "Regenerate", false );
        return p;
    }
    void ProceduralRaceScene::setProperties( SmartPtr<Properties> p )
    {
        if( !p ) return;
        Component::setProperties( p );
        p->getPropertyValue( "Seed", m_seed );
        s32 quality = getQuality();
        p->getPropertyValue( "Appearance Quality", quality );
        setQuality( quality );
        if( p->isButtonPressed( "Regenerate" ) )
            regenerate();
    }
}
