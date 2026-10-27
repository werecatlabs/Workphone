#include <EditorPCH.hpp>
#include <editor/ProjectSettingsProjectBackend.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/ProjectServices.hpp>
#include <Workphone/Graphics/ProjectAssetPolicy.hpp>
#include <Workphone/IO/ZipUtil.hpp>
#include <Workphone/System/ResourcePackageProfile.hpp>
#include <Workphone/Script/ProjectScriptPolicy.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <mutex>
#include <set>
#include <cstdlib>
#include <tinyxml.h>

namespace workphone::editor
{
    namespace
    {
        namespace fs = std::filesystem;

        String configuration( const Properties &s )
        {
            constexpr const char *names[] = { "Debug", "RelWithDebInfo", "Release", "MinSizeRel" };
            const auto value = s.getPropertyAsInt( "build.config", 1 );
            if( value < 0 || value > 3 ) throw std::invalid_argument( "Invalid build configuration" );
            return names[value];
        }
        String platform( const Properties &s )
        {
            constexpr const char *names[] = { "PC", "macOS", "iOS", "Android" };
            const auto value = s.getPropertyAsInt( "platform.active", 0 );
            if( value < 0 || value > 3 ) throw std::invalid_argument( "Invalid target platform" );
            return names[value];
        }
        String architecture( const Properties &s )
        {
            const auto target = s.getPropertyAsInt( "platform.active", 0 );
            const char *key = target == 0 ? "build.architecture.pc" : target == 1 ? "build.architecture.mac" :
                              target == 2 ? "build.architecture.ios" : "build.architecture.android";
            const auto value = s.getPropertyAsInt( key, target == 2 ? 0 : 1 );
            static const Array<Array<String>> options = {
                { "Win32", "x64", "ARM64" }, { "x86_64", "arm64", "x86_64;arm64" },
                { "arm64" }, { "armeabi-v7a", "arm64-v8a", "x86", "x86_64", "Universal" } };
            if( value < 0 || static_cast<size_t>( value ) >= options[target].size() )
                throw std::invalid_argument( "Invalid target architecture" );
            return options[target][value];
        }
        fs::path outputDirectory( SmartPtr<Project> project, const Properties &s )
        {
            auto arch = architecture( s );
            if( s.getPropertyAsInt( "platform.active", 0 ) == 3 &&
                s.getPropertyAsBool( "build.android.universalStaging", false ) ) arch = "Universal";
            std::replace( arch.begin(), arch.end(), ';', '_' );
            return fs::u8path( project->getPath().c_str() ) / "Build" / platform( s ).c_str() /
                   arch.c_str() / configuration( s ).c_str();
        }
        void writeText( const fs::path &path, const String &text )
        {
            fs::create_directories( path.parent_path() );
            std::ofstream stream( path, std::ios::binary | std::ios::trunc );
            stream.exceptions( std::ios::badbit | std::ios::failbit );
            stream.write( text.data(), text.size() ); stream.close();
        }
        String cmakeQuote( const String &value )
        {
            String result = "\"";
            for( const auto c : value )
            {
                if( c == '\0' || c == '\n' || c == '\r' ) throw std::invalid_argument( "Invalid CMake profile value" );
                if( c == '\\' || c == '"' || c == '$' || c == ';' ) result += '\\';
                result += c;
            }
            return result + "\"";
        }
        String groovyQuote( const String &value )
        {
            String result = "'";
            for( const auto c : value )
            {
                if( c == '\0' || c == '\n' || c == '\r' )
                    throw std::invalid_argument( "Invalid Android profile value" );
                if( c == '\\' || c == '\'' ) result += '\\';
                result += c;
            }
            return result + "'";
        }
        Array<String> scenePaths( const Properties &s )
        {
            Array<String> paths;
            std::set<String> seen;
            auto add = [&]( String value ) {
                if( value.empty() || !seen.insert( value ).second ) return;
                paths.push_back( value );
            };
            add( s.getProperty( "project.startupScene" ) );
            std::istringstream list( s.getProperty( "project.sceneList" ).c_str() );
            std::string item;
            while( std::getline( list, item, ';' ) ) add( item.c_str() );
            return paths;
        }
        fs::path checkedMember( const fs::path &root, const String &value )
        {
            const auto relative = fs::u8path( value.c_str() );
            if( relative.empty() || relative.is_absolute() || value.find( '\0' ) != String::npos )
                throw std::invalid_argument( "Packaged paths must be project relative" );
            const auto resolved = fs::weakly_canonical( root / relative );
            const auto local = resolved.lexically_relative( root );
            if( local.empty() || *local.begin() == ".." )
                throw std::invalid_argument( "Packaged file escapes the project root" );
            return resolved;
        }
        void stageAndroidManifest( const fs::path &root, const fs::path &output, const Properties &settings )
        {
            const auto source = checkedMember( root, "Android/app/src/main/AndroidManifest.xml" );
            if( !fs::is_regular_file( source ) || fs::file_size( source ) > 4 * 1024 * 1024 )
                throw std::runtime_error( "Android packaging requires Android/app/src/main/AndroidManifest.xml" );
            std::ifstream stream( source, std::ios::binary );
            std::ostringstream contents; contents << stream.rdbuf();
            if( !stream || stream.bad() ) throw std::runtime_error( "Cannot read the Android manifest" );
            TiXmlDocument document;
            document.Parse( contents.str().c_str(), nullptr, TIXML_ENCODING_UTF8 );
            const auto manifest = document.RootElement();
            auto application = manifest ? manifest->FirstChildElement( "application" ) : nullptr;
            if( document.Error() || !manifest || String( manifest->Value() ) != "manifest" || !application )
                throw std::runtime_error( "The Android application manifest is invalid" );
            application->SetAttribute( "android:label", settings.getProperty( "project.productName", "Workphone Game" ).c_str() );
            TiXmlPrinter printer;
            document.Accept( &printer );
            writeText( output / "AndroidManifest.xml", String( printer.CStr(), printer.Size() ) );
        }
        Array<fs::path> stageRuntimeSupportFiles( const fs::path &root, const fs::path &output )
        {
            Array<fs::path> staged;
            u64 total = 0;
            for( const char *folder : { "Localization", ".workphone/lighting", ".workphone/generated" } )
            {
                const auto source = root / folder;
                if( !fs::exists( source ) ) continue;
                if( fs::is_symlink( fs::symlink_status( source ) ) )
                    throw std::runtime_error( "Runtime data staging refuses redirected folders" );
                for( const auto &entry : fs::recursive_directory_iterator( source ) )
                {
                    if( entry.is_symlink() ) throw std::runtime_error( "Runtime data staging refuses redirected files" );
                    if( !entry.is_regular_file() ) continue;
                    const auto bytes = entry.file_size(); total += bytes;
                    if( bytes > 8 * 1024 * 1024 || total > 64 * 1024 * 1024 || staged.size() >= 4096 )
                        throw std::runtime_error( "Runtime dictionary/probe data exceeds staging limits" );
                    const auto relative = fs::relative( entry.path(), root );
                    const auto checked = checkedMember( root, relative.generic_u8string().c_str() );
                    const auto destination = output / relative;
                    fs::create_directories( destination.parent_path() );
                    fs::copy_file( checked, destination, fs::copy_options::overwrite_existing );
                    staged.push_back( destination );
                }
            }
            return staged;
        }
        SmartPtr<Properties> resultFor( bool success, const String &message )
        {
            auto result = make_ptr<Properties>(); result->setProperty( "success", success );
            result->setProperty( "status", message ); return result;
        }
        void validate( const Properties &s )
        {
            configuration( s ); platform( s ); architecture( s );
            const auto backend = s.getPropertyAsInt( "scripting.backend", 0 );
            if( backend < 0 || backend > 3 ) throw std::invalid_argument( "Invalid scripting backend" );
            if( backend == 3 )
            {
                auto app = core::IApplicationManager::instancePtr();
                auto factory = app ? app->getFactoryManager() : nullptr;
                const auto provider = factory ? factory->getFactoryByName( "ExternalProjectScriptManager" ) : nullptr;
                if( !provider || !provider->isObjectDerivedFromByInfo( IScriptManager::typeInfo() ) )
                    throw std::invalid_argument( "Install an IScriptManager provider named ExternalProjectScriptManager for External VM" );
            }
            const auto reflection = s.getPropertyAsInt( "scripting.reflection", 1 );
            if( reflection < 0 || reflection > 3 ) throw std::invalid_argument( "Invalid reflection mode" );
            const auto network = s.getPropertyAsInt( "network.backend", 0 );
            if( network < 0 || network > 3 ) throw std::invalid_argument( "Invalid network backend" );
            if( s.getPropertyAsBool( "network.multiplayer" ) && network == 0 )
                throw std::invalid_argument( "Multiplayer requires a network backend" );
            if( s.getPropertyAsBool( "network.multiplayer" ) )
            {
                auto app = core::IApplicationManager::instancePtr();
                auto factory = app ? app->getFactoryManager() : nullptr;
                const String provider = network == 1 ? "WPNetworkManager" : network == 2 ? "BoostAsioNetworkManager" : "PlatformNetworkManager";
                const auto implementation = factory ? factory->getFactoryByName( provider ) : nullptr;
                if( !implementation || !implementation->isObjectDerivedFromByInfo( INetworkManager::typeInfo() ) )
                    throw std::invalid_argument( ( "Install the network provider plugin: " + provider ).c_str() );
            }
            if( s.getPropertyAsInt( "build.android.minSdk", 26 ) > s.getPropertyAsInt( "build.android.targetSdk", 35 ) )
                throw std::invalid_argument( "Minimum Android SDK exceeds target SDK" );
            for( const char *key : { "assets.compression", "assets.textureCompression.pc", "assets.textureCompression.mac",
                                     "assets.textureCompression.ios", "assets.textureCompression.android" } )
            {
                if( !s.hasProperty( key ) ) continue;
                const auto maximum = String( key ) == "assets.textureCompression.android" ? 4 : 3;
                const auto value = s.getPropertyAsInt( key );
                if( value < 0 || value > maximum ) throw std::invalid_argument( ( String( "Invalid asset policy: " ) + key ).c_str() );
            }
            const char *textureKeys[]={"assets.textureCompression.pc","assets.textureCompression.mac","assets.textureCompression.ios","assets.textureCompression.android"};
            const auto active=s.getPropertyAsInt("platform.active",0);
            if(s.hasProperty(textureKeys[active]))
            {
                ProjectAssetPolicy policy;policy.platform=active;policy.textureCompression=s.getPropertyAsInt(textureKeys[active]);
                const auto format=projectTextureCompressionName(policy,true);
                if(!hasTextureCompressionEncoder(format)) throw std::invalid_argument(("Install a texture encoder provider for "+format+" before cooking the active platform").c_str());
            }
        }
    }

