#include "UnitTests.hpp"
#include "TypeManagerFixture.hpp"
#include "UnitTestsFixture.hpp"
#include "Workphone/Workphone.hpp"
#include <boost/test/unit_test.hpp>
#include <memory>

#ifdef _WP_STATIC_LIB_
#    include "FBAssimp/FBAssimp.hpp"
#    include "FBAudio/FBAudio.hpp"
#    include "FBRenderUI/FBRenderUI.hpp"
#    include <FBProcedural/FBProcedural.hpp>
#    include <WPSQLite/WPSQLite.hpp>
#    include <FBOISInput/FBOISInput.hpp>

#    if WP_BUILD_PHYSX
#        include <FBPhysx/FBPhysx.hpp>
#    elif WP_BUILD_PHYSICS3
#        include "FBPhysics3/FBPhysics3.hpp"
#    endif

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#    endif

#    if WP_ENABLE_LUA
#        include <FBLua/FBLua.hpp>
#    elif WP_ENABLE_PYTHON
#        include <FBPython/FBPython.hpp>
#    endif
#endif

namespace workphone
{
    using namespace scene;

    namespace
    {
        void releaseInitialReference( core::ApplicationManager *ptr )
        {
            if( ptr )
            {
                ptr->removeReference();
            }
        }
    }  // namespace

    UnitTestsFixture *UnitTests::m_fixture = nullptr;
    //TypeManagerFixture UnitTests::sTypeManagerFixture = TypeManagerFixture();

    UnitTestsFixture *UnitTests::getFixture()
    {
        return m_fixture;
    }

    void UnitTests::setFixture( UnitTestsFixture *fixture )
    {
        m_fixture = fixture;
    }

    SmartPtr<WorkphonePlugin> UnitTests::m_plugin;
    TypeManager *UnitTests::sTypeManager = nullptr;
    // m_pluginsFilePath is implemented as a function-local static inside getPluginsFilePath()
// to avoid the static destruction order fiasco (the static instance of
// CharacterPoolAllocator is lazily initialised and would be destroyed before
// a global String static member, leaving a dangling pool pointer).

    void UnitTests::setupPlugins()
    {
        WP_DEBUG_TRACE;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

#ifndef WP_STATIC_LIB
        auto corePlugin = workphone::make_ptr<WorkphonePlugin>();
        applicationManager->addPlugin( corePlugin );
        m_plugin = corePlugin;

        auto configFile = factoryManager->make_object<IConfigFile>();

        auto pluginsConfigFilePath = getPluginsFilePath();
        configFile->loadFromFilePath( pluginsConfigFilePath );

        auto pluginKey = String( "Plugin" );
        auto plugins = configFile->getSettings( pluginKey );

        for( auto pluginPath : plugins )
        {
            auto job = factoryManager->make_ptr<LoadPluginJob>();
            job->setPluginPath( pluginPath );
            job->execute();
        }
#else
        auto corePlugin = workphone::make_ptr<WPCore>();
        applicationManager->addPlugin( corePlugin );

        auto audioPlugin = workphone::make_ptr<FBAudio>();
        applicationManager->addPlugin( audioPlugin );

        auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
        applicationManager->addPlugin( databasePlugin );

        auto inputPlugin = workphone::make_ptr<OISInput>();
        applicationManager->addPlugin( inputPlugin );
#endif
    }

    void UnitTests::setupDefault()
    {
        WP_DEBUG_TRACE;

        auto currentThreadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( currentThreadId );

        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto taskFlags = std::numeric_limits<u32>::max();
        Thread::setTaskFlags( taskFlags );

        auto applicationManagerOwner =
            std::unique_ptr<core::ApplicationManager, decltype( &releaseInitialReference )>(
                new core::ApplicationManager, &releaseInitialReference );
        auto applicationManager = applicationManagerOwner.get();
        BOOST_CHECK( applicationManager );

        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = sTypeManager;
            TypeManager::setInstance( typeManager );
        }

        applicationManager->load( nullptr );

        core::IApplicationManager::setInstance( applicationManager );
        applicationManagerOwner.release();
        BOOST_CHECK( core::IApplicationManager::instance() );

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( nullptr );
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto pluginManager = factoryManager->make_ptr<core::PluginManager>();
        applicationManager->setPluginManager( pluginManager );

        auto logManager = factoryManager->make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        setupPlugins();

