#include "TestGuard.hpp"
#include "UnitTests.hpp"
#include "Workphone/Workphone.hpp"
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>

namespace workphone
{

    TestGuard::TestGuard( bool requirePhysics ) : requiresPhysics( requirePhysics )
    {
        applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        factoryManager = applicationManager->getFactoryManager();
        fileSystem = applicationManager->getFileSystem();
        stateManager = applicationManager->getStateManager();
        resourceDatabase = applicationManager->getResourceDatabase();
        typeManager = TypeManager::instance();
        graphicsSystem = applicationManager->getGraphicsSystem();

        timer = applicationManager->getTimer();
        BOOST_REQUIRE( timer );

        physicsManager = applicationManager->getPhysicsManager();
        if( requirePhysics && !physicsManager )
        {
            BOOST_TEST_MESSAGE( "Physics manager is not available - skipping physics test" );
            isAvailable = false;
            return;
        }

        sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        taskManager = applicationManager->getTaskManager();
        BOOST_REQUIRE( taskManager );

        setupThread();

        auto currentTask = Thread::getCurrentTask();
        u32 taskFlags = std::numeric_limits<u32>::max();
        Thread::setTaskFlags( currentTask, taskFlags );
        isAvailable = true;
    }

    TestGuard::~TestGuard()
    {
        cleanup();

        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager )
        {
            applicationManager->clearAllEvents();
        }
    }

    void TestGuard::addCleanup( std::function<void()> cleanup )
    {
        cleanupActions.push_back( std::move( cleanup ) );
    }

    void TestGuard::trackFilesystemPath( const String &path )
    {
        addCleanup( [path]() {
            if( !path.empty() && std::filesystem::exists( path.c_str() ) )
            {
                std::filesystem::remove_all( path.c_str() );
            }
        } );
    }

    void TestGuard::trackPhysicsActor( SmartPtr<physics::IPhysicsScene3> scene,
                                       SmartPtr<physics::IPhysicsBody3> body )
    {
        addCleanup( [scene, body]() mutable {
            if( scene && body )
            {
                scene->removeActor( body );
            }
        } );
    }

    void TestGuard::setupThread()
    {
        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto threadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( threadId );
    }

    void TestGuard::resetTimer()
    {
        if( timer )
        {
            timer->reset();
            timer->setSceneLoadTime( 0.0 );
        }
    }

    void TestGuard::updateScene( u32 iterations /*= 10 */ )
    {
        for( u32 i = 0; i < iterations; ++i )
        {
            if( timer )
            {
                timer->update();
            }

            if( sceneManager )
            {
                sceneManager->preUpdate();
                sceneManager->update();
                sceneManager->postUpdate();
            }
        }
    }

    void TestGuard::updatePhysics( u32 iterations /*= 10 */ )
    {
        for( u32 i = 0; i < iterations; ++i )
        {
            timer->update();

            if( stateManager )
            {
                stateManager->preUpdate();
                stateManager->update();
                stateManager->postUpdate();
            }

            sceneManager->preUpdate();
            sceneManager->update();
            sceneManager->postUpdate();

            if( timer->getTimeSinceSceneLoad() > 0.1 )
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
            }
        }
    }

    void TestGuard::runUpdateCycle( int frameCount /*= 1 */ )
    {
        for( int i = 0; i < frameCount; ++i )
        {
            if( timer )
            {
                timer->update();
            }

            if( stateManager )
            {
                stateManager->preUpdate();
                stateManager->update();
                stateManager->postUpdate();
            }

            if( sceneManager )
            {
                sceneManager->preUpdate();
                sceneManager->update();
                sceneManager->postUpdate();
            }

            if( graphicsSystem )
            {
                graphicsSystem->messagePump();
                graphicsSystem->update();
            }
        }
    }

    SmartPtr<scene::IGameActor> TestGuard::createBasicActor( bool isStatic /*= false */ )
    {
        auto actor = sceneManager ? sceneManager->createActor() : nullptr;
        if( actor && isStatic )
        {
            actor->setStatic( true );
        }

        return actor;
    }

    void TestGuard::cleanup()
    {
        for( auto it = cleanupActions.rbegin(); it != cleanupActions.rend(); ++it )
        {
            ( *it )();
        }
        cleanupActions.clear();

        if( scene )
        {
            scene->clear( true );
        }

        if( sceneManager )
        {
            sceneManager->clear();
        }

        if( graphicsSystem )
        {
            graphicsSystem->clearGraphicScenes();
        }
    }

}  // namespace workphone
