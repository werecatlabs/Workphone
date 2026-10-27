#include <WPLua/LuaManager.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include "../../Engine/cpp/Source/Workphone/Scene/Components/CarControllerInput.hpp"
#include <luabind/luabind.hpp>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
#include <limits>
#include <cmath>
extern "C"
{
#include <lauxlib.h>
#include <lualib.h>
}

namespace
{
    using namespace workphone;
    namespace input = scene::controller_input;
    unsigned controllerAssertions = 0;
    void checkController( bool value, const char *message )
    {
        ++controllerAssertions;
        if( !value ) throw std::runtime_error( message );
    }
    bool nearController( f32 value, f32 expected ) { return std::abs( value - expected ) < 0.00001f; }
    void neutralController( const input::Sample &value )
    {
        checkController( value.configured && !value.deviceMatched && value.throttle == 0 &&
                         value.brake == 0 && value.steering == 0, "invalid/disconnected profile must be neutral" );
    }
    SmartPtr<Properties> gamepadProfile()
    {
        auto p = make_ptr<Properties>();
        p->setName( "Vehicle Controller" );
        p->setProperty( "Enabled", true ); p->setProperty( "Profile", String( "Gamepad" ) );
        p->setProperty( "Device Index", 0 ); p->setProperty( "Device Name", String( "Synthetic pad" ) );
        p->setProperty( "Device Axes", 6 ); p->setProperty( "Device Buttons", 14 );
        p->setProperty( "Device Count", 1 ); p->setProperty( "Steering Axis", 0 );
        p->setProperty( "Throttle Axis", 5 ); p->setProperty( "Brake Axis", 2 );
        p->setProperty( "Pedal Mode", String( "separate01" ) );
        return p;
    }
    SmartPtr<Joystick> syntheticPad()
    {
        auto pad = make_ptr<Joystick>();
        pad->setName( "Synthetic pad" ); pad->setNumAxes( 6 ); pad->setNumButtons( 14 );
        pad->updateAxisValue( 0, 0.55f ); pad->updateAxisValue( 5, 0.51f );
        pad->updateAxisValue( 2, 0.02f );
        return pad;
    }
    void controllerInputContracts()
    {
        auto pad = syntheticPad();
        Array<SmartPtr<IJoystick>> devices{ pad };
        auto p = gamepadProfile();
        pad->setDeadZone( 0.9f ); pad->setSensitivity( 4 ); pad->setInvertX( true );
        checkController( pad->getAxis( 0 ) == 0 && nearController( pad->getRawAxis( 0 ), 0.55f ),
                         "raw input must bypass global calibration" );
        checkController( pad->getRawAxis( -1 ) == 0 && pad->getRawAxis( IJoystick::MAX_AXES ) == 0,
                         "invalid raw-axis indices must be safe" );
        auto s = input::sample( p, devices );
        checkController( s.configured && s.deviceMatched && nearController( s.steering, 0.5f ) &&
                         nearController( s.throttle, 0.5f ) && s.brake == 0,
                         "gamepad mapping must use raw LX/RT/LT without double calibration" );
        p->setProperty( "Invert Steering", true ); p->setProperty( "Steering Sensitivity", 2.0f );
        s = input::sample( p, devices );
        checkController( nearController( s.steering, -1 ), "steering inversion and sensitivity" );
        pad->updateAxisValue( 0, 0.05f );
        checkController( input::sample( p, devices ).steering == 0, "steering drift must be suppressed" );

        p->setProperty( "Invert Throttle", true ); p->setProperty( "Invert Brake", true );
        pad->updateAxisValue( 5, 0 ); pad->updateAxisValue( 2, 1 );
        s = input::sample( p, devices );
        checkController( s.throttle == 1 && s.brake == 0, "separate zero-one pedal inversion" );
        p->setProperty( "Pedal Mode", String( "separateSigned" ) );
        pad->updateAxisValue( 5, 1 ); pad->updateAxisValue( 2, -1 );
        s = input::sample( p, devices );
        checkController( s.throttle == 0 && s.brake == 1, "signed wheel pedals must map rest and press correctly" );
        pad->updateAxisValue( 5, -1 ); pad->updateAxisValue( 2, 1 );
        s = input::sample( p, devices );
        checkController( s.throttle == 1 && s.brake == 0, "signed wheel pedal endpoints" );

        p->setProperty( "Pedal Mode", String( "combined" ) );
        p->setProperty( "Invert Throttle", false ); p->setProperty( "Brake Axis", -1 );
        pad->updateAxisValue( 5, 0.51f );
        s = input::sample( p, devices );
        checkController( nearController( s.throttle, 0.5f ) && s.brake == 0,
                         "combined positive input must accelerate, ignoring unused brake mapping" );
        pad->updateAxisValue( 5, -0.51f );
        s = input::sample( p, devices );
        checkController( s.throttle == 0 && nearController( s.brake, 0.5f ), "combined negative input must brake" );
        p->setProperty( "Invert Throttle", true );
        s = input::sample( p, devices );
        checkController( nearController( s.throttle, 0.5f ) && s.brake == 0, "combined axis inversion" );

        neutralController( input::sample( p, {} ) );
        neutralController( input::sample( p, { nullptr } ) );
        p->setProperty( "Device Index", 1 ); neutralController( input::sample( p, devices ) );
        p->setProperty( "Device Index", 0.5f ); neutralController( input::sample( p, devices ) );
        p->setProperty( "Device Index", String( "nan" ) ); neutralController( input::sample( p, devices ) );
        p->setProperty( "Device Index", 0 ); p->setProperty( "Device Name", String( "Other pad" ) );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Device Name", String( "Synthetic pad" ) ); p->setProperty( "Device Axes", 5 );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Device Axes", 6 ); p->setProperty( "Device Buttons", 13 );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Device Buttons", 14 ); p->setProperty( "Device Count", 2 );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Device Count", 1 ); p->setProperty( "Steering Axis", -1 );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Steering Axis", 0 ); p->setProperty( "Throttle Axis", 8 );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Throttle Axis", 5 ); pad->updateAxisValue( 5, std::numeric_limits<f32>::quiet_NaN() );
        neutralController( input::sample( p, devices ) );
        pad->updateAxisValue( 5, 1 ); p->setProperty( "Steering Deadzone", 1.0f );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Steering Deadzone", 0.1f ); p->setProperty( "Pedal Deadzone", -0.1f );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Pedal Deadzone", 0.02f ); p->setProperty( "Steering Sensitivity", 0.0f );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Steering Sensitivity", std::numeric_limits<f32>::infinity() );
        neutralController( input::sample( p, devices ) );
        p->setProperty( "Steering Sensitivity", 1.0f ); p->setProperty( "Pedal Mode", String( "invalid" ) );
        neutralController( input::sample( p, devices ) );
        s = input::sample( p, {}, { true, false, true, false } );
        checkController( s.throttle == 1 && s.brake == 0 && s.steering == -1,
                         "keyboard must remain available for malformed/disconnected profiles" );
        s = input::sample( p, devices, { true, true, true, true } );
        checkController( s.throttle == 1 && s.brake == 1 && s.steering == 0,
                         "opposed keyboard steering must cancel" );
        p->setProperty( "Enabled", false ); neutralController( input::sample( p, devices ) );
        checkController( !input::sample( nullptr, devices ).configured, "no profile must preserve legacy behavior" );

        std::printf( "Vehicle controller input: %u assertions passed.\n", controllerAssertions );
    }
}

