#include <EditorPCH.hpp>
#include <editor/ProjectSettingsActions.hpp>
#include <editor/ProjectSettingsRuntime.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
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
            auto settings = workphone::make_ptr<Properties>();
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

        void fillResponse( Properties &response, const Properties &settings )
        {
            for( const auto &definition : settingDefinitions )
            {
                response.addProperty( settings.getPropertyObject( definition.name ) );
                response.setProperty( String( "capability." ) + definition.name, false );
                response.setProperty( String( "unavailable." ) + definition.name,
                                      String( "The current engine has no consumer for this setting." ) );
            }
            describeProjectSettingsCapabilities( &response );
            for( const char *key : { "project.name", "project.company", "project.productName",
                                    "project.version", "project.bundleId", "project.startupScene",
                                    "project.autoLoadLastScene", "platform.active",
                                    "build.architecture.pc" } )
                response.setProperty( String( "capability." ) + key, true );
            response.setProperty( "availability.platform.active",
                                  String( "The desktop plugin builder reports an error for unsupported platform targets." ) );
            response.setProperty( "availability.build.architecture.pc",
                                  String( "Project plugins must match the running Editor architecture." ) );
            response.setProperty( "unavailable.project.defaultLanguage",
                                  String( "The engine has no project localization service." ) );
            response.setProperty( "unavailable.project.sceneList",
                                  String( "The desktop plugin builder does not package scenes." ) );
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
            settings->setProperty( "projectSettingsVersion", 1 );
            try
            {
                std::ofstream stream( temporary, std::ios::binary | std::ios::trunc );
                stream.exceptions( std::ios::badbit | std::ios::failbit );
                const auto text = DataUtil::toString( settings.get(), true );
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
                request->setProperty( "status", String( "Project settings loaded. Unavailable controls identify missing engine support." ) );
            }
            else if( action == "importProjectSettingsJson" )
            {
                const auto path = chooseSettingsPath( request, project, false );
                if( path.empty() ) { request->setProperty( "status", String( "Import cancelled." ) ); return; }
                mergeSettings( *settings, *importSettings( path ) );
                fillResponse( *request, *settings );
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
                    request->setProperty( "status", String( "Project settings values are valid. Unavailable controls require engine support." ) );
                }
                else if( action == "reloadRenderer" )
                {
                    throw std::runtime_error( "Renderer hot reload is unavailable. Save settings and launch the game again to switch its graphics backend." );
                }
                else if( action == "applyProjectSettings" || action == "saveProjectSettings" )
                {
                    auto result = applyProjectSettingsRuntime( settings );
                    if( !result->getPropertyAsBool( "success" ) )
                        throw std::runtime_error( result->getProperty( "status" ).c_str() );
                    auto properties = project->getProperties();
                    mergeSettings( *properties, *settings );
                    project->setProperties( properties );
                    if( action == "saveProjectSettings" ) project->save();
                    auto status = result->getProperty( "status" );
                    if( action == "saveProjectSettings" ) status = "Saved project settings. " + status;
                    request->setProperty( "status", status );
                }
                else if( action == "buildSelectedPlatform" )
                {
                    if( settings->getPropertyAsInt( "platform.active" ) != 0 )
                        throw std::runtime_error( "This Editor builds desktop project plugins. macOS, iOS and Android packaging is unavailable on this host." );
                    const auto architecture = settings->getPropertyAsInt( "build.architecture.pc" );
#if defined( _WIN64 )
                    if( architecture != 1 )
                        throw std::runtime_error( "This Editor requires an x64 project plugin." );
#endif
                    auto result = applyProjectSettingsRuntime( settings );
                    if( !result->getPropertyAsBool( "success" ) )
                        throw std::runtime_error( result->getProperty( "status" ).c_str() );
                    auto properties = project->getProperties();
                    mergeSettings( *properties, *settings );
                    project->setProperties( properties );
                    project->save();
                    project->compile();
                    request->setProperty( "status", String( "Desktop project plugin built and loaded. Logs: Cache/project." ) );
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
