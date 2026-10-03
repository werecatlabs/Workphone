#include "SampleVehicle.h"
#include <Workphone/Workphone.hpp>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>

#if WP_GRAPHICS_SYSTEM_CLAW
#    include <WPGraphics/ClawMesh.hpp>
#    include <WPGraphics/ClawSceneNode.hpp>
#    include <workphone_graphics_object.h>
#endif

#ifdef _WP_STATIC_LIB_
#    include <FBOISInput/FBOISInput.hpp>
#    include <WPSQLite/WPSQLite.hpp>
#    include <WPVehiclePhysics/WPVehiclePhysics.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#    endif

#    if WP_BUILD_PHYSX
#        include <FBPhysx/FBPhysx.hpp>
#    elif WP_BUILD_ODE
#        include <FBODE3/CPhysicsManagerODE.hpp>
#    endif
#endif

namespace workphone
{
    namespace
    {
        constexpr auto physicsTargetFps = 120.0;
        // Run the sample tasks on one executor so controls, reset and camera poses stay coherent.
        constexpr auto activeThreadCount = 0u;
        constexpr hash_type debugTextId = 0x56454800;
        constexpr real_Num vehicleSpawnHeight = static_cast<real_Num>( 2.0 );
        constexpr real_Num cameraDistance = static_cast<real_Num>( 8.0 );
        constexpr real_Num cameraHeight = static_cast<real_Num>( 3.0 );

        Vector3<real_Num> getVehicleSpawnPosition()
        {
            return Vector3<real_Num>::unitY() * vehicleSpawnHeight;
        }

        Vector3<real_Num> getInitialCameraPosition()
        {
            return getVehicleSpawnPosition() +
                   Vector3<real_Num>( static_cast<real_Num>( 0.0 ), cameraHeight, cameraDistance );
        }

        void addBoxMesh( SmartPtr<scene::IGameActor> actor, SmartPtr<render::IMaterial> material )
        {
            auto mesh = actor->addComponent<scene::Mesh>();
            mesh->setMeshPath( "cube_internal.fbmeshbin" );
            actor->addComponent<scene::MeshRenderer>();
            actor->addComponent<scene::Material>()->setMaterial( material );
        }

        SmartPtr<IMeshResource> createWheelMesh()
        {
            // A cylinder centered on the X axle keeps a constant rolling radius while spinning.
            constexpr u32 segments = 32;
            constexpr real_Num radius = 0.35f, halfWidth = 0.125f;
            Array<Vector3<real_Num>> positions, normals;
            Array<Vector2<real_Num>> uvs;
            Array<u32> indices;
            for( u32 i = 0; i <= segments; ++i )
            {
                const auto angle = 2.0f * Math<real_Num>::pi() * i / segments;
                const auto normal = Vector3<real_Num>( 0.0f, Math<real_Num>::Cos( angle ),
                                                      Math<real_Num>::Sin( angle ) );
                for( u32 side = 0; side < 2; ++side )
                {
                    positions.emplace_back( side ? halfWidth : -halfWidth,
                                            normal.Y() * radius, normal.Z() * radius );
                    normals.push_back( normal );
                    uvs.emplace_back( static_cast<real_Num>( side ),
                                      static_cast<real_Num>( i ) / segments );
                }
                if( i < segments )
                {
                    const auto v = 2 * i;
                    for( auto index : std::array<u32, 6>{ v, v + 2, v + 1, v + 1, v + 2, v + 3 } )
                        indices.push_back( index );
                }
            }
            for( u32 side = 0; side < 2; ++side )
            {
                const auto start = static_cast<u32>( positions.size() );
                const auto x = side ? halfWidth : -halfWidth;
                const auto normal = Vector3<real_Num>( side ? 1.0f : -1.0f, 0.0f, 0.0f );
                positions.emplace_back( x, 0.0f, 0.0f );
                normals.push_back( normal );
                uvs.emplace_back( 0.5f, 0.5f );
                for( u32 i = 0; i <= segments; ++i )
                {
                    const auto angle = 2.0f * Math<real_Num>::pi() * i / segments;
                    const auto y = Math<real_Num>::Cos( angle ), z = Math<real_Num>::Sin( angle );
                    positions.emplace_back( x, radius * y, radius * z );
                    normals.push_back( normal );
                    uvs.emplace_back( 0.5f + 0.5f * y, 0.5f + 0.5f * z );
                    if( i < segments )
                        for( auto index : std::array<u32, 3>{ start, start + i + ( side ? 1 : 2 ),
                                                             start + i + ( side ? 2 : 1 ) } )
                            indices.push_back( index );
                }
            }
            auto app = core::IApplicationManager::instance();
            auto manager = dynamic_pointer_cast<MeshManager>( app->getMeshManager() );
            if( !manager )
            {
                manager = make_ptr<MeshManager>();
                app->setMeshManager( manager );
            }
            auto generated = MeshUtil::createMesh( positions, normals, uvs, indices );
            generated->updateAABB( true );
            auto resource = dynamic_pointer_cast<IMeshResource>(
                manager->createOrRetrieve( "__procedural/sample_vehicle_wheel.meshbin" ).first );
            if( !resource )
                throw std::runtime_error( "Could not create the SampleVehicle wheel mesh." );
            resource->setName( resource->getFilePath() );
            resource->setMesh( generated );
            resource->setLoadingState( LoadingState::Loaded );
            return resource;
        }

