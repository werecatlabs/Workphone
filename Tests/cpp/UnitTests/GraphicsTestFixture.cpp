#include "GraphicsTestFixture.hpp"
#include "UnitTests.hpp"
#include "Workphone/Workphone.hpp"

using namespace workphone;

GraphicsTestFixture::GraphicsTestFixture()
{
    //Thread::sleep( 3.0 );
    //UnitTests::setupGraphics();
}

GraphicsTestFixture::~GraphicsTestFixture()
{
    /*    UnitTests::destroyDefault();

        Thread::sleep( 3.0 );
        */
}

void GraphicsTestFixture::createTasks()
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
