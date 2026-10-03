#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

//#include "FBPlugin/PluginInterface.hpp"

//DECLARE_FUNCTION_ARG1( load, void, workphone::s32 * );
//DECLARE_FUNCTION( isValid, bool );
//DECLARE_FUNCTION_ARG2( postPluginEvent, void, workphone::s32 *, workphone::s32 * );
//DECLARE_FUNCTION_ARG2( step, bool, bool, double );
//DECLARE_FUNCTION_ARG1( setPlaying, void, bool );
//DECLARE_FUNCTION( getCurrentState, workphone::StateData* );

#if 0

BOOST_AUTO_TEST_CASE( unity_plugin_load )
{
    using namespace fb;
    using namespace workphone::core;

    Vector3<f32> v;
    v.x = 0;

    auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
    BOOST_CHECK( applicationManager );

    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );
    core::ApplicationManager::setInstance( applicationManager );
    BOOST_CHECK( core::ApplicationManager::instance() );

    try
    {
        auto factoryManager = workphone::make_ptr<FactoryManager>();
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto logManager = workphone::make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        BOOST_CHECK( applicationManager->isValid() );
        WP_ASSERT( applicationManager->isValid() );

        auto pluginManager = workphone::make_ptr<PluginManager>();
        pluginManager->load( nullptr );
        BOOST_CHECK( pluginManager->getLoadingState() == LoadingState::Loaded );

        auto plugin = pluginManager->loadPlugin( "PluginUnity.dll" );
        BOOST_CHECK( plugin->getLoadingState() != LoadingState::Loaded );
        plugin->load( nullptr );

        auto loadingState = plugin->getLoadingState();
        BOOST_CHECK( loadingState == LoadingState::Loaded );

        auto isValidFunction = (isValid)plugin->getFunction( "isValid" );
        BOOST_CHECK( isValidFunction != nullptr );

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }

        auto loadFunction = (load)plugin->getFunction( "load" );
        BOOST_CHECK( loadFunction != nullptr );

        if( loadFunction )
        {
            loadFunction( nullptr );
        }

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    applicationManager->unload( nullptr );
    core::ApplicationManager::setInstance( nullptr );
    applicationManager = nullptr;
    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );

#    if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#    endif
}

BOOST_AUTO_TEST_CASE( unity_plugin_null_event )
{
    using namespace fb;
    using namespace workphone::core;

    auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
    BOOST_CHECK( applicationManager );

    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );
    core::ApplicationManager::setInstance( applicationManager );
    BOOST_CHECK( core::ApplicationManager::instance() );

    try
    {
        auto factoryManager = workphone::make_ptr<FactoryManager>();
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto logManager = workphone::make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        BOOST_CHECK( applicationManager->isValid() );
        WP_ASSERT( applicationManager->isValid() );

        auto pluginManager = workphone::make_ptr<PluginManager>();
        pluginManager->load( nullptr );
        BOOST_CHECK( pluginManager->getLoadingState() == LoadingState::Loaded );

        auto plugin = pluginManager->loadPlugin( "PluginUnity.dll" );
        BOOST_CHECK( plugin->getLoadingState() != LoadingState::Loaded );
        plugin->load( nullptr );

        auto loadingState = plugin->getLoadingState();
        BOOST_CHECK( loadingState == LoadingState::Loaded );

        auto isValidFunction = (isValid)plugin->getFunction( "isValid" );
        BOOST_CHECK( isValidFunction != nullptr );

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }

        auto loadFunction = (load)plugin->getFunction( "load" );
        BOOST_CHECK( loadFunction != nullptr );

        if( loadFunction )
        {
            loadFunction( nullptr );
        }

        auto postPluginEventFunction = (postPluginEvent)plugin->getFunction( "postPluginEvent" );
        BOOST_CHECK( postPluginEventFunction != nullptr );

        if( postPluginEventFunction )
        {
            postPluginEventFunction( nullptr, nullptr );
        }

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    applicationManager->unload( nullptr );
    core::ApplicationManager::setInstance( nullptr );
    applicationManager = nullptr;
    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );

#    if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#    endif
}

