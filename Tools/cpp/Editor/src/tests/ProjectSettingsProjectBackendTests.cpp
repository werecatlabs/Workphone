#include <EditorPCH.hpp>
#if WP_EDITOR_TESTS
#include <boost/test/unit_test.hpp>
#include <editor/ProjectSettingsProjectBackend.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ProjectServices.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Graphics/ProjectAssetPolicy.hpp>
#include <Workphone/Graphics/ProjectIrradiance.hpp>
#include <Workphone/IO/ZipUtil.hpp>
#include <limits>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>

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
            setProjectServiceProviders( {} );
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
    // Applying settings must retain plugin-injected providers instead of silently
    // replacing them with the local defaults.
    const auto injectedRoot = temporary.root / "injected";
    setProjectServiceProviders( makeLocalProjectServiceProviders( injectedRoot.u8string().c_str() ) );
    auto settings = make_ptr<Properties>();
    settings->setProperty( "services.cloudSave", true );
    settings->setProperty( "services.analytics", false );
    settings->setProperty( "services.crashReporting", false );
    BOOST_REQUIRE_MESSAGE( applyProjectServiceSettings( settings, temporary.root.u8string().c_str(), error ), error );
    BOOST_REQUIRE( putProjectCloudSave( "injected-slot", original, error ) );
    BOOST_CHECK( std::filesystem::is_regular_file( injectedRoot / "cloud-saves/slot-injected-slot.save" ) );
    struct ApplicationGuard
    {
        SmartPtr<core::IApplicationManager> previous = core::IApplicationManager::instance();
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        ApplicationGuard() { core::IApplicationManager::setInstance( app ); }
        ~ApplicationGuard() { core::IApplicationManager::setInstance( previous ); }
    } application;
    settings->setProperty( "services.cloudSave", false );
    settings->setProperty( "scripting.backend", 3 );
    BOOST_CHECK( !applyProjectServiceSettings( settings, temporary.root.u8string().c_str(), error ) );
    restored.clear();
    BOOST_REQUIRE_MESSAGE( getProjectCloudSave( "injected-slot", restored, error ), error );
    BOOST_CHECK_EQUAL_COLLECTIONS( original.begin(), original.end(), restored.begin(), restored.end() );
}

