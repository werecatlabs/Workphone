#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

using namespace workphone;

// Mock implementations for IJob and IJobQueue
class MockJob : public IJob
{
public:
    State state;
    s32 affinity;
    u32 progress;
    s32 priority;
    bool primary;
    bool coroutine;
    bool finished;

    MockJob() :
        state( State::Ready ),
        affinity( 0 ),
        progress( 0 ),
        priority( 0 ),
        primary( false ),
        coroutine( false ),
        finished( false )
    {
    }

    s32 getAffinity() const override
    {
        return affinity;
    }
    void setAffinity( s32 affinity ) override
    {
        this->affinity = affinity;
    }
    void execute() override
    {
    }
    void coroutine_execute() override
    {
    }
    void coroutine_execute_step( SmartPtr<ICoroutineData> &yield ) override
    {
    }
    State getState() const override
    {
        return state;
    }
    void setState( State state ) override
    {
        this->state = state;
    }
    u32 getProgress() const override
    {
        return progress;
    }
    void setProgress( u32 progress ) override
    {
        this->progress = progress;
    }
    s32 getPriority() const override
    {
        return priority;
    }
    void setPriority( s32 priority ) override
    {
        this->priority = priority;
    }
    bool isPrimary() const override
    {
        return primary;
    }
    void setPrimary( bool primary ) override
    {
        this->primary = primary;
    }
    bool isFinished() const override
    {
        return finished;
    }
    bool wait() override
    {
        return finished;
    }
    bool wait( f64 maxWaitTime ) override
    {
        return finished;
    }
    bool isCoroutine() const override
    {
        return coroutine;
    }
    void setCoroutine( bool coroutine ) override
    {
        this->coroutine = coroutine;
    }

    void setCallbackFunction( std::function<void( int )> callbackFunction ) override
    {
    }

    void setInterrupted( bool interrupted ) override
    {
    }

    bool isInterrupted() const override
    {
        return false;
    }

    void stop() override
    {
    }

    Parameter handleEvent(EventType eventType, hash_type eventValue, const Array<Parameter>& arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
        SmartPtr<IEvent> event)
    {
        return {};
    }
};

class MockJobQueue : public IJobQueue
{
public:
    bool running;
    f32 rate;
    bool useAffinity;
    std::vector<SmartPtr<IJob>> jobs;

    MockJobQueue() : running( false ), rate( 0.0f ), useAffinity( false )
    {
    }

    bool hasJobs() const override
    {
        return !jobs.empty();
    }
    void addJob( SmartPtr<IJob> job ) override
    {
        jobs.push_back( job );
    }
    void addJob( SmartPtr<IJob> job, TaskId task ) override
    {
        jobs.push_back( job );
    }
    void addJobAllTasks( SmartPtr<IJob> job ) override
    {
        jobs.push_back( job );
    }
    bool isRunning() const override
    {
        return running;
    }
    void setRunning( bool running ) override
    {
        this->running = running;
    }
    f32 getRate() const override
    {
        return rate;
    }
    void setRate( f32 rate ) override
    {
        this->rate = rate;
    }
    bool getUseAffinity() const override
    {
        return useAffinity;
    }
    void setUseAffinity( bool affinity ) override
    {
        this->useAffinity = affinity;
    }
    void shutdown() override
    {
        jobs.clear();
    }
    void startCoroutine( std::function<void( ICoroutineData::PullType & )> func ) override
    {
    }

    SmartPtr<IJob> startJob( std::function<void()> func ) override
    {
        func();
        return nullptr;
    }

    void clearEventJobs() override
    {
    }
};

BOOST_AUTO_TEST_CASE( test_job_queue_add_job )
{
    MockJobQueue jobQueue;
    auto job = workphone::make_ptr<MockJob>();

    BOOST_CHECK( !jobQueue.hasJobs() );
    jobQueue.addJob( job );
    BOOST_CHECK( jobQueue.hasJobs() );
}

BOOST_AUTO_TEST_CASE( test_job_queue_running_state )
{
    MockJobQueue jobQueue;

    BOOST_CHECK( !jobQueue.isRunning() );
    jobQueue.setRunning( true );
    BOOST_CHECK( jobQueue.isRunning() );
}

BOOST_AUTO_TEST_CASE( test_job_queue_rate )
{
    MockJobQueue jobQueue;
    float rate = 2.5f;

    jobQueue.setRate( rate );
    BOOST_CHECK_EQUAL( jobQueue.getRate(), rate );
}

BOOST_AUTO_TEST_CASE( test_job_queue_use_affinity )
{
    MockJobQueue jobQueue;

    BOOST_CHECK( !jobQueue.getUseAffinity() );
    jobQueue.setUseAffinity( true );
    BOOST_CHECK( jobQueue.getUseAffinity() );
}

BOOST_AUTO_TEST_CASE( test_job_queue_shutdown )
{
    MockJobQueue jobQueue;
    auto job = workphone::make_ptr<MockJob>();
    jobQueue.addJob( job );

    BOOST_CHECK( jobQueue.hasJobs() );
    jobQueue.shutdown();
    BOOST_CHECK( !jobQueue.hasJobs() );
}

BOOST_AUTO_TEST_CASE( test_job_start )
{
    auto jobQueue = workphone::make_ptr<JobQueue>();

    jobQueue->startJob( []() { WP_LOG( "job" ); } );

    while( jobQueue->hasJobs() )
    {
        jobQueue->update();
    }
}
