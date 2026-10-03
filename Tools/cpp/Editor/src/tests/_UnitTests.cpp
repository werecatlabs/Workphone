#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>
#include <EditorApplication.hpp>

#if WP_EDITOR_TESTS

#    define BOOST_TEST_ALTERNATIVE_INIT_API
#    define BOOST_TEST_NO_MAIN
//#define BOOST_TEST_MAIN
#    define BOOST_TEST_MODULE WP_Editor_Tests
#    include <boost/test/included/unit_test.hpp>

using namespace workphone;

#    if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
BOOST_AUTO_TEST_CASE( editor_cross_module_string_pool )
{
    const auto before = String::character_pool_stats().liveBlocks;
    {
        // This exported function allocates its returned string inside Workphone.
        // The editor must observe and release the same pool allocation.
        auto uuid = StringUtil::getUUID();
        BOOST_CHECK_EQUAL( uuid.size(), 36u );
        BOOST_CHECK_EQUAL( String::character_pool_stats().liveBlocks, before + 1 );
    }
    BOOST_CHECK_EQUAL( String::character_pool_stats().liveBlocks, before );
}
#    endif

BOOST_AUTO_TEST_CASE( expired_pooled_weak_references )
{
    // Factories destroy objects before releasing their pool storage. Exercise
    // every teardown operation while that expired storage remains addressable.
    alignas( ISharedObject ) unsigned char storage[sizeof( ISharedObject )];
    auto object = new( storage ) ISharedObject;
    WeakPtr<ISharedObject> resetPointer( object );
    WeakPtr<ISharedObject> assignedPointer( object );
    AtomicWeakPtr<ISharedObject> loadedPointer( resetPointer );
    AtomicWeakPtr<ISharedObject> storedPointer( resetPointer );
    AtomicWeakPtr<ISharedObject> exchangedPointer( resetPointer );
    AtomicWeakPtr<ISharedObject> destroyedPointer( resetPointer );

    object->~ISharedObject();
    BOOST_CHECK( resetPointer.expired() );
    resetPointer = nullptr;
    assignedPointer = WeakPtr<ISharedObject>();
    BOOST_CHECK( !loadedPointer.load().lock() );
    storedPointer.store( WeakPtr<ISharedObject>() );
    BOOST_CHECK( !storedPointer.get() );
    BOOST_CHECK( !exchangedPointer.exchange( WeakPtr<ISharedObject>() ).lock() );
    BOOST_CHECK( !exchangedPointer.get() );
}

BOOST_AUTO_TEST_CASE( editor_repeated_lifecycle )
{
    for( int cycle = 0; cycle < 3; ++cycle )
    {
        BOOST_TEST_CONTEXT( "editor lifecycle " << cycle )
        {
            editor::EditorApplication app;
            app.setDebugMode( true );
            app.setActiveThreads( 0 );
            app.load( nullptr );
            BOOST_REQUIRE( app.isLoaded() );
            auto manager = core::IApplicationManager::instance();
            BOOST_REQUIRE( manager );
            auto tasks = manager->getTaskManager();
            BOOST_REQUIRE( tasks );
            for( int tick = 0; tick < 3; ++tick )
                tasks->update();
            BOOST_CHECK( manager->isRunning() );
            BOOST_CHECK( !manager->getQuit() );
            manager->setQuit( true );
            manager->setRunning( false );
            tasks = nullptr;
            manager = nullptr;
            app.unload( nullptr );
            BOOST_CHECK( !core::IApplicationManager::instance() );
#    if defined( _WIN32 ) && WP_GRAPHICS_SYSTEM_CLAW
            WNDCLASSW windowClass = {};
            BOOST_CHECK( !GetClassInfoW( GetModuleHandleW( nullptr ), L"WorkphoneWindowClass",
                                        &windowClass ) );
#    endif
        }
    }
}

BOOST_AUTO_TEST_CASE( editor_start )
{
    auto typeManager = TypeManager::instance();
    if( !typeManager )
    {
        typeManager = new TypeManager;
        TypeManager::setInstance( typeManager );
    }

    editor::EditorApplication app;
    app.load( nullptr );
    //app.run();

    auto applicationManager = core::IApplicationManager::instance();
    applicationManager->setQuit( true );
    applicationManager->setRunning( false );

    app.unload( nullptr );
}

BOOST_AUTO_TEST_CASE( editor_run )
{
    try
    {
        editor::EditorApplication app;

        const auto threads = Thread::hardware_concurrency();
        app.setActiveThreads( threads );

        app.load( nullptr );
        //app.run();

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto timer = applicationManager->getTimer();
        auto endTime = timer->now() + 15.0;

        while( applicationManager->isRunning() )
        {
            try
            {
                if( taskManager )
                {
                    taskManager->update();
                }

                Thread::yield();

                auto now = timer->now();
                if( now > endTime )
                {
                    break;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        applicationManager->setQuit( true );
        applicationManager->setRunning( false );

        app.unload( nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( editor_run_single_threaded )
{
    try
    {
        editor::EditorApplication app;
        app.setActiveThreads( 0 );
        app.load( nullptr );
        //app.run();

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto threadPool = applicationManager->getThreadPool();
        WP_ASSERT( threadPool );

        BOOST_CHECK( threadPool->getNumThreads() == 0 );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto timer = applicationManager->getTimer();
        auto endTime = timer->now() + 10.0;

        while( applicationManager->isRunning() )
        {
            try
            {
                if( taskManager )
                {
                    taskManager->update();
                }

                Thread::yield();

                auto now = timer->now();
                if( now > endTime )
                {
                    break;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        applicationManager->setQuit( true );
        applicationManager->setRunning( false );

        app.unload( nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

#endif