BOOST_AUTO_TEST_CASE( project_settings_build_profile_test )
{
    ProjectBackendTemporary temporary;
    auto project = make_ptr<Project>(); project->create( ( temporary.root / "Project" ).u8string().c_str() );
    const auto dictionary=temporary.root/"Project/Localization/en.settings";
    const auto probes=temporary.root/"Project/.workphone/lighting/fixture.settings";
    std::filesystem::create_directories(dictionary.parent_path());std::filesystem::create_directories(probes.parent_path());
    {std::ofstream stream(dictionary,std::ios::binary);stream<<std::string(8192,'L');}
    {std::ofstream stream(probes,std::ios::binary);stream<<"deterministic baked probe fixture";}
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
    BOOST_CHECK(std::filesystem::is_regular_file(output/"Localization/en.settings"));
    BOOST_CHECK(std::filesystem::is_regular_file(output/".workphone/lighting/fixture.settings"));
    Array<StringW> zipFiles{dictionary.wstring().c_str()};
    const auto rawZip=temporary.root/"raw.zip",compressedZip=temporary.root/"compressed.zip";
    BOOST_REQUIRE_EQUAL(ZipUtil::createZipFileFromPath(rawZip.wstring().c_str(),zipFiles,(temporary.root/"Project").wstring().c_str(),true,0),0);
    BOOST_REQUIRE_EQUAL(ZipUtil::createZipFileFromPath(compressedZip.wstring().c_str(),zipFiles,(temporary.root/"Project").wstring().c_str(),true,9),0);
    BOOST_CHECK(std::filesystem::file_size(rawZip)>std::filesystem::file_size(compressedZip)*4);
    auto manifest = make_ptr<Properties>();
    std::ifstream stream( output / "ProjectPackage.json" ); std::ostringstream text; text << stream.rdbuf();
    DataUtil::parse( text.str().c_str(), manifest.get() );
    BOOST_CHECK_EQUAL( manifest->getProperty( "project" ), "Fixture Product" );
    BOOST_CHECK_EQUAL( manifest->getPropertyAsInt( "defaultLanguage" ), 3 );
    // Only the selected platform requires its encoder. Saving mobile defaults
    // on a desktop project remains possible without mobile encoder plugins.
    settings->setProperty("assets.textureCompression.pc",s32(1));
    settings->setProperty("assets.textureCompression.ios",s32(2));
    settings->setProperty("assets.textureCompression.android",s32(3));
    BOOST_CHECK(validateProjectSettingsProjectBackend(settings)->getPropertyAsBool("success"));
    registerTextureCompressionEncoder("bc7",{});
    settings->setProperty("assets.textureCompression.pc",s32(2));
    auto missingEncoder=validateProjectSettingsProjectBackend(settings);
    BOOST_CHECK(!missingEncoder->getPropertyAsBool("success"));
    BOOST_CHECK(missingEncoder->getProperty("status").find("bc7")!=String::npos);
    settings->setProperty("assets.textureCompression.pc",s32(1));
    auto metadata=make_ptr<Properties>();metadata->setProperty("project.productName","Only metadata");
    auto noDomain=applyProjectSettingsProjectBackend(metadata);
    BOOST_CHECK(noDomain->getPropertyAsBool("success"));
    BOOST_CHECK(noDomain->getProperty("appliedKeys").empty());
    BOOST_CHECK(noDomain->getProperty("pendingKeys").empty());
    {
        struct NdkEnvironment
        {
            bool existed = std::getenv( "ANDROID_NDK_HOME" ) != nullptr;
            std::string previous = existed ? std::getenv( "ANDROID_NDK_HOME" ) : "";
            static void set( const std::string &value )
            {
#if defined( _WIN32 )
                _putenv_s( "ANDROID_NDK_HOME", value.c_str() );
#else
                setenv( "ANDROID_NDK_HOME", value.c_str(), 1 );
#endif
            }
            ~NdkEnvironment()
            {
#if defined( _WIN32 )
                set( previous );
#else
                if( existed ) set( previous ); else unsetenv( "ANDROID_NDK_HOME" );
#endif
            }
        } environment;
        const auto ndk = temporary.root / "NDK";
        std::filesystem::create_directories( ndk / "build/cmake" );
        std::ofstream( ndk / "build/cmake/android.toolchain.cmake" ) << "# Profile-generation fixture\n";
        NdkEnvironment::set( ndk.u8string() );
        const auto manifestTemplate = temporary.root / "Project/Android/app/src/main/AndroidManifest.xml";
        std::filesystem::create_directories( manifestTemplate.parent_path() );
        std::ofstream( manifestTemplate ) << R"(<manifest xmlns:android="http://schemas.android.com/apk/res/android"><application android:label="Old"><activity android:name=".MainActivity"/></application></manifest>)";
        auto android = make_ptr<Properties>();
        android->setProperty( "project.productName", "Fixture Android" );
        android->setProperty( "platform.active", 3 );
        android->setProperty( "build.config", 0 );
        android->setProperty( "build.architecture.android", 2 );
        android->setProperty( "build.android.universalStaging", true );
        android->setProperty( "build.android.stageOnly", true );
        android->setProperty( "build.android.appBundle", false );
        android->setProperty( "build.signingProfile", "projectReleaseKey" );
        const auto args = prepareProjectSettingsBuild( project, android );
        BOOST_CHECK( std::find( args.begin(), args.end(), String( "-DANDROID_ABI=x86" ) ) != args.end() );
        BOOST_CHECK( std::find( args.begin(), args.end(), String( "Ninja" ) ) != args.end() );
        const auto directory = temporary.root / "Project/Build/Android/Universal/Debug";
        const auto read = []( const std::filesystem::path &path ) {
            std::ifstream stream( path ); std::ostringstream result; result << stream.rdbuf(); return result.str();
        };
        const auto gradle = read( directory / "WorkphoneAndroidProfile.gradle" );
        BOOST_CHECK( gradle.find( "['armeabi-v7a','arm64-v8a','x86','x86_64']" ) != std::string::npos );
        BOOST_CHECK( gradle.find( "projectReleaseKey" ) != std::string::npos );
        BOOST_CHECK( gradle.find( "main.manifest.srcFile" ) != std::string::npos );
        const auto manifest = read( directory / "AndroidManifest.xml" );
        BOOST_CHECK( manifest.find( "Fixture Android" ) != std::string::npos );
        BOOST_CHECK( manifest.find( ".MainActivity" ) != std::string::npos );
        BOOST_CHECK( read( directory / "WorkphoneProjectProfile.cmake" ).find( "assembleDebug" ) == std::string::npos );
        android->setProperty( "build.android.stageOnly", false );
        prepareProjectSettingsBuild( project, android );
        BOOST_CHECK( read( directory / "WorkphoneProjectProfile.cmake" ).find( "assembleDebug" ) != std::string::npos );
    }
    settings->setProperty( "network.multiplayer", true );
    settings->setProperty( "network.backend", s32( 0 ) );
    BOOST_CHECK( !validateProjectSettingsProjectBackend( settings )->getPropertyAsBool( "success" ) );
}