        auto jobQueue = factoryManager->make_ptr<JobQueue>();
        jobQueue->load( nullptr );
        applicationManager->setJobQueue( jobQueue );

        auto taskManager = factoryManager->make_ptr<TaskManager>();
        applicationManager->setTaskManager( taskManager );

        auto fileSystem = factoryManager->make_ptr<FileSystem>();
        BOOST_CHECK( fileSystem );
        applicationManager->setFileSystem( fileSystem );
        BOOST_CHECK( applicationManager->getFileSystem() );

        auto mediaFolderPath = String( "" );

#if defined WP_PLATFORM_WIN32
        mediaFolderPath = String( "../../../../../Media" );
#elif defined WP_PLATFORM_APPLE
        mediaFolderPath = String( "../../Media/" );
#else
        mediaFolderPath = String( "../../Media/" );
#endif

        auto workingDirectory = Path::getWorkingDirectory();
        fileSystem->addFolder( workingDirectory );

        auto mediaAbsolutePath = Path::getAbsolutePath( workingDirectory, mediaFolderPath );

        auto cachePath = applicationManager->getCachePath();
        auto cacheAbsolutePath = Path::getAbsolutePath( workingDirectory, cachePath );
        fileSystem->addFolder( cacheAbsolutePath );

        auto settingsCachePath = applicationManager->getSettingsPath();
        auto settingsCacheAbsolutePath = Path::getAbsolutePath( workingDirectory, settingsCachePath );
        fileSystem->addFolder( settingsCacheAbsolutePath );

        fileSystem->addFolder( mediaFolderPath, false );

        applicationManager->setMediaPath( mediaFolderPath );

        auto stateManager = factoryManager->make_ptr<StateManager>();
        applicationManager->setStateManager( stateManager );
        BOOST_CHECK( applicationManager->getStateManager() );

        auto timer = factoryManager->make_ptr<TimerMT>();
        applicationManager->setTimer( timer );

        auto fsmManager = factoryManager->make_ptr<FSMManager>();
        fsmManager->load( nullptr );
        WP_ASSERT( fsmManager->isLoaded() );
        WP_ASSERT( fsmManager->isValid() );

        applicationManager->setFsmManager( fsmManager );
        WP_ASSERT( applicationManager->isValid() );

        auto meshManager = factoryManager->make_ptr<MeshManager>();
        applicationManager->setMeshManager( meshManager );

        auto meshLoader = factoryManager->make_object<IMeshLoader>();
        applicationManager->setMeshLoader( meshLoader );

        auto resourceDatabase = factoryManager->make_ptr<ResourceDatabase>();
        resourceDatabase->load( nullptr );
        applicationManager->setResourceDatabase( resourceDatabase );

        BOOST_CHECK( applicationManager->getPhysicsManager() == nullptr );
        auto physicsManager = factoryManager->make_object<physics::IPhysicsManager>();
        if( physicsManager )
        {
            physicsManager->load( nullptr );
            applicationManager->setPhysicsManager( physicsManager );
            BOOST_CHECK( applicationManager->getPhysicsManager() != nullptr );
        }

        auto soundManager = factoryManager->make_object<ISoundManager>();
        if( soundManager )
        {
            soundManager->load( nullptr );
            applicationManager->setSoundManager( soundManager );
        }

        // create scene
        if( physicsManager )
        {
            auto physicsScene = physicsManager->addScene();
            physicsManager->setPhysicsScene( physicsScene );
        }

        auto prefabManager = factoryManager->make_ptr<GamePrefabManager>();
        applicationManager->setPrefabManager( prefabManager );

        auto sceneManager = factoryManager->make_ptr<GameManager>();
        applicationManager->setGameManager( sceneManager );
        sceneManager->load( nullptr );

