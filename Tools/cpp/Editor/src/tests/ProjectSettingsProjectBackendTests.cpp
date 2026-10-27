#include <EditorPCH.hpp>
#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
#include <editor/ProjectSettingsProjectBackend.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ProjectServices.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace workphone;
using namespace workphone::editor;

namespace
{
    struct ProjectBackendTemporary
    {
        std::filesystem::path root = std::filesystem::temp_directory_path() /
            std::string( ( "workphone-project-backend-" + StringUtil::getUUID() ).c_str() );
        ProjectBackendTemporary() { std::filesystem::create_directories( root ); }
        ~ProjectBackendTemporary()
        {
            String error; configureProjectServices( {}, false, false, false, error );
            std::error_code ignored; std::filesystem::remove_all( root, ignored );
        }
    };
}

BOOST_AUTO_TEST_CASE( project_services_local_providers_test )
{
    ProjectBackendTemporary temporary;
    String error;
    auto providers = makeLocalProjectServiceProviders( temporary.root.u8string().c_str() );
    BOOST_REQUIRE_MESSAGE( configureProjectServices( providers, true, true, false, error ), error );
    const Array<u8> original = { 0, 127, 255, 0, 42 };
    BOOST_REQUIRE_MESSAGE( putProjectCloudSave( "slot-1", original, error ), error );
    Array<u8> restored;
    BOOST_REQUIRE_MESSAGE( getProjectCloudSave( "slot-1", restored, error ), error );
    BOOST_CHECK_EQUAL_COLLECTIONS( original.begin(), original.end(), restored.begin(), restored.end() );
    BOOST_CHECK( !putProjectCloudSave( "../escape", original, error ) );
    Array<String> slots;
    BOOST_REQUIRE_MESSAGE( listProjectCloudSaves( slots, error ), error );
    BOOST_REQUIRE_EQUAL( slots.size(), 1u ); BOOST_CHECK_EQUAL( slots[0], "slot-1" );
    BOOST_REQUIRE( putProjectCloudSave( "slot-1", {}, error ) );
    BOOST_REQUIRE( getProjectCloudSave( "slot-1", restored, error ) );
    BOOST_CHECK( restored.empty() );
    BOOST_REQUIRE( removeProjectCloudSave( "slot-1", error ) );
    BOOST_REQUIRE( listProjectCloudSaves( slots, error ) ); BOOST_CHECK( slots.empty() );

    auto fields = make_ptr<Properties>(); fields->setProperty( "detail", "escaped \"text\"\nline" );
    BOOST_REQUIRE_MESSAGE( recordProjectAnalytics( "first", fields, error ), error );
    BOOST_REQUIRE_MESSAGE( recordProjectAnalytics( "second", nullptr, error ), error );
    std::ifstream journal( temporary.root / "analytics/events.jsonl" );
    std::string line; size_t count = 0;
    while( std::getline( journal, line ) )
    {
        BOOST_CHECK( DataUtil::isValidData( line.c_str() ) ); ++count;
    }
    BOOST_CHECK_EQUAL( count, 2u );
    BOOST_REQUIRE( configureProjectServices( providers, false, false, false, error ) );
    BOOST_CHECK( !putProjectCloudSave( "disabled", original, error ) );
    BOOST_CHECK( !getProjectCloudSave( "slot-1", restored, error ) );
    BOOST_CHECK( !recordProjectAnalytics( "disabled", nullptr, error ) );
    String report; BOOST_CHECK( !captureProjectCrashReport( "disabled", report, error ) );
#if defined( _WIN32 )
    BOOST_REQUIRE( configureProjectServices( providers, false, false, true, error ) );
    BOOST_REQUIRE_MESSAGE( captureProjectCrashReport( "Controlled diagnostic capture", report, error ), error );
    std::ifstream dump( std::filesystem::u8path( report.c_str() ), std::ios::binary );
    char signature[4]{}; dump.read( signature, sizeof( signature ) );
    BOOST_CHECK_EQUAL( std::string( signature, 4 ), "MDMP" );
#endif
}

BOOST_AUTO_TEST_CASE( project_settings_build_profile_test )
{
    ProjectBackendTemporary temporary;
    auto project = make_ptr<Project>(); project->create( ( temporary.root / "Project" ).u8string().c_str() );
    auto settings = make_ptr<Properties>();
    settings->setProperty( "build.config", s32( 2 ) );
    settings->setProperty( "build.assetBundles", false );
    settings->setProperty( "build.debugSymbols", false );
    settings->setProperty( "build.stripSymbols", true );
    settings->setProperty( "project.defaultLanguage", s32( 3 ) );
    settings->setProperty( "project.productName", "Fixture Product" );
    const auto arguments = prepareProjectSettingsBuild( project, settings );
    BOOST_CHECK( !arguments.empty() );
    const auto output = temporary.root / "Project/Build/PC/x64/Release";
    BOOST_CHECK( std::filesystem::is_regular_file( output / "WorkphoneProjectProfile.cmake" ) );
    auto manifest = make_ptr<Properties>();
    std::ifstream stream( output / "ProjectPackage.json" ); std::ostringstream text; text << stream.rdbuf();
    DataUtil::parse( text.str().c_str(), manifest.get() );
    BOOST_CHECK_EQUAL( manifest->getProperty( "project" ), "Fixture Product" );
    BOOST_CHECK_EQUAL( manifest->getPropertyAsInt( "defaultLanguage" ), 3 );
    settings->setProperty( "network.multiplayer", true );
    settings->setProperty( "network.backend", s32( 0 ) );
    BOOST_CHECK( !validateProjectSettingsProjectBackend( settings )->getPropertyAsBool( "success" ) );
}
#endif
