#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CAircraftPropellerUnitSimple.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include "WPVehiclePhysics/CAircraftEngine.hpp"
#include "WPVehiclePhysics/CAircraftMotor.hpp"
#include "WPVehiclePhysics/CAircraftPropeller.hpp"
#include <Workphone/Core/Properties.hpp>
#include "WPVehiclePhysics/CAircraftBody.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    u32 CAircraftPropellerUnitSimple::m_idExt = 0;

    CAircraftPropellerUnitSimple::CAircraftPropellerUnitSimple()
    {
        m_id = StringUtil::parseInt( "PropellerUnit" + StringUtil::toString( m_idExt++ ) );
    }

    CAircraftPropellerUnitSimple::~CAircraftPropellerUnitSimple()
    {
    }

    Vector3<real_Num> CAircraftPropellerUnitSimple::getThrust() const
    {
        return m_thrust;
    }

    void CAircraftPropellerUnitSimple::setThrust( const Vector3<real_Num> &thrust )
    {
        m_thrust = thrust;
    }

    Vector3<real_Num> CAircraftPropellerUnitSimple::getPropwash() const
    {
        return m_thrust;
    }

    void CAircraftPropellerUnitSimple::setPropwash( const Vector3<real_Num> &propwash )
    {
    }

    bool CAircraftPropellerUnitSimple::isValid() const
    {
        return m_parent != nullptr && m_parentAircraft != nullptr && m_esc != nullptr &&
               m_batteryPack != nullptr && m_propeller != nullptr && m_powerUnit != nullptr;
    }

    void CAircraftPropellerUnitSimple::update( const double &time, const double &deltaTime )
    {
        WP_ASSERT( Math<real_Num>::isFinite( time ) );
        WP_ASSERT( Math<real_Num>::isFinite( deltaTime ) );

        auto aircraftBody = getParent();
        auto aircraft = getParentAircraft();

        WP_ASSERT( aircraftBody );
        WP_ASSERT( aircraft );

        SmartPtr<CAircraftPropeller> pPropeller =
            workphone::static_pointer_cast<CAircraftPropeller>( m_propeller );
        CAircraftPropeller &prop = *pPropeller;

        SmartPtr<CBatteryPackStandard> pBatteryPack =
            workphone::static_pointer_cast<CBatteryPackStandard>( m_batteryPack );
        CBatteryPackStandard &pack = *pBatteryPack;

        SmartPtr<CESController> pESC = workphone::static_pointer_cast<CESController>( m_esc );
        CESController          &esc = *pESC;

        auto aircraftTransform = aircraft->getBodyTransform();

        auto worldTransform = getWorldTransform();
        auto localTransform = getLocalTransform();

        auto velocity = aircraft->getLinearVelocity();

        if( Math<real_Num>::Abs( prop.m_wProp ) > std::numeric_limits<real_Num>::epsilon() )
        {
            auto propTorque = prop.m_totMoI / prop.m_wProp;
            prop.propTorque( propTorque );
        }

        if( m_powerUnit->isElectric() )
        {
            SmartPtr<CAircraftMotor> pEngine =
                workphone::static_pointer_cast<CAircraftMotor>( m_powerUnit );
            CAircraftMotor &motor = *pEngine;

            prop.m_totMoI = prop.m_propMoI + motor.getMoi(); // make sure the MoI total includes the
                                                             // motor
            motor.setMotorOmega( prop.m_wProp ); // pass the props current rotation rate to the Motor

            auto engineRPM = prop.m_wProp * static_cast<real_Num>( 30.0 ) / Math<real_Num>::pi();
            motor.setRPM( engineRPM );

            prop.m_inputTorque = motor.getTorque(); // pass the motor torque out to the prop
            WP_ASSERT( Math<real_Num>::isFinite( prop.m_inputTorque ) );
        }
        else
        {
            SmartPtr<CAircraftEngine> pEngine =
                workphone::static_pointer_cast<CAircraftEngine>( m_powerUnit );
            CAircraftEngine &thisEngine = *pEngine;

            if( prop.m_wProp < 0.01 )
            {
                prop.m_wProp = 0.01;
            }

            WP_ASSERT( prop.m_wProp < 1e10 );
            WP_ASSERT( Math<real_Num>::isFinite( prop.m_wProp ) );

            if( Math<real_Num>::Abs( prop.m_wProp ) > std::numeric_limits<real_Num>::epsilon() )
            {
                auto enginePower = thisEngine.getEnginePower();

                WP_ASSERT( Math<real_Num>::isFinite( prop.m_wProp ) );
                WP_ASSERT( Math<real_Num>::isFinite( enginePower ) );

                auto engineTorque = enginePower / prop.m_wProp;
                WP_ASSERT( Math<real_Num>::isFinite( engineTorque ) );

                thisEngine.setTorque( engineTorque );
            }
            else
            {
                thisEngine.setTorque( static_cast<real_Num>( 0.0 ) );
            }

            // thisEngine.setMotorOmega(prop.m_wProp); //pass the props current rotation rate to the
            // Motor

            auto engineRPM = prop.m_wProp * static_cast<real_Num>( 30.0 ) / Math<real_Num>::pi();
            WP_ASSERT( Math<real_Num>::isFinite( prop.m_inputTorque ) );

            m_powerUnit->setRPM( engineRPM );

            prop.m_inputTorque = thisEngine.getTorque(); // pass the motor torque out to the prop
            WP_ASSERT( Math<real_Num>::isFinite( prop.m_inputTorque ) );
        }

        auto deltaW = static_cast<real_Num>( 0.0 );
        if( prop.m_totMoI > std::numeric_limits<real_Num>::epsilon() )
        {
            deltaW = deltaTime * ( prop.m_inputTorque - prop.propTorque() ) / prop.m_totMoI;
            // calc the change in RPM in this timestep
        }

        WP_ASSERT( deltaW < 1e10 );
        WP_ASSERT( prop.m_wProp < 1e10 );

        // limit acceleration to 4000 radians/s (about 40,000 rpm/s)
        if( deltaW > static_cast<real_Num>( 4000.0 ) * deltaTime )
        {
            deltaW = static_cast<real_Num>( 4000.0 ) * deltaTime;
        }

        if( deltaW < static_cast<real_Num>( -4000.0 ) * deltaTime )
        {
            deltaW = static_cast<real_Num>( -4000.0 ) * deltaTime;
        }

        prop.m_wProp = prop.m_wProp + deltaW;

        auto engineRps = m_powerUnit->getRPM();

        auto factorA = static_cast<real_Num>( 1.0 );
        auto factorB = static_cast<real_Num>( 1.0 );
        auto enginePower = static_cast<real_Num>( 200.0 );
        auto factor = static_cast<real_Num>( 1.0 );

        auto diameter = m_propeller->getDiameter();
        auto throttle = m_powerUnit->getThrottle();
        auto velocityLength = velocity.length();

        auto advanceRatio = velocityLength / ( engineRps * diameter );
        auto thrust = 0.0;

        if( engineRps > std::numeric_limits<real_Num>::epsilon() )
        {
            thrust = throttle * factor * enginePower *
                     ( factorA + factorB * advanceRatio * advanceRatio ) / ( engineRps * diameter );
        }

        m_propeller->setThrustValue( thrust );

        if( m_parentAircraft->getEnablePowerUnit() )
        {
            auto transform = getLocalTransform();
            auto localPosition = transform.getPosition();

            auto thrust = m_propeller->getThrustValue() * Vector3<real_Num>::UNIT_Z *
                          m_powerUnit->getThrustMultiplier();
            setThrust( thrust );

            auto localTorque =
                -thrust.normaliseCopy() * m_powerUnit->getTorque() * m_powerUnit->getTorqueMultiplier();
            localTorque = vecToYFrame( localTorque );

            m_parent->addLocalForceAtLocalPosition( thrust, localPosition );
            m_parent->addLocalTorque( localTorque );
        }
    }

    SmartPtr<IBatteryPack> &CAircraftPropellerUnitSimple::getBatteryPack()
    {
        return m_batteryPack;
    }

    const SmartPtr<IBatteryPack> &CAircraftPropellerUnitSimple::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void CAircraftPropellerUnitSimple::setBatteryPack( SmartPtr<IBatteryPack> batteryPack )
    {
        m_batteryPack = batteryPack;
    }

    SmartPtr<IESController> &CAircraftPropellerUnitSimple::getESC()
    {
        return m_esc;
    }

    const SmartPtr<IESController> &CAircraftPropellerUnitSimple::getESC() const
    {
        return m_esc;
    }

    void CAircraftPropellerUnitSimple::setESC( SmartPtr<IESController> esc )
    {
        m_esc = esc;
    }

    SmartPtr<IAircraftPowerUnit> &CAircraftPropellerUnitSimple::getPowerUnit()
    {
        return m_powerUnit;
    }

    const SmartPtr<IAircraftPowerUnit> &CAircraftPropellerUnitSimple::getPowerUnit() const
    {
        return m_powerUnit;
    }

    void CAircraftPropellerUnitSimple::setPowerUnit( SmartPtr<IAircraftPowerUnit> powerUnit )
    {
        m_powerUnit = powerUnit;
    }

    SmartPtr<IAircraftPropeller> &CAircraftPropellerUnitSimple::getPropeller()
    {
        return m_propeller;
    }

    const SmartPtr<IAircraftPropeller> &CAircraftPropellerUnitSimple::getPropeller() const
    {
        return m_propeller;
    }

    void CAircraftPropellerUnitSimple::setPropeller( SmartPtr<IAircraftPropeller> propeller )
    {
        m_propeller = propeller;
    }
} // namespace workphone::vehicle
