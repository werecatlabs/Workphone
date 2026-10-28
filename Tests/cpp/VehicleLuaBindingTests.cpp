#include <WPLua/LuaManager.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <WPLuabind/Helpers/GraphicsSystemHelper.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Graphics/GraphicsWindow.hpp>
#include <Workphone/Graphics/GraphicsPipeline.hpp>
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
    class SettingsWindow : public render::GraphicsWindow
    {
    public:
        Vector2I size{1600,900};
        bool fullscreen=false, failResize=false;
        unsigned displayCalls=0;
        Vector2I getSize() const override { return size; }
        bool isFullScreen() const override { return fullscreen; }
        void setFullscreen(bool value) override { ++displayCalls; fullscreen=value; }
        void resize(u32 width,u32 height) override
        {
            ++displayCalls;
            if(failResize) { failResize=false; throw std::runtime_error("synthetic resize failure"); }
            size=Vector2I(width,height);
        }
    };
    class SettingsGraphics : public render::GraphicsSystem
    {
    public:
        SmartPtr<render::GraphicsPipeline> pipeline=make_ptr<render::GraphicsPipeline>();
        bool failApply=false;
        unsigned applies=0;
        SmartPtr<Properties> stored=make_ptr<Properties>();
        SmartPtr<render::IGraphicsPipeline> getGraphicsPipeline() const override { return pipeline; }
        SmartPtr<Properties> getProperties() const override
        {
            auto result=make_ptr<Properties>(); *result=*stored;
            result->setProperty("msaa_supported_mask",7);
            return result;
        }
        void setProperties(SmartPtr<Properties> properties) override
        {
            ++applies;
            if(failApply) { failApply=false; throw std::runtime_error("synthetic pipeline failure"); }
            stored=properties;
        }
    };
    void runtimeGraphicsContracts()
    {
        unsigned assertions=0;
        const auto check=[&](bool value,const char *message)
        { ++assertions; if(!value) throw std::runtime_error(message); };
        auto app=make_ptr<core::ApplicationManager>();
        core::IApplicationManager::setInstance(app);
        struct ResetApp { ~ResetApp(){ core::IApplicationManager::setInstance(nullptr); } } reset;
        auto graphics=make_ptr<SettingsGraphics>();
        auto window=make_ptr<SettingsWindow>();
        graphics->setDefaultWindow(window);
        app->setGraphicsSystem(graphics);
        app->setEditor(true);
        auto original=make_ptr<Properties>(); original->setName("Player");
        original->setProperty("Unrelated",String("keep"));
        auto controller=make_ptr<Properties>(); controller->setName("Vehicle Controller");
        controller->setProperty("Enabled",true); original->addChild(controller);
        app->setPlayerSettings(original);
        auto before=GraphicsSystemHelper::getRuntimeSettings(graphics.get());
        check(before->getPropertyAsInt("graphics.customWidth")==1600,"runtime width must start at the real output size");
        check(before->getPropertyAsInt("graphics.customHeight")==900,"runtime height must start at the real output size");
        check(before->getPropertyAsBool("editor") && before->getPropertyAsBool("displayPending"),"editor display policy is pending standalone");
        check(before->getPropertyAsInt("msaa_supported_mask")==7,"MSAA capabilities must be exposed");
        auto request=make_ptr<Properties>();
        request->setProperty("graphics.bloom",true); request->setProperty("graphics.motionBlur",true);
        request->setProperty("graphics.ambientOcclusion",true);
        request->setProperty("graphics.vsync",true);
        auto applied=GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request);
        check(applied->getPropertyAsBool("success"),"post effect apply must succeed");
        check(graphics->pipeline->isBloomEnabled() && graphics->pipeline->isMotionBlurEnabled() && graphics->pipeline->isGTAOEnabled(),"software effects must follow their runtime controls");
        check(graphics->stored->getPropertyAsBool("vsync"),"vsync must reach the graphics backend");
        check(!original->hasProperty("graphics.bloom"),"published player snapshots must remain immutable");
        check(app->getPlayerSettings()->getProperty("Unrelated")=="keep" && app->getPlayerSettings()->getChild("Vehicle Controller")==controller,"apply must preserve unrelated data and controller profile");
        request=make_ptr<Properties>(); request->setProperty("graphics.postProcessing",false);
        check(GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request)->getPropertyAsBool("success") && !graphics->pipeline->isBloomEnabled() && !graphics->pipeline->isMotionBlurEnabled() && !graphics->pipeline->isGTAOEnabled(),"post effect master must gate effects without discarding their individual choices");
        request->setProperty("graphics.postProcessing",true);
        check(GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request)->getPropertyAsBool("success") && graphics->pipeline->isBloomEnabled() && graphics->pipeline->isMotionBlurEnabled() && graphics->pipeline->isGTAOEnabled(),"restoring the post effect master must restore individual effects");
        request=make_ptr<Properties>();
        request->setProperty("graphics.customWidth",1920); request->setProperty("graphics.customHeight",1080);
        request->setProperty("graphics.fullscreen",true);
        applied=GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request);
        check(applied->getPropertyAsBool("success") && applied->getPropertyAsBool("displayPending"),"editor display changes must be retained for standalone");
        check(window->displayCalls==0 && !window->fullscreen && window->size==Vector2I(1600,900),"game settings must never resize or fullscreen the Editor");
        app->setEditor(false);
        applied=GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request);
        check(applied->getPropertyAsBool("success") && !applied->getPropertyAsBool("displayPending") && window->fullscreen,"standalone fullscreen must reach the native window");
        request->setProperty("graphics.fullscreen",false);
        applied=GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request);
        check(applied->getPropertyAsBool("success") && !window->fullscreen && window->size==Vector2I(1920,1080),"leaving fullscreen must use the selected windowed size");
        request=make_ptr<Properties>(); request->setProperty("quality.texture",0);
        applied=GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request);
        check(applied->getPropertyAsBool("success") && app->getPlayerSettings()->getPropertyAsInt("quality.maxTextureSize")==1024,"low texture preset must limit texture residency");
        check(app->getPlayerSettings()->getPropertyAsInt("graphics.textureFiltering")==1 && app->getPlayerSettings()->getPropertyAsInt("graphics.anisotropy")==1,"low texture preset filtering");
        request->setProperty("quality.maxTextureSize",777);
        check(GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request)->getPropertyAsBool("success"),"explicit texture cap must apply");
        request=make_ptr<Properties>(); request->setProperty("graphics.bloom",false);
        check(GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request)->getPropertyAsBool("success") && app->getPlayerSettings()->getPropertyAsInt("quality.maxTextureSize")==777,"unchanged texture quality must retain advanced texture policy");
        const auto reject=[&](SmartPtr<Properties> bad)
        {
            auto prior=app->getPlayerSettings(); const auto calls=graphics->applies;
            check(!GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),bad)->getPropertyAsBool("success"),"invalid graphics settings must be rejected");
            check(app->getPlayerSettings()==prior && calls==graphics->applies,"validation failure must not publish or call the backend");
        };
        request=make_ptr<Properties>(); request->setProperty("quality.mesh",0.5f); reject(request);
        request=make_ptr<Properties>(); request->setProperty("graphics.antiAliasing",5); reject(request);
        request=make_ptr<Properties>(); request->setProperty("graphics.renderScale",std::numeric_limits<f32>::quiet_NaN()); reject(request);
        request=make_ptr<Properties>(); request->setProperty("graphics.fullscreen",String("yes")); reject(request);
        request=make_ptr<Properties>(); request->setProperty("graphics.customWidth",0); reject(request);
        auto prior=app->getPlayerSettings();
        request=make_ptr<Properties>(); request->setProperty("graphics.bloom",true); graphics->failApply=true;
        check(!GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request)->getPropertyAsBool("success") && app->getPlayerSettings()==prior,"backend failure must retain the published player snapshot");
        request=make_ptr<Properties>(); request->setProperty("graphics.customWidth",1280); window->failResize=true;
        check(!GraphicsSystemHelper::applyRuntimeSettings(graphics.get(),request)->getPropertyAsBool("success") && app->getPlayerSettings()==prior && window->size==Vector2I(1920,1080),"failed display changes must restore the native window");
        auto modes=GraphicsSystemHelper::getDisplayModes(graphics.get());
        check(modes->getPropertyAsInt("count")>0,"display mode enumeration must return usable modes");
        check(modes->getPropertyAsInt("mode.0.width")>=320 && modes->getPropertyAsInt("mode.0.height")>=200,"display modes use flat Lua-safe properties");
        std::printf("Runtime graphics settings: %u assertions passed.\n",assertions);
        app->setGraphicsSystem(nullptr);
    }
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
        runtimeGraphicsContracts();
        manager.load( nullptr );
        auto state = manager.getLuaState();
        if ( !state ) throw std::runtime_error( "Lua manager did not create a state" );
        auto runtimeGraphics=make_ptr<SettingsGraphics>();
        luabind::globals(state)["runtimeGraphics"]=static_pointer_cast<render::IGraphicsSystem>(runtimeGraphics);
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
            -- Exercise the real runtime graphics bindings and Properties return
            -- conversion without creating a graphics device or native window.
            local graphics = runtimeGraphics:getRuntimeSettings()
            assert(graphics:getPropertyAsInt('graphics.customWidth') > 0)
            assert(graphics:getPropertyAsInt('graphics.customHeight') > 0)
            assert(runtimeGraphics:getDisplayModes():getPropertyAsInt('count') > 0)
            local unavailable = runtimeGraphics:applyRuntimeSettings(graphics)
            assert(not unavailable:getPropertyAsBool('success'))
            assert(unavailable:getPropertyAsString('status') ~= '')
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