BOOST_AUTO_TEST_CASE( unity_plugin_create_model )
{
    using namespace fb;
    using namespace workphone::core;

    auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
    BOOST_CHECK( applicationManager );

    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );
    core::ApplicationManager::setInstance( applicationManager );
    BOOST_CHECK( core::ApplicationManager::instance() );

    try
    {
        auto factoryManager = workphone::make_ptr<FactoryManager>();
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto logManager = workphone::make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        BOOST_CHECK( applicationManager->isValid() );
        WP_ASSERT( applicationManager->isValid() );

        auto pluginManager = workphone::make_ptr<PluginManager>();
        pluginManager->load( nullptr );
        BOOST_CHECK( pluginManager->getLoadingState() == LoadingState::Loaded );

        auto plugin = pluginManager->loadPlugin( "PluginUnity.dll" );
        BOOST_CHECK( plugin->getLoadingState() != LoadingState::Loaded );
        plugin->load( nullptr );

        auto loadingState = plugin->getLoadingState();
        BOOST_CHECK( loadingState == LoadingState::Loaded );

        auto isValidFunction = (isValid)plugin->getFunction( "isValid" );
        BOOST_CHECK( isValidFunction != nullptr );

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }

        auto loadFunction = (load)plugin->getFunction( "load" );
        BOOST_CHECK( loadFunction != nullptr );

        if( loadFunction )
        {
            loadFunction( nullptr );
        }

        auto setPlayingFunction = (setPlaying)plugin->getFunction( "setPlaying" );
        BOOST_CHECK( setPlayingFunction != nullptr );

        if( setPlayingFunction )
        {
            setPlayingFunction( true );
        }

        auto postPluginEventFunction = (postPluginEvent)plugin->getFunction( "postPluginEvent" );
        BOOST_CHECK( postPluginEventFunction != nullptr );

        // if( postPluginEventFunction )
        //{
        //     auto eventName = String( "createModel" );
        //     auto eventPtr = (s32 *)eventName.c_str();
        //     postPluginEventFunction( eventPtr, nullptr );
        // }

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    applicationManager->unload( nullptr );
    core::ApplicationManager::setInstance( nullptr );
    applicationManager = nullptr;
    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );

#    if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#    endif
}

BOOST_AUTO_TEST_CASE( unity_plugin_scene_load )
{
    using namespace fb;
    using namespace workphone::core;

    auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
    BOOST_CHECK( applicationManager );

    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );
    core::ApplicationManager::setInstance( applicationManager );
    BOOST_CHECK( core::ApplicationManager::instance() );

    try
    {
        auto factoryManager = workphone::make_ptr<FactoryManager>();
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto logManager = workphone::make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        BOOST_CHECK( applicationManager->isValid() );
        WP_ASSERT( applicationManager->isValid() );

        auto pluginManager = workphone::make_ptr<PluginManager>();
        pluginManager->load( nullptr );
        BOOST_CHECK( pluginManager->getLoadingState() == LoadingState::Loaded );

        auto plugin = pluginManager->loadPlugin( "PluginUnity.dll" );
        BOOST_CHECK( plugin->getLoadingState() != LoadingState::Loaded );
        plugin->load( nullptr );

        auto loadingState = plugin->getLoadingState();
        BOOST_CHECK( loadingState == LoadingState::Loaded );

        auto isValidFunction = (isValid)plugin->getFunction( "isValid" );
        BOOST_CHECK( isValidFunction != nullptr );

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }

        auto setPlayingFunction = (setPlaying)plugin->getFunction( "setPlaying" );
        BOOST_CHECK( setPlayingFunction != nullptr );

        if( setPlayingFunction )
        {
            setPlayingFunction( true );
        }

        auto loadFunction = (load)plugin->getFunction( "load" );
        BOOST_CHECK( loadFunction != nullptr );

        if( loadFunction )
        {
            loadFunction( nullptr );
        }

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    applicationManager->unload( nullptr );
    core::ApplicationManager::setInstance( nullptr );
    applicationManager = nullptr;
    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );

#    if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#    endif
}

BOOST_AUTO_TEST_CASE( unity_plugin_goToWorkbench )
{
    using namespace fb;
    using namespace workphone::core;

    auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
    BOOST_CHECK( applicationManager );

    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );
    core::ApplicationManager::setInstance( applicationManager );
    BOOST_CHECK( core::ApplicationManager::instance() );

    try
    {
        auto factoryManager = workphone::make_ptr<FactoryManager>();
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto logManager = workphone::make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        BOOST_CHECK( applicationManager->isValid() );
        WP_ASSERT( applicationManager->isValid() );

        auto pluginManager = workphone::make_ptr<PluginManager>();
        pluginManager->load( nullptr );
        BOOST_CHECK( pluginManager->getLoadingState() == LoadingState::Loaded );

        auto workingDirectory = PathW::getWorkingDirectory();
        auto pluginPath = StringW( L"PluginUnity.dll" );
        auto pluginPathUTF8 = StringUtil::toUTF16to8( pluginPath );

        auto plugin = pluginManager->loadPlugin( pluginPathUTF8 );
        BOOST_CHECK( plugin->getLoadingState() != LoadingState::Loaded );
        plugin->setFilePath( pluginPath );
        plugin->load( nullptr );

        auto loadingState = plugin->getLoadingState();
        BOOST_CHECK( loadingState == LoadingState::Loaded );

        auto isValidFunction = (isValid)plugin->getFunction( "isValid" );
        BOOST_CHECK( isValidFunction != nullptr );

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }

        auto setPlayingFunction = (setPlaying)plugin->getFunction( "setPlaying" );
        BOOST_CHECK( setPlayingFunction != nullptr );

        if( setPlayingFunction )
        {
            setPlayingFunction( true );
        }

        auto loadFunction = (load)plugin->getFunction( "load" );
        BOOST_CHECK( loadFunction != nullptr );

        if( loadFunction )
        {
            loadFunction( nullptr );
        }

        /*
        auto postPluginEventFunction = (postPluginEvent)plugin->getFunction( "postPluginEvent" );
        BOOST_CHECK( postPluginEventFunction != nullptr );

        if( postPluginEventFunction )
        {
            auto eventName = String( "goToWorkbench" );
            auto eventPtr = (s32 *)eventName.c_str();
            postPluginEventFunction( eventPtr, nullptr );
        }

        auto stepFunction = (step)plugin->getFunction( "step" );
        BOOST_CHECK( stepFunction != nullptr );

        auto getCurrentStateFunction = (getCurrentState)plugin->getFunction( "getCurrentState" );
        BOOST_CHECK( getCurrentStateFunction != nullptr );

        if( stepFunction )
        {
            auto count = 0;
            while( count++ < 60 )
            {
                auto result = stepFunction( false, 1.0 / 60.0 );

                auto state = getCurrentStateFunction();
                if (state->appState == ApplicationTypes::ApplicationState::WP_STATE_WORK_BENCH)
                {
                   break; 
                }                
            }
        }
        */

        //auto state = getCurrentStateFunction();
        //BOOST_CHECK((state->appState == ApplicationTypes::ApplicationState::WP_STATE_WORK_BENCH));

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    applicationManager->unload( nullptr );
    core::ApplicationManager::setInstance( nullptr );
    applicationManager = nullptr;
    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );

#    if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#    endif
}

BOOST_AUTO_TEST_CASE( unity_plugin_states )
{
    using namespace fb;
    using namespace workphone::core;

    auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
    BOOST_CHECK( applicationManager );

    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );
    core::ApplicationManager::setInstance( applicationManager );
    BOOST_CHECK( core::ApplicationManager::instance() );

    try
    {
        auto factoryManager = workphone::make_ptr<FactoryManager>();
        applicationManager->setFactoryManager( factoryManager );
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto logManager = workphone::make_ptr<LogManagerDefault>();
        applicationManager->setLogManager( logManager );
        logManager->open( "UnitTests.log" );

        BOOST_CHECK( applicationManager->isValid() );
        WP_ASSERT( applicationManager->isValid() );

        auto pluginManager = workphone::make_ptr<PluginManager>();
        pluginManager->load( nullptr );
        BOOST_CHECK( pluginManager->getLoadingState() == LoadingState::Loaded );

        auto workingDirectory = PathW::getWorkingDirectory();
        auto pluginPath = StringW( L"PluginUnity.dll" );
        auto pluginPathUTF8 = StringUtil::toUTF16to8( pluginPath );

        auto plugin = pluginManager->loadPlugin( pluginPathUTF8 );
        BOOST_CHECK( plugin->getLoadingState() != LoadingState::Loaded );
        plugin->setFilePath( pluginPath );
        plugin->load( nullptr );

        auto loadingState = plugin->getLoadingState();
        BOOST_CHECK( loadingState == LoadingState::Loaded );

        auto isValidFunction = (isValid)plugin->getFunction( "isValid" );
        BOOST_CHECK( isValidFunction != nullptr );

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }

        auto setPlayingFunction = (setPlaying)plugin->getFunction( "setPlaying" );
        BOOST_CHECK( setPlayingFunction != nullptr );

        if( setPlayingFunction )
        {
            setPlayingFunction( true );
        }

        auto loadFunction = (load)plugin->getFunction( "load" );
        BOOST_CHECK( loadFunction != nullptr );

        if( loadFunction )
        {
            loadFunction( nullptr );
        }

        /*
        auto postPluginEventFunction = (postPluginEvent)plugin->getFunction( "postPluginEvent" );
        BOOST_CHECK( postPluginEventFunction != nullptr );

        if( postPluginEventFunction )
        {
            auto eventName = String( "goToWorkbench" );
            auto eventPtr = (s32 *)eventName.c_str();
            postPluginEventFunction( eventPtr, nullptr );
        }

        auto states = Array<ApplicationTypes::ApplicationState>();

        auto stepFunction = (step)plugin->getFunction( "step" );
        BOOST_CHECK( stepFunction != nullptr );

        if( stepFunction )
        {
            auto count = 0;
            while( count++ < 60 )
            {
                auto result = stepFunction( false, 1.0 / 60.0 );
            }
        }
        */

        if( isValidFunction )
        {
            auto valid = isValidFunction();
            BOOST_CHECK( valid );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    applicationManager->unload( nullptr );
    core::ApplicationManager::setInstance( nullptr );
    applicationManager = nullptr;
    BOOST_CHECK( core::ApplicationManager::instance() == nullptr );

#    if WP_ENABLE_MEMORY_TRACKER
    auto &memoryTracker = MemoryTracker::get();
    memoryTracker.reportLeaks();
#    endif
}

#endif
