#include <WPRuntime/WPRuntimePCH.hpp>
#include <WPRuntime/RuntimeApplication.hpp>
#include <Workphone/Workphone.hpp>

#ifdef _WP_STATIC_LIB_
#    include <WPSQLite/WPSQLite.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#    endif

#    if defined USE_FMOD
#        include <WPFMODStudio/WPFMODStudio.hpp>
#    else
#    endif

#    if defined USE_LUA
#        ifdef _FINAL_
#            include <WPLua/WPLua.hpp>
#        else
#            include <WPLua/WPLua.hpp>
#        endif
#    elif defined USE_PYTHON
#        include <WPPython/WPPython.hpp>
#    else
#    endif

#    if WP_BUILD_OISINPUT
#        include <WPOISInput/WPOISInput.hpp>
#    endif

#    if defined USE_FFMPEG
#        include <WPFFMpeg/WPFFMpeg.hpp>
#    endif

#    if __has_include( <FBParticleSystem/FBParticleSystem.hpp> )
#        include <FBParticleSystem/FBParticleSystem.hpp>
#    endif

#    if defined USE_BULLET
#        include <WPBulletPhysics/WPBulletPhysics.hpp>
#    elif defined USE_PHYSX
#        include <WPPhysx/WPPhysx.hpp>
#    elif defined USE_PHYSICS_2D
//#include <FBPhysics2/WPPhysics2.hpp>
#        include <WPODE2/WPPhysics2.hpp>
#    else
#    endif

#    if WP_BUILD_PHYSX
#        include <WPPhysx/WPPhysx.hpp>
#    endif

#    include <WPRuntime/RuntimeProject.hpp>

#    ifdef WP_USE_BOOST
#        include <boost/thread/thread.hpp>
#    endif

#    if WP_USE_TBB
#        include <tbb/tbb_thread.h>
#    endif

#    if WP_USE_ASSET_IMPORT && __has_include( <WPAssimp/WPAssimp.hpp> )
#        include <WPAssimp/WPAssimp.hpp>
#    endif
#    if !defined( __ANDROID__ ) && WP_BUILD_PHYSICS3 && __has_include( <FBPhysics3/FBPhysics3.hpp> )
#        include <FBPhysics3/FBPhysics3.hpp>
#    endif
#    if !defined( __ANDROID__ ) && __has_include( <FBRenderUI/FBRenderUI.hpp> )
#        include <FBRenderUI/FBRenderUI.hpp>
#    endif

#endif

namespace workphone
{
    const f32 RuntimeApplication::DEFAULT_THREAD_UPDATE = 1.0f / 200.0f;
    const f32 RuntimeApplication::DEFAULT_TASK_UPDATE = 1.0f / 60.0f;

    const hash_type RuntimeApplication::INITIALISE_GRAPHICS_SYSTEM_HASH =
        StringUtil::getHash( "initialiseGraphicsSystem" );
    const hash_type RuntimeApplication::INITIALISE_VIDEO_SYSTEM_HASH =
        StringUtil::getHash( "initialiseVideoSystem" );
    const hash_type RuntimeApplication::INITIALISE_INPUT_HASH = StringUtil::getHash( "initialiseInput" );
    const hash_type RuntimeApplication::CREATE_INPUT_HASH = StringUtil::getHash( "createInput" );
    const hash_type RuntimeApplication::CREATE_VIEWPORT_HASH = StringUtil::getHash( "createViewport" );
    const hash_type RuntimeApplication::CREATE_CAMERA_HASH = StringUtil::getHash( "createCamera" );
    const hash_type RuntimeApplication::CREATE_SCENE_HASH = StringUtil::getHash( "createScene" );
    const hash_type RuntimeApplication::CREATE_SCENE_MANAGER_HASH =
        StringUtil::getHash( "createSceneManager" );
    const hash_type RuntimeApplication::ON_INPUT_CHANGED_HASH = StringUtil::getHash( "OnInputChanged" );

    const String RuntimeApplication::DEFAULT_SCENE_MANAGER_NAME = String( "Default" );
    const String RuntimeApplication::DEFAULT_CAMERA_NAME = String( "DefaultCamera" );

