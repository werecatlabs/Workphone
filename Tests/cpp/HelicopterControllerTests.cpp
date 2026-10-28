#include <Workphone/Workphone.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/Timer.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Input/InputDeviceManager.hpp>
#include <Workphone/Scene/Components/HelicopterController.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <WPVehiclePhysics/WPVehiclePhysics.hpp>
#include <WPVehiclePhysics/CVehicleManager.hpp>
#include <WPVehiclePhysics/HelicopterController.hpp>
#include <WPVehiclePhysics/AdvancedHelicopterController.hpp>
#include <WPVehiclePhysics/ExtremeHelicopterController.hpp>
#include <WPVehiclePhysics/DCSHelicopterController.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <WPVehiclePhysics/CAircraftBody.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <thread>
#include <type_traits>

using namespace workphone;

namespace
{
    static_assert( !std::is_abstract_v<vehicle::HelicopterController> );
    static_assert( !std::is_abstract_v<vehicle::AdvancedHelicopterController> );
    static_assert( !std::is_abstract_v<vehicle::ExtremeHelicopterController> );
    static_assert( !std::is_abstract_v<vehicle::DCSHelicopterController> );
    unsigned assertions = 0;
    void check( bool value, const char *message )
    {
        ++assertions;
        if( !value ) throw std::runtime_error( message );
    }
    bool nearHelicopter( real_Num a, real_Num b, real_Num tolerance = static_cast<real_Num>( 0.0001 ) )
    {
        return std::abs( a - b ) < tolerance;
    }
    bool finite( const Vector3<real_Num> &v )
    {
        return std::isfinite( v.X() ) && std::isfinite( v.Y() ) && std::isfinite( v.Z() );
    }

    class FixedTimer : public Timer
    {
    public:
        double delta = 0.01;
        time_interval getDeltaTime() const override { return delta; }
        time_interval getDeltaTime( TaskId ) const override { return delta; }
        time_interval getTime() const override { return 5.0; }
        time_interval getTime( TaskId ) const override { return 5.0; }
    };

    class Manager : public CVehicleManager
    {
    public:
        using CVehicleManager::getVehicles;
        using CVehicleManager::setVehicles;
    };

    // Records the production solver callback contract without starting a physics
    // device, loading a scene, or integrating nondeterministic wall-clock time.
    class Forces : public vehicle::IAircraftCallback
    {
    public:
        Vector3<real_Num> force = Vector3<real_Num>::zero();
        Vector3<real_Num> torque = Vector3<real_Num>::zero();
        Vector3<real_Num> position{ 0, 20, 0 };
        Quaternion<real_Num> orientation = Quaternion<real_Num>::identity();
        unsigned deliveries = 0;
        vehicle::IAircraft *probeAircraft = nullptr;
        bool heldDuringDelivery = false;
        void clear() { force = torque = Vector3<real_Num>::zero(); deliveries = 0; }
        String getData( const String & ) override { return {}; }
        void addForce( s32, const Vector3<real_Num> &value ) override
        {
            if( probeAircraft )
            {
                // A nonblocking probe on another thread proves that the whole
                // solver step, including callback dispatch, owns the same lock
                // used by scene-task configuration and callback retirement.
                auto aircraft = probeAircraft; probeAircraft = nullptr;
                std::thread probe( [&] {
                    const bool acquired = aircraft->try_lock();
                    heldDuringDelivery = !acquired;
                    if( acquired ) aircraft->unlock();
                } );
                probe.join();
            }
            force += value; ++deliveries;
        }
        void addForce( s32 body, const Vector3<real_Num> &value, const Vector3<real_Num> & ) override
        { addForce( body, value ); }
        void addTorque( s32, const Vector3<real_Num> &value ) override { torque += value; }
        void addLocalForce( s32 body, const Vector3<real_Num> &value, const Vector3<real_Num> & ) override
        { addForce( body, orientation * value ); }
        void addLocalTorque( s32 body, const Vector3<real_Num> &value ) override
        { addTorque( body, orientation * value ); }
        Vector3<real_Num> getAngularVelocity() const override { return Vector3<real_Num>::zero(); }
        Vector3<real_Num> getLinearVelocity() const override { return Vector3<real_Num>::zero(); }
        Vector3<real_Num> getLocalAngularVelocity() const override { return Vector3<real_Num>::zero(); }
        Vector3<real_Num> getLocalLinearVelocity() const override { return Vector3<real_Num>::zero(); }
        Vector3<real_Num> getPosition() const override { return position; }
        Vector3<real_Num> getScale() const override { return Vector3<real_Num>::unit(); }
        Quaternion<real_Num> getOrientation() const override { return orientation; }
        void displayLocalVector( s32, const Vector3<real_Num> &, const Vector3<real_Num> &, s32 ) const override {}
        void displayVector( s32, s32, const Vector3<real_Num> &, const Vector3<real_Num> &, s32 ) const override {}
        void displayLocalVector( s32, s32, const Vector3<real_Num> &, const Vector3<real_Num> &, s32 ) const override {}
        void *getCallbackFunction() const override { return nullptr; }
        void setCallbackFunction( void * ) override {}
        void *getCallbackDataFunction() const override { return nullptr; }
        void setCallbackDataFunction( void * ) override {}
        FixedArray<f32, 8> getInputData() const override { return {}; }
        FixedArray<f32, 11> getControlAngles() const override { return {}; }
        Vector3<real_Num> getPointVelocity( const Vector3<real_Num> & ) override
        { return Vector3<real_Num>::zero(); }
        bool castLocalRay( const Ray3<real_Num> &, SmartPtr<physics::IRaycastHit> & ) override { return false; }
        bool castWorldRay( const Ray3<real_Num> &, SmartPtr<physics::IRaycastHit> & ) override { return false; }
    };