        bool checkMeshTransforms( SmartPtr<scene::IGameActor> actor,
                                  const Transform3<real_Num> &expected )
        {
            if( auto renderer = actor->getComponent<scene::MeshRenderer>() )
            {
                auto node = renderer->getGraphicsNode();
                if( !node )
                    return false;

                const auto actual = node->getTransform();
                if( ( actual.getPosition() - expected.getPosition() ).length() > 0.01f ||
                    ( actual.getScale() - expected.getScale() ).length() > 0.01f ||
                    ( actual.forward() - expected.forward() ).length() > 0.01f ||
                    ( actual.up() - expected.up() ).length() > 0.01f )
                    return false;

#if WP_GRAPHICS_SYSTEM_CLAW
                auto mesh = dynamic_pointer_cast<render::ClawMesh>( renderer->getGraphicsObject() );
                auto nativeNode = dynamic_pointer_cast<render::ClawSceneNode>( node );
                auto object = mesh ? mesh->getNativeRenderObject() : nullptr;
                if( !object || !nativeNode || !nativeNode->getNativeNode() ||
                    wp_graphics_object_get_owner( object ) != nativeNode->getNativeNode() ||
                    !mesh->getNativeMesh() || wp_graphics_mesh_get_vertex_count( mesh->getNativeMesh() ) == 0 )
                    return false;

                // Read the same owner matrix that the native mesh draw call uses.
                wp_mat4f rendered;
                wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( object ), &rendered );
                Matrix4F expectedMatrix;
                expectedMatrix.makeTransform( expected.getPosition(), expected.getScale(),
                                              expected.getOrientation() );
                for( size_t i = 0; i < 16; ++i )
                    if( Math<real_Num>::Abs( rendered.m[i / 4][i % 4] - expectedMatrix.ptr()[i] ) > 0.01f )
                        return false;
#endif
            }

            for( auto child : actor->getChildren() )
            {
                Transform3<real_Num> childWorld;
                childWorld.transformFromParent( expected, child->getLocalTransform() );
                if( !checkMeshTransforms( child, childWorld ) )
                    return false;
            }

