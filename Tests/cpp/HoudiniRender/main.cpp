#include <FBCore/FBCore.hpp>

#if FB_GRAPHICS_SYSTEM_OGRENEXT
#include <FBGraphicsOgreNext/FBGraphicsOgreNext.hpp>
#elif FB_GRAPHICS_SYSTEM_OGRE
#include <FBGraphicsOgre/FBGraphicsOgre.hpp>
#endif


#include <FBRenderUI/FBRenderUI.hpp>
#include <FBCore/Interface/Procedural/IProceduralEngine.hpp>




using namespace fb;


int main()
{
    auto task = Thread::Task::Primary;
    Thread::setCurrentTask( task );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );

    auto applicationManager = fb::make_ptr<core::ApplicationManagerMT>();
    core::IApplicationManager::setInstance( applicationManager );

    auto factoryManager = fb::make_ptr<FactoryManager>();
    applicationManager->setFactoryManager( factoryManager );
    FB_ASSERT( applicationManager->getFactoryManager() );

    auto timer = fb::make_ptr<TimerBoost>();
    applicationManager->setTimer( timer );

    auto logManager = fb::make_ptr<LogManagerDefault>();
    applicationManager->setLogManager( logManager );
    logManager->open( "HoudiniRender.log" );

    auto stateManager = fb::make_ptr<StateManagerStandard>();
    applicationManager->setStateManager( stateManager );

    auto fileSystem = factoryManager->make_object<IFileSystem>();
    applicationManager->setFileSystem( fileSystem );

    auto workingDirectory = Path::getWorkingDirectory();
    fileSystem->addFolder( workingDirectory );

    auto mediaFolderPath = String( "" );

#if defined FB_PLATFORM_WIN32
    mediaFolderPath = String( "../../../../Media/" );
#elif defined FB_PLATFORM_APPLE
	mediaFolderPath = String("../../Media/");
#endif

    static const auto packExt = String( ".zip" );

    auto packs = fileSystem->getFiles( mediaFolderPath + "/packs" );
    for(auto &pack : packs)
    {
        auto ext = Path::getFileExtension( pack );
        if(ext == packExt)
        {
            fileSystem->addFileArchive( pack, true, true, IFileSystem::ArchiveType::Zip );
        }
    }

    fileSystem->addFolder( mediaFolderPath, true );

    applicationManager->setMediaPath( mediaFolderPath );

#if FB_GRAPHICS_SYSTEM_OGRENEXT
	auto graphicsSystem = render::FBGraphicsOgreNext::createGraphicsOgre();
#elif FB_GRAPHICS_SYSTEM_OGRE
	auto graphicsSystem = render::FBGraphicsOgre::createGraphicsOgre();
#else
    SmartPtr<render::IGraphicsState> graphicsSystem;
#endif

    applicationManager->setGraphicsSystem( graphicsSystem );
    graphicsSystem->load( nullptr );

    if(!graphicsSystem->configure( nullptr ))
    {
        return false;
    }

    auto window = graphicsSystem->getDefaultWindow();

    auto renderSceneManager = graphicsSystem->addGraphicsScene( "DefaultSceneManager",
                                                               "GameSceneManager" );

    auto camera = renderSceneManager->addCamera( "DefaultCamera" );
    auto cameraSceneNode = renderSceneManager->getRootSceneNode()->addChildSceneNode( "DefaultCamera" );
    cameraSceneNode->attachObject( camera );

    camera->setNearClipDistance( 0.001f );
    camera->setFarClipDistance( 10000.f );

    auto vp = window->addViewport( 0, camera );
    vp->setBackgroundColour( ColourF( 1, 0, 0, 1 ) );
    //camera->setAspectRatio(
    //    static_cast<f32>(vp->getActualWidth()) / static_cast<f32>(vp->getActualHeight()) );
    vp->setClearEveryFrame( true );

    auto roadHDAFullPath = String( "road.hda" );
    //auto session = houdini::SessionManager::getOrCreateDefaultSession();
    //auto rootGO = houdini::HAPIUtility::instantiateHDA(roadHDAFullPath, Vector3F::zero(), session, true);

    //auto proceduralEngine = houdini::Houdini::createProceduralEngine();
    //auto rootGO = proceduralEngine->import( roadHDAFullPath );

    //graphicsSystem->setupRenderer( renderSceneManager, window, camera, "TestOverlay", true );

    //while(applicationManager->isRunning())
    //{
    //    timer->update();

    //    stateManager->update();

    //    graphicsSystem->update();

    //    graphicsSystem->messagePump();
    //    if(applicationManager->getQuit())
    //    {
    //        applicationManager->setRunning( false );
    //    }
    //}

    applicationManager->unload( nullptr );
    core::IApplicationManager::setInstance( nullptr );
    return 0;
}
