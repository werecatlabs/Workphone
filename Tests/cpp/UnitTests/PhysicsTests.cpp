#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <WPVehiclePhysics/CCarController.hpp>
#include <WPVehiclePhysics/WheelControllerBrush.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <thread>
#include <functional>

using namespace workphone;

namespace
{
    class PlayingStateGuard
    {
    public:
        explicit PlayingStateGuard( SmartPtr<core::IApplicationManager> applicationManager ) :
            m_applicationManager( applicationManager ),
            m_wasPlaying( applicationManager && applicationManager->isPlaying() )
        {
        }

        ~PlayingStateGuard()
        {
            if( m_applicationManager )
            {
                m_applicationManager->setPlaying( m_wasPlaying );
            }
        }

        void setPlaying( bool playing )
        {
            if( m_applicationManager )
            {
                m_applicationManager->setPlaying( playing );
            }
        }

    private:
        SmartPtr<core::IApplicationManager> m_applicationManager;
        bool m_wasPlaying = false;
    };

    class PhysicsTransformCapture final : public IEventListener
    {
    public:
        Parameter handleEvent( EventType, hash_type eventValue, const Array<Parameter> &arguments,
                               SmartPtr<ISharedObject> , SmartPtr<ISharedObject> , SmartPtr<IEvent> ) override
        {
            if( eventValue == IEvent::transform && arguments.size() >= 2 )
            {
                position = arguments[0].getVector3();
                orientation = arguments[1].getQuaternion();
                ++eventCount;
            }
            return {};
        }

        Vector3<real_Num> position = Vector3<real_Num>::zero();
        Quaternion<real_Num> orientation = Quaternion<real_Num>::identity();
        u32 eventCount = 0;
    };

    class PhysicsRemovalCapture final : public IEventListener
    {
    public:
        Parameter handleEvent( EventType, hash_type eventValue, const Array<Parameter> &,
                               SmartPtr<ISharedObject>, SmartPtr<ISharedObject>, SmartPtr<IEvent> ) override
        {
            if( eventValue == IEvent::transform && onTransform )
            {
                auto callback = std::move( onTransform );
                callback();
            }
            return {};
        }
        std::function<void()> onTransform;
    };

    bool hasLiveActors( const Array<SmartPtr<scene::IGameActor>> &actors )
    {
        return std::any_of( actors.begin(), actors.end(),
                            []( const auto &actor ) { return actor != nullptr; } );
    }

    bool skipWhenPhysicsUnavailable()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager && applicationManager->getPhysicsManager() )
        {
            return false;
        }

        BOOST_TEST_MESSAGE( "Skipping physics test because no physics manager is available." );
        return true;
    }

    void destroyAllActors()
    {
        if( auto applicationManager = core::IApplicationManager::instance() )
        {
            if( auto sceneManager = applicationManager->getGameManager() )
            {
                sceneManager->destroyActors();

                auto actors = sceneManager->getActors();
                BOOST_CHECK( !hasLiveActors( actors ) );
            }
        }
    }

    void drainSceneWork( TestGuard &guard, u32 iterations = 20 )
    {
        auto jobQueue = guard.applicationManager->getJobQueue();
        BOOST_REQUIRE( jobQueue );

        for( u32 i = 0; i < iterations; ++i )
        {
            jobQueue->update();
            guard.taskManager->update();
            guard.runUpdateCycle();
        }
    }
}  // namespace

BOOST_AUTO_TEST_CASE( physics_simulation )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto sceneManager = applicationManager->getGameManager();
    BOOST_REQUIRE( sceneManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        return;
    }

    try
    {
        auto box = ApplicationUtil::createDefaultCube();
        BOOST_CHECK( box );

        auto actors = sceneManager->getActors();
        auto it = std::find( actors.begin(), actors.end(), box );
        BOOST_CHECK( it != actors.end() );

        for( size_t i = 0; i < 10; ++i )
        {
            physicsManager->update();
        }

        sceneManager->destroyActor( box );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }

    destroyAllActors();
}

BOOST_AUTO_TEST_CASE( physics_collision_smoke )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        BOOST_REQUIRE( physicsManager );

        // This is intentionally a smoke test until collision event/query APIs are asserted here.
        physicsManager->update();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }

    destroyAllActors();
}

