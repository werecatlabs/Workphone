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
#include <set>
#include <stdexcept>

namespace workphone::editor
{
    namespace
    {
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

    SmartPtr<Properties> applyProjectSettingsRuntime( SmartPtr<Properties> settings )
    {
        auto result = validateProjectSettingsRuntime( settings );
        if( !result->getPropertyAsBool( "success" ) ) return result;
        auto app = core::IApplicationManager::instancePtr();
        auto immutable = snapshot( *settings );
        auto previous = app ? app->getPlayerSettings() : nullptr;
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
            if( project ) updateProjectGraphicsSettings( immutable, project->getGraphicsSettingsDirector() );
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
}