    class Devices : public InputDeviceManager
    {
    public:
        Array<SmartPtr<IJoystick>> devices;
        Array<KeyCodes> keys;
        Array<SmartPtr<IJoystick>> getJoysticks() const override { return devices; }
        bool isKeyPressed( KeyCodes key ) const override
        { return std::find( keys.begin(), keys.end(), key ) != keys.end(); }
    };

    SmartPtr<Properties> controllerProfile()
    {
        auto p = make_ptr<Properties>();
        p->setName( "Vehicle Controller" );
        p->setProperty( "Device Name", String( "Synthetic helicopter stick" ) );
        p->setProperty( "Device Index", 0 ); p->setProperty( "Device Count", 1 );
        p->setProperty( "Device Axes", 6 ); p->setProperty( "Device Buttons", 14 );
        p->setProperty( "Helicopter Enabled", true );
        p->setProperty( "Helicopter Pitch Axis", 1 ); p->setProperty( "Helicopter Roll Axis", 0 );
        p->setProperty( "Helicopter Yaw Axis", 3 ); p->setProperty( "Helicopter Throttle Axis", 5 );
        p->setProperty( "Helicopter Throttle Mode", String( "01" ) );
        p->setProperty( "Helicopter Deadzone", 0.1f ); p->setProperty( "Helicopter Sensitivity", 1.0f );
        return p;
    }