        auto scene = factoryManager->make_ptr<GameScene>();
        scene->load( nullptr );
        scene->setLabel( "Untitled" );
        sceneManager->setCurrentScene( scene );
    }

    void UnitTests::setupGraphics()
    {
        WP_DEBUG_TRACE;

        auto currentThreadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( currentThreadId );

        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto taskFlags = std::numeric_limits<u32>::max();
        Thread::setTaskFlags( taskFlags );

        auto applicationManagerOwner =
            std::unique_ptr<core::ApplicationManager, decltype( &releaseInitialReference )>(
                new core::ApplicationManager, &releaseInitialReference );
        auto applicationManager = applicationManagerOwner.get();
        BOOST_CHECK( applicationManager );

        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = sTypeManager;
            TypeManager::setInstance( typeManager );
        }

        applicationManager->load( nullptr );

        core::IApplicationManager::setInstance( applicationManager );
        applicationManagerOwner.release();
        BOOST_CHECK( core::IApplicationManager::instance() );

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( nullptr );
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto pluginManager = factoryManager->make_ptr<core::PluginManager>();
        applicationManager->setPluginManager( pluginManager );

        auto logManager = factoryManager->make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        setupPlugins();

        auto jobQueue = factoryManager->make_ptr<JobQueue>();
        jobQueue->load( nullptr );
        applicationManager->setJobQueue( jobQueue );

        auto taskManager = factoryManager->make_ptr<TaskManager>();
        applicationManager->setTaskManager( taskManager );

        auto fileSystem = factoryManager->make_object<IFileSystem>();
        applicationManager->setFileSystem( fileSystem );
        BOOST_CHECK( fileSystem );

        auto workingDirectory = Path::getWorkingDirectory();
        fileSystem->addFolder( workingDirectory, false );

        auto mediaPath = String( "" );
        auto scriptsPath = String( "" );

#if !WP_FINAL
#    if defined WP_PLATFORM_WIN32
        mediaPath = String( "../../../../../Media" );
        scriptsPath = String( "../../../../Media/Scripts" );
#    elif defined WP_PLATFORM_APPLE
        mediaPath = String( "../../../../../Media/" );
        scriptsPath = String( "../../../../../Media/Scripts/" );
#    else
        mediaPath = String( "../../Media/" );
#    endif

        auto cachePath = applicationManager->getCachePath();
        auto cacheAbsolutePath = Path::getAbsolutePath( workingDirectory, cachePath );
        fileSystem->addFolder( cacheAbsolutePath );

        auto settingsCachePath = applicationManager->getSettingsPath();
        auto settingsCacheAbsolutePath = Path::getAbsolutePath( workingDirectory, settingsCachePath );
        fileSystem->addFolder( settingsCacheAbsolutePath );

        fileSystem->addFolder( scriptsPath, true );
        fileSystem->addFolder( mediaPath, true );

        applicationManager->setMediaPath( mediaPath );
#else
        applicationManager->setMediaPath( "./" );
        mediaPath = String( "./Media" );
        fileSystem->addFolder( mediaPath, true );

        fileSystem->addFileArchive( "Media.zip", false, false, IFileSystem::ArchiveType::Zip );
