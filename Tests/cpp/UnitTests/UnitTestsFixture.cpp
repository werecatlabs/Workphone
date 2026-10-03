#include "UnitTestsFixture.hpp"
#include "UnitTests.hpp"
#include "Workphone/Workphone.hpp"
#include <boost/test/unit_test_log.hpp>

namespace workphone
{

    UnitTestsFixture::UnitTestsFixture()
    {
        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto threadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( threadId );

        UnitTests::setFixture( this );
        UnitTests::setupGame();
        createTasks();

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        if( taskManager )
        {
            taskManager->setState( ITaskManager::State::FreeStep );
        }

        auto threadPool = applicationManager->getThreadPool();
        if( threadPool )
        {
            threadPool->setState( IThreadPool::State::Start );
        }
    }

    UnitTestsFixture::~UnitTestsFixture()
    {
        unload( nullptr );
        UnitTests::setFixture( nullptr );
    }

    void UnitTestsFixture::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        auto taskManager = applicationManager->getTaskManager();
        auto sceneManager = applicationManager->getGameManager();
        auto fsmManager = applicationManager->getFsmManager();
        auto timer = applicationManager->getTimer();
        auto soundManager = applicationManager->getSoundManager();
        auto inputManager = applicationManager->getInputDeviceManager();
        auto cameraManager = applicationManager->getCameraManager();

        stateManager->preUpdate();

        timer->update();
        auto t = timer->getTime();
        auto dt = timer->getDeltaTime();

        if( sceneManager )
        {
            sceneManager->preUpdate();
        }

        if( inputManager )
        {
            inputManager->preUpdate();
        }

        if( soundManager )
        {
            soundManager->preUpdate();
        }

        if( cameraManager )
        {
            cameraManager->update();
        }

        stateManager->update();

        if( sceneManager )
        {
            sceneManager->update();
        }

        if( inputManager )
        {
            inputManager->update();
        }

        switch( auto task = Thread::getCurrentTask() )
        {
        case TaskId::Application:
        {
        }
        break;
        case TaskId::GarbageCollect:
        {
            try
            {
                auto timer = applicationManager->getTimer();

                auto dt = timer->getDeltaTime();
                auto t = timer->getTime();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
        break;
        case TaskId::Physics:
        {
            try
            {
                auto physicsManager = applicationManager->getPhysicsManager();

                if( timer->getTimeSinceSceneLoad() > 3.0 )
                {
                    if( physicsManager )
                    {
                        physicsManager->preUpdate();
                        physicsManager->update();
                        physicsManager->postUpdate();

                        auto physicsScene = physicsManager->getPhysicsScene();
                        if( physicsScene )
                        {
                            physicsScene->preUpdate();
                            physicsScene->update();
                            physicsScene->postUpdate();
                        }
                    }

                    if( auto vehicleManager = applicationManager->getVehicleManager() )
                    {
                        vehicleManager->preUpdate();
                        vehicleManager->update();
                        vehicleManager->postUpdate();
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
        break;
        case TaskId::Primary:
        {
            auto jobQueue = applicationManager->getJobQueue();
            if( jobQueue )
            {
                jobQueue->preUpdate();
                jobQueue->update();
                jobQueue->postUpdate();
            }

            if( applicationManager->getQuit() )
            {
                applicationManager->setRunning( false );
            }

            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                graphicsSystem->messagePump();
            }
        }
        break;
        case TaskId::Render:
        {
            try
            {
                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    graphicsSystem->update();
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
        break;
        case TaskId::Sound:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        if( sceneManager )
        {
            sceneManager->postUpdate();
        }

        if( inputManager )
        {
            inputManager->postUpdate();
        }

        stateManager->postUpdate();
    }

    void UnitTestsFixture::iterate()
    {
        WP_DEBUG_TRACE;

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto timer = applicationManager->getTimer();
        WP_ASSERT( timer );

        auto startTime = timer->now();

        taskManager->update();

        auto endTime = timer->now();
        auto timeTaken = endTime - startTime;

        auto message = String( "Time taken: " ) + StringUtil::toString( timeTaken );
        BOOST_TEST_MESSAGE( message );
    }

    void UnitTestsFixture::unload( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        applicationManager->setQuit( true );
        applicationManager->setRunning( false );

        if( auto gameManager = applicationManager->getGameManager() )
        {
            if( auto gameScene = gameManager->getCurrentScene() )
            {
                gameScene->clear();
            }
        }

        UnitTests::destroyDefault();
    }

    void UnitTestsFixture::createTasks()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto taskManager = applicationManager->getTaskManager();
            WP_ASSERT( taskManager );

            auto profiler = applicationManager->getProfiler();

            if( auto primaryTask = taskManager->getTask( TaskId::Primary ) )
            {
                primaryTask->setTask( TaskId::Primary );
                primaryTask->setThreadTaskFlags( Thread::Primary_Flag );
                primaryTask->setPrimary( true );
                primaryTask->setEnabled( true );
                primaryTask->setOwner( this );
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
                applicationTask->setOwner( this );
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
                renderTask->setOwner( this );
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
                physicsTask->setOwner( this );
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
                garbageCollectTask->setOwner( this );
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

}  // namespace workphone
