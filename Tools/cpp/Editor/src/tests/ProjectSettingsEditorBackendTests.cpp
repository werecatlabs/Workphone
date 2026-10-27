#include <EditorPCH.hpp>
#include <editor/ProjectSettingsEditorBackend.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/ProjectRuntimeSettings.hpp>
#include <Workphone/System/Profile.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
using namespace workphone;
using namespace workphone::editor;

BOOST_AUTO_TEST_CASE( project_settings_editor_policy_recovery_test )
{
    struct Guard
    {
        SmartPtr<core::IApplicationManager> previousApp = core::IApplicationManager::instance();
        SmartPtr<EditorManager> previousEditor = EditorManager::getSingleton();
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<EditorManager> editor = make_ptr<EditorManager>();
        std::filesystem::path directory = std::filesystem::temp_directory_path() /
            std::string( ( "workphone-settings-recovery-" + StringUtil::getUUID() ).c_str() );
        Guard()
        {
            core::IApplicationManager::setInstance( app );
            EditorManager::setSingletonPtr( editor );
            std::filesystem::create_directories( directory );
            directory = std::filesystem::canonical( directory );
        }
        ~Guard()
        {
            editor->setProject( nullptr );
            core::IApplicationManager::setInstance( previousApp );
            EditorManager::setSingletonPtr( previousEditor );
            std::error_code ignored;
            // Only the explicitly created and canonicalized test directory is removed.
            if( directory.filename().u8string().find( "workphone-settings-recovery-" ) == 0 )
                std::filesystem::remove_all( directory, ignored );
        }
    } guard;
    auto project = make_ptr<Project>();
    project->create( guard.directory.u8string() );
    guard.editor->setProject( project );
    auto settings = project->getProperties();
    settings->setProperty( "tasks.threadCount", 3 );
    settings->setProperty( "tasks.jobSystem", 0 );
    settings->setProperty( "tasks.backgroundTasks", true );
    settings->setProperty( "diagnostics.logLevel", 3 );
    settings->setProperty( "diagnostics.validationWarnings", false );
    settings->setProperty( "diagnostics.profiler", false );
    settings->setProperty( "diagnostics.telemetry", true );
    settings->setProperty( "editor.autosave", true );
    settings->setProperty( "editor.autosaveInterval", 1 );
    project->setProperties( settings );
    project->save();
    BOOST_REQUIRE( std::filesystem::is_regular_file( guard.directory / "ProjectRuntime.settings" ) );
    guard.app->setPlayerSettings( nullptr );
    loadProjectRuntimeSettings( guard.directory.u8string() );
    auto active = guard.app->getPlayerSettings();
    BOOST_REQUIRE( active );
    BOOST_CHECK_EQUAL( projectWorkerCount( 8 ), 3u );
    BOOST_CHECK_EQUAL( projectWorkerCount( 0 ), 0u );
    BOOST_CHECK( projectRunsBackgroundTasks() );
    BOOST_CHECK( !projectLogEnabled( 2 ) );
    BOOST_CHECK( !projectLogEnabled( 3 ) );
    BOOST_CHECK( projectLogEnabled( 4 ) );
    auto profile = make_ptr<Profile>();
    profile->setTimeTaken( 1.0f );
    BOOST_CHECK_EQUAL( profile->getAverageTimeTaken(), 0.0 );

    LogManagerDefault log;
    const auto logPath = guard.directory / "filter.log";
    log.open( logPath.u8string() );
    log.setEnableQueue( false );
    log.logMessage( String( "must not log info" ), ILogManager::Type::Info );
    log.logMessage( String( "must not log warning" ), ILogManager::Type::Warning );
    log.logMessage( String( "expected error" ), ILogManager::Type::Error );
    log.close();
    std::ifstream logStream( logPath );
    std::ostringstream logContents;
    logContents << logStream.rdbuf();
    BOOST_CHECK( logContents.str().find( "expected error" ) != std::string::npos );
    BOOST_CHECK( logContents.str().find( "must not" ) == std::string::npos );

    project->setLabel( "Recovery draft" );
    updateProjectSettingsEditorBackend( 0 );
    updateProjectSettingsEditorBackend( 2 );
    const auto telemetry = guard.directory / "Cache/Diagnostics/telemetry.csv";
    BOOST_REQUIRE( std::filesystem::is_regular_file( telemetry ) );
    updateProjectSettingsEditorBackend( 61 );
    const auto recoveryPath = guard.directory / "project.autosave.fbproject";
    BOOST_REQUIRE( std::filesystem::is_regular_file( recoveryPath ) );
    BOOST_CHECK( project->isDirty() );
    Project saved;
    saved.loadFromFile( project->getFilePath() );
    BOOST_CHECK_NE( saved.getLabel(), "Recovery draft" );
    Project recovered;
    recovered.loadFromFile( recoveryPath.u8string() );
    BOOST_CHECK_EQUAL( recovered.getLabel(), "Recovery draft" );

    auto disabled = project->getProperties();
    disabled->setProperty( "editor.autosave", false );
    disabled->setProperty( "editor.autosaveInterval", 1 );
    disabled->setProperty( "diagnostics.telemetry", false );
    disabled->setProperty( "tasks.backgroundTasks", false );
    guard.app->setPlayerSettings( disabled );
    BOOST_CHECK( !projectRunsBackgroundTasks() );
    BOOST_CHECK_EQUAL( projectWorkerCount( 8 ), 0u );
    const auto telemetryBytes = std::filesystem::file_size( telemetry );
    project->setLabel( "Autosave disabled" );
    updateProjectSettingsEditorBackend( 122 );
    BOOST_CHECK_EQUAL( std::filesystem::file_size( telemetry ), telemetryBytes );
    Project unchanged;
    unchanged.loadFromFile( recoveryPath.u8string() );
    BOOST_CHECK_EQUAL( unchanged.getLabel(), "Recovery draft" );
}
#endif
