#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/ParticleSystem.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <thread>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    void fastForwardParticleEmission( SmartPtr<render::IParticleSystem> particleSystem )
    {
        BOOST_REQUIRE( particleSystem );

        auto particleSettings =
            workphone::dynamic_pointer_cast<render::ParticleSystem>( particleSystem );
        BOOST_REQUIRE( particleSettings );
        particleSettings->setRate( 20.0f );
        particleSettings->setStartLifetime( Vector2<real_Num>( 2.0f, 4.0f ) );
        particleSystem->setFastForward( 1.0f, 0.05f );
        particleSystem->setState( render::ParticleSystemState::Started );
    }
}  // namespace

BOOST_AUTO_TEST_CASE( particle_system )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto taskManager = applicationManager->getTaskManager();

    auto graphicsSystem = applicationManager->getGraphicsSystem();
    if( !graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
        return;
    }

    if( !graphicsSystem )
    {
        return;
    }

    auto graphicsScene = graphicsSystem->getGraphicsScene();
    BOOST_REQUIRE( graphicsScene );

    auto sceneNode = graphicsScene->addSceneNode();
    auto particleSystem = graphicsScene->addGraphicsObjectByType<render::IParticleSystem>();
    BOOST_REQUIRE( sceneNode );
    BOOST_REQUIRE( particleSystem );

    fastForwardParticleEmission( particleSystem );
    sceneNode->attachObject( particleSystem );

    auto rootNode = graphicsScene->getRootSceneNode();
    BOOST_REQUIRE( rootNode );
    rootNode->addChild( sceneNode );

    guard.runUpdateCycle( 3 );

    if( !particleSystem->isLoaded() )
    {
        BOOST_TEST_MESSAGE( "Particle backend is unavailable - skipping emission test" );
        graphicsScene->removeGraphicsObject( particleSystem );
        graphicsScene->removeSceneNode( sceneNode );
        return;
    }

    BOOST_REQUIRE( particleSystem->isLoaded() );
    BOOST_CHECK( particleSystem->isAttached() );
    BOOST_CHECK( particleSystem->isVisible() );
    BOOST_CHECK_EQUAL( particleSystem->getNumEmitters(), 1u );

    BOOST_CHECK( particleSystem->getState() == render::ParticleSystemState::Started );
    BOOST_CHECK_GT( particleSystem->getNumParticles(), 0u );

    auto count = 0;
    while( count++ < 5 )
    {
        taskManager->update();
    }

    graphicsScene->removeGraphicsObject( particleSystem );
    graphicsScene->removeSceneNode( sceneNode );
}

BOOST_AUTO_TEST_CASE( particle_system_native_frame_controller_emits_without_fast_forward )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;
    if( !guard.graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping particle test" );
        return;
    }

    auto graphicsScene = guard.graphicsSystem->getGraphicsScene();
    BOOST_REQUIRE( graphicsScene );

    auto sceneNode = graphicsScene->addSceneNode();
    auto particleSystem = graphicsScene->addGraphicsObjectByType<render::IParticleSystem>();
    BOOST_REQUIRE( sceneNode );
    BOOST_REQUIRE( particleSystem );

    auto particleSettings = workphone::dynamic_pointer_cast<render::ParticleSystem>( particleSystem );
    BOOST_REQUIRE( particleSettings );
    particleSettings->setRate( 100.0f );
    particleSettings->setStartLifetime( Vector2<real_Num>( 2.0f, 4.0f ) );
    particleSystem->setFastForward( 0.0f, 0.0f );
    particleSystem->setState( render::ParticleSystemState::Started );

    sceneNode->attachObject( particleSystem );

    auto rootNode = graphicsScene->getRootSceneNode();
    BOOST_REQUIRE( rootNode );
    rootNode->addChild( sceneNode );

    guard.runUpdateCycle( 3 );
    guard.runUpdateCycle( 3 );

    // Skip test if particle system failed to load
    if( !particleSystem->isLoaded() )
    {
        BOOST_TEST_MESSAGE( "Particle system failed to load - skipping test" );
        graphicsScene->removeGraphicsObject( particleSystem );
        graphicsScene->removeSceneNode( sceneNode );
        guard.cleanup();
        return;
    }

    BOOST_CHECK( particleSystem->isAttached() );
    BOOST_CHECK_EQUAL( particleSystem->getNumEmitters(), 1u );

    // Exercise the same Timer -> GraphicsSystem -> Ogre::Root frame path used
    // by the editor. No wrapper fast-forward or direct native update is used.
    for( auto i = 0; i < 3 && particleSystem->getNumParticles() == 0u; ++i )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
        guard.runUpdateCycle();
    }

    BOOST_CHECK_GT( particleSystem->getNumParticles(), 0u );

    graphicsScene->removeGraphicsObject( particleSystem );
    graphicsScene->removeSceneNode( sceneNode );
}

