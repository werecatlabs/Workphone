#include "SampleVehicleAdvanced.h"
#include "ProceduralScene.h"
#include "FrameCapture.h"
#include <WPProcedural/WPProcedural.hpp>
#include <Workphone/Workphone.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Scene/Systems/LODSystem.hpp>

#if WP_GRAPHICS_SYSTEM_CLAW
#    include <WPGraphics/ClawMesh.hpp>
#    include <WPGraphics/ClawRendererDX11.hpp>
#    include <workphone_graphics_renderer.h>
#    include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
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
        constexpr real_Num vehicleSpawnHeight = static_cast<real_Num>( 0.42 );
        constexpr real_Num cameraDistance = static_cast<real_Num>( 8.0 );
        constexpr real_Num cameraHeight = static_cast<real_Num>( 3.0 );

        bool checkSceneWinding( const advanced::SceneAssets &assets )
        {
            size_t checked = 0;
            for( const auto &resource : assets.meshes )
            {
                for( const auto &section : resource->getMesh()->getSubMeshes() )
                {
                    double alignment = 0;
                    auto vertices = section->getVertexBuffer();
                    auto declaration = vertices->getVertexDeclaration();
                    auto position = declaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                    auto normal = declaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                    auto indices = section->getIndexBuffer();
                    if( !position || !normal || !indices || indices->getNumIndices() % 3 != 0 )
                        return false;
                    auto readVector = [&]( u32 index, u32 offset ) {
                        const auto data = reinterpret_cast<const f32 *>(
                            static_cast<const u8 *>( vertices->getVertexData() ) +
                            size_t( index ) * declaration->getSize() + offset );
                        return Vector3F( data[0], data[1], data[2] );
                    };
                    auto readIndex = [&]( u32 index ) -> u32 {
                        return indices->getIndexType() == IIndexBuffer::Type::IT_16BIT
                                   ? static_cast<const u16 *>( indices->getIndexData() )[index]
                                   : static_cast<const u32 *>( indices->getIndexData() )[index];
                    };
                    for( u32 i = 0; i < indices->getNumIndices(); i += 3 )
                    {
                        const auto a = readIndex( i ), b = readIndex( i + 1 ), c = readIndex( i + 2 );
                        if( a >= vertices->getNumVertices() || b >= vertices->getNumVertices() ||
                            c >= vertices->getNumVertices() )
                            return false;
                        const auto face = ( readVector( b, position->getOffset() ) -
                                            readVector( a, position->getOffset() ) )
                                              .crossProduct( readVector( c, position->getOffset() ) -
                                                             readVector( a, position->getOffset() ) );
                        const auto outward = readVector( a, normal->getOffset() ) +
                                             readVector( b, normal->getOffset() ) +
                                             readVector( c, normal->getOffset() );
                        // Smooth normals at narrow/concave car seams can oppose an individual
                        // face. Area-weighted section alignment detects a reversed upload without
                        // requiring every authored smooth normal to equal its triangle normal.
                        alignment += face.dotProduct( outward );
                        ++checked;
                    }
                    if( alignment <= 0 )
                    {
                        WP_LOG_ERROR( "VehicleAdvanced: reversed winding in " + resource->getFilePath() );
                        return false;
                    }
                }
            }
            WP_LOG( "VehicleAdvanced outward winding validated: triangles=" + StringUtil::toString( checked ) );
            return checked > 0;
        }

        Vector3<real_Num> getVehicleSpawnPosition()
        {
            return Vector3<real_Num>::unitY() * vehicleSpawnHeight;
        }

        Vector3<real_Num> getInitialCameraPosition()
        {
            return getVehicleSpawnPosition() +
                   Vector3<real_Num>( static_cast<real_Num>( 0.0 ), cameraHeight, cameraDistance );
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
                {
                    WP_LOG_ERROR( "Vehicle scene-node mismatch: " + actor->getName() +
                                  " position error=" + StringUtil::toString( ( actual.getPosition() - expected.getPosition() ).length() ) +
                                  " orientation error=" + StringUtil::toString( ( actual.forward() - expected.forward() ).length() ) );
                    return false;
                }

#if WP_GRAPHICS_SYSTEM_CLAW
                auto mesh = dynamic_pointer_cast<render::ClawMesh>( renderer->getGraphicsObject() );
                auto nativeNode = dynamic_pointer_cast<render::ClawSceneNode>( node );
                auto object = mesh ? mesh->getNativeRenderObject() : nullptr;
                if( !object || !nativeNode || !nativeNode->getNativeNode() ||
                    wp_graphics_object_get_owner( object ) != nativeNode->getNativeNode() ||
                    !mesh->getNativeMesh() ||
                    wp_graphics_mesh_get_vertex_count( mesh->getNativeMesh() ) == 0 )
                    return false;

                // Read the same owner matrix that the native mesh draw call uses.
                wp_mat4f rendered;
                wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( object ), &rendered );
                Matrix4F expectedMatrix;
                expectedMatrix.makeTransform( expected.getPosition(), expected.getScale(),
                                              expected.getOrientation() );
                for( size_t i = 0; i < 16; ++i )
                    if( Math<real_Num>::Abs( rendered.m[i / 4][i % 4] - expectedMatrix.ptr()[i] ) >
                        0.01f )
                    {
                        WP_LOG_ERROR( "Vehicle render mismatch: " + actor->getName() +
                                      " matrix element " + StringUtil::toString( i ) +
                                      " actual=" + StringUtil::toString( rendered.m[i / 4][i % 4] ) +
                                      " expected=" + StringUtil::toString( expectedMatrix.ptr()[i] ) );
                        return false;
                    }
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

    SampleVehicleAdvanced::SampleVehicleAdvanced()
    {
        setPluginsConfigFilePath( "wp_plugins_samples.cfg" );
    }

    SampleVehicleAdvanced::~SampleVehicleAdvanced()
    {
        unload( nullptr );
    }

    void SampleVehicleAdvanced::load( SmartPtr<ISharedObject> data )
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
            // Application::load logs scene exceptions instead of propagating them.
            // Do not enter Play with a partially generated vehicle/circuit.
            if( !m_raceScene || !m_raceScene->isGenerated() || m_assets.circuit.samples.empty() ||
                !m_cameraActor )
            {
                const auto error = m_raceScene ? m_raceScene->getGenerationError() : String();
                throw std::runtime_error( error.empty() ? "Vehicle scene initialization failed." : error.c_str() );
            }

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
            m_raceScene->configurePhysics();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            unload( data );
        }
    }

    void SampleVehicleAdvanced::unload( SmartPtr<ISharedObject> data )
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
            m_raceScene = nullptr;
            m_assets = {};

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

    void SampleVehicleAdvanced::update()
    {
        if( Thread::getCurrentTask() == TaskId::Physics )
        {
            if( !m_physicsConfigured &&
                core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad() > 3 )
            {
                m_raceScene->configurePhysics();
                m_physicsConfigured = true;
            }
            if( m_resetRequested.exchange( false ) )
            {
                performReset();
            }

            updateControls();

        }

        // The application camera is a render object, so only touch its scene node from the render
        // task. VehicleCameraController advances the smooth rig on the render task.
        if( Thread::getCurrentTask() == TaskId::Render )
        {
            // Physics listeners publish the root pose between scene updates. Resolve its dirty
            // descendants before drawing, preserving their local offsets and wheel rotations.
            if( m_vehicleActor )
                m_vehicleActor->updateTransform();
        }

        Application::update();
        if( m_smokeTest && Thread::getCurrentTask() == TaskId::Physics )
        {
            updateSmokeTest();
        }

        if( Thread::getCurrentTask() == TaskId::Render )
        {
            updateRenderCamera();
            if( m_smokeTest && m_smokePhase > 0 )
            {
                auto car = m_vehicleActor->getComponent<scene::CarController>();
                auto vehicle = car->getVehicleController();
                // Check the sustained turn and return to centre after the application
                // task has published the pose; input changes can precede it by one frame.
                for( u32 i = 0; m_smokePhase >= 2 && m_smokeTime > .5 && i < 2; ++i )
                {
                    const auto angle = vehicle->getWheelController( i )->getSteeringAngle();
                    const auto expectedAngle = float( vehicle->getChannel(
                        s32( vehicle::IVehicle::Input::STEERING ) ) ) *
                        float( m_assets.vehicle.physics.wheels[i].maxSteerRad * 180.0 /
                               3.14159265358979323846 );
                    // The assisted controller can be travelling toward the input
                    // or applying its speed limit. Check the authored lock rather
                    // than requiring an instantaneous, unfiltered input angle.
                    if( !std::isfinite(float(angle)) || std::abs(float(angle)) >
                        float(m_assets.vehicle.physics.wheels[i].maxSteerRad * 180.0 /
                              3.14159265358979323846) + .01f )
                    {
                        WP_LOG_ERROR( "Vehicle smoke: steering input lost its configured angle scale." );
                        m_smokeTestPassed = false;
                        core::IApplicationManager::instance()->setQuit( true );
                    }
                    const auto expectedAxle =
                        QuaternionF::eulerDegrees( 0, -float( angle ), 0 ) * Vector3F::unitX();
                    const auto renderedAxle =
                        m_wheelActors[i]->getLocalOrientation() * Vector3F::unitX();
                    // Spin about the axle cannot affect this comparison: it verifies
                    // that the visible steering frame matches the tyre contact frame.
                    // Physics can advance after the render wheel pose was
                    // published. Allow one tick of the configured steering slew.
                    const auto steeringTolerance = .002f + float(90.0 *
                        std::clamp(core::IApplicationManager::instance()->getTimer()->getDeltaTime(TaskId::Physics),
                                   0.0, 1.0 / 30.0) * 3.14159265358979323846 / 180.0);
                    if( ( expectedAxle - renderedAxle ).length() > steeringTolerance )
                    {
                        WP_LOG_ERROR( "Vehicle smoke: rendered steering disagrees with wheel physics." );
                        m_smokeTestPassed = false;
                        core::IApplicationManager::instance()->setQuit( true );
                    }
                }
                auto app = core::IApplicationManager::instance();
                auto timer = app->getTimer();
                auto transform = m_vehicleActor->getTransform();
                auto expectedPose = m_vehicleActor->getWorldTransform();
                app->getGameManager()->getTransformState(
                    m_vehicleActor->getHandle()->getInstanceId(),
                    timer->getTime( TaskId::Render ) - scene::IGameManager::smoothMotionDelay,
                    timer->getDeltaTime( TaskId::Render ), expectedPose, transform->getTask() );
                if( !checkMeshTransforms( m_vehicleActor, expectedPose ) )
                {
                    WP_LOG_ERROR( "Vehicle smoke: rendered vehicle hierarchy does not match its pose." );
                    m_smokeTestPassed = false;
                    core::IApplicationManager::instance()->setQuit( true );
                }
            }

            updateDebugText();
            auto app = core::IApplicationManager::instance();
            const auto profileNow = app->getTimer()->getTimeSinceSceneLoad();
            if( profileNow > 5 && !m_benchmarkStarted )
            {
                m_benchmarkStarted = true;
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
                if( auto renderer = dynamic_cast<render::ClawRendererDX11 *>(
                        app->getGraphicsSystem()->getRendererPtr() ) )
                    wp_renderer_dx11_reset_statistics(
                        wp_renderer_get_dx11( renderer->getNativeRenderer() ) );
#endif
            }
            if( profileNow > 5 )
            {
                if( m_profileStart == 0 )
                    m_profileStart = profileNow;
                ++m_profileFrames;
            }
            if( ( !m_capturePath.empty() || m_benchmarkSeconds > 0 ) && !m_captureAttempted &&
                profileNow > ( m_benchmarkSeconds > 0 ? 5 + m_benchmarkSeconds : 8 ) )
            {
                size_t treeMeshes = 0, treeImposters = 0;
                bool treeLODValid = !m_assets.treeLODs.empty();
                for( const auto &group : m_assets.treeLODs )
                {
                    auto levels = group->getLevels();
                    treeLODValid &=
                        levels[0].renderers[0]->isLODVisible() != levels[1].renderers[0]->isLODVisible();
                    treeMeshes += levels[0].renderers[0]->isLODVisible() ? 1 : 0;
                    treeImposters += levels[1].renderers[0]->isLODVisible() ? 1 : 0;
                }
                WP_LOG( String( "Tree LOD patches: meshes=" ) + StringUtil::toString( treeMeshes ) +
                        " imposters=" + StringUtil::toString( treeImposters ) );
                bool vehicleLODValid = m_assets.vehicleLOD && m_assets.vehicleLOD->validate();
                if( vehicleLODValid )
                {
                    size_t visibleLevels = 0;
                    auto levels = m_assets.vehicleLOD->getLevels();
                    for( size_t i = 0; i < levels.size(); ++i )
                    {
                        const bool visible = levels[i].renderers.front()->isLODVisible();
                        for( const auto &renderer : levels[i].renderers )
                            vehicleLODValid &= renderer->isLODVisible() == visible;
                        if( visible )
                        {
                            ++visibleLevels;
                            WP_LOG( String( "Vehicle LOD level=" ) + StringUtil::toString( i ) );
                        }
                    }
                    vehicleLODValid &= visibleLevels == 1;
                }
                WP_LOG( String( "VehicleAdvanced mean frame milliseconds=" ) +
                        StringUtil::toString( m_profileFrames > 1
                                                  ? 1000 * ( profileNow - m_profileStart ) /
                                                        ( m_profileFrames - 1 )
                                                  : 0 ) +
                        " scene meshes=" + StringUtil::toString( m_assets.meshes.size() ) +
                        " textures=" + StringUtil::toString( m_assets.textures.size() ) );
                m_captureAttempted = true;
#if WP_GRAPHICS_SYSTEM_CLAW && defined( _WIN32 )
                if( auto renderer = dynamic_cast<render::ClawRendererDX11 *>(
                        app->getGraphicsSystem()->getRendererPtr() ) )
                {
                    wp_render_statistics_dx11 stats{};
                    wp_renderer_dx11_get_statistics(
                        wp_renderer_get_dx11( renderer->getNativeRenderer() ), &stats );
                    const auto frames = double( std::max<uint64_t>( stats.frames, 1 ) );
                    std::ostringstream report;
                    report << "VehicleAdvanced benchmark: frames=" << stats.frames
                           << " gpuSamples=" << stats.gpu_samples << " meanMs=" << stats.interval_ms
                           << " p95Ms=" << stats.interval_p95_ms
                           << " cpuIncludingPresentMs=" << stats.cpu_frame_ms
                           << " presentMs=" << stats.present_ms << " gpuMs=" << stats.gpu_frame_ms
                           << " drawsPerFrame=" << stats.draws / frames
                           << " trianglesPerFrame=" << stats.triangles / frames
                           << " materialUploadsPerFrame=" << stats.material_uploads / frames
                           << " transformUploadsPerFrame=" << stats.transform_uploads / frames
                           << " stateBindingsPerFrame=" << stats.state_bindings / frames;
                    WP_LOG( report.str() );
                }
#endif
                const bool reflectionValid = advanced::validateReflection( m_assets );
                if( !reflectionValid )
                    WP_LOG_ERROR(
                        "VehicleAdvanced: cubemap actor, texture or vehicle material binding is "
                        "invalid." );
                const bool windingValid = checkSceneWinding( m_assets );
                m_capturePassed = ( m_capturePath.empty() || advanced::captureFrame( m_capturePath ) ) &&
                                  treeLODValid && vehicleLODValid && reflectionValid && windingValid;
                if( !vehicleLODValid )
                    WP_LOG_ERROR( "Vehicle LOD visibility validation failed." );
                if( !treeLODValid )
                    WP_LOG_ERROR( "Tree LOD: expected exactly one visible level per patch." );
                WP_LOG( m_capturePassed ? "VehicleAdvanced review completed."
                                        : "VehicleAdvanced capture failed." );
                app->setQuit( true );
            }
        }
    }

    void SampleVehicleAdvanced::reset()
    {
        m_resetRequested = true;
    }

    void SampleVehicleAdvanced::setSmokeTest( bool enabled )
    {
        m_smokeTest = enabled;
    }

    bool SampleVehicleAdvanced::smokeTestPassed() const
    {
        return m_smokeTestPassed;
    }

    void SampleVehicleAdvanced::performReset()
    {
        if( m_raceScene ) m_raceScene->setControls( 0, 0, 0 );
        m_lapStart = 0;
        m_nextCheckpoint = 1;
        m_lastTrackIndex = 0;
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

    void SampleVehicleAdvanced::updateControls()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto input = applicationManager ? applicationManager->getInputDeviceManager() : nullptr;
        auto car = m_vehicleActor ? m_vehicleActor->getComponent<scene::CarController>() : nullptr;
        auto vehicle = car ? car->getVehicleController() : nullptr;
        if( !input || !vehicle )
        {
            return;
        }

        float throttle = 0, brake = 0, steering = 0;

        if( m_smokeTest )
        {
            throttle = m_smokePhase == 1 || m_smokePhase == 2 ? 1.0f : 0.0f;
            brake = m_smokePhase == 3 ? 1.0f : 0.0f;
            steering = m_smokePhase == 2 ? 0.35f : 0.0f;
        }

        if( !m_capturePath.empty() || m_benchmarkSeconds > 0 )
        {
            throttle = 0;
            brake = 1;
            steering = 0;
        }

        // At speed, limit requested lateral acceleration rather than allowing a full
        // parking-speed steering angle to overturn the car. Keep full lock for hairpins.
        const auto speed =
            m_vehicleActor->getComponent<scene::Rigidbody>()->getLinearVelocity().length();
        const auto maxSteer = m_assets.vehicle.physics.wheels[0].maxSteerRad;
        const auto steerScale =
            std::min( 1.0, m_assets.vehicle.physics.wheelbaseM * 8.0 /
                               ( std::max( double( speed * speed ), 1.0 ) * maxSteer ) );
        steering *= float( steerScale );
        if( m_trackSmokeTest && m_physicsConfigured )
        {
            const auto position = m_vehicleActor->getPosition();
            const auto &circuit = m_assets.circuit;
            const auto nearest = circuit.nearest( position );
            const auto lookahead = std::max( 8.f, speed * .75f );
            const auto target = circuit
                                    .samples[( nearest + size_t( lookahead * circuit.samples.size() /
                                                                 circuit.length ) ) %
                                             circuit.samples.size()]
                                    .position;
            const auto local = m_vehicleActor->getOrientation().inverse() * ( target - position );
            const auto angle = std::atan2( 2.f * float( m_assets.vehicle.physics.wheelbaseM ) * local.x,
                                           std::max( local.x * local.x + local.z * local.z, 1.f ) );
            steering = float( std::clamp( double( angle ) / maxSteer, -1.0, 1.0 ) );
            const auto wanted = std::clamp( 7.f / ( 1.f + std::abs( angle ) * 2.f ), 3.5f, 7.f );
            throttle = std::clamp( ( wanted - speed ) * .25f, 0.f, .4f );
            brake = std::clamp( ( speed - wanted ) * .12f, 0.f, .4f );
            auto flat = position - circuit.samples[nearest].position;
            flat.y = 0;
            const auto now = applicationManager->getTimer()->getTimeSinceSceneLoad();
            if( position.y < .1f || position.y > 1.f || flat.length() > 12.f || now > 300 )
            {
                WP_LOG_ERROR( String( "VehicleAdvanced full circuit: FAIL position=" ) +
                              StringUtil::toString( position ) +
                              " offset=" + StringUtil::toString( flat.length() ) +
                              " index=" + StringUtil::toString( nearest ) +
                              " steer=" + StringUtil::toString( steering ) );
                applicationManager->setQuit( true );
            }
            else if( m_lap > 1 )
            {
                m_smokeTestPassed = true;
                WP_LOG( "VehicleAdvanced full circuit: PASS." );
                applicationManager->setQuit( true );
            }
        }
        if( m_smokeTest || m_trackSmokeTest || !m_capturePath.empty() )
            m_raceScene->setControls( throttle, brake, steering );
        else
            m_raceScene->usePlayerControls();
    }

    void SampleVehicleAdvanced::updateWheelVisuals()
    {
        m_raceScene->updateWheelVisuals();
    }

    void SampleVehicleAdvanced::updateSmokeTest()
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
        const auto stable =
            m_smokeMinHeight > 0.0f && heightRange < 0.15f && m_smokePeakVerticalSpeed < 0.8f;
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
            passed =
                passed &&
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
                ( passed ? ": PASS" : ": FAIL" ) + " position=" + StringUtil::toString( position ) +
                " speed=" + StringUtil::toString( speed ) +
                " min height=" + StringUtil::toString(m_smokeMinHeight) +
                " max height=" + StringUtil::toString(m_smokeMaxHeight) +
                " orientation=" + StringUtil::toString(body->getTransform().getOrientation()) +
                " angular velocity=" + StringUtil::toString(body->getAngularVelocity()) +
                " height range=" + StringUtil::toString( heightRange ) +
                " peak vertical speed=" + StringUtil::toString( m_smokePeakVerticalSpeed ) );
        if( auto car = m_vehicleActor->getComponent<scene::CarController>() )
            for( u32 i = 0; i < 4; ++i )
            {
                auto wheel = car->getVehicleController()->getWheelController(i);
                float load = 0, acceleration = 0;
                auto props = wheel->getProperties();
                props->getPropertyValue("Normal Force", load);
                props->getPropertyValue("Contact Acceleration", acceleration);
                WP_LOG("Handling wheel " + StringUtil::toString(i) + " compression=" +
                    StringUtil::toString(wheel->getCompression()) + " load=" + StringUtil::toString(load) +
                    " acceleration=" + StringUtil::toString(acceleration));
            }
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

    void SampleVehicleAdvanced::updateDebugText()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager ? applicationManager->getTimer() : nullptr;
        if( !m_hudEnabled || !timer || !m_vehicleActor || m_assets.circuit.samples.empty() ||
            timer->getTime() < m_nextDebugUpdate )
        {
            return;
        }
        const auto now = timer->getTime();
        m_nextDebugUpdate = now + 0.1;
        if( m_lapStart == 0 )
            m_lapStart = now;
        auto car = m_vehicleActor->getComponent<scene::CarController>();
        auto vehicle = car ? car->getVehicleController() : nullptr;
        auto rigidbody = m_vehicleActor->getComponent<scene::Rigidbody>();
        const auto velocity = rigidbody ? rigidbody->getLinearVelocity() : Vector3<real_Num>::zero();
        const auto position = m_vehicleActor->getPosition();
        auto index = m_assets.circuit.nearest( position );
        auto roadOffset = position - m_assets.circuit.samples[index].position;
        roadOffset.y = 0;
        const auto onRoad = roadOffset.length() < 6.85f;
        if( m_lastTrackIndex > m_assets.circuit.samples.size() * 3 / 4 &&
            index < m_assets.circuit.samples.size() / 4 && m_nextCheckpoint == 4 && onRoad )
        {
            m_lastLapTime = now - m_lapStart;
            if( m_bestLapTime == 0 || m_lastLapTime < m_bestLapTime )
                m_bestLapTime = m_lastLapTime;
            ++m_lap;
            m_lapStart = now;
            m_nextCheckpoint = 1;
        }
        auto quarter = index * 4 / m_assets.circuit.samples.size();
        if( quarter == m_nextCheckpoint && onRoad )
            ++m_nextCheckpoint;
        m_lastTrackIndex = index;
        std::ostringstream text;
        text << std::fixed << std::setprecision( 1 );
        s32 gear = 0;
        double rpm = 0;
        if( vehicle && vehicle->getDriveTrain() )
        {
            auto props = vehicle->getDriveTrain()->getProperties();
            props->getPropertyValue( "Gear", gear );
            props->getPropertyValue( "RPM", rpm );
        }
        text << "GRAND PRIX | " << velocity.length() * 3.6f << " km/h | Gear "
             << ( gear == 0   ? "N"
                  : gear == 1 ? "R"
                              : std::to_string( gear - 1 ) )
             << " | " << int( rpm ) << " rpm | Lap " << m_lap << '\n';
        text << "Lap time " << ( m_lapStart > 0 ? now - m_lapStart : 0 ) << " s | Last " << m_lastLapTime
             << " s | Best " << m_bestLapTime << " s\n";
        text << "Circuit " << m_assets.circuit.length << " m | Seed " << m_seed;
        if( ( position - m_assets.circuit.samples[index].position ).length() > 11 )
            text << " | OFF TRACK";
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
            if( ( m_smokeTest || m_trackSmokeTest ) && vehicle )
            {
                auto w = vehicle->getWheelController( 0 );
                WP_LOG(
                    String( "Advanced physics: dt=" ) + StringUtil::toString( timer->getDeltaTime() ) +
                    " mass=" + StringUtil::toString( rigidbody->getRigidDynamic()->getMass() ) +
                    " inertia=" +
                    StringUtil::toString( rigidbody->getRigidDynamic()->getMassSpaceInertiaTensor() ) +
                    " angular=" + StringUtil::toString( rigidbody->getAngularVelocity() ) +
                    " controllerMass=" + StringUtil::toString( vehicle->getBody()->getMass() ) +
                    " root=" + StringUtil::toString( position ) +
                    " mount=" + StringUtil::toString( w->getLocalTransform().getPosition() ) +
                    " rate=" + StringUtil::toString( w->getSpringRate() ) +
                    " damping=" + StringUtil::toString( w->getDamping() ) +
                    " travel=" + StringUtil::toString( w->getSuspensionTravel() ) );
            }
            m_nextDebugLog = now + 2.0;
        }
    }

    void SampleVehicleAdvanced::createPlugins()
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
        if( !factoryManager->hasFactoryByName( "IVehicleGenerator" ) )
        {
            auto plugin = workphone::make_ptr<procedural::WPProcedural>();
            core::IApplicationManager::instance()->addPlugin( plugin );
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
            applicationManager->addPlugin( workphone::make_ptr<procedural::WPProcedural>() );

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

    void SampleVehicleAdvanced::createScene()
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

        m_vehicleActor = sceneManager->createActor();
        m_vehicleActor->setName( "Procedural Grand Prix" );
        m_vehicleActor->setPosition( getVehicleSpawnPosition() );
        m_raceScene = m_vehicleActor->addComponent<scene::ProceduralRaceScene>();
        m_raceScene->setSeed( m_seed );
        m_raceScene->setQuality( static_cast<s32>( m_quality ) );
        if( !m_raceScene->regenerate() )
            throw std::runtime_error( m_raceScene->getGenerationError() );
        m_assets = m_raceScene->getAssets();
        for( auto &group : m_assets.treeLODs ) group->setForcedLOD( m_forcedLOD );
        m_assets.vehicleLOD->setForcedLOD( m_forcedVehicleLOD );
        // The fixed high-altitude circuit review must retain surface contrast.
        if( m_captureView == "track" && !m_orbitCamera )
            applicationManager->getGraphicsSystem()->getGraphicsScene()->setFog(
                render::IGraphicsScene::FOG_NONE );
        if( auto window = applicationManager->getGraphicsSystem()->getDefaultWindow() )
            window->setSize( Vector2I( m_reviewWidth, m_reviewHeight ) );
        m_boxGround = m_assets.ground;
        m_chassisMeshActor = m_assets.body;
        m_wheelActors = m_assets.wheels;
        scene->addActor( m_vehicleActor );
        scene->registerAllUpdates( m_vehicleActor );

        m_cameraActor = sceneManager->createActor();
        WP_ASSERT( m_cameraActor );
        m_cameraActor->setName( "Vehicle Follow Camera" );
        m_cameraActor->setSmoothMotion( true );
        m_cameraActor->setPosition( getInitialCameraPosition() );
        m_cameraActor->lookAt( getVehicleSpawnPosition(), Vector3<real_Num>::unitY() );

        m_cameraController = m_cameraActor->addComponent<scene::VehicleCameraController>();
        WP_ASSERT( m_cameraController );
        m_cameraController->setTarget( m_vehicleActor );
        m_cameraController->setDistance( cameraDistance );
        m_cameraController->setHeight( cameraHeight );

        scene->addActor( m_cameraActor );
        scene->registerAllUpdates( m_cameraActor );

        if( m_camera )
        {
            // The overhead view needs more depth precision to resolve the road's
            // millimetre surface offsets at a distance of nearly 500 metres.
            m_camera->setNearClipDistance( m_captureView == "track" && !m_orbitCamera ? 10.f : .5f );
            m_camera->setFarClipDistance( 1000 );
        }
        updateRenderCamera();

        if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
        {
            if( auto debug = m_hudEnabled ? graphicsSystem->getDebug() : nullptr )
            {
                debug->drawText(
                    0, Vector2F( 0.02f, 0.02f ),
                    "W/Up: throttle  S/Down: brake  A/D or Left/Right: steer  R: reset  Esc: quit", 0 );
                debug->drawText( 1, Vector2F( 0.02f, 0.06f ), "Mouse wheel: camera zoom", 0 );
            }
        }
    }

    void SampleVehicleAdvanced::updateRenderCamera()
    {
        if( m_cameraSceneNode && m_cameraActor )
        {
            if( auto transform = m_cameraActor->getTransform() )
            {
                m_cameraSceneNode->setTransform( transform->getWorldTransform() );
                if( ( !m_capturePath.empty() || m_benchmarkSeconds > 0 ) && ( m_captureView != "follow" || m_orbitCamera ) )
                {
                    if( m_orbitCamera )
                    {
                        const auto elapsed = core::IApplicationManager::instance()->getTimer()->getTimeSinceSceneLoad();
                        const auto angle = float( std::max( elapsed - 5.0, 0.0 ) * .15 );
                        const auto target = m_assets.circuit.samples.front().position + Vector3F( 0, 1, 0 );
                        const auto radius = 45.f + 35.f * ( 1 + std::sin( angle * .5f ) );
                        m_cameraActor->setPosition( target + Vector3F( std::sin(angle)*radius, 8, std::cos(angle)*radius ) );
                        m_cameraActor->lookAt( target, Vector3F::unitY() );
                    }
                    else if( m_captureView == "corner" )
                    {
                        const auto &point =
                            m_assets.circuit.samples[m_assets.circuit.samples.size() / 4];
                        m_cameraActor->setPosition( point.position + point.right * 22.f +
                                                    Vector3F( 0, 10, 0 ) );
                        m_cameraActor->lookAt( point.position, Vector3F::unitY() );
                    }
                    else if( m_captureView == "track" )
                    {
                        m_cameraActor->setPosition( Vector3F( 75, 480, -30 ) );
                        m_cameraActor->lookAt( Vector3F( 75, 0, -30 ), Vector3F::unitZ() );
                    }
                    else
                    {
                        auto target = m_vehicleActor->getPosition();
                        m_cameraActor->setPosition( target + Vector3F( -5.5f, 2.2f, -6.5f ) );
                        m_cameraActor->lookAt( target, Vector3F::unitY() );
                    }
                    m_cameraActor->updateTransform();
                    m_cameraSceneNode->setTransform( m_cameraActor->getWorldTransform() );
                }
            }
        }
        if( m_camera && m_cameraActor && m_assets.vehicleLOD )
        {
            auto system = workphone::static_pointer_cast<scene::LODSystem>(
                m_assets.vehicleLOD->getComponentSystem() );
            if( system )
            {
                scene::LODSystem::View view;
                view.position = m_cameraActor->getWorldTransform().getPosition();
                view.verticalFovRadians = m_camera->getFOVy();
                view.nearClipDistance = m_camera->getNearClipDistance();
                view.lodBias = m_camera->getLodBias();
                system->setViewOverride( view );
            }
        }
    }

    SampleVehicleAdvanced::InputListener::InputListener() = default;

    SampleVehicleAdvanced::InputListener::~InputListener() = default;

    void SampleVehicleAdvanced::InputListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    Parameter SampleVehicleAdvanced::InputListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::inputEvent )
        {
            auto result = inputEvent( event );
            return Parameter( result );
        }

        return Parameter();
    }

    bool SampleVehicleAdvanced::InputListener::inputEvent( SmartPtr<IInputEvent> event )
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

    void SampleVehicleAdvanced::InputListener::setPriority( s32 priority )
    {
        m_priority = priority;
    }

    s32 SampleVehicleAdvanced::InputListener::getPriority() const
    {
        return m_priority;
    }

    SmartPtr<SampleVehicleAdvanced> SampleVehicleAdvanced::InputListener::getOwner() const
    {
        return m_owner.lock();
    }

    void SampleVehicleAdvanced::InputListener::setOwner( SmartPtr<SampleVehicleAdvanced> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone

int main( int argc, char **argv )
{
    using namespace workphone;

    for( int i = 1; i < argc; ++i )
    {
        if( std::string( argv[i] ) == "--validate-circuit" )
        {
            try
            {
                advanced::validateCircuit();
                std::cout << "100 seeded circuits: closure, spacing, spawn and determinism passed\n";
                return 0;
            }
            catch( const std::exception &e )
            {
                std::cerr << e.what() << '\n';
                return 1;
            }
        }
        if( std::string( argv[i] ) == "--help" )
        {
            std::cout
                << "SampleVehicleAdvanced [--seed N] [--quality low|medium|high] [--plugins PATH]\n"
                   "  --smoke-test                 Settle, accelerate, turn, brake and reset checks\n"
                   "  --track-smoke-test            Drive one full lap through the physics controller\n"
                   "  --validate-circuit           Validate 100 generated closed circuits\n"
                   "  --capture PATH.bmp --view follow|car|track|corner   Save a rendered view and "
                   "exit\n"
                   "  --no-hud --force-lod auto|0|1  Clean captures and tree LOD comparisons\n"
                   "  --force-vehicle-lod auto|0|1|2  Override the vehicle level (clamped for low)\n"
                   "  --benchmark SECONDS [--orbit] Measure after five seconds of warmup\n"
                   "  --resolution WIDTHxHEIGHT     Set the review window resolution\n"
                   "  W/Up throttle; S/Down brake; A/D steer; R reset; Esc quit; wheel camera zoom\n";
            return 0;
        }
    }
    TypeManager *typeManager = nullptr;
    bool ownsTypeManager = false;
    SmartPtr<SampleVehicleAdvanced> app;
    bool smokeTest = false, trackSmokeTest = false;
    String pluginsConfig;
    std::string capturePath, captureView = "follow";
    u32 seed = 7;
    auto quality = procedural::VehicleAppearanceQuality::High;
    bool hud = true, orbit = false;
    s32 forcedLOD = -1;
    s32 forcedVehicleLOD = -1;
    f64 benchmarkSeconds = 0;
    u32 width = 1280, height = 720;
    try
    {
        for( int i = 1; i < argc; ++i )
        {
            if( String( argv[i] ) == "--smoke-test" )
                smokeTest = true;
            else if( String( argv[i] ) == "--no-hud" )
                hud = false;
            else if( String( argv[i] ) == "--orbit" )
                orbit = true;
            else if( String( argv[i] ) == "--force-lod" && i + 1 < argc )
            {
                const String value = argv[++i];
                if( value != "auto" && value != "0" && value != "1" )
                    throw std::runtime_error( "Forced LOD must be auto, 0, or 1." );
                forcedLOD = value == "auto" ? -1 : value == "0" ? 0 : 1;
            }
            else if( String( argv[i] ) == "--force-vehicle-lod" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                if( value != "auto" && value != "0" && value != "1" && value != "2" )
                    throw std::runtime_error( "Vehicle LOD must be auto, 0, 1 or 2." );
                forcedVehicleLOD = value == "auto" ? -1 : std::stoi( value );
            }
            else if( String( argv[i] ) == "--benchmark" && i + 1 < argc )
            {
                size_t consumed = 0;
                const std::string value = argv[++i];
                benchmarkSeconds = std::stod( value, &consumed );
                if( consumed != value.size() || !std::isfinite( benchmarkSeconds ) ||
                    benchmarkSeconds <= 0 || benchmarkSeconds > 3600 )
                    throw std::runtime_error(
                        "Benchmark duration must be between zero and 3600 seconds." );
            }
            else if( String( argv[i] ) == "--resolution" && i + 1 < argc )
            {
                const std::string value = argv[++i];
                const auto separator = value.find( 'x' );
                if( separator == std::string::npos )
                    throw std::runtime_error( "Resolution must be WIDTHxHEIGHT." );
                size_t left = 0, right = 0;
                width = std::stoul( value.substr( 0, separator ), &left );
                height = std::stoul( value.substr( separator + 1 ), &right );
                if( left != separator || right != value.size() - separator - 1 || width < 320 ||
                    height < 180 || width > 7680 || height > 4320 )
                    throw std::runtime_error( "Resolution must be between 320x180 and 7680x4320." );
            }
            else if( String( argv[i] ) == "--track-smoke-test" )
                trackSmokeTest = true;
            else if( String( argv[i] ) == "--plugins" && i + 1 < argc )
                pluginsConfig = argv[++i];
            else if( String( argv[i] ) == "--capture" && i + 1 < argc )
                capturePath = argv[++i];
            else if( String( argv[i] ) == "--view" && i + 1 < argc )
                captureView = argv[++i];
            else if( String( argv[i] ) == "--seed" && i + 1 < argc )
                seed = static_cast<u32>( std::stoul( argv[++i] ) );
            else if( String( argv[i] ) == "--quality" && i + 1 < argc )
            {
                const String value = argv[++i];
                if( value == "low" )
                    quality = procedural::VehicleAppearanceQuality::Preview;
                else if( value == "medium" )
                    quality = procedural::VehicleAppearanceQuality::Standard;
                else if( value == "high" )
                    quality = procedural::VehicleAppearanceQuality::High;
                else
                    throw std::runtime_error( "Quality must be low, medium, or high." );
            }
            else
                throw std::runtime_error( std::string( "Unknown option or missing value: " ) + argv[i] );
        }
        if( captureView != "follow" && captureView != "car" && captureView != "track" &&
            captureView != "corner" )
            throw std::runtime_error( "View must be follow, car, track, or corner." );
        if( ( smokeTest && trackSmokeTest ) ||
            ( ( !capturePath.empty() || benchmarkSeconds > 0 ) && ( smokeTest || trackSmokeTest ) ) )
            throw std::runtime_error( "Choose one smoke test or a capture per run." );
        if( orbit && benchmarkSeconds <= 0 ) throw std::runtime_error( "--orbit requires --benchmark." );
    }
    catch( const std::exception &e )
    {
        std::cerr << e.what() << '\n';
        return 2;
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

        app = workphone::make_ptr<SampleVehicleAdvanced>();
        app->setActiveThreads( activeThreadCount );
        app->setSmokeTest( smokeTest );
        app->setTrackSmokeTest( trackSmokeTest );
        app->setGenerationOptions( seed, quality );
        app->setCapture( capturePath, captureView );
        app->setReviewOptions( hud, forcedLOD, forcedVehicleLOD, benchmarkSeconds, orbit, width, height );
        if( !pluginsConfig.empty() )
            app->setPluginsConfigFilePath( pluginsConfig );
        app->load( nullptr );
        if( app->getLoadingState() != LoadingState::Loaded )
            throw std::runtime_error( "SampleVehicleAdvanced failed to load." );
        app->run();
        exitCode = ( ( smokeTest || trackSmokeTest ) && !app->smokeTestPassed() ) ||
                           ( ( !capturePath.empty() || benchmarkSeconds > 0 ) && !app->capturePassed() )
                       ? 1
                       : 0;

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