BOOST_AUTO_TEST_CASE( physics_dynamic_body_steps_and_publishes_transform )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    BOOST_REQUIRE( physicsManager );

    auto physicsScene = physicsManager->getPhysicsScene();
    BOOST_REQUIRE( physicsScene );

    auto timer = applicationManager->getTimer();
    BOOST_REQUIRE( timer );

    PlayingStateGuard playingState( applicationManager );
    playingState.setPlaying( true );

    physicsScene->clear();
    physicsScene->setGravity( Vector3<real_Num>(
        static_cast<real_Num>( 0 ), static_cast<real_Num>( -9.81 ), static_cast<real_Num>( 0 ) ) );
    BOOST_CHECK_CLOSE( physicsScene->getGravity().Y(), static_cast<real_Num>( -9.81 ), 0.01 );

    auto shape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
    BOOST_REQUIRE( shape );
    shape->setExtents( Vector3<real_Num>( static_cast<real_Num>( 1.81 ), static_cast<real_Num>( 1.17 ),
                                          static_cast<real_Num>( 4.405 ) ) );

    Transform3<real_Num> initialTransform;
    initialTransform.setPosition( Vector3<real_Num>(
        static_cast<real_Num>( 0 ), static_cast<real_Num>( 6.3 ), static_cast<real_Num>( 0 ) ) );

    auto body = physicsManager->addRigidDynamic( initialTransform );
    BOOST_REQUIRE( body );
    body->addShape( shape );
    body->setKinematic( false );
    body->setEnabled( true );

    auto capture = workphone::make_ptr<PhysicsTransformCapture>();
    body->addObjectListener( capture );
    physicsScene->addActor( body );
    BOOST_REQUIRE( physicsScene->hasActor( body ) );
    const auto dynamicCount = physicsScene->numDynamicActors();
    physicsScene->addActor( body );
    BOOST_CHECK_EQUAL( physicsScene->numDynamicActors(), dynamicCount );

    timer->update();
    std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
    timer->update();
    BOOST_CHECK( physicsScene->isLoaded() );
    BOOST_CHECK( applicationManager->isPlaying() );
    BOOST_CHECK_GT( timer->getDeltaTime(), Math<time_interval>::epsilon() );
    physicsScene->update();

    const auto simulatedTransform = body->getTransform();
    BOOST_CHECK_LT( simulatedTransform.getPosition().Y(), initialTransform.getPosition().Y() );
    BOOST_CHECK_GT( capture->eventCount, 0u );
    BOOST_CHECK_CLOSE( capture->position.Y(), simulatedTransform.getPosition().Y(),
                       static_cast<real_Num>( 0.01 ) );

    physicsScene->removeActor( body );
    body->removeObjectListener( capture );
    physicsManager->removePhysicsBody( body );
    physicsManager->removeCollisionShape( shape );
}

BOOST_AUTO_TEST_CASE( physics_transform_publication_preserves_callback_removal )
{
    if( skipWhenPhysicsUnavailable() )
        return;
    auto manager = core::IApplicationManager::instance()->getPhysicsManager();
    auto scene = manager->getPhysicsScene();
    BOOST_REQUIRE( scene );
    scene->clear();
    auto first = manager->addRigidDynamic( Transform3<real_Num>::identity() );
    auto second = manager->addRigidDynamic( Transform3<real_Num>::identity() );
    auto fixed = manager->addRigidStatic( Transform3<real_Num>::identity() );
    BOOST_REQUIRE( first && second && fixed );
    auto removal = workphone::make_ptr<PhysicsRemovalCapture>();
    auto secondCapture = workphone::make_ptr<PhysicsTransformCapture>();
    auto staticCapture = workphone::make_ptr<PhysicsTransformCapture>();
    bool removed = false;
    removal->onTransform = [&] {
        scene->removeActor( second );
        removed = manager->removePhysicsBody( second );
    };
    first->addObjectListener( removal );
    second->addObjectListener( secondCapture );
    fixed->addObjectListener( staticCapture );
    scene->addActor( first );
    scene->addActor( second );
    scene->addActor( fixed );
    scene->simulate( static_cast<real_Num>( 1.0 / 60.0 ), nullptr, 0, true );
    BOOST_REQUIRE( scene->fetchResults( true, nullptr ) );
    BOOST_CHECK( removed );
    BOOST_CHECK( !scene->hasActor( second ) );
    BOOST_CHECK_EQUAL( secondCapture->eventCount, 0u );
    BOOST_CHECK_EQUAL( staticCapture->eventCount, 0u );
    first->removeObjectListener( removal );
    second->removeObjectListener( secondCapture );
    fixed->removeObjectListener( staticCapture );
    scene->removeActor( first );
    scene->removeActor( fixed );
    manager->removePhysicsBody( first );
    manager->removePhysicsBody( fixed );
}

