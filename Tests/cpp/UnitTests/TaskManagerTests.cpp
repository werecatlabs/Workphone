#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( taskmanager )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();

        auto count = 0;
        while( count < 3 )
        {
            taskManager->update();
            count++;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( taskmanager_fixed )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();

        auto count = 0;
        while( count < 3 )
        {
            taskManager->update();
            count++;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( taskmanager_minimal )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();

        auto count = 0;
        while( count < 3 )
        {
            taskManager->update();
            count++;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( taskmanager_standard )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();

        auto count = 0;
        while( count < 3 )
        {
            taskManager->update();
            count++;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( taskmanager_tbb )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();

        auto count = 0;
        while( count < 3 )
        {
            taskManager->update();
            count++;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( taskmanager_tasklock )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        BOOST_CHECK( taskManager );

        auto lock = taskManager->lockTask( TaskId::Render );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

class TaskManagerWorkerThread : public WorkerThread
{
public:
    void run() override
    {
        std::cout << "thread start run id: " << id_ << std::endl;

        WorkerThread::run();

        std::cout << "thread end run id: " << id_ << std::endl;
    }

    s32 id_ = -1;
};

BOOST_AUTO_TEST_CASE( taskmanager_worker_thread )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto threadPool = applicationManager->getThreadPool();
        auto taskManager = applicationManager->getTaskManager();

        Array<SmartPtr<IWorkerThread>> workerThreads;
        auto updateFrequency = 1.0 / 60.0;
        auto numThreads = 20;

        if( threadPool )
        {
            workerThreads.reserve( numThreads );
            for( s32 threadIdx = 0; threadIdx < numThreads; ++threadIdx )
            {
                auto workerThread = threadPool->addWorkerThread();
                //workerThread->id_ = threadIdx;
                workerThread->setTargetFPS( 300.0 );
                workerThreads.push_back( workerThread );
            }
        }

        auto renderTask = taskManager->getTask( TaskId::Render );
        if( renderTask )
        {
            renderTask->setTask( TaskId::Render );
            renderTask->setTargetFPS( 60.0 );
        }

        auto applicationTask = taskManager->getTask( TaskId::Application );
        if( applicationTask )
        {
            applicationTask->setTask( TaskId::Application );
            applicationTask->setTargetFPS( 30.0 );
        }

        auto physicsTask = taskManager->getTask( TaskId::Physics );
        if( physicsTask )
        {
            physicsTask->setTask( TaskId::Physics );
            physicsTask->setTargetFPS( 200.0 );
        }

        taskManager->setState( ITaskManager::State::FreeStep );

        bool running = true;
        while( running )
        {
            taskManager->update();
            Thread::sleep( 1.0 / 60.0 );
            running = false;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
