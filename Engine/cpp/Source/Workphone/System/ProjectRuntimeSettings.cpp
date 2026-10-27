#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ProjectRuntimeSettings.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#if defined( _WIN32 )
#    include <windows.h>
#endif

namespace workphone
{
    namespace
    {
        SmartPtr<Properties> activeSettings()
        {
            auto app = core::IApplicationManager::instancePtr();
            return app ? app->getPlayerSettings() : nullptr;
        }
    }

    void loadProjectRuntimeSettings( const String &projectDirectory )
    {
        auto app = core::IApplicationManager::instancePtr();
        if( !app ) return;
        const auto path = std::filesystem::u8path( projectDirectory.c_str() ) / "ProjectRuntime.settings";
        if( !std::filesystem::exists( path ) ) return;
        const auto bytes = std::filesystem::file_size( path );
        if( bytes > 8 * 1024 * 1024 )
            throw std::runtime_error( "Project runtime settings exceed 8 MiB" );
        std::ifstream stream( path, std::ios::binary );
        if( !stream ) throw std::runtime_error( "Unable to read project runtime settings" );
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        const String text = buffer.str();
        if( stream.bad() || !DataUtil::isValidData( text ) )
            throw std::runtime_error( "Invalid project runtime settings JSON" );
        auto settings = workphone::make_ptr<Properties>();
        DataUtil::parse( text, settings.get() );
        app->setPlayerSettings( settings );
        app->setProjectPath( std::filesystem::absolute( path ).parent_path().u8string() );
    }

    u32 projectWorkerCount( u32 applicationDefault )
    {
        // Explicit single-thread applications/test fixtures retain that contract.
        if( applicationDefault == 0 ) return 0;
        auto settings = activeSettings();
        if( !settings ) return applicationDefault;
        if( !settings->getPropertyAsBool( "tasks.backgroundTasks", true ) ||
            settings->getPropertyAsInt( "tasks.jobSystem", 0 ) == 2 ) return 0;
        return static_cast<u32>( std::clamp(
            settings->getPropertyAsInt( "tasks.threadCount", static_cast<s32>( applicationDefault ) ),
            1, 32 ) );
    }

    bool projectRunsBackgroundTasks()
    {
        auto settings = activeSettings();
        return !settings || ( settings->getPropertyAsBool( "tasks.backgroundTasks", true ) &&
                              settings->getPropertyAsInt( "tasks.jobSystem", 0 ) != 2 );
    }

    bool projectLogEnabled( u32 severity )
    {
        auto settings = activeSettings();
        if( !settings ) return true;
        if( severity == 3 && !settings->getPropertyAsBool( "diagnostics.validationWarnings", true ) )
            return false;
        return severity >= static_cast<u32>( settings->getPropertyAsInt( "diagnostics.logLevel", 2 ) );
    }

    bool projectProfilingEnabled()
    {
        auto settings = activeSettings();
        return !settings || settings->getPropertyAsBool( "diagnostics.profiler", true );
    }

    void applyProjectWorkerPolicy( SmartPtr<Properties> settings, u32 workerIndex )
    {
        if( !settings ) return;
#if defined( _WIN32 )
        const s32 priority = std::clamp( settings->getPropertyAsInt( "tasks.priority", 5 ), 0, 10 );
        const int priorities[] = { THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL,
                                  THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL,
                                  THREAD_PRIORITY_HIGHEST };
        SetThreadPriority( GetCurrentThread(), priorities[priority < 3 ? 0 : priority < 5 ? 1 :
                                                        priority == 5 ? 2 : priority < 8 ? 3 : 4] );
        const auto affinity = settings->getPropertyAsInt( "tasks.workerAffinity", 0 );
        using GetSets = BOOL ( WINAPI * )( PSYSTEM_CPU_SET_INFORMATION, ULONG, PULONG, HANDLE, ULONG );
        using SelectSets = BOOL ( WINAPI * )( HANDLE, const ULONG *, ULONG );
        auto kernel = GetModuleHandleW( L"kernel32.dll" );
        auto getSets = reinterpret_cast<GetSets>( GetProcAddress( kernel, "GetSystemCpuSetInformation" ) );
        auto selectSets = reinterpret_cast<SelectSets>( GetProcAddress( kernel, "SetThreadSelectedCpuSets" ) );
        if( !getSets || !selectSets )
        {
            if( affinity == 3 )
                SetThreadAffinityMask( GetCurrentThread(), DWORD_PTR( 1 ) <<
                    ( workerIndex % std::min<u32>( 64, std::max<u32>( 1, Thread::hardware_concurrency() ) ) ) );
            return;
        }
        if( affinity == 0 ) { selectSets( GetCurrentThread(), nullptr, 0 ); return; }
        ULONG bytes = 0;
        getSets( nullptr, 0, &bytes, GetCurrentProcess(), 0 );
        if( !bytes || bytes > ( 1u << 20 ) ) return;
        std::vector<unsigned char> buffer( bytes );
        if( !getSets( reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>( buffer.data() ), bytes,
                      &bytes, GetCurrentProcess(), 0 ) ) return;
        std::vector<std::pair<ULONG, BYTE>> cores;
        for( ULONG offset = 0; offset + sizeof( ULONG ) * 2 <= bytes; )
        {
            auto info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>( buffer.data() + offset );
            if( info->Size == 0 || info->Size > bytes - offset ) break;
            if( info->Type == CpuSetInformation && !info->CpuSet.Parked &&
                ( !info->CpuSet.Allocated || info->CpuSet.AllocatedToTargetProcess ) )
                cores.emplace_back( info->CpuSet.Id, info->CpuSet.EfficiencyClass );
            offset += info->Size;
        }
        if( cores.empty() ) return;
        std::vector<ULONG> selected;
        if( affinity == 3 ) selected.push_back( cores[workerIndex % cores.size()].first );
        else
        {
            auto efficiency = cores.front().second;
            for( const auto &core : cores )
                efficiency = affinity == 1 ? std::max( efficiency, core.second ) :
                                             std::min( efficiency, core.second );
            for( const auto &core : cores )
                if( core.second == efficiency ) selected.push_back( core.first );
        }
        selectSets( GetCurrentThread(), selected.data(), static_cast<ULONG>( selected.size() ) );
#else
        (void)workerIndex;
#endif
    }
}
