#include <EditorPCH.hpp>
#include <editor/ProjectSettingsRuntime.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Directors/GraphicsSettingsDirector.hpp>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <set>
#include <stdexcept>

namespace workphone::editor
{
    namespace
    {
        struct Capability
        {
            const char *key;
            const char *description;
        };

        // Keep this list restricted to settings with an actual consumer. Backend API setters
        // which only store a value (for example native physics thread counts) are excluded.
        const Capability runtimeCapabilities[] = {
            { "graphics.frameRateLimit", "Applied to the primary, application and render task rates." },
            { "physics.gravity", "Applied to the active physics scene." },
            { "physics.simulationRate", "Applied to the physics task rate; simulation uses elapsed time." },
            { "physics.debugDraw", "Applied to the active physics backend." },
            { "sound.masterVolume", "Applied to the active sound backend." }
        };

        const Capability graphicsCapabilities[] = {
            { "graphics.renderer.pc", "Auto preserves the Game Graphics API; DirectX 11 and Software select the next standalone renderer." },
            { "quality.preset", "Game software pipeline quality; Custom preserves the current preset and effect overrides." },
            { "graphics.antiAliasing", "None and TAA are supported by the game software pipeline; FXAA and MSAA are unavailable." },
            { "graphics.shadowQuality", "Off disables game software shadows; enabled quality choices use Shadow Resolution." },
            { "graphics.shadowResolution", "Controls the game software shadow map size." },
            { "graphics.postProcessing", "Controls the game software bloom, motion blur and ambient occlusion effects." },
            { "graphics.bloom", "Game software pipeline bloom; enabled when Post Processing is enabled." },
            { "graphics.motionBlur", "Game software pipeline motion blur; enabled when Post Processing is enabled." },
            { "graphics.ambientOcclusion", "Game software pipeline ambient occlusion; enabled when Post Processing is enabled." }
        };

        String join( const Array<String> &values )
        {
            String result;
            for( const auto &value : values )
            {
                if( !result.empty() )
                    result += "; ";
                result += value;
            }
            return result;
        }

        bool number( const Properties &settings, const char *key, double minimum, double maximum,
                     bool integer, String &error )
        {
            if( !settings.hasProperty( key ) )
                return true;
            const auto value = settings.getPropertyObject( key ).getValue();
            char *end = nullptr;
            errno = 0;
            const auto parsed = std::strtod( value.c_str(), &end );
            while( end && *end && std::isspace( static_cast<unsigned char>( *end ) ) )
                ++end;
            if( value.empty() || end == value.c_str() || !end || *end || errno == ERANGE ||
                !std::isfinite( parsed ) || parsed < minimum || parsed > maximum ||
                ( integer && parsed != std::floor( parsed ) ) )
            {
                error = String( "Invalid project setting: " ) + key;
                return false;
            }
            return true;
        }

        bool validate( const Properties &settings, String &error )
        {
            if( !number( settings, "graphics.frameRateLimit", 15, 240, true, error ) ||
                !number( settings, "physics.gravity", -100, 100, false, error ) ||
                !number( settings, "physics.simulationRate", 15, 240, true, error ) ||
                !number( settings, "sound.masterVolume", 0, 100, true, error ) ||
                !number( settings, "quality.preset", 0, 4, true, error ) ||
                !number( settings, "graphics.renderer.pc", 0, 5, true, error ) ||
                !number( settings, "graphics.antiAliasing", 0, 2, true, error ) ||
                !number( settings, "graphics.shadowQuality", 0, 4, true, error ) ||
                !number( settings, "graphics.shadowDistance", 0, 300, false, error ) ||
                !number( settings, "graphics.shadowResolution", 0, 3, true, error ) )
                return false;
            s32 renderer = 0;
            if( settings.getPropertyValue( "graphics.renderer.pc", renderer ) &&
                renderer >= 2 && renderer <= 4 )
            {
                error = "This project supports Auto, DirectX 11 or Software rendering.";
                return false;
            }
            s32 aa = 0;
            if( settings.getPropertyValue( "graphics.antiAliasing", aa ) && aa == 1 )
            {
                error = "FXAA is unavailable. Select None or TAA.";
                return false;
            }
            for( const char *key : { "physics.debugDraw", "graphics.postProcessing", "graphics.bloom",
                                    "graphics.motionBlur", "graphics.ambientOcclusion" } )
            {
                if( settings.hasProperty( key ) )
                {
                    const auto value = settings.getPropertyObject( key ).getValue();
                    if( value != "true" && value != "false" && value != "0" && value != "1" )
                    {
                        error = String( "Invalid boolean project setting: " ) + key;
                        return false;
                    }
                }
            }
            return true;
        }

