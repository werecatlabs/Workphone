#pragma once
#include <WPAudio/AudioOwnerDispatch.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <thread>
#include <atomic>
#include <array>
#include <stdexcept>

namespace audio_owner_dispatch_contracts
{
    using namespace workphone;
    inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
    inline void run()
    {
        auto app=core::IApplicationManager::instancePtr();auto tasks=app?app->getTaskManager():nullptr;
        auto primary=tasks?tasks->getTask(TaskId::Primary):nullptr;require(primary&&primary->isLoaded(),"Audio owner dispatch requires the actual loaded Primary task");
        const auto originalOwner=primary->getOwner();const auto originalState=primary->getState();
        struct RestoreTask {SmartPtr<ITask> task;SmartPtr<ISharedObject> owner;ITask::State state;~RestoreTask(){task->setOwner(owner);task->setState(state);}} restoreTask{primary,originalOwner,originalState};
        // Exercise its real job queue, without invoking unrelated Editor frame
        // work from this focused dispatch fixture.
        // Task::Flags promotes Idle to Executing for this update, then returns
        // to Idle on exit. Preselecting Executing would toggle it back to Idle
        // before the task reaches its job queue.
        primary->setOwner(nullptr);primary->setState(ITask::State::Idle);
        std::array<std::shared_ptr<audio::AudioOwnerDispatch>,3> gates{
            std::make_shared<audio::AudioOwnerDispatch>(),std::make_shared<audio::AudioOwnerDispatch>(),std::make_shared<audio::AudioOwnerDispatch>()};
        struct CancelGates {decltype(gates) &values;~CancelGates(){for(auto &gate:values)gate->cancel();}} cancel{gates};
        const auto osOwner=std::this_thread::get_id();std::atomic<u32> calls{0};std::atomic_bool correct{true},accepted{true};
        auto update=[&]{if(std::this_thread::get_id()!=osOwner||Thread::getCurrentTask()!=TaskId::Primary)correct=false;++calls;};
        std::thread worker([&]{Thread::setCurrentThreadId(Thread::ThreadId::WorkerThread);Thread::setCurrentTask(TaskId::Sound);
            String error;for(unsigned i=0;i<128;++i)if(!gates[0]->request(primary,update,error))accepted=false;});worker.join();
        require(accepted&&gates[0]->pending()&&calls==0,"Sound worker updates must enqueue one coalesced owner job, never directly execute the pump");
        primary->setNextUpdateTime(0);primary->update();
        require(correct&&calls==1&&gates[0]->executed()==1&&!gates[0]->pending(),"Actual Primary task execution must run and retire the coalesced owner pump");
        std::thread retired([&]{String error;if(!gates[1]->request(primary,update,error))accepted=false;});retired.join();gates[1]->cancel();
        primary->setNextUpdateTime(0);primary->update();require(calls==1&&!gates[1]->pending(),"A retired audio control generation must ignore a queued update safely");
        auto scheduler=WorldStreamingScheduler::shared();const auto category=size_t(StreamMemoryClass::PendingCpu);const auto limit=scheduler->stats().limits[category];String error;
        struct RestoreBudget {std::shared_ptr<WorldStreamingScheduler> scheduler;u64 bytes;~RestoreBudget(){String error;scheduler->setBudget(StreamMemoryClass::PendingCpu,bytes,error);}} restoreBudget{scheduler,limit};
        require(scheduler->setBudget(StreamMemoryClass::PendingCpu,scheduler->stats().used[category],error),error.c_str());
        bool refused=false;std::thread pressured([&]{String error;refused=!gates[2]->request(primary,update,error);});pressured.join();
        require(refused&&!gates[2]->pending()&&gates[2]->refused()>0,"Refused owner-job admission must clear coalescing state for a later retry");
        require(scheduler->setBudget(StreamMemoryClass::PendingCpu,limit,error),error.c_str());
        std::thread retry([&]{String error;if(!gates[2]->request(primary,update,error))accepted=false;});retry.join();
        primary->setNextUpdateTime(0);primary->update();require(accepted&&correct&&calls==2&&!gates[2]->pending(),"Audio owner dispatch must recover through the actual primary queue after pressure");
    }
}
