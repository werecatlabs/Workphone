#include <WPAudio/WPAudioPCH.hpp>
#include <WPAudio/AudioOwnerDispatch.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Interface/System/ITask.hpp>

namespace workphone::audio
{
    class AudioOwnerPumpJob final : public Job
    {
    public:
        StreamBudget admission;
        std::shared_ptr<AudioOwnerDispatch> gate;
        std::function<void()> update;
        ~AudioOwnerPumpJob() override{if(gate)gate->m_pending=false;}
        void execute() override
        {
            if(!gate||!gate->m_alive)return;
            if(!gate->owner()){++gate->m_refused;return;}
            ++gate->m_executed;if(update)update();
        }
    };
    AudioOwnerDispatch::AudioOwnerDispatch():m_owner(std::this_thread::get_id()){}
    bool AudioOwnerDispatch::request(SmartPtr<ITask> primary,std::function<void()> update,String &error)
    {
        error.clear();if(!m_alive){error="Audio control generation was retired";return false;}
        bool expected=false;if(!m_pending.compare_exchange_strong(expected,true))return true;
        try {
            if(!primary||!primary->isLoaded()||primary->getTask()!=TaskId::Primary||!update)throw std::runtime_error("Audio control update requires the existing Primary task dispatcher");
            auto admission=WorldStreamingScheduler::shared()->reserve(StreamMemoryClass::PendingCpu,4096,error);
            if(!admission){m_pending=false;++m_refused;return false;}
            auto job=make_ptr<AudioOwnerPumpJob>();job->admission=std::move(admission);job->gate=shared_from_this();job->update=std::move(update);primary->addJob(job);return true;
        }catch(const std::exception &failure){m_pending=false;++m_refused;try{error=failure.what();}catch(...){error.clear();}return false;}
    }
    void AudioOwnerDispatch::cancel() noexcept{m_alive=false;}
    bool AudioOwnerDispatch::owner() const noexcept{return m_owner==std::this_thread::get_id();}
    bool AudioOwnerDispatch::pending() const noexcept{return m_pending.load();}
    u64 AudioOwnerDispatch::executed() const noexcept{return m_executed.load();}
    u64 AudioOwnerDispatch::refused() const noexcept{return m_refused.load();}
}