        SmartPtr<Properties> graphicsProperties( const Properties &settings,
                                                const Properties &existing )
        {
            auto properties = workphone::make_ptr<Properties>();
            s32 value = 0;
            if( settings.getPropertyValue( "quality.preset", value ) && value < 4 )
                properties->setProperty( "Quality", value );
            if( settings.getPropertyValue( "graphics.renderer.pc", value ) && value == 1 )
                properties->setProperty( "Render API", 1 );
            else if( settings.getPropertyValue( "graphics.renderer.pc", value ) && value == 5 )
                properties->setProperty( "Render API", 0 );
            if( settings.getPropertyValue( "graphics.antiAliasing", value ) )
                properties->setProperty( "Temporal anti-aliasing", value == 2 );
            if( settings.getPropertyValue( "graphics.shadowQuality", value ) )
            {
                properties->setProperty( "Cascaded shadows", value > 0 );
            }
            if( settings.getPropertyValue( "graphics.shadowResolution", value ) )
            {
                constexpr s32 resolutions[] = { 512, 1024, 2048, 4096 };
                properties->setProperty( "Shadow map size", resolutions[value] );
            }
            bool postProcessing = true;
            settings.getPropertyValue( "graphics.postProcessing", postProcessing );
            const bool hasPostProcessing = settings.hasProperty( "graphics.postProcessing" );
            const struct
            {
                const char *key;
                const char *property;
            } effects[] = {
                { "graphics.bloom", "Bloom" },
                { "graphics.motionBlur", "Motion blur" },
                { "graphics.ambientOcclusion", "Ambient occlusion" }
            };
            for( const auto &effect : effects )
            {
                bool enabled = false;
                existing.getPropertyValue( effect.property, enabled );
                if( settings.getPropertyValue( effect.key, enabled ) || hasPostProcessing )
                    properties->setProperty( effect.property, postProcessing && enabled );
            }
            return properties;
        }

        bool settingsNamespace( const String &key )
        {
            for( const char *prefix : { "project.", "platform.", "graphics.", "quality.", "assets.",
                                       "physics.", "sound.", "input.", "build.", "tasks.",
                                       "performance.", "scripting.", "network.", "services.",
                                       "diagnostics.", "editor." } )
            {
                if( key.compare( 0, std::char_traits<char>::length( prefix ), prefix ) == 0 )
                    return true;
            }
            return false;
        }
    }

    void describeProjectSettingsCapabilities( SmartPtr<Properties> response )
    {
        if( !response )
            return;
        for( const auto &capability : runtimeCapabilities )
        {
            response->setProperty( String( "capability." ) + capability.key, true );
            response->setProperty( String( "availability." ) + capability.key, capability.description );
        }
        for( const auto &capability : graphicsCapabilities )
        {
            response->setProperty( String( "capability." ) + capability.key, true );
            response->setProperty( String( "availability." ) + capability.key, capability.description );
        }
        response->setProperty( "unavailable.physics.threads",
            "The native physics backend stores this value but does not use it to run simulation workers." );
        response->setProperty( "unavailable.graphics.fullscreen",
            "The current native graphics window does not implement fullscreen transitions." );
        response->setProperty( "unavailable.graphics.resolution",
            "Project game display dimensions are separate from the Editor window." );
        response->setProperty( "unavailable.graphics.hdr",
            "The current pipeline does not expose a project HDR output switch." );
        response->setProperty( "unavailable.graphics.shadowDistance",
            "The native game shadow pipeline does not consume the configurable fade distance." );
        response->setProperty( "unavailable.tasks.threadCount",
            "Worker pools cannot be resized while running; project workers are not created at Editor startup." );
    }

    void updateProjectGraphicsSettings(
        SmartPtr<Properties> settings, SmartPtr<scene::GraphicsSettingsDirector> director )
    {
        if( !settings || !director )
            return;
        String error;
        if( !validate( *settings, error ) )
            throw std::invalid_argument( error.c_str() );
        const auto existing = director->getProperties();
        const auto properties = graphicsProperties( *settings, *existing );
        // A quality change applies a preset and returns before overrides are read.
        director->setProperties( properties );
        director->setProperties( properties );
    }

    SmartPtr<Properties> validateProjectSettingsRuntime( SmartPtr<Properties> settings )
    {
        auto response = workphone::make_ptr<Properties>();
        String error;
        const bool success = settings && validate( *settings, error );
        response->setProperty( "success", success );
        response->setProperty( "status", success ? String( "Validation passed." ) :
                              settings ? error : String( "Project settings are unavailable." ) );
        return response;
    }

