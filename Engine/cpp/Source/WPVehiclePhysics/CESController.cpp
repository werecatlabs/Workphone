#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CESController.hpp>
#include <WPVehiclePhysics/CBatteryPackStandard.hpp>
#include <WPVehiclePhysics/CAircraftMotor.hpp>
#include "WPVehiclePhysics/CAircraft.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    CESController::CESController() :
        m_mode(0),
        m_fixedRpm(0),
        m_rpmRangeBottom(0),
        m_rpmRangeTop(0),
        m_input(0),
        m_remoteSig(0),
        m_reqRpm(0),
        m_lastRpm(0),

        m_targetRpm(0),
        m_slowRampTc(1),
        m_fastRampTc(0),
        m_softStartDelay(0),
        m_softStartTimer(0),
        m_rampRate(0),
        m_acceleration(0),
        m_accTc(0),
        m_rpmError(0),
        m_phaseError(0),
        m_accGain(0),
        m_rpmGain(0),
        m_phaseGain(0),
        m_accLimit(0),
        m_rpmErrorLimit(0),
        m_phaseErrorLimit(0),
        m_output(0),

        m_maxControlPoint(0),
        m_minControlPoint(0),
        m_iLimit(static_cast<real_Num>(1e10)),

        m_cutoffV(0),
        m_cutoffTimer(0),
        m_govRpm(0),

        m_resistance(0.00001),
        m_resetLimiter(0),
        m_motorReversed(false),

        m_active(false),
        m_lastActive(false),
        m_softStart(true),

        m_positiveGrowthEnable(false),
        m_negativeGrowthEnable(false),
        m_cutoffActive(false)
    {
        m_braking = false;
        m_remoteSig = 0;
        m_active = false;
        m_lastActive = false;
        m_lastRpm = 0;
        m_targetRpm = 0;
        m_softStart = true; // set when the unit is set up to do a soft start
        m_rampRate = m_rpmRangeTop / m_slowRampTc;
        // holds the engagement RPM ramp rate in RPM/s  note reduced by factor of 2 for initial ramp
        m_acceleration = 0; // holds a smoothed acceleration rate
        m_accTc = 0.1; // set the filter TC for the acceleration measurements to 0.1 seconds
        m_rpmError = 0; // the speed error in engine radians/s Positive = engine fast
        m_phaseError = 0;
        // the phase angle by which the engine leads the reference (i.e. positive is engine fast)
        m_output = 0; // the output duty cycle of the controller
        m_maxControlPoint = 1; // the limit to the throttle open swing (usually set to 1)
        m_positiveGrowthEnable = true; // flag for anti integral wind-up control
        m_negativeGrowthEnable = true; // flag for anti integral wind-up control
        m_cutoffActive = false; // set if the voltage cutout in action
        m_resetLimiter = 0.7;

        m_mode = 2;
    }

    CESController::~CESController()
    {
    }

    /*
                CESController::CESController() :
            Mode( 0 ),
            MotorReversed( false ),
            Active( false ),
            LastActive( false ),
            SoftStart( true ),
            PositiveGrowthEnable( false ),
            NegativeGrowthEnable( false ),
            CutoffActive( false ),

            FixedRPM( 0 ),
            RPMRangeBottom( 0 ),
            RPMRangeTop( 0 ),
            Input( 0 ),
            RemoteSig( 0 ),
            ReqRPM( 0 ),
            LastRPM( 0 ),
            TargetRPM( 0 ),
            SlowRampTC( 0 ),
            FastRampTC( 0 ),
            SoftStartDelay( 0 ),
            SoftStartTimer( 0 ),
            RampRate( 0 ),
            Acceleration( 0 ),
            AccTC( 0 ),
            RPMError( 0 ),
            PhaseError( 0 ),

            AccGain( 0 ),
            RPMGain( 0 ),
            PhaseGain( 0 ),

            AccLimit( 0 ),
            RPMErrorLimit( 0 ),
            PhaseErrorLimit( 0 ),

            Output( 0 ),
            MaxControlPoint( 0 ),
            MinControlPoint( 0 ),

            ILimit( 0 ),
            CutoffV( 0 ),
            CutoffTimer( 0 ),

            GovRPM( 0 ),
            Resistance( 0 ),
            ResetLimiter( 0 )
        {
            MotorReversed = true;  //hack for testing
        }

        CESController::~CESController()
        {
        }
        */

    void CESController::load(SmartPtr<ISharedObject> sharedObject)
    {
        auto properties = workphone::dynamic_pointer_cast<Properties>(sharedObject);
        if(properties)
        {
            properties->getPropertyValue("FlEqGovReqHeadRPM", m_fixedRpm);
            properties->getPropertyValue("FlEqGovReqHeadRPM", m_reqRpm);
            properties->getPropertyValue("FlEqGovMode", m_mode);
            properties->getPropertyValue("FlEqESCSlowRampTC", m_slowRampTc);
            properties->getPropertyValue("FlEqESCAccelerationGain", m_accGain);
            properties->getPropertyValue("FlEqESCRPMGain", m_rpmGain);
            properties->getPropertyValue("FlEqESCPhaseGain", m_phaseGain);
            properties->getPropertyValue("FlEqESCRPMErrorLimit", m_rpmErrorLimit);
            properties->getPropertyValue("FlEqESCAccelerationLimit", m_accLimit);
            properties->getPropertyValue("FlEqESCPhaseErrorLimit", m_phaseErrorLimit);
            properties->getPropertyValue("FlEqESCMinControlPoint", m_minControlPoint);
            properties->getPropertyValue("FlEqESCILimit", m_iLimit);
            properties->getPropertyValue("FlEqESCCutoffV", m_cutoffV);
            properties->getPropertyValue("FlEqESCFastRampTC", m_fastRampTc);
            properties->getPropertyValue("FlEqSoftStartDelay", m_softStartDelay);
            properties->getPropertyValue("FlEqESCBraking", m_braking);
        }
    }

    void CESController::update(const double &currentTime, const double &deltaTime)
    {
        auto pThis = getSharedFromThis<IESController>();
        auto esc = pThis;
        auto motor = m_motor;

        m_input = m_parentAircraft->getChannel(CAircraft::m_thrChannel);
        m_input = (static_cast<real_Num>(0.8) + m_input) /
                  (static_cast<real_Num>(0.8) * static_cast<real_Num>(2.0));
        m_input = static_cast<real_Num>(1.0) - m_input;

        auto rpm = static_cast<real_Num>(0.0);

        if(motor)
        {
            rpm = motor->getRPM();
        }

        escDoGovernor(m_input, deltaTime, rpm, m_batteryPack, esc);
    }

    void CESController::rampControl(float deltaTime, SmartPtr<IESController> &escHandle)
    {
        SmartPtr<CESController> esc = workphone::static_pointer_cast<CESController>(escHandle);
        CESController &controller = *esc;

        /*with ESC do*/
        {
            if(controller.m_active == false) // if the governor is inactive
            {
                if(controller.m_lastActive == true) // but was active in the previous frame
                {
                    // gov has just gone inactive
                    controller.m_lastActive = false; // save the avtivity state
                    controller.m_softStart = false; // flag the unit is in fast start mode
                    controller.m_rampRate = controller.m_rpmRangeTop / controller.m_fastRampTc;
                    // set the ramp to fast
                    controller.m_softStartTimer = controller.m_softStartDelay;
                    // set the timer to the delay value in seconds
                } // end of gov just gone inactive
                else // gov was is already inactive
                {
                    // gov staying inactive
                    controller.m_softStartTimer = controller.m_softStartTimer - deltaTime;
                    // knock off this dt from the timer
                    if(controller.m_softStartTimer < 0)
                    {
                        // soft start delay has run to its end
                        controller.m_softStart = true; // flag the unit is in soft start mode
                        controller.m_rampRate = controller.m_rpmRangeTop / controller.m_slowRampTc;
                        // set the ramp to slow
                    } // end of soft start timer run to end
                } // end of gov staying inactive
            } // end of Active = False
            else
            {
                if(controller.m_lastActive == false)
                {
                    controller.m_lastActive = true;
                }
            } // end of active = true
        } // end of with ESC
    }

    SmartPtr<IVehiclePowerUnit> CESController::getMotor() const
    {
        return m_motor;
    }

    void CESController::escDoGovernor(real_Num inputSignal, real_Num deltaTime, real_Num currentRpm,
                                      SmartPtr<IBatteryPack> &batteryPack,
                                      SmartPtr<IESController> &escHandle)
    {
        SmartPtr<CBatteryPackStandard> batteryPackPtr =
            workphone::static_pointer_cast<CBatteryPackStandard>(batteryPack);
        CBatteryPackStandard &pack = *batteryPackPtr;

        SmartPtr<CESController> esc = workphone::static_pointer_cast<CESController>(escHandle);
        CESController &controller = *esc;

        real_Num dTarg; // use in ramping the target speed
        real_Num OTarg; // holds the target output
        real_Num DeltaO; // holds the unlimited change in output
        real_Num ThisAcc; // holds the new acceleration value
        real_Num K; // temp for the accel timeconst factor
        real_Num DeltaPhase; // holds the change in phase error

        /*with ESC do*/
        {
            controller.m_resetLimiter = controller.m_resetLimiter + static_cast<real_Num>(0.2) *
                                        deltaTime;
            // update the ResetLimiter
            if(controller.m_resetLimiter > static_cast<real_Num>(1.0))
                controller.m_resetLimiter = static_cast<real_Num>(1.0); // limit the ResetLimiter to 1

            controller.m_input = static_cast<real_Num>(1.11) * (
                                     inputSignal - static_cast<real_Num>(0.1));
            // adjust Incoming throttle signal to allow some stick before motor starts
            if(controller.m_input < 0)
                controller.m_input = 0; // limit the range of the input
            if(controller.m_input > controller.m_resetLimiter)
                controller.m_input = controller.m_resetLimiter;
            // apply the Reset limit (<1 for the first 5 seconds after reset)

            switch(controller.m_mode)
            {
            case // based on the governor mode set the ReqRPM value
            0:
                controller.m_reqRpm = 0;
                break;
            case 1:
                controller.m_reqRpm = controller.m_fixedRpm;
                break;
            case 2:
            {
                controller.m_remoteSig =
                    controller.m_input - static_cast<real_Num>(0.25) * static_cast<real_Num>(1.333);
                // derive a 'speed signal' from the throttle
                if(controller.m_remoteSig > 1)
                    controller.m_remoteSig = 1; // bound the remote signal
                if(controller.m_remoteSig < 0)
                    controller.m_remoteSig = 0;
                controller.m_reqRpm = controller.m_rpmRangeBottom +
                                      controller.m_remoteSig * (
                                          controller.m_rpmRangeTop - controller.m_rpmRangeBottom);
            }
            break;
            } // case

            rampControl(deltaTime, escHandle);
            // look after the soft start condition based on activity state and time

            if(controller.m_input < static_cast<real_Num>(0.08))
            {
                controller.m_active = false;
                // deactivate the governor if input below 8% throttle now in StartRampControl
            }

            if(Math<real_Num>::equals(controller.m_reqRpm,
                                      0.0)) // if Governor not in use
            {
                controller.m_active = false; // disable Governor if the required RPM is set to zero!
                controller.m_output = controller.m_input;
                return;
            }

            if(controller.m_active == false)
            {
                //
                if((controller.m_input > static_cast<real_Num>(0.12)) &&
                   (currentRpm >
                    static_cast<real_Num>(0.1) * controller.m_reqRpm)) // test for activation conditions
                {
                    controller.m_targetRpm = currentRpm; // set the Target RPM to match the current RPM
                    controller.m_lastRpm = currentRpm;
                    // initialise the LastRPM so acceleration calc does not go nuts on activation
                    controller.m_rpmError = 0; //
                    controller.m_phaseError = 0; // zero the integral error
                    if(controller.m_targetRpm > controller.m_reqRpm)
                        controller.m_targetRpm = controller.m_reqRpm;
                    // but if the current RPM is > the required RPM then limit it
                    controller.m_active = true; // activate the governor
                } // end of activation
                else // i.e unit not active
                {
                    controller.m_output =
                        controller.m_input; // if inactive simply pass throttle signal through unchanged
                }
            } // end of active = false
            else // i.e the unit is active
            {
                ThisAcc = (currentRpm - controller.m_lastRpm) / deltaTime;
                // calculate the acceleration in this time step
                controller.m_lastRpm = currentRpm; // update last RPM value
                K = deltaTime / controller.m_accTc; // calculate the factor for the accel filter
                controller.m_acceleration = (1 - K) * controller.m_acceleration + K * ThisAcc;
                // update the acceleration value
                Math<real_Num>::Limit(controller.m_acceleration, controller.m_accLimit);
                // apply the limit to the acceleration value
                dTarg = controller.m_reqRpm - controller.m_targetRpm;
                // find if the target needs ramping up or down
                Math<real_Num>::Limit(dTarg,
                                      controller.m_rampRate * deltaTime);
                // apply limit to dTarg of 4000 RPM/sec
                controller.m_targetRpm = controller.m_targetRpm + dTarg;
                // ramp the Target omega towards the current Target received
                controller.m_rpmError = currentRpm - controller.m_targetRpm;
                // get the speed error in rad./s
                Math<real_Num>::Limit(controller.m_rpmError,
                                      controller.m_rpmErrorLimit); // apply a limit to this value
                DeltaPhase = deltaTime * controller.m_rpmError;
                // calc the shift in phase error in RPM seconds (which is a unit equal to 6 degrees!)

                // now test if the phase error is to be permitted to be added in (based on the servo
                // already being on the given limit
                if(((DeltaPhase < 0) && controller.m_positiveGrowthEnable) ||
                   ((DeltaPhase > 0) && controller.m_negativeGrowthEnable))
                    controller.m_phaseError = controller.m_phaseError + DeltaPhase;
                Math<real_Num>::Limit(controller.m_phaseError, controller.m_phaseErrorLimit);
                OTarg = static_cast<real_Num>(0.5) -
                        (controller.m_accGain * controller.m_acceleration + controller.m_rpmGain *
                         controller.m_rpmError +
                         controller.m_phaseGain * controller.m_phaseError);
                // calc the appropriate governor output (centring it on half throttle)
                DeltaO = OTarg - controller.m_output; // apply a slew limit to the output
                Math<real_Num>::Limit(DeltaO, static_cast<real_Num>(10.0) * deltaTime);
                // limit it so that slew takes about 0.1s
                controller.m_output = controller.m_output + DeltaO; // add the Delta

                if(controller.m_output > controller.m_maxControlPoint)
                {
                    // if output on the full-throttle limit
                    controller.m_output = controller.m_maxControlPoint;
                    // apply a limit to the output value
                    controller.m_positiveGrowthEnable = false;
                    // turn off further positive integration of the phase error (to limit integral
                    // term wind-up)
                }
                else // i.e. if output not on the full-throttle stop
                {
                    controller.m_positiveGrowthEnable = true; // enable positive integral growth
                }

                if(controller.m_output <
                   controller.m_minControlPoint) // now test if throttle at the minimum control poin
                {
                    controller.m_output = controller.m_minControlPoint;
                    // set output to minimum throttle point for the active governor
                    controller.m_negativeGrowthEnable = false;
                    // prevent further negative growth of the integral term
                }
                else // i.e the throttle not at the minimum control point
                {
                    controller.m_negativeGrowthEnable = true; // allow negative integral term growth
                }
            } // end of governor active
        } // With ESC
    }

    void CESController::escCutoutControl(real_Num deltaTime, SmartPtr<IBatteryPack> &batteryPack)
    {
        SmartPtr<CBatteryPackStandard> pPack =
            workphone::static_pointer_cast<CBatteryPackStandard>(batteryPack);
        CBatteryPackStandard &pack = *pPack;

        if(pack.getVoltage() < static_cast<real_Num>(0.93) * m_cutoffV * pack.getNumCells())
        {
            // if pack less than 93% of the cutoff voltage then hard cut
            m_cutoffActive = true;
        }
        else
        {
            if(pack.getVoltage() < m_cutoffV * pack.getNumCells())
            {
                m_cutoffTimer = m_cutoffTimer + deltaTime;
                if(Math<real_Num>::equals(
                    Math<real_Num>::Mod(Math<real_Num>::trunc(m_cutoffTimer), 5),
                    0.0))
                    m_cutoffActive = true;
                else
                    m_cutoffActive = false; // if below cutoff but  >93% cutoff pulse power on and off
            }
            else
            {
                m_cutoffTimer = 0;
            }
        }
    }

    void CESController::rampControl(real_Num deltaTime)
    {
        CESController &controller = *this;

        /*with ESC do*/
        {
            if(controller.m_active == false) // if the governor is inactive
            {
                if(controller.m_lastActive == true) // but was active in the previous frame
                {
                    // gov has just gone inactive
                    controller.m_lastActive = false; // save the avtivity state
                    controller.m_softStart = false; // flag the unit is in fast start mode
                    controller.m_rampRate = controller.m_rpmRangeTop / controller.m_fastRampTc;
                    // set the ramp to fast
                    controller.m_softStartTimer = controller.m_softStartDelay;
                    // set the timer to the delay value in seconds
                } // end of gov just gone inactive
                else // gov was is already inactive
                {
                    // gov staying inactive
                    controller.m_softStartTimer = controller.m_softStartTimer - deltaTime;
                    // knock off this dt from the timer
                    if(controller.m_softStartTimer < 0)
                    {
                        // soft start delay has run to its end
                        controller.m_softStart = true; // flag the unit is in soft start mode
                        controller.m_rampRate = controller.m_rpmRangeTop / controller.m_slowRampTc;
                        // set the ramp to slow
                    } // end of soft start timer run to end
                } // end of gov staying inactive
            } // end of Active = False
            else
            {
                if(controller.m_lastActive == false)
                {
                    controller.m_lastActive = true;
                }
            } // end of active = true
        } // end of with ESC
    }

    SmartPtr<IVehicle> CESController::getOwner() const
    {
        return nullptr;
    }

    void CESController::setOwner(SmartPtr<IVehicle> owner)
    {
    }

    SmartPtr<IBatteryPack> CESController::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void CESController::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    void CESController::setMotor(SmartPtr<IVehiclePowerUnit> motor)
    {
        m_motor = motor;
    }
}
