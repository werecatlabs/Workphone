#include <EditorPCH.hpp>
#include <editor/ProjectSettingsEditorBackend.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
#include <ui/ProfilerWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ProjectRuntimeSettings.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>
#if defined( _WIN32 )
#    include <windows.h>
#endif

namespace workphone::editor
{
    namespace
    {
        const char *keys[] = { "tasks.threadCount", "tasks.backgroundTasks", "tasks.priority",
            "tasks.jobSystem", "tasks.workerAffinity", "tasks.renderThread", "diagnostics.logLevel",
            "diagnostics.profiler", "diagnostics.validationWarnings", "diagnostics.telemetry",
            "editor.theme", "editor.autosave", "editor.autosaveInterval", "editor.advancedWarnings" };

        void writeRecovery( const std::filesystem::path &destination, const String &text )
        {
            std::filesystem::create_directories( destination.parent_path() );
            auto temporary = destination;
            temporary += std::string( ( "." + StringUtil::getUUID() + ".tmp" ).c_str() );
            try
            {
                std::ofstream stream( temporary, std::ios::binary | std::ios::trunc );
                stream.exceptions( std::ios::badbit | std::ios::failbit );
                stream.write( text.data(), static_cast<std::streamsize>( text.size() ) );
                stream.close();
#if defined( _WIN32 )
                if( !MoveFileExW( temporary.c_str(), destination.c_str(),
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                    throw std::runtime_error( "Unable to publish autosave recovery" );
#else
                std::filesystem::rename( temporary, destination );
#endif
            }
            catch( ... )
            {
                std::error_code ignored;
                std::filesystem::remove( temporary, ignored );
                throw;
            }
        }
    }

    void describeProjectSettingsEditorBackend( SmartPtr<Properties> response )
    {
        for( auto key : keys ) response->setProperty( String( "capability." ) + key, true );
        response->setProperty( "availability.tasks.threadCount",
            "Reductions affect active workers immediately. Pool growth is applied at the next launch." );
        response->setProperty( "availability.tasks.jobSystem",
            "Engine threads, Windows platform workers, single-thread execution or a configured external pool." );
        response->setProperty( "availability.editor.autosave",
            "Writes project.autosave.fbproject and scene recovery data without overwriting your source files." );
    }

    SmartPtr<Properties> validateProjectSettingsEditorBackend( SmartPtr<Properties> settings )
    {
        auto response = make_ptr<Properties>();
        response->setProperty( "success", true );
        if( !settings )
        {
            response->setProperty( "success", false );
            response->setProperty( "status", "Editor settings are missing." );
            return response;
        }
        if( settings->getPropertyAsInt( "tasks.jobSystem", 0 ) == 3 )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto factories = app ? app->getFactoryManager() : nullptr;
            const auto provider = settings->getProperty( "tasks.providerType" );
            const auto factory = !provider.empty() && factories ? factories->getFactoryByName( provider ) : nullptr;
            if( !factory || !factory->isObjectDerivedFromByInfo( IThreadPool::typeInfo() ) )
            {
                response->setProperty( "success", false );
                response->setProperty( "status", "External jobs require a registered IThreadPool factory in tasks.providerType." );
            }
        }
#if !defined( _WIN32 )
        if( settings->getPropertyAsInt( "tasks.jobSystem", 0 ) == 1 ||
            settings->getPropertyAsInt( "tasks.workerAffinity", 0 ) != 0 )
        {
            response->setProperty( "success", false );
            response->setProperty( "status", "Platform jobs and CPU class affinity currently require Windows." );
        }
#endif
        return response;
    }

