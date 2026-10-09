#include <vector>

#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/DroneMotor.hpp"
#include "WPVehiclePhysics/DroneProp.hpp"

namespace workphone::vehicle
{
    DroneMotor::DroneMotor()
    {
    }

    DroneMotor::~DroneMotor()
    {
    }

    void DroneMotor::initialise(const std::string &objectID)
    {
    }

    void DroneMotor::setupUserData()
    {
    }

    void DroneMotor::input(real_Num inputValue, const double &dt)
    {
    }

    void DroneMotor::enterFlightState()
    {
    }

    void DroneMotor::enterWorkbenchState()
    {
    }

    void DroneMotor::clearAverageSoundRPM()
    {
    }

    void DroneMotor::updateSound()
    {
    }

    real_Num DroneMotor::getMaxTorque() const
    {
        return m_maxTorque;
    }

    void DroneMotor::setMaxTorque(real_Num maxTorque)
    {
        m_maxTorque = maxTorque;
    }

    Vector3<real_Num> DroneMotor::getLocalPos() const
    {
        return m_localPos;
    }

    void DroneMotor::setLocalPos(const Vector3<real_Num> &localPos)
    {
        m_localPos = localPos;
    }

    f32 DroneMotor::getAverageMotorRPM() const
    {
        return m_averageMotorRPM;
    }

    void DroneMotor::setAverageMotorRPM(f32 averageMotorRPM)
    {
        m_averageMotorRPM = averageMotorRPM;
    }

    int DroneMotor::getDebugId() const
    {
        return m_debugId;
    }

    void DroneMotor::setDebugId(int debugId)
    {
        m_debugId = debugId;
    }

    Quaternion<real_Num> DroneMotor::getRotation() const
    {
        return m_rotation;
    }

    void DroneMotor::setRotation(const Quaternion<real_Num> &rotation)
    {
        m_rotation = rotation;
    }

    s32 DroneMotor::getIndex() const
    {
        return m_index;
    }

    void DroneMotor::setIndex(s32 index)
    {
        m_index = index;
    }

    std::string DroneMotor::getPropReference() const
    {
        return m_propReference;
    }

    void DroneMotor::setPropReference(const std::string &propReference)
    {
        m_propReference = propReference;
    }

    void DroneMotor::setProp(SmartPtr<DroneProp> prop)
    {
        m_prop = prop;
    }

    SmartPtr<DroneProp> DroneMotor::getProp() const
    {
        return m_prop;
    }
}