BOOST_AUTO_TEST_CASE( project_irradiance_provider_lifecycle_test )
{
    ProjectBackendTemporary temporary;
    struct Provider final : IProjectIrradianceProvider
    {
        u32 calls=0;bool invalid=false;
        bool sample(const float position[3],u32 rays,s32 selection,bool sky,wp_irradiance_probe &probe,String &error) override
        {
            (void)rays;(void)sky;++calls;wp_irradiance_probe candidate{};
            std::copy(position,position+3,candidate.position);
            candidate.sh[0][0]=selection==2?0.f:1.f;candidate.sh[0][2]=selection==1?0.f:2.f;
            if(invalid)candidate.sh[0][0]=std::numeric_limits<float>::quiet_NaN();
            probe=candidate;error.clear();return true;
        }
    };
    struct Cleanup {~Cleanup(){releaseProjectIrradiance();}} cleanup;
    auto provider=std::make_shared<Provider>();setProjectIrradianceProvider(provider);
    auto settings=make_ptr<Properties>();settings->setProperty("graphics.gi",s32(1));
    settings->setProperty("project.startupScene","Fixture.fbscene");String error;
    BOOST_REQUIRE_MESSAGE(applyProjectIrradianceSettings(settings,temporary.root.u8string().c_str(),error),error);
    BOOST_CHECK_EQUAL(provider->calls,4u);const auto baked=getProjectIrradianceProbes();BOOST_REQUIRE_EQUAL(baked.size(),4u);
    BOOST_REQUIRE(applyProjectIrradianceSettings(settings,temporary.root.u8string().c_str(),error));
    BOOST_CHECK_EQUAL(provider->calls,4u); // The deterministic local bake is read from disk.
    settings->setProperty("graphics.gi",s32(3));
    BOOST_REQUIRE(applyProjectIrradianceSettings(settings,temporary.root.u8string().c_str(),error));
    auto hybrid=getProjectIrradianceProbes();BOOST_REQUIRE_EQUAL(hybrid.size(),4u);
    BOOST_CHECK_EQUAL(hybrid[0].sh[0][0],1.f);BOOST_CHECK_EQUAL(hybrid[0].sh[0][2],2.f);
    BOOST_CHECK_EQUAL(hybrid[1].sh[0][2],0.f);
    updateProjectIrradiance();updateProjectIrradiance();hybrid=getProjectIrradianceProbes();
    BOOST_CHECK_EQUAL(hybrid[1].sh[0][2],2.f);
    provider->invalid=true;settings->setProperty("graphics.gi",s32(2));
    BOOST_CHECK(!applyProjectIrradianceSettings(settings,temporary.root.u8string().c_str(),error));
    BOOST_CHECK_EQUAL(getProjectIrradianceMode(),3);
    const auto retained=getProjectIrradianceProbes();BOOST_CHECK_EQUAL(retained[1].sh[0][2],2.f);
    updateProjectIrradiance();BOOST_CHECK_EQUAL(getProjectIrradianceProbes()[1].sh[0][2],2.f);
    settings->setProperty("graphics.gi",s32(0));BOOST_REQUIRE(applyProjectIrradianceSettings(settings,temporary.root.u8string().c_str(),error));
    BOOST_CHECK(getProjectIrradianceProbes().empty());
}
#endif