BOOST_AUTO_TEST_CASE( physics_sustained_small_force_wakes_heavy_body )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    BOOST_REQUIRE( physicsManager );

    auto physicsScene = physicsManager->getPhysicsScene();
    BOOST_REQUIRE( physicsScene );
    physicsScene->clear();
    physicsScene->setGravity( Vector3<real_Num>::zero() );

    auto body = physicsManager->addRigidDynamic( Transform3<real_Num>() );
    BOOST_REQUIRE( body );
    body->setMass( static_cast<real_Num>( 1000 ) );
    body->setSleepThreshold( static_cast<real_Num>( 0.01 ) );
    physicsScene->addActor( body );

    for( u32 step = 0; step < 120; ++step )
    {
        body->addForce( Vector3<real_Num>( static_cast<real_Num>( 100 ), static_cast<real_Num>( 0 ),
                                           static_cast<real_Num>( 0 ) ) );
        physicsScene->simulate( static_cast<real_Num>( 0.01 ), nullptr, 0, true );
        BOOST_REQUIRE( physicsScene->fetchResults( true, nullptr ) );
    }

    BOOST_CHECK( !body->isSleeping() );
    BOOST_CHECK_GT( body->getLinearVelocity().X(), static_cast<real_Num>( 0.1 ) );
    BOOST_CHECK_GT( body->getTransform().getPosition().X(), static_cast<real_Num>( 0.05 ) );

    physicsScene->removeActor( body );
    physicsManager->removePhysicsBody( body );
}

BOOST_AUTO_TEST_CASE( physics_default_group_boxes_collide )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    BOOST_REQUIRE( physicsManager );

    auto physicsScene = physicsManager->getPhysicsScene();
    BOOST_REQUIRE( physicsScene );
    physicsScene->clear();
    physicsScene->setGravity( Vector3<real_Num>(
        static_cast<real_Num>( 0 ), static_cast<real_Num>( -9.81 ), static_cast<real_Num>( 0 ) ) );

    auto groundShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
    BOOST_REQUIRE( groundShape );
    groundShape->setExtents( Vector3<real_Num>( static_cast<real_Num>( 10 ), static_cast<real_Num>( 1 ),
                                                static_cast<real_Num>( 10 ) ) );
    groundShape->setCollisionType( 0u );
    groundShape->setCollisionMask( 0u );

    auto groundBody = physicsManager->addRigidStatic( Transform3<real_Num>() );
    BOOST_REQUIRE( groundBody );
    groundBody->setCollisionType( 0u );
    groundBody->setCollisionMask( 0u );
    groundBody->addShape( groundShape );
    physicsScene->addActor( groundBody );

    auto fallingShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
    BOOST_REQUIRE( fallingShape );
    fallingShape->setExtents( Vector3<real_Num>::unit() );
    fallingShape->setCollisionType( 0u );
    fallingShape->setCollisionMask( 0u );

    auto fallingTransform = Transform3<real_Num>();
    fallingTransform.setPosition( Vector3<real_Num>(
        static_cast<real_Num>( 0 ), static_cast<real_Num>( 5 ), static_cast<real_Num>( 0 ) ) );
    auto fallingBody = physicsManager->addRigidDynamic( fallingTransform );
    BOOST_REQUIRE( fallingBody );
    fallingBody->setCollisionType( 0u );
    fallingBody->setCollisionMask( 0u );
    fallingBody->addShape( fallingShape );
    physicsScene->addActor( fallingBody );

    auto fetchedAllResults = true;
    for( u32 step = 0; step < 240; ++step )
    {
        physicsScene->simulate( static_cast<real_Num>( 1.0 / 60.0 ), nullptr, 0, true );
        if( !physicsScene->fetchResults( true, nullptr ) )
        {
            fetchedAllResults = false;
            break;
        }
    }
    BOOST_REQUIRE( fetchedAllResults );

    BOOST_CHECK_GT( fallingBody->getTransform().getPosition().Y(), static_cast<real_Num>( 0.8 ) );

    physicsScene->removeActor( fallingBody );
    physicsScene->removeActor( groundBody );
    physicsManager->removePhysicsBody( fallingBody );
    physicsManager->removePhysicsBody( groundBody );
    physicsManager->removeCollisionShape( fallingShape );
    physicsManager->removeCollisionShape( groundShape );
}

