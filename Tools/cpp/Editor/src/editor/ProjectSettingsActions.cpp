#include <EditorPCH.hpp>
#include <editor/ProjectSettingsActions.hpp>
#include <editor/ProjectSettingsRuntime.hpp>
#include <editor/ProjectSettingsProjectBackend.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#if defined( _WIN32 )
#    include <windows.h>
#endif

namespace workphone::editor
{
    namespace
    {
        namespace fs = std::filesystem;

        struct SettingDefinition
        {
            const char *name;
            const char *type;
            const char *defaultValue;
            double minimum;
            double maximum;
            bool bounded;
        };

#include "ProjectSettingsSchema.inl"

        void setTypedValue( Properties &destination, const SettingDefinition &definition,
                            const String &value )
        {
            const auto invalid = [&]() {
                return std::invalid_argument( String( "Invalid project setting: " ) + definition.name );
            };
            if( std::strcmp( definition.type, "string" ) == 0 )
            {
                if( value.find( '\0' ) != String::npos || value.size() > 65536 )
                    throw invalid();
                destination.setProperty( definition.name, value );
            }
            else if( std::strcmp( definition.type, "bool" ) == 0 )
            {
                if( value != "true" && value != "false" && value != "0" && value != "1" )
                    throw invalid();
                destination.setProperty( definition.name, value == "true" || value == "1" );
            }
            else
            {
                size_t consumed = 0;
                double number = 0;
                try
                {
                    number = std::stod( std::string( value.c_str() ), &consumed );
                }
                catch( const std::exception & )
                {
                    throw invalid();
                }
                if( consumed != value.size() || !std::isfinite( number ) ||
                    ( definition.bounded &&
                      ( number < definition.minimum || number > definition.maximum ) ) )
                    throw invalid();
                if( std::strcmp( definition.type, "int" ) == 0 )
                {
                    if( number != std::floor( number ) || number < -2147483648.0 ||
                        number > 2147483647.0 )
                        throw invalid();
                    destination.setProperty( definition.name, static_cast<s32>( number ) );
                }
                else
                {
                    destination.setProperty( definition.name, static_cast<f32>( number ) );
                }
            }
        }

        SmartPtr<Properties> settingsFromProject( SmartPtr<Project> project )
        {
            auto source = project->getProperties();
            auto settings = project->getProperties();
            for( const auto &definition : settingDefinitions )
            {
                auto value = source->hasProperty( definition.name )
                                 ? source->getProperty( definition.name )
                                 : String( definition.defaultValue );
                // Older projects used these metadata names before the settings panel.
                if( !source->hasProperty( definition.name ) )
                {
                    if( String( definition.name ) == "project.name" ) value = project->getLabel();
                    if( String( definition.name ) == "project.company" ) value = source->getProperty( "companyName" );
                    if( String( definition.name ) == "project.productName" ) value = source->getProperty( "productName" );
                }
                setTypedValue( *settings, definition, value );
            }
            return settings;
        }