    SmartPtr<Properties> applyProjectSettingsRuntime( SmartPtr<Properties> settings )
    {
        auto result = workphone::make_ptr<Properties>();
        result->setProperty( "success", false );
        if( !settings )
        {
            result->setProperty( "status", "Project settings are unavailable." );
            return result;
        }
        String error;
        if( !validate( *settings, error ) )
        {
            result->setProperty( "status", error );
            return result;
        }
        auto editor = EditorManager::getSingletonPtr();
        auto project = editor ? editor->getProject() : nullptr;
        auto director = project ? project->getGraphicsSettingsDirector() : nullptr;
        if( director )
            updateProjectGraphicsSettings( settings, director );
        auto app = core::IApplicationManager::instancePtr();
        if( !app )
        {
            result->setProperty( "success", true );
            result->setProperty( "status", "Project settings stored; runtime is not loaded." );
            result->setProperty( "appliedKeys", String() );
            result->setProperty( "unavailableKeys", String() );
            return result;
        }

        Array<String> applied;
        Array<String> unavailable;
        std::set<String> handled;
        for( const char *key : { "project.name", "project.company", "project.productName",
                                "project.version", "project.bundleId", "project.startupScene",
                                "project.autoLoadLastScene" } )
            handled.insert( key ); // Committed by the project's metadata/settings path.
        auto record = [&]( const char *key, bool available ) {
            if( settings->hasProperty( key ) )
            {
                ( available ? applied : unavailable ).push_back( key );
                handled.insert( key );
            }
        };

        for( const auto &capability : graphicsCapabilities )
            record( capability.key, director != nullptr );

        s32 frameRate = 0;
        auto taskManager = app->getTaskManager();
        bool frameRateApplied = false;
        if( settings->getPropertyValue( "graphics.frameRateLimit", frameRate ) && taskManager )
        {
            for( auto id : { TaskId::Primary, TaskId::Application, TaskId::Render } )
            {
                if( auto task = taskManager->getTask( id ) )
                {
                    task->setTargetFPS( frameRate );
                    frameRateApplied = true;
                }
            }
        }
        record( "graphics.frameRateLimit", frameRateApplied );

        s32 simulationRate = 0;
        bool simulationRateApplied = false;
        if( settings->getPropertyValue( "physics.simulationRate", simulationRate ) && taskManager )
        {
            if( auto task = taskManager->getTask( TaskId::Physics ) )
            {
                task->setTargetFPS( simulationRate );
                simulationRateApplied = true;
            }
        }
        record( "physics.simulationRate", simulationRateApplied );

        auto physics = app->getPhysicsManager();
        auto physicsScene = physics ? physics->getPhysicsScene() : nullptr;
        f32 gravity = 0;
        if( settings->getPropertyValue( "physics.gravity", gravity ) && physicsScene )
        {
            ScopedLock lock( physicsScene.get() );
            auto vector = physicsScene->getGravity();
            vector.Y() = gravity;
            physicsScene->setGravity( vector );
        }
        record( "physics.gravity", physicsScene != nullptr );
        bool debugDraw = false;
        if( settings->getPropertyValue( "physics.debugDraw", debugDraw ) && physics )
            physics->setEnableDebugDraw( debugDraw );
        record( "physics.debugDraw", physics != nullptr );

        auto sound = app->getSoundManager();
        s32 masterVolume = 0;
        if( settings->getPropertyValue( "sound.masterVolume", masterVolume ) && sound )
            sound->setVolume( static_cast<f32>( masterVolume ) / 100.0f );
        record( "sound.masterVolume", sound != nullptr );

        for( const auto &property : settings->getPropertiesAsArray() )
        {
            const auto key = property.getName();
            if( settingsNamespace( key ) && handled.find( key ) == handled.end() )
                unavailable.push_back( key );
        }
        result->setProperty( "success", true );
        result->setProperty( "appliedKeys", join( applied ) );
        result->setProperty( "unavailableKeys", join( unavailable ) );
        String status = "Applied " + StringUtil::toString( static_cast<u32>( applied.size() ) ) +
                        " available settings. Game graphics apply to the next standalone launch.";
        if( !unavailable.empty() )
            status += " " + StringUtil::toString( static_cast<u32>( unavailable.size() ) ) +
                      " settings have no active runtime consumer; unavailable controls explain the limits.";
        result->setProperty( "status", status );
        return result;
    }
}