    void propertiesContracts()
    {
        auto controller = make_ptr<scene::HelicopterController>();
        check( controller->getPhysicsSolution() == 0, "Arcade is the usable default helicopter solver" );
        check( controller->getAircraftControlProfile() == "helicopter" && controller->getAircraftPlayerControls(),
               "new helicopter components consume the helicopter profile by default" );
        auto properties = controller->getProperties();
        const auto &solution = properties->getPropertyObject( "Physics Solution" );
        check( solution.getType() == ParameterType::PARAM_TYPE_ENUM,
               "physics solution is an editor enum, not a free-form string" );
        const auto choices = solution.getAttribute( "enum" );
        check( choices.find( "Arcade" ) != String::npos && choices.find( "Advanced" ) != String::npos &&
               choices.find( "Extreme" ) != String::npos && choices.find( "DCS" ) != String::npos,
               "all four real helicopter solutions appear in the enum" );
        for( s32 value = 0; value != 4; ++value )
        {
            properties->setProperty( "Physics Solution", value );
            properties->setProperty( "Engine Throttle", 0.65f );
            controller->setProperties( properties );
            check( controller->getPhysicsSolution() == value && nearHelicopter( controller->getEngineThrottle(), 0.65f ),
                   "solver and engine throttle survive property deserialization" );
            auto copy = make_ptr<scene::HelicopterController>();
            copy->setProperties( controller->getProperties() );
            check( copy->getPhysicsSolution() == value && nearHelicopter( copy->getEngineThrottle(), 0.65f ),
                   "saved helicopter properties round-trip" );
        }
        controller->setPhysicsSolution( -1 ); controller->setPhysicsSolution( 4 );
        check( controller->getPhysicsSolution() == 3, "out-of-range solver indices preserve the current selection" );
        controller->setEngineThrottle( -1 );
        check( controller->getEngineThrottle() == 0, "negative engine throttle is clamped" );
        controller->setEngineThrottle( 2 );
        check( controller->getEngineThrottle() == 1, "engine throttle cannot exceed full governed power" );
        controller->setEngineThrottle( std::numeric_limits<f32>::quiet_NaN() );
        check( controller->getEngineThrottle() == 1, "non-finite engine throttle cannot poison the solver" );
        const auto lift = controller->getMaximumLift();
        controller->setMaximumLift( std::numeric_limits<real_Num>::infinity() );
        check( controller->getMaximumLift() == lift, "non-finite lift cannot enter the solver tuning" );
        properties = controller->getProperties(); properties->setProperty( "Vehicle Type", 0 );
        controller->setProperties( properties );
        check( controller->getVehicleType() == static_cast<s32>( scene::VehicleController::VehicleType::Aircraft ),
               "helicopter property loads preserve the aircraft backend regardless of inherited legacy vehicle type" );
        check( !controller->getAircraftController(), "editing properties does not allocate or register live physics" );
    }

    bool expectedSolver( SmartPtr<vehicle::IAircraft> value, s32 solver )
    {
        switch( solver )
        {
        case 0: return value->isDerived<vehicle::HelicopterController>();
        case 1: return value->isDerived<vehicle::AdvancedHelicopterController>();
        case 2: return value->isDerived<vehicle::ExtremeHelicopterController>();
        case 3: return value->isDerived<vehicle::DCSHelicopterController>();
        default: return false;
        }
    }

