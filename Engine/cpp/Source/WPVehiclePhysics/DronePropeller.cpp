#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/DronePropeller.hpp"

namespace workphone::vehicle
{
    DronePropeller::DronePropeller()
    {
    }

    DronePropeller::~DronePropeller()
    {
    }

    void DronePropeller::initialise( const std::string &objectID )
    {
    }

    void DronePropeller::calcThrust( const double &dt )
    {
    }

    void DronePropeller::calcGroundEffect( const Vector3<real_Num> &position )
    {
    }

    void DronePropeller::input( real_Num rpm, const double &dt )
    {
    }

    Vector3<real_Num> DronePropeller::getLocalPos() const
    {
        return Vector3<real_Num>( m_localPos.X(), m_localPos.Y(), m_localPos.Z() );
    }

    void DronePropeller::setLocalPos( const Vector3<real_Num> &localPos )
    {
    }

    int DronePropeller::getDebugId() const
    {
        return m_debugId;
    }

    void DronePropeller::setDebugId( int debugId )
    {
        m_debugId = debugId;
    }

    real_Num DronePropeller::getDiameter() const
    {
        return m_diameter;
    }

    void DronePropeller::setDiameter( real_Num diameter )
    {
        m_diameter = diameter;
    }

    real_Num DronePropeller::getPitch() const
    {
        return m_pitch;
    }

    void DronePropeller::setPitch( real_Num pitch )
    {
        m_pitch = pitch;
    }

    real_Num DronePropeller::getMaxThrust() const
    {
        return m_maxThrust;
    }

    void DronePropeller::setMaxThrust( real_Num maxThrust )
    {
        m_maxThrust = maxThrust;
    }

    s32 DronePropeller::getIndex() const
    {
        return m_index;
    }

    void DronePropeller::setIndex( s32 index )
    {
        m_index = index;
    }

    real_Num DronePropeller::getVoMultiplier() const
    {
        return m_voMultiplier;
    }

    void DronePropeller::setVoMultiplier( real_Num voMultiplier )
    {
        m_voMultiplier = voMultiplier;
    }

    real_Num DronePropeller::getGroundEffectMultiplier() const
    {
        return m_groundEffectMultiplier;
    }

    void DronePropeller::setGroundEffectMultiplier( real_Num groundEffectMultiplier )
    {
        m_groundEffectMultiplier = groundEffectMultiplier;
    }

    real_Num DronePropeller::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void DronePropeller::setThrustMultiplier( real_Num thrustMultiplier )
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    bool DronePropeller::getUseDebugPos() const
    {
        return m_bUseDebugPos;
    }

    void DronePropeller::setUseDebugPos( bool useDebugPos )
    {
        m_bUseDebugPos = useDebugPos;
    }

    real_Num DronePropeller::getThrust() const
    {
        return m_thrust;
    }

    void DronePropeller::setThrust( real_Num thrust )
    {
        m_thrust = thrust;
    }

    void DronePropeller::init()
    {
    }

    void DronePropeller::setupUserData()
    {
    }
}  // namespace workphone::vehicle
