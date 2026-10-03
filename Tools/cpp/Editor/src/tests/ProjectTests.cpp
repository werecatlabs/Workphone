#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>

#include <editor/Project.hpp>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>
#    include <filesystem>
#    include <fstream>
#    include <stdexcept>
#    include <Workphone/System/ApplicationManager.hpp>
#    include <Workphone/System/PluginManager.hpp>

using namespace workphone;
using namespace workphone::editor;

BOOST_AUTO_TEST_CASE( project_metadata_test )
{
    Project project;

    const String label = "Test Project";
    const String projectPath = "Projects/TestProject";
    const String workingDirectory = "Projects/TestProject/Build";
    const String applicationFilePath = "Projects/TestProject/Build/TestProject.exe";
    const String selectedProjectPath = "Projects/TestProject/project.fbproject";
    const String currentScenePath = "Assets/Scenes/Main.scene";

    project.setLabel( label );
    project.setPath( projectPath );
    project.setWorkingDirectory( workingDirectory );
    project.setApplicationFilePath( applicationFilePath );
    project.setSelectedProjectPath( selectedProjectPath );
    project.setCurrentScenePath( currentScenePath );

    BOOST_CHECK_EQUAL( project.getLabel(), label );
    BOOST_CHECK_EQUAL( project.getPath(), projectPath );
    BOOST_CHECK_EQUAL( project.getWorkingDirectory(), workingDirectory );
    BOOST_CHECK_EQUAL( project.getApplicationFilePath(), applicationFilePath );
    BOOST_CHECK_EQUAL( project.getSelectedProjectPath(), selectedProjectPath );
    BOOST_CHECK_EQUAL( project.getCurrentScenePath(), currentScenePath );
}

BOOST_AUTO_TEST_CASE( project_paths_and_build_settings_test )
{
    Project project;

    Array<String> paths = { "Assets", "Scripts" };
    project.setPaths( paths );
    project.addPath( "Plugins" );

    Array<String> mediaPaths = { "Media", "SharedMedia" };
    project.setMediaPaths( mediaPaths );

    Array<String> scriptFilePaths = { "Scripts/Main.lua", "Scripts/Helpers.lua" };
    project.setScriptFilePaths( scriptFilePaths );

    Array<String> resourceFolders = { "Assets", "Media" };
    project.setResourceFolders( resourceFolders );

    project.setApplicationType( "Desktop" );
    project.setArchive( true );
    project.setDirty( true );

    auto actualPaths = project.getPaths();
    actualPaths.push_back( "LocalOnly" );
    BOOST_CHECK_EQUAL( project.getPaths().size(), 3u );
    BOOST_CHECK_EQUAL( project.getPaths()[0], "Assets" );
    BOOST_CHECK_EQUAL( project.getPaths()[2], "Plugins" );

    BOOST_CHECK_EQUAL( project.getMediaPaths().size(), mediaPaths.size() );
    BOOST_CHECK_EQUAL( project.getMediaPaths()[1], "SharedMedia" );
    BOOST_CHECK_EQUAL( project.getScriptFilePaths().size(), scriptFilePaths.size() );
    BOOST_CHECK_EQUAL( project.getScriptFilePaths()[0], "Scripts/Main.lua" );
    BOOST_CHECK_EQUAL( project.getResourceFolders().size(), resourceFolders.size() );
    BOOST_CHECK_EQUAL( project.getResourceFolders()[1], "Media" );
    BOOST_CHECK_EQUAL( project.getApplicationType(), "Desktop" );
    BOOST_CHECK( project.isArchive() );
    BOOST_CHECK( project.isDirty() );

    project.setArchive( false );
    project.setDirty( false );
    BOOST_CHECK( !project.isArchive() );
    BOOST_CHECK( !project.isDirty() );
}

BOOST_AUTO_TEST_CASE( project_default_data_test )
{
    Project project;
    auto properties = workphone::static_pointer_cast<Properties>( project.getDefaultData() );

    BOOST_REQUIRE( properties );

    String projectVersion;
    String uuid;
    String productName;
    String companyName;
    String currentScenePath;

    BOOST_REQUIRE( properties->getPropertyValue( "projectVersion", projectVersion ) );
    BOOST_REQUIRE( properties->getPropertyValue( "uuid", uuid ) );
    BOOST_REQUIRE( properties->getPropertyValue( "productName", productName ) );
    BOOST_REQUIRE( properties->getPropertyValue( "companyName", companyName ) );
    BOOST_REQUIRE( properties->getPropertyValue( "currentScenePath", currentScenePath ) );

    BOOST_CHECK_EQUAL( projectVersion, "1.0.0" );
    BOOST_CHECK( !StringUtil::isNullOrEmpty( uuid ) );
    BOOST_CHECK_EQUAL( productName, "Untitled" );
    BOOST_CHECK_EQUAL( companyName, "Untitled" );
    BOOST_CHECK( StringUtil::isNullOrEmpty( currentScenePath ) );
}

