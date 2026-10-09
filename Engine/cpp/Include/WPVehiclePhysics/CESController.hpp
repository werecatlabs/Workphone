#ifndef CESController_h__
#define CESController_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Vehicle/IESController.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>

namespace workphone::vehicle
{
    class WPVehiclePhysics_API CESController : public CAircraftAttachment<IESController>
    {
    public:
        CESController();
        ~CESController() override;

        void load(SmartPtr<ISharedObject> sharedObject) override;

        void update(const double &currentTime, const double &deltaTime) override;

        SmartPtr<IVehicle> getOwner() const override;
        void setOwner(SmartPtr<IVehicle> owner) override;

        SmartPtr<IBatteryPack> getBatteryPack() const override;
        void setBatteryPack(SmartPtr<IBatteryPack> batteryPack) override;

        SmartPtr<IESController> getEsc() const override
        {
            return m_esc;
        }

        void setEsc(SmartPtr<IESController> esc) override
        {
            m_esc = esc;
        }

        SmartPtr<IVehiclePowerUnit> getMotor() const override;
        void setMotor(SmartPtr<IVehiclePowerUnit> motor) override;

        void escDoGovernor(real_Num inputSignal, real_Num deltaTime, real_Num currentRpm,
                           SmartPtr<IBatteryPack> &batteryPack, SmartPtr<IESController> &esc);
        // end of governor

        void escCutoutControl(real_Num deltaTime, SmartPtr<IBatteryPack> &batteryPack);

        void rampControl(float deltaTime, SmartPtr<IESController> &esc);
        void rampControl(real_Num deltaTime);

        s32 m_mode;
        // if 0 the governor is off, if 1 the gov is using fixed rpm if 2 it uses channel 7 for RPM
        real_Num m_fixedRpm; // holds the required RPM when mode = 1 (fixed)
        real_Num m_rpmRangeBottom;
        // when mode = 2 (Channel 7 sets RPM) this is the engine speed for channel = -1;
        real_Num m_rpmRangeTop;
        // when mode = 2 (Channel 7 sets RPM) this is the engine speed for channel = +1;
        real_Num m_input; // This is the throttle signal from the Tx scaled 0 to 1
        real_Num m_remoteSig;
        // used to set the governor RPM but is not actually a separate channel but derived from the
        // throttle sig
        real_Num m_reqRpm; // holds the required engine speed in RPM
        real_Num m_lastRpm; // holds the last RPM value for acceleration calculation
        real_Num
        m_targetRpm; // the target speed in RPM (a slugged version of the input required RPM
        real_Num m_slowRampTc; // the time to ramp from zero to RPMRangeTop in ground spool-up mode
        real_Num
        m_fastRampTc; // the time to ramp from zero to RPMRangeTop in autorotation abort mode;
        real_Num m_softStartDelay; // delay in seconds before ESC reverts to the soft start mode.
        real_Num m_softStartTimer;
        // used to sum the interval between throttle hold and governor re engagement
        real_Num m_rampRate; // holds the engagement RPM ramp rate in RPM/s
        real_Num m_acceleration; // holds a smoothed acceleration rate
        real_Num m_accTc; // the time constant for the acceleration calculation
        real_Num m_rpmError; // the speed error in engine radians/s Positive = engine fast
        real_Num m_phaseError;
        // the phase angle by which the engine leads the reference (i.e. positive is engine fast)
        real_Num m_accGain;
        real_Num m_rpmGain; // gain for the Omega term
        real_Num m_phaseGain; // gain for Phase term
        real_Num m_accLimit;
        real_Num m_rpmErrorLimit; // the limit for RPMError
        real_Num m_phaseErrorLimit; // the limit for the PhaseError
        real_Num m_output; // the output duty cycle of the controller
        real_Num m_maxControlPoint; // the limit to the throttle open swing (usually set to 1)
        real_Num m_minControlPoint; // minimum throttle position when governor active (uually 0.15)
        real_Num m_iLimit; // current limit of ESC
        real_Num m_cutoffV; // the per-cell cut-off voltage
        real_Num m_cutoffTimer; // times the interval since battery entered cutoff range
        real_Num m_govRpm;
        real_Num m_resistance; // the electrical resistance of the ESC inc its wiring
        real_Num m_resetLimiter;
        // allows for a reset delay to stop snap restart if the idle up is left engaged
        bool m_braking;
        bool m_motorReversed; // To reverse the motor.
        bool m_active; // set if governor armed by throttle opening
        bool m_lastActive; // used to save previous 'frame's' Active status for Ramp Rate Control
        bool m_softStart; // set when the unit is set up to do a soft start
        bool m_positiveGrowthEnable; // flag for anti integral wind-up control
        bool m_negativeGrowthEnable; // flag for anti integral wind-up control
        bool m_cutoffActive; // set if the voltage cutout in action

        SmartPtr<IBatteryPack> m_batteryPack;
        SmartPtr<IESController> m_esc;
        SmartPtr<IVehiclePowerUnit> m_motor;
    };
}

#endif  // CESController_h__