    void solverContracts( SmartPtr<core::ApplicationManager> app, SmartPtr<Manager> manager )
    {
        app->setPlaying( true ); app->setPaused( false );
        for( s32 solver = 0; solver != 4; ++solver )
        {
            auto controller = make_ptr<scene::HelicopterController>();
            controller->setPhysicsSolution( solver );
            controller->setAircraftControls( 0.8f, 0.2f, -0.3f, 0.4f );
            Thread::setCurrentTask( TaskId::Render ); controller->update();
            check( !controller->getAircraftController(), "render task cannot allocate helicopter physics" );
            Thread::setCurrentTask( TaskId::Application ); controller->update();
            auto aircraft = controller->getAircraftController();
            check( aircraft && expectedSolver( aircraft, solver ), "selected enum creates the matching real backend" );
            check( !aircraft->isDerived<vehicle::CAircraft>(), "rotor physics cannot masquerade as the fixed-wing implementation" );
            check( !aircraft->isElectric() && aircraft->getEnablePowerUnit() && !aircraft->getEmulateBattery(),
                   "helicopter backends default to enabled combustion power without battery emulation" );
            aircraft->setElectric( true ); aircraft->setEnablePowerUnit( false ); aircraft->setEmulateBattery( true );
            check( aircraft->isElectric() && !aircraft->getEnablePowerUnit() && aircraft->getEmulateBattery(),
                   "all concrete rotor backends retain their IVehicle power configuration" );
            manager->update();
            check( aircraft->getEngineRPM( 0 ) == 0, "a disabled power unit cannot spool a stationary rotor" );
            aircraft->setElectric( false ); aircraft->setEnablePowerUnit( true ); aircraft->setEmulateBattery( false );
            auto body = make_ptr<vehicle::CAircraftBody>(); aircraft->setBody( body );
            check( aircraft->getBody() == body && aircraft->getBodyPtr() == body.get(),
                   "the retained and borrowed body accessors describe the same assigned body" );
            aircraft->setBody( nullptr );
            check( !aircraft->getBody() && !aircraft->getBodyPtr(), "clearing a rotor body clears both accessor forms" );
            check( manager->getVehicles()->size() == 1, "one helicopter component registers exactly one physics vehicle" );
            check( nearHelicopter( aircraft->getChannel( 1 ), -0.3f ) && nearHelicopter( aircraft->getChannel( 2 ), 0.2f ) &&
                   nearHelicopter( aircraft->getChannel( 3 ), 0.4f ), "cyclic and pedals reach the real selected solver" );
            check( nearHelicopter( aircraft->getChannel( solver == 0 ? 0 : 5 ), 0.8f ),
                   "collective reaches the channel consumed by each selected solution" );
            check( nearHelicopter( aircraft->getProperties()->getPropertyAsFloat( "Maximum Lift" ), 22000, 0.1f ),
                   "each backend receives the same authored maximum-lift tuning" );
            auto forces = make_ptr<Forces>();
            forces->probeAircraft = aircraft.get();
            aircraft->setVehicleCallback( forces ); aircraft->setCallback( forces );
            for( unsigned step = 0; step != 500; ++step ) manager->update();
            check( forces->heldDuringDelivery, "solver simulation and callback delivery hold the shared physics mutation lock" );
            check( forces->deliveries == 500 && finite( forces->force ) && finite( forces->torque ),
                   "normal no-argument manager updates step and dispatch finite helicopter forces" );
            check( forces->force.Y() > 0, "every selected solution produces real upward rotor thrust" );
            check( forces->torque.dotProduct( forces->torque ) > 0, "pedal/cyclic input produces real rotor control torque" );
            check( solver == 0 ? ( forces->torque.X() > 0 && forces->torque.Z() > 0 ) :
                                ( forces->force.Z() > 0 && forces->force.X() < 0 ),
                   "positive pitch and negative roll consistently command forward and left across physics solutions" );
            forces->clear();
            forces->orientation.fromAngleAxis( Math<real_Num>::pi() * static_cast<real_Num>( 0.5 ),
                                               Vector3<real_Num>::unitZ() );
            manager->update();
            check( forces->force.X() < 0 && std::abs( forces->force.Y() ) < std::abs( forces->force.X() ) * 0.2f,
                   "rotor force uses the current callback orientation in the same step" );
            controller->unload( nullptr );
            check( manager->getVehicles()->empty() && !aircraft->getVehicleCallback() && !aircraft->getCallback(),
                   "unload unregisters physics and detaches both callback interfaces" );
        }
        app->setPlaying( false );
    }

    void switchingContracts( SmartPtr<core::ApplicationManager> app, SmartPtr<FactoryManager> factories,
                             SmartPtr<Manager> manager )
    {
        auto controller = make_ptr<scene::HelicopterController>();
        controller->setAircraftControls( 0.6f, 0.2f, 0.3f, -0.4f );
        app->setPlaying( false ); controller->update();
        check( !controller->getAircraftController(), "edit mode does not create live helicopter physics" );
        app->setPlaying( true ); controller->update();
        auto first = controller->getAircraftController();
        check( first && expectedSolver( first, 0 ), "initial active update prepares Arcade physics" );
        controller->setPhysicsSolution( 3 );
        check( controller->getAircraftController() == first, "property changes stage the new solver until the scene update" );
        Thread::setCurrentTask( TaskId::Render ); controller->update();
        check( controller->getAircraftController() == first, "render task cannot replace active physics" );
        Thread::setCurrentTask( TaskId::Application ); controller->update();
        auto second = controller->getAircraftController();
        check( second && second != first && expectedSolver( second, 3 ) && manager->getVehicles()->size() == 1,
               "scene update commits exactly one matching replacement solver" );
        check( !first->getVehicleCallback() && !first->getCallback(),
               "retired solver callbacks cannot keep applying forces after a switch" );
        controller->setPhysicsSolution( 3 ); controller->update();
        check( controller->getAircraftController() == second, "selecting the current solution is idempotent" );

        auto missing = factories->getFactoryByName( "workphone::vehicle::ExtremeHelicopterController" );
        check( missing != nullptr, "physics implementations use their fully qualified factory identity" );
        factories->removeFactory( missing );
        controller->setPhysicsSolution( 2 ); controller->update();
        check( controller->getAircraftController() == second && manager->getVehicles()->size() == 1 &&
               second->getVehicleCallback() && second->getCallback(),
               "a missing exact solver factory preserves the working helicopter without an arbitrary fallback" );
        factories->addFactory( missing ); controller->update();
        auto third = controller->getAircraftController();
        check( third && third != second && expectedSolver( third, 2 ) && manager->getVehicles()->size() == 1,
               "a pending switch succeeds once its exact solver becomes available" );
        check( !second->getVehicleCallback() && !second->getCallback(), "retry detaches the previous solution" );

        FactoryUtil::addFactory<scene::HelicopterController>();
        auto arcade = factories->getFactoryByName( "workphone::vehicle::HelicopterController" );
        factories->removeFactory( arcade ); controller->setPhysicsSolution( 0 ); controller->update();
        check( controller->getAircraftController() == third && manager->getVehicles()->size() == 1,
               "missing Arcade physics never instantiates the scene component with the same short class name" );
        factories->addFactory( arcade ); controller->setPhysicsSolution( 2 );

        auto properties = controller->getProperties(); properties->setButtonPressed( "Reset", true );
        controller->setProperties( properties ); controller->update();
        check( controller->getAircraftController() != third && manager->getVehicles()->size() == 1,
               "Reset recreates the selected solution to clear rotor and inflow state" );
        controller->unload( nullptr );
        check( manager->getVehicles()->empty(), "switched helicopter unload leaves no retired vehicle registrations" );
        app->setPlaying( false );
    }