namespace
{
    struct ProjectTestDirectory
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            std::string( ( "lioncat-project-" + StringUtil::getUUID() ).c_str() );

        ProjectTestDirectory() { std::filesystem::create_directories( path ); }
        ~ProjectTestDirectory()
        {
            std::error_code ignored;
            std::filesystem::remove_all( path, ignored );
        }
    };
}

BOOST_AUTO_TEST_CASE( project_serialization_roundtrip_test )
{
    Project source;
    source.setLabel( "Production Project" );
    source.setApplicationType( "Tool" );
    source.setCurrentScenePath( "Assets/Scenes/Intro.scene" );
    source.setWorkingDirectory( "Build" );
    source.setApplicationFilePath( "Build/Tool.exe" );
    source.setPaths( { "Shared;Art", "Scripts" } );
    source.setMediaPaths( { "Art", "Audio" } );
    source.setResourceFolders( { "Assets" } );
    source.setScriptFilePaths( { "Scripts/Main.lua" } );
    auto properties = source.getProperties();
    properties->setProperty( "companyName", "Studio" );
    properties->setProperty( "productName", "Tool" );
    properties->setProperty( "projectVersion", "2.3.4" );
    properties->setProperty( "customSetting", "keep me" );
    auto custom = workphone::make_ptr<Properties>();
    custom->setName( "customGroup" );
    custom->setProperty( "setting", "original" );
    properties->addChild( custom );
    source.setProperties( properties );
    custom->setProperty( "setting", "changed externally" );
    BOOST_CHECK_EQUAL( source.getProperties()->getChild( "customGroup" )->getProperty( "setting" ),
                       "original" );
    auto uuid = source.getProperties()->getProperty( "uuid" );

    // Exercise the actual JSON representation, not just an in-memory copy.
    auto serialized = DataUtil::toString( source.getProperties().get(), true );
    auto parsed = workphone::make_ptr<Properties>();
    DataUtil::parse( serialized, parsed.get() );
    Project restored;
    restored.fromData( parsed );
    BOOST_CHECK_EQUAL( restored.getLabel(), "Production Project" );
    BOOST_CHECK_EQUAL( restored.getApplicationType(), "Tool" );
    BOOST_CHECK_EQUAL( restored.getCurrentScenePath(), "Assets/Scenes/Intro.scene" );
    BOOST_CHECK_EQUAL( restored.getWorkingDirectory(), "Build" );
    BOOST_CHECK_EQUAL( restored.getApplicationFilePath(), "Build/Tool.exe" );
    BOOST_REQUIRE_EQUAL( restored.getPaths().size(), 2u );
    BOOST_CHECK_EQUAL( restored.getPaths()[0], "Shared;Art" );
    BOOST_REQUIRE_EQUAL( restored.getMediaPaths().size(), 2u );
    BOOST_CHECK_EQUAL( restored.getMediaPaths()[1], "Audio" );
    BOOST_REQUIRE_EQUAL( restored.getScriptFilePaths().size(), 1u );
    BOOST_CHECK_EQUAL( restored.getScriptFilePaths()[0], "Scripts/Main.lua" );
    BOOST_REQUIRE_EQUAL( restored.getResourceFolders().size(), 1u );
    auto result = restored.getProperties();
    BOOST_CHECK_EQUAL( result->getProperty( "uuid" ), uuid );
    BOOST_CHECK_EQUAL( result->getProperty( "projectVersion" ), "2.3.4" );
    BOOST_CHECK_EQUAL( result->getProperty( "companyName" ), "Studio" );
    BOOST_CHECK_EQUAL( result->getProperty( "productName" ), "Tool" );
    BOOST_CHECK_EQUAL( result->getProperty( "customSetting" ), "keep me" );
    BOOST_CHECK( !restored.isDirty() );
    result->setProperty( "label", "External mutation" );
    BOOST_CHECK_EQUAL( restored.getLabel(), "Production Project" );
    result->getChild( "customGroup" )->setProperty( "setting", "snapshot mutation" );
    BOOST_CHECK_EQUAL( restored.getProperties()->getChild( "customGroup" )->getProperty( "setting" ),
                       "original" );
}

