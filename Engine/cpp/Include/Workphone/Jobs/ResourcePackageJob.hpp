#pragma once
#include <Workphone/System/Job.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <atomic>
#include <mutex>
#include <memory>

namespace workphone
{
    /** Detached request/result. Worker code never reads application or UI state. */
    struct WPCore_API ResourcePackageJobState
    {
        mutable std::mutex mutex;
        Array<String> arguments;
        String projectRoot, executable, logPath, snapshotPath;
        String previewPath;
        // Captured before queueing. Zero explicitly disables the respective limit.
        u32 maximumRunMilliseconds = 30 * 60 * 1000;
        u32 cancelGraceMilliseconds = 30 * 1000;
        u32 historyKeepCount = 64;
        u32 historyMaximumBytes = 64 * 1024 * 1024;
        u32 maximumLogBytes = 32 * 1024 * 1024;
        String status = "queued", diagnostic;
        std::atomic<bool> cancel{ false };
        std::atomic<bool> finished{ false };
        int exitCode = -1;
    };

    class WPCore_API ResourcePackageJob final : public Job
    {
    public:
        explicit ResourcePackageJob( std::shared_ptr<ResourcePackageJobState> state );
        void execute() override;
        void stop() override;

    private:
        std::shared_ptr<ResourcePackageJobState> m_request;
    };
}  // namespace workphone
