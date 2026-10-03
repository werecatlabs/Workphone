#include <Workphone/Workphone.hpp>

using namespace workphone;

int main()
{
    Thread::setCurrentTask( TaskId::Primary );
    Thread::setCurrentThreadId( Thread::ThreadId::Primary );

    // auto logManager = LogManager::instance();
    // logManager->open("Test.log");

    auto applicationManager = new core::ApplicationManager;
    core::ApplicationManager::setInstance( applicationManager );

    auto logManager = workphone::make_ptr<LogManagerDefault>();
    applicationManager->setLogManager( logManager );
    logManager->open( "Renderer.log" );

    auto fsmManager = workphone::make_ptr<FSMManager>();
    applicationManager->setFsmManager( fsmManager );
    WP_ASSERT( applicationManager->getFsmManager() );

    auto factoryManager = workphone::make_ptr<FactoryManager>();
    applicationManager->setFactoryManager( factoryManager );
    WP_ASSERT( applicationManager->getFactoryManager() );

    auto timer = workphone::make_ptr<TimerBoost>();
    applicationManager->setTimer( timer );

    auto stateManager = workphone::make_ptr<StateManager>();
    applicationManager->setStateManager( stateManager );

    auto fileSystem = factoryManager->make_object<IFileSystem>();
    applicationManager->setFileSystem( fileSystem );

    auto mediaPath = String( "" );

#if defined WP_PLATFORM_WIN32
    mediaPath = String( "../../../../Media" );
#elif defined WP_PLATFORM_APPLE
    mediaPath = String( "../../Media" );
#else
    mediaPath = String( "../../Media" );
#endif

    applicationManager->setMediaPath( mediaPath );

    auto packs = fileSystem->getFiles( mediaPath + "/packs" );
    for( auto &pack : packs )
    {
        fileSystem->addFileArchive( pack, true, true, IFileSystem::ArchiveType::Zip );
    }

    fileSystem->addFolder( mediaPath, true );

    auto graphicsSystem = factoryManager->make_object<render::IGraphicsSystem>( "OgreNext" );

    applicationManager->setGraphicsSystem( graphicsSystem );
    graphicsSystem->load( nullptr );

    if( !graphicsSystem->configure( nullptr ) )
    {
        return false;
    }

    auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
    WP_ASSERT( resourceGroupManager );

    resourceGroupManager->load( nullptr );

    auto window = graphicsSystem->getDefaultWindow();

    auto renderSceneManager =
        graphicsSystem->addGraphicsScene( "DefaultSceneManager", "GameSceneManager" );

    auto camera = renderSceneManager->addGraphicsObjectByType<render::IGraphicsCamera>();
    camera->setName( "DefaultCamera" );

    auto cameraSceneNode = renderSceneManager->getRootSceneNode()->addChildSceneNode( "DefaultCamera" );
    cameraSceneNode->attachObject( camera );

    camera->setNearClipDistance( 0.001f );
    camera->setFarClipDistance( 10000.f );

    auto vp = window->addViewport( 0, camera );
    vp->setBackgroundColour( ColourF( 1, 0, 0, 1 ) );
    //camera->setAspectRatio(
    //    static_cast<f32>(vp->getActualWidth()) / static_cast<f32>(vp->getActualHeight()) );
    vp->setClearEveryFrame( true );

    const auto sceneIndex = 3;
    switch( sceneIndex )
    {
    case 0:
    {
        // do nothing
    }
    break;
    case 1:
    {
        auto box = renderSceneManager->addGraphicsObjectByType<render::IGraphicsMesh>();
        box->setMeshName( "cube.fbmeshbin" );

        // box = smgr->addMesh("Barrel.mesh");
        auto boxNode = renderSceneManager->getRootSceneNode()->addChildSceneNode();
        boxNode->attachObject( box );
        boxNode->setScale( Vector3F::unit() * 1.0f );
        box->setVisible( true );

        cameraSceneNode->setPosition( Vector3F( 0, 1, 10 ) );
        cameraSceneNode->lookAt( Vector3F::zero() );
    }
    break;
    case 2:
    {
        auto prefabManager = workphone::make_ptr<scene::GamePrefabManager>();
        applicationManager->setPrefabManager( prefabManager );

        auto sceneManager = workphone::make_ptr<scene::GameManager>();
        applicationManager->setGameManager( sceneManager );
        sceneManager->load( nullptr );

        auto scene = workphone::make_ptr<scene::GameScene>();
        scene->load( nullptr );
        scene->setLabel( "Untitled" );
        sceneManager->setCurrentScene( scene );

        auto light = sceneManager->createActor();

        auto name = String( "Main Light" );
        light->setName( name );

        auto lightComponent = light->addComponent<scene::Light>();

        scene->addActor( light );
        scene->registerAllUpdates( light );

        auto box = sceneManager->createActor();
        auto meshComponent = box->addComponent<scene::Mesh>();
        if( meshComponent )
        {
            meshComponent->setMeshPath( "cube.fbmeshbin" );
        }

        auto meshRenderer = box->addComponent<scene::MeshRenderer>();
        auto boxCollider = box->addComponent<scene::CollisionBox>();
        auto rigidBody = box->addComponent<scene::Rigidbody>();

        auto materialComponent = box->addComponent<scene::Material>();
        if( materialComponent )
        {
            materialComponent->setMaterialPath( "Standard.mat" );

            auto material = materialComponent->getMaterial();
            if( material )
            {
                auto textureName = String( "checker.png" );
                material->setTexture( textureName );
            }
        }

        scene->registerAllUpdates( box );
        scene->addActor( box );

        box->setLocalOrientation( Quaternion<real_Num>::angleAxis( -90.0, Vector3<real_Num>::unitX() ) );
        // box->setLocalScale(Vector3<real_Num>::unit() * real_Num(0.05));

        auto debug = graphicsSystem->getDebug();
        debug->drawPoint( 1, Vector3F::zero(), 0xFFFFFF );

        cameraSceneNode->setPosition( Vector3F( 0, 1, 1.0 ) );
        cameraSceneNode->lookAt( Vector3F::zero() );
    }
    break;
    case 3:
    {
        auto debug = graphicsSystem->getDebug();
        debug->drawPoint( 1, Vector3F::zero(), 0xFFFFFF );
        debug->drawLine( 2, Vector3F::zero(), Vector3F::unitY(), 0xFFFFFF );

        cameraSceneNode->setPosition( Vector3F( 0, 1, 10 ) );
        cameraSceneNode->lookAt( Vector3F::zero() );
    }
    break;
    case 4:
    {
        auto debug = graphicsSystem->getDebug();
        debug->drawPoint( 1, Vector3F::zero(), 0xFFFFFF );
        debug->drawLine( 2, Vector3F::zero(), Vector3F::unitY(), 0xFFFFFF );

        cameraSceneNode->setPosition( Vector3F( 0, 1, 10 ) );
        cameraSceneNode->lookAt( Vector3F::zero() );
    }
    break;
    default:
    {
        // do nothing
    }
    }

    graphicsSystem->setupRenderer( renderSceneManager, window, camera, "TestOverlay", true );

    while( applicationManager->isRunning() )
    {
        timer->update();
        auto t = timer->getTime();
        auto dt = timer->getDeltaTime();

        stateManager->update();

        auto sceneManager = applicationManager->getGameManager();
        if( sceneManager )
        {
            sceneManager->preUpdate();
            sceneManager->update();
            sceneManager->postUpdate();
        }

        switch( sceneIndex )
        {
        case 2:
        {
            auto debug = graphicsSystem->getDebug();
            debug->drawPoint( 1, Vector3F::zero(), 0xFFFFFF );
        }
        break;
        case 3:
        {
            auto debug = graphicsSystem->getDebug();
            //debug->drawPoint( 1, Vector3F::zero(), 0xFFFFFF );
            debug->drawLine( 2, Vector3F::zero(), Vector3F::unitY(), 0xFFFFFF );
        }
        break;
        }

        graphicsSystem->update();

        graphicsSystem->messagePump();
        if( applicationManager->getQuit() )
        {
            applicationManager->setRunning( false );
        }
    }

    applicationManager->unload( nullptr );
    core::ApplicationManager::setInstance( nullptr );

    return 0;
}
