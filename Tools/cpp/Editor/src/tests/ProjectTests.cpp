#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>

#include <editor/Project.hpp>
#include <editor/ProjectSettingsActions.hpp>
#include <Workphone/System/ProjectServices.hpp>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>
#    include <filesystem>
#    include <fstream>
#    include <regex>
#    include <sstream>
#    include <stdexcept>
#    include <Workphone/System/ApplicationManager.hpp>
#    include <Workphone/System/PluginManager.hpp>
#    include <Workphone/Database/ResourceDatabase.hpp>
#    include <Workphone/Mesh/MeshSerializer.hpp>
#    include <Workphone/Scene/Directors/GraphicsSettingsDirector.hpp>
#    include <EditorApplication.hpp>
#    include <ui/SceneWindow.hpp>
#    include <ui/UIManager.hpp>

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
        std::filesystem::path path =
            std::filesystem::temp_directory_path() /
            std::string( ( "lioncat-project-" + StringUtil::getUUID() ).c_str() );

        ProjectTestDirectory()
        {
            std::filesystem::create_directories( path );
        }

        ~ProjectTestDirectory()
        {
            std::error_code ignored;
            std::filesystem::remove_all( path, ignored );
        }
    };

    struct ProjectSettingSchemaEntry
    {
        String name;
        String type;
    };

    Array<ProjectSettingSchemaEntry> readProjectSettingsUiSchema()
    {
        auto root = std::filesystem::path( __FILE__ );
        for( int i = 0; i < 6; ++i )
        {
            root = root.parent_path();
        }
        std::ifstream script( root / "bin/Media/Scripts/Lua/Editor/ProjectSettings.lua" );
        BOOST_REQUIRE_MESSAGE( script, "Cannot read the project settings UI schema" );
        std::ostringstream scriptBuffer;
        scriptBuffer << script.rdbuf();
        const auto source = scriptBuffer.str();
        const auto schemaBegin = source.find( "local ProjectSettingsStoreMap" );
        const auto schemaEnd = source.find( "local PlatformNames", schemaBegin );
        BOOST_REQUIRE( schemaBegin != std::string::npos );
        BOOST_REQUIRE( schemaEnd != std::string::npos );
        const auto schema = source.substr( schemaBegin, schemaEnd - schemaBegin );
        const std::regex pattern(
            R"wp(\w+\s*=\s*\{\s*name\s*=\s*"([^"]+)"\s*,\s*type\s*=\s*"([^"]+)"\s*\})wp" );
        Array<ProjectSettingSchemaEntry> definitions;
        for( auto entry = std::sregex_iterator( schema.begin(), schema.end(), pattern );
             entry != std::sregex_iterator(); ++entry )
        {
            definitions.push_back( { ( *entry )[1].str(), ( *entry )[2].str() } );
        }
        BOOST_REQUIRE_GE( definitions.size(), 120u );
        return definitions;
    }
}  // namespace

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