            return true;
        }

    }  // namespace

    SampleVehicle::SampleVehicle()
    {
        setPluginsConfigFilePath( "wp_plugins_samples.cfg" );
    }

    SampleVehicle::~SampleVehicle()
    {
        unload( nullptr );
    }

    void SampleVehicle::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Loading || loadingState == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Thread::setCurrentTask( TaskId::Primary );
            Thread::setCurrentThreadId( Thread::ThreadId::Primary );
            Thread::setTaskFlags( std::numeric_limits<u32>::max() );

            auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
            WP_ASSERT( applicationManager );

            applicationManager->load( data );
            core::IApplicationManager::setInstance( applicationManager );
            m_applicationManager = applicationManager;
            applicationManager->setApplication( this );

            Application::load( data );

            m_inputListener = workphone::make_ptr<InputListener>();
            m_inputListener->setOwner( this );

            auto inputManager = applicationManager->getInputDeviceManager();
            WP_ASSERT( inputManager );
            if( inputManager )
            {
                inputManager->addListener( m_inputListener );
            }

            auto taskManager = applicationManager->getTaskManager();
            WP_ASSERT( taskManager );
            if( auto physicsTask = taskManager->getTask( TaskId::Physics ) )
            {
                physicsTask->setTargetFPS( physicsTargetFps );
            }

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            applicationManager->setPlaying( true );
            applicationManager->setPaused( false );
            sceneManager->play();

            auto car = m_vehicleActor->getComponent<scene::CarController>();
            auto vehicleController = car->getVehicleController();
            car->setMOI( Vector3<real_Num>( 2500.0f, 3000.0f, 1000.0f ) );

            for( u32 i = 0; i < 4; ++i )
            {
                auto wheelController = vehicleController->getWheelController( i );
                wheelController->setDamping( 4000.0f );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            unload( data );
        }
    }

    void SampleVehicle::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            if( m_inputListener )
            {
                if( auto applicationManager = core::IApplicationManager::instance() )
                {
                    if( auto inputManager = applicationManager->getInputDeviceManager() )
                    {
                        inputManager->removeListener( m_inputListener );
                    }
                }

                m_inputListener->unload( data );
                m_inputListener = nullptr;
            }

            m_boxGround = nullptr;
            m_cameraActor = nullptr;
            m_vehicleActor = nullptr;
            m_chassisMeshActor = nullptr;
            for( auto &wheelActor : m_wheelActors )
                wheelActor = nullptr;
            m_cameraController = nullptr;

            if( core::IApplicationManager::instance() )
            {
                Application::unload( data );
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Unloaded );
        }
    }

    void SampleVehicle::update()
    {
        if( Thread::getCurrentTask() == TaskId::Physics )
        {
            if( m_resetRequested.exchange( false ) )
            {
                performReset();
            }

            updateControls();
        }

        // The application camera is a render object, so only touch its scene node from the render
        // task. The camera rig itself is updated by VehicleCameraController on the application task.
        if( Thread::getCurrentTask() == TaskId::Render )
        {
            // Physics listeners publish the root pose between scene updates. Resolve its dirty
            // descendants before drawing, preserving their local offsets and wheel rotations.
            if( m_vehicleActor )
                m_vehicleActor->updateTransform();
            updateRenderCamera();
        }

        Application::update();
        if( Thread::getCurrentTask() == TaskId::Physics )
            updateWheelVisuals();

        if( m_smokeTest && Thread::getCurrentTask() == TaskId::Physics )
        {
            updateSmokeTest();
        }

        if( Thread::getCurrentTask() == TaskId::Render )
        {
            if( m_smokeTest && m_smokePhase > 0 )
            {
                if( !checkMeshTransforms( m_vehicleActor, m_vehicleActor->getWorldTransform() ) )
                {
                    WP_LOG_ERROR( "Vehicle smoke: rendered vehicle hierarchy does not match its pose." );
                    m_smokeTestPassed = false;
                    core::IApplicationManager::instance()->setQuit( true );
                }
            }

            updateDebugText();
        }
    }

    void SampleVehicle::reset()
    {
        m_resetRequested = true;
    }

    void SampleVehicle::setSmokeTest( bool enabled )
    {
        m_smokeTest = enabled;
    }

    bool SampleVehicle::smokeTestPassed() const
    {
        return m_smokeTestPassed;
    }

    void SampleVehicle::performReset()
    {
        if( m_vehicleActor )
        {
            if( auto carController = m_vehicleActor->getComponent<scene::CarController>() )
            {
                carController->setThrottle( 0.0f );
                carController->setBrake( 0.0f );
                carController->setSteering( 0.0f );

                if( auto vehicle = carController->getVehicleController() )
                {
                    vehicle->reset();
                    vehicle->setChannel( static_cast<s32>( vehicle::IVehicle::Input::THROTTLE ), 0.0f );
                    vehicle->setChannel( static_cast<s32>( vehicle::IVehicle::Input::BRAKE ), 0.0f );
                    vehicle->setChannel( static_cast<s32>( vehicle::IVehicle::Input::STEERING ), 0.0f );
                }
            }

            if( auto rigidbody = m_vehicleActor->getComponent<scene::Rigidbody>() )
            {
                rigidbody->setLinearVelocity( Vector3<real_Num>::zero() );
                rigidbody->setAngularVelocity( Vector3<real_Num>::zero() );
                if( auto body = rigidbody->getRigidDynamic() )
                {
                    body->clearForce();
                    body->clearTorque();
                    body->setTransform( Transform3<real_Num>( getVehicleSpawnPosition(),
                                                              Quaternion<real_Num>::identity() ) );
                }
            }

            m_vehicleActor->setPosition( getVehicleSpawnPosition() );
            m_vehicleActor->setOrientation( Quaternion<real_Num>::identity() );
            m_vehicleActor->updateTransform();
        }

        if( m_cameraActor )
        {
            m_cameraActor->setPosition( getInitialCameraPosition() );
            m_cameraActor->lookAt( getVehicleSpawnPosition(), Vector3<real_Num>::unitY() );
            m_cameraActor->updateTransform();
        }
    }

    void SampleVehicle::updateControls()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto input = applicationManager ? applicationManager->getInputDeviceManager() : nullptr;
        auto car = m_vehicleActor ? m_vehicleActor->getComponent<scene::CarController>() : nullptr;
        auto vehicle = car ? car->getVehicleController() : nullptr;
        if( !input || !vehicle )
        {
            return;
        }

        auto throttle = ( input->isKeyPressed( KeyCodes::KEY_KEY_W ) ||
                          input->isKeyPressed( KeyCodes::KEY_UP ) ) ? 1.0f : 0.0f;
        auto brake = ( input->isKeyPressed( KeyCodes::KEY_KEY_S ) ||
                       input->isKeyPressed( KeyCodes::KEY_DOWN ) ) ? 1.0f : 0.0f;
        const auto left = input->isKeyPressed( KeyCodes::KEY_KEY_A ) ||
                          input->isKeyPressed( KeyCodes::KEY_LEFT );
        const auto right = input->isKeyPressed( KeyCodes::KEY_KEY_D ) ||
                           input->isKeyPressed( KeyCodes::KEY_RIGHT );
        auto steering = static_cast<f32>( right ) - static_cast<f32>( left );
        
        if( m_smokeTest )
        {
            throttle = m_smokePhase == 1 || m_smokePhase == 2 ? 1.0f : 0.0f;
            brake = m_smokePhase == 3 ? 1.0f : 0.0f;
            steering = m_smokePhase == 2 ? 0.35f : 0.0f;
        }

        car->setThrottle( throttle );
        car->setBrake( brake );
        car->setSteering( steering );

        vehicle->setChannel( static_cast<s32>( vehicle::IVehicle::Input::THROTTLE ), throttle );
        vehicle->setChannel( static_cast<s32>( vehicle::IVehicle::Input::BRAKE ), brake );
        vehicle->setChannel( static_cast<s32>( vehicle::IVehicle::Input::STEERING ), steering );
    }

    void SampleVehicle::updateWheelVisuals()
    {
        if( !m_vehicleActor )
            return;
        auto car = m_vehicleActor->getComponent<scene::CarController>();
        auto vehicle = car ? car->getVehicleController() : nullptr;
        auto physicsScene = core::IApplicationManager::instance()->getPhysicsManager()->getPhysicsScene();
        if( !vehicle || !physicsScene )
            return;
        for( u32 i = 0; i < m_wheelActors.size(); ++i )
        {
            auto wheel = vehicle->getWheelController( i );
            if( !wheel || !m_wheelActors[i] )
                continue;
            auto extension = wheel->getSuspensionTravel();
            auto hit = make_ptr<physics::RaycastHit>();
            hit->setCheckDynamic( false );
            hit->setCheckStatic( true );
            const auto mount = wheel->getWorldTransform().getPosition();
            const auto up = vehicle->getWorldTransform().up();
            if( physicsScene->castRay( Ray3<real_Num>( mount, -up ), hit ) )
                extension = Math<real_Num>::clamp( hit->getDistance() - wheel->getRadius(),
                                                  0.0f, wheel->getSuspensionTravel() );
            // Apply suspension to the axle actor; offsetting the spinning mesh makes it orbit.
            auto position = wheel->getLocalTransform().getPosition();
            position.Y() -= extension;
            m_wheelActors[i]->setLocalPosition( position );
        }
    }

    void SampleVehicle::updateSmokeTest()
    {
        auto app = core::IApplicationManager::instance();
        // Application allows scene state and resources to settle before stepping physics.
        if( app->getTimer()->getTimeSinceSceneLoad() <= 3.0 )
            return;
        m_smokeTime += Math<f64>::clamp( app->getTimer()->getDeltaTime(), 0.0, 1.0 / 30.0 );
        auto body = m_vehicleActor->getComponent<scene::Rigidbody>()->getRigidDynamic();
        const auto position = body->getTransform().getPosition();
        const auto settling = m_smokePhase == 0 || m_smokePhase == 4;
        const auto phaseDuration = settling ? 3.0 : m_smokePhase == 2 ? 2.0 : 4.0;
        // Ignore the initial drop, then monitor every physics step instead of just phase endpoints.
        if( !settling || m_smokeTime > phaseDuration - 1.0 )
        {
            m_smokeMinHeight = Math<real_Num>::min( m_smokeMinHeight, position.Y() );
            m_smokeMaxHeight = Math<real_Num>::max( m_smokeMaxHeight, position.Y() );
            m_smokePeakVerticalSpeed = Math<real_Num>::max(
                m_smokePeakVerticalSpeed, Math<real_Num>::Abs( body->getLinearVelocity().Y() ) );
        }
        auto horizontalVelocity = body->getLinearVelocity();
        horizontalVelocity.Y() = 0.0f;
        const auto speed = horizontalVelocity.length();
        if( m_smokeTime < phaseDuration )
            return;
        m_smokeTime = 0.0;
        bool passed = true;
        const auto heightRange = m_smokeMaxHeight - m_smokeMinHeight;
        const auto stable = m_smokeMinHeight > 0.0f && heightRange < 0.15f &&
                            m_smokePeakVerticalSpeed < 0.8f;
        switch( m_smokePhase )
        {
        case 0:
            passed = position.Y() > 0.0f && position.Y() < vehicleSpawnHeight && speed < 2.0f;
            m_smokeStartPosition = position;
            break;
        case 1:
            passed = ( position - m_smokeStartPosition ).length() > 1.0f && speed > 0.5f;
            m_smokeStartOrientation = body->getTransform().getOrientation();
            break;
        case 2:
        {
            auto heading = body->getTransform().getOrientation() * Vector3<real_Num>::unitZ();
            auto startHeading = m_smokeStartOrientation * Vector3<real_Num>::unitZ();
            heading.Y() = startHeading.Y() = 0.0f;
            heading.normalise();
            startHeading.normalise();
            passed = ( heading - startHeading ).length() > 0.02f;
            m_smokeDriveSpeed = speed;
            break;
        }
        case 3:
            passed = speed < m_smokeDriveSpeed;
            performReset();
            passed = passed &&
                     ( body->getTransform().getPosition() - getVehicleSpawnPosition() ).length() < 0.01f &&
                     body->getLinearVelocity().length() < 0.01f &&
                     body->getAngularVelocity().length() < 0.01f;
            break;
        case 4:
            passed = position.Y() > 0.0f && position.Y() < vehicleSpawnHeight && speed < 2.0f;
            break;
        }
        passed = passed && stable;
        if( m_smokePhase == 4 )
        {
            m_smokeTestPassed = passed;
            if( passed )
                WP_LOG( "Vehicle smoke: chassis and wheel render matrices match their poses." );
        }
        WP_LOG( String( "Vehicle smoke phase " ) + StringUtil::toString( m_smokePhase ) +
                ( passed ? ": PASS" : ": FAIL" ) + " position=" +
                StringUtil::toString( position ) + " speed=" + StringUtil::toString( speed ) +
                " height range=" + StringUtil::toString( heightRange ) + " peak vertical speed=" +
                StringUtil::toString( m_smokePeakVerticalSpeed ) );
        m_smokeMinHeight = std::numeric_limits<real_Num>::max();
        m_smokeMaxHeight = std::numeric_limits<real_Num>::lowest();
        m_smokePeakVerticalSpeed = 0.0f;
        if( !passed || m_smokePhase == 4 )
        {
            app->setQuit( true );
            return;
        }
        ++m_smokePhase;
    }

    void SampleVehicle::updateDebugText()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager ? applicationManager->getTimer() : nullptr;
        if( !timer || !m_vehicleActor || timer->getTime() < m_nextDebugUpdate )
        {
            return;
        }
        const auto now = timer->getTime();
        m_nextDebugUpdate = now + 0.25;
        auto car = m_vehicleActor->getComponent<scene::CarController>();
        auto vehicle = car ? car->getVehicleController() : nullptr;
        auto rigidbody = m_vehicleActor->getComponent<scene::Rigidbody>();
        const auto velocity = rigidbody ? rigidbody->getLinearVelocity() : Vector3<real_Num>::zero();
        const auto position = m_vehicleActor->getPosition();
        std::ostringstream text;
        text << std::fixed << std::setprecision( 2 );
        text << "Vehicle: " << ( vehicle ? "ready" : "MISSING" )
             << " | " << ( applicationManager->isPlaying() ? "PLAYING" : "STOPPED" ) << '\n';
        text << "Speed: " << velocity.length() * 3.6f << " km/h | vertical: " << velocity.Y() << " m/s\n";
        text << "Throttle: " << ( car ? car->getThrottle() : 0.0f )
             << " | brake: " << ( car ? car->getBrake() : 0.0f )
             << " | steering: " << ( car ? car->getSteering() : 0.0f ) << '\n';
        text << "Position: " << position.X() << ", " << position.Y() << ", " << position.Z();
        if( m_chassisMeshActor )
        {
            auto renderer = m_chassisMeshActor->getComponent<scene::MeshRenderer>();
            if( auto node = renderer->getGraphicsNode() )
                text << " | chassis mesh offset: " << ( node->getTransform().getPosition() - position ).length();
        }
        if( vehicle )
        {
            text << "\nDynamics: " << ( vehicle->getState() == vehicle::IVehicle::State::PLAY ? "PLAY" : "idle" );
            if( auto wheel = vehicle->getWheelController( 0 ) )
            {
                text << " | wheel rpm: " << wheel->getAngularVelocity() * 9.5493f
                     << " | torque: " << wheel->getTorque()
                     << " | suspension extension: "
                     << ( wheel->getLocalTransform().getPosition().Y() -
                          m_wheelActors[0]->getLocalPosition().Y() );
                auto hit = make_ptr<physics::RaycastHit>();
                hit->setCheckDynamic( false );
                hit->setCheckStatic( true );
                const auto wheelPosition = wheel->getWorldTransform().getPosition();
                const auto rayHit = applicationManager->getPhysicsManager()->getPhysicsScene()->castRay(
                    Ray3<real_Num>( wheelPosition, -Vector3<real_Num>::unitY() ), hit );
                text << "\nWheel position: " << StringUtil::toString( wheelPosition )
                     << " | ground ray: " << rayHit << " | distance: " << hit->getDistance();
            }
        }
        if( auto graphics = applicationManager->getGraphicsSystem() )
        {
            if( auto debug = graphics->getDebug() )
            {
                std::istringstream lines( text.str() );
                std::string line;
                for( size_t i = 0; std::getline( lines, line ); ++i )
                {
                    debug->drawText( debugTextId + i,
                                     Vector2<real_Num>( 0.02f, 0.14f + static_cast<f32>( i ) * 0.04f ),
                                     String( line.c_str() ), 0 );
                }
            }
        }
        if( now >= m_nextDebugLog )
        {
            WP_LOG( String( text.str().c_str() ) );
            m_nextDebugLog = now + 2.0;
        }
    }

    void SampleVehicle::createPlugins()
    {
        Application::createPlugins();

#ifndef _WP_STATIC_LIB_
        auto factoryManager = core::IApplicationManager::instance()->getFactoryManager();
        if( !factoryManager->hasFactoryByName( "workphone::CCarController" ) )
        {
            auto job = factoryManager->make_ptr<LoadPluginJob>();
            job->setPluginPath( "WPVehiclePhysics" );
            job->execute();
        }
#endif

#ifdef _WP_STATIC_LIB_
        try
        {
            WP_DEBUG_TRACE;

            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto corePlugin = workphone::make_ptr<WPCore>();
            applicationManager->addPlugin( corePlugin );
            applicationManager->addPlugin( workphone::make_ptr<WPVehiclePhysics>() );

            auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
            applicationManager->addPlugin( databasePlugin );

            auto inputPlugin = workphone::make_ptr<OISInput>();
            applicationManager->addPlugin( inputPlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
            applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
            applicationManager->addPlugin( graphicsPlugin );
#    endif

#    if WP_BUILD_PHYSX
            auto physxPlugin = workphone::make_ptr<physics::FBPhysx>();
            applicationManager->addPlugin( physxPlugin );
#    elif WP_BUILD_ODE
#    endif

            ApplicationUtil::createFactories();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
#endif
    }

    void SampleVehicle::createScene()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );
        if( !physicsManager->getPhysicsScene() )
        {
            physicsManager->setPhysicsScene( physicsManager->addScene() );
        }
        WP_ASSERT( physicsManager->getPhysicsScene() );
        physicsManager->getPhysicsScene()->setGravity( Vector3<real_Num>( 0.0f, -9.81f, 0.0f ) );
        if( auto vehicleManager = applicationManager->getVehicleManager() )
        {
            vehicleManager->load( nullptr );
        }

        if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
        {
            if( auto graphicsScene = graphicsSystem->getGraphicsScene() )
            {
                graphicsScene->setAmbientLight( ColourF::White * 0.25f );
            }
        }

        auto light = ApplicationUtil::createDirectionalLight();
        WP_ASSERT( light );
        light->setOrientation( QuaternionF::eulerDegrees( -60.0f, -30.0f, 0.0f ) );

        auto sampleMaterial = ApplicationUtil::createDefaultMaterial();
        WP_ASSERT( sampleMaterial );
        sampleMaterial->setDiffuse( ColourF( 0.3f, 0.35f, 0.3f, 1.0f ) );

        auto chassisMaterial = ApplicationUtil::createDefaultMaterial();
        chassisMaterial->setDiffuse( ColourF( 0.1f, 0.35f, 0.85f, 1.0f ) );

        auto wheelMaterial = ApplicationUtil::createDefaultMaterial();
        wheelMaterial->setDiffuse( ColourF( 0.08f, 0.08f, 0.08f, 1.0f ) );

        auto markerMaterial = ApplicationUtil::createDefaultMaterial();
        markerMaterial->setDiffuse( ColourF::White );

        // Use explicit collision dimensions; WPPhysics does not scale shapes via local poses.
        m_boxGround = sceneManager->createActor();
        m_boxGround->setName( "Ground" );
        m_boxGround->setStatic( true );
        m_boxGround->setPosition( Vector3<real_Num>( 0.0f, -0.5f, 0.0f ) );

        const auto groundSize = Vector3<real_Num>( 1000.0f, 1.0f, 1000.0f );

        m_boxGround->addComponent<scene::CollisionBox>()->setExtents( groundSize );
        m_boxGround->addComponent<scene::Rigidbody>();
        auto groundMesh = sceneManager->createActor();
        groundMesh->setName( "Ground mesh" );
        groundMesh->setScale( groundSize );
        m_boxGround->addChild( groundMesh );
        addBoxMesh( groundMesh, sampleMaterial );
        scene->addActor( m_boxGround );

        // Road markings give the follow camera a reference for speed and direction.
        for( s32 z = -200; z <= 50; z += 10 )
        {
            for( const auto x : { -4.0f, 4.0f } )
            {
                auto marker = sceneManager->createActor();
                marker->setName( "Road marking" );
                marker->setPosition( Vector3<real_Num>( x, 0.01f, static_cast<real_Num>( z ) ) );
                marker->setScale( Vector3<real_Num>( 0.15f, 0.02f, 5.0f ) );
                addBoxMesh( marker, markerMaterial );
                scene->addActor( marker );
            }
        }

        m_vehicleActor = sceneManager->createActor();
        m_vehicleActor->setName( "Vehicle" );
        m_vehicleActor->setPosition( getVehicleSpawnPosition() );
        // The sample uses one executor. Updating the actor directly from physics also dirties
        // its child transforms, so every mesh is current when the renderer draws the scene.
        m_vehicleActor->setSmoothMotion( false );
        m_vehicleActor->addComponent<scene::CollisionBox>();
        auto chassis = m_vehicleActor->addComponent<scene::Rigidbody>();
        chassis->setAngularDamping( 0.3f );
        chassis->setLinearDamping( 0.05f );
        auto car = m_vehicleActor->addComponent<scene::CarController>();
        if( !car->getVehicleController() )
        {
            throw std::runtime_error( "SampleVehicle requires the WPVehiclePhysics plugin." );
        }

        // Keep the wheel rays outside the chassis collider; box extents are full dimensions.
        const auto chassisSize = Vector3<real_Num>( 1.6f, 0.6f, 4.405f );
        m_vehicleActor->getComponent<scene::CollisionBox>()->setExtents( chassisSize );
        auto chassisMesh = sceneManager->createActor();
        m_chassisMeshActor = chassisMesh;
        chassisMesh->setName( "Chassis mesh" );
        chassisMesh->setLocalPosition( Vector3<real_Num>( 0.0f, 0.0f, -chassisSize.z * 0.5f ) );
        chassisMesh->setScale( chassisSize );

        m_vehicleActor->addChild( chassisMesh );
        addBoxMesh( chassisMesh, chassisMaterial );
        auto wheelResource = createWheelMesh();
        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelActor = sceneManager->createActor();
            m_wheelActors[i] = wheelActor;
            wheelActor->setName( String( "Wheel " ) + StringUtil::toString( i ) );
            m_vehicleActor->addChild( wheelActor );
            wheelActor->addComponent<scene::WheelController>();
            auto wheelMesh = sceneManager->createActor();
            wheelMesh->setName( "Wheel mesh" );
            wheelActor->addChild( wheelMesh );
            wheelMesh->addComponent<scene::Mesh>()->setMeshResource( wheelResource );
            wheelMesh->addComponent<scene::MeshRenderer>();
            wheelMesh->addComponent<scene::Material>()->setMaterial( wheelMaterial );
        }

        scene->addActor( m_vehicleActor );
        scene->registerAllUpdates( m_vehicleActor );

        m_cameraActor = sceneManager->createActor();
        WP_ASSERT( m_cameraActor );
        m_cameraActor->setName( "Vehicle Follow Camera" );
        m_cameraActor->setPosition( getInitialCameraPosition() );
        m_cameraActor->lookAt( getVehicleSpawnPosition(), Vector3<real_Num>::unitY() );

        m_cameraController = m_cameraActor->addComponent<scene::VehicleCameraController>();
        WP_ASSERT( m_cameraController );
        m_cameraController->setTarget( m_vehicleActor );
        m_cameraController->setDistance( cameraDistance );
        m_cameraController->setHeight( cameraHeight );

        scene->addActor( m_cameraActor );
        scene->registerAllUpdates( m_cameraActor );

        updateRenderCamera();

        if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
        {
            if( auto debug = graphicsSystem->getDebug() )
            {
                debug->drawText( 0, Vector2F( 0.02f, 0.02f ),
                                 "W/Up: throttle  S/Down: brake  A/D or Left/Right: steer  R: reset  Esc: quit",
                                 0 );
                debug->drawText( 1, Vector2F( 0.02f, 0.06f ), "Mouse wheel: camera zoom", 0 );
            }
        }
    }

    void SampleVehicle::updateRenderCamera()
    {
        if( m_cameraSceneNode && m_cameraActor )
        {
            if( auto transform = m_cameraActor->getTransform() )
            {
                m_cameraSceneNode->setTransform( transform->getWorldTransform() );
            }
        }
    }

    SampleVehicle::InputListener::InputListener() = default;

    SampleVehicle::InputListener::~InputListener() = default;

    void SampleVehicle::InputListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    Parameter SampleVehicle::InputListener::handleEvent( EventType eventType, hash_type eventValue,
                                                         const Array<Parameter> &arguments,
                                                         SmartPtr<ISharedObject> sender,
                                                         SmartPtr<ISharedObject> object,
                                                         SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::inputEvent )
        {
            auto result = inputEvent( event );
            return Parameter( result );
        }

        return Parameter();
    }

    bool SampleVehicle::InputListener::inputEvent( SmartPtr<IInputEvent> event )
    {
        if( !event )
        {
            return false;
        }

        if( auto owner = getOwner() )
        {
            switch( event->getEventType() )
            {
            case IInputEvent::EventType::Key:
            {
                if( auto keyboardState = event->getKeyboardState() )
                {
                    if( keyboardState->isPressedDown() )
                    {
                        if( keyboardState->getKeyCode() == static_cast<u32>( KeyCodes::KEY_KEY_R ) )
                        {
                            owner->reset();
                            return true;
                        }
                        if( keyboardState->getKeyCode() == static_cast<u32>( KeyCodes::KEY_ESCAPE ) )
                        {
                            core::IApplicationManager::instance()->setQuit( true );
                            return true;
                        }
                    }
                }
            }
            break;
            default:
                break;
            };
        }

        return false;
    }

    void SampleVehicle::InputListener::setPriority( s32 priority )
    {
        m_priority = priority;
    }

    s32 SampleVehicle::InputListener::getPriority() const
    {
        return m_priority;
    }

    SmartPtr<SampleVehicle> SampleVehicle::InputListener::getOwner() const
    {
        return m_owner.lock();
    }

    void SampleVehicle::InputListener::setOwner( SmartPtr<SampleVehicle> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone

int main( int argc, char **argv )
{
    using namespace workphone;

    TypeManager *typeManager = nullptr;
    bool ownsTypeManager = false;
    SmartPtr<SampleVehicle> app;
    bool smokeTest = false;
    String pluginsConfig;
    for( int i = 1; i < argc; ++i )
    {
        if( String( argv[i] ) == "--smoke-test" )
            smokeTest = true;
        else if( String( argv[i] ) == "--plugins" && i + 1 < argc )
            pluginsConfig = argv[++i];
    }
    int exitCode = 1;

    auto unloadApplication = [&app]() {
        if( app )
        {
            if( app->getLoadingState() != LoadingState::Unloaded )
            {
                app->unload( nullptr );
            }

            app = nullptr;
        }

        core::IApplicationManager::setInstance( nullptr );
    };

    auto unloadTypeManager = [&typeManager, &ownsTypeManager]() {
        if( ownsTypeManager && typeManager )
        {
            typeManager->unload();
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
            ownsTypeManager = false;
        }
    };

    try
    {
        typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            typeManager->load();
            TypeManager::setInstance( typeManager );
            ownsTypeManager = true;
        }

        app = workphone::make_ptr<SampleVehicle>();
        app->setActiveThreads( activeThreadCount );
        app->setSmokeTest( smokeTest );
        if( !pluginsConfig.empty() )
            app->setPluginsConfigFilePath( pluginsConfig );
        app->load( nullptr );
        if( app->getLoadingState() != LoadingState::Loaded )
            throw std::runtime_error( "SampleVehicle failed to load." );
        app->run();
        exitCode = smokeTest && !app->smokeTestPassed() ? 1 : 0;

        unloadApplication();
        unloadTypeManager();
    }
    catch( Exception &e )
    {
        unloadApplication();
        unloadTypeManager();
        MessageBoxUtil::show( e.what() );
    }
    catch( std::exception &e )
    {
        unloadApplication();
        unloadTypeManager();
        MessageBoxUtil::show( e.what() );
    }
    catch( ... )
    {
        unloadApplication();
        unloadTypeManager();
        MessageBoxUtil::show( "Unknown error" );
    }

    return exitCode;
}