BOOST_AUTO_TEST_CASE( project_legacy_defaults_and_dirty_test )
{
    Project project;
    project.setPaths( { "Previous" } );
    project.setApplicationType( "Previous" );
    auto legacy = workphone::make_ptr<Properties>();
    legacy->setProperty( "version", "0.9.0" );
    legacy->setProperty( "uuid", "legacy-id" );
    legacy->setProperty( "mediaPaths", Array<String>{ "LegacyMedia" } );
    project.fromData( legacy );
    BOOST_CHECK_EQUAL( project.getProperties()->getProperty( "projectVersion" ), "0.9.0" );
    BOOST_CHECK_EQUAL( project.getProperties()->getProperty( "uuid" ), "legacy-id" );
    BOOST_CHECK( project.getPaths().empty() );
    BOOST_CHECK_EQUAL( project.getApplicationType(), "Desktop" );
    BOOST_REQUIRE_EQUAL( project.getMediaPaths().size(), 1u );
    BOOST_CHECK_EQUAL( project.getMediaPaths()[0], "LegacyMedia" );
    BOOST_CHECK( !project.isDirty() );
    project.setLabel( project.getLabel() );
    BOOST_CHECK( !project.isDirty() );
    project.addPath( "Assets" );
    BOOST_CHECK( project.isDirty() );
    project.setDirty( false );
    project.addPath( "Assets" );
    project.addPath( "" );
    BOOST_CHECK( !project.isDirty() );
    BOOST_REQUIRE_EQUAL( project.getPaths().size(), 1u );
    BOOST_CHECK_THROW( project.fromData( nullptr ), std::invalid_argument );
    BOOST_CHECK_THROW( project.setProperties( nullptr ), std::invalid_argument );
    BOOST_CHECK_EQUAL( project.getPaths()[0], "Assets" );
}

BOOST_AUTO_TEST_CASE( project_file_lifecycle_test )
{
    ProjectTestDirectory temporary;
    const auto directory = temporary.path / "Project with spaces";
    Project project;
    project.create( directory.u8string() );
    BOOST_CHECK( !project.isDirty() );
    BOOST_CHECK( std::filesystem::exists( directory / "Assets" ) );
    BOOST_CHECK( std::filesystem::exists( directory / "Plugin/Plugin.hpp" ) );
    BOOST_CHECK( std::filesystem::exists( directory / "Plugin/Plugin.cpp" ) );
    project.setLabel( "Saved Project" );
    project.setMediaPaths( { "Custom Media" } );
    project.save();
    BOOST_CHECK( !project.isDirty() );
    Project loaded;
    loaded.loadFromFile( project.getFilePath() );
    BOOST_CHECK_EQUAL( loaded.getLabel(), "Saved Project" );
    BOOST_CHECK_EQUAL( loaded.getPath(), directory.u8string() );
    BOOST_CHECK_EQUAL( loaded.getFilePath(), project.getFilePath() );
    BOOST_REQUIRE_EQUAL( loaded.getMediaPaths().size(), 1u );
    BOOST_CHECK_EQUAL( loaded.getMediaPaths()[0], "Custom Media" );
    BOOST_CHECK( !loaded.isDirty() );
    auto uuid = loaded.getProperties()->getProperty( "uuid" );
    BOOST_CHECK_THROW( loaded.create( directory.u8string() ), std::runtime_error );
    BOOST_CHECK_EQUAL( loaded.getProperties()->getProperty( "uuid" ), uuid );

    const auto invalid = temporary.path / "invalid.fbproject";
    std::ofstream( invalid ) << "{}";
    BOOST_CHECK_THROW( loaded.loadFromFile( invalid.u8string() ), std::runtime_error );
    BOOST_CHECK_EQUAL( loaded.getFilePath(), project.getFilePath() );
    BOOST_CHECK_EQUAL( loaded.getLabel(), "Saved Project" );

    loaded.setLabel( "Unsaved" );
    BOOST_CHECK_THROW( loaded.saveToFile( directory.u8string() ), std::exception );
    BOOST_CHECK( loaded.isDirty() );
    BOOST_CHECK_EQUAL( loaded.getFilePath(), project.getFilePath() );
    for( const auto &entry : std::filesystem::directory_iterator( temporary.path ) )
        BOOST_CHECK( entry.path().extension() != ".tmp" );
    loaded.setArchive( true );
    BOOST_CHECK_THROW( loaded.save(), std::runtime_error );
    BOOST_CHECK_THROW( loaded.compile(), std::runtime_error );
}

