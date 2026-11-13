#if defined( _WIN32 ) && !defined( NOMINMAX )
#    define NOMINMAX
#endif
#ifdef WP_SCRIPT_DISCOVERY_TEST_STANDALONE
#    define BOOST_TEST_MODULE ScriptDiscoveryTests
#    include <boost/test/included/unit_test.hpp>
#else
#    include <boost/test/unit_test.hpp>
#endif
#include <Workphone/Script/ScriptDiscoveryPolicy.hpp>
#include <filesystem>
#include <string>

namespace
{
    namespace fs = std::filesystem;
    using workphone::Array;
    using workphone::String;
    using workphone::shouldAutoLoadProjectScript;

    String pathString( const fs::path &path )
    {
        return String( path.generic_u8string().c_str() );
    }

    // These paths need not exist: discovery policy is lexical and must not read,
    // create or follow arbitrary project files to make the decision.
    const fs::path project = fs::current_path() / "ScriptDiscoveryFixture" / "Project";

    void expect( const String &script, const String &root, bool allowed )
    {
        BOOST_CHECK_MESSAGE( shouldAutoLoadProjectScript( script, root ) == allowed,
                             "Unexpected auto-discovery policy for " << script.c_str()
                             << " under " << root.c_str() );
    }
}

BOOST_AUTO_TEST_SUITE( ScriptDiscoveryTests )

BOOST_AUTO_TEST_CASE( top_level_generated_and_test_directories_are_excluded )
{
    const auto root = pathString( project );
    for( const auto *directory : { "Cache", "SettingsCache", "Tests", ".workphone" } )
    {
        expect( String( directory ) + "/Probe.lua", root, false );
        expect( String( directory ) + "/nested/deeper/Probe.lua", root, false );
        expect( pathString( project / directory / "Probe.lua" ), root, false );
    }
}

BOOST_AUTO_TEST_CASE( authored_assets_and_similar_names_remain_discoverable )
{
    const auto root = pathString( project );
    for( const auto *relative : { "Application.lua", "Tests.lua", "Assets/WerecatApplication.lua",
                                 "Assets/Cache/Probe.lua", "Assets/SettingsCache/Probe.lua",
                                 "Assets/Tests/Probe.lua", "Assets/.workphone/Probe.lua",
                                 "Scripts/Tests/Probe.lua", "CacheModels/Probe.lua",
                                 "TestsExtra/Probe.lua", "SettingsCacheExtra/Probe.lua",
                                 ".workphone-extra/Probe.lua" } )
    {
        expect( relative, root, true );
        expect( pathString( project / relative ), root, true );
    }
}

BOOST_AUTO_TEST_CASE( slashes_and_dot_segments_do_not_bypass_directory_boundaries )
{
    const auto root = pathString( project / "." );
    for( const auto *relative : { "./Cache/Probe.lua", "Assets/../Cache/Probe.lua",
                                 "Assets\\..\\Tests\\Probe.lua",
                                 "Cache//nested/./Probe.lua", "Cache/nested/../Probe.lua",
                                 "./SettingsCache/Probe.lua", "Assets/../.workphone/Probe.lua" } )
    {
        expect( relative, root, false );
    }
    expect( "Cache/../Assets/Probe.lua", root, true );
    expect( "Tests/../Application.lua", root, true );
    expect( "Assets/Cache/../Tests/Probe.lua", root, true );

    auto windowsRoot = std::string( root.c_str() );
    std::replace( windowsRoot.begin(), windowsRoot.end(), '/', '\\' );
    expect( "Cache\\Probe.lua", String( ( windowsRoot + "\\" ).c_str() ), false );
    expect( "Assets\\Probe.lua", String( windowsRoot.c_str() ), true );
}

BOOST_AUTO_TEST_CASE( external_mounts_and_project_prefix_siblings_are_preserved )
{
    const auto root = pathString( project );
    expect( pathString( project.parent_path() / "Media/Scripts/BaseComponent.lua" ), root, true );
    expect( pathString( project.parent_path() / "Cache/External.lua" ), root, true );
    expect( pathString( project.parent_path() / "ProjectSibling/Cache/Probe.lua" ), root, true );
    expect( "../Media/Scripts/BaseComponent.lua", root, true );
    expect( "../Cache/External.lua", root, true );
    expect( "Cache/../../Media/Scripts/BaseComponent.lua", root, true );
}