    SmartPtr<Properties> validateProjectSettingsProjectBackend( SmartPtr<Properties> settings )
    {
        try { if( !settings ) throw std::invalid_argument( "Project profile is unavailable" ); validate( *settings ); return resultFor( true, "Project backends validated." ); }
        catch( const std::exception &e ) { return resultFor( false, e.what() ); }
    }

    void describeProjectSettingsProjectBackend( SmartPtr<Properties> response )
    {
        if( !response ) return;
        const char *keys[] = { "project.sceneList", "project.defaultLanguage", "build.config", "build.architecture.pc",
            "build.architecture.mac", "build.architecture.ios", "build.architecture.android", "build.ios.minVersion",
            "build.android.minSdk", "build.android.targetSdk", "build.android.appBundle", "build.signingProfile",
            "build.stripSymbols", "build.debugSymbols", "build.includeEditorData", "build.assetBundles", "build.compression",
            "scripting.backend", "scripting.lua", "scripting.hotReload", "scripting.reflection", "network.backend",
            "network.multiplayer", "services.cloudSave", "services.analytics", "services.crashReporting",
            "assets.compression", "assets.textureCompression.pc", "assets.textureCompression.mac",
            "assets.textureCompression.ios", "assets.textureCompression.android" };
        for( const auto key : keys )
        {
            response->setProperty( String( "capability." ) + key, true );
            response->setProperty( String( "availability." ) + key,
                "Project build/runtime adapter. Cloud and analytics use local development providers unless replaced by a plugin." );
        }
        response->setProperty("availability.assets.compression","Bounded deflate storage for checked texture/material/shader artifacts; None/Fast/Balanced/Maximum select raw/level1/level6/level9.");
        for( const char *key : { "assets.textureCompression.pc", "assets.textureCompression.mac", "assets.textureCompression.ios", "assets.textureCompression.android" } )
            response->setProperty(String("availability.")+key,"Platform texture cook default. Explicit asset recipes win. BGRA/BC1/BC3 are built in; BC7/ASTC/PVRTC/ETC2 dispatch to registered encoder providers and require their platform runtime installer.");
    }

