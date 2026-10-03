#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>

#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include "commands/DragDropActorCmd.hpp"
#include "commands/AddActorCmd.hpp"
#include <EditorApplication.hpp>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace editor;

BOOST_AUTO_TEST_CASE( ui_test )
{
    EditorApplication app;

    try
    {
        app.load( nullptr );
        //app.run();

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto timer = applicationManager->getTimer();
        auto endTime = timer->now() + 10.0;
        auto counter = 0;

        while( applicationManager->isRunning() )
        {
            try
            {
                if( taskManager )
                {
                    taskManager->update();
                }

                Thread::yield();

                if( timer->now() > endTime && counter++ > 1000 )
                {
                    break;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        SmartPtr<scene::Layout> canvas;
        if( auto actor = sceneManager->createActor() )
        {
            canvas = actor->addComponent<scene::Layout>();
        }
        BOOST_REQUIRE( canvas );

        endTime = timer->now() + 10.0;

        while( applicationManager->isRunning() )
        {
            try
            {
                if( taskManager )
                {
                    taskManager->update();
                }

                Thread::yield();

                if( timer->now() > endTime )
                {
                    break;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        auto window = canvas->getLayout();
        BOOST_REQUIRE( window );
        WeakPtr<ui::IUILayoutWindow> windowObserver( window );
        canvas->unload( nullptr );
        BOOST_CHECK( !canvas->getLayout() );
        window = nullptr;
        BOOST_CHECK( windowObserver.expired() );
        windowObserver = WeakPtr<ui::IUILayoutWindow>();
        canvas = nullptr;

        applicationManager->setQuit( true );
        applicationManager->setRunning( false );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }

    app.unload( nullptr );
}

#endif