    SmartPtr<Properties> applyProjectSettingsEditorBackend( SmartPtr<Properties> settings )
    {
        auto response = validateProjectSettingsEditorBackend( settings );
        if( !response->getPropertyAsBool( "success" ) ) return response;
        String handled;
        for( auto key : keys ) if( settings->hasProperty( key ) )
        { if( !handled.empty() ) handled += "; "; handled += key; }
        response->setProperty( "handledKeys", handled );
        response->setProperty( "appliedKeys", handled );
        auto app = core::IApplicationManager::instancePtr();
        if( !app ) return response;
        if( auto tasks = app->getTaskManager() )
        {
            if( !settings->getPropertyAsBool( "diagnostics.profiler", true ) )
                for( auto task : tasks->getTasks() )
                    if( task && task->getProfile() ) task->getProfile()->clear();
        }
        if( auto editor = EditorManager::getSingletonPtr() )
            if( auto ui = editor->getUI() )
                if( auto profiler = ui->getProfilerWindow() )
                    if( !settings->getPropertyAsBool( "diagnostics.profiler", true ) )
                        profiler->setWindowVisible( false );
        String pending;
        if( settings->hasProperty( "tasks.jobSystem" ) ) pending = "tasks.jobSystem";
        if( auto pool = app->getThreadPool() )
        {
            if( settings->getPropertyAsBool( "tasks.backgroundTasks", true ) &&
                settings->getPropertyAsInt( "tasks.threadCount", 4 ) > static_cast<s32>( pool->getNumThreads() ) )
            {
                if( !pending.empty() ) pending += "; ";
                pending += "tasks.threadCount";
            }
        }
        response->setProperty( "pendingKeys", pending );
        response->setProperty( "status", "Editor policies applied. Worker pool growth and pool provider changes apply on next launch." );
        return response;
    }

    void updateProjectSettingsEditorBackend( f64 timeSeconds )
    {
        if( !std::isfinite( timeSeconds ) ) return;
        auto app = core::IApplicationManager::instancePtr();
        auto editor = EditorManager::getSingletonPtr();
        auto project = editor ? editor->getProject() : nullptr;
        auto settings = app ? app->getPlayerSettings() : nullptr;
        if( !project || !settings || project->getPath().empty() ) return;
        struct Schedule
        {
            const void *owner = nullptr;
            String path;
            f64 lastTime = -1;
            f64 nextSave = 0;
            s32 interval = 0;
        };
        static thread_local Schedule schedule;
        const auto interval = std::clamp( settings->getPropertyAsInt( "editor.autosaveInterval", 10 ), 1, 60 );
        if( schedule.owner != app || schedule.path != project->getPath() || timeSeconds < schedule.lastTime ||
            interval != schedule.interval )
        {
            schedule.owner = app;
            schedule.path = project->getPath();
            schedule.interval = interval;
            schedule.nextSave = timeSeconds + interval * 60;
        }
        schedule.lastTime = timeSeconds;
        const auto root = std::filesystem::u8path( project->getPath().c_str() );
        updateProjectRuntimeDiagnostics( timeSeconds, project->getPath() );
        if( !settings->getPropertyAsBool( "editor.autosave", true ) ||
            timeSeconds < schedule.nextSave || app->isPlaying() ) return;
        schedule.nextSave = timeSeconds + interval * 60;
        try
        {
            auto recovery = project->getProperties();
            recovery->setProperty( "recoverySourceProject", project->getFilePath() );
            auto scenes = app->getGameManager();
            auto scene = scenes ? scenes->getCurrentScene() : nullptr;
            if( scene )
            {
                const String scenePath = "Cache/Autosave/scene.fbscene";
                auto data = dynamic_pointer_cast<Properties>( scene->toData() );
                if( !data ) throw std::runtime_error( "Scene could not be serialized for autosave" );
                writeRecovery( root / std::filesystem::u8path( scenePath.c_str() ),
                               DataUtil::toString( data.get(), true ) );
                recovery->setProperty( "currentScenePath", scenePath );
                recovery->setProperty( "project.startupScene", scenePath );
                recovery->setProperty( "project.autoLoadLastScene", true );
            }
            writeRecovery( root / "project.autosave.fbproject", DataUtil::toString( recovery.get(), true ) );
        }
        catch( const std::exception &error ) { WP_LOG_EXCEPTION( error ); }
    }
}