    SmartPtr<Properties> applyProjectSettingsProjectBackend( SmartPtr<Properties> settings )
    {
        try
        {
            if( !settings ) throw std::invalid_argument( "Project profile is unavailable" );
            bool domain = false, assetDomain=false, serviceDomain=false;
            String requested, runtimeKeys, buildKeys;
            const auto appendKey=[](String &list,const String &key){if(!list.empty())list+="; ";list+=key;};
            const char *textureKeys[]={"assets.textureCompression.pc","assets.textureCompression.mac","assets.textureCompression.ios","assets.textureCompression.android"};
            const auto activePlatform=settings->getPropertyAsInt("platform.active",0);
            const String activeTextureKey=textureKeys[std::clamp(activePlatform,0,3)];
            for( const auto &property : settings->getPropertiesAsArray() )
            {
                const auto key = property.getName();
                serviceDomain=serviceDomain || key.find("services.")==0 || key.find("scripting.")==0 || key.find("network.")==0;
                assetDomain=assetDomain || key.find("assets.")==0 || key=="platform.active";
                if( key.find( "services." ) == 0 || key.find( "scripting." ) == 0 || key.find( "network." ) == 0 ||
                    key.find( "build." ) == 0 || key.find( "assets." ) == 0 || key=="platform.active" )
                {
                    domain=true;appendKey(requested,key);
                    if(key.find("build.")==0 || key=="platform.active" ||
                       (key.find("assets.textureCompression.")==0 && key!=activeTextureKey))appendKey(buildKeys,key);
                    else appendKey(runtimeKeys,key);
                }
            }
            if( !domain ) return resultFor( true, "No project backend settings in this request." );
            validate( *settings );
            auto app = core::IApplicationManager::instancePtr();
            const auto pendingReply=[&](const String &message){auto reply=resultFor(true,message);reply->setProperty("pendingKeys",requested);return reply;};
            if( !app ) return pendingReply("Project backends will initialize at application startup.");
            auto editor = EditorManager::getSingletonPtr();
            const auto root = editor && editor->getProject() ? editor->getProject()->getPath() : app->getProjectPath();
            if( root.empty() ) return pendingReply("Project backends will initialize when a project directory is available.");
            String error;
            if( assetDomain && !writeProjectAssetPolicy( root, settings, error ) ) throw std::runtime_error( error.c_str() );
            if( serviceDomain && !applyProjectServiceSettings( settings, root, error ) )
                throw std::runtime_error( error.c_str() );
            auto reply=resultFor( true, "Project script policy, local service providers and network lifecycle applied." );
            reply->setProperty("appliedKeys",runtimeKeys);reply->setProperty("pendingKeys",buildKeys);return reply;
        }
        catch( const std::exception &e ) { return resultFor( false, e.what() ); }
    }