BOOST_AUTO_TEST_CASE( physics_box_scale_updates_and_clone_preserve_dimensions )
{
    if( skipWhenPhysicsUnavailable() )
        return;

    auto manager = core::IApplicationManager::instance()->getPhysicsManager();
    auto shape = manager->addCollisionShape<physics::IBoxShape3>( nullptr );
    BOOST_REQUIRE( shape );
    const auto extents = Vector3<real_Num>( 2, 1, 3 );
    Transform3<real_Num> pose;
    pose.setPosition( Vector3<real_Num>( 1, 2, 3 ) );
    pose.setScale( Vector3<real_Num>( 4, 2, 0.5f ) );

    auto checkDimensions = []( SmartPtr<physics::IBoxShape3> box, const Vector3<real_Num> &expected ) {
        const auto actual = box->getAABB().getExtent();
        BOOST_CHECK_SMALL( actual.X() - expected.X(), static_cast<real_Num>( 0.001 ) );
        BOOST_CHECK_SMALL( actual.Y() - expected.Y(), static_cast<real_Num>( 0.001 ) );
        BOOST_CHECK_SMALL( actual.Z() - expected.Z(), static_cast<real_Num>( 0.001 ) );
    };
    // Both setters are called repeatedly as scene components refresh their transforms.
    for( u32 i = 0; i < 3; ++i )
    {
        shape->setLocalPose( pose );
        shape->setExtents( extents );
        shape->setLocalPose( pose );
        checkDimensions( shape, Vector3<real_Num>( 8, 2, 1.5f ) );
    }
    auto clone = workphone::dynamic_pointer_cast<physics::IBoxShape3>( shape->clone() );
    BOOST_REQUIRE( clone );
    checkDimensions( clone, Vector3<real_Num>( 8, 2, 1.5f ) );

    pose.setScale( Vector3<real_Num>::unit() );
    shape->setLocalPose( pose );
    checkDimensions( shape, extents );
    checkDimensions( clone, Vector3<real_Num>( 8, 2, 1.5f ) );
    pose.setScale( Vector3<real_Num>( -4, 2, -0.5f ) );
    shape->setLocalPose( pose );
    checkDimensions( shape, Vector3<real_Num>( 8, 2, 1.5f ) );
    manager->removeCollisionShape( shape );
}

BOOST_AUTO_TEST_CASE( physics_scaled_ground_supports_entire_sample_stack )
{
    if( skipWhenPhysicsUnavailable() )
        return;

    auto manager = core::IApplicationManager::instance()->getPhysicsManager();
    auto scene = manager->getPhysicsScene();
    BOOST_REQUIRE( scene );
    scene->clear();
    scene->setGravity( Vector3<real_Num>( 0, -9.81f, 0 ) );

    auto groundShape = manager->addCollisionShape<physics::IBoxShape3>( nullptr );
    BOOST_REQUIRE( groundShape );
    groundShape->setExtents( Vector3<real_Num>::unit() );
    Transform3<real_Num> localPose;
    localPose.setScale( Vector3<real_Num>( 500, 1, 500 ) );
    groundShape->setLocalPose( localPose );
    Transform3<real_Num> groundPose;
    groundPose.setPosition( Vector3<real_Num>( 0, -0.5f, 0 ) );
    auto ground = manager->addRigidStatic( groundPose );
    ground->addShape( groundShape );
    scene->addActor( ground );

    Array<SmartPtr<physics::IRigidDynamic3>> boxes;
    Array<SmartPtr<physics::IBoxShape3>> shapes;
    for( u32 y = 0; y < 4; ++y )
        for( u32 x = 0; x < 4; ++x )
            for( u32 z = 0; z < 4; ++z )
            {
                auto shape = manager->addCollisionShape<physics::IBoxShape3>( nullptr );
                shape->setExtents( Vector3<real_Num>::unit() );
                Transform3<real_Num> pose;
                pose.setPosition( Vector3<real_Num>( x * 1.1f - 1.65f,
                                                     2 + ( y + 0.5f ) * 1.1f,
                                                     z * 1.1f - 1.65f ) );
                auto box = manager->addRigidDynamic( pose );
                box->addShape( shape );
                scene->addActor( box );
                boxes.push_back( box );
                shapes.push_back( shape );
            }

    auto lowestY = static_cast<real_Num>( 100 );
    for( u32 step = 0; step < 600; ++step )
    {
        scene->simulate( static_cast<real_Num>( 1.0 / 60.0 ), nullptr, 0, true );
        scene->fetchResults( true, nullptr );
        for( const auto &box : boxes )
            lowestY = std::min( lowestY, box->getTransform().getPosition().Y() );
    }
    BOOST_CHECK_GT( lowestY, static_cast<real_Num>( 0.25 ) );
    for( const auto &box : boxes )
    {
        BOOST_CHECK_LT( box->getTransform().getPosition().Y(), static_cast<real_Num>( 5 ) );
        scene->removeActor( box );
        manager->removePhysicsBody( box );
    }
    scene->removeActor( ground );
    manager->removePhysicsBody( ground );
    for( const auto &shape : shapes )
        manager->removeCollisionShape( shape );
    manager->removeCollisionShape( groundShape );
}