        void describeInheritedGraphics( Properties &response, SmartPtr<Project> project,
                                        const Properties *imported = nullptr )
        {
            const auto projectSettings = project->getProperties();
            const auto app = core::IApplicationManager::instancePtr();
            const auto graphics = app ? app->getGraphicsSystem() : nullptr;
            const auto pipeline = graphics ? graphics->getGraphicsPipeline() : nullptr;
            const auto pipelineValues = pipeline ? pipeline->getProperties() : nullptr;
            const auto nativeValues = graphics ? graphics->getProperties() : nullptr;
            const auto window = graphics ? graphics->getDefaultWindow() : nullptr;
            const auto activeGraphics = app ? app->getGraphicsSettings() : nullptr;
            const auto size = window ? window->getSize() : Vector2I();
            const bool windowed = window && !window->isFullScreen() && size.x > 0 && size.y > 0;
            s32 sampleMask = 0;
            const bool direct3D = nativeValues && nativeValues->getPropertyValue( "msaa_supported_mask", sampleMask );
            const bool hdr = pipelineValues && pipelineValues->getPropertyAsBool(
                "graphics.hdr", pipelineValues->getPropertyAsInt( "graphics.tonemapping" ) != 0 );
            const bool hasPreset = pipelineValues && pipelineValues->getPropertyAsInt( "quality.preset", 4 ) < 4;
            const bool inheritEffects = pipelineValues &&
                ( hasPreset || pipelineValues->hasProperty( "graphics.postProcessing" ) );
            if( direct3D )
            {
                for( s32 choice = 3; choice <= 5; ++choice )
                {
                    const auto option = String( "graphics.antiAliasing." ) + std::to_string( choice );
                    const bool available = ( sampleMask & ( 1 << ( choice - 2 ) ) ) != 0;
                    response.setProperty( "capability." + option, available );
                    if( !available )
                        response.setProperty( "unavailable." + option,
                                              "The active device does not support this MSAA sample count." );
                }
            }

            for( const auto &definition : settingDefinitions )
            {
                const String key( definition.name );
                if( key.find( "graphics." ) != 0 || key.find( "graphics.renderer." ) == 0 ||
                    projectSettings->hasProperty( key ) || ( imported && imported->hasProperty( key ) ) )
                    continue;

                response.setProperty( "inherited." + key, true );
                if( window && key == "graphics.fullscreen" )
                    response.setProperty( key, window->isFullScreen() );
                else if( windowed && key == "graphics.resolution" )
                    response.setProperty( key, 5 );
                else if( windowed && key == "graphics.customWidth" )
                    response.setProperty( key, std::clamp( size.x, 320, 8192 ) );
                else if( windowed && key == "graphics.customHeight" )
                    response.setProperty( key, std::clamp( size.y, 200, 8192 ) );
                else if( nativeValues && key == "graphics.vsync" && nativeValues->hasProperty( "vsync" ) )
                    response.setProperty( key, nativeValues->getPropertyAsBool( "vsync" ) );
                else if( pipelineValues && pipelineValues->hasProperty( key ) )
                {
                    // A legacy graphics child may contain a valid namespaced value.
                    // Keep project/schema validation independent from inherited UI values.
                    try { setTypedValue( response, definition, pipelineValues->getProperty( key ) ); }
                    catch( const std::invalid_argument & ) {}
                }
                else if( direct3D )
                {
                    // The legacy Editor viewport renders in display space until an
                    // explicit colour policy opts into the scene resolve.
                    if( key == "graphics.hdr" ) response.setProperty( key, hdr );
                    else if( key == "graphics.colorSpace" ) response.setProperty( key, hdr ? 1 : 0 );
                    else if( key == "graphics.tonemapping" ) response.setProperty( key, hdr ? 3 : 0 );
                    else if( key == "graphics.renderPipeline" ) response.setProperty( key, 0 );
                    else if( key == "graphics.antiAliasing" )
                        response.setProperty( key, hasPreset && pipelineValues->getPropertyAsBool( "Temporal anti-aliasing" ) ? 2 : 0 );
                    else if( key == "graphics.bloom" )
                        response.setProperty( key, inheritEffects && pipelineValues->getPropertyAsBool( "Bloom" ) );
                    else if( key == "graphics.ambientOcclusion" )
                        response.setProperty( key, inheritEffects && pipelineValues->getPropertyAsBool( "Ambient occlusion" ) );
                    else if( key == "graphics.motionBlur" )
                        response.setProperty( key, inheritEffects && pipelineValues->getPropertyAsBool( "Motion blur" ) );
                    else if( key == "graphics.shadowQuality" && activeGraphics )
                        response.setProperty( key, activeGraphics->getShadowQuality() );
                    else if( key == "graphics.shadowResolution" && activeGraphics )
                        response.setProperty( key, activeGraphics->getShadowResolution() );
                    else if( key == "graphics.shadowDistance" && activeGraphics )
                        response.setProperty( key, activeGraphics->getShadowDistance() );
                }
            }
        }

        void mergeSettings( Properties &destination, const Properties &source )
        {
            for( const auto &definition : settingDefinitions )
            {
                if( source.hasProperty( definition.name ) )
                    setTypedValue( destination, definition, source.getProperty( definition.name ) );
            }
            if( destination.getPropertyAsInt( "build.android.minSdk" ) >
                destination.getPropertyAsInt( "build.android.targetSdk" ) )
                throw std::invalid_argument( "Minimum Android SDK exceeds Target Android SDK" );
        }

        String applySettingsPatch( SmartPtr<Project> project, const Properties &request,
                                   const Properties &settings )
        {
            auto patch = make_ptr<Properties>();
            for( const auto &definition : settingDefinitions )
                if( request.hasProperty( definition.name ) )
                    patch->addProperty( settings.getPropertyObject( definition.name ) );
            if( patch->getNumProperties() == 0 ) return {};

            if( patch->getPropertyAsInt( "graphics.resolution", -1 ) == 5 )
            {
                for( const auto *key : { "graphics.customWidth", "graphics.customHeight" } )
                    if( !patch->hasProperty( key ) )
                        patch->addProperty( settings.getPropertyObject( key ) );
            }
            auto result = applyProjectSettingsRuntime( patch );
            if( !result->getPropertyAsBool( "success" ) )
                throw std::runtime_error( result->getProperty( "status" ).c_str() );
            auto properties = project->getProperties();
            for( const auto &property : patch->getPropertiesAsArray() ) properties->addProperty( property );
            project->setProperties( properties );
            return result->getProperty( "status" );
        }

