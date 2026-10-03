#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CDifferential.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    CDifferential::CDifferential() = default;

    CDifferential::~CDifferential() = default;

    void CDifferential::setRatio( f32 ratio )
    {
        WP_ASSERT( ratio > 0.0f );
        if( ratio <= 0.0f )
        {
            WP_LOG_ERROR( "CDifferential::setRatio rejected non-positive ratio." );
            return;
        }

        m_ratio = ratio;
    }

    f32 CDifferential::getRatio() const
    {
        return m_ratio;
    }

    void CDifferential::setWheel( u32 idx, SmartPtr<IWheelComponent> wheel )
    {
        WP_ASSERT( idx < m_wheels.size() );
        if( idx >= m_wheels.size() )
        {
            WP_LOG_ERROR( "CDifferential::setWheel index out of range." );
            return;
        }

        m_wheels[idx] = wheel;
    }

    SmartPtr<IWheelComponent> CDifferential::getWheel( u32 idx ) const
    {
        WP_ASSERT( idx < m_wheels.size() );
        if( idx >= m_wheels.size() )
        {
            WP_LOG_ERROR( "CDifferential::getWheel index out of range." );
            return nullptr;
        }

        return m_wheels[idx];
    }

    void CDifferential::setWheelTorque( f32 wheelForce )
    {
        m_wheelTorque = wheelForce;

        const auto torquePerWheel = wheelForce * m_ratio / static_cast<f32>( m_wheels.size() );
        for( auto &wheel : m_wheels )
        {
            if( wheel )
            {
                wheel->setTorque( torquePerWheel );
            }
        }
    }

    f32 CDifferential::getLockCoefficient() const
    {
        return m_lockCoefficient;
    }

    void CDifferential::setLockCoefficient( f32 lockCoefficient )
    {
        WP_ASSERT( lockCoefficient >= 0.0f );
        m_lockCoefficient = Math<f32>::clamp( lockCoefficient, 0.0f, 1.0f );
    }

    SmartPtr<Properties> CDifferential::getProperties() const
    {
        auto properties = CVehicleComponent<IDifferential>::getProperties();
        WP_ASSERT( properties );

        properties->setProperty( "Ratio", getRatio() );
        properties->setProperty( "Lock Coefficient", getLockCoefficient() );
        properties->setProperty( "Wheel Torque", getWheelTorque() );

        return properties;
    }

    void CDifferential::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            WP_LOG_ERROR( "CDifferential::setProperties received null properties." );
            return;
        }

        CVehicleComponent<IDifferential>::setProperties( properties );

        f32 ratio = getRatio();
        f32 lockCoefficient = getLockCoefficient();
        f32 wheelTorque = getWheelTorque();

        properties->getPropertyValue( "Ratio", ratio );
        properties->getPropertyValue( "Lock Coefficient", lockCoefficient );
        properties->getPropertyValue( "Wheel Torque", wheelTorque );

        setRatio( ratio );
        setLockCoefficient( lockCoefficient );
        setWheelTorque( wheelTorque );
    }

    f32 CDifferential::getWheelTorque() const
    {
        return m_wheelTorque;
    }
} // namespace workphone