    const String RuntimeApplication::configFilePath = String( "wp_plugins_runtime.cfg" );

    WP_CLASS_REGISTER_DERIVED( workphone, RuntimeApplication, core::Application );
    WP_CLASS_REGISTER_DERIVED( workphone, RuntimeApplication::ApplicationListener, IEventListener );

    RuntimeApplication::RuntimeApplication() = default;

    RuntimeApplication::RuntimeApplication( const String &path )
    {
        setProjectFilePath( path );
    }

    RuntimeApplication::~RuntimeApplication() = default;

    void RuntimeApplication::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto task = TaskId::Primary;
            Thread::setCurrentTask( task );

            auto threadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( threadId );

            auto taskFlags = std::numeric_limits<u32>::max();
            Thread::setTaskFlags( taskFlags );

            auto applicationManager = new core::ApplicationManager;
            core::IApplicationManager::setInstance( applicationManager );

            setCreateFrameStatistics( true );
            Application::load( data );

            auto cameraManager = workphone::make_ptr<scene::CameraManager>();
            cameraManager->load( nullptr );
            applicationManager->setCameraManager( cameraManager );

            auto meshManager = workphone::make_ptr<MeshManager>();
            meshManager->load( nullptr );
            applicationManager->setMeshManager( meshManager );

            auto projectFilePath = getProjectFilePath();
            auto projectPath = Path::getFilePath( projectFilePath );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            fileSystem->addFolder( projectPath, true );

            auto scriptManager = applicationManager->getScriptManagerPtr();
            if( scriptManager )
            {
                auto supportedExtensions = scriptManager->getSupportedFileExtensions();
                for( const auto &scriptExt : supportedExtensions )
                {
                    auto scripts = fileSystem->getFileNamesWithExtension( scriptExt );
                    Util::sort( scripts.begin(), scripts.end() );
                    scriptManager->loadScripts( scripts );
                }
            }

            if( !StringUtil::isNullOrEmpty( projectFilePath ) )
            {
                const auto extension =
                    StringUtil::make_lower( Path::getFileExtension( projectFilePath ) );
                if( extension == ".fbproject" || extension == ".fbp" )
                {
                    RuntimeProject project;
                    project.load( projectFilePath );

                    const auto projectPath = project.getPath();
                    WP_ASSERT( !StringUtil::isNullOrEmpty( projectPath ) );
                    if( !StringUtil::isNullOrEmpty( projectPath ) )
                    {
                        applicationManager->setProjectPath( projectPath );
                    }

                    const auto sceneFilePath = project.getSceneFilePath();
                    if( !StringUtil::isNullOrEmpty( sceneFilePath ) )
                    {
                        setSceneFilePath( sceneFilePath );
                    }
                }
                else if( extension == ".fbscene" || extension == ".fbscenexml" ||
                         extension == ".fbscenebin" )
                {
                    setSceneFilePath( projectFilePath );

                    const auto scenePath = Path::getFilePath( projectFilePath );
                    if( !StringUtil::isNullOrEmpty( scenePath ) )
                    {
                        applicationManager->setProjectPath( scenePath );
                    }
                }
                else if( Path::isExistingFolder( projectFilePath ) )
                {
                    applicationManager->setProjectPath( projectFilePath );
                }
            }

            auto jobQueue = applicationManager->getJobQueuePtr();
            WP_ASSERT( jobQueue );
            if( jobQueue )
            {
                jobQueue->startCoroutine(
                    std::bind( &RuntimeApplication::loadSceneCoroutine, this, std::placeholders::_1 ) );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void RuntimeApplication::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );
            m_sceneLoadCoroutineQueued = false;

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            Application::unload( data );

            if( applicationManager )
            {
                applicationManager->unload( nullptr );
            }
            core::IApplicationManager::setInstance( nullptr );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    String RuntimeApplication::getProjectFilePath() const
    {
        return m_projectFilePath;
    }

    void RuntimeApplication::setProjectFilePath( const String &projectFilePath )
    {
        m_projectFilePath = StringUtil::cleanupPath( projectFilePath );
    }

    String RuntimeApplication::getSceneFilePath() const
    {
        return m_sceneFilePath;
    }

