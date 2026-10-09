#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/DroneProp.hpp"

namespace workphone::vehicle
{
    DroneProp::DroneProp()
    {
    }

    DroneProp::~DroneProp()
    {
    }

    void DroneProp::initialise(const std::string &objectID)
    {
    }

    void DroneProp::calcThrust(const double &dt)
    {
    }

    void DroneProp::calcGroundEffect(const Vector3<real_Num> &position)
    {
    }

    void DroneProp::input(real_Num rpm, const double &dt)
    {
    }

    Vector3<real_Num> DroneProp::getLocalPos() const
    {
        return Vector3<real_Num>(m_localPos.X(), m_localPos.Y(), m_localPos.Z());
    }

    void DroneProp::setLocalPos(const Vector3<real_Num> &localPos)
    {
    }

    int DroneProp::getDebugId() const
    {
        return m_debugId;
    }

    void DroneProp::setDebugId(int debugId)
    {
        m_debugId = debugId;
    }

    real_Num DroneProp::getDiameter() const
    {
        return m_diameter;
    }

    void DroneProp::setDiameter(real_Num diameter)
    {
        m_diameter = diameter;
    }

    real_Num DroneProp::getPitch() const
    {
        return m_pitch;
    }

    void DroneProp::setPitch(real_Num pitch)
    {
        m_pitch = pitch;
    }

    real_Num DroneProp::getMaxThrust() const
    {
        return m_maxThrust;
    }

    void DroneProp::setMaxThrust(real_Num maxThrust)
    {
        m_maxThrust = maxThrust;
    }

    s32 DroneProp::getIndex() const
    {
        return m_index;
    }

    void DroneProp::setIndex(s32 index)
    {
        m_index = index;
    }

    real_Num DroneProp::getVoMultiplier() const
    {
        return m_voMultiplier;
    }

    void DroneProp::setVoMultiplier(real_Num voMultiplier)
    {
        m_voMultiplier = voMultiplier;
    }

    real_Num DroneProp::getGroundEffectMultiplier() const
    {
        return m_groundEffectMultiplier;
    }

    void DroneProp::setGroundEffectMultiplier(real_Num groundEffectMultiplier)
    {
        m_groundEffectMultiplier = groundEffectMultiplier;
    }

    real_Num DroneProp::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void DroneProp::setThrustMultiplier(real_Num thrustMultiplier)
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    bool DroneProp::getUseDebugPos() const
    {
        return m_bUseDebugPos;
    }

    void DroneProp::setUseDebugPos(bool useDebugPos)
    {
        m_bUseDebugPos = useDebugPos;
    }

    real_Num DroneProp::getThrust() const
    {
        return m_thrust;
    }

    void DroneProp::setThrust(real_Num thrust)
    {
        m_thrust = thrust;
    }

    void DroneProp::init()
    {
    }

    void DroneProp::setupUserData()
    {
    }
}
