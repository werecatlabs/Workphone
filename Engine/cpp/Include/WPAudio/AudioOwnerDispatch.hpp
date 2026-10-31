#pragma once
#include <WPAudio/WPAudioPrerequisites.hpp>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>

namespace workphone::audio
{
    class AudioOwnerPumpJob;
    /** Coalesces native control work on the existing Primary task. The gate
     * outlives cancelled jobs and never dereferences a retired platform state. */
    class WPAudio_API AudioOwnerDispatch final : public std::enable_shared_from_this<AudioOwnerDispatch>
    {
    public:
        AudioOwnerDispatch();
        bool request(SmartPtr<ITask> primary,std::function<void()> update,String &error);
        void cancel() noexcept;
        bool owner() const noexcept;
        bool pending() const noexcept;
        u64 executed() const noexcept;
        u64 refused() const noexcept;
    private:
        friend class AudioOwnerPumpJob;
        std::thread::id m_owner;
        std::atomic_bool m_alive{true},m_pending{false};
        std::atomic<u64> m_executed{0},m_refused{0};
    };
}