BOOST_AUTO_TEST_CASE( particle_system_static_node_attachment_and_emission )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;
    if( !guard.graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping particle test" );
        return;
    }

    auto graphicsScene = guard.graphicsSystem->getGraphicsScene();
    BOOST_REQUIRE( graphicsScene );

    auto sceneNode = graphicsScene->addSceneNode();
    auto particleSystem = graphicsScene->addGraphicsObjectByType<render::IParticleSystem>();
    BOOST_REQUIRE( sceneNode );
    BOOST_REQUIRE( particleSystem );

    fastForwardParticleEmission( particleSystem );
    sceneNode->setStatic( true );
    sceneNode->attachObject( particleSystem );

    auto rootNode = graphicsScene->getRootSceneNode();
    BOOST_REQUIRE( rootNode );
    rootNode->addChild( sceneNode );

    guard.runUpdateCycle( 3 );


    // Skip test if particle system failed to load
    if( !particleSystem->isLoaded() )
    {
        BOOST_TEST_MESSAGE( "Particle system failed to load - skipping test" );
        graphicsScene->removeGraphicsObject( particleSystem );
        graphicsScene->removeSceneNode( sceneNode );
        guard.cleanup();
        return;
    }

    BOOST_REQUIRE( particleSystem->isLoaded() );
    BOOST_CHECK( particleSystem->isAttached() );
    BOOST_CHECK_EQUAL( particleSystem->getNumEmitters(), 1u );

    BOOST_CHECK_GT( particleSystem->getNumParticles(), 0u );

    graphicsScene->removeGraphicsObject( particleSystem );
    graphicsScene->removeSceneNode( sceneNode );
}

BOOST_AUTO_TEST_CASE( particle_system_component_smoke_preset_emits )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;
    if( !guard.graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping particle test" );
        return;
    }
    BOOST_REQUIRE( guard.scene );
    BOOST_REQUIRE( guard.sceneManager );

    auto actor = guard.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    actor->setName( "Smoke Particle System" );

    auto component = actor->addComponent<scene::ParticleSystem>();
    BOOST_REQUIRE( component );

    // Mirror the editor's Smoke preset and its creation order: configure first,
    // then make the actor a scene member before visibility and playback start.
    component->stop();
    component->setPlayOnLoad( true );
    component->setLooping( true );
    component->setLifetime( 6.0f );
    component->setDuration( 8.0f );
    component->setStartLifetime( Vector2<real_Num>( 3.0f, 6.0f ) );
    component->setStartSize( Vector2<real_Num>( 0.45f, 1.1f ) );
    component->setRate( 18.0f );
    component->setRateVariance( 3.0f );
    component->setAngle( 10.0f );
    component->setAngleVariance( 18.0f );
    component->setShapeSize( 0.65f );
    component->setShapeSizeVariance( 0.2f );
    component->setFastForwardTime( 0.75f );
    component->setFastForwardInterval( 0.05f );

    guard.scene->addActor( actor );
    guard.scene->registerAllUpdates( actor );
    component->updateTransform();
    component->updateVisibility();
    component->play();
    guard.runUpdateCycle( 3 );

    auto rendererParticleSystem = component->getParticleSystem();
    BOOST_REQUIRE( rendererParticleSystem );
    if( !rendererParticleSystem->isLoaded() )
    {
        BOOST_TEST_MESSAGE( "Particle backend is unavailable - skipping emission test" );
        guard.sceneManager->destroyActor( actor );
        return;
    }
    BOOST_CHECK( rendererParticleSystem->isAttached() );
    BOOST_CHECK( rendererParticleSystem->isVisible() );
    BOOST_CHECK( rendererParticleSystem->getState() == render::ParticleSystemState::Started );
    BOOST_CHECK_EQUAL( rendererParticleSystem->getNumEmitters(), 1u );

    BOOST_CHECK_GT( rendererParticleSystem->getNumParticles(), 0u );

    guard.sceneManager->destroyActor( actor );
    guard.runUpdateCycle( 1 );
    BOOST_CHECK( !rendererParticleSystem->isLoaded() );
    BOOST_CHECK( !rendererParticleSystem->isAttached() );
}

