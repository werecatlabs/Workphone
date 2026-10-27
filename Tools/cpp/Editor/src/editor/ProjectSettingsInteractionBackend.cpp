#include <EditorPCH.hpp>
#include <editor/ProjectSettingsInteractionBackend.hpp>
#include <Workphone/Workphone.hpp>
#include <cstdlib>
#include <cerrno>
#include <cmath>
#include <set>

namespace workphone::editor
{
    namespace
    {
        const char *physicsKeys[] = { "physics.gravity", "physics.simulationRate", "physics.solverIterations",
            "physics.collisionLayers", "physics.broadphase", "physics.continuousCollision", "physics.substeps",
            "physics.threads", "physics.deterministic", "physics.debugDraw" };
        const char *audioKeys[] = { "sound.masterVolume", "sound.musicVolume", "sound.sfxVolume", "sound.voiceVolume",
            "sound.spatialization", "sound.backend", "sound.outputDevice", "sound.sampleRate", "sound.bufferSize",
            "sound.voiceLimit", "sound.streaming", "sound.occlusion" };
        const char *inputKeys[] = { "input.system", "input.gamepad", "input.touch", "input.mouseCapture",
            "input.rebinds", "input.gestures", "input.haptics" };

        String appendKey( const String &list, const String &key )
        {
            return list.empty() ? key : list + "; " + key;
        }

        bool validNumber( const Properties &settings, const char *key, double minimum, double maximum,
                          bool integral, String &error )
        {
            if( !settings.hasProperty( key ) ) return true;
            const auto text = settings.getPropertyObject( key ).getValue();
            char *end = nullptr;
            errno = 0;
            const auto value = std::strtod( text.c_str(), &end );
            if( end == text.c_str() || !end || *end || errno == ERANGE || !std::isfinite( value ) ||
                value < minimum || value > maximum || ( integral && value != std::floor( value ) ) )
            {
                error = "Invalid interaction setting: " + String( key );
                return false;
            }
            return true;
        }

        bool validate( const Properties &settings, String &error )
        {
            const struct { const char *key; double minimum, maximum; bool integral; } ranges[] = {
                { "physics.gravity", -100, 100, false }, { "physics.simulationRate", 15, 240, true },
                { "physics.solverIterations", 1, 64, true }, { "physics.substeps", 1, 8, true },
                { "physics.threads", 1, 16, true }, { "physics.broadphase", 0, 2, true },
                { "sound.masterVolume", 0, 100, true }, { "sound.musicVolume", 0, 100, true },
                { "sound.sfxVolume", 0, 100, true }, { "sound.voiceVolume", 0, 100, true },
                { "sound.sampleRate", 22050, 96000, true }, { "sound.bufferSize", 128, 4096, true },
                { "sound.voiceLimit", 1, 512, true }, { "sound.outputDevice", 0, 4, true },
                { "sound.backend", 0, 3, true }, { "input.system", 0, 3, true }
            };
            for( const auto &range : ranges )
                if( !validNumber( settings, range.key, range.minimum, range.maximum, range.integral, error ) )
                    return false;
            const auto audioBackend = settings.getPropertyAsInt( "sound.backend", 0 );
            if( audioBackend == 1 || audioBackend == 2 )
            {
                error = "FMOD/OpenAL require their audio plugin. Select Default or Platform Native for the built-in audio backend.";
                return false;
            }
            if( settings.getPropertyAsInt( "input.system", 0 ) == 1 )
            {
                error = "The SDL input plugin is not included in this build. Engine Input and Platform Native are implemented.";
                return false;
            }
            for( const auto *key : { "physics.continuousCollision", "physics.deterministic", "physics.debugDraw",
                 "sound.spatialization", "sound.streaming", "sound.occlusion", "input.gamepad", "input.touch",
                 "input.mouseCapture", "input.rebinds", "input.gestures", "input.haptics" } )
            {
                if( !settings.hasProperty( key ) ) continue;
                const auto value = settings.getPropertyObject( key ).getValue();
                if( value != "true" && value != "false" && value != "0" && value != "1" )
                { error = "Invalid boolean interaction setting: " + String( key ); return false; }
            }
            if( settings.hasProperty( "physics.collisionLayers" ) )
            {
                const auto text = settings.getProperty( "physics.collisionLayers" );
                const auto layers = StringUtil::split( text, "," );
                std::set<String> unique;
                if( layers.empty() || layers.size() > 32 || text.empty() || text.back() == ',' )
                { error = "Collision Layers requires 1 to 32 distinct, comma-separated names."; return false; }
                for( const auto &layer : layers )
                {
                    const auto name = StringUtil::trim( layer );
                    if( name.empty() || !unique.insert( name ).second )
                    { error = "Collision layer names must be nonempty and unique."; return false; }
                }
            }
            return true;
        }
    }