BOOST_AUTO_TEST_CASE( physics_test_scene_vehicle_moves_under_gravity )
{
    TestGuard guard( true );
    BOOST_REQUIRE( guard.isAvailable );

    PlayingStateGuard playingState( guard.applicationManager );
    playingState.setPlaying( false );

    guard.scene->setState( scene::IGameScene::State::Edit );
    guard.scene->clear( true );
    guard.scene->loadScene( "Tests/physics_test.fbscene", false );
    drainSceneWork( guard );

    auto vehicleActor = guard.scene->findActorByName( "Vehicle" );
    BOOST_REQUIRE( vehicleActor );

    auto groundActor = guard.scene->findActorByName( "Plane" );
    BOOST_REQUIRE( groundActor );

    playingState.setPlaying( true );
    guard.scene->setState( scene::IGameScene::State::Play );
    drainSceneWork( guard );

    auto rigidbody = vehicleActor->getComponent<scene::Rigidbody>();
    BOOST_REQUIRE( rigidbody );

    auto body = rigidbody->getRigidDynamic();
    BOOST_REQUIRE( body );
    BOOST_REQUIRE_GT( body->getNumShapes(), 0u );

    auto groundRigidbody = groundActor->getComponent<scene::Rigidbody>();
    BOOST_REQUIRE( groundRigidbody );

    auto groundBody = groundRigidbody->getRigidStatic();
    BOOST_REQUIRE( groundBody );
    BOOST_REQUIRE_GT( groundBody->getNumShapes(), 0u );
    const auto groundShape = groundBody->getShapes().front();
    BOOST_REQUIRE( groundShape );
    BOOST_REQUIRE( workphone::dynamic_pointer_cast<physics::IPlaneShape3>( groundShape ) );
    BOOST_CHECK( groundShape->isEnabled() );
    BOOST_CHECK( !groundShape->isTrigger() );

    auto physicsScene = guard.physicsManager->getPhysicsScene();
    BOOST_REQUIRE( physicsScene );
    BOOST_REQUIRE( physicsScene->hasActor( body ) );
    BOOST_REQUIRE( physicsScene->hasActor( groundBody ) );

    BOOST_CHECK_EQUAL( body->getCollisionType(), 0u );
    BOOST_CHECK_EQUAL( body->getCollisionMask(), 0u );
    BOOST_CHECK_EQUAL( groundBody->getCollisionType(), 0u );
    BOOST_CHECK_EQUAL( groundBody->getCollisionMask(), 0u );

    const auto rayOrigin = Vector3<real_Num>( static_cast<real_Num>( 50 ), static_cast<real_Num>( 10 ),
                                              static_cast<real_Num>( 50 ) );
    Array<SmartPtr<physics::IRaycastHit>> groundHits;
    BOOST_REQUIRE( physicsScene->castRay( rayOrigin, -Vector3<real_Num>::unitY(), groundHits ) );
    BOOST_REQUIRE( !groundHits.empty() );
    BOOST_CHECK_GT( groundHits.front()->getNormal().Y(), static_cast<real_Num>( 0.9 ) );

    const auto initialBodyPosition = body->getTransform().getPosition();
    guard.timer->update();
    std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
    guard.timer->update();
    physicsScene->update();
    rigidbody->update();

    const auto simulatedBodyPosition = body->getTransform().getPosition();
    BOOST_CHECK_LT( simulatedBodyPosition.Y(), initialBodyPosition.Y() );
    BOOST_CHECK_CLOSE( vehicleActor->getPosition().Y(), simulatedBodyPosition.Y(),
                       static_cast<real_Num>( 0.01 ) );

    auto lowestBodyY = simulatedBodyPosition.Y();
    auto fetchedAllResults = true;
    for( u32 step = 0; step < 240; ++step )
    {
        physicsScene->simulate( static_cast<real_Num>( 1.0 / 60.0 ), nullptr, 0, true );
        if( !physicsScene->fetchResults( true, nullptr ) )
        {
            fetchedAllResults = false;
            break;
        }
        lowestBodyY = std::min( lowestBodyY, body->getTransform().getPosition().Y() );
    }
    BOOST_REQUIRE( fetchedAllResults );

    rigidbody->update();
    const auto landedBodyPosition = body->getTransform().getPosition();
    BOOST_CHECK_GT( lowestBodyY, static_cast<real_Num>( -0.5 ) );
    BOOST_CHECK_GT( landedBodyPosition.Y(), static_cast<real_Num>( -0.5 ) );
    BOOST_CHECK_CLOSE( vehicleActor->getPosition().Y(), landedBodyPosition.Y(),
                       static_cast<real_Num>( 0.01 ) );
}