    void updateProjectSettingsProjectBackend()
    {
        auto app = core::IApplicationManager::instancePtr();
        if( !app ) return;
        auto settings = app->getPlayerSettings();
        if( !settings ) return;
        if( settings->getPropertyAsBool( "network.multiplayer" ) )
            if( auto network = app->getNetworkManager() ) network->poll();
    }

    Array<String> prepareProjectSettingsBuild( SmartPtr<Project> project, SmartPtr<Properties> settings )
    {
        if( !project || !settings ) throw std::invalid_argument( "A project and profile are required" );
        validate( *settings );
        const auto root = fs::canonical( fs::u8path( project->getPath().c_str() ) );
        String policyError;
        if( !writeProjectAssetPolicy( root.u8string().c_str(), settings, policyError ) )
            throw std::runtime_error( policyError.c_str() );
        const auto output = outputDirectory( project, *settings );
        const auto profile = output / "WorkphoneProjectProfile.cmake";
        fs::create_directories( output );
        auto manifest = make_ptr<Properties>();
        manifest->setProperty( "projectSettingsVersion", 1 );
        manifest->setProperty( "project", settings->getProperty( "project.productName" ) );
        manifest->setProperty( "bundleId", settings->getProperty( "project.bundleId" ) );
        manifest->setProperty( "version", settings->getProperty( "project.version" ) );
        manifest->setProperty( "defaultLanguage", settings->getPropertyAsInt( "project.defaultLanguage" ) );
        manifest->setProperty( "includeEditorData", settings->getPropertyAsBool( "build.includeEditorData" ) );
        manifest->setProperty( "sceneList", scenePaths( *settings ) );
        writeText( output / "ProjectPackage.json", DataUtil::toString( manifest.get(), true ) );
        writeText( output / "ProjectRuntime.settings", DataUtil::toString( settings.get(), true ) );
        stageRuntimeSupportFiles( root, output );

        String script = "function(workphone_apply_project_profile)\n if(NOT TARGET Plugin)\n  message(FATAL_ERROR \"Project has no Plugin target\")\n endif()\n";
        script += " target_compile_definitions(Plugin PRIVATE WP_PROJECT_LUA_ENABLED=" + StringUtil::toString( settings->getPropertyAsBool( "scripting.lua", true ) ? 1 : 0 ) +
                  " WP_PROJECT_SCRIPT_BACKEND=" + StringUtil::toString( settings->getPropertyAsInt( "scripting.backend" ) ) +
                  " WP_PROJECT_REFLECTION_MODE=" + StringUtil::toString( settings->getPropertyAsInt( "scripting.reflection", 1 ) ) + ")\n";
        script += " if(MSVC)\n";
        if( settings->getPropertyAsBool( "build.debugSymbols", true ) )
            script += "  target_compile_options(Plugin PRIVATE /Zi)\n  target_link_options(Plugin PRIVATE /DEBUG)\n";
        else script += "  set_property(TARGET Plugin PROPERTY MSVC_DEBUG_INFORMATION_FORMAT \"\")\n  target_link_options(Plugin PRIVATE /DEBUG:NONE)\n";
        if( settings->getPropertyAsBool( "build.stripSymbols", true ) )
            script += "  target_link_options(Plugin PRIVATE /OPT:REF /OPT:ICF)\n";
        script += " else()\n";
        script += settings->getPropertyAsBool( "build.debugSymbols", true ) ? "  target_compile_options(Plugin PRIVATE -g)\n" : "  target_compile_options(Plugin PRIVATE -g0)\n";
        if( settings->getPropertyAsBool( "build.stripSymbols", true ) ) script += "  target_link_options(Plugin PRIVATE -s)\n";
        script += " endif()\n";
        if( settings->getPropertyAsInt( "platform.active" ) == 3 )
        {
            const auto androidProject = root / "Android";
#if defined( _WIN32 )
            const auto gradle = androidProject / "gradlew.bat";
#else
            const auto gradle = androidProject / "gradlew";
#endif
            const bool debug = settings->getPropertyAsInt( "build.config", 1 ) == 0;
            const String variant = debug ? "Debug" : "Release";
            const String variantDirectory = debug ? "debug" : "release";
            const auto task = String( settings->getPropertyAsBool( "build.android.appBundle", true ) ? "bundle" : "assemble" ) + variant;
            const auto init = output / "WorkphoneAndroidProfile.gradle";
            const auto id = settings->getProperty( "project.bundleId", "com.company.workphone" );
            if( id.empty() || id.find_first_not_of( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_." ) != String::npos )
                throw std::invalid_argument( "Android bundle identifier is invalid" );
            String gradleProfile = "allprojects { p -> p.plugins.withId('com.android.application') { p.androidComponents.finalizeDsl { android ->\n";
            gradleProfile += " p.android.defaultConfig.minSdk = " + StringUtil::toString( settings->getPropertyAsInt( "build.android.minSdk", 26 ) ) + "\n";
            gradleProfile += " p.android.defaultConfig.targetSdk = " + StringUtil::toString( settings->getPropertyAsInt( "build.android.targetSdk", 35 ) ) + "\n";
            gradleProfile += " p.android.defaultConfig.applicationId = " + groovyQuote( id ) + "\n";
            gradleProfile += " p.android.defaultConfig.versionName = " + groovyQuote( settings->getProperty( "project.version", "0.1.0" ) ) + "\n";
            gradleProfile += " p.android.defaultConfig.manifestPlaceholders['workphoneProductName'] = " + groovyQuote( settings->getProperty( "project.productName" ) ) + "\n";
            const bool universal = settings->getPropertyAsBool( "build.android.universalStaging" );
            gradleProfile += " p.android.defaultConfig.ndk.abiFilters.clear()\n p.android.defaultConfig.ndk.abiFilters.addAll(";
            gradleProfile += universal ? "['armeabi-v7a','arm64-v8a','x86','x86_64']" : "[" + groovyQuote( architecture( *settings ) ) + "]";
            gradleProfile += ")\n p.android.splits.abi.enable = false\n";
            gradleProfile += " p.android.sourceSets.main.jniLibs.srcDirs += [new File(" + groovyQuote( ( output / "native" ).generic_u8string().c_str() ) + ")]\n";
            stageAndroidManifest( root, output, *settings );
            gradleProfile += " p.android.sourceSets.main.manifest.srcFile(new File(" + groovyQuote( ( output / "AndroidManifest.xml" ).generic_u8string().c_str() ) + "))\n";
            const auto signing = settings->getProperty( "build.signingProfile", "Default" );
            if( !signing.empty() && signing != "Default" )
            {
                gradleProfile += " def signing = p.android.signingConfigs.findByName(" + groovyQuote( signing ) + ")\n";
                gradleProfile += " if (signing == null) throw new GradleException('Project signing profile is not defined')\n";
                gradleProfile += " p.android.buildTypes.getByName(" + groovyQuote( variantDirectory ) + ").signingConfig = signing\n";
            }
            gradleProfile += "} } }\n";
            writeText( init, gradleProfile );
            if( !settings->getPropertyAsBool( "build.android.stageOnly" ) )
                script += " add_custom_command(TARGET Plugin POST_BUILD COMMAND " + cmakeQuote( gradle.generic_u8string().c_str() ) +
                " --project-dir " + cmakeQuote( androidProject.generic_u8string().c_str() ) + " --init-script " +
                cmakeQuote( init.generic_u8string().c_str() ) + " " + task + " COMMAND ${CMAKE_COMMAND} -E copy_directory " +
                cmakeQuote( ( androidProject / "app/build/outputs" /
                    ( String( settings->getPropertyAsBool( "build.android.appBundle", true ) ? "bundle/" : "apk/" ) + variantDirectory ).c_str() ).generic_u8string().c_str() ) +
                " " + cmakeQuote( ( output / "Android" ).generic_u8string().c_str() ) + " VERBATIM)\n";
        }
        const auto signing = settings->getProperty( "build.signingProfile", "Default" );
        if( settings->getPropertyAsInt( "platform.active" ) != 3 && !signing.empty() && signing != "Default" )
            script += " if(WIN32)\n  find_program(WP_SIGNTOOL signtool REQUIRED)\n  add_custom_command(TARGET Plugin POST_BUILD COMMAND ${WP_SIGNTOOL} sign /fd SHA256 /sha1 " + cmakeQuote( signing ) +
                      " $<TARGET_FILE:Plugin> VERBATIM)\n elseif(APPLE)\n  add_custom_command(TARGET Plugin POST_BUILD COMMAND codesign --force --sign " + cmakeQuote( signing ) + " $<TARGET_FILE:Plugin> VERBATIM)\n endif()\n";
        script += "endfunction()\ncmake_language(DEFER CALL workphone_apply_project_profile)\n";
        writeText( profile, script );
        Array<String> args = { "-DCMAKE_PROJECT_INCLUDE=" + String( profile.generic_u8string().c_str() ) };
        auto config = configuration( *settings ); std::transform( config.begin(), config.end(), config.begin(), ::toupper );
        args.push_back( "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_" + config + "=" + String( output.generic_u8string().c_str() ) );
        args.push_back( "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_" + config + "=" + String( output.generic_u8string().c_str() ) );
        const auto target = settings->getPropertyAsInt( "platform.active", 0 );
        if( target == 0 )
        {
#if defined( _WIN32 )
            args.push_back( "-A" ); args.push_back( architecture( *settings ) );
#endif
        }
        else if( target == 1 || target == 2 )
        {
            args.push_back( "-DCMAKE_OSX_ARCHITECTURES=" + architecture( *settings ) );
            if( target == 2 )
            {
                args.push_back( "-DCMAKE_SYSTEM_NAME=iOS" );
                args.push_back( "-DCMAKE_OSX_DEPLOYMENT_TARGET=" + StringUtil::toString( settings->getPropertyAsInt( "build.ios.minVersion", 14 ) ) );
            }
        }
        else
        {
            const auto ndk = std::getenv( "ANDROID_NDK_HOME" );
            if( !ndk ) throw std::runtime_error( "Set ANDROID_NDK_HOME to the installed Android NDK" );
            const auto toolchain = fs::u8path( ndk ) / "build/cmake/android.toolchain.cmake";
            if( !fs::is_regular_file( toolchain ) ) throw std::runtime_error( "Android NDK toolchain file is missing" );
            args.push_back( "-G" ); args.push_back( "Ninja" );
            args.push_back( "-DCMAKE_BUILD_TYPE=" + configuration( *settings ) );
            args.push_back( "-DCMAKE_TOOLCHAIN_FILE=" + String( toolchain.generic_u8string().c_str() ) );
            args.push_back( "-DANDROID_ABI=" + architecture( *settings ) );
            args.push_back( "-DANDROID_PLATFORM=android-" + StringUtil::toString( settings->getPropertyAsInt( "build.android.minSdk", 26 ) ) );
            args.push_back( "-DANDROID_TARGET_SDK=" + StringUtil::toString( settings->getPropertyAsInt( "build.android.targetSdk", 35 ) ) );
            args.push_back( "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_" + config + "=" +
                String( ( output / "native" / architecture( *settings ).c_str() ).generic_u8string().c_str() ) );
        }
        return args;
    }

    SmartPtr<Properties> buildProjectSettingsProjectBackend( SmartPtr<Properties> settings )
    {
        try
        {
            auto editor = EditorManager::getSingletonPtr(); auto project = editor ? editor->getProject() : nullptr;
            if( !project || !settings ) throw std::runtime_error( "Open a project before building" );
            const auto target = settings->getPropertyAsInt( "platform.active" );
#if !defined( __APPLE__ )
            if( target == 1 || target == 2 ) throw std::runtime_error( "macOS/iOS builds require an Apple host with Xcode and its SDKs" );
#endif
            if( target == 3 )
            {
                const auto androidProject = fs::u8path( project->getPath().c_str() ) / "Android";
#if defined( _WIN32 )
                const auto gradle = androidProject / "gradlew.bat";
#else
                const auto gradle = androidProject / "gradlew";
#endif
                if( !fs::is_regular_file( gradle ) )
                    throw std::runtime_error( "Android app packaging requires the project's Android/gradlew application wrapper" );
            }
            if( target == 3 && architecture( *settings ) == "Universal" )
            {
                for( s32 abi = 0; abi < 4; ++abi )
                {
                    auto member = make_ptr<Properties>();
                    DataUtil::parse( DataUtil::toString( settings.get() ), member.get() );
                    member->setProperty( "build.architecture.android", abi );
                    member->setProperty( "build.android.universalStaging", true );
                    member->setProperty( "build.android.stageOnly", abi < 3 );
                    const auto args = prepareProjectSettingsBuild( project, member );
                    project->compile( configuration( *member ), args, false );
                }
                writeText( outputDirectory( project, *settings ) / "ProjectRuntime.settings",
                           DataUtil::toString( settings.get(), true ) );
            }
            else
            {
                const auto args = prepareProjectSettingsBuild( project, settings );
                project->compile( configuration( *settings ), args, false );
            }
            auto app = core::IApplicationManager::instancePtr();
            if( settings->getPropertyAsBool( "build.assetBundles", true ) )
            {
                auto packages = app ? app->getPackageManager() : nullptr;
                if( !packages ) throw std::runtime_error( "Resource packaging requires the package manager" );
                if( packages->getResourcePackageProfile().empty() )
                    throw std::runtime_error( "Select a checked Resource Package profile before building asset bundles" );
                if( !packages->startResourcePackage() ) throw std::runtime_error( packages->getResourcePackageDiagnostic().c_str() );
                auto result = resultFor( true, "Plugin built. Checked asset packaging is running; inspect Resource Packages for completion." );
                result->setProperty( "pending", true ); return result;
            }
            const auto root = fs::canonical( fs::u8path( project->getPath().c_str() ) );
            const auto output = outputDirectory( project, *settings );
            Array<StringW> files;
            for( const auto &entry : fs::directory_iterator( output ) )
                if( entry.is_regular_file() && entry.path().extension() != ".zip" &&
                    ( settings->getPropertyAsBool( "build.debugSymbols", true ) || entry.path().extension() != ".pdb" ) )
                    files.push_back( entry.path().wstring().c_str() );
            for( const auto &file : stageRuntimeSupportFiles( root, output ) ) files.push_back( file.wstring().c_str() );
            for( const char *folder : { "native", "Android" } )
            {
                const auto artifacts = output / folder;
                if( !fs::is_directory( artifacts ) ) continue;
                for( const auto &entry : fs::recursive_directory_iterator( artifacts ) )
                {
                    if( entry.is_symlink() ) throw std::runtime_error( "Build artifacts cannot be redirected" );
                    if( entry.is_regular_file() )
                    {
                        const auto checked = checkedMember( output, entry.path().lexically_relative( output ).generic_u8string().c_str() );
                        files.push_back( checked.wstring().c_str() );
                    }
                }
            }
            for( const auto &scene : scenePaths( *settings ) )
            {
                const auto source = checkedMember( root, scene );
                if( !fs::is_regular_file( source ) ) throw std::runtime_error( ( "Packaged scene is missing: " + scene ).c_str() );
                const auto destination = output / fs::u8path( scene.c_str() );
                fs::create_directories( destination.parent_path() ); fs::copy_file( source, destination, fs::copy_options::overwrite_existing );
                files.push_back( destination.wstring().c_str() );
            }
            if( settings->getPropertyAsBool( "build.includeEditorData" ) )
            {
                const auto destination = output / "Editor" / "project.fbproject";
                fs::create_directories( destination.parent_path() ); fs::copy_file( fs::u8path( project->getFilePath().c_str() ), destination, fs::copy_options::overwrite_existing );
                files.push_back( destination.wstring().c_str() );
            }
            const auto compression = settings->getPropertyAsInt( "build.compression", 1 );
            constexpr int levels[] = { 0, 1, 6, 9 };
            if( compression < 0 || compression > 3 ) throw std::invalid_argument( "Invalid build compression" );
            const auto archive = output / "Project.zip";
            const auto code = ZipUtil::createZipFileFromPath( archive.wstring().c_str(), files, output.wstring().c_str(), true, levels[compression] );
            if( code != 0 || !fs::is_regular_file( archive ) ) throw std::runtime_error( "Unable to produce project archive" );
            auto result = resultFor( true, "Project build and selected scene archive completed." );
            result->setProperty( "path", archive.u8string().c_str() ); return result;
        }
        catch( const std::exception &e ) { return resultFor( false, e.what() ); }
    }
}
