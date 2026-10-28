#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/HelicopterController.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Interface/Vehicle/IAircraft.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include "AircraftControllerInput.hpp"
#include <cmath>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, HelicopterController, VehicleController );

    HelicopterController::HelicopterController()
    {
        m_vehicleType = VehicleType::Aircraft;
        m_mass = 1050.0f;
        m_moi = Vector3<real_Num>( 1400, 2200, 1800 );
        m_rotorTuning = workphone::make_ptr<Properties>();
        m_rotorTuning->setProperty( "Maximum Lift", getMaximumLift() );
        useAircraftPlayerControls( "helicopter" );
    }
    HelicopterController::~HelicopterController() = default;

    bool HelicopterController::isSimulationActive() const
    {
        const auto app = core::IApplicationManager::instancePtr();
        const auto actor = getActor();
        return app && app->isPlaying() && !app->isPaused() && isEnabled() &&
               ( !actor || ( actor->getScene() && actor->isEnabledInScene() ) );
    }
    bool HelicopterController::shouldCreateAircraftController() const
    {
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        return isSimulationActive() &&
               Thread::getCurrentTask() == ( manager ? manager->getSceneTask() : TaskId::Application );
    }
    void HelicopterController::neutralizeControls()
    {
        setAircraftThrottle( 0 ); setAircraftPitch( 0 ); setAircraftRoll( 0 ); setAircraftYaw( 0 );
        m_keyboardCollective = 0;
    }
    void HelicopterController::samplePlayerControls()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto input = app ? app->getInputDeviceManager() : nullptr;
        auto settings = app ? app->getPlayerSettings() : nullptr;
        auto profile = settings ? settings->getChild( "Vehicle Controller" ) : nullptr;
        bool controllerEnabled = false;
        if( profile ) profile->getPropertyValue( "Helicopter Enabled", controllerEnabled );
        if( controllerEnabled )
        {
            const auto value = aircraft_input::sample( profile,
                input && input->getCreateJoysticks() ? input->getJoysticks() : Array<SmartPtr<IJoystick>>(), "helicopter" );
            setAircraftThrottle( value.throttle ); setAircraftPitch( value.pitch );
            setAircraftRoll( value.roll ); setAircraftYaw( value.yaw );
            m_keyboardCollective = 0;
            return;
        }
        if( !input ) { neutralizeControls(); return; }
        auto timer = app->getTimerPtr();
        const auto rawDt = timer ? timer->getDeltaTime() : 0.0;
        const auto dt = std::isfinite( rawDt ) ? Math<f32>::clamp( static_cast<f32>( rawDt ), 0.f, .1f ) : 0.f;
        const auto key = [&]( KeyCodes code ) { return input->isKeyPressed( code ) ? 1.f : 0.f; };
        m_keyboardCollective = Math<f32>::clamp( m_keyboardCollective + dt * .35f *
            ( key( KeyCodes::KEY_SPACE ) - key( KeyCodes::KEY_LCONTROL ) ), 0.f, 1.f );
        setAircraftThrottle( m_keyboardCollective );
        setAircraftPitch( key( KeyCodes::KEY_KEY_W ) - key( KeyCodes::KEY_KEY_S ) );
        setAircraftRoll( key( KeyCodes::KEY_KEY_D ) - key( KeyCodes::KEY_KEY_A ) );
        setAircraftYaw( key( KeyCodes::KEY_KEY_E ) - key( KeyCodes::KEY_KEY_Q ) );
    }
    void HelicopterController::update()
    {
        // Several tasks update components; only the scene task owns physics/input.
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        const auto sceneTask = manager ? manager->getSceneTask() : TaskId::Application;
        if( Thread::getCurrentTask() != sceneTask ) return;
        Component::update();
        if( !isSimulationActive() )
        {
            neutralizeControls(); destroyAircraftController();
            m_activePhysicsSolution = -1;
            return;
        }
        const auto reset = m_resetPhysics.exchange( false );
        if( m_activePhysicsSolution != getPhysicsSolution() || reset )
        {
            const auto previous = m_activePhysicsSolution;
            m_activePhysicsSolution = getPhysicsSolution();
            if( !createAircraftController( true ) ) m_activePhysicsSolution = previous;
        }
        if( getAircraftPlayerControls() ) samplePlayerControls();
        if( !m_aircraftController ) createAircraftController();
        applyAircraftConfiguration();
    }
    void HelicopterController::unload( SmartPtr<ISharedObject> data )
    {
        neutralizeControls(); m_activePhysicsSolution = -1;
        VehicleController::unload( data );
    }
    String HelicopterController::getAircraftImplementationName() const
    {
        switch( static_cast<PhysicsSolution>( getPhysicsSolution() ) )
        {
        case PhysicsSolution::Advanced: return "workphone::vehicle::AdvancedHelicopterController";
        case PhysicsSolution::Extreme: return "workphone::vehicle::ExtremeHelicopterController";
        case PhysicsSolution::DCS: return "workphone::vehicle::DCSHelicopterController";
        default: return "workphone::vehicle::HelicopterController";
        }
    }
    void HelicopterController::applyAircraftConfiguration()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        if( Thread::getCurrentTask() != ( manager ? manager->getSceneTask() : TaskId::Application ) ) return;
        if( !m_aircraftController ) return;
        ScopedLock<vehicle::IAircraft> simulationLock( m_aircraftController );
        VehicleController::applyAircraftConfiguration();
        const auto collective = getAircraftThrottle();
        const auto activeSolution = m_activePhysicsSolution < 0 ? getPhysicsSolution() : m_activePhysicsSolution;
        const auto engine = getEngineThrottle();
        m_aircraftController->setChannel( 0, activeSolution == 0 ? collective * engine : engine );
        m_aircraftController->setChannel( 5, collective );
        const auto lift = getMaximumLift();
        if( lift != m_appliedMaximumLift )
        {
            m_rotorTuning->setProperty( "Maximum Lift", lift );
            m_appliedMaximumLift = lift;
        }
        m_aircraftController->setProperties( m_rotorTuning );
    }
    SmartPtr<Properties> HelicopterController::getProperties() const
    {
        auto properties = VehicleController::getProperties();
        properties->removeProperty( tireModelStr );
        properties->removeProperty( vehicleTypeStr );
        properties->setPropertyAsEnum( "Physics Solution", getPhysicsSolution(),
            Array<String>( { "Arcade", "Advanced", "Extreme", "DCS" } ) );
        properties->setProperty( "Engine Throttle", getEngineThrottle() );
        properties->setProperty( "Maximum Lift", getMaximumLift() );
        return properties;
    }
    void HelicopterController::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties ) return;
        s32 solution = getPhysicsSolution(); f32 throttle = getEngineThrottle(); real_Num lift = getMaximumLift();
        properties->getPropertyValue( "Physics Solution", solution );
        properties->getPropertyValue( "Engine Throttle", throttle );
        properties->getPropertyValue( "Maximum Lift", lift );
        setPhysicsSolution( solution ); setEngineThrottle( throttle ); setMaximumLift( lift );
        // The specialized component never enters the car branch of the base setter.
        auto values = workphone::make_ptr<Properties>();
        *values = *properties;
        values->setProperty( vehicleTypeStr, static_cast<s32>( VehicleType::Aircraft ) );
        VehicleController::setProperties( values );
        m_vehicleType = VehicleType::Aircraft;
        setAircraftControlProfile( "helicopter" );
        if( properties->isButtonPressed( resetStr ) || properties->isButtonPressed( resetTransformStr ) )
        {
            neutralizeControls(); m_resetPhysics = true;
        }
        if( properties->isButtonPressed( resetTransformStr ) )
        {
            if( auto chassis = getChassis() )
            {
                if( auto body = chassis->getRigidDynamic() )
                {
                    auto actor = getActor();
                    if( actor ) body->setTransform( Transform3<real_Num>( actor->getPosition(), actor->getOrientation(), Vector3<real_Num>::unit() ) );
                }
            }
        }
    }
    s32 HelicopterController::getPhysicsSolution() const { return m_physicsSolution.load(); }
    void HelicopterController::setPhysicsSolution( s32 solution )
    {
        if( solution >= 0 && solution <= static_cast<s32>( PhysicsSolution::DCS ) ) m_physicsSolution = solution;
    }
    f32 HelicopterController::getEngineThrottle() const { return m_engineThrottle.load(); }
    void HelicopterController::setEngineThrottle( f32 throttle )
    {
        if( std::isfinite( throttle ) ) m_engineThrottle = Math<f32>::clamp( throttle, 0.f, 1.f );
    }
    real_Num HelicopterController::getMaximumLift() const { return m_maximumLift.load(); }
    void HelicopterController::setMaximumLift( real_Num lift )
    {
        if( std::isfinite( lift ) ) m_maximumLift = Math<real_Num>::clamp( lift,
            static_cast<real_Num>( 1 ), static_cast<real_Num>( 10000000 ) );
    }
}
