#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/HeliVars.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    const int ABool = 0;
    const int AInt = 1;
    const int ASingle = 2;
    const int AVec = 3;

    const int GET_TX_CHANNEL = 1;
    const int GET_ANGULAR_VELOCITY = 2;
    const int GET_LINEAR_VELOCITY = 3;
    const int ADD_LOCAL_FORCE = 4;
    const int ADD_LOCAL_TORQUE = 5;
    const int DISPLAY_LOCAL_VECTOR = 6;
    const int SET_MASS_PROPS = 7;
    const int GET_GLOBAL_POSITION = 8;
    const int GET_GLOBAL_ORIENTATION = 9;
    const int CAST_LOCAL_RAY = 10;
    const int GET_CONTROL_ANGLES = 11;
    const int GET_GYRO_OUTPUT = 12;
    const int GET_GOVERNOR_OUTPUT = 13;
    const int GET_ENGINE_OUTPUT = 14;
    const int SET_SERVO_INPUT = 15;
    const int RESET_GOVERNOR = 16;
    const int GET_HELI_CTRL_INFO = 17;
    const int FLYBAR_CTRL_UPDATE = 18;
    const int GET_TX_CHANNELS = 19;
    const int RESET_GYRO = 20;

    const int THR_CHANNEL = 0;
    const int AIL_CHANNEL = 1;
    const int ELE_CHANNEL = 2;
    const int YAW_CHANNEL = 3;
    const int GEAR_CHANNEL = 4;
    const int GYRO_GAIN_CHANNEL = 4;
    const int COL_CHANNEL = 5;
    const int AUX1_CHANNEL = 6;
    const int GOV_RPM_CHANNEL = 6;
    const int AUX2_CHANNEL = 7;
    const int CB_MODEL = 0;
    const int CB_ROTOR_HEAD = 1;

    void InitHeliVars()
    {
    }

    const double RPMToOmega = Math<double>::pi() / 30.0;
    const double OmegaToRPM = 30.0 / Math<double>::pi();
    // const int Locked = 2;
    // const int Overrun = 3;

    VehicleParam::VehicleParam()
    {
        m_bVal = false;
        m_iVal = 0;
        m_sVal = 0;
        m_vVal = physics_Vec::zero();
    }

    String VehicleParam::toString() const
    {
        return this->m_name + " " + StringUtil::toString(m_bVal) + " " +
               StringUtil::toString(m_iVal) + " " + StringUtil::toString(m_sVal) + " " +
               StringUtil::toString(m_vVal.x) + ", " + StringUtil::toString(m_vVal.y) + ", " +
               StringUtil::toString(m_vVal.z);
    }

    ClimbTransitionData::ClimbTransitionData()
    {
        m_rate = static_cast<physics_Num>(0.0);
        m_factor = static_cast<physics_Num>(0.0);
    }
}
