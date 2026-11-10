#ifndef JobCreatePackage_h__
#define JobCreatePackage_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <atomic>
#include <mutex>

namespace workphone
{
    /** Builds and publishes a standalone Windows game from a saved project. */
    class WPCore_API JobCreatePackage final : public Job
    {
    public:
        JobCreatePackage( const String &projectFile, const String &destination, const String &mediaPath,
                          const String &cachePath, const String &settingsPath,
                          const String &mediaCatalogPath );

        void execute() override;
        void stop() override;
        void cancel();

        String getStatus() const;
        String getDiagnostic() const;
        String getOutputPath() const;

        WP_CLASS_REGISTER_DECL;

    private:
        void report( const String &status, const String &diagnostic );
        void checkCancelled() const;

        const String m_projectFile;
        const String m_destination;
        const String m_mediaPath;
        const String m_cachePath;
        const String m_settingsPath;
        const String m_mediaCatalogPath;
        SmartPtr<IDatabase> m_catalog;

        std::atomic<bool> m_cancelled{ false };
        mutable std::mutex m_resultMutex;
        String m_status = "queued";
        String m_diagnostic = "Waiting for a build worker";
        String m_outputPath;
    };
}  // namespace workphone

#endif  // JobCreatePackage_h__