BOOST_AUTO_TEST_CASE( project_all_settings_file_roundtrip_test )
{
    // Read the actual UI schema so adding a control also extends persistence coverage.
    auto settings = workphone::make_ptr<Properties>();
    size_t settingCount = 0;
    for( const auto &definition : readProjectSettingsUiSchema() )
    {
        const auto &name = definition.name;
        const auto &type = definition.type;
        BOOST_REQUIRE_MESSAGE( !settings->hasProperty( name ), "Duplicate setting: " << name );
        if( type == "bool" )
        {
            settings->setProperty( name, settingCount % 2 == 0 );
        }
        else if( type == "int" )
        {
            settings->setProperty( name, static_cast<s32>( settingCount + 17 ) );
        }
        else if( type == "float" )
        {
            settings->setProperty( name, static_cast<f32>( settingCount ) + 0.375f );
        }
        else
        {
            BOOST_REQUIRE_EQUAL( type, "string" );
            settings->setProperty( name, String( "kept; \"escaped\"\n" ) + name );
        }
        ++settingCount;
    }
    settings->setProperty( "project.name", "Settings Roundtrip" );
    settings->setProperty( "project.company", String() );
    settings->setProperty( "project.productName", "Settings Product" );
    settings->setProperty( "project.version", "4.5.6" );
    settings->setProperty( "graphics.fullscreen", false );
    settings->setProperty( "sound.outputDevice", s32( 0 ) );
    settings->setProperty( "build.signingProfile", String() );

    ProjectTestDirectory temporary;
    Project project;
    project.create( ( temporary.path / "All settings" ).u8string() );
    auto properties = project.getProperties();
    const auto schemaVersion = properties->getProperty( "projectVersion" );
    const auto uuid = properties->getProperty( "uuid" );
    for( const auto &setting : settings->getPropertiesAsArray() )
    {
        properties->setProperty( setting );
    }
    properties->setProperty( "unrecognisedProjectSetting", "keep extension data" );
    auto extension = workphone::make_ptr<Properties>();
    extension->setName( "Custom Extension" );
    auto nested = workphone::make_ptr<Properties>();
    nested->setName( "Nested Settings" );
    nested->setProperty( "enabled", false );
    nested->setProperty( "empty", String() );
    nested->setProperty( "count", s32( 0 ) );
    extension->addChild( nested );
    properties->addChild( extension );
    project.setProperties( properties );
    BOOST_CHECK( project.isDirty() );
    project.save();
    BOOST_CHECK( !project.isDirty() );

    Project restored;
    restored.loadFromFile( project.getFilePath() );
    BOOST_CHECK( !restored.isDirty() );
    auto result = restored.getProperties();
    for( const auto &expected : settings->getPropertiesAsArray() )
    {
        BOOST_TEST_CONTEXT( "Project setting " << expected.getName() )
        {
            Property actual;
            BOOST_REQUIRE( result->getProperty( expected.getName(), actual ) );
            BOOST_CHECK_EQUAL( actual.getTypeName(), expected.getTypeName() );
            BOOST_CHECK_EQUAL( actual.getValue(), expected.getValue() );
        }
    }
    BOOST_CHECK_EQUAL( restored.getLabel(), "Settings Roundtrip" );
    BOOST_CHECK_EQUAL( result->getProperty( "companyName" ), "" );
    BOOST_CHECK_EQUAL( result->getProperty( "productName" ), "Settings Product" );
    BOOST_CHECK_EQUAL( result->getProperty( "projectVersion" ), schemaVersion );
    BOOST_CHECK_EQUAL( result->getProperty( "uuid" ), uuid );
    BOOST_CHECK_EQUAL( result->getProperty( "unrecognisedProjectSetting" ), "keep extension data" );
    auto restoredExtension = result->getChild( "Custom Extension" );
    BOOST_REQUIRE( restoredExtension );
    auto restoredNested = restoredExtension->getChild( "Nested Settings" );
    BOOST_REQUIRE( restoredNested );
    bool enabled = true;
    s32 count = -1;
    String empty = "not empty";
    BOOST_REQUIRE( restoredNested->getPropertyValue( "enabled", enabled ) );
    BOOST_REQUIRE( restoredNested->getPropertyValue( "count", count ) );
    BOOST_REQUIRE( restoredNested->getPropertyValue( "empty", empty ) );
    BOOST_CHECK( !enabled );
    BOOST_CHECK_EQUAL( count, 0 );
    BOOST_CHECK( empty.empty() );

    result->setProperty( "project.name", "Changed snapshot" );
    restoredNested->setProperty( "enabled", true );
    BOOST_CHECK_EQUAL( restored.getLabel(), "Settings Roundtrip" );
    BOOST_CHECK_EQUAL( restored.getProperties()->getProperty( "project.name" ), "Settings Roundtrip" );
    BOOST_CHECK_EQUAL( restored.getProperties()->getChild( "Custom Extension" )
                           ->getChild( "Nested Settings" )->getProperty( "enabled" ), "false" );
}

