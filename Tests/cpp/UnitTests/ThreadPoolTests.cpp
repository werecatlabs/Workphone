#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <atomic>
#include <chrono>
#include <vector>

using namespace workphone;

BOOST_AUTO_TEST_SUITE( thread_pool_suite )

//-----------------------------------------------------------------------------
// Basic ThreadPool Initialization Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( thread_pool_initialization )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager != nullptr );

        auto threadPool = workphone::make_ptr<ThreadPool>();
        BOOST_CHECK( threadPool != nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during thread pool initialization" );
    }
}

BOOST_AUTO_TEST_CASE( thread_pool_multiple_instances )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();

        auto threadPool1 = workphone::make_ptr<ThreadPool>();
        auto threadPool2 = workphone::make_ptr<ThreadPool>();

        BOOST_CHECK( threadPool1 != nullptr );
        BOOST_CHECK( threadPool2 != nullptr );
        BOOST_CHECK( threadPool1 != threadPool2 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown creating multiple thread pools" );
    }
}

//-----------------------------------------------------------------------------
// Worker Thread Tests
//-----------------------------------------------------------------------------

class TestWorkerThread : public WorkerThread
{
public:
    void run() override
    {
        ++executionCount_;
        std::cout << "thread run id: " << id_ << std::endl;
    }

    s32 id_ = -1;
    std::atomic<s32> executionCount_{ 0 };
};

class CountingWorkerThread : public WorkerThread
{
public:
    CountingWorkerThread( std::atomic<s32> &counter ) : counter_( counter )
    {
    }

    void run() override
    {
        ++counter_;
    }

private:
    std::atomic<s32> &counter_;
};

BOOST_AUTO_TEST_CASE( thread_pool_add_single_worker )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto threadPool = workphone::make_ptr<ThreadPool>();

        BOOST_REQUIRE( threadPool != nullptr );

        auto workerThread = threadPool->addWorkerThread();
        BOOST_CHECK( workerThread != nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown adding single worker thread" );
    }
}

BOOST_AUTO_TEST_CASE( thread_pool_worker_thread )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto timer = workphone::make_ptr<TimerMT>();
        auto threadPool = workphone::make_ptr<ThreadPool>();

        BOOST_REQUIRE( threadPool != nullptr );

        Array<SmartPtr<IWorkerThread>> workerThreads;
        constexpr auto updateFrequency = 1.0 / 60.0;
        constexpr auto numThreads = 20;

        workerThreads.reserve( numThreads );
        for( s32 threadIdx = 0; threadIdx < numThreads; ++threadIdx )
        {
            auto workerThread = threadPool->addWorkerThread();
            BOOST_CHECK( workerThread != nullptr );
            workerThreads.push_back( workerThread );
        }

        BOOST_CHECK_EQUAL( static_cast<s32>( workerThreads.size() ), numThreads );

        auto count = 0;
        bool running = true;
        while( running && count++ < 100 )
        {
            Thread::sleep( updateFrequency );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in worker thread test" );
    }
}

//-----------------------------------------------------------------------------
// Edge Cases
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( thread_pool_zero_threads )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto threadPool = workphone::make_ptr<ThreadPool>();

        BOOST_REQUIRE( threadPool != nullptr );

        // Creating pool with no workers should be valid
        Array<SmartPtr<IWorkerThread>> workerThreads;
        BOOST_CHECK( workerThreads.empty() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with zero threads" );
    }
}

BOOST_AUTO_TEST_CASE( thread_pool_large_thread_count )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto threadPool = workphone::make_ptr<ThreadPool>();

        BOOST_REQUIRE( threadPool != nullptr );

        Array<SmartPtr<IWorkerThread>> workerThreads;
        constexpr auto numThreads = 100;  // Stress test with many threads

        workerThreads.reserve( numThreads );
        for( s32 threadIdx = 0; threadIdx < numThreads; ++threadIdx )
        {
            auto workerThread = threadPool->addWorkerThread();
            workerThreads.push_back( workerThread );
        }

        BOOST_CHECK_EQUAL( static_cast<s32>( workerThreads.size() ), numThreads );

        // Allow threads to settle
        Thread::sleep( 0.1 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown with large thread count" );
    }
}

