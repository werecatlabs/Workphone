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

BOOST_AUTO_TEST_CASE( actor_component_test )
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
                auto factories = factoryManager->getFactories();
                for( auto factory : factories )
                {
                    //if (factory->isObjectDerivedFrom<scene::IComponent>())
                    if( factory->isObjectDerivedFrom<scene::Mesh>() )
                    {
                        auto component = factory->make_ptr<scene::Mesh>();
                        actor->addComponentInstance( component );

                        auto timer = applicationManager->getTimer();

                        auto start = timer->now();
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
                                if( now > start + 3.0 )
                                {
                                    break;
                                }
                            }
                            catch( std::exception &e )
                            {
                                WP_LOG_EXCEPTION( e );
                            }
                        }

                        actor->removeComponentInstance( component );
                        component = nullptr;
                    }
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

BOOST_AUTO_TEST_CASE( actor_test )
{
    try
    {
        EditorApplication app;
        app.load( nullptr );
        //app.run();

        auto isPlayMode = false;

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            BOOST_CHECK( applicationManager );

            auto taskManager = applicationManager->getTaskManager();
            BOOST_CHECK( taskManager );

            auto timer = applicationManager->getTimer();
            auto endTime = timer->now() + 10.0;

            Vector2I position;
            SmartPtr<ui::IUIElement> src;
            SmartPtr<ui::IUIElement> dst;
            String text;

            auto cmd = workphone::make_ptr<DragDropActorCmd>();
            cmd->setPosition( position );
            cmd->setSrc( nullptr );
            cmd->setDst( nullptr );
            cmd->setData( text );

            while( applicationManager->isRunning() )
            {
                try
                {
                    if( taskManager )
                    {
                        taskManager->update();
                    }

                    auto editorManager = EditorManager::getSingletonPtr();
                    BOOST_CHECK( editorManager );

                    if( editorManager )
                    {
                        auto ui = editorManager->getUI();
                        BOOST_CHECK( ui );

                        if( ui )
                        {
                            BOOST_CHECK( ui->isValid() );
                        }
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

            auto commandManager = applicationManager->getCommandManager();
            //commandManager->addCommand( cmd );

            while( applicationManager->isRunning() )
            {
                try
                {
                    if( taskManager )
                    {
                        taskManager->update();
                    }

                    auto editorManager = EditorManager::getSingletonPtr();
                    BOOST_CHECK( editorManager );

                    if( editorManager )
                    {
                        auto ui = editorManager->getUI();
                        BOOST_CHECK( ui );

                        if( ui )
                        {
                            BOOST_CHECK( ui->isValid() );
                        }
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

BOOST_AUTO_TEST_CASE( actor_dragdrop_test )
{
    try
    {
        EditorApplication app;
        app.load( nullptr );
        //app.run();

        auto isPlayMode = false;

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            BOOST_CHECK( applicationManager );

            auto taskManager = applicationManager->getTaskManager();
            BOOST_CHECK( taskManager );

            auto timer = applicationManager->getTimer();
            auto endTime = timer->now() + 10.0;

            Vector2I position;
            SmartPtr<ui::IUIElement> src;
            SmartPtr<ui::IUIElement> dst;
            String text;

            auto cmd = workphone::make_ptr<DragDropActorCmd>();
            cmd->setPosition( position );
            cmd->setSrc( nullptr );
            cmd->setDst( nullptr );
            cmd->setData( text );

            while( applicationManager->isRunning() )
            {
                try
                {
                    if( taskManager )
                    {
                        taskManager->update();
                    }

                    auto editorManager = EditorManager::getSingletonPtr();
                    BOOST_CHECK( editorManager );

                    if( editorManager )
                    {
                        auto ui = editorManager->getUI();
                        BOOST_CHECK( ui );

                        if( ui )
                        {
                            BOOST_CHECK( ui->isValid() );
                        }
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

            //auto commandManager = applicationManager->getCommandManager();
            //commandManager->addCommand( cmd );

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

BOOST_AUTO_TEST_CASE( actor_prefab_test )
{
    try
    {
        EditorApplication app;
        app.load( nullptr );
        //app.run();

        auto isPlayMode = false;

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            BOOST_CHECK( applicationManager );

            auto taskManager = applicationManager->getTaskManager();
            BOOST_CHECK( taskManager );

            try
            {
                auto timer = applicationManager->getTimer();
                auto endTime = timer->now() + 10.0;

                auto commandManager = applicationManager->getCommandManager();

                Vector2I position;
                SmartPtr<ui::IUIElement> src;
                SmartPtr<ui::IUIElement> dst;
                String text;

                /*
                auto filePath = String( "F40.fbx" );
                auto cmd = fb::make_ptr<AddActorCmd>();
                cmd->setActorType( AddActorCmd::ActorType::Actor );
                cmd->setFilePath( filePath );
                */

                while( applicationManager->isRunning() )
                {
                    try
                    {
                        if( taskManager )
                        {
                            taskManager->update();
                        }

                        /*
                        auto editorManager = EditorManager::getSingletonPtr();
                        BOOST_CHECK( editorManager );

                        if( editorManager )
                        {
                            auto ui = editorManager->getUI();
                            BOOST_CHECK( ui );

                            if( ui )
                            {
                                BOOST_CHECK( ui->isValid() );
                            }
                        }
                         */

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

                //commandManager->addCommand( cmd );

                while( applicationManager->isRunning() )
                {
                    try
                    {
                        if( taskManager )
                        {
                            taskManager->update();
                        }

                        auto editorManager = EditorManager::getSingletonPtr();
                        BOOST_CHECK( editorManager );

                        if( editorManager )
                        {
                            auto ui = editorManager->getUI();
                            BOOST_CHECK( ui );

                            if( ui )
                            {
                                BOOST_CHECK( ui->isValid() );
                            }
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
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
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

BOOST_AUTO_TEST_CASE( actor_addremove_test )
{
    try
    {
        EditorApplication app;
        app.load( nullptr );
        //app.run();

        auto isPlayMode = false;

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            BOOST_CHECK( applicationManager );

            auto taskManager = applicationManager->getTaskManager();
            BOOST_CHECK( taskManager );

            auto timer = applicationManager->getTimer();
            auto endTime = timer->now() + 10.0;

            Vector2I position;
            SmartPtr<ui::IUIElement> src;
            SmartPtr<ui::IUIElement> dst;
            String text;

            auto cmd = workphone::make_ptr<DragDropActorCmd>();
            cmd->setPosition( position );
            cmd->setSrc( nullptr );
            cmd->setDst( nullptr );
            cmd->setData( text );

            while( applicationManager->isRunning() )
            {
                try
                {
                    if( taskManager )
                    {
                        taskManager->update();
                    }

                    auto editorManager = EditorManager::getSingletonPtr();
                    BOOST_CHECK( editorManager );

                    if( editorManager )
                    {
                        auto ui = editorManager->getUI();
                        BOOST_CHECK( ui );

                        if( ui )
                        {
                            BOOST_CHECK( ui->isValid() );
                        }
                    }

                    Thread::yield();

                    auto timer = applicationManager->getTimer();
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

            //auto commandManager = applicationManager->getCommandManager();
            //commandManager->addCommand( cmd );

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
