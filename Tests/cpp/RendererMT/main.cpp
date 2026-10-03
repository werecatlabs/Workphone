#include <Workphone/Workphone.hpp>

using namespace workphone;

int main()
{
    Thread::setCurrentTask( TaskId::Primary );
    Thread::setCurrentThreadId( Thread::ThreadId::Primary );

    auto applicationManager = new core::ApplicationManager;
    core::ApplicationManager::setInstance( applicationManager );

    auto factoryManager = applicationManager->getFactoryManager();

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

    auto sceneIndex = 2;
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

        auto scene = workphone::make_ptr<scene::GameScene>();
        scene->load( nullptr );
        scene->setLabel( "Untitled" );
        sceneManager->setCurrentScene( scene );

        auto light = workphone::make_ptr<scene::GameActor>();

        auto name = String( "Main Light" );
        light->setName( name );

        auto lightComponent = light->addComponent<scene::Light>();

        scene->addActor( light );
        scene->registerAllUpdates( light );

        auto box = factoryManager->make_ptr<scene::GameActor>();
        auto meshComponent = box->addComponent<scene::Mesh>();
        if( meshComponent )
        {
            meshComponent->setMeshPath( "F40f40.fbmeshbin" );
        }

        auto meshRenderer = box->addComponent<scene::MeshRenderer>();
        auto boxCollider = box->addComponent<scene::CollisionBox>();
        auto rigidBody = box->addComponent<scene::Rigidbody>();

        auto material = box->addComponent<scene::Material>();
        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
        }

        scene->registerAllUpdates( box );
        scene->addActor( box );

        box->setLocalOrientation( Quaternion<real_Num>::angleAxis( -90.0, Vector3<real_Num>::unitX() ) );
        box->setLocalScale( Vector3<real_Num>::unit() * static_cast<real_Num>( 0.05 ) );

        cameraSceneNode->setPosition( Vector3F( 0, 1, 1.5 ) );
        cameraSceneNode->lookAt( Vector3F::zero() );
    }
    break;
    default:
    {
        // do nothing
    }
    }

    graphicsSystem->setupRenderer( renderSceneManager, window, camera, "TestOverlay", true );

    bool running = true;
    while( running )
    {
        stateManager->update();

        auto sceneManager = applicationManager->getGameManager();
        if( sceneManager )
        {
            sceneManager->preUpdate();
            sceneManager->update();
            sceneManager->postUpdate();
        }

        graphicsSystem->messagePump();
        graphicsSystem->update();
    }

    return 0;
}