        void fillResponse( Properties &response, const Properties &settings )
        {
            response.setProperty( "settingsSnapshot", true );
            for( const auto &definition : settingDefinitions )
            {
                response.addProperty( settings.getPropertyObject( definition.name ) );
                response.setProperty( String( "capability." ) + definition.name, true );
            }
            describeProjectSettingsCapabilities( &response );
            for( const char *key : { "project.name", "project.company", "project.productName",
                                    "project.version", "project.bundleId", "project.startupScene",
                                    "project.autoLoadLastScene", "platform.active",
                                    "build.architecture.pc" } )
                response.setProperty( String( "capability." ) + key, true );
            response.setProperty( "availability.platform.active",
                                  String( "Builds use the selected architecture and installed platform toolchain." ) );
        }

        String chooseSettingsPath( SmartPtr<Properties> request, SmartPtr<Project> project,
                                   bool save )
        {
            auto path = request->getProperty( "path" );
            if( !path.empty() ) return path;
            auto app = core::IApplicationManager::instancePtr();
            auto files = app ? app->getFileSystem() : nullptr;
            auto dialog = files ? files->openFileDialog() : nullptr;
            if( !dialog ) throw std::runtime_error( "The settings file dialog is unavailable" );
            dialog->setDialogMode( save ? INativeFileDialog::DialogMode::Save
                                       : INativeFileDialog::DialogMode::Open );
            dialog->setFileExtension( ".json" );
            dialog->setFilePath( project->getPath() );
            const auto result = dialog->openDialog();
            if( result == INativeFileDialog::Result::Dialog_Cancel ) return {};
            if( result != INativeFileDialog::Result::Dialog_Okay )
                throw std::runtime_error( "Unable to select a settings file" );
            path = dialog->getFilePath();
            if( save && fs::u8path( path.c_str() ).extension().empty() ) path += ".json";
            return path;
        }