#endif

        auto stateManager = factoryManager->make_ptr<StateManager>();
        applicationManager->setStateManager( stateManager );
        BOOST_CHECK( applicationManager->getStateManager() );

        auto timer = factoryManager->make_ptr<TimerMT>();
        applicationManager->setTimer( timer );

        auto fsmManager = factoryManager->make_ptr<FSMManager>();
        fsmManager->load( nullptr );
        WP_ASSERT( fsmManager->isLoaded() );
        WP_ASSERT( fsmManager->isValid() );

        applicationManager->setFsmManager( fsmManager );
        WP_ASSERT( applicationManager->isValid() );

        auto meshManager = factoryManager->make_ptr<MeshManager>();
        applicationManager->setMeshManager( meshManager );

        auto meshLoader = factoryManager->make_object<IMeshLoader>();
        applicationManager->setMeshLoader( meshLoader );

        auto graphicsSystem = factoryManager->make_object<render::IGraphicsSystem>();
        if( graphicsSystem )
        {
            graphicsSystem->load( nullptr );

            if( graphicsSystem->configure( nullptr ) )
            {
                applicationManager->setGraphicsSystem( graphicsSystem );
                auto window = graphicsSystem->getDefaultWindow();
                BOOST_CHECK( window );

                auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
                BOOST_CHECK( resourceGroupManager );
                resourceGroupManager->load( nullptr );

                BOOST_CHECK( resourceGroupManager->isLoaded() );
                BOOST_CHECK( resourceGroupManager->isValid() );

                auto renderSceneManager =
                    graphicsSystem->addGraphicsScene( "DefaultSceneManager", "GameSceneManager" );
                BOOST_CHECK( graphicsSystem->getGraphicsScene() );

                auto camera = renderSceneManager->addGraphicsObjectByType<render::IGraphicsCamera>();
                camera->setName( "DefaultCamera" );

                auto cameraSceneNode =
                    renderSceneManager->getRootSceneNode()->addChildSceneNode( "DefaultCamera" );
                cameraSceneNode->attachObject( camera );

                camera->setNearClipDistance( 0.001f );
                camera->setFarClipDistance( 10000.f );

                cameraSceneNode->setPosition( Vector3F( 1000, 800, 1000 ) );
                cameraSceneNode->lookAt( Vector3F::zero() );
            }
            else
            {
                graphicsSystem->unload( nullptr );
                graphicsSystem = nullptr;
            }
        }

        auto renderUI = applicationManager->getRenderUI();
        if( !renderUI )
        {
            if( graphicsSystem )
            {
                if( auto graphicsFactory = graphicsSystem->getFactoryManager() )
                {
                    renderUI = graphicsFactory->make_object<ui::IUIManager>();
                }
            }
        }
        if( !renderUI )
        {
            renderUI = factoryManager->make_object<ui::IUIManager>();
        }

        if( renderUI )
        {
            applicationManager->setRenderUI( renderUI );

            if( graphicsSystem )
            {
                graphicsSystem->loadObject( renderUI, true );
            }

            if( !renderUI->isLoaded() )
            {
                renderUI->load( nullptr );
            }
        }

        auto resourceDatabase = factoryManager->make_ptr<ResourceDatabase>();
        resourceDatabase->load( nullptr );
        applicationManager->setResourceDatabase( resourceDatabase );

        auto prefabManager = factoryManager->make_ptr<GamePrefabManager>();
        applicationManager->setPrefabManager( prefabManager );
    }

    void UnitTests::setupGame()
    {
        WP_DEBUG_TRACE;

        auto currentThreadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( currentThreadId );

        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto taskFlags = std::numeric_limits<u32>::max();
        Thread::setTaskFlags( taskFlags );

        auto applicationManagerOwner =
            std::unique_ptr<core::ApplicationManager, decltype( &releaseInitialReference )>(
                new core::ApplicationManager, &releaseInitialReference );
        auto applicationManager = applicationManagerOwner.get();
        BOOST_CHECK( applicationManager );

        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = sTypeManager;
            TypeManager::setInstance( typeManager );
        }

        applicationManager->load( nullptr );

        core::IApplicationManager::setInstance( applicationManager );
        applicationManagerOwner.release();
        BOOST_CHECK( core::IApplicationManager::instance() );

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( nullptr );
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto pluginManager = factoryManager->make_ptr<core::PluginManager>();
        applicationManager->setPluginManager( pluginManager );

        auto logManager = factoryManager->make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        setupPlugins();

        auto aiManager = factoryManager->make_ptr<AiManager>();
        aiManager->load( nullptr );
        applicationManager->setAiManager( aiManager );

        auto profiler = factoryManager->make_object<IProfiler>();
        applicationManager->setProfiler( profiler );

        auto fsmManager = factoryManager->make_ptr<FSMManager>();
        fsmManager->load( nullptr );
        WP_ASSERT( fsmManager->isLoaded() );
        WP_ASSERT( fsmManager->isValid() );

        applicationManager->setFsmManager( fsmManager );
        WP_ASSERT( applicationManager->isValid() );

        auto jobQueue = factoryManager->make_ptr<JobQueue>();
        jobQueue->load( nullptr );
        applicationManager->setJobQueue( jobQueue );

        auto taskManager = factoryManager->make_ptr<TaskManager>();
        applicationManager->setTaskManager( taskManager );
        taskManager->load( nullptr );
        applicationManager->setTaskManager( taskManager );

        auto fileSystem = factoryManager->make_object<IFileSystem>();
        applicationManager->setFileSystem( fileSystem );
        BOOST_CHECK( fileSystem );

        auto workingDirectory = Path::getWorkingDirectory();
        fileSystem->addFolder( workingDirectory, false );

        auto mediaFolderPath = String( "" );

#if defined WP_PLATFORM_WIN32
        mediaFolderPath = String( "../../../../../Media" );
#elif defined WP_PLATFORM_APPLE
        mediaFolderPath = String( "../../Media/" );
#else
        mediaFolderPath = String( "../../Media/" );