BOOST_AUTO_TEST_CASE( physics_loaded_vehicle_drives_and_updates_smooth_transform )
{
    TestGuard guard( true );
    BOOST_REQUIRE( guard.isAvailable );
    PlayingStateGuard playingState( guard.applicationManager );
    playingState.setPlaying( false );

    const auto addedCar = !guard.factoryManager->hasFactoryById( CCarController::typeInfo() );
    const auto addedWheel = !guard.factoryManager->hasFactoryById( WheelControllerBrush::typeInfo() );
    if( addedCar )
        FactoryUtil::addFactory<CCarController>();
    if( addedWheel )
        FactoryUtil::addFactory<WheelControllerBrush>();
    guard.addCleanup( [addedCar, addedWheel]() {
        if( addedWheel )
            FactoryUtil::removeFactory<WheelControllerBrush>();
        if( addedCar )
            FactoryUtil::removeFactory<CCarController>();
    } );
    auto oldInputManager = guard.applicationManager->getInputDeviceManager();
    auto inputManager = make_ptr<InputDeviceManager>();
    inputManager->load( nullptr );
    guard.applicationManager->setInputDeviceManager( inputManager );
    guard.addCleanup( [app = guard.applicationManager, oldInputManager, inputManager]() mutable {
        inputManager->unload( nullptr );
        app->setInputDeviceManager( oldInputManager );
    } );
    const auto oldVehicleManager = guard.applicationManager->getVehicleManager();
    auto vehicleManager = make_ptr<vehicle::VehicleManager>();
    vehicleManager->load( nullptr );
    guard.applicationManager->setVehicleManager( vehicleManager );
    guard.addCleanup( [app = guard.applicationManager, oldVehicleManager, vehicleManager]() mutable {
        vehicleManager->unload( nullptr );
        app->setVehicleManager( oldVehicleManager );
    } );
    guard.addCleanup( [scene = guard.scene]() mutable { scene->clear( true ); } );
    guard.addCleanup( []() { Thread::setCurrentTask( TaskId::Primary ); } );

    guard.scene->setState( scene::IGameScene::State::Edit );
    guard.scene->clear( true );
    guard.scene->loadScene( "Tests/physics_test.fbscene", false );
    drainSceneWork( guard );
    auto car = guard.sceneManager->getObjectByType<scene::CarController>();
    BOOST_REQUIRE( car );
    auto actor = car->getActor();
    BOOST_REQUIRE( actor );
    actor->setSmoothMotion( true );
    auto rigidbody = actor->getComponent<scene::Rigidbody>();
    BOOST_REQUIRE( rigidbody );
    auto body = rigidbody->getRigidDynamic();
    BOOST_REQUIRE( body );
    BOOST_REQUIRE( guard.physicsManager->getPhysicsScene()->hasActor( body ) );

    // Keep the vehicle away from the falling cube in the physics fixture.
    guard.sceneManager->edit();
    drainSceneWork( guard );
    actor->setPosition( Vector3<real_Num>( 50.0f, 6.3f, 50.0f ) );
    actor->getTransform()->update();
    rigidbody->updateTransform();

    playingState.setPlaying( true );
    guard.sceneManager->play();
    drainSceneWork( guard );
    BOOST_REQUIRE( car->getState() == scene::IComponent::State::Play );
    auto vehicle = car->getVehicleController();
    BOOST_REQUIRE( vehicle );
    BOOST_REQUIRE( vehicle->getState() == vehicle::IVehicle::State::PLAY );
    vehicle->setChannel( static_cast<s32>( vehicle::IVehicle::Input::THROTTLE ), 1.0f );
    const auto initialPosition = body->getTransform().getPosition();
    auto physicsScene = guard.physicsManager->getPhysicsScene();
    auto timer = workphone::dynamic_pointer_cast<TimerMT>( guard.timer );
    BOOST_REQUIRE( timer );
    timer->reset( timer->getTime() );
    guard.addCleanup( [timer]() mutable { timer->reset( timer->getTime() ); } );
    for( u32 step = 0; step < 240; ++step )
    {
        Thread::setCurrentTask( TaskId::Physics );
        timer->update( 1.0 / 60.0 );
        physicsScene->update();
        vehicleManager->update();
        Thread::setCurrentTask( guard.sceneManager->getStateTask() );
        timer->update( 1.0 / 60.0 );
        guard.sceneManager->preUpdate();
        guard.sceneManager->update();
        guard.sceneManager->postUpdate();
        if( guard.sceneManager->getSceneTask() != guard.sceneManager->getStateTask() )
        {
            Thread::setCurrentTask( guard.sceneManager->getSceneTask() );
            timer->update( 1.0 / 60.0 );
            guard.sceneManager->preUpdate();
            guard.sceneManager->update();
            guard.sceneManager->postUpdate();
        }
        Thread::setCurrentTask( TaskId::Render );
        timer->update( 1.0 / 60.0 );
        guard.sceneManager->preUpdate();
        guard.sceneManager->update();
        guard.sceneManager->postUpdate();
    }
    guard.setupThread();
    const auto finalPosition = body->getTransform().getPosition();
    BOOST_TEST_MESSAGE( "Vehicle position: " << finalPosition.X() << ", " << finalPosition.Y()
                                            << ", " << finalPosition.Z() );
    BOOST_CHECK_GT( ( finalPosition - initialPosition ).length(), 0.1f );
    BOOST_CHECK_GT( Math<real_Num>::Abs( finalPosition.Z() - initialPosition.Z() ), 0.5f );
    BOOST_CHECK_LT( ( actor->getPosition() - finalPosition ).length(), 1.0f );
}