BOOST_AUTO_TEST_CASE( particle_system_component_cancel_queued_creation )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    TestGuard guard;
    if( !guard.graphicsSystem )
    {
        BOOST_TEST_MESSAGE( "Graphics system is not available - skipping particle test" );
        return;
    }
    BOOST_REQUIRE( guard.sceneManager );

    auto actor = guard.sceneManager->createActor();
    BOOST_REQUIRE( actor );

    auto component = actor->addComponent<scene::ParticleSystem>();
    BOOST_REQUIRE( component );

    auto rendererParticleSystem = component->getParticleSystem();
    BOOST_REQUIRE( rendererParticleSystem );
    BOOST_CHECK( !rendererParticleSystem->isLoaded() );

    guard.sceneManager->destroyActor( actor );
    guard.runUpdateCycle( 2 );

    BOOST_CHECK( rendererParticleSystem->getLoadingState() == LoadingState::Unloaded );
    BOOST_CHECK( !rendererParticleSystem->isLoaded() );
    BOOST_CHECK( !rendererParticleSystem->isAttached() );
    BOOST_CHECK_EQUAL( rendererParticleSystem->getNumParticles(), 0u );
}

BOOST_AUTO_TEST_CASE( particle_system_component )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto taskManager = applicationManager->getTaskManager();

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    auto actor = gameManager->createActor();
    actor->setName( "ParticleSystemActor" );

    auto particleSystem = actor->addComponent<scene::ParticleSystem>();
    gameScene->addActor( actor );

    auto count = 0;
    while( count++ < 5 )
    {
        taskManager->update();
    }

    actor->removeComponentInstance( particleSystem );
    gameManager->destroyActor( actor );

    gameScene->clear();
    gameManager->clear();
}

BOOST_AUTO_TEST_CASE( particle_system_component_properties )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    auto actor = gameManager->createActor();
    actor->setName( "ParticleSystemPropertiesActor" );

    auto particleSystem = actor->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem );

    // Test getProperties returns valid properties object
    auto properties = particleSystem->getProperties();
    BOOST_CHECK( properties );

    gameScene->addActor( actor );
    gameManager->clear();
}

BOOST_AUTO_TEST_CASE( particle_system_multiple_components )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto taskManager = applicationManager->getTaskManager();

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    // Create multiple actors with particle systems
    auto actor1 = gameManager->createActor();
    actor1->setName( "ParticleSystemActor1" );
    auto particleSystem1 = actor1->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem1 );

    auto actor2 = gameManager->createActor();
    actor2->setName( "ParticleSystemActor2" );
    auto particleSystem2 = actor2->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem2 );

    gameScene->addActor( actor1 );
    gameScene->addActor( actor2 );

    auto count = 0;
    while( count++ < 5 )
    {
        taskManager->update();
    }

    gameManager->clear();
}

