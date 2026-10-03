#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/coroutine2/coroutine.hpp>
#include <chrono>
#include <thread>

using namespace workphone;

BOOST_AUTO_TEST_CASE( thread_id )
{
    auto task = TaskId::Primary;
    Thread::setCurrentTask( task );
    BOOST_CHECK( Thread::getCurrentTask() == TaskId::Primary );

    auto threadId = Thread::ThreadId::Primary;
    Thread::setCurrentThreadId( threadId );
    BOOST_CHECK( Thread::getCurrentThreadId() == Thread::ThreadId::Primary );
}

// Represents a wait for a specified duration
class WaitForSecondsTest
{
public:
    explicit WaitForSecondsTest( ICoroutineData::PullType &yield, double seconds ) : duration( seconds )
    {
        auto timer = workphone::make_ptr<TimerBoost>();
        auto start = timer->now();
        auto end = start + seconds;

        while( timer->now() < end )
        {
            yield();
        }
    }

    bool await_ready() const noexcept
    {
        return false;  // Coroutine will always suspend
    }

    void await_suspend( boost::coroutines2::coroutine<void>::push_type &yield )
    {
        std::this_thread::sleep_for( std::chrono::duration<double>( duration ) );
        yield();  // Resume coroutine after duration
    }

    void await_resume() noexcept
    {
    }

private:
    double duration;  // Duration to wait
};

class TestCocourtineObject : public ISharedObject
{
public:
    TestCocourtineObject() = default;
    ~TestCocourtineObject() override = default;

    void testCoroutine( ICoroutineData::PullType &pull )
    {
        auto timer = workphone::make_ptr<TimerBoost>();
        auto start = timer->now();

        std::cout << "Coroutine: " << std::endl;
        pull();
        std::cout << "Coroutine: " << std::endl;
        pull();
        std::cout << "Coroutine: " << std::endl;
        pull();

        // Yield for 1 second using WaitForSeconds
        WaitForSecondsTest( pull, 1.0 );
        std::cout << "Coroutine: " << std::endl;
        pull();

        auto end = timer->now();
        auto timeTaken = end - start;

        std::cout << "Coroutine: start: " << start << std::endl;
        std::cout << "Coroutine: end: " << end << std::endl;
        std::cout << "Coroutine: Time taken: " << timeTaken << std::endl;

        BOOST_CHECK( timeTaken >= 1.0 );
    }

    ICoroutineData::PushType testCoroutine2( ICoroutineData::PullType &pull )
    {
        using namespace boost::coroutines2;

        std::cout << "Coroutine: " << std::endl;
        pull();
    }
};

BOOST_AUTO_TEST_CASE( coroutine_test )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto jobQueue = applicationManager->getJobQueue();

        auto object = workphone::make_ptr<TestCocourtineObject>();
        auto boundFunc =
            std::bind( &TestCocourtineObject::testCoroutine, object.get(), std::placeholders::_1 );

        auto coroutineFunc = [&boundFunc]( ICoroutineData::PullType &pull ) { boundFunc( pull ); };

        ICoroutineData::PushType coroutine( coroutineFunc );

        // Execute the coroutine until it's done
        auto count = 0;
        while( coroutine )
        {
            coroutine();
            count++;
        }

        BOOST_CHECK( count > 0 );

        applicationManager = nullptr;  //added to be a break point
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( coroutine_jobqueue_test )
{
    try
    {
        auto currentThreadId = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( currentThreadId );

        auto task = TaskId::Primary;
        Thread::setCurrentTask( task );

        auto taskFlags = std::numeric_limits<u32>::max();
        Thread::setTaskFlags( taskFlags );

        auto applicationManager = core::IApplicationManager::instance();
        auto taskManager = applicationManager->getTaskManager();
        auto jobQueue = applicationManager->getJobQueue();

        auto object = workphone::make_ptr<TestCocourtineObject>();
        auto boundFunc =
            std::bind( &TestCocourtineObject::testCoroutine, object.get(), std::placeholders::_1 );
        jobQueue->startCoroutine( boundFunc );

        auto running = jobQueue->hasJobs();
        while( running )
        {
            // The job queue only steps coroutines while it is running. In the
            // headless unit-test environment the queue is not running, so without
            // this guard the coroutine would never advance and the loop would
            // spin forever on pending jobs.
            if( !jobQueue->isRunning() )
            {
                break;
            }

            jobQueue->update();

            running = jobQueue->hasJobs();
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
