#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ProjectServices.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Net/INetworkManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IFactory.hpp>
#include <Workphone/Script/ProjectScriptPolicy.hpp>
#include <Workphone/Graphics/ProjectIrradiance.hpp>
#include <Workphone/Graphics/ProjectAssetPolicy.hpp>
#include <Workphone/Graphics/ProjectGraphicsProviders.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <stdexcept>
#if defined( _WIN32 )
#    include <windows.h>
#    include <dbghelp.h>
#endif

namespace workphone
{
    namespace
    {
        namespace fs = std::filesystem;
        struct ProjectServiceState
        {
            ProjectServiceProviders providers;
            bool cloud = false, analytics = false, crash = false;
        };
        std::shared_ptr<const ProjectServiceState> state = std::make_shared<ProjectServiceState>();
        std::mutex configureMutex;
        std::mutex lifecycleMutex;
        String localProviderRoot;
        ProjectServiceProviders localProviders;
        ProjectServiceProviders providerOverrides;
        s32 selectedNetworkBackend = -1;
        SmartPtr<INetworkManager> projectNetwork;
#if defined( _WIN32 )
        thread_local EXCEPTION_POINTERS *currentException = nullptr;
#endif

        bool slotValid( const String &slot )
        {
            return !slot.empty() && slot.size() <= 128 && slot != "." && slot != ".." &&
                   slot.find_first_not_of( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-" ) == String::npos;
        }

        class LocalCloud final : public IProjectCloudSaveProvider
        {
            fs::path root;
            std::mutex mutex;
            fs::path pathFor( const String &slot ) const
            {
                if( !slotValid( slot ) ) throw std::invalid_argument( "Invalid cloud save slot" );
                const auto path = root / ( "slot-" + std::string( slot.c_str() ) + ".save" );
                if( fs::is_symlink( fs::symlink_status( path ) ) )
                    throw std::runtime_error( "Cloud save refuses redirected files" );
                return path;
            }
        public:
            explicit LocalCloud( fs::path directory ) : root( std::move( directory ) )
            {
                fs::create_directories( root );
                root = fs::canonical( root );
            }
            bool put( const String &slot, const Array<u8> &bytes, String &error ) override
            {
                std::lock_guard<std::mutex> lock( mutex );
                fs::path temporary;
                try
                {
                    if( bytes.size() > 64 * 1024 * 1024 ) throw std::runtime_error( "Save exceeds 64 MiB" );
                    const auto path = pathFor( slot );
                    temporary = root / ( std::string( StringUtil::getUUID().c_str() ) + ".tmp" );
                    std::ofstream stream( temporary, std::ios::binary | std::ios::trunc );
                    stream.exceptions( std::ios::failbit | std::ios::badbit );
                    if( !bytes.empty() ) stream.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
                    stream.close();
#if defined( _WIN32 )
                    if( !MoveFileExW( temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                        throw std::runtime_error( "Unable to publish cloud save" );
#else
                    fs::rename( temporary, path );
#endif
                    error.clear(); return true;
                }
                catch( const std::exception &e )
                {
                    std::error_code ignored; if( !temporary.empty() ) fs::remove( temporary, ignored );
                    error = e.what(); return false;
                }
            }
            bool get( const String &slot, Array<u8> &bytes, String &error ) override
            {
                std::lock_guard<std::mutex> lock( mutex );
                try
                {
                    const auto path = pathFor( slot );
                    const auto size = fs::file_size( path );
                    if( size > 64 * 1024 * 1024 ) throw std::runtime_error( "Save exceeds 64 MiB" );
                    std::ifstream stream( path, std::ios::binary );
                    stream.exceptions( std::ios::failbit | std::ios::badbit );
                    Array<u8> result( static_cast<size_t>( size ) );
                    if( size ) stream.read( reinterpret_cast<char *>( result.data() ), size );
                    bytes = std::move( result ); error.clear(); return true;
                }
                catch( const std::exception &e ) { error = e.what(); return false; }
            }
            bool remove( const String &slot, String &error ) override
            {
                std::lock_guard<std::mutex> lock( mutex );
                try { fs::remove( pathFor( slot ) ); error.clear(); return true; }
                catch( const std::exception &e ) { error = e.what(); return false; }
            }
            bool list( Array<String> &slots, String &error ) override
            {
                std::lock_guard<std::mutex> lock( mutex );
                try
                {
                    Array<String> result;
                    for( const auto &entry : fs::directory_iterator( root ) )
                        if( !entry.is_symlink() && entry.is_regular_file() && entry.path().extension() == ".save" )
                        {
                            const auto stem = entry.path().stem().u8string();
                            if( stem.compare( 0, 5, "slot-" ) == 0 ) result.push_back( stem.substr( 5 ).c_str() );
                        }
                    std::sort( result.begin(), result.end() ); slots = std::move( result ); error.clear(); return true;
                }
                catch( const std::exception &e ) { error = e.what(); return false; }
            }
        };

        class LocalAnalytics final : public IProjectAnalyticsProvider
        {
            fs::path journal;
            std::mutex mutex;
        public:
            explicit LocalAnalytics( const fs::path &directory ) : journal( directory / "events.jsonl" )
            { fs::create_directories( directory ); }
            bool record( const String &event, SmartPtr<Properties> fields, String &error ) override
            {
                std::lock_guard<std::mutex> lock( mutex );
                try
                {
                    if( event.empty() || event.size() > 256 ) throw std::invalid_argument( "Invalid analytics event name" );
                    auto entry = make_ptr<Properties>();
                    entry->setProperty( "event", event );
                    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch() ).count();
                    entry->setProperty( "timestampMs", StringUtil::toString( timestamp ) );
                    if( fields )
                    {
                        auto copy = make_ptr<Properties>();
                        DataUtil::parse( DataUtil::toString( fields.get() ), copy.get() );
                        copy->setName( "fields" ); entry->addChild( copy );
                    }
                    const auto text = DataUtil::toString( entry.get(), false );
                    if( text.size() > 1024 * 1024 ) throw std::runtime_error( "Analytics event exceeds 1 MiB" );
                    std::ofstream stream( journal, std::ios::binary | std::ios::app );
                    stream.exceptions( std::ios::failbit | std::ios::badbit );
                    stream.write( text.data(), text.size() ); stream.put( '\n' ); stream.flush();
                    error.clear(); return true;
                }
                catch( const std::exception &e ) { error = e.what(); return false; }
            }
        };

        class LocalCrash final : public IProjectCrashProvider
        {
            fs::path root;
            bool enabled = false;
            std::recursive_mutex mutex;
        public:
            explicit LocalCrash( fs::path directory ) : root( std::move( directory ) ) { fs::create_directories( root ); }
            bool setEnabled( bool value, String &error ) override
            { std::lock_guard<std::recursive_mutex> lock( mutex ); enabled = value; error.clear(); return true; }
            bool capture( const String &reason, String &path, String &error ) override
            {
                std::lock_guard<std::recursive_mutex> lock( mutex );
                if( !enabled ) { error = "Crash reporting is disabled"; return false; }
                try
                {
                    const auto base = root / std::string( StringUtil::getUUID().c_str() );
                    auto diagnostic = make_ptr<Properties>(); diagnostic->setProperty( "reason", reason );
                    std::ofstream metadata( base.u8string() + ".json", std::ios::binary );
                    metadata.exceptions( std::ios::failbit | std::ios::badbit );
                    metadata << DataUtil::toString( diagnostic.get(), true ); metadata.close();
#if defined( _WIN32 )
                    const auto library = LoadLibraryW( L"dbghelp.dll" );
                    if( !library ) throw std::runtime_error( "DbgHelp is unavailable" );
                    using WriteDump = BOOL (WINAPI *)(HANDLE,DWORD,HANDLE,MINIDUMP_TYPE,PMINIDUMP_EXCEPTION_INFORMATION,PMINIDUMP_USER_STREAM_INFORMATION,PMINIDUMP_CALLBACK_INFORMATION);
                    const auto writeDump = reinterpret_cast<WriteDump>( GetProcAddress( library, "MiniDumpWriteDump" ) );
                    const auto dump = fs::path( base.u8string() + ".dmp" );
                    const auto file = CreateFileW( dump.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr );
                    MINIDUMP_EXCEPTION_INFORMATION information{ GetCurrentThreadId(), currentException, FALSE };
                    const bool success = writeDump && file != INVALID_HANDLE_VALUE &&
                        writeDump( GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal,
                                   currentException ? &information : nullptr, nullptr, nullptr );
                    if( file != INVALID_HANDLE_VALUE ) CloseHandle( file );
                    FreeLibrary( library );
                    if( !success ) throw std::runtime_error( "Unable to capture Windows minidump" );
                    path = dump.u8string().c_str();
#else
                    path = ( base.u8string() + ".json" ).c_str();
#endif
                    error.clear(); return true;
                }
                catch( const std::exception &e ) { error = e.what(); return false; }
            }
        };
#if defined( _WIN32 )
        std::atomic<LPTOP_LEVEL_EXCEPTION_FILTER> previousExceptionFilter{ nullptr };
        bool crashFilterInstalled = false;
        LONG WINAPI crashFilter( EXCEPTION_POINTERS *exception )
        {
            static std::atomic_flag capturing = ATOMIC_FLAG_INIT;
            if( capturing.test_and_set() ) return EXCEPTION_CONTINUE_SEARCH;
            const auto current = std::atomic_load( &state );
            if( current->crash && current->providers.crash )
            {
                String path, error;
                currentException = exception;
                current->providers.crash->capture( "Unhandled Windows exception", path, error );
                currentException = nullptr;
            }
            capturing.clear();
            const auto previous = previousExceptionFilter.load();
            return previous ? previous( exception ) : EXCEPTION_CONTINUE_SEARCH;
        }
#endif
    }

    ProjectServiceProviders makeLocalProjectServiceProviders( const String &directory )
    {
        const auto root = fs::absolute( fs::u8path( directory.c_str() ) ).lexically_normal();
        return { std::make_shared<LocalCloud>( root / "cloud-saves" ),
                 std::make_shared<LocalAnalytics>( root / "analytics" ),
                 std::make_shared<LocalCrash>( root / "crashes" ) };
    }

    void setProjectServiceProviders( ProjectServiceProviders providers )
    {
        std::lock_guard<std::mutex> lock( lifecycleMutex );
        providerOverrides = std::move( providers );
    }

    bool configureProjectServices( ProjectServiceProviders providers, bool cloud, bool analytics,
                                    bool crash, String &error )
    {
        std::lock_guard<std::mutex> lock( configureMutex );
        if( ( cloud && !providers.cloud ) || ( analytics && !providers.analytics ) || ( crash && !providers.crash ) )
        { error = "An enabled project service requires a provider"; return false; }
        if( providers.crash && !providers.crash->setEnabled( crash, error ) ) return false;
        auto next = std::make_shared<ProjectServiceState>(); next->providers = std::move( providers );
        next->cloud = cloud; next->analytics = analytics; next->crash = crash;
        std::atomic_store( &state, std::shared_ptr<const ProjectServiceState>( next ) );
#if defined( _WIN32 )
        if( crash && !crashFilterInstalled )
        {
            previousExceptionFilter.store( SetUnhandledExceptionFilter( crashFilter ) );
            crashFilterInstalled = true;
        }
        else if( !crash && crashFilterInstalled )
        {
            SetUnhandledExceptionFilter( previousExceptionFilter.load() );
            crashFilterInstalled = false;
        }
#endif
        error.clear(); return true;
    }

    bool applyProjectServiceSettings( SmartPtr<Properties> settings, const String &projectDirectory,
                                      String &error )
    {
        if( !settings || projectDirectory.empty() ) { error = "Project service profile or directory is missing"; return false; }
        std::lock_guard<std::mutex> lock( lifecycleMutex );
        struct PendingNetwork
        {
            SmartPtr<INetworkManager> instance;
            ~PendingNetwork()
            {
                if( instance ) try { instance->unload( nullptr ); } catch( ... ) {}
            }
        } pendingNetwork;
        try
        {
            auto app = core::IApplicationManager::instancePtr();
            const bool configureNetwork = settings->hasProperty( "network.multiplayer" ) || settings->hasProperty( "network.backend" );
            const auto networkBackend = settings->getPropertyAsInt( "network.backend", std::max( 0, selectedNetworkBackend ) );
            const bool multiplayer = settings->getPropertyAsBool( "network.multiplayer", projectNetwork != nullptr );
            if( multiplayer && ( networkBackend < 1 || networkBackend > 3 ) )
                throw std::invalid_argument( "Multiplayer requires a network backend" );
            auto &nextNetwork = pendingNetwork.instance;
            if( configureNetwork && multiplayer && app && ( !projectNetwork || selectedNetworkBackend != networkBackend ) )
            {
                auto factory = app->getFactoryManager();
                const String provider = networkBackend == 1 ? "WPNetworkManager" :
                                        networkBackend == 2 ? "BoostAsioNetworkManager" : "PlatformNetworkManager";
                auto registration = factory ? factory->getFactoryByName( provider ) : nullptr;
                if( !registration || !registration->isObjectDerivedFromByInfo( INetworkManager::typeInfo() ) )
                    throw std::runtime_error( ( "Install a registered INetworkManager provider: " + provider ).c_str() );
                nextNetwork = factory ? factory->createObjectFromType<INetworkManager>( provider ) : nullptr;
                if( !nextNetwork ) throw std::runtime_error( ( "Install network provider: " + provider ).c_str() );
                nextNetwork->setProperties( settings );
                nextNetwork->load( settings );
                nextNetwork->setServer( false );
            }
            const auto directory = ( fs::u8path( projectDirectory.c_str() ) / ".workphone/services" ).u8string();
            if( localProviderRoot != directory.c_str() )
            {
                localProviders = makeLocalProjectServiceProviders( directory.c_str() );
                localProviderRoot = directory.c_str();
            }
            auto providers = localProviders;
            if( providerOverrides.cloud ) providers.cloud = providerOverrides.cloud;
            if( providerOverrides.analytics ) providers.analytics = providerOverrides.analytics;
            if( providerOverrides.crash ) providers.crash = providerOverrides.crash;
            const auto previousServices = std::atomic_load( &state );
            if( !configureProjectServices( std::move( providers ), settings->getPropertyAsBool( "services.cloudSave", previousServices->cloud ),
                    settings->getPropertyAsBool( "services.analytics", previousServices->analytics ),
                    settings->getPropertyAsBool( "services.crashReporting", previousServices->crash ), error ) ) return false;
            const bool configureScripts = settings->hasProperty( "scripting.backend" ) || settings->hasProperty( "scripting.lua" ) ||
                settings->hasProperty( "scripting.hotReload" ) || settings->hasProperty( "scripting.reflection" );
            if( app && configureScripts && !applyProjectScriptSettings( settings, projectDirectory, error ) )
            {
                String rollbackError;
                if( !configureProjectServices( previousServices->providers, previousServices->cloud,
                                               previousServices->analytics, previousServices->crash, rollbackError ) )
                    error += "; unable to restore previous service state: " + rollbackError;
                return false;
            }
            if( configureNetwork && app && !multiplayer )
            {
                auto previous = app->getNetworkManager(); app->setNetworkManager( nullptr );
                projectNetwork = nullptr; selectedNetworkBackend = -1;
                if( previous ) try { previous->unload( nullptr ); } catch( const std::exception &e ) { WP_LOG_EXCEPTION( e ); }
            }
            else if( app && nextNetwork )
            {
                auto previous = app->getNetworkManager(); app->setNetworkManager( nextNetwork );
                projectNetwork = nextNetwork; selectedNetworkBackend = networkBackend;
                pendingNetwork.instance = nullptr;
                if( previous && previous != projectNetwork ) try { previous->unload( nullptr ); } catch( const std::exception &e ) { WP_LOG_EXCEPTION( e ); }
            }
            error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    void releaseProjectServiceSettings()
    {
        std::lock_guard<std::mutex> lock( lifecycleMutex );
        String error; configureProjectServices( {}, false, false, false, error );
        auto app = core::IApplicationManager::instancePtr();
        if( app && app->getNetworkManager() == projectNetwork ) app->setNetworkManager( nullptr );
        if( projectNetwork ) projectNetwork->unload( nullptr );
        projectNetwork = nullptr; selectedNetworkBackend = -1;
        localProviders = {}; providerOverrides = {}; localProviderRoot.clear();
        releaseProjectScriptManager(); releaseProjectIrradiance(); releaseProjectAssetEncoders();
        releaseProjectGraphicsProviders();
    }

    bool putProjectCloudSave( const String &slot, const Array<u8> &bytes, String &error )
    { auto s = std::atomic_load( &state ); if( !s->cloud ) { error = "Cloud saves are disabled"; return false; } return s->providers.cloud->put( slot, bytes, error ); }
    bool getProjectCloudSave( const String &slot, Array<u8> &bytes, String &error )
    { auto s = std::atomic_load( &state ); if( !s->cloud ) { error = "Cloud saves are disabled"; return false; } return s->providers.cloud->get( slot, bytes, error ); }
    bool removeProjectCloudSave( const String &slot, String &error )
    { auto s = std::atomic_load( &state ); if( !s->cloud ) { error = "Cloud saves are disabled"; return false; } return s->providers.cloud->remove( slot, error ); }
    bool listProjectCloudSaves( Array<String> &slots, String &error )
    { auto s = std::atomic_load( &state ); if( !s->cloud ) { error = "Cloud saves are disabled"; return false; } return s->providers.cloud->list( slots, error ); }
    bool recordProjectAnalytics( const String &event, SmartPtr<Properties> fields, String &error )
    { auto s = std::atomic_load( &state ); if( !s->analytics ) { error = "Analytics are disabled"; return false; } return s->providers.analytics->record( event, fields, error ); }
    bool captureProjectCrashReport( const String &reason, String &path, String &error )
    { auto s = std::atomic_load( &state ); if( !s->crash ) { error = "Crash reporting is disabled"; return false; } return s->providers.crash->capture( reason, path, error ); }
}