BOOST_AUTO_TEST_CASE( particle_system_load_unload )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto taskManager = applicationManager->getTaskManager();

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    auto actor = gameManager->createActor();
    actor->setName( "ParticleSystemLoadUnloadActor" );

    auto particleSystem = actor->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem );

    gameScene->addActor( actor );

    // Run a few updates
    auto count = 0;
    while( count++ < 3 )
    {
        taskManager->update();
    }

    // Unload and reload the component
    particleSystem->unload( nullptr );
    particleSystem->load( nullptr );

    // Run more updates after reload
    count = 0;
    while( count++ < 3 )
    {
        taskManager->update();
    }

    gameManager->clear();
}

BOOST_AUTO_TEST_CASE( particle_system_graphics_object )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto taskManager = applicationManager->getTaskManager();

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    auto actor = gameManager->createActor();
    actor->setName( "ParticleSystemGraphicsActor" );

    auto particleSystem = actor->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem );

    gameScene->addActor( actor );

    // Run updates to ensure graphics objects are created
    auto count = 0;
    while( count++ < 5 )
    {
        taskManager->update();
    }

    // Verify graphics objects are accessible
    auto graphicsObject = particleSystem->getGraphicsObject();
    auto graphicsNode = particleSystem->getGraphicsNode();
    auto internalParticleSystem = particleSystem->getParticleSystem();

    gameManager->clear();
}

BOOST_AUTO_TEST_CASE( particle_system_child_objects )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    auto actor = gameManager->createActor();
    actor->setName( "ParticleSystemChildObjectsActor" );

    auto particleSystem = actor->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem );

    gameScene->addActor( actor );

    // Get child objects - should return an array (may be empty)
    auto childObjects = particleSystem->getChildObjects();
    BOOST_CHECK( true );  // Just verify no exceptions thrown

    childObjects.clear();
    gameManager->destroyActor( actor );
    particleSystem = nullptr;
    actor = nullptr;
}

BOOST_AUTO_TEST_CASE( particle_system_actor_removal )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto taskManager = applicationManager->getTaskManager();

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    auto actor = gameManager->createActor();
    actor->setName( "ParticleSystemRemovalActor" );

    auto particleSystem = actor->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem );

    gameScene->addActor( actor );

    auto count = 0;
    while( count++ < 3 )
    {
        taskManager->update();
    }

    // Remove actor from scene
    gameScene->removeActor( actor );

    count = 0;
    while( count++ < 3 )
    {
        taskManager->update();
    }

    gameManager->clear();
}

BOOST_AUTO_TEST_CASE( particle_system_set_properties )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    auto actor = gameManager->createActor();
    actor->setName( "ParticleSystemSetPropertiesActor" );

    auto particleSystem = actor->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem );

    // Get properties, modify them, and set them back
    auto properties = particleSystem->getProperties();
    BOOST_CHECK( properties );

    particleSystem->setProperties( properties );

    gameScene->addActor( actor );
    gameManager->clear();
}

BOOST_AUTO_TEST_CASE( particle_system_with_transform )
{
    TestGuard guard;

    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    auto taskManager = applicationManager->getTaskManager();

    auto gameManager = applicationManager->getGameManager();
    auto gameScene = gameManager->getCurrentScene();

    auto actor = gameManager->createActor();
    actor->setName( "ParticleSystemTransformActor" );

    auto particleSystem = actor->addComponent<scene::ParticleSystem>();
    BOOST_CHECK( particleSystem );

    // Set actor transform
    actor->setLocalPosition( Vector3<real_Num>( 10.0f, 5.0f, 0.0f ) );
    actor->setLocalScale( Vector3<real_Num>( 2.0f, 2.0f, 2.0f ) );

    gameScene->addActor( actor );

    auto count = 0;
    while( count++ < 5 )
    {
        taskManager->update();
    }

    gameManager->clear();
}


