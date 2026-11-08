#include <EditorPCH.hpp>
#include <editor/ProjectSettingsRuntime.hpp>
#include <editor/ProjectSettingsGraphicsBackend.hpp>
#include <editor/ProjectSettingsInteractionBackend.hpp>
#include <editor/ProjectSettingsProjectBackend.hpp>
#include <editor/ProjectSettingsEditorBackend.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Directors/GraphicsSettingsDirector.hpp>
#if WP_BUILD_PHYSICS3
#include <WPPhysics/WPPhysicsUtil.hpp>
#endif
#include <set>
#include <stdexcept>
#include <cstdlib>
#include <cerrno>
#include <cmath>
#include <cstring>

namespace workphone::editor
{
    namespace
    {
        struct SettingDefinition
        {
            const char *name;
            const char *type;
            const char *defaultValue;
            double minimum, maximum;
            bool bounded;
        };
#include "ProjectSettingsSchema.inl"

        String validateValues( const Properties &settings )
        {
            for( const auto &definition : settingDefinitions )
            {
                if( !settings.hasProperty( definition.name ) ) continue;
                const auto value = settings.getProperty( definition.name );
                bool valid = value.find( '\0' ) == String::npos && value.size() <= 65536;
                if( std::strcmp( definition.type, "bool" ) == 0 )
                    valid = valid && ( value == "true" || value == "false" || value == "0" || value == "1" );
                else if( std::strcmp( definition.type, "string" ) != 0 )
                {
                    char *end = nullptr;
                    errno = 0;
                    const auto number = std::strtod( value.c_str(), &end );
                    valid = valid && end != value.c_str() && end && !*end && errno != ERANGE &&
                        std::isfinite( number ) && ( !definition.bounded ||
                        ( number >= definition.minimum && number <= definition.maximum ) ) &&
                        ( std::strcmp( definition.type, "int" ) != 0 || number == std::floor( number ) );
                }
                if( !valid ) return String( "Invalid project setting: " ) + definition.name;
            }
            return {};
        }

        SmartPtr<Properties> snapshot( const Properties &source )
        {
            auto result = make_ptr<Properties>();
            result->setName( source.getName() );
            for( const auto &property : source.getPropertiesAsArray() ) result->addProperty( property );
            for( auto child : source.getChildren() ) if( child ) result->addChild( snapshot( *child ) );
            return result;
        }

        void append( String &destination, const String &value )
        {
            if( value.empty() ) return;
            if( !destination.empty() ) destination += "; ";
            destination += value;
        }
    }

    void describeProjectSettingsCapabilities( SmartPtr<Properties> response )
    {
        if( !response ) return;
        describeProjectSettingsGraphicsBackend( response );
        describeProjectSettingsInteractionCapabilities( response );
        describeProjectSettingsProjectBackend( response );
        describeProjectSettingsEditorBackend( response );
    }

    SmartPtr<Properties> validateProjectSettingsRuntime( SmartPtr<Properties> settings )
    {
        auto response = make_ptr<Properties>();
        response->setProperty( "success", false );
        if( !settings )
        {
            response->setProperty( "status", "Project settings are unavailable." );
            return response;
        }
        const auto valuesError = validateValues( *settings );
        if( !valuesError.empty() )
        {
            response->setProperty( "status", valuesError );
            return response;
        }
        using Validator = SmartPtr<Properties> ( * )( SmartPtr<Properties> );
        for( auto validate : { Validator( validateProjectSettingsGraphicsBackend ),
                               Validator( validateProjectSettingsInteraction ),
                               Validator( validateProjectSettingsProjectBackend ),
                               Validator( validateProjectSettingsEditorBackend ) } )
        {
            auto result = validate( settings );
            if( !result || !result->getPropertyAsBool( "success" ) )
            {
                response->setProperty( "status", result ? result->getProperty( "status" ) :
                    String( "The settings backend did not return a validation result." ) );
                return response;
            }
        }
        response->setProperty( "success", true );
        response->setProperty( "status", "Project settings validated against the selected backends." );
        return response;
    }

    void updateProjectGraphicsSettings( SmartPtr<Properties> settings,
                                       SmartPtr<scene::GraphicsSettingsDirector> director )
    {
        if( !settings || !director ) return;
        auto result = validateProjectSettingsGraphicsBackend( settings );
        if( !result->getPropertyAsBool( "success" ) )
            throw std::invalid_argument( result->getProperty( "status" ).c_str() );
        auto properties = projectGraphicsBackendProperties( settings );
        director->setProperties( properties );
        director->setProperties( properties );
    }