BOOST_AUTO_TEST_CASE( project_settings_actions_test )
{
    ProjectTestDirectory temporary;
    struct EditorGuard
    {
        SmartPtr<EditorManager> previous = EditorManager::getSingleton();
        SmartPtr<core::IApplicationManager> previousApp = core::IApplicationManager::instance();
        SmartPtr<EditorManager> editor = workphone::make_ptr<EditorManager>();
        SmartPtr<core::ApplicationManager> app = workphone::make_ptr<core::ApplicationManager>();

        EditorGuard()
        {
            EditorManager::setSingletonPtr( editor );
            core::IApplicationManager::setInstance( app );
        }

        ~EditorGuard()
        {
            releaseProjectServiceSettings();
            EditorManager::setSingletonPtr( previous );
            core::IApplicationManager::setInstance( previousApp );
        }
    } guard;
    auto project = workphone::make_ptr<Project>();
    project->create( ( temporary.path / "Command project" ).u8string() );
    guard.editor->setProject( project );
    const auto requestFor = []( const String &action ) {
        auto request = workphone::make_ptr<Properties>();
        request->setProperty( "action", action );
        return request;
    };
    const auto checkSuccess = []( SmartPtr<Properties> request ) {
        BOOST_REQUIRE_MESSAGE( request->getPropertyAsBool( "success" ),
                               request->getProperty( "status" ) );
    };

    auto read = requestFor( "readProjectSettings" );
    processProjectSettingsAction( read );
    checkSuccess( read );
    BOOST_CHECK_EQUAL( read->getProperty( "project.name" ), project->getLabel() );
    for( const auto &definition : readProjectSettingsUiSchema() )
    {
        BOOST_TEST_CONTEXT( "Native schema for " << definition.name )
        {
            Property setting;
            BOOST_REQUIRE( read->getProperty( definition.name, setting ) );
            const auto nativeType = definition.type == "int"     ? String( "s32" ) :
                                    definition.type == "float" ? String( "f32" ) : definition.type;
            BOOST_CHECK_EQUAL( setting.getTypeName(), nativeType );
            BOOST_CHECK( read->hasProperty( "capability." + definition.name ) );
        }
    }
    size_t capabilityCount = 0;
    for( const auto &property : read->getPropertiesAsArray() )
    {
        if( property.getName().find( "capability." ) == 0 )
        {
            ++capabilityCount;
        }
    }
    BOOST_CHECK_GT( capabilityCount, 0u );
    BOOST_CHECK( !project->isDirty() );

    auto apply = requestFor( "applyProjectSettings" );
    apply->setProperty( "project.name", "Applied via command" );
    apply->setProperty( "project.company", String() );
    apply->setProperty( "graphics.fullscreen", false );
    apply->setProperty( "graphics.renderScale", 0.75f );
    apply->setProperty( "quality.preset", s32( 0 ) );
    apply->setProperty( "graphics.postProcessing", true );
    apply->setProperty( "graphics.bloom", true );
    apply->setProperty( "graphics.shadowResolution", s32( 3 ) );
    apply->setProperty( "sound.outputDevice", s32( 0 ) );
    processProjectSettingsAction( apply );
    checkSuccess( apply );
    BOOST_CHECK_EQUAL( project->getLabel(), "Applied via command" );
    BOOST_CHECK( project->isDirty() );
    auto appliedGraphics = project->getGraphicsSettingsDirector()->getProperties();
    BOOST_CHECK_EQUAL( appliedGraphics->getPropertyAsInt( "Quality", -1 ), 0 );
    BOOST_CHECK( appliedGraphics->getPropertyAsBool( "Bloom" ) );
    BOOST_CHECK_EQUAL( appliedGraphics->getPropertyAsInt( "Shadow map size" ), 4096 );

    auto save = requestFor( "saveProjectSettings" );
    save->setProperty( "project.name", "Saved via command" );
    processProjectSettingsAction( save );
    checkSuccess( save );
    BOOST_CHECK( !project->isDirty() );
    Project saved;
    saved.loadFromFile( project->getFilePath() );
    BOOST_CHECK_EQUAL( saved.getLabel(), "Saved via command" );
    BOOST_CHECK_EQUAL( saved.getProperties()->getProperty( "project.company" ), "" );
    BOOST_CHECK( !saved.getProperties()->getPropertyAsBool( "graphics.fullscreen", true ) );
    BOOST_CHECK_EQUAL( saved.getProperties()->getPropertyAsInt( "sound.outputDevice", -1 ), 0 );
    BOOST_CHECK_CLOSE( saved.getProperties()->getPropertyAsFloat( "graphics.renderScale" ), 0.75f,
                       0.001f );
    auto savedGraphics = saved.getGraphicsSettingsDirector()->getProperties();
    BOOST_CHECK_EQUAL( savedGraphics->getPropertyAsInt( "Quality", -1 ), 0 );
    BOOST_CHECK( savedGraphics->getPropertyAsBool( "Bloom" ) );
    BOOST_CHECK_EQUAL( savedGraphics->getPropertyAsInt( "Shadow map size" ), 4096 );
    const auto unchangedProject = DataUtil::toString( project->getProperties().get(), true );

    auto validate = requestFor( "validateProjectSettings" );
    processProjectSettingsAction( validate );
    checkSuccess( validate );
    BOOST_CHECK_EQUAL( DataUtil::toString( project->getProperties().get(), true ), unchangedProject );
    BOOST_CHECK( !project->isDirty() );

    const auto exportedPath = temporary.path / "Exported settings.json";
    auto exportRequest = requestFor( "exportProjectSettingsJson" );
    exportRequest->setProperty( "path", exportedPath.u8string() );
    exportRequest->setProperty( "project.name", "Export Only" );
    exportRequest->setProperty( "build.signingProfile", String() );
    processProjectSettingsAction( exportRequest );
    checkSuccess( exportRequest );
    BOOST_REQUIRE( std::filesystem::is_regular_file( exportedPath ) );
    BOOST_CHECK_EQUAL( DataUtil::toString( project->getProperties().get(), true ), unchangedProject );
    BOOST_CHECK( !project->isDirty() );
    std::ifstream exportedStream( exportedPath );
    std::ostringstream exportedBuffer;
    exportedBuffer << exportedStream.rdbuf();
    auto exported = workphone::make_ptr<Properties>();
    DataUtil::parse( exportedBuffer.str(), exported.get() );
    BOOST_CHECK_EQUAL( exported->getPropertyAsInt( "projectSettingsVersion" ), 1 );
    BOOST_CHECK_EQUAL( exported->getProperty( "project.name" ), "Export Only" );
    BOOST_CHECK_EQUAL( exported->getProperty( "build.signingProfile" ), "" );

    auto importRequest = requestFor( "importProjectSettingsJson" );
    importRequest->setProperty( "path", exportedPath.u8string() );
    processProjectSettingsAction( importRequest );
    checkSuccess( importRequest );
    BOOST_CHECK_EQUAL( importRequest->getProperty( "project.name" ), "Export Only" );
    BOOST_CHECK_EQUAL( importRequest->getProperty( "build.signingProfile" ), "" );
    BOOST_CHECK_EQUAL( DataUtil::toString( project->getProperties().get(), true ), unchangedProject );
    BOOST_CHECK( !project->isDirty() );

    const auto invalidPath = temporary.path / "Invalid settings.json";
    for( const auto &document : {
             "{ malformed",
             "{\"projectSettingsVersion\":1,\"unknown.setting\":1}",
             "{\"projectSettingsVersion\":1,\"graphics.renderScale\":99}",
             "{\"projectSettingsVersion\":1,\"graphics.fullscreen\":\"yes\"}",
             "{\"projectSettingsVersion\":2,\"project.name\":\"Unsupported version\"}" } )
    {
        BOOST_TEST_CONTEXT( "Rejected import " << document )
        {
            std::ofstream( invalidPath ) << document;
            auto invalid = requestFor( "importProjectSettingsJson" );
            invalid->setProperty( "path", invalidPath.u8string() );
            processProjectSettingsAction( invalid );
            BOOST_CHECK( !invalid->getPropertyAsBool( "success", true ) );
            BOOST_CHECK( !invalid->getProperty( "status" ).empty() );
            BOOST_CHECK_EQUAL( DataUtil::toString( project->getProperties().get(), true ),
                               unchangedProject );
            BOOST_CHECK( !project->isDirty() );
        }
    }

    auto invalidApply = requestFor( "applyProjectSettings" );
    invalidApply->setProperty( "graphics.renderScale", 99.0f );
    processProjectSettingsAction( invalidApply );
    BOOST_CHECK( !invalidApply->getPropertyAsBool( "success", true ) );
    BOOST_CHECK_EQUAL( DataUtil::toString( project->getProperties().get(), true ), unchangedProject );
    BOOST_CHECK( !project->isDirty() );

    auto unavailableAA = requestFor( "applyProjectSettings" );
    unavailableAA->setProperty( "graphics.antiAliasing", s32( 6 ) );
    processProjectSettingsAction( unavailableAA );
    BOOST_CHECK( !unavailableAA->getPropertyAsBool( "success", true ) );
    BOOST_CHECK_EQUAL( DataUtil::toString( project->getProperties().get(), true ), unchangedProject );
    BOOST_CHECK( !project->isDirty() );

    auto invalidValidate = requestFor( "validateProjectSettings" );
    invalidValidate->setProperty( "build.android.minSdk", s32( 35 ) );
    invalidValidate->setProperty( "build.android.targetSdk", s32( 26 ) );
    processProjectSettingsAction( invalidValidate );
    BOOST_CHECK( !invalidValidate->getPropertyAsBool( "success", true ) );
    BOOST_CHECK_EQUAL( DataUtil::toString( project->getProperties().get(), true ), unchangedProject );
    BOOST_CHECK( !project->isDirty() );

    const auto projectFile = std::filesystem::u8path( project->getFilePath().c_str() );
    const auto projectDirectory = std::filesystem::weakly_canonical( projectFile.parent_path() );
    BOOST_REQUIRE( projectDirectory ==
                   std::filesystem::weakly_canonical( temporary.path / "Command project" ) );
    std::ifstream previousProjectStream( projectFile, std::ios::binary );
    BOOST_REQUIRE( previousProjectStream );
    std::ostringstream previousProjectBuffer;
    previousProjectBuffer << previousProjectStream.rdbuf();
    previousProjectStream.close();
    const auto previousProjectBytes = previousProjectBuffer.str();

    // A failed companion-file write must not publish a new primary project file.
    const auto graphicsFile = projectDirectory / "GameGraphics.settings";
    BOOST_REQUIRE( graphicsFile.parent_path() == projectDirectory );
    BOOST_REQUIRE( std::filesystem::remove( graphicsFile ) );
    BOOST_REQUIRE( std::filesystem::create_directory( graphicsFile ) );
    auto failedSave = requestFor( "saveProjectSettings" );
    failedSave->setProperty( "project.name", "Failed Save remains dirty" );
    processProjectSettingsAction( failedSave );
    BOOST_CHECK( !failedSave->getPropertyAsBool( "success", true ) );
    BOOST_CHECK( !failedSave->getProperty( "status" ).empty() );
    BOOST_CHECK( project->isDirty() );
    std::ifstream retainedProjectStream( projectFile, std::ios::binary );
    BOOST_REQUIRE( retainedProjectStream );
    std::ostringstream retainedProjectBuffer;
    retainedProjectBuffer << retainedProjectStream.rdbuf();
    BOOST_CHECK_EQUAL( retainedProjectBuffer.str(), previousProjectBytes );
    BOOST_REQUIRE( std::filesystem::remove( graphicsFile ) );
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
    {
        BOOST_CHECK( entry.path().extension() != ".tmp" );
    }
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

BOOST_AUTO_TEST_CASE( project_loaded_cache_paths_test )
{
    ProjectTestDirectory temporary;

    struct ApplicationGuard
    {
        SmartPtr<core::IApplicationManager> previous = core::IApplicationManager::instance();
        SmartPtr<core::ApplicationManager> app = workphone::make_ptr<core::ApplicationManager>();

        ApplicationGuard()
        {
            core::IApplicationManager::setInstance( app );
        }

        ~ApplicationGuard()
        {
            core::IApplicationManager::setInstance( previous );
        }
    } guard;

    const auto workingDirectory = std::filesystem::current_path();
    Project project;
    for( const auto &name : { "First Project", "Second Project" } )
    {
        const auto directory = temporary.path / name;
        std::filesystem::create_directories( directory );
        const auto file = directory / "project.fbproject";
        std::ofstream( file ) << "{\"projectVersion\":\"1.0.0\"}";
        // Resolve the selected file once, even when the selection is relative to the editor.
        auto selectedPath = std::filesystem::relative( file, workingDirectory );
        // Windows cannot express a C: temporary directory relative to a G: editor.
        if( selectedPath.empty() )
        {
            selectedPath = file;
        }
        project.loadFromFile( selectedPath.u8string() );
        BOOST_CHECK( std::filesystem::u8path( guard.app->getProjectPath().c_str() ) == directory );
        BOOST_CHECK_EQUAL( guard.app->getCachePath(), ( directory / "Cache" ).generic_u8string() + "/" );
        BOOST_CHECK_EQUAL( guard.app->getSettingsPath(),
                           ( directory / "SettingsCache" ).generic_u8string() + "/" );
        BOOST_CHECK( std::filesystem::is_directory( directory / "Cache" ) );
        BOOST_CHECK( std::filesystem::is_directory( directory / "SettingsCache" ) );
        const auto prefab = ApplicationUtil::getPrefabPath( "Assets/Triangle.obj" );
        BOOST_CHECK( std::filesystem::u8path( prefab.c_str() ).parent_path() == directory / "Cache" );
        const auto settings = guard.app->getSettingsPath() + "mesh.resourcedata";
        BOOST_CHECK( std::filesystem::u8path( settings.c_str() ).parent_path() ==
                     directory / "SettingsCache" );
        BOOST_CHECK( std::filesystem::current_path() == workingDirectory );
    }
    const auto previousCache = guard.app->getCachePath();
    const auto previousProject = guard.app->getProjectPath();
    const auto invalid = temporary.path / "invalid.fbproject";
    std::ofstream( invalid ) << "{}";
    BOOST_CHECK_THROW( project.loadFromFile( invalid.u8string() ), std::runtime_error );
    BOOST_CHECK_EQUAL( guard.app->getProjectPath(), previousProject );
    BOOST_CHECK_EQUAL( guard.app->getCachePath(), previousCache );
}

BOOST_AUTO_TEST_CASE( project_mesh_import_cache_test )
{
    ProjectTestDirectory temporary;
    const auto directory = temporary.path / "Mesh Project with spaces";
    Project project;
    project.create( directory.u8string() );
    project.setMediaPaths( {} );
    project.save();
    std::ofstream( directory / "Assets/Triangle.obj" )
        << "o Triangle\nv 0 0 0\nv 1 0 0\nv 0 1 0\n"
           "vt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\nf 1/1/1 2/2/1 3/3/1\n";
    const auto configuration = temporary.path / "plugins.cfg";
    std::ofstream plugins( configuration );
    plugins << "PluginFolder=.\n";
#    if WP_GRAPHICS_SYSTEM_OGRENEXT
    plugins << "Plugin=WPGraphicsOgreNext\n";
#    elif WP_GRAPHICS_SYSTEM_OGRE
    plugins << "Plugin=WPGraphicsOgre\n";
#    else
    plugins << "Plugin=WPGraphics\n";
#    endif
    plugins << "Plugin=WPSQLite\nPlugin=WPPhysics\nPlugin=WPAssimp\n";
    plugins.close();

    struct EditorGuard
    {
        EditorApplication app;

        ~EditorGuard()
        {
            if( auto manager = core::IApplicationManager::instance() )
            {
                manager->setQuit( true );
                manager->setRunning( false );
            }
            app.unload( nullptr );
        }
    } guard;

    //guard.app.pluginConfiguration = configuration.generic_u8string().c_str();
    guard.app.setDebugMode( true );
    guard.app.setActiveThreads( 0 );
    guard.app.load( nullptr );
    BOOST_REQUIRE( guard.app.isLoaded() );
    auto app = core::IApplicationManager::instance();
    BOOST_REQUIRE( app );
    auto editor = EditorManager::getSingletonPtr();
    BOOST_REQUIRE( editor );
    const auto workingDirectory = std::filesystem::current_path();
    editor->loadProject( project.getFilePath() );
    BOOST_CHECK_EQUAL( editor->getCachePath(), app->getCachePath() );
    BOOST_REQUIRE( app->getGraphicsSystem()->getMeshConverter() );
    BOOST_REQUIRE( app->getMeshLoader() );
    ResourceDatabase::ImportFileJob job;
    job.setFilePath( "Assets/Triangle.obj" );
    job.setOverwrite( true );
    job.execute();
    BOOST_CHECK( std::filesystem::current_path() == workingDirectory );
    const auto prefab = ApplicationUtil::getPrefabPath( "Assets/Triangle.obj" );
    BOOST_CHECK( std::filesystem::is_regular_file( std::filesystem::u8path( prefab.c_str() ) ) );
    const auto settings = app->getSettingsPath() +
                          StringUtil::toString( StringUtil::getUUID( "Assets/Triangle.obj" ) ) +
                          ".resourcedata";
    BOOST_CHECK( std::filesystem::is_regular_file( std::filesystem::u8path( settings.c_str() ) ) );
    size_t meshCount = 0;
    for( const auto &entry : std::filesystem::directory_iterator( directory / "Cache" ) )
    {
        if( entry.path().extension() != ".fbmeshbin" )
        {
            continue;
        }
        ++meshCount;
        BOOST_REQUIRE( std::filesystem::file_size( entry.path() ) > 0 );
        const auto relativePath =
            std::filesystem::relative( entry.path(), directory ).generic_u8string();
        auto stream = app->getFileSystem()->open( relativePath, true, true, false, false, false );
        BOOST_REQUIRE( stream );
        MeshSerializer serializer;
        auto mesh = serializer.loadMesh( stream );
        BOOST_REQUIRE( mesh );
        BOOST_CHECK_EQUAL( mesh->getNumSubMeshes(), 1u );
        BOOST_CHECK_EQUAL( mesh->getSubMesh( 0 )->getIndexBuffer()->getNumIndices(), 3u );
        auto graphicsScene = app->getGraphicsSystem()->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );
        auto graphicsMesh = workphone::dynamic_pointer_cast<render::IGraphicsMesh>(
            graphicsScene->addGraphicsObjectByTypeId( render::IGraphicsMesh::typeInfo() ) );
        BOOST_REQUIRE( graphicsMesh );
        graphicsMesh->setMeshName( relativePath );
        graphicsMesh->load( nullptr );
        BOOST_CHECK( graphicsMesh->isLoaded() );
        graphicsScene->removeGraphicsObject( graphicsMesh );
    }
    BOOST_CHECK_EQUAL( meshCount, 1u );
    auto gameManager = app->getGameManager();
    auto scene = gameManager ? gameManager->getCurrentScene() : nullptr;
    BOOST_REQUIRE( scene );
    auto actorsBeforeDrop = scene->getActors().size();
    auto dropData = workphone::make_ptr<Properties>();
    dropData->setProperty( "assetPath", "Assets/Triangle.obj" );
    dropData->setProperty( "assetType", "file" );
    dropData->setProperty( "filePath", "Assets/Triangle.obj" );
    auto payload = DataUtil::toString( dropData.get(), true );
    auto sceneWindow = editor->getUI()->getSceneWindow();
    BOOST_REQUIRE( sceneWindow );
    auto rootWindow = sceneWindow->getParentWindow();
    BOOST_REQUIRE( rootWindow );
    auto dropTarget = rootWindow->getDropTarget();
    BOOST_REQUIRE( dropTarget );
    Array<Parameter> dropArguments;
    dropArguments.emplace_back( payload );
    dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, dropArguments, nullptr, rootWindow,
                             nullptr );
    auto jobQueue = app->getJobQueue();
    BOOST_REQUIRE( jobQueue );
    for( int i = 0; i < 100 && jobQueue->hasJobs(); ++i )
    {
        jobQueue->update();
    }
    BOOST_CHECK_EQUAL( scene->getActors().size(), actorsBeforeDrop + 1 );
    if( scene->getActors().size() > actorsBeforeDrop )
    {
        auto createdActor = scene->getActors().back();
        BOOST_CHECK_EQUAL( createdActor->getName(), "Triangle" );
        app->getCommandManager()->clearAll();
        gameManager->destroyActor( createdActor );
    }
    for( const auto &entry : std::filesystem::directory_iterator( directory ) )
    {
        BOOST_CHECK( entry.path().extension() != ".prefab" &&
                     entry.path().extension() != ".resourcedata" );
    }
}

#    if defined( _WIN32 )
BOOST_AUTO_TEST_CASE( project_compile_integration_test )
{
    ProjectTestDirectory temporary;
    const auto directory = temporary.path / "Build Project with spaces";
    Project project;
    project.create( directory.u8string() );
    auto root = std::filesystem::path( __FILE__ );
    for( int i = 0; i < 6; ++i )
    {
        root = root.parent_path();
    }
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
    BOOST_CHECK_EQUAL( guard.app->getCachePath(), ( directory / "Cache" ).generic_u8string() + "/" );
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
#    endif

#endif