int main( int argc, char **argv )
{
    using namespace workphone;
    struct RuntimeTypes
    {
        TypeManager types;
        RuntimeTypes() { types.load(); TypeManager::setInstance(&types); }
        ~RuntimeTypes() { TypeManager::setInstance(nullptr); types.unload(); }
    } runtimeTypes;
    LuaManager manager;
    try
    {
        controllerInputContracts();
        manager.load( nullptr );
        auto state = manager.getLuaState();
        if ( !state ) throw std::runtime_error( "Lua manager did not create a state" );
        auto race = make_ptr<scene::ProceduralRaceScene>();
        luabind::globals( state )["raceScene"] = race;
        auto controller = syntheticPad();
        controller->setDeadZone( 0.9f ); controller->setSensitivity( 4 ); controller->setInvertX( true );
        Array<SmartPtr<IJoystick>> devices{ controller };
        luabind::globals( state )["controllerDevices"] = devices;
        auto settingsSource = make_ptr<Properties>();
        settingsSource->setProperty( "Unrelated", String( "keep" ) );
        settingsSource->addChild( gamepadProfile() );
        auto replacementProfile = gamepadProfile(); replacementProfile->setProperty( "Enabled", false );
        luabind::globals( state )["controllerSettingsSource"] = settingsSource;
        luabind::globals( state )["controllerSettingsCopy"] = make_ptr<Properties>();
        luabind::globals( state )["controllerSettingsReplacement"] = replacementProfile;
        if( argc > 1 && String( argv[1] ) == "--probe-properties-array" )
        {
            if( luaL_dostring( state,
                "controllerSettingsCopy:setPropertiesAsArray(controllerSettingsSource:getPropertiesAsArray())" ) != LUA_OK )
                throw std::runtime_error( lua_tostring( state, -1 ) );
            std::puts( "Properties array copy binding: PASS" );
        }
        const auto core =
            std::filesystem::path( WP_TEST_SOURCE_ROOT ) / "bin/Media/Scripts/Lua/Game/Core";
        for ( const auto file :
              { "BaseComponent.lua", "RaceSession.lua", "SampleVehicleAdvanced.lua" } )
            if ( luaL_dofile( state, ( core / file ).string().c_str() ) != LUA_OK )
                throw std::runtime_error( lua_tostring( state, -1 ) );
        const char* test = R"(
            assert(controllerDevices:size() == 1)
            local device = controllerDevices:at(0)
            assert(device:getName() == 'Synthetic pad')
            assert(device:getNumAxes() == 6 and device:getNumButtons() == 14)
            assert(device:getAxis(0) == 0)
            assert(math.abs(device:getRawAxis(0) - 0.55) < 0.00001)
            assert(device:getRawAxis(-1) == 0 and device:getRawAxis(99) == 0)
            -- Publication copies the root arrays before replacing one child.
            local copy, original = controllerSettingsCopy, controllerSettingsSource
            copy['operator='](copy, original)
            assert(copy:getPropertyAsString('Unrelated') == 'keep')
            copy:removeChild('Vehicle Controller')
            copy:addChild(controllerSettingsReplacement)
            assert(original:getChild('Vehicle Controller'):getPropertyAsBool('Enabled'))
            assert(not copy:getChild('Vehicle Controller'):getPropertyAsBool('Enabled'))
            assert(raceScene:getAudioEnabled() and raceScene:getEffectsEnabled())
            assert(not raceScene:isAudioAvailable() and not raceScene:isEffectsAvailable())
            assert(raceScene:getParticleCount() == 0 and raceScene:getSkidDecalCount() == 0)
            assert(not raceScene:getFoliageWorldEnabled())
            raceScene:setFoliageWorldEnabled(true)
            assert(raceScene:getFoliageWorldEnabled())
            raceScene:setMaterialExportDirectory('generated-material-source')
            assert(raceScene:getMaterialExportDirectory() == 'generated-material-source')
            local sample = SampleVehicleAdvanced(raceScene)
            sample.raceScene = raceScene
            sample:setPresentationOptions(false, true)
            assert(not raceScene:getAudioEnabled() and raceScene:getEffectsEnabled())
            sample:setPresentationOptions(true, false)
            assert(raceScene:getAudioEnabled() and not raceScene:getEffectsEnabled())
            sample:setPresentationOptions(false, false)
            assert(not raceScene:getAudioEnabled() and not raceScene:getEffectsEnabled())
            -- Remove the test reference before Lua finalization invokes sample cleanup.
            sample.raceScene = nil
            collectgarbage('collect')
        )";
        if ( luaL_dostring( state, test ) != LUA_OK )
            throw std::runtime_error( lua_tostring( state, -1 ) );
        manager.unload( nullptr );
        std::puts( "Vehicle Lua presentation bindings: PASS" );
        return 0;
    }
    catch ( const std::exception& e )
    {
        std::fprintf( stderr, "Vehicle Lua presentation bindings: FAIL: %s\n", e.what() );
        manager.unload( nullptr );
        return 1;
    }
}