    static SmartPtr<Properties> applyRuntimeProfile( SmartPtr<Properties> settings, bool replace,
                                                    SmartPtr<Properties> graphicsReplacement = nullptr )
    {
        if( !settings ) return validateProjectSettingsRuntime( settings );
        auto app = core::IApplicationManager::instancePtr();
        auto previous = app ? app->getPlayerSettings() : nullptr;
        auto immutable = previous && !replace ? snapshot( *previous ) : make_ptr<Properties>();
        for( const auto &property : settings->getPropertiesAsArray() ) immutable->addProperty( property );
        for( auto child : settings->getChildren() ) if( child )
        {
            immutable->removeChild( child->getName() );
            immutable->addChild( snapshot( *child ) );
        }
        auto result = validateProjectSettingsRuntime( immutable );
        if( !result->getPropertyAsBool( "success" ) ) return result;
        if( app ) app->setPlayerSettings( immutable );
        String applied, pending, requirements;
        try
        {
            using Backend = SmartPtr<Properties> ( * )( SmartPtr<Properties> );
            for( auto apply : { Backend( applyProjectSettingsGraphicsBackend ),
                                Backend( applyProjectSettingsInteraction ),
                                Backend( applyProjectSettingsProjectBackend ),
                                Backend( applyProjectSettingsEditorBackend ) } )
            {
                auto reply = apply( immutable );
                if( !reply || !reply->getPropertyAsBool( "success" ) )
                    throw std::runtime_error( reply ? reply->getProperty( "status" ).c_str() :
                                               "The settings backend did not return an apply result." );
                append( applied, reply->getProperty( "appliedKeys" ) );
                append( pending, reply->getProperty( "pendingKeys" ) );
                append( requirements, reply->getProperty( "externalRequirements" ) );
            }
            auto editor = EditorManager::getSingletonPtr();
            auto project = editor ? editor->getProject() : nullptr;
            if( graphicsReplacement )
            {
                if( app ) if( auto graphics = app->getGraphicsSystem() )
                    if( auto pipeline = graphics->getGraphicsPipeline() )
                    {
                        auto replacement = snapshot( *graphicsReplacement );
                        replacement->setProperty( "Replace project settings", true );
                        pipeline->setProperties( replacement );
                        pipeline->setProperties( graphicsReplacement );
                    }
                if( project )
                {
                    auto director = project->getGraphicsSettingsDirector();
                    director->setProperties( graphicsReplacement );
                    director->setProperties( graphicsReplacement );
                }
            }
            else if( project ) updateProjectGraphicsSettings( immutable, project->getGraphicsSettingsDirector() );
            if( app )
            {
                auto tasks = app->getTaskManager();
                if( tasks && immutable->hasProperty( "graphics.frameRateLimit" ) )
                {
                    for( auto id : { TaskId::Primary, TaskId::Application, TaskId::Render } )
                        if( auto task = tasks->getTask( id ) )
                            task->setTargetFPS( immutable->getPropertyAsInt( "graphics.frameRateLimit", 60 ) );
                    append( applied, "graphics.frameRateLimit" );
                }
                if( tasks && immutable->hasProperty( "physics.simulationRate" ) )
                    if( auto task = tasks->getTask( TaskId::Physics ) )
                    {
                        task->setTargetFPS( immutable->getPropertyAsInt( "physics.simulationRate", 60 ) );
                        append( applied, "physics.simulationRate" );
                    }
            }
            result->setProperty( "success", true );
            result->setProperty( "appliedKeys", applied );
            result->setProperty( "pendingKeys", pending );
            result->setProperty( "externalRequirements", requirements );
            result->setProperty( "unavailableKeys", String() );
            String status = app ? "Project backend settings applied." :
                                  "Project settings stored; runtime is not loaded.";
            if( !pending.empty() ) status += " Startup/restart settings: " + pending + ".";
            if( !requirements.empty() ) status += " Backend requirements: " + requirements + ".";
            result->setProperty( "status", status );
        }
        catch( const std::exception &error )
        {
            if( app ) app->setPlayerSettings( previous );
            result->setProperty( "success", false );
            result->setProperty( "status", String( error.what() ) );
        }
        return result;
    }

    SmartPtr<Properties> applyProjectSettingsRuntime( SmartPtr<Properties> settings )
    {
        return applyRuntimeProfile( settings, false );
    }

    SmartPtr<Properties> loadProjectSettingsRuntime( SmartPtr<Properties> settings )
    {
        if( !settings ) return validateProjectSettingsRuntime( settings );
        auto fresh = snapshot( *settings );
        if( fresh->getPropertyAsInt( "physics.broadphase" ) == 3 )
        {
#if WP_BUILD_PHYSICS3
            const bool hasProvider = physics::hasBroadphaseProvider();
#else
            constexpr bool hasProvider = false;
#endif
            if( !hasProvider )
            {
                fresh->setProperty( "physics.broadphase", 1 );
                WP_LOG_WARNING( "Saved GPU Broadphase provider is unavailable. Using Dynamic AABB Tree." );
            }
        }
        for( const auto &definition : settingDefinitions )
        {
            const String name( definition.name );
            if( fresh->hasProperty( name ) ) continue;
            // Missing legacy graphics keys must not opt the viewport into HDR,
            // resize its window, select a device or overwrite its saved preset.
            // Missing themes likewise retain the host's existing ImGui style.
            if( ( name.find( "graphics." ) == 0 && name != "graphics.frameRateLimit" ) ||
                name == "quality.preset" || name == "editor.theme" ) continue;
            if( std::strcmp( definition.type, "bool" ) == 0 )
                fresh->setProperty( name, std::strcmp( definition.defaultValue, "true" ) == 0 );
            else if( std::strcmp( definition.type, "int" ) == 0 )
                fresh->setProperty( name, static_cast<s32>( std::strtol( definition.defaultValue, nullptr, 10 ) ) );
            else if( std::strcmp( definition.type, "float" ) == 0 )
                fresh->setProperty( name, static_cast<f32>( std::strtod( definition.defaultValue, nullptr ) ) );
            else fresh->setProperty( name, String( definition.defaultValue ) );
        }
        auto legacyGraphics = settings->getChild( "Game Graphics" );
        auto replacement = legacyGraphics ? snapshot( *legacyGraphics ) :
            make_ptr<scene::GraphicsSettingsDirector>()->getProperties();
        // Saved friendly graphics fields are restored first; explicit namespaced
        // B settings then take precedence over those legacy values.
        for( const auto &property : projectGraphicsBackendProperties( fresh )->getPropertiesAsArray() )
            replacement->addProperty( property );
        return applyRuntimeProfile( fresh, true, replacement );
    }
}
