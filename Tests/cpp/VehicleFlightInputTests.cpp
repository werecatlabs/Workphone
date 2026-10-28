#include <Workphone/Workphone.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Scene/Components/VehicleController.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <WPVehiclePhysics/CAircraftBody.hpp>
#include "../../Engine/cpp/Source/Workphone/Scene/Components/AircraftControllerInput.hpp"
#include <cstdio>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace workphone;
namespace
{
    unsigned assertions = 0;
    void check( bool value, const char *message )
    {
        ++assertions;
        if( !value ) throw std::runtime_error( message );
    }
    bool nearFlight( f32 a, f32 b ) { return std::abs( a - b ) < 0.00001f; }
    SmartPtr<Properties> profile()
    {
        auto p = make_ptr<Properties>();
        p->setName( "Vehicle Controller" );
        p->setProperty( "Device Name", String( "Synthetic flight stick" ) );
        p->setProperty( "Device Index", 0 ); p->setProperty( "Device Count", 1 );
        p->setProperty( "Device Axes", 6 ); p->setProperty( "Device Buttons", 14 );
        p->setProperty( "Enabled", false );
        for( const auto prefix : { "Plane ", "Helicopter ", "Drone " } )
        {
            const auto key = [prefix]( const char *name ) { return String( prefix ) + name; };
            p->setProperty( key( "Enabled" ), true );
            p->setProperty( key( "Pitch Axis" ), 1 ); p->setProperty( key( "Roll Axis" ), 0 );
            p->setProperty( key( "Yaw Axis" ), 3 ); p->setProperty( key( "Throttle Axis" ), 5 );
            p->setProperty( key( "Throttle Mode" ), String( "01" ) );
            p->setProperty( key( "Deadzone" ), 0.1f ); p->setProperty( key( "Sensitivity" ), 1.0f );
        }
        return p;
    }
    SmartPtr<Joystick> device()
    {
        auto d = make_ptr<Joystick>(); d->setName( "Synthetic flight stick" );
        d->setNumAxes( 6 ); d->setNumButtons( 14 );
        d->updateAxisValue( 1, 0.55f ); d->updateAxisValue( 0, -0.55f );
        d->updateAxisValue( 3, 1.0f ); d->updateAxisValue( 5, 0.55f );
        d->setDeadZone( 0.9f ); d->setSensitivity( 4 ); d->setInvertX( true );
        return d;
    }
    void neutral( const scene::aircraft_input::Sample &s )
    {
        check( !s.deviceMatched && s.throttle == 0 && s.pitch == 0 && s.roll == 0 && s.yaw == 0,
               "invalid/disabled flight input must be neutral" );
    }
    void samplerContracts()
    {
        auto p = profile(); auto d = device(); Array<SmartPtr<IJoystick>> devices{ d };
        for( const auto kind : { "plane", "helicopter", "drone" } )
        {
            const auto s = scene::aircraft_input::sample( p, devices, kind );
            check( s.configured && s.deviceMatched && nearFlight( s.pitch, 0.5f ) && nearFlight( s.roll, -0.5f ) &&
                   s.yaw == 1 && nearFlight( s.throttle, 0.5f ), "all flight profiles use physical raw axes independently of ground calibration" );
        }
        check( d->getAxis( 1 ) == 0 && nearFlight( d->getRawAxis( 1 ), 0.55f ), "physical raw device preview remains independent" );
        p->setProperty( "Plane Invert Pitch", true ); p->setProperty( "Plane Invert Roll", true );
        p->setProperty( "Plane Invert Yaw", true ); p->setProperty( "Plane Sensitivity", 2.0f );
        auto s = scene::aircraft_input::sample( p, devices, "plane" );
        check( nearFlight( s.pitch, -1 ) && nearFlight( s.roll, 1 ) && nearFlight( s.yaw, -1 ), "attitude inversion and sensitivity" );
        p = profile(); p->setProperty( "Plane Throttle Mode", String( "signed" ) );
        for( auto value : { -1.0f, 0.0f, 1.0f } )
        {
            d->updateAxisValue( 5, value ); s = scene::aircraft_input::sample( p, devices, "plane" );
            check( nearFlight( s.throttle, scene::controller_input::pedalValue( ( value + 1 ) * 0.5f, 0.1f ) ), "signed throttle endpoints and center" );
        }
        p->setProperty( "Plane Invert Throttle", true );
        check( scene::aircraft_input::sample( p, devices, "plane" ).throttle == 0, "throttle inversion at full raw input" );
        d = device(); devices[0] = d;
        const auto reject = [&]( const char *key, const auto &value ) {
            auto invalid = profile(); invalid->setProperty( key, value );
            neutral( scene::aircraft_input::sample( invalid, devices, "plane" ) );
        };
        reject( "Plane Enabled", false ); reject( "Plane Enabled", String( "yes" ) );
        reject( "Device Index", 0.5f ); reject( "Device Index", -1 ); reject( "Device Count", 2 );
        reject( "Device Axes", 5 ); reject( "Device Buttons", 13 ); reject( "Device Name", String( "Other" ) );
        reject( "Plane Pitch Axis", -1 ); reject( "Plane Roll Axis", 6 ); reject( "Plane Yaw Axis", 0.5f );
        reject( "Plane Throttle Axis", String( "nan" ) ); reject( "Plane Throttle Mode", String( "invalid" ) );
        reject( "Plane Deadzone", 1.0f ); reject( "Plane Deadzone", String( "bad" ) );
        reject( "Plane Sensitivity", 0.0f ); reject( "Plane Invert Roll", String( "yes" ) );
        reject( "Plane Sensitivity", std::numeric_limits<f32>::quiet_NaN() );
        neutral( scene::aircraft_input::sample( profile(), {}, "plane" ) );
        neutral( scene::aircraft_input::sample( profile(), { nullptr }, "plane" ) );
        neutral( scene::aircraft_input::sample( profile(), devices, "invalid" ) );
        neutral( scene::aircraft_input::sample( nullptr, devices, "plane" ) );
        d->updateAxisValue( 3, std::numeric_limits<f32>::quiet_NaN() );
        neutral( scene::aircraft_input::sample( profile(), devices, "plane" ) );
    }
    class Devices : public InputDeviceManager
    {
    public:
        Array<SmartPtr<IJoystick>> devices;
        Array<SmartPtr<IJoystick>> getJoysticks() const override { return devices; }
    };
    class Controller : public scene::VehicleController
    {
    public:
        void attach( SmartPtr<vehicle::IAircraft> aircraft )
        { m_vehicleType = VehicleType::Aircraft; m_aircraftController = aircraft; }
    };
    void runtimeContracts( SmartPtr<core::ApplicationManager> app )
    {
        auto stick = device();
        auto input = make_ptr<Devices>(); input->devices = { stick }; app->setInputDeviceManager( input );
        auto settings = make_ptr<Properties>(); settings->addChild( profile() ); app->setPlayerSettings( settings );
        app->setPlaying( true ); app->setPaused( false );
        auto aircraft = make_ptr<vehicle::CAircraft>();
        aircraft->setBody( make_ptr<vehicle::CAircraftBody>() );
        auto controller = make_ptr<Controller>(); controller->attach( aircraft );
        check( !controller->getAircraftPlayerControls() && controller->getAircraftControlProfile() == "plane", "manual control remains default" );
        Thread::setCurrentTask( TaskId::Application );
        controller->setAircraftControls( 0.7f, 0.2f, 0.3f, 0.4f ); controller->update();
        check( nearFlight( aircraft->getChannel( 0 ), 0.7f ) && nearFlight( aircraft->getChannel( 1 ), 0.3f ), "manual controls reach existing aircraft channels" );
        for( const auto kind : { "plane", "helicopter", "drone" } )
        {
            controller->useAircraftPlayerControls( kind ); controller->update();
            check( controller->getAircraftPlayerControls() && controller->getAircraftControlProfile() == kind &&
                   nearFlight( aircraft->getChannel( 0 ), 0.5f ) && nearFlight( aircraft->getChannel( 1 ), -0.5f ) &&
                   nearFlight( aircraft->getChannel( 2 ), 0.5f ) && aircraft->getChannel( 3 ) == 1,
                   "opt-in profile reaches actual existing throttle/roll/pitch/yaw channel consumer" );
        }
        stick->updateAxisValue( 1, -1 );
        Thread::setCurrentTask( TaskId::Render ); controller->update();
        check( nearFlight( aircraft->getChannel( 2 ), 0.5f ), "render task never samples configured input" );
        Thread::setCurrentTask( TaskId::Application ); controller->update();
        check( aircraft->getChannel( 2 ) == -1, "scene/Application task publishes changed physical input" );
        app->setPaused( true ); controller->update();
        check( aircraft->getChannel( 0 ) == 0 && aircraft->getChannel( 1 ) == 0 && aircraft->getChannel( 2 ) == 0 && aircraft->getChannel( 3 ) == 0, "paused aircraft controls neutralize" );
        app->setPaused( false ); input->devices.clear(); controller->update();
        check( aircraft->getChannel( 0 ) == 0 && aircraft->getChannel( 2 ) == 0, "disconnect cannot latch flight controls" );
        controller->setAircraftControls( 0.8f, 0.1f, 0.2f, 0.3f ); controller->update();
        check( !controller->getAircraftPlayerControls() && nearFlight( aircraft->getChannel( 0 ), 0.8f ), "manual controls explicitly disable automatic input" );
        bool rejected = false;
        try { controller->useAircraftPlayerControls( "invalid" ); } catch( const std::invalid_argument & ) { rejected = true; }
        check( rejected && !controller->getAircraftPlayerControls(), "invalid profile cannot enable aircraft input" );
        controller = nullptr; aircraft = nullptr; app->setInputDeviceManager( nullptr );
    }
}

int main()
{
    TypeManager types; types.load(); TypeManager::setInstance( &types );
    auto app = make_ptr<core::ApplicationManager>(); core::IApplicationManager::setInstance( app );
    const auto previousTask = Thread::getCurrentTask();
    int result = 0;
    try { samplerContracts(); runtimeContracts( app ); std::printf( "Vehicle flight input: %u assertions passed.\n", assertions ); }
    catch( const std::exception &e ) { std::fprintf( stderr, "Vehicle flight input FAIL: %s\n", e.what() ); result = 1; }
    app->setInputDeviceManager( nullptr ); app = nullptr; core::IApplicationManager::setInstance( nullptr );
    Thread::setCurrentTask( previousTask ); TypeManager::setInstance( nullptr ); types.unload();
    return result;
}