    void inputAndGatingContracts( SmartPtr<core::ApplicationManager> app,
                                 SmartPtr<Manager> manager, SmartPtr<FixedTimer> timer )
    {
        auto input = make_ptr<Devices>(); input->setCreateJoysticks( true );
        auto stick = make_ptr<Joystick>(); stick->setName( "Synthetic helicopter stick" );
        stick->setNumAxes( 6 ); stick->setNumButtons( 14 );
        stick->updateAxisValue( 1, 0.55f ); stick->updateAxisValue( 0, -0.55f );
        stick->updateAxisValue( 3, 1.0f ); stick->updateAxisValue( 5, 0.55f );
        input->devices = { stick }; app->setInputDeviceManager( input );
        auto settings = make_ptr<Properties>(); settings->addChild( controllerProfile() );
        app->setPlayerSettings( settings ); app->setPlaying( true ); app->setPaused( false );
        auto controller = make_ptr<scene::HelicopterController>();
        controller->update(); auto aircraft = controller->getAircraftController();
        check( aircraft && nearHelicopter( aircraft->getChannel( 0 ), 0.5f ) && nearHelicopter( aircraft->getChannel( 1 ), -0.5f ) &&
               nearHelicopter( aircraft->getChannel( 2 ), 0.5f ) && nearHelicopter( aircraft->getChannel( 3 ), 1 ),
               "helicopter profile samples physical cyclic, pedals, and collective" );
        stick->updateAxisValue( 1, -1 );
        Thread::setCurrentTask( TaskId::Render ); controller->update();
        check( nearHelicopter( aircraft->getChannel( 2 ), 0.5f ), "render update cannot publish flight input changes" );
        Thread::setCurrentTask( TaskId::Application ); controller->update();
        check( nearHelicopter( aircraft->getChannel( 2 ), -1 ), "application update publishes the changed cyclic input" );
        input->keys = { KeyCodes::KEY_SPACE, KeyCodes::KEY_KEY_W };
        input->devices.clear(); controller->update();
        check( aircraft->getChannel( 0 ) == 0 && aircraft->getChannel( 1 ) == 0 &&
               aircraft->getChannel( 2 ) == 0 && aircraft->getChannel( 3 ) == 0,
               "disconnect neutralizes a configured controller and cannot silently fall back to held keyboard keys" );

        input->devices = { stick }; input->keys.clear(); controller->update();
        auto forces = make_ptr<Forces>(); aircraft->setVehicleCallback( forces ); aircraft->setCallback( forces );
        manager->update(); check( forces->deliveries == 1, "active helicopter steps through the vehicle manager" );
        forces->clear(); timer->delta = 0; manager->update();
        check( forces->deliveries == 0, "zero simulation time cannot advance rotor physics" );
        timer->delta = std::numeric_limits<double>::quiet_NaN(); manager->update();
        check( forces->deliveries == 0, "non-finite simulation time cannot enter rotor physics" );
        timer->delta = 0.01;
        app->setPaused( true ); controller->update(); manager->update();
        check( forces->deliveries == 0 && !controller->getAircraftController() &&
               controller->getAircraftThrottle() == 0 && controller->getAircraftPitch() == 0,
               "paused Play neutralizes helicopter input and suppresses force delivery" );
        app->setPaused( false ); controller->setEnabled( false ); controller->update(); manager->update();
        check( forces->deliveries == 0 && !controller->getAircraftController() && controller->getAircraftThrottle() == 0,
               "disabled helicopter components cannot apply rotor forces" );
        controller->setEnabled( true ); app->setPlaying( false ); controller->update(); manager->update();
        check( forces->deliveries == 0 && !controller->getAircraftController() && controller->getAircraftThrottle() == 0,
               "leaving Play prevents helicopter simulation" );
        controller->unload( nullptr ); app->setInputDeviceManager( nullptr );
    }

