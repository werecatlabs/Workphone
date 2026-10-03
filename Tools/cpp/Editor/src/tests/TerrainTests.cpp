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

BOOST_AUTO_TEST_CASE( terrain_test )
{
    try
    {
        EditorApplication app;
        app.load( nullptr );
        //app.run();

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto taskManager = applicationManager->getTaskManager();
            WP_ASSERT( taskManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

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

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );

            if( auto actor = sceneManager->createActor() )
            {
                auto terrain = actor->addComponent<scene::TerrainSystem>();
                BOOST_CHECK( terrain );
            }

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

            applicationManager->setQuit( true );
            applicationManager->setRunning( false );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        app.unload( nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

#endif
