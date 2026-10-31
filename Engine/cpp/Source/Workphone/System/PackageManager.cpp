#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/PackageManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/INativeFileDialog.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Jobs/JobCreatePackage.hpp>
#include <Workphone/Jobs/ResourcePackageJob.hpp>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <filesystem>
#include <cstdlib>
#ifdef _WIN32
#    include <Windows.h>
#endif

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, PackageManager, IPackageManager );

    PackageManager::PackageManager() = default;

    PackageManager::~PackageManager()
    {
        cancelResourcePackage();
    }

    void PackageManager::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_buildTargetPlatform.resize( (size_t)TargetPlatform::Count );
        setLoadingState( LoadingState::Loaded );
    }

    void PackageManager::unload( SmartPtr<ISharedObject> data )
    {
        cancelResourcePackage();
        setLoadingState( LoadingState::Unloading );
        m_buildTargetPlatform.clear();
        setLoadingState( LoadingState::Unloaded );
    }

    void PackageManager::createPackage()
    {
        startResourcePackage();
    }

    void PackageManager::setResourcePackageProfile( const String &path )
    {
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        m_resourcePackageProfile = path;
    }

    String PackageManager::getResourcePackageProfile() const
    {
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        return m_resourcePackageProfile;
    }

    bool PackageManager::startResourcePackage()
    {
        return startResourcePackageRequest( false );
    }

    bool PackageManager::startResourcePackagePreview()
    {
        return startResourcePackageRequest( true );
    }

    String PackageManager::getResourcePackagePreviewPath() const
    {
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        if( !m_resourcePackageError.empty() || !m_resourcePackageRequest ||
            !m_resourcePackageRequest->finished )
        {
            return {};
        }
        auto &request = *m_resourcePackageRequest;
        std::lock_guard<std::mutex> resultLock( request.mutex );
        auto app = core::IApplicationManager::instancePtr();
        std::error_code ec;
        if( !app ||
            !std::filesystem::equivalent( std::filesystem::u8path( app->getProjectPath().c_str() ),
                                          std::filesystem::u8path( request.projectRoot.c_str() ), ec ) ||
            ec || request.status != "succeeded" )
        {
            return {};
        }
        return request.previewPath;
    }

    bool PackageManager::startResourcePackageRequest( bool preview )
    {
        namespace fs = std::filesystem;
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        if( m_resourcePackageRequest && !m_resourcePackageRequest->finished )
        {
            return false;
        }
        m_resourcePackageError.clear();
        try
        {
            auto app = core::IApplicationManager::instance();
            auto queue = app->getJobQueue();
            if( !queue || !queue->isRunning() || !app->isRunning() || app->getQuit() ||
                app->getProjectPath().empty() )
            {
                throw std::runtime_error( "A project and job queue are required" );
            }
            auto workers = app->getThreadPool();
            if( !workers || workers->getNumThreads() == 0 ||
                workers->getState() != IThreadPool::State::Start )
            {
                throw std::runtime_error( "Resource packaging requires a worker thread pool" );
            }
            const auto project = fs::canonical( fs::u8path( app->getProjectPath().c_str() ) );
            auto profile = fs::u8path( m_resourcePackageProfile.c_str() );
            if( profile.empty() )
            {
                throw std::runtime_error( "Select a saved resource package profile" );
            }
            if( profile.is_relative() )
            {
                profile = project / profile;
            }
            auto arguments = loadPackageProfile( profile.u8string().c_str() );
            if( !fs::equivalent( project, fs::u8path( arguments[1].c_str() ) ) )
            {
                throw std::runtime_error(
                    "Package profile source root does not match the active project" );
            }
            auto request = std::make_shared<ResourcePackageJobState>();
            auto executionLimit = []( const char *name, u32 fallback, u32 maximum, u32 minimum = 1000 ) -> u32 {
                const auto raw = std::getenv( name );
                if( !raw )
                {
                    return fallback;
                }
                const std::string value( raw );
                if( value.empty() || value.size() > 10 ||
                    value.find_first_not_of( "0123456789" ) != std::string::npos )
                {
                    throw std::runtime_error( std::string( "Invalid execution limit: " ) + name );
                }
                const auto number = std::stoull( value );
                if( number > maximum || ( number != 0 && number < minimum ) )
                {
                    throw std::runtime_error( std::string( "Execution limit is out of range: " ) +
                                              name );
                }
                return static_cast<u32>( number );
            };
            request->maximumRunMilliseconds = executionLimit(
                "WP_RESOURCE_PACKAGE_MAX_RUN_MS", request->maximumRunMilliseconds, 24 * 60 * 60 * 1000 );
            request->cancelGraceMilliseconds =
                executionLimit( "WP_RESOURCE_PACKAGE_CANCEL_GRACE_MS", request->cancelGraceMilliseconds,
                                10 * 60 * 1000 );
            request->historyKeepCount = executionLimit("WP_RESOURCE_PACKAGE_HISTORY_KEEP_COUNT",
                request->historyKeepCount, 4096, 1);
            request->historyMaximumBytes = executionLimit("WP_RESOURCE_PACKAGE_HISTORY_MAX_BYTES",
                request->historyMaximumBytes, 1024 * 1024 * 1024, 1024);
            request->maximumLogBytes = executionLimit("WP_RESOURCE_PACKAGE_MAX_LOG_BYTES",
                request->maximumLogBytes, 512 * 1024 * 1024, 1024);
            request->arguments = arguments;
            request->projectRoot = project.u8string().c_str();
            const auto id = StringUtil::getUUID();
            const auto jobs = project / ".workphone" / "package-jobs";
            request->snapshotPath = ( jobs / fs::u8path( ( id + ".json" ).c_str() ) ).u8string().c_str();
            request->logPath = ( jobs / fs::u8path( ( id + ".log" ).c_str() ) ).u8string().c_str();
            if( preview )
            {
                request->previewPath =
                    ( jobs / fs::u8path( ( id + ".preview.json" ).c_str() ) ).u8string().c_str();
            }
#ifdef _WIN32
            std::vector<wchar_t> module( 32768 );
            const auto count =
                GetModuleFileNameW( nullptr, module.data(), static_cast<DWORD>( module.size() ) );
            if( count == 0 || count >= module.size() )
            {
                throw std::runtime_error( "Cannot locate package executable" );
            }
            request->executable = ( fs::path( std::wstring( module.data(), count ) ).parent_path() /
                                    L"WorkphoneResourcePackage.exe" )
                                      .u8string()
                                      .c_str();
            if( !fs::is_regular_file( fs::u8path( request->executable.c_str() ) ) )
            {
                throw std::runtime_error(
                    "WorkphoneResourcePackage.exe must be deployed beside the Editor" );
            }
#else
            throw std::runtime_error(
                "Asynchronous resource packaging is currently supported on Windows only" );
#endif
            auto job = make_ptr<ResourcePackageJob>( request );
            m_resourcePackageRequest = request;
            try
            {
                queue->addJob( job );
            }
            catch( ... )
            {
                m_resourcePackageRequest.reset();
                throw;
            }
            if( job->getState() == IJob::State::Ready )
            {
                m_resourcePackageRequest.reset();
                throw std::runtime_error( "The job queue did not accept the package request" );
            }
            return true;
        }
        catch( const std::exception &error )
        {
            m_resourcePackageError = error.what();
            return false;
        }
    }

    void PackageManager::cancelResourcePackage()
    {
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        if( m_resourcePackageRequest && !m_resourcePackageRequest->finished )
        {
            m_resourcePackageRequest->cancel = true;
        }
    }

    bool PackageManager::isResourcePackageBusy() const
    {
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        return m_resourcePackageRequest && !m_resourcePackageRequest->finished;
    }

    String PackageManager::getResourcePackageStatus() const
    {
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        if( !m_resourcePackageError.empty() )
        {
            return "failed";
        }
        if( !m_resourcePackageRequest )
        {
            return "idle";
        }
        auto &request = *m_resourcePackageRequest;
        auto app = core::IApplicationManager::instancePtr();
        std::error_code error;
        if( app && !std::filesystem::equivalent(
                       std::filesystem::u8path( app->getProjectPath().c_str() ),
                       std::filesystem::u8path( request.projectRoot.c_str() ), error ) )
        {
            request.cancel = true;
            return "previous project";
        }
        std::lock_guard<std::mutex> resultLock( request.mutex );
        if( request.cancel && !request.finished )
        {
            return "cancelling";
        }
        return request.status;
    }

    String PackageManager::getResourcePackageDiagnostic() const
    {
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        if( !m_resourcePackageError.empty() )
        {
            return m_resourcePackageError;
        }
        if( !m_resourcePackageRequest )
        {
            return "Select a saved profile to build a checked resource directory";
        }
        std::lock_guard<std::mutex> resultLock( m_resourcePackageRequest->mutex );
        return m_resourcePackageRequest->diagnostic;
    }

    String PackageManager::getResourcePackageLogPath() const
    {
        std::lock_guard<std::mutex> lock( m_resourcePackageMutex );
        return m_resourcePackageRequest ? m_resourcePackageRequest->logPath : String();
    }

    bool PackageManager::getPackageTextures() const
    {
        return m_packageTextures;
    }

    void PackageManager::setPackageTextures( bool packageTextures )
    {
        m_packageTextures = packageTextures;
    }

    Array<bool> PackageManager::getBuildTargetPlatform() const
    {
        return m_buildTargetPlatform;
    }

    void PackageManager::setBuildTargetPlatform( const Array<bool> &buildTargetPlatform )
    {
        m_buildTargetPlatform = buildTargetPlatform;
    }

    bool PackageManager::getBuildTargetPlatformEnabled( TargetPlatform targetPlatform ) const
    {
        auto index = static_cast<size_t>( targetPlatform );
        if( index >= m_buildTargetPlatform.size() )
        {
            return false;
        }

        return m_buildTargetPlatform[index];
    }

    void PackageManager::setBuildTargetPlatformEnabled( TargetPlatform targetPlatform, bool enabled )
    {
        auto index = static_cast<size_t>( targetPlatform );
        if( index >= m_buildTargetPlatform.size() )
        {
            m_buildTargetPlatform.resize( static_cast<size_t>( TargetPlatform::Count ) );
        }

        m_buildTargetPlatform[index] = enabled;
    }

}  // namespace workphone