BOOST_AUTO_TEST_CASE( physics_box_adds_shape_and_actor )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        scene->clear();
        physicsManager->update();

        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );

        boxShape->setExtents( Vector3<real_Num>( 1.0, 1.0, 1.0 ) );

        Transform3<real_Num> transform;
        BOOST_CHECK( transform.isValid() );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK_EQUAL( shapes.size(), 1 );

        const auto staticActorsBefore = scene->numStaticActors();
        scene->addActor( rigidBody );

        physicsManager->update();

        scene->removeActor( rigidBody );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( physics_raycasts_box_hits )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );

        const auto boxSize = static_cast<real_Num>( 500.0 );
        boxShape->setExtents( Vector3<real_Num>::unit() * boxSize );

        Transform3<real_Num> transform;
        auto position = Vector3<real_Num>::unitY() * -( boxSize * static_cast<real_Num>( 0.5 ) );
        transform.setPosition( position );

        BOOST_CHECK( transform.isValid() );
        BOOST_CHECK( transform.isSane() );

        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK_EQUAL( shapes.size(), 1 );

        const auto staticActorsBefore = scene->numStaticActors();
        scene->addActor( rigidBody );

        const auto origin = Vector3<real_Num>::unitY() * static_cast<real_Num>( 250.0 );
        const auto dir = -Vector3<real_Num>::unitY();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        auto result = scene->castRay( origin, dir, hits );
        BOOST_CHECK( result );
        BOOST_CHECK( !hits.empty() );

        for( size_t i = 0; i < 3; ++i )
        {
            physicsManager->update();
        }

        hits.clear();
        result = scene->castRay( origin, dir, hits );
        BOOST_CHECK( result );
        BOOST_CHECK( !hits.empty() );

        scene->removeActor( rigidBody );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( physics_raycasts_box_miss )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        auto boxShape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape );

        boxShape->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 10.0 ) );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape );
        const auto staticActorsBefore = scene->numStaticActors();
        BOOST_CHECK_EQUAL( staticActorsBefore, 0 );
        scene->addActor( rigidBody );

        const auto origin = Vector3<real_Num>::unitY() * static_cast<real_Num>( 100.0 );
        const auto dir = Vector3<real_Num>::unitY();  // away from box at origin

        Array<SmartPtr<physics::IRaycastHit>> hits;
        const auto result = scene->castRay( origin, dir, hits );
        BOOST_CHECK( result == false );
        BOOST_CHECK( hits.empty() );

        scene->removeActor( rigidBody );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( physics_raycasts_empty_scene_misses_after_clear )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        scene->clear();
        physicsManager->update();

        const auto origin = Vector3<real_Num>(
            static_cast<real_Num>( 0.0 ), static_cast<real_Num>( 100.0 ), static_cast<real_Num>( 0.0 ) );
        const auto dir = Vector3<real_Num>::unitY();

        Array<SmartPtr<physics::IRaycastHit>> hits;
        const auto result = scene->castRay( origin, dir, hits );

        BOOST_CHECK( result == false );
        BOOST_CHECK( hits.empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( physics_raycasts_zero_direction_is_safe )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        const auto origin = Vector3<real_Num>::unitY() * static_cast<real_Num>( 10.0 );
        const auto dir = Vector3<real_Num>( static_cast<real_Num>( 0.0 ), static_cast<real_Num>( 0.0 ),
                                            static_cast<real_Num>( 0.0 ) );

        Array<SmartPtr<physics::IRaycastHit>> hits;
        const auto result = scene->castRay( origin, dir, hits );

        // Expected behavior may vary by backend; the key is: no crash, and no hits is acceptable.
        if( result )
        {
            // If backend treats zero-dir as valid, ensure it doesn't return garbage/null hits.
            for( auto &hit : hits )
            {
                BOOST_CHECK( hit );
            }
        }
        else
        {
            BOOST_CHECK( hits.empty() );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( physics_multiple_shapes_on_rigid_body )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        return;
    }

    try
    {
        auto scene = physicsManager->getPhysicsScene();
        BOOST_REQUIRE( scene );

        auto boxShape1 = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        auto boxShape2 = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
        BOOST_REQUIRE( boxShape1 );
        BOOST_REQUIRE( boxShape2 );

        boxShape1->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 5.0 ) );
        boxShape2->setExtents( Vector3<real_Num>::unit() * static_cast<real_Num>( 3.0 ) );

        Transform3<real_Num> transform;
        auto rigidBody = physicsManager->addRigidStatic( transform );
        BOOST_REQUIRE( rigidBody );

        rigidBody->addShape( boxShape1 );
        rigidBody->addShape( boxShape2 );

        auto shapes = rigidBody->getShapes();
        BOOST_CHECK_EQUAL( shapes.size(), 2 );

        const auto staticActorsBefore = scene->numStaticActors();
        scene->addActor( rigidBody );
        physicsManager->update();

        scene->removeActor( rigidBody );
        BOOST_CHECK_LE( scene->numStaticActors(), staticActorsBefore );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( scene_destroyActors_is_idempotent )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto sceneManager = applicationManager->getGameManager();
    BOOST_REQUIRE( sceneManager );

    try
    {
        sceneManager->destroyActors();
        sceneManager->destroyActors();  // should be safe

        const auto actors = sceneManager->getActors();
        BOOST_CHECK( !hasLiveActors( actors ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( scene_destroyActors_removes_created_actor_references )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto sceneManager = applicationManager->getGameManager();
    BOOST_REQUIRE( sceneManager );

    try
    {
        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto actors = sceneManager->getActors();
        BOOST_CHECK( std::find( actors.begin(), actors.end(), actor ) != actors.end() );

        sceneManager->destroyActors();

        actors = sceneManager->getActors();
        BOOST_CHECK( !hasLiveActors( actors ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( physics_constraint_create_and_unload )
{
    if( skipWhenPhysicsUnavailable() )
    {
        return;
    }

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto physicsManager = applicationManager->getPhysicsManager();
    if( !physicsManager )
    {
        return;
    }

    try
    {
        auto constraint = ApplicationUtil::createDefaultConstraint();
        BOOST_CHECK( constraint );

        if( constraint )
        {
            constraint->unload( nullptr );
            constraint = nullptr;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}
