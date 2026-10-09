#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/AerodymanicsUtil.hpp>
#include "WPVehiclePhysics/CESController.hpp"
#include "WPVehiclePhysics/CGyroUnit.hpp"
#include "WPVehiclePhysics/HeliAero.hpp"
#include "WPVehiclePhysics/CEMotor.hpp"
#include "WPVehiclePhysics/Rotor.hpp"
#include "WPVehiclePhysics/RotorHead.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    physics_Vec AerodymanicsUtil::bodyForce(const physics_Vec &SaracenFlow,
                                            const physics_Vec &BodyCdA)
    {
        physics_Vec result;
        physics_Vec FloSq;

        // Calculate signed square flow for each axis.
        // The sign is preserved to maintain flow direction while squaring magnitude.
        FloSq.x = SaracenFlow.x * Math<physics_Num>::Abs(SaracenFlow.x);
        FloSq.y = SaracenFlow.y * Math<physics_Num>::Abs(SaracenFlow.y);
        FloSq.z = SaracenFlow.z * Math<physics_Num>::Abs(SaracenFlow.z);

        // Calculate drag force: F = -CdA * v^2
        // Negative sign indicates drag opposes motion direction.
        result.x = -BodyCdA.x * FloSq.x;
        result.y = -BodyCdA.y * FloSq.y;
        result.z = -BodyCdA.z * FloSq.z;

        return result;
    }

    void AerodymanicsUtil::adjustGyroParam(int ParamID, float ParamVal)
    {
        auto &aero = HeliAero::getSingleton();
        auto &gyro = aero->m_tailGyro;

        // Scale factor converts parameter value to throw limit in appropriate units
        constexpr physics_Num ThrowLimitScale = 0.015;

        switch(ParamID)
        {
        case 1:
            gyro.m_throwLimit1 = ThrowLimitScale * ParamVal;
            break;
        case 2:
            gyro.m_throwLimit2 = ThrowLimitScale * ParamVal;
            break;
        }
    }

    void AerodymanicsUtil::resetGyro(CGyroUnit &Gyro)
    {
        // Reset runtime state variables
        Gyro.m_yawDemand = 0;
        Gyro.m_yawRate = 0;
        Gyro.m_yawError = 0;
        Gyro.m_hlError = 0;

        // Heading lock gain: Full-scale deflection at 15 degrees heading error
        Gyro.m_hlGain = static_cast<physics_Num>(1.0) /
                        (static_cast<physics_Num>(15.0) * Math<physics_Num>::pi() /
                         static_cast<physics_Num>(180.0));

        // Heading lock mode flags
        Gyro.m_hlMode = false; // Set when Mode/gain is configured for heading lock
        Gyro.m_hlOn = false; // Used for heading lock stop control

        // Default master gain (used when no channel is allocated)
        Gyro.m_gain = static_cast<physics_Num>(0.5);
        Gyro.m_hlOffTimer = 0;

        // Reset acceleration tracking
        Gyro.m_acceleration = 0;
        Gyro.m_currentStopGain = static_cast<physics_Num>(1.0);

        // Stop control state machine initialization
        // States: 0=disarmed, 1=armed positive yaw, 2=armed negative yaw, 3=triggered
        Gyro.m_scMode = 0;

        // Stop control thresholds (converted from degrees to radians)
        Gyro.m_scArmDemand = static_cast<physics_Num>(200.0) * Math<physics_Num>::pi() /
                             static_cast<physics_Num>(180.0);
        Gyro.m_scArmRate = static_cast<physics_Num>(180.0) * Math<physics_Num>::pi() /
                           static_cast<physics_Num>(180.0);
        Gyro.m_scTrigDemand = static_cast<physics_Num>(30.0) * Math<physics_Num>::pi() /
                              static_cast<physics_Num>(180.0);
        Gyro.m_scTrigRateError = static_cast<physics_Num>(100.0) * Math<physics_Num>::pi() /
                                 static_cast<physics_Num>(180.0);
        Gyro.m_scDisarmDemand = static_cast<physics_Num>(25.0) * Math<physics_Num>::pi() /
                                static_cast<physics_Num>(180.0);
        Gyro.m_scDisarmRateError = static_cast<physics_Num>(60.0) * Math<physics_Num>::pi() /
                                   static_cast<physics_Num>(180.0);
        Gyro.m_scStopDoneYaw = static_cast<physics_Num>(50.0) * Math<physics_Num>::pi() /
                               static_cast<physics_Num>(180.0);
        Gyro.m_scStopAbortDemand = static_cast<physics_Num>(50.0) * Math<physics_Num>::pi() /
                                   static_cast<physics_Num>(180.0);
    }

    void AerodymanicsUtil::stopControl(CGyroUnit &TG, physics_Num dt)
    {
        int LastMode = TG.m_scMode;

        if(TG.m_hlMode == true)
        {
            switch(TG.m_scMode)
            {
            case 0: // Disarmed state
            {
                TG.m_hlOn = true; // Ensure heading lock is enabled

                // Check conditions to arm for positive yaw direction
                if((TG.m_yawDemand > TG.m_scArmDemand) && (TG.m_yawRate > TG.m_scArmRate))
                {
                    TG.m_scMode = 1;
                }
                // Check conditions to arm for negative yaw direction
                if((TG.m_yawDemand < -TG.m_scArmDemand) && (TG.m_yawRate < -TG.m_scArmRate))
                {
                    TG.m_scMode = 2;
                }
            }
            break;

            case 1: // Armed with positive yaw
            {
                // Test for trigger condition: low demand + high rate error
                if((TG.m_yawDemand < TG.m_scTrigDemand) &&
                   (TG.m_yawError > TG.m_scTrigRateError))
                {
                    TG.m_scMode = 3; // Trigger stop control
                    TG.m_hlError = 0;
                    TG.m_hlOn = false;
                    TG.m_hlOffTimer = 0;
                }
                else
                {
                    // Test for disarm without trigger
                    if((TG.m_yawDemand < TG.m_scDisarmDemand) &&
                       (TG.m_yawError < TG.m_scDisarmRateError))
                    {
                        TG.m_scMode = 0;
                    }
                }
            }
            break;

            case 2: // Armed with negative yaw
            {
                // Test for trigger condition (using absolute values for negative direction)
                if((-TG.m_yawDemand < TG.m_scTrigDemand) &&
                   (-TG.m_yawError > TG.m_scTrigRateError))
                {
                    TG.m_scMode = 3; // Trigger stop control
                    TG.m_hlError = 0;
                    TG.m_hlOn = false;
                    TG.m_hlOffTimer = 0;
                }
                else
                {
                    // Test for disarm without trigger
                    if((-TG.m_yawDemand < TG.m_scDisarmDemand) &&
                       (-TG.m_yawError < TG.m_scDisarmRateError))
                    {
                        TG.m_scMode = 0;
                    }
                }
            }
            break;

            case 3: // Triggered - active stop control
            {
                TG.m_hlOffTimer = TG.m_hlOffTimer + dt;

                // End stop control if: timer expired, yaw rate low enough, or stick moved too far
                if((TG.m_hlOffTimer > TG.m_hlKillTime) ||
                   (abs(TG.m_yawRate) < TG.m_scStopDoneYaw) ||
                   (abs(TG.m_yawDemand) > TG.m_scStopAbortDemand))
                {
                    TG.m_hlOn = true;
                    TG.m_hlOffTimer = 0;
                    TG.m_scMode = 0;
                }
            }
            break;
            }
        }
        else
        {
            // Gyro not in heading lock mode - reset stop control state
            TG.m_scMode = 0;
            TG.m_hlOn = true;
            TG.m_hlError = 0; // Zero error to prevent glitch on entering HL mode
        }
    }

    void AerodymanicsUtil::calcAcceleration(CGyroUnit &TG)
    {
        physics_Num ThisAcc, K;

        // Calculate instantaneous acceleration
        ThisAcc = (TG.m_yawRate - TG.m_lastYawRate) / TG.m_deltaT;

        // Enforce minimum time constant to prevent instability
        if(TG.m_accTC < static_cast<physics_Num>(0.02))
            TG.m_accTC = static_cast<physics_Num>(0.02);

        // Calculate low-pass filter coefficient
        K = TG.m_deltaT / TG.m_accTC;
        if(K > static_cast<physics_Num>(0.3))
            K = static_cast<physics_Num>(0.3);

        // Apply exponential moving average filter
        TG.m_acceleration =
            TG.m_acceleration * (static_cast<physics_Num>(1.0) - K) + ThisAcc * K;
        TG.m_lastYawRate = TG.m_yawRate;

        // Calculate and limit acceleration contribution to servo output
        TG.m_accTerm = TG.m_accGain * TG.m_acceleration;
        if(TG.m_accTerm > TG.m_accTermLimit)
            TG.m_accTerm = TG.m_accTermLimit;
        if(TG.m_accTerm < -TG.m_accTermLimit)
            TG.m_accTerm = -TG.m_accTermLimit;
    }

    void AerodymanicsUtil::calcStopGain(CGyroUnit &TG)
    {
        // Apply directional stop gain when heading lock is disabled (during stop maneuver)
        if(TG.m_hlOn == false)
        {
            if(TG.m_yawRate > static_cast<physics_Num>(0))
                TG.m_currentStopGain = TG.m_leftStopGain;
            else
                TG.m_currentStopGain = TG.m_rightStopGain;
        }
        else
        {
            TG.m_currentStopGain = static_cast<physics_Num>(1.0);
        }
    }

    void AerodymanicsUtil::gyroIn(physics_Num InSig, physics_Num GainSig, physics_Num dt,
                                  CGyroUnit &ThisGyro)
    {
        physics_Num DecayFactor;

        ThisGyro.m_deltaT = dt;

        // Apply deadband to input signal
        if(InSig > 0)
        {
            ThisGyro.m_input = InSig - ThisGyro.m_stickDeadBand;
            if(ThisGyro.m_input < 0)
                ThisGyro.m_input = 0;
        }
        else
        {
            ThisGyro.m_input = InSig + ThisGyro.m_stickDeadBand;
            if(ThisGyro.m_input > 0)
                ThisGyro.m_input = 0;
        }

        // Clamp input to [-1, 1] range and apply sense reversal
        if(ThisGyro.m_input > 1)
            ThisGyro.m_input = 1;
        if(ThisGyro.m_input < -1)
            ThisGyro.m_input = -1;
        if(ThisGyro.m_senseReverse)
            ThisGyro.m_input = -ThisGyro.m_input;

        // Extract and limit master gain from gain channel
        ThisGyro.m_gain = Math<physics_Num>::Abs(GainSig);
        if(ThisGyro.m_gain < static_cast<physics_Num>(0.1))
            ThisGyro.m_gain = static_cast<physics_Num>(0.1);
        if(ThisGyro.m_gain > static_cast<physics_Num>(1))
            ThisGyro.m_gain = static_cast<physics_Num>(1);

        // Negative gain signal indicates heading lock mode
        ThisGyro.m_hlMode = (GainSig < 0);

        calcAcceleration(ThisGyro);

        // Calculate yaw demand with expo curve: y = S(x(1-e) + x^2(e))
        ThisGyro.m_yawDemand =
            ThisGyro.m_stickSensitivity *
            (ThisGyro.m_input * (static_cast<physics_Num>(1.0) - ThisGyro.m_stickExpo) +
             ThisGyro.m_input * Math<physics_Num>::Abs(ThisGyro.m_input) * ThisGyro.m_stickExpo);

        // Calculate and limit rate error
        ThisGyro.m_yawError = ThisGyro.m_yawDemand - ThisGyro.m_yawRate;
        if(ThisGyro.m_yawError > ThisGyro.m_yawErrorLimit)
            ThisGyro.m_yawError = ThisGyro.m_yawErrorLimit;
        if(ThisGyro.m_yawError < -ThisGyro.m_yawErrorLimit)
            ThisGyro.m_yawError = -ThisGyro.m_yawErrorLimit;

        // Calculate heading lock range limit
        ThisGyro.m_hlLimit = 1 / (ThisGyro.m_gain * ThisGyro.m_hlGain);
        if(ThisGyro.m_hlLimit > ThisGyro.m_hlRange)
            ThisGyro.m_hlLimit = ThisGyro.m_hlRange;

        // Process heading lock integration
        if(ThisGyro.m_hlOn && ThisGyro.m_hlMode)
        {
            // Integrate yaw error over time
            ThisGyro.m_hlError = ThisGyro.m_hlError + ThisGyro.m_yawError * dt;

            // Apply exponential decay if configured
            if(ThisGyro.m_hlDecay > static_cast<physics_Num>(0.01))
            {
                DecayFactor = (static_cast<physics_Num>(1.0) - dt / ThisGyro.m_hlDecay);
                if(DecayFactor < static_cast<physics_Num>(0.1))
                    DecayFactor = static_cast<physics_Num>(0.1);
                ThisGyro.m_hlError = ThisGyro.m_hlError * DecayFactor;
            }

            // Apply limits to heading lock error
            if(ThisGyro.m_hlError > ThisGyro.m_hlLimit)
                ThisGyro.m_hlError = ThisGyro.m_hlLimit;
            if(ThisGyro.m_hlError < -ThisGyro.m_hlLimit)
                ThisGyro.m_hlError = -ThisGyro.m_hlLimit;
        }
        else
        {
            ThisGyro.m_hlError = static_cast<physics_Num>(0);
        }

        calcStopGain(ThisGyro);

        // Main gyro control law combining all terms:
        // Output = Gain * (StopGain * RateError + HLError + AccTerm) + DirectInput
        ThisGyro.m_output =
            ThisGyro.m_gain *
            (ThisGyro.m_currentStopGain * ThisGyro.m_yawErrorGain * ThisGyro.m_yawError +
             ThisGyro.m_hlGain * ThisGyro.m_hlError + ThisGyro.m_accTerm) +
            ThisGyro.m_directGain * ThisGyro.m_input;

        auto ThrowLimit1 = ThisGyro.m_throwLimit1;
        auto ThrowLimit2 = ThisGyro.m_throwLimit2;

        // Apply sense reversal and output limits
        if(ThisGyro.m_senseReverse)
            ThisGyro.m_output = -ThisGyro.m_output;
        if(ThisGyro.m_output > ThrowLimit1)
            ThisGyro.m_output = ThrowLimit1;
        if(ThisGyro.m_output < -ThrowLimit2)
            ThisGyro.m_output = -ThrowLimit2;

        stopControl(ThisGyro, dt);
    }

    physics_Num AerodymanicsUtil::getTailGyro(physics_Num InSig, physics_Num GainSig,
                                              physics_Num dt, physics_Num YawRate)
    {
        auto &aero = HeliAero::getSingleton();
        auto &gyro = aero->m_tailGyro;

        gyro.m_yawRate = YawRate;
        gyroIn(InSig, GainSig, dt, gyro);
        return gyro.m_output + gyro.m_servoOffset;
    }

    void AerodymanicsUtil::initTailGyro()
    {
        auto &aero = HeliAero::getSingleton();
        auto &gyro = aero->m_tailGyro;

        aero->readDefaultGyroParameters();
        aero->readCurrentGyroParameters();
        resetGyro(gyro);
    }

    void AerodymanicsUtil::initVBar(CFlybarlessUnit &vBar)
    {
        std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();

        // Initialize VBar to align with main rotor shaft
        vBar.m_frame = heliAero->m_rotorHead->m_shaftFrame;

        // Reset all error and command values
        vBar.m_errorVector.x = 0;
        vBar.m_errorVector.y = 0;
        vBar.m_errorVector.z = 0;
        vBar.m_errorMag = 0;
        vBar.m_rollErrorAngle = 0;
        vBar.m_pitchErrorAngle = 0;
        vBar.m_ailCommand = 0;
        vBar.m_eleCommand = 0;
    }

    void AerodymanicsUtil::limitAndDecayVBar(physics_Num dt, CFlybarlessUnit &vBar)
    {
        std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
        physics_Vec stabVec;

        // Auto-stabilization: gradually align VBar toward vertical
        if(vBar.m_stabilize)
        {
            stabVec.x = static_cast<physics_Num>(0);
            stabVec.y = static_cast<physics_Num>(1); // Vertical reference
            stabVec.z = static_cast<physics_Num>(0);

            // Blend VBar vector toward vertical based on stabilization gain
            vBar.m_frame.m_yAxis =
                VSum(vBar.m_frame.m_yAxis, VScale(stabVec, dt * vBar.m_stabGain));
            vBar.m_frame.m_yAxis = VUnit(vBar.m_frame.m_yAxis);
        }

        // Bail-out mode processing
        switch(vBar.m_bailValue)
        {
        case 0:
            // No bail-out action
            break;
        case 1:
        case 2:
            // Apply stabilization toward vertical
            stabVec.x = static_cast<physics_Num>(0);
            stabVec.y = static_cast<physics_Num>(1);
            stabVec.z = static_cast<physics_Num>(0);

            vBar.m_frame.m_yAxis =
                VSum(vBar.m_frame.m_yAxis, VScale(stabVec, dt * vBar.m_stabGain));
            vBar.m_frame.m_yAxis = VUnit(vBar.m_frame.m_yAxis);
            break;
        }

        // Calculate error between shaft orientation and VBar reference
        vBar.m_errorVector =
            VDif(heliAero->m_rotorHead->m_shaftFrame.m_yAxis, vBar.m_frame.m_yAxis);
        vBar.m_errorMag = VMag(vBar.m_errorVector);

        // Apply angle limit to prevent excessive VBar-to-shaft deviation
        if(vBar.m_errorMag > vBar.m_angleLimit)
        {
            vBar.m_errorVector = VScale(vBar.m_errorVector, vBar.m_angleLimit / vBar.m_errorMag);
            vBar.m_frame.m_yAxis =
                VDif(heliAero->m_rotorHead->m_shaftFrame.m_yAxis, vBar.m_errorVector);
        }

        // Apply exponential decay to error vector
        vBar.m_errorVector =
            VScale(vBar.m_errorVector, static_cast<physics_Num>(1.0) - (dt / vBar.m_decay));
        vBar.m_frame.m_yAxis =
            VDif(heliAero->m_rotorHead->m_shaftFrame.m_yAxis, vBar.m_errorVector);
        vBar.m_frame.m_yAxis = VUnit(vBar.m_frame.m_yAxis);
    }

    void AerodymanicsUtil::controlVBar(physics_Num dt, physics_Num ailInSig, physics_Num eleInSig,
                                       CFlybarlessUnit &vBar)
    {
        std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();

        physics_Vec tVec;
        physics_Num rollAngle, pitchAngle;
        physics_Num newAilCmd, deltaAilCmd, limAilCmd, newEleCmd, deltaEleCmd, limEleCmd, ailChanVal,
                    eleChanVal;

        // Apply deadband to aileron input
        if(ailInSig > 0)
        {
            ailChanVal = ailInSig - vBar.m_stickDeadBand;
            if(ailChanVal < 0)
                ailChanVal = 0;
        }
        else
        {
            ailChanVal = ailInSig + vBar.m_stickDeadBand;
            if(ailChanVal > 0)
                ailChanVal = 0;
        }

        // Apply deadband to elevator input
        if(eleInSig > 0)
        {
            eleChanVal = eleInSig - vBar.m_stickDeadBand;
            if(eleChanVal < 0)
                eleChanVal = 0;
        }
        else
        {
            eleChanVal = eleInSig + vBar.m_stickDeadBand;
            if(eleChanVal > 0)
                eleChanVal = 0;
        }

        // Apply expo curve and rate limiting to aileron command
        newAilCmd = ailChanVal * ((static_cast<physics_Num>(1.0) - vBar.m_stickExpo) +
                                  Math<physics_Num>::Abs(ailChanVal) * vBar.m_stickExpo);
        deltaAilCmd = newAilCmd - vBar.m_ailCommand;
        limAilCmd = dt / vBar.m_ailFilter;
        if(deltaAilCmd > limAilCmd)
            deltaAilCmd = limAilCmd;
        if(deltaAilCmd < -limAilCmd)
            deltaAilCmd = -limAilCmd;
        vBar.m_ailCommand = vBar.m_ailCommand + deltaAilCmd;

        // Apply expo curve and rate limiting to elevator command
        newEleCmd = eleChanVal * ((static_cast<physics_Num>(1.0) - vBar.m_stickExpo) +
                                  Math<physics_Num>::Abs(eleChanVal) * vBar.m_stickExpo);
        deltaEleCmd = newEleCmd - vBar.m_eleCommand;
        limEleCmd = dt / vBar.m_eleFilter;
        if(deltaEleCmd > limEleCmd)
            deltaEleCmd = limEleCmd;
        if(deltaEleCmd < -limEleCmd)
            deltaEleCmd = -limEleCmd;
        vBar.m_eleCommand = vBar.m_eleCommand + deltaEleCmd;

        // Calculate precession angles from commands
        vBar.m_ailDemand = vBar.m_stickSensitivity * vBar.m_ailCommand;
        rollAngle = dt * vBar.m_ailDemand;

        vBar.m_eleDemand = vBar.m_stickSensitivity * vBar.m_eleCommand;
        pitchAngle = dt * vBar.m_eleDemand;

        // Create precession correction vector and apply to VBar
        tVec = VSum(VScale(heliAero->m_rotorHead->m_shaftFrame.m_xAxis, rollAngle),
                    VScale(heliAero->m_rotorHead->m_shaftFrame.m_zAxis, pitchAngle));

        vBar.m_frame.m_yAxis = VSum(vBar.m_frame.m_yAxis, tVec);
        vBar.m_frame.m_yAxis = VUnit(vBar.m_frame.m_yAxis);
    }

    void AerodymanicsUtil::getVBarOutputs(physics_Num &ailOut, physics_Num &eleOut,
                                          CFlybarlessUnit &vBar)
    {
        std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();

        // Calculate error angles by projecting VBar onto shaft axes
        vBar.m_rollErrorAngle =
            VDot(heliAero->m_rotorHead->m_shaftFrame.m_xAxis, vBar.m_frame.m_yAxis);
        vBar.m_pitchErrorAngle =
            VDot(heliAero->m_rotorHead->m_shaftFrame.m_zAxis, vBar.m_frame.m_yAxis);

        // Output combines VBar correction with direct stick input
        ailOut = vBar.m_rollErrorAngle * vBar.m_rollGain + vBar.m_ailCommand * vBar.m_directMix;
        eleOut = vBar.m_pitchErrorAngle * vBar.m_pitchGain + vBar.m_eleCommand * vBar.m_directMix;
    }

    void AerodymanicsUtil::vBarLoop(physics_Num dt, physics_Num AilInSig, physics_Num EleInSig,
                                    physics_Num &AilOut, physics_Num &EleOut, CFlybarlessUnit &VB)
    {
        controlVBar(dt, AilInSig, EleInSig, VB);
        limitAndDecayVBar(dt, VB);
        getVBarOutputs(AilOut, EleOut, VB);
    }

    bool AerodymanicsUtil::findValueOf(VehicleParam &Param, const String &PS)
    {
        try
        {
            // Parameter type constants
            constexpr int ABool = 0;
            constexpr int AInt = 1;
            constexpr int ASingle = 2;
            constexpr int AVec = 3;

            bool result;
            String S1, S2, S3;
            size_t NamePos, ValStart, ValEnd, ValLen;
            bool Status = true;

            // Search for parameter name in lowercase
            NamePos = PS.find(StringUtil::make_lower(Param.m_name));

            if(NamePos <= 0 || NamePos == String::npos)
            {
                result = false;
                WP_LOG("failure to return a value: " + Param.toString());
            }
            else
            {
                // Extract value portion between <value> and </value> tags
                S1 = PS.substr(NamePos, 500);
                ValStart = 7 + S1.find("<value>");

                // Strip leading whitespace
                while(S1[ValStart] == ' ')
                    ValStart++;

                ValEnd = S1.find("</value>") - 1;

                // Strip trailing whitespace
                while(S1[ValEnd] == ' ')
                    ValEnd++;

                ValLen = 1 + ValEnd - ValStart;
                S2 = S1.substr(ValStart, ValLen);

                // Parse value based on parameter type
                switch(Param.m_vType)
                {
                case ABool:
                {
                    if((S2 == "true") || (S2 == "yes") || (S2 == "on") ||
                       (S2 == "active") || (S2 == "enabled"))
                        Param.m_bVal = true;
                    else
                        Param.m_bVal = false;
                }
                break;

                case AInt:
                {
                    Param.m_iVal = StringUtil::parseInt(S2);
                }
                break;

                case ASingle:
                {
                    Param.m_sVal = StringUtil::parseFloat(S2);
                }
                break;

                case AVec:
                {
                    // Parse comma-separated vector components
                    auto vec = StringUtil::split(S2, ",");

                    if(vec.size() != 3)
                    {
                        Param.m_vVal.x = 0.0f;
                        Param.m_vVal.y = 0.0f;
                        Param.m_vVal.z = 0.0f;
                    }
                    else
                    {
                        Param.m_vVal.x = StringUtil::parseFloat(vec[0]);
                        Param.m_vVal.y = StringUtil::parseFloat(vec[1]);
                        Param.m_vVal.z = StringUtil::parseFloat(vec[2]);
                    }
                }
                break;

                default:
                {
                    Status = false;
                }
                }

                result = Status;
                WP_LOG("value set: " + Param.toString());
            }

            return result;
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }

        return false;
    }

    float AerodymanicsUtil::clutchTorque(float RPM, const EngineClutchUnit &EC)
    {
        // Clutch torque model: T = K * (RPM^2 - RPMEngage^2)
        // K depends on clutch dimensions and friction coefficient
        float CT = EC.m_clutchConst * (RPM * RPM - EC.m_biteRPMSq);

        if(CT < 0)
        {
            CT = 0; // No negative torque transmission
        }

        return CT;
    }

    void AerodymanicsUtil::lookupEnginePower(float Thr, EngineClutchUnit &EC)
    {
        std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();

        int LUP; // Look-up point index
        float DeltaRPM, DeltaTh;
        float P1, P2, P3, P4, P5, P6;
        float NormRPM;

        WP_ASSERT(!Math<physics_Num>::equals( EC.m_peakPowerRPM, 0.0f ));

        // Normalize RPM (1.0 = peak power RPM)
        NormRPM = EC.getCrankRPM() / EC.m_peakPowerRPM;
        EC.setCrankOmega(EC.getCrankRPM() * Math<physics_Num>::pi() /
                         static_cast<physics_Num>(30.0));

        // Calculate look-up table index (peak power at index 13)
        LUP =
            static_cast<s32>(Math<physics_Num>::trunc(NormRPM * static_cast<physics_Num>(13)));
        if(LUP < 0)
            LUP = 0;
        if(LUP > 26)
            LUP = 26;

        // Calculate interpolation factor within table cell
        DeltaRPM = static_cast<physics_Num>(13.0) * NormRPM - LUP;
        EC.m_throttlePos = Thr;

        // Bilinear interpolation based on throttle position
        if(EC.m_throttlePos > static_cast<physics_Num>(0.5))
        {
            // Upper throttle range (50% to 100%)
            DeltaTh = static_cast<physics_Num>(2.0) * (Thr - static_cast<physics_Num>(0.5));
            P1 = EC.m_powerCurves[1][LUP];
            P2 = EC.m_powerCurves[1][LUP + 1];
            P3 = EC.m_powerCurves[2][LUP];
            P4 = EC.m_powerCurves[2][LUP + 1];
            P5 = P1 + DeltaRPM * (P2 - P1); // Interpolate along RPM axis
            P6 = P3 + DeltaRPM * (P4 - P3);
            EC.m_enginePower = EC.m_peakPower * (P5 + DeltaTh * (P6 - P5));
            EC.m_enginePower *= heliAero->m_nitroFiddle;
            EC.setEngineTorque(EC.m_enginePower / EC.getCrankOmega());
        }
        else
        {
            // Lower throttle range (0% to 50%)
            DeltaTh = static_cast<physics_Num>(2.0) * Thr;
            P1 = EC.m_powerCurves[0][LUP];
            P2 = EC.m_powerCurves[0][LUP + 1];
            P3 = EC.m_powerCurves[1][LUP];
            P4 = EC.m_powerCurves[1][LUP + 1];
            P5 = P1 + DeltaRPM * (P2 - P1);
            P6 = P3 + DeltaRPM * (P4 - P3);
            EC.m_enginePower = EC.m_peakPower * (P5 + DeltaTh * (P6 - P5));
            EC.m_enginePower *= heliAero->m_nitroFiddle;
            EC.setEngineTorque(EC.m_enginePower / EC.getCrankOmega());
        }
    }

    void AerodymanicsUtil::readPowerLookups(EngineClutchUnit &EC)
    {
        try
        {
            std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();

            auto filePath =
                heliAero->getDataPath() + StringUtil::toUTF8to16(heliAero->m_pCurveFileName);

            WP_LOG("FBHeliAero opening : " + StringUtil::toUTF16to8( filePath ));

#if defined WP_PLATFORM_WIN32
            std::fstream stream(filePath.c_str(), std::fstream::in | std::fstream::binary);
#else
            std::fstream stream(StringUtil::toUTF16to8(filePath),
                                std::fstream::in | std::fstream::binary);
#endif

            SmartPtr<IStream> dataStream;

            String InStr, S1, S2;
            int nn = 0;
            float NRPM = 0;

            constexpr int bufferSize = 4096;
            char buffer[bufferSize];

            dataStream->readLine(buffer, bufferSize);

            // Read power curve data: columns are RPM, FullThrottle, HalfThrottle, IdleThrottle
            while(!dataStream->eof())
            {
                dataStream->readLine(buffer, bufferSize);

                auto values = StringUtil::split(buffer, ",");
                if(values.size() == 4)
                {
                    NRPM = StringUtil::parseFloat(values[0]); // Normalized RPM (informational)
                    EC.m_powerCurves[2][nn] = StringUtil::parseFloat(values[1]); // T = 1.0
                    EC.m_powerCurves[1][nn] = StringUtil::parseFloat(values[2]); // T = 0.5
                    EC.m_powerCurves[0][nn] = StringUtil::parseFloat(values[3]); // T = 0.0
                }

                nn++;
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    void AerodymanicsUtil::getLookups()
    {
        std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();
        readPowerLookups(*heliAero->m_theEngineClutch);
    }

    void AerodymanicsUtil::readEngineDataFile(EngineClutchUnit &EC)
    {
        try
        {
            std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();
            auto filePath =
                heliAero->getDataPath() + StringUtil::toUTF8to16(heliAero->m_engineFileName);

            WP_LOG("FBHeliAero opening : " + StringUtil::toUTF16to8( filePath ));

#if defined WP_PLATFORM_WIN32
            std::fstream stream(filePath.c_str(), std::fstream::in | std::fstream::binary);
#else
            std::fstream stream(StringUtil::toUTF16to8(filePath),
                                std::fstream::in | std::fstream::binary);
#endif

            SmartPtr<IStream> dataStream;

            constexpr int bufferSize = 4096;
            char buffer[bufferSize];

            dataStream->readLine(buffer, bufferSize);

            // Parse engine specification data
            while(!dataStream->eof())
            {
                dataStream->readLine(buffer, bufferSize);

                auto values = StringUtil::split(buffer, ",");

                EC.m_peakPower = StringUtil::parseFloat(values[0]);
                EC.m_peakPowerRPM = StringUtil::parseFloat(values[1]);
                EC.m_moiEngine = StringUtil::parseFloat(values[2]);
                EC.m_moiClutch = StringUtil::parseFloat(values[3]);
                EC.m_clutchConst = StringUtil::parseFloat(values[4]);
                EC.m_biteRPM = StringUtil::parseFloat(values[5]);
                EC.m_biteRPMSq = EC.m_biteRPM * EC.m_biteRPM; // Pre-calculate squared value
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    void AerodymanicsUtil::getEngineData()
    {
        std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();
        readEngineDataFile(*heliAero->m_theEngineClutch);
    }

    void AerodymanicsUtil::startEngine()
    {
        std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();
        EngineClutchUnit &TheEngineClutch = *heliAero->m_theEngineClutch;

        // Initialize engine to idle speed
        TheEngineClutch.setCrankRPM(2000.0);
        TheEngineClutch.setCrankOmega(TheEngineClutch.getCrankRPM() *
                                      static_cast<physics_Num>(RPMToOmega));
    }

    float AerodymanicsUtil::enginePower(float Thr)
    {
        std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();
        EngineClutchUnit &TheEngineClutch = *heliAero->m_theEngineClutch;

        float result = 0.0f;
        lookupEnginePower(Thr, TheEngineClutch);
        return result;
    }

    void AerodymanicsUtil::mainEngineClutchStep(physics_Num &OPRPM, physics_Num &ERPM,
                                                physics_Num LoadInertia, physics_Num LoadTorque,
                                                physics_Num dt, physics_Num Throttle,
                                                EngineClutchUnit &EC)
    {
        std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
        EngineClutchUnit &TheEngineClutch = *heliAero->m_theEngineClutch;

        s32 LL, Loops;
        physics_Num MicroT;
        bool ModeSwitched;

        // Subdivide timestep for numerical stability (target 1ms steps)
        Loops = static_cast<s32>(
            std::ceil(static_cast<physics_Num>(1.0) +
                      Math<physics_Num>::trunc(dt / static_cast<physics_Num>(0.001))));

        MicroT = dt / Loops;
        EC.m_spragRPM = OPRPM;
        EC.m_spragOmega = EC.m_spragRPM * static_cast<physics_Num>(RPMToOmega);

        for(LL = 0; LL < Loops; LL++)
        {
            // Force disengage at low RPM
            if(EC.getCrankRPM() < EC.m_biteRPM)
            {
                EC.m_clutchMode = m_kDisengaged;
            }

            lookupEnginePower(Throttle, TheEngineClutch);
            EC.m_clutchCapability = clutchTorque(EC.getCrankRPM(), TheEngineClutch);
            ModeSwitched = false;

            switch(EC.m_clutchMode)
            {
            case m_kDisengaged:
            {
                if(ModeSwitched == false)
                {
                    // No torque transmission - engine and load rotate independently
                    EC.m_transmittedTorque = static_cast<physics_Num>(0.0);
                    EC.setCrankOmega(EC.getCrankOmega() +
                                     MicroT * (EC.getEngineTorque() - EC.m_transmittedTorque) /
                                     (EC.m_moiEngine + EC.m_moiClutch));
                    EC.setCrankRPM(EC.getCrankOmega() * static_cast<physics_Num>(OmegaToRPM));
                    EC.m_spragOmega = EC.m_spragOmega +
                                      MicroT * (EC.m_transmittedTorque - LoadTorque) / LoadInertia;
                    EC.m_spragRPM = EC.m_spragOmega * static_cast<physics_Num>(OmegaToRPM);

                    // Check for engagement conditions
                    if(EC.getCrankRPM() > EC.m_biteRPM)
                    {
                        if(EC.getCrankRPM() > EC.m_spragRPM)
                            EC.m_clutchMode = m_kSlipping;
                        else
                            EC.m_clutchMode = m_kOverrun;
                    }
                    ModeSwitched = true;
                }
            }
            break;

            case m_kSlipping:
            {
                if(ModeSwitched == false)
                {
                    // Clutch transmitting at maximum capacity
                    EC.m_transmittedTorque = EC.m_clutchCapability;
                    EC.setCrankOmega(EC.getCrankOmega() +
                                     MicroT * (EC.getEngineTorque() - EC.m_transmittedTorque) /
                                     (EC.m_moiEngine + EC.m_moiClutch));
                    EC.setCrankRPM(EC.getCrankOmega() * static_cast<physics_Num>(OmegaToRPM));
                    EC.m_spragOmega = EC.m_spragOmega +
                                      MicroT * (EC.m_transmittedTorque - LoadTorque) / LoadInertia;
                    EC.m_spragRPM = EC.m_spragOmega * static_cast<physics_Num>(OmegaToRPM);

                    if(EC.getCrankRPM() < EC.m_biteRPM)
                    {
                        EC.m_clutchMode = m_kDisengaged;
                    }

                    // Check for lock-up: speeds matched and clutch has capacity
                    if((EC.m_spragRPM > EC.getCrankRPM() - static_cast<physics_Num>(60.0)) &&
                       (EC.m_clutchCapability > EC.getEngineTorque()) &&
                       (EC.getEngineTorque() > 0))
                    {
                        EC.m_clutchMode = m_kLocked;
                        ModeSwitched = true;
                    }

                    // Check for overrun: load driving engine
                    if((EC.m_spragRPM > EC.getCrankRPM()) && (EC.getEngineTorque() < 0))
                    {
                        EC.m_clutchMode = m_kOverrun;
                        ModeSwitched = true;
                    }
                }
            }
            break;

            case m_kLocked:
            {
                if(ModeSwitched == false)
                {
                    // Clutch fully engaged - engine and load rotate together
                    EC.m_transmittedTorque = EC.getEngineTorque();
                    EC.m_spragOmega =
                        EC.m_spragOmega + MicroT * (EC.m_transmittedTorque - LoadTorque) /
                        (EC.m_moiEngine + EC.m_moiClutch + LoadInertia);
                    EC.m_spragRPM = EC.m_spragOmega * static_cast<physics_Num>(OmegaToRPM);
                    EC.setCrankOmega(EC.m_spragOmega);
                    EC.setCrankRPM(EC.m_spragRPM);

                    // Check for slip: torque exceeds clutch capacity
                    if(EC.m_clutchCapability < EC.getEngineTorque())
                    {
                        EC.m_clutchMode = m_kSlipping;
                        ModeSwitched = true;
                    }

                    // Check for overrun
                    if(EC.getEngineTorque() < 0)
                    {
                        EC.m_clutchMode = m_kOverrun;
                        ModeSwitched = true;
                    }

                    // Check for disengage
                    if(EC.getCrankRPM() < EC.m_biteRPM)
                    {
                        EC.m_clutchMode = m_kDisengaged;
                        ModeSwitched = true;
                    }
                }
            }
            break;

            case m_kOverrun:
            {
                if(ModeSwitched == false)
                {
                    // Sprag clutch freewheeling - load driving engine side
                    EC.m_transmittedTorque = 0;
                    EC.m_spragOmega = EC.m_spragOmega +
                                      MicroT * (EC.m_transmittedTorque - LoadTorque) / LoadInertia;
                    EC.m_spragRPM = EC.m_spragOmega * static_cast<physics_Num>(OmegaToRPM);
                    EC.setCrankOmega(EC.getCrankOmega() +
                                     MicroT * (EC.getEngineTorque() - EC.m_transmittedTorque) /
                                     (EC.m_moiEngine + EC.m_moiClutch));
                    EC.setCrankRPM(EC.getCrankOmega() * static_cast<physics_Num>(OmegaToRPM));

                    if(EC.getCrankRPM() < EC.m_biteRPM)
                    {
                        EC.m_clutchMode = m_kDisengaged;
                        ModeSwitched = true;
                    }

                    // Re-engage when engine catches up
                    if(EC.getCrankRPM() > EC.m_spragRPM)
                    {
                        EC.m_clutchMode = m_kSlipping;
                        ModeSwitched = true;
                    }
                }
            }
            break;
            }
        }

        OPRPM = EC.m_spragRPM;
        ERPM = EC.getCrankRPM();
    }

    void AerodymanicsUtil::resetGovernor(CGovernorUnit &governor)
    {
        // Reset all governor state variables
        governor.m_input = 0;
        governor.m_active = false;
        governor.m_lastRpm = 0;
        governor.m_targetRpm = 0;
        governor.m_acceleration = 0;
        governor.m_accTimeConstant = static_cast<physics_Num>(0.1);
        governor.m_rpmError = 0;
        governor.m_phaseError = 0;
        governor.m_output = 0;
        governor.m_maxControlPoint = 1;
        governor.m_positiveGrowthEnable = true;
        governor.m_negativeGrowthEnable = true;
        governor.m_resetLimiter = static_cast<physics_Num>(0.05);
    }

    void AerodymanicsUtil::doGovernor(physics_Num inputSignal, physics_Num speedSignal,
                                      physics_Num deltaTime, physics_Num currentRpm,
                                      CGovernorUnit &governor)
    {
        physics_Num deltaTarget;
        physics_Num outputTarget;
        physics_Num deltaOutput;
        physics_Num thisAcceleration;
        physics_Num filterFactor;
        physics_Num deltaPhase;

        // Gradually increase reset limiter over first 5 seconds
        governor.m_resetLimiter =
            governor.m_resetLimiter + static_cast<physics_Num>(0.2) * deltaTime;
        if(governor.m_resetLimiter > static_cast<physics_Num>(1.0))
            governor.m_resetLimiter = static_cast<physics_Num>(1.0); // limit the ResetLimiter to 1

        // Determine required RPM based on governor mode
        switch(governor.m_mode)
        {
        case 0: // Disabled
            governor.m_reqRpm = static_cast<physics_Num>(0.0);
            break;
        case 1: // Fixed RPM
            governor.m_reqRpm = governor.m_fixedRpm;
            break;
        case 2: // Remote control
        {
            governor.m_remoteSig = speedSignal;
            // Clamp remote signal to [-1, 1]
            if(governor.m_remoteSig < static_cast<physics_Num>(-1.0))
                governor.m_remoteSig = static_cast<physics_Num>(-1.0);
            if(governor.m_remoteSig > static_cast<physics_Num>(1.0))
                governor.m_remoteSig = static_cast<physics_Num>(1.0);

            // Map signal to RPM range
            governor.m_reqRpm = governor.m_rpmRangeBottom +
                                static_cast<physics_Num>(0.5) *
                                (static_cast<physics_Num>(1.0) + governor.m_remoteSig) *
                                (governor.m_rpmRangeTop - governor.m_rpmRangeBottom);
        }
        break;
        }

        governor.m_input = inputSignal;

        // Apply reset limiter during startup
        if(governor.m_input > governor.m_resetLimiter)
        {
            governor.m_input = governor.m_resetLimiter;
        }

        // Deactivate at low throttle
        if(governor.m_input < static_cast<physics_Num>(0.15))
        {
            governor.m_active = false;
        }

        // Bypass governor if not configured
        if(governor.m_reqRpm == static_cast<physics_Num>(0.0))
        {
            governor.m_active = false;
            governor.m_output = governor.m_input;
            return;
        }

        if(governor.m_active == false)
        {
            // Check activation conditions: throttle > 25% and RPM > 70% of target
            if((governor.m_input > static_cast<physics_Num>(0.25)) &&
               (currentRpm > static_cast<physics_Num>(0.7) * governor.m_reqRpm))
            {
                governor.m_targetRpm = currentRpm;
                governor.m_lastRpm = currentRpm;
                governor.m_rpmError = static_cast<physics_Num>(0.0);
                governor.m_phaseError = static_cast<physics_Num>(0.0);

                if(governor.m_targetRpm > governor.m_reqRpm)
                {
                    governor.m_targetRpm = governor.m_reqRpm;
                }

                governor.m_active = true;
            }
            else
            {
                // Pass through throttle unchanged when inactive
                governor.m_output = governor.m_input;
            }
        }
        else
        {
            // Active governor control loop

            // Calculate filtered acceleration
            thisAcceleration = (currentRpm - governor.m_lastRpm) / deltaTime;
            governor.m_lastRpm = currentRpm;
            filterFactor = deltaTime / governor.m_accTimeConstant;
            governor.m_acceleration =
                (1 - filterFactor) * governor.m_acceleration + filterFactor * thisAcceleration;
            Math<physics_Num>::Limit(governor.m_acceleration, governor.m_accLimit);

            // Ramp target toward required RPM
            deltaTarget = governor.m_reqRpm - governor.m_targetRpm;
            Math<physics_Num>::Limit(deltaTarget, governor.m_rampRate * deltaTime);
            governor.m_targetRpm = governor.m_targetRpm + deltaTarget;

            // Calculate proportional error
            governor.m_rpmError = currentRpm - governor.m_targetRpm;
            Math<physics_Num>::Limit(governor.m_rpmError, governor.m_rpmErrorLimit);

            // Calculate integral (phase) error with anti-windup
            deltaPhase = deltaTime * governor.m_rpmError;
            if(((deltaPhase < static_cast<physics_Num>(0.0)) &&
                governor.m_positiveGrowthEnable) ||
               ((deltaPhase > static_cast<physics_Num>(0.0)) &&
                governor.m_negativeGrowthEnable))
            {
                governor.m_phaseError = governor.m_phaseError + deltaPhase;
            }
            Math<physics_Num>::Limit(governor.m_phaseError, governor.m_phaseErrorLimit);

            // PID output calculation (centered on 50% throttle)
            outputTarget =
                static_cast<physics_Num>(0.5) - (governor.m_accGain * governor.m_acceleration +
                                                 governor.m_rpmGain * governor.m_rpmError +
                                                 governor.m_phaseGain * governor.m_phaseError);

            // Apply slew rate limiting (~0.1s for full travel)
            deltaOutput = outputTarget - governor.m_output;
            Math<physics_Num>::Limit(deltaOutput, static_cast<physics_Num>(10.0) * deltaTime);
            governor.m_output = governor.m_output + deltaOutput;

            // Apply upper limit with anti-windup
            if(governor.m_output > governor.m_maxControlPoint)
            {
                governor.m_output = governor.m_maxControlPoint;
                governor.m_positiveGrowthEnable = false;
            }
            else
            {
                governor.m_positiveGrowthEnable = true;
            }

            // Apply lower limit with anti-windup
            if(governor.m_output < governor.m_minControlPoint)
            {
                governor.m_output = governor.m_minControlPoint;
                governor.m_negativeGrowthEnable = false;
            }
            else
            {
                governor.m_negativeGrowthEnable = true;
            }
        }
    }

    void AerodymanicsUtil::initGovernor()
    {
        std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();
        CGovernorUnit &TheGovernor = *heliAero->m_theGovernor;

        heliAero->readDefaultGovernorParameters();
        heliAero->readCurrentGovernorParameters();
        resetGovernor(TheGovernor);
    }

    void AerodymanicsUtil::setGovernorRPM(float RPM, float Gash)
    {
        std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();
        CGovernorUnit &TheGovernor = *heliAero->m_theGovernor;

        TheGovernor.m_reqRpm = RPM;
    }

    physics_Num AerodymanicsUtil::getGovernor(physics_Num InSig, physics_Num SpeedSig,
                                              physics_Num dt, physics_Num RPM) /* export */
    {
        std::shared_ptr<HeliAero> heliAero = HeliAero::getSingleton();
        CGovernorUnit &TheGovernor = *heliAero->m_theGovernor;

        doGovernor(InSig, SpeedSig, dt, RPM, TheGovernor);
        return TheGovernor.m_output;
    }

    void AerodymanicsUtil::resetMotor(CEMotor &Motor)
    {
    }

    void AerodymanicsUtil::chargePack(CBatteryPack &Pack)
    {
        Pack.m_packState = 1; // set the pack to fully charged
        Pack.m_aHrUsed = 0; // reset the sum of the used AHr used
        Pack.m_dischargeDuration = 0; // reset the time for which a non-zero current has been drawn
    }

    void AerodymanicsUtil::dischargePack(CBatteryPack &Pack, physics_Num I, physics_Num dt)
    {
        auto &heliAero = HeliAero::getSingleton();

        // PFloat PackFactor;
        physics_Num DeltaAHr;
        physics_Num VRange;

        if(heliAero->m_emulateBattery.m_bVal == true)
        {
            if(I > static_cast<physics_Num>(0.0))
            {
                Pack.m_dischargeDuration = Pack.m_dischargeDuration + dt; // sum up the discharge
                // time
            }

            DeltaAHr = I * dt / static_cast<physics_Num>(3600.0);
            // this is the amount of discharge in AHr
            Pack.m_aHrUsed = Pack.m_aHrUsed + DeltaAHr; // accumulate the used AHr of the pack
            Pack.m_packState = Pack.m_packState -
                               DeltaAHr / Pack.m_cellAHr; // and calculate the new state of charge

            // now use our simple three-point curve to get no-load voltage
            VRange =
                Pack.m_cellFullV - Pack.m_cellFlatV; // calc the voltage range between full and flat

            if(Pack.m_packState > static_cast<physics_Num>(0.9))
            {
                Pack.m_cellV = static_cast<physics_Num>(0.72) * VRange + Pack.m_cellFlatV +
                               static_cast<physics_Num>(2.8) * VRange *
                               (Pack.m_packState -
                                static_cast<physics_Num>(0.9)); // if pack >90% charged
            }
            else
            {
                if(Pack.m_packState > static_cast<physics_Num>(0.05))
                {
                    Pack.m_cellV = static_cast<physics_Num>(0.44) * VRange + Pack.m_cellFlatV +
                                   static_cast<physics_Num>(0.329) * VRange *
                                   (Pack.m_packState - static_cast<physics_Num>(0.05));
                    // else if pack>5% charged
                }
                else
                {
                    Pack.m_cellV =
                        Pack.m_cellFlatV + static_cast<physics_Num>(8.8) * VRange *
                        (Pack.m_packState); // else if pack <5% charged
                }
            }

            Pack.m_packV = Pack.m_cellsInPack * Pack.m_cellV;
            // use the current Cell no-load voltage to calc the total pack no-load voltage
            Pack.m_packR = Pack.m_cellsInPack * Pack.m_cellR; // calc the total pack resistance
        }
        else
        {
            Pack.m_packState = static_cast<physics_Num>(0.8);
            // if battery not emulated then maintain steady 80% charge state
            Pack.m_packV =
                Pack.m_cellsInPack *
                (Pack.m_cellFlatV + (Pack.m_cellFullV - Pack.m_cellFlatV) * Pack.m_packState);
            Pack.m_packR = Pack.m_cellsInPack * Pack.m_cellR;
        }

        escCutoutControl(dt, Pack, *heliAero->m_esc);
    }

    void AerodymanicsUtil::motorCalc(const CBatteryPack &Pack, CEMotor &Motor,
                                     const CESController &ESC)
    {
        physics_Num DriveVolts;
        physics_Num CurrentLim;
        // the limit depending on the ESC and motor limits (the lower of the two ruling)

        if(Motor.m_currentLimit > ESC.m_iLimit)
        {
            // if ESC limit is the lower
            CurrentLim = (ESC.m_iLimit + static_cast<physics_Num>(0.3) *
                          Motor.m_currentLimit); // use 100% of the lower limit +
            // 30% of the higher limit
        }
        else
        {
            // if Motor limit is the lower
            CurrentLim = (Motor.m_currentLimit + static_cast<physics_Num>(0.3) * ESC.m_iLimit);
        }

        Motor.m_motorTi =
            static_cast<physics_Num>(60.0) * Motor.m_motorEfficiency /
            (Math<physics_Num>::pi() * Motor.m_motorKv); // compute the torque constant of the
        // motor from the KV and efficiency
        Motor.m_motorEmf = Motor.m_motorRpm / Motor.m_motorKv; // calculate the motor back emp
        DriveVolts =
            Pack.m_packV -
            Motor.m_motorEmf; // this is the voltage available across the pack+motor resistances
        Motor.m_motorCurrent =
            ESC.m_output * DriveVolts /
            (Pack.m_packR + Motor.m_motorR +
             ESC.m_resistance); // calc current as (Duty Cycle of ESC)*(V-EMF)/Rtot

        if(Motor.m_motorCurrent > ESC.m_iLimit)
        {
            Motor.m_motorCurrent = CurrentLim; // apply the calculated current limit
        }

        if(ESC.m_cutoffActive)
        {
            Motor.m_motorCurrent = static_cast<physics_Num>(0.0);
            // if the cutoff is active we zero the motor current
        }

        Motor.m_motorTorque =
            Motor.m_motorTi *
            (Motor.m_motorCurrent - Motor.m_noLoadCurrent); // calc the torque for this current
        // allowing for no-load current;
        Motor.m_motorPower =
            Motor.m_motorOmega * Motor.m_motorTorque; // calculate the output power of the motor
    }

    void AerodymanicsUtil::escCutoutControl(physics_Num dt, const CBatteryPack &Pack,
                                            CESController &ESC)
    {
        if(Pack.m_packV < static_cast<physics_Num>(0.93) * ESC.m_cutoffV * Pack.m_cellsInPack)
        {
            // if pack less than 93% of the cutoff voltage then hard cut
            ESC.m_cutoffActive = true;
        }
        else
        {
            if(Pack.m_packV < ESC.m_cutoffV * Pack.m_cellsInPack)
            {
                ESC.m_cutoffTimer = ESC.m_cutoffTimer + dt;
                if(Math<physics_Num>::equals(
                    Math<physics_Num>::Mod(Math<physics_Num>::trunc(ESC.m_cutoffTimer),
                                           5.0),
                    0.0))
                    ESC.m_cutoffActive = true;
                else
                    ESC.m_cutoffActive =
                        false; // if below cutoff but  >93% cutoff pulse power on and off
            }
            else
            {
                ESC.m_cutoffTimer = static_cast<physics_Num>(0.0);
            }
        }
    }

    void AerodymanicsUtil::initMotorAndESC(CEMotor &Motor, CESController &ESC)
    {
        Motor.m_motorRpm = 0;
        Motor.m_motorOmega = 0;
        Motor.m_motorEmf = 0;
        Motor.m_motorCurrent = 0;
        Motor.m_motorTorque = 0;
        Motor.m_motorPower = 0;
        Motor.m_motorAccel = 0;
        Motor.m_spragRpm = 0;
        Motor.m_spragOmega = 0;
        Motor.m_spragMode = m_kOverrun; // set the motor mode to a realistic one!
        ESC.m_remoteSig = 0;
        ESC.m_active = false;
        ESC.m_lastActive = false;
        ESC.m_lastRpm = 0;
        ESC.m_targetRpm = 0;
        ESC.m_softStart = true; // set when the unit is set up to do a soft start
        ESC.m_rampRate = ESC.m_rpmRangeTop / ESC.m_slowRampTc;
        // holds the engagement RPM ramp rate in RPM/s  note reduced by factor of 2 for initial ramp
        ESC.m_acceleration = 0; // holds a smoothed acceleration rate
        ESC.m_accTc = 0.1; // set the filter TC for the acceleration measurements to 0.1 seconds
        ESC.m_rpmError = 0; // the speed error in engine radians/s Positive = engine fast
        ESC.m_phaseError = 0; // the phase angle by which the engine leads the reference (i.e. positive
        // is engine fast)
        ESC.m_output = 0; // the output duty cycle of the controller
        ESC.m_maxControlPoint = 1; // the limit to the throttle open swing (usually set to 1)
        ESC.m_positiveGrowthEnable = true; // flag for anti integral wind-up control
        ESC.m_negativeGrowthEnable = true; // flag for anti integral wind-up control
        ESC.m_cutoffActive = false; // set if the voltage cutout in action
        ESC.m_resetLimiter = 0.07;
    }

    void AerodymanicsUtil::rampControl(physics_Num dt, CESController &ESC)
    {
        /*with ESC do*/
        {
            if(ESC.m_active == false) // if the governor is inactive
            {
                if(ESC.m_lastActive == true) // but was active in the previous frame
                {
                    // gov has just gone inactive
                    ESC.m_lastActive = false; // save the avtivity state
                    ESC.m_softStart = false; // flag the unit is in fast start mode
                    ESC.m_rampRate = ESC.m_rpmRangeTop / ESC.m_fastRampTc; // set the ramp to fast
                    ESC.m_softStartTimer =
                        ESC.m_softStartDelay; // set the timer to the delay value in seconds
                } // end of gov just gone inactive
                else // gov was is already inactive
                {
                    // gov staying inactive
                    ESC.m_softStartTimer = ESC.m_softStartTimer - dt; // knock off this dt from the timer
                    if(ESC.m_softStartTimer < 0)
                    {
                        // soft start delay has run to its end
                        ESC.m_softStart = true; // flag the unit is in soft start mode
                        ESC.m_rampRate = ESC.m_rpmRangeTop / ESC.m_slowRampTc; // set the ramp to slow
                    } // end of soft start timer run to end
                } // end of gov staying inactive
            } // end of Active = False
            else
            {
                if(ESC.m_lastActive == false)
                {
                    ESC.m_lastActive = true;
                }
            } // end of active = true
        } // end of with ESC
    }

    void AerodymanicsUtil::escDoGovernor(physics_Num InSig, physics_Num dt, physics_Num RPM,
                                         const CBatteryPack &Pack, CESController &ESC)
    {
        physics_Num dTarg; // use in ramping the target speed
        physics_Num OTarg; // holds the target output
        physics_Num DeltaO; // holds the unlimited change in output
        physics_Num ThisAcc; // holds the new acceleration value
        physics_Num K; // temp for the accel timeconst factor
        physics_Num DeltaPhase; // holds the change in phase error

        /*with ESC do*/
        {
            ESC.m_resetLimiter = ESC.m_resetLimiter + static_cast<physics_Num>(0.2) * dt;
            // update the ResetLimiter
            if(ESC.m_resetLimiter > static_cast<physics_Num>(1.0))
                ESC.m_resetLimiter = static_cast<physics_Num>(1.0); // limit the ResetLimiter to 1

            ESC.m_input =
                static_cast<physics_Num>(1.11) * (InSig - static_cast<physics_Num>(0.1));
            // adjust Incoming throttle signal to allow some stick before motor starts
            if(ESC.m_input < 0)
                ESC.m_input = 0; // limit the range of the input
            if(ESC.m_input > ESC.m_resetLimiter)
                ESC.m_input = ESC.m_resetLimiter;
            // apply the Reset limit (<1 for the first 5 seconds after reset)

            switch(ESC.m_mode)
            {
            case // based on the governor mode set the ReqRPM value
            0:
                ESC.m_reqRpm = 0;
                break;
            case 1:
                ESC.m_reqRpm = ESC.m_fixedRpm;
                break;
            case 2:
            {
                ESC.m_remoteSig =
                    ESC.m_input - static_cast<physics_Num>(0.25) *
                    static_cast<physics_Num>(
                        1.333); // derive a 'speed signal' from the throttle
                if(ESC.m_remoteSig > 1)
                    ESC.m_remoteSig = 1; // bound the remote signal
                if(ESC.m_remoteSig < 0)
                    ESC.m_remoteSig = 0;
                ESC.m_reqRpm =
                    ESC.m_rpmRangeBottom + ESC.m_remoteSig * (ESC.m_rpmRangeTop - ESC.m_rpmRangeBottom);
            }
            break;
            } // case

            rampControl(
                dt,
                ESC); // look after the soft start condition based on activity state and time

            if(ESC.m_input < static_cast<physics_Num>(0.08))
            {
                ESC.m_active = false;
                // deactivate the governor if input below 8% throttle now in StartRampControl
            }

            if(Math<physics_Num>::equals(ESC.m_reqRpm, 0.0))
            // if Governor not in use
            {
                ESC.m_active = false; // disable Governor if the required RPM is set to zero!
                ESC.m_output = ESC.m_input;
                return;
            }

            if(ESC.m_active == false)
            {
                //
                if((ESC.m_input > static_cast<physics_Num>(0.12)) &&
                   (RPM > static_cast<physics_Num>(0.1) *
                    ESC.m_reqRpm)) // test for activation conditions
                {
                    ESC.m_targetRpm = RPM; // set the Target RPM to match the current RPM
                    ESC.m_lastRpm = RPM;
                    // initialise the LastRPM so acceleration calc does not go nuts on activation
                    ESC.m_rpmError = 0; //
                    ESC.m_phaseError = 0; // zero the integral error
                    if(ESC.m_targetRpm > ESC.m_reqRpm)
                        ESC.m_targetRpm =
                            ESC.m_reqRpm; // but if the current RPM is > the required RPM then limit it
                    ESC.m_active = true; // activate the governor
                } // end of activation
                else // i.e unit not active
                {
                    ESC.m_output =
                        ESC.m_input; // if inactive simply pass throttle signal through unchanged
                }
            } // end of active = false
            else // i.e the unit is active
            {
                ThisAcc = (RPM - ESC.m_lastRpm) / dt; // calculate the acceleration in this time step
                ESC.m_lastRpm = RPM; // update last RPM value
                K = dt / ESC.m_accTc; // calculate the factor for the accel filter
                ESC.m_acceleration = (static_cast<physics_Num>(1.0) - K) * ESC.m_acceleration +
                                     K * ThisAcc; // update the acceleration value
                Math<physics_Num>::Limit(
                    ESC.m_acceleration,
                    ESC.m_accLimit); // apply the limit to the acceleration value
                dTarg = ESC.m_reqRpm - ESC.m_targetRpm; // find if the target needs ramping up or down
                Math<physics_Num>::Limit(dTarg,
                                         ESC.m_rampRate *
                                         dt); // apply limit to dTarg of 4000 RPM/sec
                ESC.m_targetRpm = ESC.m_targetRpm +
                                  dTarg; // ramp the Target omega towards the current Target received
                ESC.m_rpmError = RPM - ESC.m_targetRpm; // get the speed error in rad./s
                Math<physics_Num>::Limit(ESC.m_rpmError,
                                         ESC.m_rpmErrorLimit); // apply a limit to this value
                DeltaPhase = dt * ESC.m_rpmError;
                // calc the shift in phase error in RPM seconds (which is a unit equal to 6 degrees!)

                // now test if the phase error is to be permitted to be added in (based on the servo
                // already being on the given limit
                if(((DeltaPhase < -std::numeric_limits<physics_Num>::epsilon()) &&
                    ESC.m_positiveGrowthEnable) ||
                   ((DeltaPhase > std::numeric_limits<physics_Num>::epsilon()) &&
                    ESC.m_negativeGrowthEnable))
                {
                    ESC.m_phaseError = ESC.m_phaseError + DeltaPhase;
                }

                Math<physics_Num>::Limit(ESC.m_phaseError, ESC.m_phaseErrorLimit);
                OTarg = static_cast<physics_Num>(0.5) -
                        (ESC.m_accGain * ESC.m_acceleration + ESC.m_rpmGain * ESC.m_rpmError +
                         ESC.m_phaseGain * ESC.m_phaseError);
                // calc the appropriate governor output (centring it on half throttle)
                DeltaO = OTarg - ESC.m_output; // apply a slew limit to the output
                Math<physics_Num>::Limit(DeltaO, static_cast<physics_Num>(100.0) * dt);
                // limit it so that slew takes about 0.1s
                ESC.m_output = ESC.m_output + DeltaO; // add the Delta

                if(ESC.m_output > ESC.m_maxControlPoint)
                {
                    // if output on the full-throttle limit
                    ESC.m_output = ESC.m_maxControlPoint; // apply a limit to the output value
                    ESC.m_positiveGrowthEnable = false;
                    // turn off further positive integration of the phase error (to limit integral
                    // term wind-up)
                }
                else // i.e. if output not on the full-throttle stop
                {
                    ESC.m_positiveGrowthEnable = true; // enable positive integral growth
                }

                if(ESC.m_output <
                   ESC.m_minControlPoint) // now test if throttle at the minimum control poin
                {
                    ESC.m_output = ESC.m_minControlPoint;
                    // set output to minimum throttle point for the active governor
                    ESC.m_negativeGrowthEnable =
                        false; // prevent further negative growth of the integral term
                }
                else // i.e the throttle not at the minimum control point
                {
                    ESC.m_negativeGrowthEnable = true; // allow negative integral term growth
                }
            } // end of governor active
        } // With ESC
    }

    void AerodymanicsUtil::mainEMotorStep(physics_Num &OPRPM, physics_Num &ERPM,
                                          physics_Num LoadInertia, physics_Num LoadTorque,
                                          physics_Num dt, CEMotor &EM)
    {
        auto &heliAero = HeliAero::getSingleton();

        int LL, Loops;
        physics_Num MicroT;
        physics_Num DeltaOmegaL; // the change in the Omega of the locked assembly

        physics_Num DeltaOmegaOS; // the change in the sprag Omega in overrun mode

        physics_Num DeltaOmegaOM; // the change in motor omega in overrun mode

        bool ModeSwitched;
        Loops = static_cast<s32>(
            std::ceil(static_cast<physics_Num>(1.0) +
                      Math<physics_Num>::trunc(dt / static_cast<physics_Num>(0.001))));
        // find the number of loops needed for 1ms timesteps
        if(Loops < 1)
        {
            Loops = 1;
        }

        MicroT = dt / Loops;
        EM.m_spragRpm = OPRPM;
        // make the Sprag RPM (the final output rpm) equal to the RPM passed to the procedure.
        EM.m_spragOmega = EM.m_spragRpm * static_cast<physics_Num>(RPMToOmega);

        for(LL = 0; LL < Loops; LL++)
        {
            // LookupEnginePower(Throttle,TheEngineClutch); //get the engine output
            motorCalc(*heliAero->m_pack, *heliAero->m_motor, *heliAero->m_esc);
            // DischargePack(Pack, Motor.MotorCurrent, MicroT);
            ModeSwitched = false;
            switch(EM.m_spragMode)
            {
            case m_kLocked:
            {
                if(ModeSwitched == false)
                {
                    DeltaOmegaL =
                        MicroT * (EM.m_motorTorque - LoadTorque) /
                        (EM.m_motorMoI + LoadInertia); // calculate the deltaOmega for the assembly
                    EM.m_transmittedTorque =
                        EM.m_motorTorque - (EM.m_motorMoI * DeltaOmegaL / MicroT);
                    // and sebtract the fraction of the motor torque used to accelerate the motor
                    // itself
                    EM.m_spragOmega =
                        EM.m_spragOmega + DeltaOmegaL; // update the Sprag omega to the new value
                    EM.m_spragRpm = EM.m_spragOmega * static_cast<physics_Num>(OmegaToRPM);
                    // and update the rpm
                    EM.m_motorOmega = EM.m_spragOmega; // match the motor omega to the sprag
                    EM.m_motorRpm = EM.m_spragRpm; // and likewise the rpm
                    if(EM.m_motorTorque < static_cast<physics_Num>(0.0))
                    {
                        EM.m_spragMode = m_kOverrun;
                        ModeSwitched = true;
                    } // if motor torque is less than zero then go to overrun state
                }
            }
            break; // Locked

            case m_kOverrun:
            {
                if(ModeSwitched == false)
                {
                    EM.m_transmittedTorque = static_cast<physics_Num>(0.0);
                    DeltaOmegaOS = -MicroT * LoadTorque /
                                   LoadInertia; // calculate the slowing of the undriven load inertia
                    EM.m_spragOmega =
                        EM.m_spragOmega + DeltaOmegaOS; // update the sprag omega appropriately
                    EM.m_spragRpm = EM.m_spragOmega * static_cast<physics_Num>(OmegaToRPM);
                    // update the sprag rpm  to match the omega figure
                    DeltaOmegaOM =
                        MicroT * EM.m_motorTorque /
                        EM.m_motorMoI; // calculate the change in motor omega under its own torque
                    EM.m_motorOmega =
                        EM.m_motorOmega + DeltaOmegaOM; // and update the motor omega appropriately
                    EM.m_motorRpm = EM.m_motorOmega * static_cast<physics_Num>(OmegaToRPM);
                    // and update the resulting rpm
                    if(EM.m_motorRpm > EM.m_spragRpm)
                    // if the motor has accelerated to exceed the sprag speed then go to locked mode
                    {
                        EM.m_spragMode = m_kLocked; // set the mode to locked
                        EM.m_motorRpm = EM.m_spragRpm; // match the motor to the sprag speed
                        EM.m_motorOmega = EM.m_spragOmega; //(both omega and rpm)
                        ModeSwitched = true; // flag the mode change
                    }
                }
            }
            break; // Overrun
            } // case
            if(EM.m_motorOmega < 0)
            {
                EM.m_motorOmega = 0; // stop motor going backwards!
                EM.m_motorRpm = 0;
            }
        } // for LL

        OPRPM = EM.m_spragRpm; // output the new speed for the heli mechanics
        ERPM = EM.m_motorRpm;

        dischargePack(*heliAero->m_pack, heliAero->m_motor->m_motorCurrent,
                      dt); // do the pack discharge thing here
        // Writeln(EC.SpragRPM, EC.CrankRPM, '   ', EC.ClutchMode, '  ',EC.ThrottlePos);
    }
}