    void keyboardContracts( SmartPtr<core::ApplicationManager> app )
    {
        auto input = make_ptr<Devices>();
        input->keys = { KeyCodes::KEY_SPACE, KeyCodes::KEY_KEY_W, KeyCodes::KEY_KEY_D, KeyCodes::KEY_KEY_E };
        app->setInputDeviceManager( input ); app->setPlayerSettings( make_ptr<Properties>() );
        app->setPlaying( true ); app->setPaused( false );
        auto controller = make_ptr<scene::HelicopterController>(); controller->update();
        auto aircraft = controller->getAircraftController();
        check( aircraft && aircraft->getChannel( 0 ) > 0 && aircraft->getChannel( 0 ) < 1 &&
               aircraft->getChannel( 1 ) == 1 && aircraft->getChannel( 2 ) == 1 && aircraft->getChannel( 3 ) == 1,
               "unconfigured helicopters have usable keyboard cyclic, pedals, and gradual collective" );
        const auto collective = aircraft->getChannel( 0 ); input->keys.clear(); controller->update();
        check( nearHelicopter( aircraft->getChannel( 0 ), collective ) && aircraft->getChannel( 1 ) == 0 &&
               aircraft->getChannel( 2 ) == 0 && aircraft->getChannel( 3 ) == 0,
               "releasing keyboard cyclic and pedals centers them while retaining collective" );
        input->keys = { KeyCodes::KEY_LCONTROL }; controller->update();
        check( aircraft->getChannel( 0 ) < collective, "left Control lowers collective" );
        controller->unload( nullptr ); app->setInputDeviceManager( nullptr ); app->setPlaying( false );
    }

    void detachedPrefabContracts( SmartPtr<core::ApplicationManager> app, SmartPtr<Manager> manager )
    {
        app->setPlaying( true ); app->setPaused( false );
        auto actor = make_ptr<scene::GameActor>(); actor->setEnabled( true, false );
        auto controller = make_ptr<scene::HelicopterController>(); controller->setActor( actor );
        controller->update();
        check( actor->isEnabledInScene() && !actor->getScene() && !controller->getAircraftController() &&
               manager->getVehicles()->empty(),
               "an enabled detached prefab candidate cannot allocate or register helicopter physics during Play" );
        auto scene = make_ptr<scene::GameScene>(); actor->setScene( scene ); controller->update();
        check( controller->getAircraftController() && manager->getVehicles()->size() == 1,
               "the committed prefab starts helicopter physics after scene attachment" );
        auto disabled = controller->getAircraftController(); actor->setEnabled( false, false ); controller->update();
        check( !controller->getAircraftController() && manager->getVehicles()->empty() &&
               !disabled->getVehicleCallback() && !disabled->getCallback(),
               "an inactive actor cannot leave its rotor simulation registered" );
        actor->setEnabled( true, false ); controller->update();
        check( controller->getAircraftController() && manager->getVehicles()->size() == 1,
               "reactivating an attached helicopter prepares one fresh physics backend" );
        auto retired = controller->getAircraftController(); actor->setScene( nullptr ); controller->update();
        check( !controller->getAircraftController() && manager->getVehicles()->empty() &&
               !retired->getVehicleCallback() && !retired->getCallback(),
               "detaching a helicopter removes its physics registration and callbacks" );
        controller->setActor( nullptr ); controller->unload( nullptr ); app->setPlaying( false );
    }

