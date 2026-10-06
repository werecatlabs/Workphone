#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CAircraftEngine.hpp"
#include "WPVehiclePhysics/CAircraftPropeller.hpp"
#include "WPVehiclePhysics/CAircraftBody.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    CAircraftEngine::CAircraftEngine()
    {
        setRunning( true );
        setMaxPower( 700 );
        m_peakPowerRevs = 0;
        m_peakPowerW = 0;
        m_enginePower = 0;
        m_engineTorque = 0;
        setThrottle( 0 );

        m_peakPowerRevs = 12000;
    }

    CAircraftEngine::~CAircraftEngine()
    {
    }

    void CAircraftEngine::load( SmartPtr<ISharedObject> data )
    {
        auto properties = workphone::dynamic_pointer_cast<Properties>( data );
        if( properties )
        {
            // properties->getPropertyValue("FlEqPeakPowerRevs", PeakPowerRevs);
            // properties->getPropertyValue("FlEqPeakPowerW", PeakPowerW);
            // properties->getPropertyValue("FlEqEnginePower", EnginePower);
            // properties->getPropertyValue("FlEqEngineTorque", EngineTorque);

            properties->getPropertyValue( "FlEqEngineMaxRPM", m_maxRPM );
            properties->getPropertyValue( "FlEqEnginePeakRPM", m_peakPowerRevs );
            properties->getPropertyValue( "FlEqPeakPowerW", m_peakPowerW );
            properties->getPropertyValue( "FlEqEnginePower", m_maxPower );
            properties->getPropertyValue( "FlEqEngineTorque", m_engineTorque );
            properties->getPropertyValue( "FlEqEngineMoI", m_moi );
        }

        setRunning( true );
    }

    void CAircraftEngine::load( void *pData )
    {
        // auto data = static_cast<data::aircraft_engine_data *>(pData);
        // m_torqueMultiplier = data->torqueMultiplier;
        // m_thrustMultiplier = data->thrustMultiplier;

        // data::vec4 p = data->localTransform.position;
        // data::vec4 q = data->localTransform.orientation;
        // data::vec4 s = data->localTransform.scale;

        // auto vPos = Vector3<real_Num>( p.x, p.y, -p.z );
        // auto vScale = Vector3<real_Num>( s.x, s.y, s.z );
        // auto qRot = Quaternion<real_Num>( q.w, -q.x, -q.y, q.z );

        // WP_ASSERT( vScale.length() > std::numeric_limits<f32>::epsilon() );

        // m_localTransform.setPosition( vPos );
        // m_localTransform.setScale( vScale );
        // m_localTransform.setOrientation( qRot );
    }

    void CAircraftEngine::update( const double &time, const double &deltaTime )
    {
        auto throttlePos =
            static_cast<real_Num>( 0.8 ) - m_parentAircraft->getChannel( CAircraft::m_thrChannel );
        setThrottle( throttlePos );

        auto idleRPM = m_peakPowerRevs * 0.01;
        auto throttle = throttlePos / ( 0.8 * 2.0 );
        auto rpm = idleRPM + ( ( m_peakPowerRevs - idleRPM ) * throttle );
        // setRPM(rpm);

        auto peakPowerW = getPeakPowerRevs() * ( Math<real_Num>::pi() / static_cast<real_Num>( 30.0 ) );
        setPeakPowerW( peakPowerW );

        // auto maxPower = m_maxRPM * (Math<real_Num>::pi() / real_Num(30.0));
        // setMaxPower(m_maxPower);

        SmartPtr<CAircraftPropeller> pPropeller =
            workphone::static_pointer_cast<CAircraftPropeller>( m_propeller );
        CAircraftPropeller &thisProp = *pPropeller;

        auto enginePower = static_cast<real_Num>( 1.53 ) *
                           ( static_cast<real_Num>( 1.25 ) * getThrottle() -
                             static_cast<real_Num>( 0.6 ) * thisProp.m_wProp / getPeakPowerW() ) *
                           getMaxPower() * thisProp.m_wProp / getPeakPowerW();

        // setEnginePower(enginePower);
        setEnginePower( getMaxPower() * throttle );

        /*   If WProp<=PeakPowerW Then
            EnginePower:=(1.5*Throttle-0.8*(WProp-100)/PeakPowerW)*MaxEngPower*WProp/PeakPowerW
           Else EnginePower:=Throttle*MaxEngPower*(1-2*(WProp-PeakPowerW)/PeakPowerW);*/
        if( getEnginePower() > getMaxPower() )
        {
            setEnginePower( getMaxPower() );
            // limit the engine power to the Max quoted value
        }

        if( getEnginePower() < static_cast<real_Num>( -0.3 ) * getMaxPower() )
        {
            // limit the engine braking to 30% of the max output
            setEnginePower( static_cast<real_Num>( -0.3 ) * getMaxPower() );
        }

        if( m_parentAircraft->getDisplayDebugData() )
        {
            auto localTransform = getLocalTransform();
            auto localPosition = localTransform.getPosition();
            m_parentAircraft->drawPoint( 0, 1665165123, localPosition, 0xFF0000 );
        }

        // static auto nextUpdate = 0.0;
        // if (nextUpdate < time)
        //{
        //	WP_LOG("RPM: " + StringUtil::toString(getRPM()));
        //	WP_LOG("Torque: " + StringUtil::toString(getTorque()));
        //	nextUpdate = time + 3.0;
        // }
    }

    IAircraftPropeller *CAircraftEngine::getPropellerPtr() const
    {
        return m_propeller.get();
    }

    SmartPtr<IAircraftPropeller> CAircraftEngine::getPropeller() const
    {
        return m_propeller;
    }

    void CAircraftEngine::setPropeller( SmartPtr<IAircraftPropeller> propeller )
    {
        m_propeller = propeller;
    }

    real_Num CAircraftEngine::getRPM() const
    {
        return m_rpm;
    }

    void CAircraftEngine::setRPM( real_Num rpm )
    {
        m_rpm = rpm;
    }

    bool CAircraftEngine::isValid() const
    {
        return Math<real_Num>::isFinite( getMaxPower() ) &&
               Math<real_Num>::isFinite( m_peakPowerRevs ) && Math<real_Num>::isFinite( m_peakPowerW ) &&
               Math<real_Num>::isFinite( m_enginePower ) && Math<real_Num>::isFinite( m_engineTorque ) &&
               Math<real_Num>::isFinite( getThrottle() );
    }

    bool CAircraftEngine::isRunning() const
    {
        return m_running;
    }

    void CAircraftEngine::setRunning( bool running )
    {
        m_running = running;
    }

    real_Num CAircraftEngine::getMaxPower() const
    {
        return m_maxPower;
    }

    void CAircraftEngine::setMaxPower( real_Num maxPower )
    {
        m_maxPower = maxPower;
    }

    real_Num CAircraftEngine::getMaxRPM() const
    {
        return 0.0f;
    }

    void CAircraftEngine::setTorque( real_Num torque )
    {
        m_engineTorque = torque;
    }

    f32 CAircraftEngine::getTorque( f32 throttlePosition ) const
    {
        return 0.0f;
    }

    real_Num CAircraftEngine::getTorque() const
    {
        return m_engineTorque;
    }

    f32 CAircraftEngine::getMaxTorque( u32 rpm ) const
    {
        return 0.0f;
    }

    f32 CAircraftEngine::getMinTorque( u32 rpm ) const
    {
        return 0.0f;
    }

    real_Num CAircraftEngine::getThrottle() const
    {
        return m_throttle;
    }

    void CAircraftEngine::setThrottle( real_Num throttle )
    {
        m_throttle = throttle;
    }

    real_Num CAircraftEngine::getPeakPowerRevs() const
    {
        return m_peakPowerRevs;
    }

    void CAircraftEngine::setPeakPowerRevs( real_Num peakPowerRevs )
    {
        m_peakPowerRevs = peakPowerRevs;
    }

    real_Num CAircraftEngine::getPeakPowerW() const
    {
        return m_peakPowerW;
    }

    void CAircraftEngine::setPeakPowerW( real_Num peakPowerW )
    {
        m_peakPowerW = peakPowerW;
    }

    real_Num CAircraftEngine::getEnginePower() const
    {
        return m_enginePower;
    }

    void CAircraftEngine::setEnginePower( real_Num enginePower )
    {
        m_enginePower = enginePower;
    }

    real_Num CAircraftEngine::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void CAircraftEngine::setThrustMultiplier( real_Num thrustMultiplier )
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    real_Num CAircraftEngine::getTorqueMultiplier() const
    {
        return m_torqueMultiplier;
    }

    void CAircraftEngine::setTorqueMultiplier( real_Num torqueMultiplier )
    {
        m_torqueMultiplier = torqueMultiplier;
    }

    real_Num CAircraftEngine::getMoi() const
    {
        return m_moi;
    }

    void CAircraftEngine::setMoi( real_Num moi )
    {
        m_moi = moi;
    }

    bool CAircraftEngine::isElectric() const
    {
        return false;
    }

    void CAircraftEngine::setElectric( bool electric )
    {
    }
} // namespace workphone::vehicle
