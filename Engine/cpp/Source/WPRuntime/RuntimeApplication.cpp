#include <WPRuntime/WPRuntimePCH.hpp>
#include <WPRuntime/RuntimeApplication.hpp>
#include <WPRuntime/RuntimeStartup.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/PackagePaths.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/System/ProjectRuntimeSettings.hpp>
#include <cstdlib>
#include <filesystem>

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
    namespace
    {
        namespace fs = std::filesystem;

        fs::path executableDirectory()
        {
#ifdef _WIN32
            std::vector<wchar_t> module( 32768 );
            const auto count =
                GetModuleFileNameW( nullptr, module.data(), static_cast<DWORD>( module.size() ) );
            if( count > 0 && count < module.size() )
            {
                return fs::path( std::wstring( module.data(), count ) ).parent_path();
            }
#endif
            return {};
        }

        fs::path userGameDirectory( const String &packageRoot )
        {
            fs::path userDirectory;
#ifdef _WIN32
            if( const auto localAppData = _wgetenv( L"LOCALAPPDATA" ); localAppData && *localAppData )
            {
                userDirectory = localAppData;
            }
#endif
            if( !userDirectory.is_absolute() )
            {
                userDirectory = fs::temp_directory_path();
            }
            const auto id = StringUtil::getUUID( packagePathAsciiLower( packageRoot ) );
            return userDirectory / "Workphone" / "Games" / StringUtil::toString( id ).c_str();
        }
    }  // namespace

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

    RuntimeApplication::~RuntimeApplication()
    {
        clearPackedSession();
    }

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
            applicationManager->load( nullptr );

            // Project settings, writable catalogs and scripts are needed while
            // the base application creates its subsystems, before scene loading.
            const auto startupDirectory = Path::getWorkingDirectory();
            auto projectFilePath = getProjectFilePath();
            auto projectPath = startupDirectory;
            m_packageRoot.clear();
            m_packageArchives.clear();
            m_packageMountError.clear();
            m_packedContent = false;
            if( !projectFilePath.empty() )
            {
                projectFilePath = Path::getAbsolutePath( startupDirectory, projectFilePath );
                if( Path::isExistingFolder( projectFilePath ) )
                {
                    const auto descriptor =
                        fs::u8path( projectFilePath.c_str() ) / runtimePackageProjectName;
                    if( fs::is_regular_file( descriptor ) )
                    {
                        projectFilePath = descriptor.generic_u8string().c_str();
                    }
                }
                setProjectFilePath( projectFilePath );
                const auto extension =
                    StringUtil::make_lower( Path::getFileExtension( projectFilePath ) );
                if( extension == ".fbproject" || extension == ".fbp" )
                {
                    RuntimeProject project;
                    project.load( projectFilePath );
                    const auto documentDirectory = Path::getFilePath( projectFilePath );
                    if( Path::getFileName( projectFilePath ) == runtimePackageProjectName )
                    {
                        const auto root = fs::u8path( documentDirectory.c_str() );
                        validateRuntimePackageArchives( root, project.getPackageArchives() );
                        setSceneFilePath( runtimePackageScenePath( project.getSceneFilePath() ) );
                        const auto version = project.getPackageFormatVersion();
                        if( version != 1 && version != packedContentFormatVersion )
                        {
                            throw std::runtime_error( "Unsupported game package format" );
                        }
                        m_packedContent = version == packedContentFormatVersion;
                        if( !m_packedContent )
                        {
                            if( !fs::is_directory( root / "Media" ) )
                            {
                                throw std::runtime_error( "Missing Media directory beside Runtime.fbproject" );
                            }
                            fs::current_path( root );
                            m_packageMountError = "Unable to initialize the packaged game file system";
                        }
                        m_packageRoot = projectPath = documentDirectory;
                        m_packageArchives = project.getPackageArchives();
                    }
                    else
                    {
                        projectPath = project.getPath().empty()
                                          ? documentDirectory
                                          : Path::getAbsolutePath( documentDirectory, project.getPath() );
                        if( !project.getSceneFilePath().empty() )
                        {
                            setSceneFilePath( Path::getAbsolutePath( projectPath,
                                                                    project.getSceneFilePath() ) );
                        }
                    }
                }
                else if( isRuntimeSceneExtension( extension ) )
                {
                    projectPath = Path::getFilePath( projectFilePath );
                    setSceneFilePath( projectFilePath );
                }
                else if( Path::isExistingFolder( projectFilePath ) )
                {
                    projectPath = projectFilePath;
                }
                else
                {
                    WP_EXCEPTION( "Runtime project or scene does not exist: " + projectFilePath );
                }
            }
            applicationManager->setProjectPath( projectPath );
            if( m_packedContent )
            {
                preparePackedContent();
            }
            else
            {
                const auto cachePath = Path::getAbsolutePath( projectPath, "Cache" );
                const auto settingsPath = Path::getAbsolutePath( projectPath, "SettingsCache" );
                fs::create_directories( fs::u8path( cachePath.c_str() ) );
                fs::create_directories( fs::u8path( settingsPath.c_str() ) );
                applicationManager->setCachePath( cachePath + "/" );
                applicationManager->setSettingsPath( settingsPath + "/" );
            }

            Application::load( data );
            if( !m_packageMountError.empty() )
            {
                throw std::runtime_error( m_packageMountError );
            }
            if( m_packedContent )
            {
                const auto resources = dynamic_pointer_cast<ResourceDatabase>(
                    applicationManager->getResourceDatabase() );
                if( !resources || !resources->getMediaCatalog() )
                {
                    throw std::runtime_error( "Unable to open the packaged Media catalog. See Application.log." );
                }
            }

            if( auto renderUI = applicationManager->getRenderUI() )
            {
                m_fpsText = renderUI->addElementByType<ui::IUIText>();
                if( m_fpsText )
                {
                    m_fpsText->setPosition( Vector2F( 0.81f, 0.055f ) );
                    m_fpsText->setSize( Vector2F( 0.17f, 0.05f ) );
                    m_fpsText->setHorizontalAlignment( static_cast<u8>( HorizontalAlignment::RIGHT ) );
                    m_fpsText->setTextSize( 16.0f );
                    m_fpsText->setColour( ColourF::White );
                    m_fpsText->setHandleInputEvents( false );
                    m_fpsText->setRuntimeLiteralText( true );
                    m_fpsText->setText( "FPS: --" );
                    m_fpsText->setVisible( true );
                    renderUI->setOverlay( m_fpsText );
                }
            }
            m_fpsSampleStart = {};
            m_fpsFrameCount = 0;

            auto cameraManager = workphone::make_ptr<scene::CameraManager>();
            cameraManager->load( nullptr );
            applicationManager->setCameraManager( cameraManager );

            auto meshManager = workphone::make_ptr<MeshManager>();
            meshManager->load( nullptr );
            applicationManager->setMeshManager( meshManager );

            WP_LOG( "Runtime project: " + applicationManager->getProjectPath() );
            // Metadata-only hosts do not install a resource database.
            if( auto resourceDatabase = applicationManager->getResourceDatabase();
                resourceDatabase && !m_packedContent )
            {
                const auto dbFilePath = resourceDatabase->getFilePath();
                auto databasePath = Path::isPathAbsolute( dbFilePath )
                                        ? dbFilePath
                                        : applicationManager->getCachePath() + "/" + dbFilePath;
                auto projectCatalog = resourceDatabase->getDatabaseManager();
                if( projectCatalog )
                {
                    projectCatalog->setDatabasePath( StringUtil::cleanupPath( databasePath ) );
                    projectCatalog->open();
                }
            }

            auto jobQueue = applicationManager->getJobQueuePtr();
            WP_ASSERT( jobQueue );
            if( jobQueue )
            {
                m_sceneLoadCoroutineQueued = true;
                jobQueue->startCoroutine(
                    std::bind( &RuntimeApplication::loadSceneCoroutine, this, std::placeholders::_1 ) );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            const auto app = core::IApplicationManager::instance();
            if( m_packedContent && app && app->getApplication() == this )
            {
                // Stop any subsystems that opened the private catalogs before
                // attempting to remove their backing files.
                Application::unload( data );
            }
            clearPackedSession();
            throw;
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
            if( applicationManager )
            {
                applicationManager->setPlaying( false );
                auto taskManager = applicationManager->getTaskManager();
                auto renderLock = taskManager ? taskManager->lockTask( TaskId::Render )
                                              : TaskLock{};
                m_fpsText = nullptr;
            }
            Application::unload( data );
            clearPackedSession();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void RuntimeApplication::update()
    {
        Application::update();

        if( Thread::getCurrentTask() != TaskId::Render || !m_fpsText )
        {
            return;
        }

        using Clock = std::chrono::steady_clock;
        const auto now = Clock::now();
        if( m_fpsSampleStart == Clock::time_point{} )
        {
            m_fpsSampleStart = now;
            return;
        }

        ++m_fpsFrameCount;
        const auto elapsed = now - m_fpsSampleStart;
        if( elapsed >= std::chrono::milliseconds( 500 ) )
        {
            const auto seconds = std::chrono::duration<f64>( elapsed ).count();
            const auto fps = static_cast<u32>( m_fpsFrameCount / seconds + 0.5 );
            m_fpsText->setText( "FPS: " + StringUtil::toString( fps ) );
            m_fpsSampleStart = now;
            m_fpsFrameCount = 0;
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

        // Loading notifications can arrive before startup has finished. Keep
        // the one queued coroutine alive until every required subsystem is ready.
        while( !graphicsSystem->isLoaded() || !renderUI->isLoaded() || !sceneManager->isLoaded() )
        {
            if( applicationManager->getQuit() || !applicationManager->isRunning() )
            {
                return;
            }
            pull();
        }
        {
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
                                                return fileInfo.fileName == "Application.fbscenebin" ||
                                                       fileInfo.fileName == "Application.fbscene" ||
                                                       fileInfo.fileName == "Application.fbscenexml";
                                            } );
                    if( it != sceneFileNames.end() )
                    {
                        sceneFilePath = it->absolutePath.c_str();
                    }
                    else
                    {
                        const auto &frontFile = sceneFileNames.front();
                        sceneFilePath = frontFile.absolutePath.c_str();
                    }
                }
            }

            if( sceneFilePath.empty() )
            {
                WP_LOG_ERROR( "Runtime startup: no scene was configured or found in the project." );
                return;
            }
            WP_LOG( "Runtime loading scene: " + sceneFilePath );
            if( !loadRuntimeScene( applicationManager, sceneFilePath ) )
            {
                WP_LOG_ERROR( "Runtime startup: scene did not finish loading: " + sceneFilePath );
                return;
            }
            setSceneFilePath( sceneFilePath );

            WaitForSeconds( pull, 1.0 );

            if( applicationManager->getQuit() || !applicationManager->isRunning() )
            {
                return;
            }
            if( startRuntimePlay( applicationManager ) )
            {
                // Scene cameras own their viewports. Remove the bootstrap view
                // once one is ready, so the main window renders the game's
                // camera instead of the temporary startup camera.
                auto taskManager = applicationManager->getTaskManager();
                auto renderLock = taskManager->lockTask( TaskId::Render );
                auto window = graphicsSystem->getDefaultWindow();
                auto sceneCameras = scene->getComponents<scene::Camera>();
                const auto hasMainCamera = std::any_of(
                    sceneCameras.begin(), sceneCameras.end(), [&]( const auto &camera ) {
                        auto viewport = camera->getViewport();
                        return camera->isActive() && viewport && viewport->isActive() &&
                               viewport->getRenderTarget() == window;
                    } );
                if( hasMainCamera )
                {
                    if( auto bootstrapViewport = getViewport() )
                    {
                        bootstrapViewport->setActive( false );
                        bootstrapViewport->setRenderTarget( nullptr );
                        window->removeViewport( bootstrapViewport );
                        setViewport( nullptr );
                    }
                    if( auto bootstrapCamera = getCamera() )
                    {
                        bootstrapCamera->setVisible( false );
                    }
                    WP_LOG( "Runtime handed the main window to the scene camera." );
                }
                WP_LOG( "Runtime entered Play mode: " + sceneFilePath );
            }
            else
            {
                WP_LOG_ERROR( "Runtime startup: cannot enter Play mode: " + sceneFilePath );
            }
        }
    }

    Parameter RuntimeApplication::handleEvent( EventType eventType, hash_type eventValue,
                                               const Array<Parameter> &arguments,
                                               SmartPtr<ISharedObject> sender,
                                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventType == EventType::Loading && eventValue == IEvent::loadingStateChanged )
        {
            if( !isLoaded() )
            {
                return {};
            }
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
        // Launchers may choose a project or source directory as the working
        // directory; the runtime plugin list travels with the executable.
        const auto directory = executableDirectory();
        setPluginsConfigFilePath( directory.empty()
                                      ? configFilePath
                                      : String( ( directory / fs::u8path( configFilePath.c_str() ) )
                                                    .generic_u8string().c_str() ) );

        Application::createPlugins();
    }

    void RuntimeApplication::createFileSystem()
    {
        if( m_packedContent )
        {
            // Prepared before Application::load so the thread and graphics
            // settings can be read from the same mounted archives.
            for( const auto &name : m_packageArchives )
            {
                WP_LOG( "Mounted game archive: " + m_packageRoot + "/" + name );
            }
            return;
        }
        Application::createFileSystem();
        if( m_packageArchives.empty() )
        {
            return;
        }
        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = dynamic_pointer_cast<FileSystem>( applicationManager->getFileSystem() );
        if( !fileSystem )
        {
            throw std::runtime_error( m_packageMountError );
        }
        mountGameArchives( *fileSystem );
        if( !fileSystem->isExistingFile( getSceneFilePath(), false, false ) )
        {
            m_packageMountError = "Packaged startup scene is missing: " + getSceneFilePath();
            throw std::runtime_error( m_packageMountError );
        }
        m_packageMountError.clear();
    }

    void RuntimeApplication::mountGameArchives( FileSystem &files )
    {
        for( const auto &name : m_packageArchives )
        {
            const auto path = m_packageRoot + "/" + name;
            if( !files.addFileArchive( path, true, false, IFileSystem::ArchiveType::ObfuscatedZip ) )
            {
                m_packageMountError = "Unable to mount package archive: " + path;
                throw std::runtime_error( m_packageMountError );
            }
            WP_LOG( "Mounted game archive: " + path );
        }
        setPackagePathRemapping( files, true );
    }

    void RuntimeApplication::preparePackedContent()
    {
        auto app = core::IApplicationManager::instance();
        auto files = make_ptr<FileSystem>();
        files->load( nullptr );
        mountGameArchives( *files );
        setPackageContentOnly( *files, true );
        app->setFileSystem( files );
        app->setMediaPath( getMediaPath() );
        if( !files->open( getSceneFilePath(), true, true, false, false, true ) )
        {
            throw std::runtime_error( "Packaged startup scene is missing: " + getSceneFilePath() );
        }
        loadProjectRuntimeSettings( m_packageRoot );

        const auto userDirectory = userGameDirectory( m_packageRoot );
        const auto settings = userDirectory / "SettingsCache";
        const auto sessions = userDirectory / "Sessions";
        const auto session = sessions / StringUtil::getUUID().c_str();
        const auto cache = session / "Cache";
        fs::create_directories( settings );
        fs::create_directories( sessions );
        if( !fs::create_directory( session ) )
        {
            throw std::runtime_error( "Unable to create private game cache" );
        }
        m_packageSessionPath = session.generic_u8string().c_str();
        fs::create_directory( cache );
        app->setCachePath( String( cache.generic_u8string().c_str() ) + "/" );
        app->setSettingsPath( String( settings.generic_u8string().c_str() ) + "/" );

        auto media = files->open( "Media/.workphone/media-assets.db", true, true, false );
        if( !media )
        {
            throw std::runtime_error( "The package is missing its bundled Media catalog" );
        }
        copyRuntimePackageCatalog( *media, cache / "media-assets.db" );
        if( auto project = files->open( "Cache/asset.db", true, true, false ) )
        {
            copyRuntimePackageCatalog( *project, cache / "asset.db" );
        }

        m_previousWorkingDirectory = fs::current_path().generic_u8string().c_str();
        fs::current_path( userDirectory );
    }

    void RuntimeApplication::clearPackedSession()
    {
        std::error_code ignored;
        if( !m_previousWorkingDirectory.empty() )
        {
            fs::current_path( fs::u8path( m_previousWorkingDirectory.c_str() ), ignored );
            if( !ignored )
            {
                m_previousWorkingDirectory.clear();
            }
        }
        if( !m_packageSessionPath.empty() )
        {
            fs::remove_all( fs::u8path( m_packageSessionPath.c_str() ), ignored );
            if( !ignored )
            {
                m_packageSessionPath.clear();
            }
        }
    }

    String RuntimeApplication::getMediaPath() const
    {
        return m_packageRoot.empty() ? Application::getMediaPath() : m_packageRoot + "/Media";
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

    try
    {
        String appPath = argc >= 2 ? String( argv[1] ) : String( "./" );
        if( argc < 2 )
        {
            const auto directory = executableDirectory();
            if( !directory.empty() )
            {
                const auto descriptor = directory / runtimePackageProjectName;
                if( fs::is_regular_file( descriptor ) )
                {
                    appPath = descriptor.generic_u8string().c_str();
                }
            }
        }

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
        return 1;
    }
    catch( std::exception &e )
    {
        MessageBoxUtil::show( e.what() );
        return 1;
    }
    catch( ... )
    {
        MessageBoxUtil::show( "Unknown error" );
        return 1;
    }

    return 0;
}

int main( int argc, char *argv[] )
{
    return WPRuntimeMain( argc, argv );
}
