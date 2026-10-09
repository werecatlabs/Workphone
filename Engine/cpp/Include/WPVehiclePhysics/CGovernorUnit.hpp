#ifndef GovUnit1H
#define GovUnit1H

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include "WPVehiclePhysics/HeliVars.hpp"

namespace workphone::vehicle
{
    class WPVehiclePhysics_API CGovernorUnit
    {
    public:
        CGovernorUnit();
        CGovernorUnit(const CGovernorUnit &other) = delete;

        int m_mode;
        // if 0 the governor is off, if 1 the gov is using fixed rpm if 2 it uses channel 7 for RPM

        physics_Num m_fixedRpm; // holds the required RPM when mode = 1 (fixed)
        physics_Num m_rpmRangeBottom;
        // when mode = 2 (Channel 7 sets RPM) this is the engine speed for channel = -1;
        physics_Num m_rpmRangeTop;
        // when mode = 2 (Channel 7 sets RPM) this is the engine speed for channel = +1;
        physics_Num m_input; // This is the throttle signal from the Tx scaled 0 to 1
        physics_Num m_remoteSig; // the remote rpm control signal
        physics_Num m_reqRpm; // holds the required engine speed in RPM
        physics_Num m_lastRpm; // holds the last RPM value for acceleration calculation
        physics_Num
        m_targetRpm; // the target speed in RPM (a slugged version of the input required RPM
        physics_Num m_rampRate; // holds the engagement RPM ramp rate in RPM/s
        physics_Num m_acceleration; // holds a smoothed acceleration rate
        physics_Num m_accTimeConstant; // the time constant for the acceleration calculation
        physics_Num m_rpmError; // the speed error in engine radians/s Positive = engine fast
        physics_Num m_phaseError;
        // the phase angle by which the engine leads the reference (i.e. positive is engine fast)
        physics_Num m_accGain;
        physics_Num m_rpmGain; // gain for the Omega term
        physics_Num m_phaseGain; // gain for Phase term
        physics_Num m_accLimit;
        physics_Num m_rpmErrorLimit; // the limit for RPMError
        physics_Num m_phaseErrorLimit; // the limit for the PhaseError
        physics_Num m_output; // the output to the throttle (via a servo if needed)
        physics_Num m_maxControlPoint; // the limit to the throttle open swing (usually set to 1)
        physics_Num
        m_minControlPoint; // minimum throttle position when governor active (usually 0.15)
        physics_Num m_resetLimiter; // used to force a slow ramp up on the input to gov after a reset

        bool m_positiveGrowthEnable; // flag for anti integral wind-up control
        bool m_negativeGrowthEnable; // flag for anti integral wind-up control
        bool m_active; // set if governor armed by throttle opening
    }; // TGovernor
}

#endif //  GovUnit1H