#endif

        auto mediaAbsolutePath = Path::getAbsolutePath( workingDirectory, mediaFolderPath );

        auto cachePath = applicationManager->getCachePath();
        auto cacheAbsolutePath = Path::getAbsolutePath( workingDirectory, cachePath );
        fileSystem->addFolder( cacheAbsolutePath );

        auto settingsCachePath = applicationManager->getSettingsPath();
        auto settingsCacheAbsolutePath = Path::getAbsolutePath( workingDirectory, settingsCachePath );
        fileSystem->addFolder( settingsCacheAbsolutePath );

        fileSystem->addFolder( mediaFolderPath, false );
        fileSystem->addFolder( mediaFolderPath + "/Scripts", true );
        fileSystem->addFolder( mediaFolderPath + "/Tests", true );

        auto absoluteScriptsPath = Path::lexically_normal( workingDirectory, mediaFolderPath );
        applicationManager->setMediaPath( mediaFolderPath );

        auto stateManager = factoryManager->make_ptr<StateManager>();
        stateManager->load( nullptr );
        applicationManager->setStateManager( stateManager );
        BOOST_CHECK( applicationManager->getStateManager() );

        auto timer = factoryManager->make_ptr<TimerMT>();
        timer->load( nullptr );
        applicationManager->setTimer( timer );

        auto meshManager = factoryManager->make_ptr<MeshManager>();
        applicationManager->setMeshManager( meshManager );

        auto meshLoader = factoryManager->make_object<IMeshLoader>();
        applicationManager->setMeshLoader( meshLoader );

        auto graphicsSystem = factoryManager->make_object<render::IGraphicsSystem>();
        if( graphicsSystem )
        {
            applicationManager->setGraphicsSystem( graphicsSystem );

            graphicsSystem->load( nullptr );

            if( graphicsSystem->configure( nullptr ) )
            {
                applicationManager->setGraphicsSystem( graphicsSystem );
                auto window = graphicsSystem->getDefaultWindow();
                BOOST_CHECK( window );

                auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
                BOOST_CHECK( resourceGroupManager );
                resourceGroupManager->load( nullptr );

                BOOST_CHECK( resourceGroupManager->isLoaded() );
                BOOST_CHECK( resourceGroupManager->isValid() );

                auto renderSceneManager =
                    graphicsSystem->addGraphicsScene( "DefaultSceneManager", "GameSceneManager" );
                BOOST_CHECK( graphicsSystem->getGraphicsScene() );

                auto camera = renderSceneManager->addGraphicsObjectByType<render::IGraphicsCamera>();
                camera->setName( "DefaultCamera" );

                auto cameraSceneNode =
                    renderSceneManager->getRootSceneNode()->addChildSceneNode( "DefaultCamera" );
                cameraSceneNode->attachObject( camera );

                camera->setNearClipDistance( 0.001f );
                camera->setFarClipDistance( 10000.f );

                cameraSceneNode->setPosition( Vector3F( 1000, 800, 1000 ) );
                cameraSceneNode->lookAt( Vector3F::zero() );
            }
            else
            {
                graphicsSystem->unload( nullptr );
                graphicsSystem = nullptr;
            }
        }

        auto renderUI = applicationManager->getRenderUI();
        if( !renderUI )
        {
            if( graphicsSystem )
            {
                if( auto graphicsFactory = graphicsSystem->getFactoryManager() )
                {
                    renderUI = graphicsFactory->make_object<ui::IUIManager>();
                }
            }
        }
        if( !renderUI )
        {
            renderUI = factoryManager->make_object<ui::IUIManager>();
        }

        if( renderUI )
        {
            applicationManager->setRenderUI( renderUI );

            if( graphicsSystem )
            {
                graphicsSystem->loadObject( renderUI, true );
            }

            if( !renderUI->isLoaded() )
            {
                renderUI->load( nullptr );
            }
        }

        auto resourceDatabase = factoryManager->make_ptr<ResourceDatabase>();
        resourceDatabase->load( nullptr );
        applicationManager->setResourceDatabase( resourceDatabase );

        BOOST_CHECK( applicationManager->getPhysicsManager() == nullptr );
        auto physicsManager = factoryManager->make_object<physics::IPhysicsManager>();
        if( physicsManager )
        {
            physicsManager->load( nullptr );
            applicationManager->setPhysicsManager( physicsManager );
            BOOST_CHECK( applicationManager->getPhysicsManager() != nullptr );
        }

        auto soundManager = factoryManager->make_object<ISoundManager>();
        if( soundManager )
        {
            soundManager->load( nullptr );
            applicationManager->setSoundManager( soundManager );
        }

        // create scene
        if( physicsManager )
        {
            auto physicsScene = physicsManager->addScene();
            physicsManager->setPhysicsScene( physicsScene );
        }

        auto scriptManager = factoryManager->make_object<IScriptManager>();
        if( scriptManager )
        {
            scriptManager->load( nullptr );
            applicationManager->setScriptManager( scriptManager );
        }

        auto prefabManager = factoryManager->make_ptr<GamePrefabManager>();
        applicationManager->setPrefabManager( prefabManager );

        auto sceneManager = factoryManager->make_ptr<GameManager>();
        applicationManager->setGameManager( sceneManager );
        sceneManager->load( nullptr );

        auto scene = factoryManager->make_ptr<GameScene>();
        scene->load( nullptr );
        scene->setLabel( "Untitled" );
        sceneManager->setCurrentScene( scene );

        const auto filePath = String( "G:/TestProject/Assets/vehicle_test.fbscenexml" );
        auto stream = fileSystem->open( filePath, true, false, false, true, true );
        if( !stream )
        {
            stream = fileSystem->open( filePath, true, false, false, true, true );
        }

        if( stream )
        {
            auto dataStr = stream->getAsString();
            int i = 0;
        }
    }

    void UnitTests::setupFactories()
    {
        /*
        WP_DEBUG_TRACE;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

#ifdef _WP_STATIC_LIB_
        auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
        applicationManager->addPlugin( databasePlugin );
#endif

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        FactoryUtil::addFactory<GameActor>();

        FactoryUtil::addFactory<CarController>();
        FactoryUtil::addFactory<Constraint>();
        FactoryUtil::addFactory<CollisionBox>();
        FactoryUtil::addFactory<scene::CollisionMesh>();
        FactoryUtil::addFactory<Material>();
        FactoryUtil::addFactory<scene::Mesh>();
        FactoryUtil::addFactory<MeshRenderer>();
        FactoryUtil::addFactory<Rigidbody>();
        FactoryUtil::addFactory<WheelController>();

        FactoryUtil::addFactory<StateMessageVector3>();
        FactoryUtil::addFactory<StateMessageVector4>();
        FactoryUtil::addFactory<StateMessageUIntValue>();
        FactoryUtil::addFactory<StateMessageIntValue>();
        FactoryUtil::addFactory<StateMessageVisible>();

        factoryManager->setPoolSizeByType<GameActor>( 128 );
        factoryManager->setPoolSizeByType<StateMessageVector3>( 128 );
        factoryManager->setPoolSizeByType<StateMessageVector4>( 128 );
        factoryManager->setPoolSizeByType<StateMessageUIntValue>( 128 );
        factoryManager->setPoolSizeByType<StateMessageIntValue>( 128 );
        factoryManager->setPoolSizeByType<StateMessageVisible>( 128 );
        */
    }

    void UnitTests::destroyDefault()
    {
        WP_DEBUG_TRACE;

        auto currentThreadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( currentThreadId );

        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager )
        {
            auto applicationManagerPtr = applicationManager.get();

            applicationManager->unload( nullptr );
            std::cerr << "Application manager unloaded" << std::endl;
            core::IApplicationManager::setInstance( nullptr );
            std::cerr << "Application manager singleton cleared" << std::endl;

            m_plugin = nullptr;
            std::cerr << "Core plugin released" << std::endl;

            applicationManager = nullptr;
            if( applicationManagerPtr )
            {
                applicationManagerPtr->removeReference();
                std::cerr << "Application manager released" << std::endl;
                applicationManagerPtr = nullptr;
            }

            BOOST_CHECK( core::ApplicationManager::instance() == nullptr );
        }
    }

    void UnitTests::updateDefault( u32 iterations )
    {
        WP_DEBUG_TRACE;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fixture = getFixture();
        WP_ASSERT( fixture );

        auto timer = applicationManager->getTimer();
        WP_ASSERT( timer );

        auto startTime = timer->now();

        auto count = 0;
        while( count < (s32)iterations )
        {
            fixture->iterate();
            ++count;

            Thread::sleep( 0.01 );
        }

        auto endTime = timer->now();
        auto timeTaken = endTime - startTime;

        auto message = String( "Time taken: " ) + StringUtil::toString( timeTaken );
        BOOST_TEST_MESSAGE( message );
    }

    void UnitTests::createTasks()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto application = applicationManager->getApplication();
            auto factoryManager = applicationManager->getFactoryManager();
            auto taskManager = applicationManager->getTaskManager();
            auto profiler = applicationManager->getProfiler();

            if( auto primaryTask = taskManager->getTask( TaskId::Primary ) )
            {
                primaryTask->setTask( TaskId::Primary );
                primaryTask->setThreadTaskFlags( Thread::Primary_Flag );
                primaryTask->setPrimary( true );
                primaryTask->setEnabled( true );
                primaryTask->setOwner( application );
                primaryTask->setTargetFPS( 60.0 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Primary" );
                primaryTask->setProfile( profile );
            }

            if( auto applicationTask = taskManager->getTask( TaskId::Application ) )
            {
                applicationTask->setTask( TaskId::Application );
                applicationTask->setThreadTaskFlags( Thread::Application_Flag );
                applicationTask->setPrimary( false );
                applicationTask->setEnabled( true );
                applicationTask->setOwner( application );
                applicationTask->setTargetFPS( 60.0 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Application" );
                applicationTask->setProfile( profile );
            }

            if( auto renderTask = taskManager->getTask( TaskId::Render ) )
            {
                renderTask->setTask( TaskId::Render );
                renderTask->setThreadTaskFlags( Thread::Render_Flag );

#if WP_GRAPHICS_SYSTEM_OGRENEXT
#    ifdef WP_PLATFORM_WIN32
                //renderTask->setPrimary( false );
                renderTask->setPrimary( true );
#    else
                renderTask->setPrimary( true );
#    endif
#elif WP_GRAPHICS_SYSTEM_OGRE
                renderTask->setPrimary( true );
#endif

                renderTask->setEnabled( true );
                renderTask->setOwner( application );
                renderTask->setTargetFPS( 60.0 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Render" );
                renderTask->setProfile( profile );
            }

            if( auto physicsTask = taskManager->getTask( TaskId::Physics ) )
            {
                physicsTask->setTask( TaskId::Physics );
                physicsTask->setThreadTaskFlags( Thread::Physics_Flag );

                physicsTask->setPrimary( false );
                physicsTask->setEnabled( true );
                physicsTask->setOwner( application );
                physicsTask->setTargetFPS( 120.0 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Physics" );
                physicsTask->setProfile( profile );
            }

            if( auto garbageCollectTask = taskManager->getTask( TaskId::GarbageCollect ) )
            {
                garbageCollectTask->setTask( TaskId::GarbageCollect );
                garbageCollectTask->setThreadTaskFlags( Thread::GarbageCollect_Flag );

                garbageCollectTask->setPrimary( false );
                garbageCollectTask->setEnabled( true );
                garbageCollectTask->setOwner( application );
                garbageCollectTask->setTargetFPS( 30.0 );

                auto profile = profiler->addProfile();
                profile->setLabel( "Garbage Collect" );
                garbageCollectTask->setProfile( profile );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    String &UnitTests::getPluginsFilePath()
    {
        // Implemented as a function-local static. Holding the path in a
        // namespace-scope String member would invoke the static destruction
        // order fiasco: the path is initialised before main, but the pool it
        // borrows from is lazy-initialised on first use. The pool would be
        // destroyed first at program exit, leaving the path with a dangling
        // m_data pointer that corrupts the heap.
        static String s_path = String( "wp_plugins_tests.cfg" );
        return s_path;
    }

    void UnitTests::setPluginsFilePath( const String &pluginsFilePath )
    {
        // Mutate through getPluginsFilePath() so the path lives as long as the pool.
        getPluginsFilePath() = pluginsFilePath;
    }
    bool UnitTests::isHeadlessGraphicsMode()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return true;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            return true;
        }

        // A real graphics pipeline needs material and texture managers available.
        return graphicsSystem->getMaterialManager() == nullptr ||
               graphicsSystem->getTextureManager() == nullptr;
    }

    bool UnitTests::isHeadlessSoundMode()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return true;
        }

        auto soundManager = applicationManager->getSoundManager();
        if( !soundManager )
        {
            return true;
        }

        return !soundManager->isValid();
    }
}  // namespace workphone