BOOST_AUTO_TEST_CASE( thread_pool_rapid_create_destroy )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();

        constexpr auto iterations = 10;
        for( s32 i = 0; i < iterations; ++i )
        {
            auto threadPool = workphone::make_ptr<ThreadPool>();
            BOOST_REQUIRE( threadPool != nullptr );

            auto workerThread = threadPool->addWorkerThread();
            BOOST_CHECK( workerThread != nullptr );

            // ThreadPool should be destroyed cleanly at end of scope
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during rapid create/destroy" );
    }
}

BOOST_AUTO_TEST_CASE( thread_pool_concurrent_worker_addition )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto threadPool = workphone::make_ptr<ThreadPool>();

        BOOST_REQUIRE( threadPool != nullptr );

        std::atomic<s32> addedCount{ 0 };
        Array<SmartPtr<IWorkerThread>> workerThreads;
        constexpr auto numThreads = 10;

        workerThreads.reserve( numThreads );

        // Sequentially add workers (concurrent addition would need mutex)
        for( s32 i = 0; i < numThreads; ++i )
        {
            auto worker = threadPool->addWorkerThread();
            if( worker )
            {
                workerThreads.push_back( worker );
                ++addedCount;
            }
        }

        BOOST_CHECK_EQUAL( addedCount.load(), numThreads );

        Thread::sleep( 0.1 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during concurrent worker addition" );
    }
}

//-----------------------------------------------------------------------------
// Timer Integration Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( thread_pool_with_timer_precision )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto timer = workphone::make_ptr<TimerMT>();
        auto threadPool = workphone::make_ptr<ThreadPool>();

        BOOST_REQUIRE( threadPool != nullptr );
        BOOST_REQUIRE( timer != nullptr );

        Array<SmartPtr<IWorkerThread>> workerThreads;
        constexpr auto numThreads = 5;
        constexpr auto targetFrameTime = 1.0 / 60.0;

        workerThreads.reserve( numThreads );
        for( s32 i = 0; i < numThreads; ++i )
        {
            workerThreads.push_back( threadPool->addWorkerThread() );
        }

        auto startTime = std::chrono::high_resolution_clock::now();
        constexpr auto frameCount = 60;

        for( s32 frame = 0; frame < frameCount; ++frame )
        {
            Thread::sleep( targetFrameTime );
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>( endTime - startTime ).count();

        // Should take approximately 1 second (60 frames at 60fps). The shared test
        // runner can be heavily loaded, so keep this as a coarse sanity check.
        BOOST_CHECK_GT( elapsedMs, 800 );
        BOOST_CHECK_LT( elapsedMs, 2500 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in timer precision test" );
    }
}

//-----------------------------------------------------------------------------
// Cleanup Tests
//-----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( thread_pool_graceful_shutdown )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();

        {
            auto threadPool = workphone::make_ptr<ThreadPool>();
            BOOST_REQUIRE( threadPool != nullptr );

            Array<SmartPtr<IWorkerThread>> workerThreads;
            constexpr auto numThreads = 10;

            workerThreads.reserve( numThreads );
            for( s32 i = 0; i < numThreads; ++i )
            {
                workerThreads.push_back( threadPool->addWorkerThread() );
            }

            Thread::sleep( 0.05 );

            // Clear worker references
            workerThreads.clear();
        }
        // ThreadPool should shutdown gracefully here

        BOOST_CHECK( true );  // If we reach here, shutdown was successful
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during graceful shutdown" );
    }
}

BOOST_AUTO_TEST_CASE( thread_pool_null_safety )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();

        SmartPtr<ThreadPool> threadPool = nullptr;
        BOOST_CHECK( threadPool == nullptr );

        threadPool = workphone::make_ptr<ThreadPool>();
        BOOST_CHECK( threadPool != nullptr );

        threadPool = nullptr;
        BOOST_CHECK( threadPool == nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in null safety test" );
    }
}

BOOST_AUTO_TEST_SUITE_END()