    void fixedWingCompatibilityContracts( SmartPtr<core::ApplicationManager> app, SmartPtr<Manager> manager )
    {
        check( vehicle::CAircraft::typeInfo() != vehicle::CAerodynamicsVehicle<vehicle::IAircraft>::typeInfo() &&
               vehicle::CAircraft::typeInfo() != vehicle::HelicopterController::typeInfo() &&
               vehicle::CAircraft::typeInfo() != vehicle::AdvancedHelicopterController::typeInfo() &&
               vehicle::CAircraft::typeInfo() != vehicle::ExtremeHelicopterController::typeInfo() &&
               vehicle::CAircraft::typeInfo() != vehicle::DCSHelicopterController::typeInfo(),
               "fixed-wing physics has a concrete type identity distinct from every rotor implementation" );
        app->setPlaying( true ); app->setPaused( false );
        auto controller = make_ptr<scene::VehicleController>();
        controller->setVehicleType( static_cast<s32>( scene::VehicleController::VehicleType::Aircraft ) );
        controller->setAircraftControls( 0.7f, 0.2f, -0.3f, 0.4f ); controller->update();
        auto aircraft = controller->getAircraftController();
        check( aircraft && aircraft->isDerived<vehicle::CAircraft>() && manager->getVehicles()->size() == 1,
               "generic scene aircraft keep the fixed-wing backend when all helicopter factories are registered" );
        check( nearHelicopter( aircraft->getChannel( 0 ), 0.7f ) && nearHelicopter( aircraft->getChannel( 1 ), -0.3f ) &&
               nearHelicopter( aircraft->getChannel( 2 ), 0.2f ) && nearHelicopter( aircraft->getChannel( 3 ), 0.4f ),
               "fixed-wing throttle, roll, pitch, and yaw retain their existing channel semantics" );
        controller->setVehicleType( static_cast<s32>( scene::VehicleController::VehicleType::Car ) );
        check( !controller->getAircraftController() && manager->getVehicles()->empty() &&
               !aircraft->getVehicleCallback() && !aircraft->getCallback(),
               "switching a generic component back to car mode releases its fixed-wing backend" );
        controller->unload( nullptr ); app->setPlaying( false );
    }
}

int main()
{
    TypeManager types; types.load(); TypeManager::setInstance( &types );
    auto app = make_ptr<core::ApplicationManager>(); core::IApplicationManager::setInstance( app );
    auto factories = make_ptr<FactoryManager>(); factories->load( nullptr ); app->setFactoryManager( factories );
    auto timer = make_ptr<FixedTimer>(); app->setTimer( timer );
    auto manager = make_ptr<Manager>(); app->setVehicleManager( manager );
    auto plugin = make_ptr<WPVehiclePhysics>();
    const auto previousTask = Thread::getCurrentTask();
    Thread::setCurrentTask( TaskId::Application );
    int result = 0;
    try
    {
        plugin->load( nullptr );
        propertiesContracts(); solverContracts( app, manager );
        switchingContracts( app, factories, manager ); inputAndGatingContracts( app, manager, timer );
        keyboardContracts( app );
        detachedPrefabContracts( app, manager );
        fixedWingCompatibilityContracts( app, manager );
        std::printf( "Helicopter controller: %u assertions passed.\n", assertions );
    }
    catch( const std::exception &e )
    { std::fprintf( stderr, "Helicopter controller FAIL: %s\n", e.what() ); result = 1; }
    manager->setVehicles( nullptr ); app->setVehicleManager( nullptr );
    plugin->unload( nullptr ); plugin = nullptr;
    app->setInputDeviceManager( nullptr ); app->setTimer( nullptr );
    factories->unload( nullptr ); app->setFactoryManager( nullptr );
    app = nullptr; core::IApplicationManager::setInstance( nullptr );
    Thread::setCurrentTask( previousTask ); TypeManager::setInstance( nullptr ); types.unload();
    return result;
}