    void RuntimeApplication::setSceneFilePath( const String &sceneFilePath )
    {
        m_sceneFilePath = sceneFilePath;
    }

    void RuntimeApplication::loadSceneCoroutine( ICoroutineData::PullType &pull )
    {
        WaitForSeconds( pull, 1.0 );

        auto applicationManager = core::ApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto renderUI = applicationManager->getRenderUI();
        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( fileSystem );
        WP_ASSERT( graphicsSystem );
        WP_ASSERT( renderUI );
        WP_ASSERT( sceneManager );
        if( !applicationManager || !fileSystem || !graphicsSystem || !renderUI || !sceneManager )
        {
            return;
        }

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );
        if( !scene )
        {
            return;
        }

        if( graphicsSystem->isLoaded() && renderUI->isLoaded() && sceneManager->isLoaded() )
        {
            WaitForSeconds( pull, 0.2 );

            String sceneFilePath;
            {
                ScopedLock lock( this );
                sceneFilePath = m_sceneFilePath;
            }

            if( StringUtil::isNullOrEmpty( sceneFilePath ) )
            {
                auto sceneFileNames =
                    fileSystem->getFilesWithExtension( ApplicationUtil::builtinBinarySceneExt );
                auto jsonSceneFileNames =
                    fileSystem->getFilesWithExtension( ApplicationUtil::builtinSceneExt );
                auto xmlSceneFileNames =
                    fileSystem->getFilesWithExtension( ApplicationUtil::builtinXmlSceneExt );
                sceneFileNames.insert( sceneFileNames.end(), jsonSceneFileNames.begin(),
                                       jsonSceneFileNames.end() );
                sceneFileNames.insert( sceneFileNames.end(), xmlSceneFileNames.begin(),
                                       xmlSceneFileNames.end() );
                if( !sceneFileNames.empty() )
                {
                    auto it = std::find_if( sceneFileNames.begin(), sceneFileNames.end(),
                                            []( const FileInfo &fileInfo ) {
                                                return fileInfo.fileName == "Splash.fbscenebin" ||
                                                       fileInfo.fileName == "Splash.fbscene" ||
                                                       fileInfo.fileName == "Splash.fbscenexml";
                                            } );
                    if( it != sceneFileNames.end() )
                    {
                        sceneFilePath = it->absolutePath.c_str();
                    }
                    else
                    {
                        const auto &frontFile = sceneFileNames.front();
                        sceneFilePath = frontFile.fileName.c_str();
                    }
                }
            }

            auto currentScenePath = scene->getFilePath();
            if( !StringUtil::isNullOrEmpty( sceneFilePath ) && currentScenePath != sceneFilePath )
            {
                sceneManager->loadScene( sceneFilePath, false );
            }

            WaitForSeconds( pull, 1.0 );

            auto gameManager = applicationManager->getGameManagerPtr();
            gameManager->play();
        }
    }

    Parameter RuntimeApplication::handleEvent( EventType eventType, hash_type eventValue,
                                               const Array<Parameter> &arguments,
                                               SmartPtr<ISharedObject> sender,
                                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventType == EventType::Loading && eventValue == IEvent::loadingStateChanged )
        {
            if( object )
            {
                if( object->isDerived<render::IGraphicsSystem>() ||
                    object->isDerived<ui::IUIManager>() || object->isDerived<scene::IGameManager>() )
                {
                    if( arguments.size() < 2 )
                    {
                        return {};
                    }

                    auto newState = static_cast<LoadingState>( arguments[1].getS32() );
                    if( newState != LoadingState::Loaded )
                    {
                        return {};
                    }

                    auto applicationManager = core::ApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );
                    if( !applicationManager )
                    {
                        return {};
                    }

                    if( applicationManager->getQuit() || !applicationManager->isRunning() )
                    {
                        return {};
                    }

                    auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                    auto renderUI = applicationManager->getRenderUI();
                    auto sceneManager = applicationManager->getGameManager();
                    if( !graphicsSystem || !renderUI || !sceneManager )
                    {
                        return {};
                    }

                    if( !graphicsSystem->isLoaded() || !renderUI->isLoaded() ||
                        !sceneManager->isLoaded() )
                    {
                        return {};
                    }

                    auto jobQueue = applicationManager->getJobQueuePtr();
                    WP_ASSERT( jobQueue );
                    if( jobQueue && jobQueue->isRunning() )
                    {
                        auto queued = false;
                        while( !m_sceneLoadCoroutineQueued.compare_exchange_weak( queued, true ) )
                        {
                            if( queued )
                            {
                                return {};
                            }
                        }

                        jobQueue->startCoroutine( std::bind( &RuntimeApplication::loadSceneCoroutine,
                                                             this, std::placeholders::_1 ) );
                    }
                }
            }
        }

        return {};
    }

    void RuntimeApplication::createPlugins()
    {
        setPluginsConfigFilePath( configFilePath );

        Application::createPlugins();
    }

    RuntimeApplication::ApplicationListener::ApplicationListener() = default;

    RuntimeApplication::ApplicationListener::~ApplicationListener() = default;

    Parameter RuntimeApplication::ApplicationListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    bool RuntimeApplication::ApplicationListener::inputEvent( SmartPtr<IInputEvent> event )
    {
        WP_ASSERT( event );
        if( !event )
        {
            return false;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return false;
        }

        auto scriptManager = applicationManager->getScriptManager();

        auto eventType = event->getEventType();
        switch( eventType )
        {
        case IInputEvent::EventType::Key:
        {
            auto keyboardState = event->getKeyboardState();
            WP_ASSERT( keyboardState );
            if( !keyboardState )
            {
                return false;
            }

            if( keyboardState->isPressedDown() )
            {
                auto keyCode = static_cast<KeyCodes>( keyboardState->getKeyCode() );
                switch( keyCode )
                {
                case KeyCodes::KEY_F5:
                {
                    if( scriptManager )
                    {
                        scriptManager->reloadScripts();
                    }

                    return true;
                }
                break;
                case KeyCodes::KEY_ESCAPE:
                {
                    applicationManager->setRunning( false );
                    return true;
                }
                break;
                case KeyCodes::KEY_KEY_D:
                {
                }
                break;
                case KeyCodes::KEY_KEY_M:
                {
                }
                break;
                case KeyCodes::KEY_KEY_B:
                {
                }
                break;
                case KeyCodes::KEY_KEY_P:
                {
                    if( auto profiler = applicationManager->getProfiler() )
                    {
                        profiler->logResults();
                        return true;
                    }
                }
                break;
                default:
                {
                }
                }
            }
        }
        break;
        case IInputEvent::EventType::Mouse:
        {
        }
        break;
        default:
        {
        }
        }

        return false;
    }

    SmartPtr<RuntimeApplication> RuntimeApplication::ApplicationListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void RuntimeApplication::ApplicationListener::setOwner( SmartPtr<RuntimeApplication> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone

int WPRuntimeMain( int argc, char *argv[] )
{
    using namespace workphone;

    static const auto defaultAppPath = String( "./" );
    auto appPath = String();

    // parse parameters
    if( argc >= 2 )
    {
        appPath = argv[1];
    }
    else
    {
        appPath = defaultAppPath;
    }

    try
    {
        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            typeManager->load();
            TypeManager::setInstance( typeManager );
        }

        auto app = workphone::make_ptr<RuntimeApplication>();
        app->setProjectFilePath( appPath );
        app->setActiveThreads( 2 );

#if !WP_FINAL
        auto flags = app->getApplicationFlags();
        app->setApplicationFlags( flags | core::IApplication::developerModeFlag );
#endif

        app->load( nullptr );
        app->run();
        app->unload( nullptr );
        app = nullptr;

        if( typeManager )
        {
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
        }
    }
    catch( Exception &e )
    {
        MessageBoxUtil::show( e.what() );
    }
    catch( std::exception &e )
    {
        MessageBoxUtil::show( e.what() );
    }
    catch( ... )
    {
        MessageBoxUtil::show( "Unknown error" );
    }

    return 0;
}

int main( int argc, char *argv[] )
{
    return WPRuntimeMain( argc, argv );
}