        void exportSettings( const String &path, SmartPtr<Properties> settings )
        {
            const auto destination = fs::absolute( fs::u8path( path.c_str() ) ).lexically_normal();
            auto temporary = destination;
            temporary += std::string( ( "." + StringUtil::getUUID() + ".tmp" ).c_str() );
            auto document = workphone::make_ptr<Properties>();
            document->setProperty( "projectSettingsVersion", 1 );
            for( const auto &definition : settingDefinitions )
                if( settings->hasProperty( definition.name ) )
                    document->addProperty( settings->getPropertyObject( definition.name ) );
            try
            {
                std::ofstream stream( temporary, std::ios::binary | std::ios::trunc );
                stream.exceptions( std::ios::badbit | std::ios::failbit );
                const auto text = DataUtil::toString( document.get(), true );
                stream.write( text.data(), static_cast<std::streamsize>( text.size() ) );
                stream.close();
#if defined( _WIN32 )
                if( !MoveFileExW( temporary.c_str(), destination.c_str(),
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                    throw std::runtime_error( "Unable to replace exported settings" );
#else
                fs::rename( temporary, destination );
#endif
            }
            catch( ... )
            {
                std::error_code ignored;
                fs::remove( temporary, ignored );
                throw;
            }
        }

        SmartPtr<Properties> importSettings( const String &path )
        {
            std::ifstream stream( fs::u8path( path.c_str() ), std::ios::binary );
            if( !stream ) throw std::runtime_error( "Unable to open settings JSON" );
            stream.seekg( 0, std::ios::end );
            if( stream.tellg() < 0 || stream.tellg() > 1024 * 1024 )
                throw std::runtime_error( "Settings JSON exceeds 1 MiB" );
            stream.seekg( 0 );
            std::ostringstream buffer;
            buffer << stream.rdbuf();
            const auto text = String( buffer.str().c_str() );
            if( stream.bad() || !DataUtil::isValidData( text ) )
                throw std::runtime_error( "Invalid settings JSON" );
            auto imported = workphone::make_ptr<Properties>();
            DataUtil::parse( text, imported.get() );
            if( imported->getPropertyAsInt( "projectSettingsVersion" ) != 1 ||
                !imported->getChildren().empty() )
                throw std::invalid_argument( "Unsupported project settings document" );
            for( const auto &property : imported->getPropertiesAsArray() )
            {
                if( property.getName() == "projectSettingsVersion" ) continue;
                bool known = false;
                for( const auto &definition : settingDefinitions )
                    if( property.getName() == definition.name ) { known = true; break; }
                if( !known ) throw std::invalid_argument( "Unknown project setting: " + property.getName() );
            }
            return imported;
        }
    }

    void processProjectSettingsAction( SmartPtr<Properties> request )
    {
        if( !request ) return;
        request->setProperty( "success", false );
        try
        {
            const auto editor = EditorManager::getSingletonPtr();
            auto project = editor ? editor->getProject() : nullptr;
            if( !project ) throw std::runtime_error( "Open a project before editing project settings" );
            const auto action = request->getProperty( "action" );
            auto settings = settingsFromProject( project );
            if( action == "readProjectSettings" )
            {
                fillResponse( *request, *settings );
                describeInheritedGraphics( *request, project );
                request->setProperty( "status", String( "Project settings loaded. Validate checks backend and toolchain requirements." ) );
            }
            else if( action == "importProjectSettingsJson" )
            {
                const auto path = chooseSettingsPath( request, project, false );
                if( path.empty() ) { request->setProperty( "status", String( "Import cancelled." ) ); return; }
                const auto imported = importSettings( path );
                mergeSettings( *settings, *imported );
                fillResponse( *request, *settings );
                describeInheritedGraphics( *request, project, imported.get() );
                for( const auto &definition : settingDefinitions )
                    if( imported->hasProperty( definition.name ) )
                        request->setProperty( String( "included." ) + definition.name, true );
                request->setProperty( "status", String( "Imported settings draft. Apply or Save to commit." ) );
            }
            else
            {
                mergeSettings( *settings, *request );
                if( action == "exportProjectSettingsJson" )
                {
                    const auto path = chooseSettingsPath( request, project, true );
                    if( path.empty() ) { request->setProperty( "status", String( "Export cancelled." ) ); return; }
                    exportSettings( path, settings );
                    request->setProperty( "status", "Exported project settings to " + path );
                }
                else if( action == "validateProjectSettings" )
                {
                    auto result = validateProjectSettingsRuntime( settings );
                    if( !result->getPropertyAsBool( "success" ) )
                        throw std::invalid_argument( result->getProperty( "status" ).c_str() );
                    request->setProperty( "status", result->getProperty( "status" ) );
                    if( settings->getPropertyAsBool( "editor.advancedWarnings", true ) )
                    {
                        String warnings;
                        if( settings->getPropertyAsInt( "build.config", 1 ) == 3 &&
                            settings->getProperty( "build.signingProfile", "Default" ) == "Default" )
                            warnings += "Shipping uses the default signing profile. ";
                        if( settings->getPropertyAsInt( "quality.streamingBudgetMb", 512 ) >
                            settings->getPropertyAsInt( "performance.memoryBudgetMb", 2048 ) )
                            warnings += "Streaming budget exceeds the total CPU asset budget. ";
                        request->setProperty( "warnings", warnings );
                        if( !warnings.empty() )
                            request->setProperty( "status", result->getProperty( "status" ) + " " + warnings );
                    }
                }
                else if( action == "reloadRenderer" )
                {
                    auto result = applyProjectSettingsRuntime( settings );
                    if( !result->getPropertyAsBool( "success" ) )
                        throw std::runtime_error( result->getProperty( "status" ).c_str() );
                    auto properties = project->getProperties();
                    mergeSettings( *properties, *settings );
                    project->setProperties( properties );
                    request->setProperty( "status", "Render settings reapplied. Changing the device API takes effect at next launch." );
                }
                else if( action == "applyProjectSettings" || action == "saveProjectSettings" )
                {
                    auto status = applySettingsPatch( project, *request, *settings );
                    if( action == "saveProjectSettings" )
                    {
                        project->save();
                        status = status.empty() ? String( "Saved project settings." )
                                                : "Saved project settings. " + status;
                    }
                    else if( status.empty() ) status = "No project settings changed.";
                    fillResponse( *request, *settings );
                    describeInheritedGraphics( *request, project );
                    request->setProperty( "status", status );
                }
                else if( action == "buildSelectedPlatform" )
                {
                    auto result = validateProjectSettingsRuntime( settings );
                    if( !result->getPropertyAsBool( "success" ) )
                        throw std::runtime_error( result->getProperty( "status" ).c_str() );
                    applySettingsPatch( project, *request, *settings );
                    project->save();
                    auto build = buildProjectSettingsProjectBackend( settings );
                    if( !build->getPropertyAsBool( "success" ) )
                        throw std::runtime_error( build->getProperty( "status" ).c_str() );
                    fillResponse( *request, *settings );
                    describeInheritedGraphics( *request, project );
                    request->setProperty( "status", build->getProperty( "status" ) );
                    request->setProperty( "path", build->getProperty( "path" ) );
                    request->setProperty( "pending", build->getPropertyAsBool( "pending", false ) );
                }
                else throw std::invalid_argument( "Unknown project settings action: " + action );
            }
            request->setProperty( "success", true );
        }
        catch( const std::exception &exception )
        {
            request->setProperty( "success", false );
            request->setProperty( "status", String( exception.what() ) );
        }
    }
}
