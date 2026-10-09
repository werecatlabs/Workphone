#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CGyroUnit.hpp"
#include "WPVehiclePhysics/HeliAero.hpp"

namespace workphone::vehicle
{
    // #define GyroUnit1__0 (System::Set< int, 0, 255 >() << int ( 1 ) << int ( 2 ) << int ( 3 ) <<
    // int ( 4 ) << int ( 5 ) << int ( 6 ) << int ( 7 ) << int ( 8 ) << int ( 9 ) << int ( 10 ) <<
    // int ( 11 ) << int ( 12 ) << int ( 13 ) << int ( 14 ) << int ( 15 ) << int ( 16 ) << int ( 17 )
    // << int ( 18 ) << int ( 19 ) << int ( 20 ) << int ( 21 ) << int ( 22 ) << int ( 23 ) << int (
    // 24 ) << int ( 25 ) << int ( 26 ) << int ( 27 ) << int ( 28 ) << int ( 29 ) << int ( 30 ) <<
    // int ( 31 ) << int ( 32 ) << int ( 33 ) << int ( 34 ) << int ( 35 ) << int ( 36 ) << int ( 37 )
    // << int ( 38 ) << int ( 39 ) << int ( 40 ) << int ( 41 ) << int ( 42 ) << int ( 43 ) << int (
    // 44 ) << int ( 45 ) << int ( 46 ) << int ( 47 ) << int ( 48 ) << int ( 49 ) << int ( 50 ) <<
    // int ( 51 ) << int ( 52 ) << int ( 53 ) << int ( 54 ) << int ( 55 ) << int ( 56 ) << int ( 57 )
    // << int ( 58 ) << int ( 59 ) << int ( 60 ) << int ( 61 ) << int ( 62 ) << int ( 63 ) << int (
    // 64 ) << int ( 65 ) << int ( 66 ) << int ( 67 ) << int ( 68 ) << int ( 69 ) << int ( 70 ) <<
    // int ( 71 ) << int ( 72 ) << int ( 73 ) << int ( 74 ) << int ( 75 ) << int ( 76 ) << int ( 77 )
    // << int ( 78 ) << int ( 79 ) << int ( 80 ) << int ( 81 ) << int ( 82 ) << int ( 83 ) << int (
    // 84 ) << int ( 85 ) << int ( 86 ) << int ( 87 ) << int ( 88 ) << int ( 89 ) << int ( 90 ) <<
    // int ( 91 ) << int ( 92 ) << int ( 93 ) << int ( 94 ) << int ( 95 ) << int ( 96 ) << int ( 97 )
    // << int ( 98 ) << int ( 99 ) << int ( 100 ) )

    CGyroUnit::CGyroUnit()
    {
        m_input = static_cast<physics_Num>(0.0);
        m_output = static_cast<physics_Num>(0.0);
        m_stickDeadBand = static_cast<physics_Num>(0.0);
        m_stickSensitivity = static_cast<physics_Num>(0.0);
        m_stickExpo = static_cast<physics_Num>(0.0);
        m_directGain = static_cast<physics_Num>(0.0);
        m_yawDemand = static_cast<physics_Num>(0.0);
        m_yawRate = static_cast<physics_Num>(0.0);
        m_yawError = static_cast<physics_Num>(0.0);
        m_yawErrorLimit = static_cast<physics_Num>(0.0);
        m_yawErrorGain = static_cast<physics_Num>(0.0);
        m_hlError = static_cast<physics_Num>(0.0);
        m_hlRange = static_cast<physics_Num>(0.0);
        m_hlLimit = static_cast<physics_Num>(0.0);
        m_hlGain = static_cast<physics_Num>(0.0);
        m_throwLimit1 = static_cast<physics_Num>(0.0);
        m_throwLimit2 = static_cast<physics_Num>(0.0);
        m_gain = static_cast<physics_Num>(0.0);
        m_servoOffset = static_cast<physics_Num>(0.0);
        m_hlOffTimer = static_cast<physics_Num>(0.0);
        m_hlKillTime = static_cast<physics_Num>(0.0);
        m_hlDecay = static_cast<physics_Num>(0.0);
        m_acceleration = static_cast<physics_Num>(0.0);
        m_lastYawRate = static_cast<physics_Num>(0.0);
        m_deltaT = static_cast<physics_Num>(0.0);
        m_accTC = static_cast<physics_Num>(0.0);
        m_accGain = static_cast<physics_Num>(0.0);
        m_accTerm = static_cast<physics_Num>(0.0);
        m_accTermLimit = static_cast<physics_Num>(0.0);
        m_leftStopGain = static_cast<physics_Num>(0.0);
        m_rightStopGain = static_cast<physics_Num>(0.0);
        m_currentStopGain = static_cast<physics_Num>(0.0);
        m_scArmDemand = static_cast<physics_Num>(0.0);
        m_scArmRate = static_cast<physics_Num>(0.0);
        m_scTrigDemand = static_cast<physics_Num>(0.0);
        m_scTrigRateError = static_cast<physics_Num>(0.0);
        m_scDisarmDemand = static_cast<physics_Num>(0.0);
        m_scDisarmRateError = static_cast<physics_Num>(0.0);
        m_scStopDoneYaw = static_cast<physics_Num>(0.0);
        m_scStopAbortDemand = static_cast<physics_Num>(0.0);

        m_scMode = 0;
        m_hlMode = false;
        m_hlOn = false;
        m_senseReverse = false;
    }
}