BOOST_AUTO_TEST_CASE( a_disposable_project_inside_cache_uses_its_own_complete_root )
{
    const auto fixture = project / "Cache/Validation/model-batch-0123456789abcdef/project";
    const auto script = pathString( fixture / "Assets/WerecatApplication.lua" );
    expect( script, pathString( fixture ), true );
    expect( "Assets/WerecatApplication.lua", pathString( fixture ), true );
    expect( script, pathString( project ), false );
    expect( pathString( fixture / "Cache/Validation/Probe.lua" ), pathString( fixture ), false );
    expect( pathString( fixture / "Tests/Probe.lua" ), pathString( fixture ), false );
    expect( pathString( fixture / "Assets/Tests/Probe.lua" ), pathString( fixture ), true );
}

BOOST_AUTO_TEST_CASE( root_and_directory_case_follow_the_host_filesystem_policy )
{
    const auto root = pathString( project );
#ifdef _WIN32
    expect( "cAcHe/Probe.lua", root, false );
    expect( "tEsTs/Probe.lua", root, false );
    expect( "SETTINGSCACHE/Probe.lua", root, false );
    expect( ".WORKPHONE/Probe.lua", root, false );
    auto upper = std::string( pathString( project / "cache/Probe.lua" ).c_str() );
    for( auto &character : upper )
        if( character >= 'a' && character <= 'z' ) character -= 'a' - 'A';
    expect( String( upper.c_str() ), root, false );
    expect( "\\\\server\\share\\game\\CACHE\\Probe.lua", "//SERVER/share/Game", false );
    expect( "\\\\server\\share\\gameOther\\Cache\\Probe.lua", "//server/share/game", true );
#else
    expect( "cache/Probe.lua", root, true );
    expect( "tests/Probe.lua", root, true );
    expect( "SETTINGSCACHE/Probe.lua", root, true );
    expect( ".WORKPHONE/Probe.lua", root, true );
    expect( pathString( project.parent_path() / "project/Cache/Probe.lua" ), root, true );
#endif
}

BOOST_AUTO_TEST_CASE( missing_context_and_relative_project_roots_remain_supported )
{
    expect( "", pathString( project ), true );
    expect( "Cache/Probe.lua", "", true );
    expect( "", "", true );
    const auto relative = pathString( project.lexically_relative( fs::current_path() ) );
    expect( "Cache/Probe.lua", relative, false );
    expect( pathString( project / "Tests/Probe.lua" ), relative, false );
    expect( "Assets/Probe.lua", relative, true );
}

BOOST_AUTO_TEST_CASE( filtering_preserves_order_duplicates_and_input_path_spelling )
{
    Array<String> scripts{ "Cache/ShadowApplication.lua", "Assets/First.lua", "Tests/Helper.lua",
                           "Assets/Tests/Authored.lua", "Assets\\Second.lua", "Assets/First.lua",
                           "../Media/Scripts/BaseComponent.lua", "SettingsCache/Hidden.lua",
                           ".workphone/Hidden.lua" };
    // Explicit project file lists are separate from this auto-discovered list;
    // filtering a discovered copy must not mutate an explicit source list.
    const auto explicitFiles = scripts;
    workphone::filterAutoDiscoveredScripts( scripts, pathString( project ) );
    const char *expected[] = { "Assets/First.lua", "Assets/Tests/Authored.lua", "Assets\\Second.lua",
                               "Assets/First.lua", "../Media/Scripts/BaseComponent.lua" };
    BOOST_REQUIRE_EQUAL( scripts.size(), 5 );
    for( size_t index = 0; index < scripts.size(); ++index )
        BOOST_CHECK_EQUAL( std::string( scripts[index].c_str() ), expected[index] );
    BOOST_CHECK_EQUAL( explicitFiles.size(), 9 );
    BOOST_CHECK_EQUAL( std::string( explicitFiles[0].c_str() ), "Cache/ShadowApplication.lua" );
    workphone::filterAutoDiscoveredScripts( scripts, pathString( project ) );
    BOOST_CHECK_EQUAL( scripts.size(), 5 );

    Array<String> excluded{ "Cache/One.lua", "Tests/Two.lua" };
    workphone::filterAutoDiscoveredScripts( excluded, pathString( project ) );
    BOOST_CHECK( excluded.empty() );
    workphone::filterAutoDiscoveredScripts( excluded, pathString( project ) );
    BOOST_CHECK( excluded.empty() );
}

BOOST_AUTO_TEST_SUITE_END()