BOOST_AUTO_TEST_CASE( project_owner_and_unload_test )
{
    Project project;
    auto owner = workphone::make_ptr<Properties>();
    project.setOwner( owner );
    BOOST_CHECK( project.getOwner() == owner );
    project.getOwner( nullptr );
    BOOST_CHECK( !project.getOwner() );
    project.setOwner( owner );
    owner = nullptr;
    BOOST_CHECK( !project.getOwner() );
    project.setLabel( "Retained" );
    project.load( nullptr );
    project.load( nullptr );
    BOOST_CHECK( project.isLoaded() );
    project.unload( nullptr );
    project.unload( nullptr );
    BOOST_CHECK_EQUAL( project.getLabel(), "Retained" );
    BOOST_CHECK_EQUAL( project.getProperties()->getProperty( "label" ), "Retained" );
}

#if defined(_WIN32)
BOOST_AUTO_TEST_CASE( project_compile_integration_test )
{
    ProjectTestDirectory temporary;
    const auto directory = temporary.path / "Build Project with spaces";
    Project project;
    project.create( directory.u8string() );
    auto root = std::filesystem::path( __FILE__ );
    for( int i = 0; i < 6; ++i ) root = root.parent_path();
    std::ofstream cmake( directory / "CMakeLists.txt" );
    cmake << "cmake_minimum_required(VERSION 3.20)\n"
          << "project(ProjectPlugin LANGUAGES CXX)\n"
          << "add_library(Plugin SHARED Plugin/Plugin.cpp)\n"
          << "target_compile_features(Plugin PRIVATE cxx_std_17)\n"
          << "target_include_directories(Plugin PRIVATE \""
          << ( root / "Engine/cpp/Include" ).generic_string() << "\")\n";
    cmake.close();
    struct ApplicationGuard
    {
        SmartPtr<core::IApplicationManager> previous = core::IApplicationManager::instance();
        SmartPtr<core::ApplicationManager> app = workphone::make_ptr<core::ApplicationManager>();
        ApplicationGuard()
        {
            core::IApplicationManager::setInstance( app );
            app->setPluginManager( workphone::make_ptr<core::PluginManager>() );
        }
        ~ApplicationGuard()
        {
            app->getPluginManager()->unload( nullptr );
            core::IApplicationManager::setInstance( previous );
        }
    } guard;
    guard.app->setProjectPath( directory.u8string() );
    const auto originalDirectory = std::filesystem::current_path();
    project.compile();
    BOOST_REQUIRE( project.getPlugin() );
    BOOST_CHECK( project.getPlugin()->getLibraryHandle() );
    BOOST_CHECK( project.getPlugin()->getFunction( "workphone_get_version" ) );
    BOOST_CHECK( project.getPlugin()->getFunction( "loadPlugin" ) );
    BOOST_CHECK( std::filesystem::current_path() == originalDirectory );
    // A second build verifies that the old DLL is released and reloaded.
    project.compile();
    BOOST_REQUIRE( project.getPlugin() );
    BOOST_CHECK( project.getPlugin()->getLibraryHandle() );
    BOOST_CHECK( std::filesystem::is_regular_file( directory / "Cache/project/build.log" ) );
    std::ofstream( directory / "Plugin/Plugin.cpp" ) << "#error Deliberate build failure\n";
    BOOST_CHECK_THROW( project.compile(), std::runtime_error );
    BOOST_REQUIRE( project.getPlugin() );
    BOOST_CHECK( project.getPlugin()->getLibraryHandle() );
    BOOST_CHECK( std::filesystem::current_path() == originalDirectory );
    std::ofstream( directory / "CMakeLists.txt" ) << "invalid cmake syntax (\n";
    auto previousPlugin = project.getPlugin();
    BOOST_CHECK_THROW( project.compile(), std::runtime_error );
    BOOST_CHECK( project.getPlugin() == previousPlugin );
    project.unload( nullptr );
    BOOST_CHECK( !project.getPlugin() );
}
#endif

#endif