    void describeProjectSettingsInteractionCapabilities( SmartPtr<Properties> response )
    {
        if( !response ) return;
        const auto describe = [&]( const char *key, const char *availability ) {
            response->setProperty( "capability." + String( key ), true );
            response->setProperty( "availability." + String( key ), String( availability ) );
        };
        for( const auto key : physicsKeys ) describe( key,
            "Native scene simulation: iterative solver, fixed or adaptive substeps, SAP/DBVT, parallel integration and named collision masks." );
        for( const auto key : audioKeys ) describe( key,
            "Native mixer and XAudio2 endpoint; category gain, positional attenuation/pan, raycast occlusion and voice admission. Device/rate apply at audio startup; streaming/buffer settings on new voices." );
        for( const auto key : inputKeys ) describe( key,
            "Native input event routing, gamepad discovery, touch/gesture registration, mouse capture, action rebinding and XInput vibration." );
    }

    SmartPtr<Properties> validateProjectSettingsInteraction( SmartPtr<Properties> settings )
    {
        auto result = workphone::make_ptr<Properties>();
        String error;
        const bool success = settings && validate( *settings, error );
        result->setProperty( "success", success );
        result->setProperty( "status", success ? String( "Interaction settings validated." ) :
                             settings ? error : String( "Interaction settings are unavailable." ) );
        return result;
    }

    SmartPtr<Properties> applyProjectSettingsInteraction( SmartPtr<Properties> settings )
    {
        auto result = validateProjectSettingsInteraction( settings );
        if( !result->getPropertyAsBool( "success" ) ) return result;
        String handled, applied, pending;
        auto app = core::IApplicationManager::instancePtr();
        const auto record = [&]( const char *key, bool active ) {
            if( settings->hasProperty( key ) )
            {
                handled = appendKey( handled, key );
                if( active ) applied = appendKey( applied, key );
                else pending = appendKey( pending, key );
            }
        };
        auto physics = app ? app->getPhysicsManager() : nullptr;
        auto scene = physics ? physics->getPhysicsScene() : nullptr;
        if( scene )
        {
            ScopedLock lock( scene.get() );
            scene->setProperties( settings );
            f32 gravity = 0;
            if( settings->getPropertyValue( "physics.gravity", gravity ) )
            {
                auto vector = scene->getGravity(); vector.Y() = gravity; scene->setGravity( vector );
            }
        }
        if( physics && settings->hasProperty( "physics.debugDraw" ) )
            physics->setEnableDebugDraw( settings->getPropertyAsBool( "physics.debugDraw" ) );
        for( const auto key : physicsKeys ) record( key, scene != nullptr );
        auto audio = app ? app->getSoundManager() : nullptr;
        auto previousAudio = audio ? audio->getProperties() : nullptr;
        if( audio ) audio->setProperties( settings );
        for( const auto key : audioKeys )
        {
            bool active = audio && audio->isLoaded();
            if( String( key ) == "sound.sampleRate" || String( key ) == "sound.outputDevice" ||
                String( key ) == "sound.backend" || String( key ) == "sound.bufferSize" || String( key ) == "sound.streaming" )
                active = active && previousAudio && previousAudio->getProperty( key ) == settings->getProperty( key );
            record( key, active );
        }
        auto input = app ? app->getInputDeviceManager() : nullptr;
        if( input ) input->setProperties( settings );
        for( const auto key : inputKeys ) record( key, input && input->isLoaded() );
        result->setProperty( "handledKeys", handled );
        result->setProperty( "appliedKeys", applied );
        result->setProperty( "pendingKeys", pending );
        result->setProperty( "status", pending.empty() ? String( "Interaction settings applied." ) :
            String( "Interaction settings applied; absent subsystems and audio device/voice changes take effect when they start." ) );
        return result;
    }
}
