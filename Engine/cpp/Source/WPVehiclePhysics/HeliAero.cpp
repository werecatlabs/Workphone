#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/HeliAero.hpp"
#include <WPVehiclePhysics/AerodymanicsUtil.hpp>
#include <WPVehiclePhysics/CAerodymanicsWind.hpp>
#include "WPVehiclePhysics/CESController.hpp"
#include "WPVehiclePhysics/CEMotor.hpp"
#include "WPVehiclePhysics/FoilData.hpp"
#include "WPVehiclePhysics/Rotor.hpp"
#include "WPVehiclePhysics/RotorHead.hpp"
#include "WPVehiclePhysics/RotorSector.hpp"
#include "WPVehiclePhysics/Surface.hpp"
#include <Workphone/Core/XmlUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <iostream>

#if defined WP_PLATFORM_WIN32
#    include <Windows.h>
#elif defined SARACEN_PLATFORM_LINUX
#    include <X11/Xlib.h>
#    include <iostream>
#    include "X11/keysym.h"
#endif

constexpr short unsigned int Keyleft = 37;
constexpr short unsigned int Keytop = 38;
constexpr short unsigned int Keyright = 39;
constexpr short unsigned int Keydown = 40;
constexpr short unsigned int Keyexit = 81;

#ifdef __cplusplus
extern "C" {
#endif

using namespace workphone;

WP_INTERFACE_EXPORT void BootDLL();
WP_INTERFACE_EXPORT void ReadCurrentHeliParameters();
WP_INTERFACE_EXPORT void ReadDefaultParameterValues();
WP_INTERFACE_EXPORT void ResetAll();
WP_INTERFACE_EXPORT void MainFrame(f32 t, f32 dt);
WP_INTERFACE_EXPORT void WorkBenchFrame(f32 t, f32 dt, int ParamID, f32 ParamValue);
WP_INTERFACE_EXPORT void SetTeeter(f32 TeeterSpring, f32 TeeterDamping, f32 IndS);
WP_INTERFACE_EXPORT void SetFlybar(f32 R1, f32 R2, f32 C1, f32 C2, f32 Weight, f32 IntFac);
WP_INTERFACE_EXPORT void SetGroundEffect(f32 Max, f32 Decay);
WP_INTERFACE_EXPORT void SetCDMultiplier(f32 CDFactor, f32 Gash);
WP_INTERFACE_EXPORT void SetDataPath(void *str);
WP_INTERFACE_EXPORT void setFunc(void *FuncPtr);
WP_INTERFACE_EXPORT f32 HeadRPMToPy();
WP_INTERFACE_EXPORT f32 PackStateToPy();
WP_INTERFACE_EXPORT f32 PackVoltsToPy();
WP_INTERFACE_EXPORT f32 MotorPowerToPy();
WP_INTERFACE_EXPORT f32 EngineRPMToPy();
WP_INTERFACE_EXPORT f32 SoundRPMToPy();
WP_INTERFACE_EXPORT f32 MaxMainAttackToPy();
WP_INTERFACE_EXPORT f32 MaxTailAttackToPy();
WP_INTERFACE_EXPORT void GetModelMassProps(f32 &ModelMass, f32 &PMoI, f32 &RMoI, f32 &YMoI,
                                           f32 &HeadMass);
WP_INTERFACE_EXPORT void Levitate(f32 Time, f32 SpinTime, f32 SpinSpeed);
WP_INTERFACE_EXPORT void PassWind();
WP_INTERFACE_EXPORT void SetLinkageControl(bool LC);
// WP_INTERFACE_EXPORT void SetWind();
WP_INTERFACE_EXPORT void SetWeather(f32 speed, f32 direction, f32 turbulence, f32 groundHeight,
                                    f32 directionOffset, f32 fieldRoughness, f32 temperature,
                                    f32 pressure, f32 smallTurbulence);
WP_INTERFACE_EXPORT void SmokeFlow(f32 &RootSmokeFlowX, f32 &RootSmokeFlowY, f32 &RootSmokeFlowZ,
                                   f32 &TipSmokeFlowX, f32 &TipSmokeFlowY, f32 &TipSmokeFlowZ);
WP_INTERFACE_EXPORT void SoundData(f32 &TipPress1, f32 &TipPress2, f32 &TipPress3, f32 &TipPress4,
                                   f32 &TipDrg1, f32 &TipDrg2, f32 &TipDrg3, f32 &TipDrg4);
WP_INTERFACE_EXPORT void TestDisplay();
WP_INTERFACE_EXPORT void callFunc();
WP_INTERFACE_EXPORT void SetCurrentRPM(f32 currentRPM);
WP_INTERFACE_EXPORT void SetBailout(f32 bailoutValue, f32 bailoutValue2, int bailoutValue3);

WP_INTERFACE_EXPORT void SetCL(f32 value);
WP_INTERFACE_EXPORT void SetCD(f32 value);
WP_INTERFACE_EXPORT void SetNitroMultiplier(f32 value);

#ifdef __cplusplus
}
#endif

namespace workphone::vehicle
{
    std::shared_ptr<HeliAero> gHeliAero;
    constexpr double ThrowLimitScale = 0.015;

    void HeliAero::debugPrint()
    {
        // WP_LOG ( "Col+Ele : " + FloatToStrF ( ( 180 / Math<physics_Num>::pi() ) * (
        // m_rotorHead->m_mainRotor.m_collective + m_rotorHead->m_mainRotor.m_eleCyclic ), ffFixed,
        // 6, 1 ) );
        //  WP_LOG( "Col+Ail : " + FloatToStrF ( ( 180 / Math<physics_Num>::pi() ) * (
        //  m_rotorHead->m_mainRotor.m_collective + m_rotorHead->m_mainRotor.m_ailCyclic ), ffFixed,
        //  6, 1 ) ); WP_LOG( "TailPitch : " + FloatToStrF ( ( 180 / Math<physics_Num>::pi() ) * (
        //  m_tailRotor->m_collective ), ffFixed, 6, 1 ) );
    }

    void HeliAero::cageRotor(TRotor &rotor, FrameOfRef &shaftFrame, physics_Num angleLimit)
    {
        physics_Vec errorVector = VDif(
            rotor.m_frame.m_yAxis,
            shaftFrame.m_yAxis); // calculate the difference vector between shaft and rotor axis
        physics_Num errorMag = VMag(errorVector);
        if(errorMag > angleLimit)
        {
            // error exceeds limit so apply limit
            errorVector = VScale(errorVector, angleLimit / errorMag);
            // rescale ErrorVec to be in direction of unlimited error but of limiting length
            rotor.m_frame.m_yAxis = VSum(shaftFrame.m_yAxis, errorVector);
            // construct new VBar vector from the shaft vector and the limited error vector
            rotor.m_frame.m_zAxis = VUnit(
                VCross(m_body.m_frame.m_xAxis,
                       rotor.m_frame.m_yAxis)); // this is the forward pointing axis of the rotor
            rotor.m_frame.m_xAxis = VUnit(
                VCross(rotor.m_frame.m_yAxis,
                       rotor.m_frame.m_zAxis)); // this is the left pointing axis of the rotor
        }
    }

    void HeliAero::defineFlightParameters()
    {
        m_rotorHeadHubPosition.m_name = "RotorHeadHubPosition";
        m_rotorHeadHubPosition.m_vType = AVec;
        m_modelWeight.m_name = "ModelWeight";
        m_modelWeight.m_vType = ASingle;
        m_modelMoI.m_name = "ModelMoI";
        m_modelMoI.m_vType = AVec;
        m_mainRotorWeight.m_name = "MainRotorWeight";
        m_mainRotorWeight.m_vType = ASingle;
        m_rotorHeadBoltRadius.m_name = "RotorHeadBoltRadius";
        m_rotorHeadBoltRadius.m_vType = ASingle;
        m_rotorHeadNumBlades.m_name = "RotorHeadNumBlades";
        m_rotorHeadNumBlades.m_vType = AInt;
        m_rotorHeadSwashToMainMix.m_name = "RotorHeadSwashToMainMix";
        m_rotorHeadSwashToMainMix.m_vType = ASingle;
        m_rotorHeadSwashToFBMix.m_name = "RotorHeadSwashToFBMix";
        m_rotorHeadSwashToFBMix.m_vType = ASingle;
        m_rotorHeadFBtoMainMix.m_name = "RotorHeadFBtoMainMix";
        m_rotorHeadFBtoMainMix.m_vType = ASingle;
        m_rotorHeadMaxSwashEle.m_name = "RotorHeadMaxSwashEle";
        m_rotorHeadMaxSwashEle.m_vType = ASingle;
        m_rotorHeadMaxSwashAil.m_name = "RotorHeadMaxSwashAil";
        m_rotorHeadMaxSwashAil.m_vType = ASingle;
        m_rotorHeadCollectivePerMM.m_name = "EqMainPerMM";
        m_rotorHeadCollectivePerMM.m_vType = ASingle;
        m_rotorHeadShaftRake.m_name = "RotorHeadShaftRake";
        m_rotorHeadShaftRake.m_vType = ASingle;
        m_rotorHeadShaftTilt.m_name = "RotorHeadShaftTilt";
        m_rotorHeadShaftTilt.m_vType = ASingle;
        m_rotorHeadTeeterFC.m_name = "RotorHeadTeeterFC";
        m_rotorHeadTeeterFC.m_vType = ASingle;
        m_rotorHeadTeeterDC.m_name = "RotorHeadTeeterDC";
        m_rotorHeadTeeterDC.m_vType = ASingle;
        m_mainBladeLength.m_name = "MainBladeLength";
        m_mainBladeLength.m_vType = ASingle;
        m_mainBladeRootRadius.m_name = "MainBladeRootRadius";
        m_mainBladeRootRadius.m_vType = ASingle;
        m_mainBladeTipChord.m_name = "MainBladeTipChord";
        m_mainBladeTipChord.m_vType = ASingle;
        m_mainBladeRootChord.m_name = "MainBladeRootChord";
        m_mainBladeRootChord.m_vType = ASingle;
        m_mainBladeTwist.m_name = "MainBladeTwist";
        m_mainBladeTwist.m_vType = ASingle;
        m_mainBladeWeight.m_name = "MainBladeWeight";
        m_mainBladeWeight.m_vType = ASingle;
        m_mainBladeRadOfGyr.m_name = "MainBladeRadOfGyr";
        m_mainBladeRadOfGyr.m_vType = ASingle;
        m_tailHubPosition.m_name = "TailHubPosition";
        m_tailHubPosition.m_vType = AVec;
        m_tailHubBoltRadius.m_name = "TailHubBoltRadius";
        m_tailHubBoltRadius.m_vType = ASingle;
        m_tailNumBlades.m_name = "TailNumBlades";
        m_tailNumBlades.m_vType = AInt;
        m_tailMaxPitch.m_name = "TailMaxPitch";
        m_tailMaxPitch.m_vType = ASingle;
        m_tailPitchTrim.m_name = "TailPitchTrim";
        m_tailPitchTrim.m_vType = ASingle;
        m_tailBladeLength.m_name = "TailBladeLength";
        m_tailBladeLength.m_vType = ASingle;
        m_tailBladeRootRadius.m_name = "TailBladeRootRadius";
        m_tailBladeRootRadius.m_vType = ASingle;
        m_tailBladeTipChord.m_name = "TailBladeTipChord";
        m_tailBladeTipChord.m_vType = ASingle;
        m_tailBladeRootChord.m_name = "TailBladeRootChord";
        m_tailBladeRootChord.m_vType = ASingle;
        m_tailBladeTwist.m_name = "TailBladeTwist";
        m_tailBladeTwist.m_vType = ASingle;
        m_tailBladeWeight.m_name = "TailBladeWeight";
        m_tailBladeWeight.m_vType = ASingle;
        m_tailBladeRadOfGyr.m_name = "TailBladeRadOfGyr";
        m_tailBladeRadOfGyr.m_vType = ASingle;
        m_flybarless.m_name = "Flybarless";
        m_flybarless.m_vType = ABool;
        m_paddleWeight.m_name = "PaddleWeight";
        m_paddleWeight.m_vType = ASingle;
        m_paddleSpan.m_name = "PaddleSpan";
        m_paddleSpan.m_vType = ASingle;
        m_paddleRootChord.m_name = "PaddleRootChord";
        m_paddleRootChord.m_vType = ASingle;
        m_paddleTipChord.m_name = "PaddleTipChord";
        m_paddleTipChord.m_vType = ASingle;
        m_paddleThreadOnLength.m_name = "PaddleThreadOnLength";
        m_paddleThreadOnLength.m_vType = ASingle;
        m_flybarRodDiameter.m_name = "FlybarRodDiameter";
        m_flybarRodDiameter.m_vType = ASingle;
        m_flybarRodDensity.m_name = "FlybarRodDensity";
        m_flybarRodDensity.m_vType = ASingle;
        m_flybarRodLength.m_name = "FlybarRodLength";
        m_flybarRodLength.m_vType = ASingle;
        m_mainToFBInterference.m_name = "MainToFBInterference";
        m_mainToFBInterference.m_vType = ASingle;
        m_groundEffectMax.m_name = "GroundEffectMax";
        m_groundEffectMax.m_vType = ASingle;
        m_groundEffectDecay.m_name = "GroundEffectDecay";
        m_groundEffectDecay.m_vType = ASingle;
        m_cdMultiplier.m_name = "CDMultiplier";
        m_cdMultiplier.m_vType = ASingle;
        m_vrMultiplier.m_name = "VRMultiplier";
        m_vrMultiplier.m_vType = ASingle;
        m_mainGearTeeth.m_name = "MainGearTeeth";
        m_mainGearTeeth.m_vType = ASingle;
        m_pinionGearTeeth.m_name = "PinionGearTeeth";
        m_pinionGearTeeth.m_vType = ASingle;
        // EngineGearing.Name:= 'EngineGearing'; //replaced by teeth counts
        // EngineGearing.VType:=ASingle;         //
        m_tailGearing.m_name = "TailGearing";
        m_tailGearing.m_vType = ASingle;
        m_drivenTail.m_name = "DrivenTail";
        m_drivenTail.m_vType = ABool;
        m_bodyCdA.m_name = "BodyCdA";
        m_bodyCdA.m_vType = AVec;
        m_bodyDragCentre.m_name = "BodyDragCentre";
        m_bodyDragCentre.m_vType = AVec;
        // ClutchEngine parameters
        m_enginePeakPower.m_name = "PeakPower";
        m_enginePeakPower.m_vType = ASingle;
        m_enginePeakPowerRPM.m_name = "PeakPowerRPM";
        m_enginePeakPowerRPM.m_vType = ASingle;
        m_engineMoI.m_name = "EngineMoI";
        m_engineMoI.m_vType = ASingle;
        m_clutchMoI.m_name = "ClutchMoI";
        m_clutchMoI.m_vType = ASingle;
        m_clutchConst.m_name = "ClutchConst";
        m_clutchConst.m_vType = ASingle;
        m_clutchBiteRPM.m_name = "ClutchBiteRPM";
        m_clutchBiteRPM.m_vType = ASingle;
        // VBar parameters
        m_vBarStickDeadBand.m_name = "VBarStickDeadBand";
        m_vBarStickDeadBand.m_vType = ASingle;
        m_vBarStickSensitivity.m_name = "VBarStickSensitivity";
        m_vBarStickSensitivity.m_vType = ASingle;
        m_vBarStickExponential.m_name = "VBarStickExponential";
        m_vBarStickExponential.m_vType = ASingle;
        m_vBarAilGain.m_name = "VBarAilGain";
        m_vBarAilGain.m_vType = ASingle;
        m_vBarEleGain.m_name = "VBarEleGain";
        m_vBarEleGain.m_vType = ASingle;
        m_vBarAngleLimit.m_name = "VBarAngleLimit";
        m_vBarAngleLimit.m_vType = ASingle;
        m_vBarDecayTime.m_name = "VBarDecayTime";
        m_vBarDecayTime.m_vType = ASingle;
        m_vBarStabilize.m_name = "VBarStabilize";
        m_vBarStabilize.m_vType = ABool;
        m_vBarStabGain.m_name = "VBarStabGain";
        m_vBarStabGain.m_vType = ASingle;
        m_vBarDirectMix.m_name = "VBarDirectMix";
        m_vBarDirectMix.m_vType = ASingle;
        m_vBarAilStickFilter.m_name = "VBarAilStickFilter";
        m_vBarAilStickFilter.m_vType = ASingle;
        m_vBarEleStickFilter.m_name = "VBarEleStickFilter";
        m_vBarEleStickFilter.m_vType = ASingle;
        // electric power parameters
        m_electricPower.m_name = "ElectricPower";
        m_electricPower.m_vType = ABool;
        m_emulateBattery.m_name = "EmulateBattery";
        m_emulateBattery.m_vType = ABool;
        // Battery pack params
        m_cellsInPack.m_name = "CellsInPack";
        m_cellsInPack.m_vType = AInt;
        m_cellFullV.m_name = "CellFullV";
        m_cellFullV.m_vType = ASingle;
        m_cellFlatV.m_name = "CellFlatV";
        m_cellFlatV.m_vType = ASingle;
        m_cellR.m_name = "CellR";
        m_cellR.m_vType = ASingle;
        m_cellAHr.m_name = "CellAHr";
        m_cellAHr.m_vType = ASingle;
        // Electric motor parameters
        m_motorMoI.m_name = "MotorMoI";
        m_motorMoI.m_vType = ASingle;
        m_motorKV.m_name = "MotorKV";
        m_motorKV.m_vType = ASingle;
        m_motorEfficiency.m_name = "MotorEfficiency";
        m_motorEfficiency.m_vType = ASingle;
        m_motorNoLoadCurrent.m_name = "NoLoadCurrent";
        m_motorNoLoadCurrent.m_vType = ASingle;
        m_motorR.m_name = "MotorR";
        m_motorR.m_vType = ASingle;
        m_motorILimit.m_name = "MotorILimit";
        m_motorILimit.m_vType = ASingle;

        // ESC parameters
        m_escSlowRampTC.m_name = "ESCSlowRampTC";
        m_escSlowRampTC.m_vType = ASingle;
        m_escFastRampTC.m_name = "ESCFastRampTC";
        m_escFastRampTC.m_vType = ASingle;
        m_escSoftStartDelay.m_name = "ESCSoftStartDelay";
        m_escSoftStartDelay.m_vType = ASingle;
        m_escAccelerationGain.m_name = "ESCAccelerationGain";
        m_escAccelerationGain.m_vType = ASingle;
        m_escRPMGain.m_name = "ESCRPMGain";
        m_escRPMGain.m_vType = ASingle;
        m_escPhaseGain.m_name = "ESCPhaseGain";
        m_escPhaseGain.m_vType = ASingle;
        m_escAccelerationLimit.m_name = "ESCAccelerationLimit";
        m_escAccelerationLimit.m_vType = ASingle;
        m_escRPMErrorLimit.m_name = "ESCRPMErrorLimit";
        m_escRPMErrorLimit.m_vType = ASingle;
        m_escPhaseErrorLimit.m_name = "ESCPhaseErrorLimit";
        m_escPhaseErrorLimit.m_vType = ASingle;
        m_escMinControlPoint.m_name = "ESCMinControlPoint";
        m_escMinControlPoint.m_vType = ASingle;
        m_escILimit.m_name = "ESCILimit";
        m_escILimit.m_vType = ASingle;
        m_escCutoffV.m_name = "ESCCutoffV";
        m_escCutoffV.m_vType = ASingle;
        m_escResistance.m_name = "ESCResistance";
        m_escResistance.m_vType = ASingle;
        m_govMode.m_name = "GovMode";
        m_govMode.m_vType = AInt;
        m_govReqHeadRPM.m_name =
            "GovReqHeadRPM"; // note this Gov param used also for electric flight setup
        m_govReqHeadRPM.m_vType = ASingle;
        m_govMinHeadRPM.m_name = "GovMinHeadRPM";
        m_govMinHeadRPM.m_vType = ASingle;
        m_govMaxHeadRPM.m_name = "GovMaxHeadRPM";
        m_govMaxHeadRPM.m_vType = ASingle;
        m_visualTailReverse.m_name = "VisualTailReverse";
        m_visualTailReverse.m_vType = ABool;
        m_directTailControl.m_name = "DirectTailControl";
        m_directTailControl.m_vType = ABool;
        m_bearingFriction.m_name = "BearingFriction";
        m_bearingFriction.m_vType = ASingle;
    }

    void HeliAero::findAllParameterValuesIn(const String &LCPS)
    {
        WP_LOG("FBHeliAero data : \n" + LCPS);

        real_dNum mbLength, mbMass, mbCofG;
        real_dNum tbLength, tbMass, tbCofG;
        real_dNum fbMass, fbEffectiveMass;

        // Parameter extraction
        AerodymanicsUtil::findValueOf(m_rotorHeadHubPosition, LCPS);
        AerodymanicsUtil::findValueOf(m_modelWeight, LCPS);
        AerodymanicsUtil::findValueOf(m_modelMoI, LCPS);
        AerodymanicsUtil::findValueOf(m_mainRotorWeight, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadBoltRadius, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadNumBlades, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadSwashToMainMix, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadSwashToFBMix, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadFBtoMainMix, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadMaxSwashEle, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadMaxSwashAil, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadCollectivePerMM, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadShaftRake, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadShaftTilt, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadTeeterFC, LCPS);
        AerodymanicsUtil::findValueOf(m_rotorHeadTeeterDC, LCPS);
        AerodymanicsUtil::findValueOf(m_mainBladeLength, LCPS);
        AerodymanicsUtil::findValueOf(m_mainBladeRootRadius, LCPS);
        AerodymanicsUtil::findValueOf(m_mainBladeTipChord, LCPS);
        AerodymanicsUtil::findValueOf(m_mainBladeRootChord, LCPS);
        AerodymanicsUtil::findValueOf(m_mainBladeTwist, LCPS);
        AerodymanicsUtil::findValueOf(m_mainBladeWeight, LCPS);
        AerodymanicsUtil::findValueOf(m_mainBladeRadOfGyr, LCPS);
        AerodymanicsUtil::findValueOf(m_tailHubPosition, LCPS);
        AerodymanicsUtil::findValueOf(m_tailHubBoltRadius, LCPS);
        AerodymanicsUtil::findValueOf(m_tailNumBlades, LCPS);
        AerodymanicsUtil::findValueOf(m_tailMaxPitch, LCPS);
        AerodymanicsUtil::findValueOf(m_tailPitchTrim, LCPS);
        AerodymanicsUtil::findValueOf(m_tailBladeLength, LCPS);
        AerodymanicsUtil::findValueOf(m_tailBladeRootRadius, LCPS);
        AerodymanicsUtil::findValueOf(m_tailBladeTipChord, LCPS);
        AerodymanicsUtil::findValueOf(m_tailBladeRootChord, LCPS);
        AerodymanicsUtil::findValueOf(m_tailBladeTwist, LCPS);
        AerodymanicsUtil::findValueOf(m_tailBladeWeight, LCPS);
        AerodymanicsUtil::findValueOf(m_tailBladeRadOfGyr, LCPS);
        AerodymanicsUtil::findValueOf(m_flybarless, LCPS);
        AerodymanicsUtil::findValueOf(m_paddleWeight, LCPS);
        AerodymanicsUtil::findValueOf(m_paddleSpan, LCPS);
        AerodymanicsUtil::findValueOf(m_paddleRootChord, LCPS);
        AerodymanicsUtil::findValueOf(m_paddleTipChord, LCPS);
        AerodymanicsUtil::findValueOf(m_paddleThreadOnLength, LCPS);
        AerodymanicsUtil::findValueOf(m_flybarRodDiameter, LCPS);
        AerodymanicsUtil::findValueOf(m_flybarRodDensity, LCPS);
        AerodymanicsUtil::findValueOf(m_flybarRodLength, LCPS);
        AerodymanicsUtil::findValueOf(m_mainToFBInterference, LCPS);
        m_interferenceFactor = m_mainToFBInterference.m_sVal;
        AerodymanicsUtil::findValueOf(m_groundEffectMax, LCPS);
        m_geMax = m_groundEffectMax.m_sVal;
        AerodymanicsUtil::findValueOf(m_groundEffectDecay, LCPS);
        m_geDecay = m_groundEffectDecay.m_sVal;
        AerodymanicsUtil::findValueOf(m_vrMultiplier, LCPS);
        AerodymanicsUtil::findValueOf(m_mainGearTeeth, LCPS);
        AerodymanicsUtil::findValueOf(m_pinionGearTeeth, LCPS);
        AerodymanicsUtil::findValueOf(m_tailGearing, LCPS);
        AerodymanicsUtil::findValueOf(m_drivenTail, LCPS);
        AerodymanicsUtil::findValueOf(m_bodyCdA, LCPS);
        m_body.m_cdA = m_bodyCdA.m_vVal;
        AerodymanicsUtil::findValueOf(m_bodyDragCentre, LCPS);
        m_body.m_dragCentre = m_bodyDragCentre.m_vVal;
        AerodymanicsUtil::findValueOf(m_enginePeakPower, LCPS);

        EngineClutchUnit &engineClutch = *m_theEngineClutch;
        engineClutch.m_peakPower = m_enginePeakPower.m_sVal;
        AerodymanicsUtil::findValueOf(m_enginePeakPowerRPM, LCPS);
        engineClutch.m_peakPowerRPM = m_enginePeakPowerRPM.m_sVal;
        AerodymanicsUtil::findValueOf(m_engineMoI, LCPS);
        engineClutch.m_moiEngine = m_engineMoI.m_sVal;
        AerodymanicsUtil::findValueOf(m_clutchMoI, LCPS);
        engineClutch.m_moiClutch = m_clutchMoI.m_sVal;
        AerodymanicsUtil::findValueOf(m_clutchConst, LCPS);
        engineClutch.m_clutchConst = m_clutchConst.m_sVal;
        AerodymanicsUtil::findValueOf(m_clutchBiteRPM, LCPS);
        engineClutch.m_biteRPM = m_clutchBiteRPM.m_sVal;
        engineClutch.m_biteRPMSq = engineClutch.m_biteRPM * engineClutch.m_biteRPM;

        // VBar parameters
        AerodymanicsUtil::findValueOf(m_vBarStickDeadBand, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarStickSensitivity, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarStickExponential, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarAilGain, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarEleGain, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarAngleLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarDecayTime, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarStabilize, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarStabGain, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarDirectMix, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarAilStickFilter, LCPS);
        AerodymanicsUtil::findValueOf(m_vBarEleStickFilter, LCPS);

        // Assign VBar parameters to the instance (using new member names)
        CFlybarlessUnit &vbar = *m_theVBar;
        vbar.m_stickDeadBand = static_cast<physics_Num>(0.02) * m_vBarStickDeadBand.m_sVal;
        vbar.m_stickSensitivity =
            static_cast<physics_Num>(0.01745) * m_vBarStickSensitivity.m_sVal;
        vbar.m_stickExpo = m_vBarStickExponential.m_sVal;
        vbar.m_rollGain = static_cast<physics_Num>(5.0) * m_vBarAilGain.m_sVal;
        vbar.m_pitchGain = static_cast<physics_Num>(5.0) * m_vBarEleGain.m_sVal;
        vbar.m_angleLimit = m_vBarAngleLimit.m_sVal;
        vbar.m_decay = m_vBarDecayTime.m_sVal;
        vbar.m_stabilize = m_vBarStabilize.m_bVal;
        vbar.m_stabGain = static_cast<physics_Num>(4.0) * m_vBarStabGain.m_sVal;
        vbar.m_directMix = static_cast<physics_Num>(1.0) * m_vBarDirectMix.m_sVal;
        vbar.m_ailFilter = static_cast<physics_Num>(0.2) * m_vBarAilStickFilter.m_sVal;
        vbar.m_eleFilter = static_cast<physics_Num>(0.2) * m_vBarEleStickFilter.m_sVal;

        // Electric power parameters
        AerodymanicsUtil::findValueOf(m_electricPower, LCPS);
        AerodymanicsUtil::findValueOf(m_emulateBattery, LCPS);
        AerodymanicsUtil::findValueOf(m_cellsInPack, LCPS);
        AerodymanicsUtil::findValueOf(m_cellFullV, LCPS);
        AerodymanicsUtil::findValueOf(m_cellFlatV, LCPS);
        AerodymanicsUtil::findValueOf(m_cellR, LCPS);
        AerodymanicsUtil::findValueOf(m_cellAHr, LCPS);
        AerodymanicsUtil::findValueOf(m_motorMoI, LCPS);
        AerodymanicsUtil::findValueOf(m_motorKV, LCPS);
        AerodymanicsUtil::findValueOf(m_motorEfficiency, LCPS);
        AerodymanicsUtil::findValueOf(m_motorNoLoadCurrent, LCPS);
        AerodymanicsUtil::findValueOf(m_motorR, LCPS);
        AerodymanicsUtil::findValueOf(m_motorILimit, LCPS);
        AerodymanicsUtil::findValueOf(m_escSlowRampTC, LCPS);
        AerodymanicsUtil::findValueOf(m_escFastRampTC, LCPS);
        AerodymanicsUtil::findValueOf(m_escSoftStartDelay, LCPS);
        AerodymanicsUtil::findValueOf(m_escAccelerationGain, LCPS);
        AerodymanicsUtil::findValueOf(m_escRPMGain, LCPS);
        AerodymanicsUtil::findValueOf(m_escPhaseGain, LCPS);
        AerodymanicsUtil::findValueOf(m_escAccelerationLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_escRPMErrorLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_escPhaseErrorLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_escMinControlPoint, LCPS);
        AerodymanicsUtil::findValueOf(m_escILimit, LCPS);
        AerodymanicsUtil::findValueOf(m_escCutoffV, LCPS);
        AerodymanicsUtil::findValueOf(m_escResistance, LCPS);
        AerodymanicsUtil::findValueOf(m_govMode, LCPS);
        AerodymanicsUtil::findValueOf(m_govReqHeadRPM, LCPS);
        AerodymanicsUtil::findValueOf(m_govMinHeadRPM, LCPS);
        AerodymanicsUtil::findValueOf(m_govMaxHeadRPM, LCPS);
        m_modelIsElectric = m_electricPower.m_bVal; // set power to electric as required
        AerodymanicsUtil::findValueOf(m_visualTailReverse, LCPS);
        AerodymanicsUtil::findValueOf(m_directTailControl, LCPS);
        AerodymanicsUtil::findValueOf(m_bearingFriction, LCPS);

        if(m_modelIsElectric)
        {
            m_pack->m_cellsInPack = m_cellsInPack.m_iVal;
            m_pack->m_cellFullV = m_cellFullV.m_sVal;
            m_pack->m_cellFlatV = m_cellFlatV.m_sVal;
            m_pack->m_cellR = m_cellR.m_sVal;
            m_pack->m_cellAHr = m_cellAHr.m_sVal;
            m_motor->m_motorMoI = m_motorMoI.m_sVal;
            m_motor->m_motorKv = m_motorKV.m_sVal;
            m_motor->m_motorEfficiency = m_motorEfficiency.m_sVal;
            m_motor->m_noLoadCurrent = m_motorNoLoadCurrent.m_sVal;
            m_motor->m_motorR = m_motorR.m_sVal;
            m_motor->m_currentLimit = m_motorILimit.m_sVal;
            m_esc->m_mode = m_govMode.m_iVal;
            m_esc->m_slowRampTc = m_escSlowRampTC.m_sVal;
            m_esc->m_fastRampTc = m_escFastRampTC.m_sVal;
            m_esc->m_softStartDelay = m_escSoftStartDelay.m_sVal;
            m_esc->m_accGain = m_escAccelerationGain.m_sVal;
            m_esc->m_rpmGain = m_escRPMGain.m_sVal;
            m_esc->m_phaseGain = m_escPhaseGain.m_sVal;
            m_esc->m_accLimit = m_escAccelerationLimit.m_sVal;
            m_esc->m_rpmErrorLimit = m_escRPMErrorLimit.m_sVal;
            m_esc->m_phaseErrorLimit = m_escPhaseErrorLimit.m_sVal;
            m_esc->m_minControlPoint = m_escMinControlPoint.m_sVal;
            // m_esc->m_maxControlPoint=1; //make sure the MaxControl Point is also set
            m_esc->m_iLimit = m_escILimit.m_sVal;
            m_esc->m_cutoffV = m_escCutoffV.m_sVal;
            m_esc->m_resistance = m_escResistance.m_sVal;
            m_esc->m_fixedRpm =
                m_govReqHeadRPM.m_sVal * m_mainGearTeeth.m_sVal /
                m_pinionGearTeeth.m_sVal; // calculate the mode 1 (fixed) required engine rpm
            m_esc->m_rpmRangeBottom = m_govMinHeadRPM.m_sVal * m_mainGearTeeth.m_sVal /
                                      m_pinionGearTeeth.m_sVal; // calc the lowest remote engine rpm
            m_esc->m_rpmRangeTop = m_govMaxHeadRPM.m_sVal * m_mainGearTeeth.m_sVal /
                                   m_pinionGearTeeth.m_sVal; // calc the highest remote engine rpm
        } // end of if model is electric

        // now allocate the parameter values into the required values in the dll
        m_rotorHead->m_mainRotor.m_hubPosition = m_rotorHeadHubPosition.m_vVal;
        m_rotorHead->m_flyBar.m_hubPosition = m_rotorHeadHubPosition.m_vVal;
        m_rotorHead->m_shaftRake = m_rotorHeadShaftRake.m_sVal;
        m_rotorHead->m_shaftTilt = m_rotorHeadShaftTilt.m_sVal;
        m_rotorHead->m_mainRotor.m_blades = m_rotorHeadNumBlades.m_iVal;
        m_rotorHead->m_mainRotor.m_minRad =
            m_rotorHeadBoltRadius.m_sVal + m_mainBladeRootRadius.m_sVal;
        m_rotorHead->m_mainRotor.m_maxRad = m_rotorHeadBoltRadius.m_sVal + m_mainBladeLength.m_sVal;
        m_rotorHead->m_mainRotor.m_cuffChord = m_mainBladeRootChord.m_sVal;
        m_rotorHead->m_mainRotor.m_tipChord = m_mainBladeTipChord.m_sVal;
        m_rotorHead->m_mainRotor.m_twist = m_mainBladeTwist.m_sVal;
        m_rotorHead->m_mainRotor.m_bladeWeight = m_mainBladeWeight.m_sVal;
        m_rotorHead->m_linkage.m_swashToMainMix =
            m_rotorHeadSwashToMainMix.m_sVal; // 0.25 based on measurement on trex 600
        m_rotorHead->m_linkage.m_swashToFBMix =
            m_rotorHeadSwashToFBMix.m_sVal; // 1 based on trex 600 set measurements
        m_rotorHead->m_linkage.m_fbToMainMix =
            m_rotorHeadFBtoMainMix.m_sVal; // 0.86 based on trex 600 measurements

        // note: the following two are no longer used now linkage control is being used
        m_rotorHead->m_linkage.m_maxSwashEle = m_rotorHeadMaxSwashEle.m_sVal *
                                               Math<real_dNum>::pi() /
                                               180.0; // note conversion to radians here
        m_rotorHead->m_linkage.m_maxSwashAil =
            m_rotorHeadMaxSwashAil.m_sVal * Math<real_dNum>::pi() / 180.0;
        m_rotorHead->m_linkage.m_collectivePerMM =
            m_rotorHeadCollectivePerMM.m_sVal * Math<real_dNum>::pi() / 180.0;
        m_rotorHead->m_teeterForceConstant =
            m_rotorHeadTeeterFC.m_sVal; // teeter stiffness that requires an experiment to refine
        m_rotorHead->m_teeterDampingConstant =
            m_rotorHeadTeeterDC
            .m_sVal; // even worse guesstimate requiring an even more fiddly experiment
        m_tailRotor->m_hubPosition = m_tailHubPosition.m_vVal;
        m_tailRotor->m_blades = m_tailNumBlades.m_iVal;
        m_tailRotor->m_minRad = m_tailHubBoltRadius.m_sVal + m_tailBladeRootRadius.m_sVal;
        m_tailRotor->m_maxRad = m_tailHubBoltRadius.m_sVal + m_tailBladeLength.m_sVal;
        m_tailRotor->m_cuffChord = m_tailBladeRootChord.m_sVal;
        m_tailRotor->m_tipChord = m_tailBladeTipChord.m_sVal;
        m_tailRotor->m_twist = m_tailBladeTwist.m_sVal;
        m_tailRotor->m_bladeWeight = m_tailBladeWeight.m_sVal;
        m_rotorHead->m_mainRotor.m_momentOfInertia =
            m_rotorHead->m_mainRotor.m_blades * m_rotorHead->m_mainRotor.m_bladeWeight *
            Math<real_dNum>::Sqr(
                m_rotorHead->m_mainRotor.m_radOfGyr); // moment of inertia of rotor about hub centre

        if(m_rotorHead->m_hasFlyBar ==
           false) // if the model is flybarless we must zero linkage ratios etc
        {
            m_rotorHead->m_linkage.m_swashToFBMix =
                static_cast<real_dNum>(0); // make sure the cyclic linkage to the rotor is zero
            m_rotorHead->m_linkage.m_fbToMainMix =
                static_cast<real_dNum>(0); // make sure the flybar to main blade mix is zero
            AerodymanicsUtil::initVBar(*m_theVBar); // init the VBar and get setup from XML file
        } // end of flybarless zeroing of parameters

        m_tailRotor->m_hubPosition.z = m_tailRotor->m_hubPosition.z * 2.0;
    }

    /* export */
    // forces the dll to read in the default heli parameters
    void HeliAero::readDefaultParameterValues()
    {
        try
        {
            WP_DEBUG_TRACE;

            defineFlightParameters();

#if defined WP_PLATFORM_WIN32
            StringW filePath = L"DefaultHeliParams.xml";
#else
            StringW filePath = getDataPath() + L"/DefaultHeliParams.xml";
            filePath = StringUtilW::replaceAll(filePath, L"//", L"/");
#endif

            auto message = "FBHeliAero opening : " + StringUtil::toUTF16to8(filePath);
            WP_LOG(message);

            auto xmlData = XmlUtil::getFromFile(filePath);
            auto LCPS = StringUtil::make_lower(xmlData);
            findAllParameterValuesIn(LCPS);
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void HeliAero::readCurrentHeliParameters() /* export */
    {
        try
        {
            WP_DEBUG_TRACE;

            // DefineFlightParameters();
            // WP_LOG( "FBHeliAero working dir : " + FileSystem::getWorkingDirectory() );
            WP_LOG("FBHeliAero opening : " +
                StringUtil::toUTF16to8( getDataPath() + L"model_data.xml" ));

            auto filePath = getDataPath() + L"/model_data.xml";
            filePath = StringUtilW::replaceAll(filePath, L"//", L"/");
            WP_LOG(filePath);
            auto xmlData = XmlUtil::getFromFile(filePath);
            WP_LOG(xmlData);

            auto LCPS = StringUtil::make_lower(xmlData);
            findAllParameterValuesIn(LCPS);
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    // receives the pointer to the Python "callback" routine and allocates it to Py_func
    void HeliAero::setFunc(void *FuncPtr)
    {
        WP_DEBUG_TRACE;

        m_fnCallback = reinterpret_cast<TProcedurePtr>(FuncPtr);
        // ShowMessage('setFunc called in delphi DLL') ;
        WP_LOG("setFunc called in aerodynamics dll");
    } // setFunc

    // allows Python to trigger a test callback
    void HeliAero::callFunc()
    {
        m_cbFun = 98; // sets the func and subfunc of the callback to unused values
        m_cbSub = 99; //
        m_va[0] = 0.11; // loads distinguishing data into the data exchange array VA
        m_va[1] = 1;
        m_va[2] = 2;
        m_va[3] = 3;
        m_va[4] = 4;
        m_va[5] = 5;
        m_tp = m_va; // makes sure the pointer to VA is initialised correctly
        callback(m_cbFun, m_cbSub, m_tp); // this calls the python "callback" routine
    } // callFunc

    // allows python to set the teerer stiffness, springing and the induced smoothing
    void HeliAero::setTeeter(f32 TeeterSpring, f32 TeeterDamping, f32 IndS)
    {
        WP_DEBUG_TRACE;

        m_rotorHead->m_teeterForceConstant = TeeterSpring; // use the passed teeter spring value
        m_rotorHead->m_teeterDampingConstant = TeeterDamping; // use the passed teeter damping value
        m_inducedSmoothing = IndS; // put the smoothing factor in the global var
    } // end of SetTeeter

    void HeliAero::setFlybar(f32 R1, f32 R2, f32 C1, f32 C2, f32 Weight,
                             f32 IntFac) /* export */ // allows python to set paddle weight
    {
        WP_DEBUG_TRACE;

        m_rotorHead->m_flyBar.m_minRad = R1;
        m_rotorHead->m_flyBar.m_maxRad = R2;
        m_rotorHead->m_flyBar.m_cuffChord = C1;
        m_rotorHead->m_flyBar.m_tipChord = C2;
        m_rotorHead->m_flyBar.m_bladeWeight = Weight; // transfer value to flybar
        m_interferenceFactor = IntFac; // load the flybar interference factor to its global
        m_rotorHead->m_flyBar.m_radOfGyr = 0.5 * (R1 + R2);
        initSurfaces(m_rotorHead->m_flyBar);
        m_rotorHead->m_flyBar.m_momentOfInertia =
            m_rotorHead->m_flyBar.m_blades * m_rotorHead->m_flyBar.m_bladeWeight *
            Math<real_dNum>::Sqr(
                m_rotorHead->m_flyBar.m_radOfGyr); // moment of inertia of rotor about hub centre
    }

    void HeliAero::setGroundEffect(
        f32 Max, f32 Decay) /* export */ // allows python to set ground effect characteristics
    {
        WP_DEBUG_TRACE;

        m_geMax = Max; // pass the max ground effect (typically 1)
        m_geDecay = Decay; // pass the decay rate vs height (typically 2)
    } // SetGroundEffect

    void HeliAero::setLinkageControl(
        bool LC) /* export */ // allows control to be switched from Tx to linkage angles
    {
        WP_DEBUG_TRACE;

        if(LC)
            m_linkageControl = true;
        else
            m_linkageControl = false;
    } // SetLinkageControl

    void HeliAero::gateForces(
        int BF, int RF) /* export */ // allows control to be switched from Tx to linkage angles
    {
        WP_DEBUG_TRACE;

        if(BF != 0)
            m_bodyForcesOn = true;
        else
            m_bodyForcesOn = false;

        if(RF != 0)
            m_rotorForcesOn = true;
        else
            m_rotorForcesOn = false;
    } // GateForces

    void HeliAero::setCDMultiplier(f32 CDFactor, f32 Gash) /* export */
    {
        WP_DEBUG_TRACE;

        m_cdFiddle = CDFactor;
    }

    // use callback to get the yaw channel (channel 3)
    void HeliAero::getYawChannel()
    {
        m_cbFun = GET_TX_CHANNEL; // Tx read
        m_cbSub = YAW_CHANNEL; // channel 3
        m_tp = m_va; // ensure TP pointer points to VA array
        callback(m_cbFun, m_cbSub, m_tp); // generate callback
        m_txChannel[YAW_CHANNEL] = m_va[0]; // pass the value to TxChannel[3]
    }

    // note Servos for ele,ail, and col added in here
    void HeliAero::getAllTxData()
    {
        int Ch;
        try
        {
            m_tp = m_va; // ensure TP pointer points to VA array
            for(Ch = 0; Ch != 7; Ch++)
            {
                callback(GET_TX_CHANNEL, Ch, m_tp); // generate callback for channel Ch
                m_txChannel[Ch] = m_va[0]; // pass the value to TxChannel[Ch]
            } // for Ch loop
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    } // GetAllTxData

    void HeliAero::driveTailServo(physics_Num Deviation)
    {
        try
        {
            m_tp = m_va; // ensure TP pointer points to VA array
            m_va[0] = Deviation; // pass the deflection value to the Pass array
            callback(SET_SERVO_INPUT, YAW_CHANNEL, m_tp); // generate callback for channel
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    void HeliAero::sendFlybarlessControlsToMixer(physics_Num AilDeflection,
                                                 physics_Num EleDeflection)
    {
        try
        {
            m_tp = m_va; // ensure TP pointer points to VA array

            m_va[0] = EleDeflection; // pass the elevator servo deflection
            m_va[1] = AilDeflection; // pass the aileron servo command
            m_va[2] = static_cast<f32>(0.0);
            m_va[3] = static_cast<f32>(0.0);

            callback(FLYBAR_CTRL_UPDATE, CB_MODEL, m_tp); // generate callback
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    // this provides the swashplate cyclic angles and the collective linear displacement from which
    // we get collective pitch angle
    void HeliAero::getControlInfo()
    {
        try
        {
            m_tp = m_va; // ensure TP pointer points to VA array
            callback(GET_HELI_CTRL_INFO, CB_MODEL, m_tp); // generate callback for control info

            m_rotorHead->m_mainRotor.m_collective = 1000.0 * static_cast<real_dNum>(m_va[0]) *
                                                    m_rotorHead->m_linkage.m_collectivePerMM;

            // use the collective/mm and the displacement of the swash (in metres) to get collective
            m_rotorHead->m_linkage.m_swashEleAngle =
                -m_va[1]; //   NOTE: Elevator first  NOTE also the sign
            m_rotorHead->m_linkage.m_swashAilAngle = m_va[2]; //  NOTE: THINK ABOUT SIGN CONVENTION

            if(m_visualTailReverse.m_bVal)
            {
                m_tailRotor->m_collective = m_va[3];
            }
            else
            {
                m_tailRotor->m_collective =
                    -m_va[3]; // NOTE: SIGN reversed to deal with getting visual tail pitch right
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    } // GetControlInfo

    // uses the single callback to return all the Tx Channels
    void HeliAero::getTxChannels()
    {
        int Ch;
        try
        {
            m_tp = m_va; // ensure TP pointer points to VA array
            callback(GET_TX_CHANNELS, CB_MODEL, m_tp); // generate callback for Tx channel info
            for(Ch = 0; Ch != 7; Ch++)
            {
                m_txChannel[Ch] = m_va[Ch]; // pass all the values to the corresponding TxChannel[Ch]
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    } // GetTxChannels

    // uses callback to recover engine o/p from external engine dll
    // VA[0] = throttle position, VA[1] = current RPM. Returns engine output in VA[0]
    void HeliAero::getEngineOutput()
    {
        try
        {
            m_tp = m_va; // ensure TP pointer points to VA array
            m_va[0] = m_engineThrottlePosition; // pass the throttle signal
            m_va[1] = m_engineRPM; // pass the required engine RPM
            callback(GET_ENGINE_OUTPUT, CB_MODEL, m_tp); // generate callback for control angles
            m_engineOutput = m_va[0]; // get the returned engine output
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    } // GetEngineOutput;

    // gets the angular velocity of the model
    void HeliAero::getModelAngularVelocity()
    {
        m_tp = m_va; // ensure TP pointer points to VA array
        callback(GET_ANGULAR_VELOCITY, CB_MODEL,
                 m_tp); // generate callback for  Angular velocity and the MODEL
        m_modelAngularVelocity.x = m_va[0];
        m_modelAngularVelocity.y = m_va[1];
        m_modelAngularVelocity.z = m_va[2]; // transfer the angular velocity to global var
    } // GetAngularVelocity

    // gets the linear velocity of the model
    void HeliAero::getModelLinearVelocity()
    {
        m_tp = m_va; // ensure TP pointer points to VA array
        callback(GET_LINEAR_VELOCITY, CB_MODEL, m_tp);
        // generate callback for  linear velocity and the MODEL NOTE - IN ITS FRAME OF REFERENCE!!!
        m_modelVelocity.x = m_va[0];
        m_modelVelocity.y = m_va[1];
        m_modelVelocity.z = m_va[2]; // transfer the velocity to a global
    }

    // adds a force to the body 'Bdy' at location Loc in its frame
    void HeliAero::addLocalForce(int Bdy, const physics_Vec &Force, const physics_Vec &Loc,
                                 f32 ForceID)
    {
        if(Force.isFinite() && Loc.isFinite())
        {
            m_tp = m_va; // ensure TP pointer points to VA array
            m_va[0] = static_cast<f32>(Force.x);
            m_va[1] = static_cast<f32>(Force.y);
            m_va[2] = static_cast<f32>(Force.z); // load the Force values into the data pass array
            m_va[3] = static_cast<f32>(Loc.x);
            m_va[4] = static_cast<f32>(Loc.y);
            m_va[5] = static_cast<f32>(Loc.z); // load the location values into the data pass array
            m_va[6] = ForceID; // put the force ident into VA[6];
            callback(ADD_LOCAL_FORCE, Bdy, m_tp); // generate callback as required
        }
    }

    // adds a torque to the body 'Bdy' at location Loc in its frame
    void HeliAero::addLocalTorque(int Bdy, const physics_Vec &Torque)
    {
        m_tp = m_va; // ensure TP pointer points to VA array
        m_va[0] = static_cast<f32>(Torque.x);
        m_va[1] = static_cast<f32>(Torque.y);
        m_va[2] = static_cast<f32>(Torque.z); // load the Force values into the data pass array
        callback(ADD_LOCAL_TORQUE, Bdy, m_tp); // generate callback as required
    }

    // displays the local vector V with origin at Org
    void HeliAero::displayLocalVector(int Bdy, const physics_Vec &V, const physics_Vec &Org)
    {
        try
        {
            m_va[0] = static_cast<f32>(V.x);
            m_va[1] = static_cast<f32>(V.y);
            m_va[2] = static_cast<f32>(V.z); // load the pass array with the vector coords
            m_va[3] = static_cast<f32>(Org.x);
            m_va[4] = static_cast<f32>(Org.y);
            m_va[5] = static_cast<f32>(Org.z); // load the origin into pass array
            m_cbFun = DISPLAY_LOCAL_VECTOR;
            m_cbSub = Bdy; // set the reference to model or rotor hed as needed
            m_tp = m_va; // ensure TP points to the data passing array VA
            callback(m_cbFun, m_cbSub, m_tp); // generate callback to Python
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    } // DisplayLocalVector

    void HeliAero::callback(int N1, int N2, TPassAPtr TP)
    {
        WP_ASSERT(Math<physics_Num>::isFinite( TP[0] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[1] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[2] ));

        WP_ASSERT(Math<physics_Num>::isFinite( TP[3] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[4] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[5] ));

        WP_ASSERT(Math<physics_Num>::isFinite( TP[6] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[7] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[8] ));

        WP_ASSERT(Math<physics_Num>::isFinite( TP[9] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[10] ));

        callback(N1, N2, TP);

        WP_ASSERT(Math<physics_Num>::isFinite( TP[0] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[1] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[2] ));

        WP_ASSERT(Math<physics_Num>::isFinite( TP[3] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[4] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[5] ));

        WP_ASSERT(Math<physics_Num>::isFinite( TP[6] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[7] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[8] ));

        WP_ASSERT(Math<physics_Num>::isFinite( TP[9] ));
        WP_ASSERT(Math<physics_Num>::isFinite( TP[10] ));
    }

    // gets the position of the model
    void HeliAero::getModelPosition()
    {
        m_tp = m_va; // ensure TP pointer points to VA array
        callback(GET_GLOBAL_POSITION, CB_MODEL, m_tp); // generate callback for  the model position
        m_modelPosition.x = m_va[0];
        m_modelPosition.y = m_va[1];
        m_modelPosition.z = m_va[2]; // transfer the velocity to a global

        WP_ASSERT(m_modelPosition.isFinite());
    }

    // gets the Frame of Reference of the model in world frame
    void HeliAero::getModelFrame()
    {
        physics_Vec XA, YA, ZA; // temps for frame vectors
        m_tp = m_va; // ensure TP pointer points to VA array
        m_va[0] = 0;
        m_va[1] = 1;
        m_va[2] = 0; // set up for the 'Y' axis of the heli
        callback(GET_GLOBAL_ORIENTATION, CB_MODEL, m_tp); // do the callback
        YA.x = m_va[0];
        YA.y = m_va[1];
        YA.z = m_va[2]; // retreve the X axis info from the pass array
        m_va[0] = 0;
        m_va[1] = 0;
        m_va[2] = 1; // set up for the 'Z' axis of the heli
        callback(GET_GLOBAL_ORIENTATION, CB_MODEL, m_tp); // do the callback
        ZA.x = m_va[0];
        ZA.y = m_va[1];
        ZA.z = m_va[2]; // retreve the X axis info from the pass array
        XA = VUnit(VCross(YA, ZA)); // generate the X axis as the cross product of y and z
        // just in case of a slight move between the y and z axis 'readings' we re-create the Y axis
        YA = VCross(ZA, XA);
        m_modelFrame.m_xAxis = XA;
        m_modelFrame.m_yAxis = YA;
        m_modelFrame.m_zAxis = ZA; // transfer result to a global

        WP_ASSERT(m_modelFrame.m_xAxis.isFinite());
        WP_ASSERT(m_modelFrame.m_yAxis.isFinite());
        WP_ASSERT(m_modelFrame.m_zAxis.isFinite());
    }

    // gets the Frame of Reference of the visual rotor in world frame
    void HeliAero::getVisualRotorFrame()
    {
        physics_Vec XA, YA, ZA; // temps for frame vectors
        m_tp = m_va; // ensure TP pointer points to VA array
        m_va[0] = 0;
        m_va[1] = 1;
        m_va[2] = 0; // set up for the 'Y' axis of the heli
        callback(GET_GLOBAL_ORIENTATION, CB_ROTOR_HEAD, m_tp); // do the callback
        YA.x = m_va[0];
        YA.y = m_va[1];
        YA.z = m_va[2]; // retreve the X axis info from the pass array
        m_va[0] = 0;
        m_va[1] = 0;
        m_va[2] = 1; // set up for the 'Z' axis of the heli
        callback(GET_GLOBAL_ORIENTATION, CB_ROTOR_HEAD, m_tp); // do the callback
        ZA.x = m_va[0];
        ZA.y = m_va[1];
        ZA.z = m_va[2]; // retreve the X axis info from the pass array
        XA = VUnit(VCross(YA, ZA)); // generate the X axis as the cross product of y and z
        // just in case of a slight move between the y and z axis 'readings' we re-create the Y axis
        YA = VCross(ZA, XA);
        m_visualRotorFrame.m_xAxis = XA;
        m_visualRotorFrame.m_yAxis = YA;
        m_visualRotorFrame.m_zAxis = ZA; // transfer result to a global

        WP_ASSERT(m_visualRotorFrame.m_xAxis.isFinite());
        WP_ASSERT(m_visualRotorFrame.m_yAxis.isFinite());
        WP_ASSERT(m_visualRotorFrame.m_zAxis.isFinite());
    }

    // gets the angular velocity of the model
    void HeliAero::getVisualRotorAngularVelocity()
    {
        m_tp = m_va; // ensure TP pointer points to VA array
        callback(GET_ANGULAR_VELOCITY, CB_ROTOR_HEAD,
                 m_tp); // generate callback for  Angular velocity and the MODEL
        m_visualRotorAngularVelocity.x = m_va[0];
        m_visualRotorAngularVelocity.y = m_va[1];
        m_visualRotorAngularVelocity.z = m_va[2]; // transfer the angular velocity to global var

        WP_ASSERT(m_visualRotorAngularVelocity.isFinite());
    } // GetAngularVelocity

    // returns the distance to the ground from point in local frame "Loc" along the direction"Dir"
    f32 HeliAero::getGroundDistance(const physics_Vec &Dir, const physics_Vec &Loc)
    {
        WP_ASSERT(Dir.isFinite());
        WP_ASSERT(Loc.isFinite());

        m_tp = m_va;
        m_va[0] = static_cast<f32>(Dir.x);
        m_va[1] = static_cast<f32>(Dir.y);
        m_va[2] = static_cast<f32>(Dir.z); // pass the direction into the pass array
        m_va[3] = static_cast<f32>(Loc.x);
        m_va[4] = static_cast<f32>(Loc.y);
        m_va[5] = static_cast<f32>(Loc.z); // pass the location into the pass array
        callback(CAST_LOCAL_RAY, CB_ROTOR_HEAD, m_tp); // do the callback to the ray cast callback

        WP_ASSERT(Math<physics_Num>::isFinite( m_va[0] ));
        return m_va[0]; // return with the ground distance (passed from python on VA[0]);
    }

    // gets all the data about the model
    void HeliAero::readModelData()
    {
        getModelAngularVelocity();
        getModelLinearVelocity();
        getModelPosition();
        getModelFrame();
        getVisualRotorFrame();
        getVisualRotorAngularVelocity();

        // convert the models linear velocity in its frame to a world frame
        m_body.m_velocity = VecFromFrame(m_modelVelocity, m_modelFrame);
        // now add in the wind to get the airflow of the body in the ground frame
        m_body.m_flowInGF = VDif(m_body.m_velocity, m_wind);
        // then convert this to a flow in the body's own FoR
        m_body.m_airFlow = VecToFrame(m_body.m_flowInGF, m_body.m_frame);
        // transfer the models orientation into the Body record
        m_body.m_frame = m_modelFrame;
        m_body.m_angularVelocity = m_modelAngularVelocity;
    } // ReadModelData;

    // The following rotates the entire induced flow vector set as required when the HELI goes
    // through the given Pitch, Yaw and Roll angles
    void HeliAero::rotateInduced(TRotor &Rotor, double PitchAng, double YawAng, double RollAng)
    {
        WP_ASSERT(Math<double>::isFinite( PitchAng ));
        WP_ASSERT(Math<double>::isFinite( YawAng ));
        WP_ASSERT(Math<double>::isFinite( RollAng ));

        real_dNum K11, K12, K13; // rotation coefs for x output
        real_dNum K21, K22, K23; // rotation coefs for y output
        real_dNum K31, K32, K33; // rotation coefs for z output

        s32 RR, SS;
        K11 = (1.0 - (Math<real_dNum>::Sqr(RollAng) + Math<real_dNum>::Sqr(YawAng)) /
               2.0); // small angle approximation
        K12 = RollAng; // note sign would normally be neg for vec rotn in frame
        K13 = -YawAng;
        K21 = -RollAng;
        K22 = (1.0 - (Math<real_dNum>::Sqr(RollAng) + Math<real_dNum>::Sqr(PitchAng)) / 2.0);
        K23 = PitchAng;
        K31 = YawAng;
        K32 = -PitchAng;
        K33 = (1.0 - (Math<real_dNum>::Sqr(YawAng) + Math<real_dNum>::Sqr(PitchAng)) / 2.0);

        /*with Rotor do*/
        {
            /*with Sector[SS] do*/
            for(SS = 1; SS < 5; SS++)
            {
                TRotorSector &sector = Rotor.m_sector[SS];

                /*with Arc[RR] do*/
                for(RR = 1; RR < static_cast<s32>(sector.m_arc.size()); RR++)
                {
                    TSurface &surface = sector.m_arc[RR];

                    surface.m_lInduced.x = static_cast<physics_Num>(
                        K11 * static_cast<real_dNum>(surface.m_lInduced.x) +
                        K12 * static_cast<real_dNum>(surface.m_lInduced.y) +
                        K13 * static_cast<real_dNum>(surface.m_lInduced.z));
                    surface.m_lInduced.y = static_cast<physics_Num>(
                        K21 * static_cast<real_dNum>(surface.m_lInduced.x) +
                        K22 * static_cast<real_dNum>(surface.m_lInduced.y) +
                        K23 * static_cast<real_dNum>(surface.m_lInduced.z));
                    surface.m_lInduced.z = static_cast<physics_Num>(
                        K31 * static_cast<real_dNum>(surface.m_lInduced.x) +
                        K32 * static_cast<real_dNum>(surface.m_lInduced.y) +
                        K33 * static_cast<real_dNum>(surface.m_lInduced.z));

                    WP_ASSERT(surface.m_lInduced.isFinite());
                } // with Arc
            } // with Sector
        } // With Rotor
    } // RotateInduced

    // sets the constant values for the rotors and body and sets initial conditions for variables
    // this may eventually be replaced by a file read of the data once data set settles down.
    void HeliAero::initialize()
    {
        try
        {
            WP_DEBUG_TRACE;

            int zz;
            physics_Vec TV; // temporary vector

            physics_Vec RotVec; // temp vector to hold elements of the shaft frame rotation
            /*with Body do*/
            {
                m_body.m_frame.m_xAxis.x = 1;
                m_body.m_frame.m_xAxis.y = 0;
                m_body.m_frame.m_xAxis.z =
                    0; // set the body frame as upright pointing in the x direction
                m_body.m_frame.m_yAxis.x = 0;
                m_body.m_frame.m_yAxis.y = 1;
                m_body.m_frame.m_yAxis.z = 0;
                m_body.m_frame.m_zAxis.x = 0;
                m_body.m_frame.m_zAxis.y = 0;
                m_body.m_frame.m_zAxis.z = 1; //

                m_body.m_frontCDA = 0.01;
                m_body.m_sideCDA = 0.03;
                m_body.m_planCDA = 0.025;

                /*with FrontCOA do*/
                {
                    m_body.m_frontCOA.x = 0;
                    m_body.m_frontCOA.y = 0;
                    m_body.m_frontCOA.z = 0;
                } // set Frontal centre of pressure
                /*with SideCOA do*/
                {
                    m_body.m_sideCOA.x = 0;
                    m_body.m_sideCOA.y = 0;
                    m_body.m_sideCOA.z = -0.2;
                } // set side centre of pressure
                /*with PlanCOA do*/
                {
                    m_body.m_planCOA.x = 0;
                    m_body.m_planCOA.y = 0;
                    m_body.m_planCOA.z = -0.2;
                } // set plan centre of pressure
            } // with Body
            /*with RotorHead do*/
            {
                TV.x = 0;
                TV.y = 0;
                TV.z = 0;

                for(zz = 1; zz != 20; zz++)
                {
                    m_rotorHead->m_inflow[zz] = TV; // set all inflow vectors to zero
                }

                RotVec.x = static_cast<physics_Num>(m_rotorHead->m_shaftRake);
                RotVec.y = 0;
                RotVec.z = static_cast<physics_Num>(m_rotorHead->m_shaftTilt);
                // feed the rotation components into RotVec ready to call RotateFrame

                m_rotorHead->m_shaftFrame = RotateFrame(
                    m_body.m_frame, RotVec); // sort out the mainshaft frame WRT the Body frame
                m_rotorHead->m_linkage.m_swashFrame =
                    m_rotorHead
                    ->m_shaftFrame; // initialize the Swashplate frame aligned to the main shaft

                /*with MainRotor do*/
                {
                    TRotor &mr = m_rotorHead->m_mainRotor;

                    mr.setSection("MainFoil.dat");
                    mr.m_totalAngleLimit = 20.0 * Math<real_dNum>::pi() / 180.0;
                    mr.setOmega(170.0); // initialize to non-zero value to prevent undefined angles
                    // of attack
                    mr.m_cone = 0;
                    mr.m_collective = 0;
                    mr.m_ailCyclic = 0;
                    mr.m_eleCyclic = 0;
                    /*with Frame do*/
                    {
                        mr.m_frame.m_xAxis.x = 1;
                        mr.m_frame.m_xAxis.y = 0;
                        mr.m_frame.m_xAxis.z = 0;
                        mr.m_frame.m_yAxis.x = 0;
                        mr.m_frame.m_yAxis.y = 1;
                        mr.m_frame.m_yAxis.z = 0;
                        mr.m_frame.m_zAxis.x = 0;
                        mr.m_frame.m_zAxis.y = 0;
                        mr.m_frame.m_zAxis.z = 1;
                    } // setting the reference frame vectors aligned with the ground frame

                    /*with Sector[zz] do*/
                    for(zz = 1; zz != 4; zz++)
                    {
                        TRotorSector &sector = mr.m_sector[zz];
                        sector.m_secForce = TV;
                        sector.m_secPower = 0;
                    } // for with sectors

                    mr.m_angularMomentum =
                        mr.m_momentOfInertia * mr.getOmega(); // angular momentum of the rotor

                    /*with ForceMoments do*/
                    {
                        mr.m_forceMoments.x = 0;
                        mr.m_forceMoments.y = 0;
                        mr.m_forceMoments.z = 0;
                    } // moments acting on disc in vector form
                } // with MainRotor

                /*with FlyBar do*/
                {
                    TRotor &fb = m_rotorHead->m_flyBar;

                    fb.setSection("FBFoil.dat");
                    fb.m_totalAngleLimit = 50.0 * Math<physics_Num>::pi() / 180.0;
                    fb.setOmega(
                        170); // initialize to non-zero value to prevent undefined angles of attack
                    fb.m_cone = 0;
                    fb.m_collective = 0;
                    fb.m_ailCyclic = 0;
                    fb.m_eleCyclic = 0;
                    /*with Frame do*/
                    {
                        fb.m_frame.m_xAxis.x = 1;
                        fb.m_frame.m_xAxis.y = 0;
                        fb.m_frame.m_xAxis.z = 0;
                        fb.m_frame.m_yAxis.x = 0;
                        fb.m_frame.m_yAxis.y = 1;
                        fb.m_frame.m_yAxis.z = 0;
                        fb.m_frame.m_zAxis.x = 0;
                        fb.m_frame.m_zAxis.y = 0;
                        fb.m_frame.m_zAxis.z = 1;
                    } // setting the reference frame vectors aligned with the ground frame

                    /*with Sector[zz] do*/
                    for(zz = 1; zz != 4; zz++)
                    {
                        TRotorSector &sector = fb.m_sector[zz];
                        sector.m_secForce = TV;
                        sector.m_secPower = 0;
                    } // for with sectors

                    fb.m_momentOfInertia =
                        fb.m_blades * fb.m_bladeWeight *
                        Math<real_dNum>::Sqr(
                            fb.m_radOfGyr); // moment of inertia of rotor about hub centre
                    fb.m_angularMomentum =
                        fb.m_momentOfInertia * fb.getOmega(); // angular momentum of the rotor about

                    /*with ForceMoments do*/
                    {
                        fb.m_forceMoments.x = 0;
                        fb.m_forceMoments.y = 0;
                        fb.m_forceMoments.z = 0;
                    } // moments acting on disc in vector form
                } // with FlyBar
            } // with RotorHead

            /*with TailRotor do*/
            {
                TRotor &tr = *m_tailRotor;

                tr.setSection("TailFoil.dat");
                tr.m_totalAngleLimit = 50.0 * Math<physics_Num>::pi() / 180.0;
                tr.setOmega(
                    450); // initialize to non-zero value to prevent undefined angles of attack
                tr.m_cone = 0;
                tr.m_collective = 0;
                tr.m_ailCyclic = 0;
                tr.m_eleCyclic = 0;

                /*with Frame do*/
                {
                    tr.m_frame.m_xAxis.x = 0;
                    tr.m_frame.m_xAxis.y = -1;
                    tr.m_frame.m_xAxis.z =
                        0; // note the tail rotor set up pointing left in the world frame
                    tr.m_frame.m_yAxis.x = 1;
                    tr.m_frame.m_yAxis.y = 0;
                    tr.m_frame.m_yAxis.z =
                        0; // we need to have a way of making this track the heli's frame
                    tr.m_frame.m_zAxis.x = 0;
                    tr.m_frame.m_zAxis.y = 0;
                    tr.m_frame.m_zAxis.z = 1;
                } // setting the reference frame vectors aligned with the ground frame

                /*with Sector[zz] do*/
                for(zz = 1; zz != 4; zz++)
                {
                    TRotorSector &sector = tr.m_sector[zz];

                    sector.m_secForce = TV;
                    sector.m_secPower = 0;
                } // for with sectors

                tr.m_momentOfInertia =
                    tr.m_blades * tr.m_bladeWeight *
                    Math<real_dNum>::Sqr(
                        tr.m_radOfGyr); // moment of inertia of rotor about hub centre
                tr.m_angularMomentum =
                    tr.m_momentOfInertia * tr.getOmega(); // angular momentum of the rotor about

                /*with ForceMoments do*/
                {
                    tr.m_forceMoments.x = 0;
                    tr.m_forceMoments.y = 0;
                    tr.m_forceMoments.z = 0;
                } // moments acting on disc in vector form
            } // with TailRotor
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    } // initialize

    // set up the 'constants' for the blade elements
    // but also initialises some that are updated every timestep
    void HeliAero::initSurfaces(TRotor &rotor)
    {
        WP_DEBUG_TRACE;

        int rr, ss;
        real_dNum tr, deltaR;
        real_dNum tc, deltaC;
        physics_Vec zVec;
        zVec.x = 0;
        zVec.y = 0;
        zVec.z = 0;

        for(ss = 1; ss < 5; ss++)
        {
            TRotorSector &sector = rotor.m_sector[ss];

            deltaR = (rotor.m_maxRad - rotor.m_minRad) / 20.0;
            deltaC = (rotor.m_tipChord - rotor.m_cuffChord) / 20.0;
            tr = rotor.m_minRad + deltaR / 2.0;
            tc = rotor.m_cuffChord + deltaC / 2.0;

            for(rr = 1; rr < static_cast<s32>(sector.m_arc.size()); rr++)
            {
                sector.m_arc[rr].m_rad = tr;
                sector.m_arc[rr].m_chord = tc;
                sector.m_arc[rr].m_span = deltaR;
                sector.m_arc[rr].m_area = sector.m_arc[rr].m_chord * sector.m_arc[rr].m_span;

                sector.m_arc[rr].m_iFactor =
                    rotor.m_blades / (Math<real_dNum>::pi() * sector.m_arc[rr].m_rad *
                                      sector.m_arc[rr].m_span * m_roair);
                sector.m_arc[rr].m_lInduced = zVec;
                sector.m_arc[rr].m_interference = zVec;
                tr = tr + deltaR;
                tc = tc + deltaC;

                if(ss == 1)
                {
                    TSurface &arc = sector.m_arc[rr];

                    arc.m_spanVec.x = static_cast<real_dNum>(0);
                    arc.m_spanVec.y = static_cast<physics_Num>(rotor.m_cone);
                    arc.m_spanVec.z = -(1.0 - 0.5 * Math<real_dNum>::Sqr(rotor.m_cone));
                    arc.m_spanVec = VUnit(arc.m_spanVec);
                    arc.m_chordVec.x =
                        static_cast<physics_Num>(1.0) -
                        static_cast<physics_Num>(0.5) * Math<real_dNum>::Sqr(arc.m_alphaG);
                    arc.m_chordVec.y = arc.m_alphaG;
                    arc.m_chordVec.z = rotor.m_cone * arc.m_alphaG;
                    arc.m_chordVec = VUnit(arc.m_chordVec);
                    arc.m_normVec = VCross(arc.m_chordVec, arc.m_spanVec);

                    arc.setLFlow(physics_Vec(-1.0, 0.0, 0.0));

                    arc.m_liftVec = VUnit(VCross(arc.m_spanVec, arc.getLFlow()));
                    arc.m_centre = VScale(arc.m_spanVec, arc.m_rad);
                }

                if(ss == 2)
                {
                    TSurface &arc = sector.m_arc[rr];

                    arc.m_spanVec.x = (1.0 - 0.5 * Math<real_dNum>::Sqr(rotor.m_cone));
                    arc.m_spanVec.y = rotor.m_cone;
                    arc.m_spanVec.z = static_cast<physics_Num>(0.0);
                    arc.m_spanVec = VUnit(arc.m_spanVec);
                    arc.m_chordVec.x = -rotor.m_cone * arc.m_alphaG;
                    arc.m_chordVec.y = arc.m_alphaG;
                    arc.m_chordVec.z =
                        static_cast<physics_Num>(1.0) -
                        static_cast<physics_Num>(0.5) * Math<real_dNum>::Sqr(arc.m_alphaG);
                    arc.m_chordVec = VUnit(arc.m_chordVec);
                    arc.m_normVec = VCross(arc.m_chordVec, arc.m_spanVec);

                    arc.setLFlow(physics_Vec(0.0, 0.0, -1.0));

                    arc.m_liftVec = VUnit(VCross(arc.m_spanVec, arc.getLFlow()));
                    arc.m_centre = VScale(arc.m_spanVec, arc.m_rad);
                }

                if(ss == 3)
                {
                    TSurface &arc = sector.m_arc[rr];

                    arc.m_spanVec.x = static_cast<physics_Num>(0.0);
                    arc.m_spanVec.y = rotor.m_cone;
                    arc.m_spanVec.z = (1.0 - 0.5 * Math<real_dNum>::Sqr(rotor.m_cone));
                    arc.m_spanVec = VUnit(arc.m_spanVec);
                    arc.m_chordVec.x = -(static_cast<physics_Num>(1.0) -
                                         static_cast<physics_Num>(0.5) *
                                         Math<physics_Num>::Sqr(arc.m_alphaG));
                    arc.m_chordVec.y = arc.m_alphaG;
                    arc.m_chordVec.z = -rotor.m_cone * arc.m_alphaG;
                    arc.m_chordVec = VUnit(arc.m_chordVec);
                    arc.m_normVec = VCross(arc.m_chordVec, arc.m_spanVec);

                    arc.setLFlow(physics_Vec(1.0, 0.0, 0.0));

                    arc.m_liftVec = VUnit(VCross(arc.m_spanVec, arc.getLFlow()));
                    arc.m_centre = VScale(arc.m_spanVec, arc.m_rad);
                }

                if(ss == 4)
                {
                    TSurface &arc = sector.m_arc[rr];

                    arc.m_spanVec.x = -(1.0 - 0.5 * Math<real_dNum>::Sqr(rotor.m_cone));
                    arc.m_spanVec.y = rotor.m_cone;
                    arc.m_spanVec.z = static_cast<physics_Num>(0.0);
                    arc.m_spanVec = VUnit(arc.m_spanVec);
                    arc.m_chordVec.x = rotor.m_cone * arc.m_alphaG;
                    arc.m_chordVec.y = arc.m_alphaG;
                    arc.m_chordVec.z = -(1.0 - 0.5 * Math<real_dNum>::Sqr(arc.m_alphaG));
                    arc.m_chordVec = VUnit(arc.m_chordVec);
                    arc.m_normVec = VCross(arc.m_chordVec, arc.m_spanVec);

                    arc.setLFlow(physics_Vec(0.0, 0.0, 1.0));

                    arc.m_liftVec = VUnit(VCross(arc.m_spanVec, arc.getLFlow()));
                    arc.m_centre = VScale(arc.m_spanVec, arc.m_rad);
                }
            }
        }
    }

    // calculates the ground frame axes of the rotor and calculates the hub centre flow
    // note the Frame.m_yAxis is the TPP for the rotor and is not affected by this coordinate rotation
    void HeliAero::calcAllRotorFrameFlows()
    {
        physics_Vec wnd;
        // holds the aircraft frame version of wind which is needed to align the cyclic controls of
        // the rotor with the helicopter's heading
        wnd = VecToFrame(m_wind, m_body.m_frame); // convert it to aircraft frame
        m_totalFlow = VDif(wnd, m_body.m_velocity);
        // apply wind correction to model velocity to get CoG flow at model (use to set sign
        // convention for flow!!)

        {
            TRotor &mr = m_rotorHead->m_mainRotor;
            mr.m_frame.m_zAxis = VUnit(
                VCross(m_body.m_frame.m_xAxis,
                       mr.m_frame.m_yAxis)); // this is the forward pointing axis of the rotor
            mr.m_frame.m_xAxis = VUnit(
                VCross(mr.m_frame.m_yAxis,
                       mr.m_frame.m_zAxis)); // this is the left pointing axis of the rotor
            mr.m_hubFlow = VecToFrame(m_totalFlow, mr.m_frame);
            // note hub flow is in the opposite direction to the model velocity so hub flow
        }

        {
            TRotor &fb = m_rotorHead->m_flyBar;
            fb.m_frame.m_zAxis = VUnit(
                VCross(m_body.m_frame.m_xAxis,
                       fb.m_frame.m_yAxis)); // this is the forward pointing axis of the rotor
            fb.m_frame.m_xAxis = VUnit(
                VCross(fb.m_frame.m_yAxis,
                       fb.m_frame.m_zAxis)); // this is the left pointing axis of the rotor
            fb.m_hubFlow = VecToFrame(m_totalFlow, fb.m_frame);
            // note hub flow is in the opposite direction to the model velocity so hub flow
        }

        m_tailRotor->m_hubFlow = VecToFrame(m_totalFlow, m_tailRotor->m_frame);
        m_tailRotor->m_hubFlow.y =
            m_tailRotor->m_hubFlow.y -
            m_modelAngularVelocity.y *
            m_tailRotor->m_hubPosition.z; // note sign req. because Posn.z is negative
    }

    // calculates the flow at each blade element of the main rotor
    void HeliAero::calcMainArcFlows(TRotor &rotor)
    {
        auto rotorOmega = rotor.getOmega();

        // Sector 1: blade velocity wrt hub is in the +x direction
        {
            TRotorSector &sector = rotor.m_sector[1];

            for(size_t rr = 1; rr < sector.m_arc.size(); rr++)
            {
                TSurface &arc = sector.m_arc[rr];

                auto lFlow = V3Sum(rotor.m_hubFlow, arc.m_lInduced, arc.m_interference);
                lFlow.y = lFlow.y - rotor.m_groundEffect[1];
                lFlow.x = lFlow.x - rotorOmega * arc.m_rad;
                arc.setLFlow(lFlow);

                arc.m_liftVec = VUnit(VCross(arc.m_spanVec, lFlow));
                arc.setFlowSpeed(VMag(lFlow));
                arc.m_flowVec = VUnit(lFlow);
            }
        }

        // Sector 2: blade velocity wrt hub is in the +z direction
        {
            TRotorSector &sector = rotor.m_sector[2];

            for(size_t rr = 1; rr < sector.m_arc.size(); rr++)
            {
                TSurface &arc = sector.m_arc[rr];

                auto lFlow = V3Sum(rotor.m_hubFlow, arc.m_lInduced, arc.m_interference);
                lFlow.y = lFlow.y - rotor.m_groundEffect[2];
                lFlow.z = lFlow.z - rotorOmega * arc.m_rad;
                arc.setLFlow(lFlow);

                arc.m_liftVec = VUnit(VCross(arc.m_spanVec, lFlow));
                arc.setFlowSpeed(VMag(lFlow));
                arc.m_flowVec = VUnit(lFlow);
            }
        }

        // Sector 3: blade velocity wrt hub is in the -x direction
        {
            TRotorSector &sector = rotor.m_sector[3];

            for(size_t rr = 1; rr < sector.m_arc.size(); rr++)
            {
                TSurface &arc = sector.m_arc[rr];

                auto lFlow = V3Sum(rotor.m_hubFlow, arc.m_lInduced, arc.m_interference);
                lFlow.y = lFlow.y - rotor.m_groundEffect[3];
                lFlow.x = lFlow.x + rotorOmega * arc.m_rad;
                arc.setLFlow(lFlow);

                arc.m_liftVec = VUnit(VCross(arc.m_spanVec, lFlow));
                arc.setFlowSpeed(VMag(lFlow));
                arc.m_flowVec = VUnit(lFlow);
            }
        }

        // Sector 4: blade velocity wrt hub is in the -z direction
        {
            TRotorSector &sector = rotor.m_sector[4];

            for(size_t rr = 1; rr < sector.m_arc.size(); rr++)
            {
                TSurface &arc = sector.m_arc[rr];

                auto lFlow = V3Sum(rotor.m_hubFlow, arc.m_lInduced, arc.m_interference);
                lFlow.y = lFlow.y - rotor.m_groundEffect[4];
                lFlow.z = lFlow.z + rotorOmega * arc.m_rad;
                arc.setLFlow(lFlow);

                arc.m_liftVec = VUnit(VCross(arc.m_spanVec, lFlow));
                arc.setFlowSpeed(VMag(lFlow));
                arc.m_flowVec = VUnit(lFlow);
            }
        }
    }

    // in ver 15 added the Ifac to adjust the coupling of the induced flow for the main rotor
    // at radial station 7 to be as the induced flow for the FlyBar
    void HeliAero::mainToFlyBarInterference(physics_Num IFac)
    {
        int SS, RR;
        for(SS = 1; SS < 5; SS++)
        {
            TRotorSector &sector = m_rotorHead->m_flyBar.m_sector[SS];

            for(RR = 1; RR < 21; RR++)
            {
                // apply the main rotor's induced flow as interference to the flybar
                m_rotorHead->m_flyBar.m_sector[SS].m_arc[RR].m_interference =
                    VScale(m_rotorHead->m_mainRotor.m_sector[SS].m_arc[7].m_lInduced,
                           IFac); // note the IFac scaling
            } // for RR
        } // for SS
    }

    // this procedure now limits the combination of collective and cyclic at any of the major
    // azimuthal angles to 22 degrees set up surface variables for current airflow, cone, rpm, etc
    // conditions
    void HeliAero::updateSurfaces(TRotor &rotor)
    {
        // Sector 1 is at azimuth = 0 (pointing rearwards)
        {
            for(int rr = 1; rr < 21; rr++)
            {
                TSurface &arc = rotor.m_sector[1].m_arc[rr];

                real_dNum ag = rotor.m_collective + rotor.m_ailCyclic;
                Math<real_dNum>::Limit(ag, rotor.m_totalAngleLimit);
                arc.m_alphaG = ag + (static_cast<physics_Num>(20.0) - rr) * rotor.m_twist /
                               static_cast<physics_Num>(19.0);

                arc.m_spanVec.x = static_cast<physics_Num>(0);
                arc.m_spanVec.y = rotor.m_cone;
                arc.m_spanVec.z =
                    -(static_cast<physics_Num>(1.0) -
                      static_cast<physics_Num>(0.5) * Math<physics_Num>::Sqr(rotor.m_cone));
                arc.m_spanVec = VUnit(arc.m_spanVec);

                arc.m_chordVec.x =
                    static_cast<physics_Num>(1.0) -
                    static_cast<physics_Num>(0.5) * Math<physics_Num>::Sqr(arc.m_alphaG);
                arc.m_chordVec.y = arc.m_alphaG;
                arc.m_chordVec.z = rotor.m_cone * arc.m_alphaG;
                arc.m_chordVec = VUnit(arc.m_chordVec);

                arc.m_normVec = VCross(arc.m_chordVec, arc.m_spanVec);
                arc.setLFlow(physics_Vec(-1.0, 0.0, 0.0));
                arc.m_liftVec = VUnit(VCross(arc.m_spanVec, arc.getLFlow()));
                arc.m_centre = VScale(arc.m_spanVec, arc.m_rad);
            }
        }

        // Sector 2 is at azimuth = 90 (pointing left)
        {
            for(int rr = 1; rr < 21; rr++)
            {
                TSurface &arc = rotor.m_sector[2].m_arc[rr];

                real_dNum ag = rotor.m_collective - rotor.m_eleCyclic;
                Math<real_dNum>::Limit(ag, rotor.m_totalAngleLimit);
                arc.m_alphaG = ag + (static_cast<physics_Num>(20.0) - rr) * rotor.m_twist /
                               static_cast<physics_Num>(19.0);

                arc.m_spanVec.x =
                (static_cast<physics_Num>(1.0) -
                 static_cast<physics_Num>(0.5) * Math<physics_Num>::Sqr(rotor.m_cone));
                arc.m_spanVec.y = rotor.m_cone;
                arc.m_spanVec.z = 0;
                arc.m_spanVec = VUnit(arc.m_spanVec);

                arc.m_chordVec.x = -rotor.m_cone * arc.m_alphaG;
                arc.m_chordVec.y = arc.m_alphaG;
                arc.m_chordVec.z =
                    static_cast<physics_Num>(1.0) -
                    static_cast<physics_Num>(0.5) * Math<physics_Num>::Sqr(arc.m_alphaG);
                arc.m_chordVec = VUnit(arc.m_chordVec);

                arc.m_normVec = VCross(arc.m_chordVec, arc.m_spanVec);
                arc.setLFlow(physics_Vec(0.0, 0.0, -1.0));
                arc.m_liftVec = VUnit(VCross(arc.m_spanVec, arc.getLFlow()));
                arc.m_centre = VScale(arc.m_spanVec, arc.m_rad);
            }
        }

        // Sector 3 is at azimuth = 180 (pointing forwards)
        {
            for(int rr = 1; rr < 21; rr++)
            {
                TSurface &arc = rotor.m_sector[3].m_arc[rr];

                real_dNum ag = rotor.m_collective - rotor.m_ailCyclic;
                Math<real_dNum>::Limit(ag, rotor.m_totalAngleLimit);
                arc.m_alphaG = ag + (static_cast<physics_Num>(20.0) - rr) * rotor.m_twist /
                               static_cast<physics_Num>(19.0);

                arc.m_spanVec.x = static_cast<physics_Num>(0);
                arc.m_spanVec.y = rotor.m_cone;
                arc.m_spanVec.z =
                (static_cast<physics_Num>(1.0) -
                 static_cast<physics_Num>(0.5) * Math<physics_Num>::Sqr(rotor.m_cone));
                arc.m_spanVec = VUnit(arc.m_spanVec);

                arc.m_chordVec.x =
                    -(static_cast<physics_Num>(1.0) -
                      static_cast<physics_Num>(0.5) * Math<physics_Num>::Sqr(arc.m_alphaG));
                arc.m_chordVec.y = arc.m_alphaG;
                arc.m_chordVec.z = -rotor.m_cone * arc.m_alphaG;
                arc.m_chordVec = VUnit(arc.m_chordVec);

                arc.m_normVec = VCross(arc.m_chordVec, arc.m_spanVec);
                arc.setLFlow(physics_Vec(1.0, 0.0, 0.0));
                arc.m_liftVec = VUnit(VCross(arc.m_spanVec, arc.getLFlow()));
                arc.m_centre = VScale(arc.m_spanVec, arc.m_rad);
            }
        }

        // Sector 4 is at azimuth = 270 (pointing right)
        {
            for(int rr = 1; rr < 21; rr++)
            {
                TSurface &arc = rotor.m_sector[4].m_arc[rr];

                real_dNum ag = rotor.m_collective + rotor.m_eleCyclic;
                Math<real_dNum>::Limit(ag, rotor.m_totalAngleLimit);
                arc.m_alphaG = ag + (20.0 - static_cast<real_dNum>(rr)) * rotor.m_twist / 19.0;

                arc.m_spanVec.x = -(1.0 - 0.5 * Math<real_dNum>::Sqr(rotor.m_cone));
                arc.m_spanVec.y = rotor.m_cone;
                arc.m_spanVec.z = static_cast<physics_Num>(0.0);
                arc.m_spanVec = VUnit(arc.m_spanVec);

                arc.m_chordVec.x = rotor.m_cone * arc.m_alphaG;
                arc.m_chordVec.y = arc.m_alphaG;
                arc.m_chordVec.z = -(1.0 - 0.5 * Math<real_dNum>::Sqr(arc.m_alphaG));
                arc.m_chordVec = VUnit(arc.m_chordVec);

                arc.m_normVec = VCross(arc.m_chordVec, arc.m_spanVec);
                arc.setLFlow(physics_Vec(0.0, 0.0, 1.0));
                arc.m_liftVec = VUnit(VCross(arc.m_spanVec, arc.getLFlow()));
                arc.m_centre = VScale(arc.m_spanVec, arc.m_rad);
            }
        }
    }

    // this factor is clearly too great in the modest descent phase of the model
    // calculate the correction factor for climb rate (passed in multiples of the induced flow speed)
    physics_Num HeliAero::climbFactor(physics_Num ClimbRate)
    {
        physics_Num result = 0.0;
        physics_Num FullVR = 0.0;
        // used to calculate the full theory vortex ring factor on the 0 to -2 range

        if(ClimbRate < static_cast<physics_Num>(-2.0))
        {
            // for fast descents use momentum theory equation
            result = static_cast<physics_Num>(-0.5) * ClimbRate -
                     Math<physics_Num>::Sqrt(
                         Math<physics_Num>::Sqr(static_cast<physics_Num>(0.5) * ClimbRate) -
                         static_cast<physics_Num>(1.0));
        } // Rate<-2
        else
        {
            if(ClimbRate < static_cast<physics_Num>(0.0))
            {
                // between 0 and -2 use fitted curve for factor
                // ClimbFactor:=1;//testing with no vortex ring
                FullVR = Math<physics_Num>::Sqrt(
                    static_cast<physics_Num>(1.0) - static_cast<physics_Num>(1.125) * ClimbRate -
                    static_cast<physics_Num>(1.372) * Math<physics_Num>::Sqr(ClimbRate) -
                    static_cast<physics_Num>(1.718) *
                    Math<physics_Num>::Pow(ClimbRate, 3.0) -
                    static_cast<physics_Num>(0.656) *
                    Math<physics_Num>::Pow(ClimbRate, 4.0));
                // note Square root used to reduce peak
                result = static_cast<physics_Num>(1.0) +
                         m_vrMultiplier.m_sVal * (FullVR - static_cast<physics_Num>(1.0));
                // apply the multiplier to the excess inflow of the VR effect
            } //-2<=Rate<0
            else
            {
                if(ClimbRate >= static_cast<physics_Num>(0.0))
                {
                    // for hover and positive climb use momentum theory equation
                    result =
                        static_cast<physics_Num>(-0.5) * ClimbRate +
                        Math<physics_Num>::Sqrt(
                            Math<physics_Num>::Sqr(static_cast<physics_Num>(0.5) * ClimbRate) +
                            static_cast<physics_Num>(1.0));
                } //>=0
            }
        }

        return result;
    } // ClimbFactor

    // take in forward flight speed/Vt and outputs induced flow correction factor
    physics_Num HeliAero::translationFactor(physics_Num TransRate)
    {
        return Math<physics_Num>::Sqrt(
            (-Math<physics_Num>::Sqr(TransRate) +
             Math<physics_Num>::Sqrt(
                 Math<physics_Num>::Pow(TransRate, 4.0) +
                 static_cast<physics_Num>(4.0))) /
            static_cast<physics_Num>(2.0));
    } // TranslationFactor

    // groundFactor now has adjustable limit and rate of decay
    // takes Height/rotor radius (Ht) and calculates ground effect upflow as a fraction of induced
    // velocity
    physics_Num HeliAero::groundFactor(physics_Num Ht)
    {
        // note originally GEMax = 0.9 and GEDecay = 2
        return m_geMax * (Math<physics_Num>::Exp(-m_geDecay * Ht));
    } // GroundFactor

    // New flow rotation method seems to be a big improvement!
    // calculates the induced flow for main rotor and the tail rotor.
    void HeliAero::calcInduced()
    {
        constexpr real_dNum MainTC = 0.3; // set the induced flow  time constant to 0.3 seconds
        constexpr real_dNum TailTC = 0.1;
        // use a shorter TC for the tail  //note: basic Induced flow speed =
        // Math<physics_Num>::Sqrt(Lift/(2*SweptArea*Ro))

        real_dNum Vh; // the hover induced velocity based on gross lift and rotor area
        real_dNum Vc; // the climb air speed of the rotor (in its own frame)
        real_dNum Vt; // the translation air speed of the rotor (in its own frame)
        real_dNum CF; // the climb factor (as required by the inflow calculation)
        real_dNum TF; // the Translation factor as required by the inflow calculation)
        real_dNum Factr; // the combined climb and translation factors

        physics_Vec Dir, Loc; // Vectors for ray-casting to ground

        real_dNum PaddleAspectFactor;
        // used to apply the aspect ratio to the flybar's induced flow calculation
        Vh = Math<real_dNum>::Sqrt(
            Math<real_dNum>::Abs(m_rotorHead->m_mainRotor.m_hubForce.y) /
            (2.0 * Math<real_dNum>::pi() * m_roair *
             Math<real_dNum>::Sqr(
                 m_rotorHead->m_mainRotor
                            .m_maxRad))); // calculate the magnitude of the hover induced flow
        if(m_rotorHead->m_mainRotor.m_hubForce.y > static_cast<physics_Num>(0.0))
        {
            Vh = -Vh;
            // deal with the sign of Vh (i.e. if the thrust is positive the induced flow must be
            // negative)
        }

        // WP_LOG('Vh = '+FloatToStr(Vh));//write the value of Vt so it can be checked visually in
        // upright/inverted climb/decent
        Vc = m_rotorHead->m_mainRotor.m_hubFlow.y; // check that this is right for sign!!
        // WP_LOG('Vc = '+FloatToStr(Vc));
        Vt = Math<real_dNum>::Sqrt(Math<real_dNum>::Sqr(m_rotorHead->m_mainRotor.m_hubFlow.x) +
                                   Math<real_dNum>::Sqr(m_rotorHead->m_mainRotor.m_hubFlow.z));
        // use the sqrt of the sum of the squares of the inplane flow components as Vt

        if(Math<physics_Num>::equals(Vh, 0.0) == false)
        {
            // i.e Vh<>0
            CF = climbFactor(Vc / Vh);
            TF = translationFactor(Vt / Vh);
            // TF = 0.1;
        } // end Vh non-zero
        else
        {
            // Vh:=0 (so the induced flow is zero)
            CF = static_cast<physics_Num>(0.1);
            // this is likely to be a good guess as with Vh = 0 the whole inflow thing will be close
            // to zero
            TF = static_cast<physics_Num>(0.1);
        }

        Factr = CF * TF; // calculate the combined factor for climb and translation

        if(Factr < static_cast<physics_Num>(0.1))
        {
            Factr = static_cast<physics_Num>(0.1); // set a lower limit for this factor
        }

        // WP_LOG('CF = ' + FloatToStr(CF) + '  TF = ' + FloatToStr(TF));

        // do the ray-casting to get ground effect here
        if(((m_rotorHead->m_mainRotor.m_frame.m_yAxis.y > static_cast<physics_Num>(0.0)) &&
            (Vh < -std::numeric_limits<physics_Num>::epsilon())) ||
           ((m_rotorHead->m_mainRotor.m_frame.m_yAxis.y < static_cast<physics_Num>(0.0)) &&
            (Vh > std::numeric_limits<physics_Num>::epsilon())))
        {
            // Vh is towards the ground (so ground effect possible)
            if(m_rotorHead->m_mainRotor.m_frame.m_yAxis.y > static_cast<physics_Num>(0.0))
            {
                Dir.y = static_cast<physics_Num>(-2.0);
            }
            else
            {
                Dir.y = static_cast<physics_Num>(2.0); // get dir pointing the right way
            }

            // now greatly reducing the displacement of the locations to remove the cyclic aspect of
            // the ground effect
            Loc.x = static_cast<physics_Num>(0);
            Loc.y = static_cast<physics_Num>(0);
            Loc.z = static_cast<physics_Num>(-0.06); // set Loc for azimuth position 1
            m_rotorHead->m_mainRotor.m_groundDistance[1] = getGroundDistance(Dir, Loc);
            Loc.x = static_cast<physics_Num>(0.06);
            Loc.y = static_cast<physics_Num>(0);
            Loc.z = static_cast<physics_Num>(0); // set Loc for azimuth position 2
            m_rotorHead->m_mainRotor.m_groundDistance[2] = getGroundDistance(Dir, Loc);
            Loc.x = static_cast<physics_Num>(0);
            Loc.y = static_cast<physics_Num>(0);
            Loc.z = static_cast<physics_Num>(0.06); // set Loc for azimuth position 3
            m_rotorHead->m_mainRotor.m_groundDistance[3] = getGroundDistance(Dir, Loc);
            Loc.x = static_cast<physics_Num>(-0.06);
            Loc.y = static_cast<physics_Num>(0);
            Loc.z = static_cast<physics_Num>(0); // set Loc for azimuth position 4
            m_rotorHead->m_mainRotor.m_groundDistance[4] = getGroundDistance(Dir, Loc);

            for(int SS = 1; SS < 5; SS++)
            {
                // note the translation/climb factor added to this calculation to make ground effect
                // flow have the same multiplier as the induced flows
                if(Math<physics_Num>::equals(m_rotorHead->m_mainRotor.m_groundDistance[SS],
                                             0.0))
                {
                    m_rotorHead->m_mainRotor.m_groundEffect[SS] = static_cast<physics_Num>(0.0);
                }
                else
                {
                    m_rotorHead->m_mainRotor.m_groundEffect[SS] =
                        Factr * Vh *
                        groundFactor(m_rotorHead->m_mainRotor.m_groundDistance[SS] /
                                     m_rotorHead->m_mainRotor.m_maxRad);
                }

                // note the sign.  In upright hover Vt is neg so GndEffect is positive (in opposition
                // to the induced flow)
                //                 In inverted hover Vt is positive and GndEffect is negative (also
                //                 in opposition to inflow)
                //          See the CalcMainArcFlows procedure for how the GndEffect adds into the
                //          flow scheme
                m_rotorHead->m_flyBar.m_groundEffect[SS] =
                    m_rotorHead->m_mainRotor
                               .m_groundEffect[SS]; // copy the main rotor ground effect to the flybar
            } // for ss
        }
        else // i.e Vh is away from ground so no ground effect
        {
            for(int SS = 1; SS < 5; SS++)
            {
                m_rotorHead->m_mainRotor.m_groundEffect[SS] = static_cast<physics_Num>(0.0);
                // set the ground effect to zero
                m_rotorHead->m_flyBar.m_groundEffect[SS] =
                    m_rotorHead->m_mainRotor
                               .m_groundEffect[SS]; // copy the main rotor ground effect to the flybar
            } // for ss
        } // end of no ground effect

        real_dNum K2 = m_thisDeltaT / MainTC;
        // this is the fraction of the new flow value transferred to the running value allowing for
        // climb and translation
        real_dNum K1 =
            1.0 - K2; // this is the retained fraction of the running value based on the timeconstant

        // now rotate the induced flow to its new orientation based on the precession rate of the
        // rotor
        rotateInduced(m_rotorHead->m_mainRotor,
                      m_rotorHead->m_mainRotor.m_precessionRate.x * m_thisDeltaT,
                      m_rotorHead->m_mainRotor.m_precessionRate.y * m_thisDeltaT,
                      m_rotorHead->m_mainRotor.m_precessionRate.z * m_thisDeltaT);

        // with m_rotorHead->m_mainRotor do
        {
            TRotor &mr = m_rotorHead->m_mainRotor;

            // with Sector[SS] do
            for(int SS = 1; SS < 5; SS++)
            {
                TRotorSector &sector = mr.m_sector[SS];

                // with Arc[RR] do
                for(int RR = 1; RR < 21; RR++)
                {
                    TSurface &surface = sector.m_arc[RR];

                    WP_ASSERT(surface.m_lInduced.isFinite());
                    WP_ASSERT(Math<physics_Num>::isFinite( surface.m_iFactor ));
                    WP_ASSERT(Math<physics_Num>::isFinite( K1 ));
                    WP_ASSERT(Math<physics_Num>::isFinite( Factr ));
                    WP_ASSERT(Math<physics_Num>::isFinite( K2 ));

                    surface.m_lInduced =
                        VScale(surface.m_lInduced,
                               K1); // apply the k1 scaling factor to the whole vector

                    // now add in the new component to the induced flow in the local frame
                    if(surface.getLift() < static_cast<physics_Num>(0.0))
                    {
                        surface.m_lInduced.y =
                            surface.m_lInduced.y + Factr * K2 *
                            Math<real_dNum>::Sqrt(Math<real_dNum>::Abs(
                                surface.getLift() * surface.m_iFactor));
                    }
                    else
                    {
                        // trap the negative Lift cases
                        surface.m_lInduced.y =
                            surface.m_lInduced.y -
                            Factr * K2 *
                            Math<real_dNum>::Sqrt(surface.getLift() * surface.m_iFactor);
                    }

                    WP_ASSERT(surface.m_lInduced.isFinite());
                } // for rr
            } // For ss
        } // with MainRotor

        // now calculate the flybar induced flow using the same timeconstant as for the main rotor
        rotateInduced(m_rotorHead->m_flyBar,
                      m_rotorHead->m_flyBar.m_precessionRate.x * m_thisDeltaT,
                      m_rotorHead->m_flyBar.m_precessionRate.y * m_thisDeltaT,
                      m_rotorHead->m_flyBar.m_precessionRate.z * m_thisDeltaT);

        // with m_rotorHead->m_flyBar do
        {
            TRotor &fb = m_rotorHead->m_flyBar;

            PaddleAspectFactor =
                1.0 / (0.85 * Math<real_dNum>::pi() * 2.0 * (fb.m_maxRad - fb.m_minRad) /
                       (fb.m_cuffChord + fb.m_tipChord));
            //= 1/(planform efficiency factor * Math<physics_Num>::pi() * Aspect ratio of paddles

            // with Sector[SS] do
            for(int SS = 1; SS < 5; SS++)
            {
                TRotorSector &sector = fb.m_sector[SS];

                // with Arc[RR] do
                for(int RR = 1; RR < 21; RR++)
                {
                    TSurface &surface = sector.m_arc[RR];

                    surface.m_lInduced =
                        VScale(surface.m_lInduced,
                               K1); // apply the k1 scaling factor to the whole vector
                    surface.m_lInduced.y =
                        surface.m_lInduced.y - K2 * surface.getFlowSpeed() * surface.m_cl *
                        PaddleAspectFactor; // check the sign!!

                    WP_ASSERT(surface.m_lInduced.isFinite());
                } // for rr
            } // For ss
        } // with MainRotor

        K2 = m_thisDeltaT / TailTC;
        K1 = 1.0 - K2;

        /*with TailRotor do*/
        {
            TRotor &tr = *m_tailRotor;

            for(int SS = 1; SS < 5; SS++)
            {
                TRotorSector &sector = tr.m_sector[SS];

                for(int RR = 1; RR < 21; RR++)
                {
                    TSurface &surface = sector.m_arc[RR];

                    surface.m_lInduced =
                        VScale(surface.m_lInduced,
                               K1); // apply the k1 scaling factor to the whole vector
                    if(surface.getLift() < static_cast<physics_Num>(0.0))
                    {
                        surface.m_lInduced.y = surface.m_lInduced.y +
                                               K2 * Math<real_dNum>::Sqrt(Math<real_dNum>::Abs(
                                                   surface.getLift() * surface.m_iFactor));
                    }
                    else
                    {
                        surface.m_lInduced.y =
                            surface.m_lInduced.y -
                            K2 * Math<real_dNum>::Sqrt(
                                surface.getLift() *
                                surface.m_iFactor); // trap the negative Lift cases
                    }

                    WP_ASSERT(surface.m_lInduced.isFinite());
                } // for rr
            } // For ss
        } // with TailRotor
    } // CalcInduced

    // this procedure allows testing of a uniform inflow at each radius around the disk
    // s is the degree of smoothing (s=1 is completely smooth s=0 gives no smoothing)
    void HeliAero::smoothInduced(TRotor &Rotor, physics_Num S)
    {
        physics_Vec Mn, Dif;

        // factor for smoothing
        // set up K for the degree of smoothing
        physics_Num K = static_cast<physics_Num>(1.0) - S;

        // with Rotor do
        {
            for(int RR = 1; RR < 21; RR++)
            {
                Mn = V3Sum(Rotor.m_sector[1].m_arc[RR].m_lInduced,
                           Rotor.m_sector[2].m_arc[RR].m_lInduced,
                           Rotor.m_sector[3].m_arc[RR].m_lInduced);
                Mn = VSum(Mn,
                          Rotor.m_sector[4]
                          .m_arc[RR]
                          .m_lInduced); // summ all the induced flows for the sectors into Mn
                Mn = VScale(Mn,
                            static_cast<physics_Num>(0.25) *
                            S); // scale by 0.25*S to give mean scaled as needed for smoothing
                for(int SS = 1; SS < 5; SS++)
                {
                    Rotor.m_sector[SS].m_arc[RR].m_lInduced =
                        VSum(Mn, VScale(Rotor.m_sector[SS].m_arc[RR].m_lInduced,
                                        K)); // Induced = S*mean + (1-S)*Induced
                } // for ss
            } // for rr
        } // with rotor
    } // SmoothInduced

    // calculates the free stream attack at surface 'S' outputs values in range
    // -Math<physics_Num>::pi() to Math<physics_Num>::pi()
    void HeliAero::calcAttack(TSurface &S)
    {
        physics_Num Nor, Par, AA; // components of flow WRT surface and the first quadrant angle
        /*with S do*/
        {
            Nor = VDot(S.getLFlow(),
                       S.m_normVec); // note if LFlow along NormVec then attack is positive
            Par = -VDot(
                S.getLFlow(),
                S.m_chordVec); // note - sign allows for ChordVec pointing forward (from TE to LE)

            if(Math<physics_Num>::equals(Par, 0.0))
            {
                if(Nor > static_cast<physics_Num>(0.0))
                    S.m_alphaL = Math<real_dNum>::pi() / static_cast<physics_Num>(2.0);
                else
                    S.m_alphaL = -Math<real_dNum>::pi() / static_cast<physics_Num>(2.0);
                // deal with the case where Par = 0;
            } // end of par =0
            else // par <>0;
            {
                AA = Math<physics_Num>::Atan(
                    Math<physics_Num>::Abs(Nor / Par)); // first quadrant angle
                if(Nor >= static_cast<physics_Num>(0.0)) // positive Angle of attack
                {
                    if(Par >= static_cast<physics_Num>(0.0))
                        S.m_alphaL = AA;
                    else
                        S.m_alphaL = Math<physics_Num>::pi() - AA;
                } // end of positive attack
                else // negative angles of attack
                {
                    if(Par >= static_cast<physics_Num>(0.0))
                        S.m_alphaL = -AA;
                    else
                        S.m_alphaL = AA - Math<physics_Num>::pi();
                } // end of negative attack
            } // end of Par<>0
        } // end of with S
    } ////CalcAttack

    // searches for the absolute maximum AlphaL (pos or neg.) in the outer 25% of the specified rotor
    void HeliAero::calcAlphaLMax(physics_Num &Max, TRotor &Rotor)
    {
        Max = static_cast<physics_Num>(0.0);

        /*with Rotor.m_sector[SS] do*/
        for(int SS = 1; SS < 5; SS++)
        {
            TRotorSector &sector = Rotor.m_sector[SS];

            for(int RR = 15; RR < 21; RR++)
            {
                TSurface &arc = sector.m_arc[RR];

                physics_Num absArcAlphaL = Math<physics_Num>::Abs(arc.m_alphaL);
                if(absArcAlphaL > Max)
                {
                    Max = absArcAlphaL; // pass any higher value into AlphaLMax
                }
            } // for rr
        } // with Sector[ss]
    } // CalcAlphaLMax

    //**REDUNDANT??
    // Get the files with the climb and translation rate correction for induced flow
    void HeliAero::getClimbTransLookup(String FName, LookupArray &Table)
    {
        /*
            file  ClimbTransitionData IFile;
            ClimbTransitionData DPoint;
            std::string IFileName;
            int N;
            AssignFile(IFile, FName); //assign the filename to the file
            Reset(IFile); //open the file
            N = 0;
            while (!EOF(IFile))   //read its full contents
            {
                Read(IFile, DPoint);//read a line of data
                Table[N].m_rate = DPoint.m_rate; //transfer data from temp read record to the table
                Table[N].m_factor = DPoint.m_factor;
                N++;
            }//while not EOF
            CloseFile(IFile); //close the data file
            for (N = 0; N != 198; N++)
            {  //calculate the slopes in the lookup to save arithmetic during lookup
                Table[N].m_dFacByRate = (Table[N + 1].m_factor - Table[N].m_factor) / (Table[N +
            1].m_rate - Table[N].m_rate);
            }
            */
    } // GetClimbTransLookup

    // note to solve issues to do with truncation around zero the lookup runs from 0=
    // -Math<physics_Num>::pi() to 400= Math<physics_Num>::pi()
    void HeliAero::getFoilLookup(const String &IFileName, TFoilTable &AFoilLookup)
    {
        try
        {
            WP_DEBUG_TRACE;

#if defined WP_PLATFORM_WIN32
            auto fileNamePath =
                getDataPath() + L"/" +
                StringUtil::toUTF8to16(IFileName); // set the full path aerofoil data filename
            WP_LOG("FBHeliAero opening : " + StringUtil::toUTF16to8( fileNamePath ));

            SmartPtr<IStream> dataStream;
            // std::fstream stream( fileNamePath, std::fstream::in | std::fstream::binary );
            // Ogre::DataStreamPtr dataStream =
            //    Ogre::DataStreamPtr( OGRE_NEW Ogre::FileStreamDataStream( &stream, false ) );
#else
            std::wstring fileNamePath =
                getDataPath() + L"/" +
                StringUtil::toUTF8to16(IFileName); // set the full path aerofoil data filename
            WP_LOG("FBHeliAero opening : " + StringUtil::toUTF16to8( fileNamePath ));

            SmartPtr<IStream> dataStream;
            if(auto applicationManager = core::IApplicationManager::instance())
            {
                if(auto fileSystem = applicationManager->getFileSystem())
                {
                    dataStream = fileSystem->open(StringUtil::toUTF16to8(fileNamePath), true,
                                                  true, false);
                }
            }
#endif

            if(dataStream && dataStream->isReadable())
            {
                constexpr size_t maxStreamSize = 401 * sizeof(TFoilData);

                size_t fileSize = dataStream->size();
                size_t streamSize = Math<size_t>::clamp(fileSize, 0, maxStreamSize);

                // size_t arraySize = streamSize / sizeof(f32);
                size_t numPoints = streamSize / sizeof(TFoilData);

                std::vector<TFoilData> buffer;
                buffer.resize(maxStreamSize);

                auto bytesRead = dataStream->read(&buffer[0], maxStreamSize);
                if(bytesRead > 0)
                {
                    for(size_t N = 0; N < 398; N++)
                    {
                        TFoilData &DPoint = buffer[N];
                        AFoilLookup[N].m_alpha = DPoint.m_alpha;
                        AFoilLookup[N].m_cl = DPoint.m_cl;
                        AFoilLookup[N].m_cd = DPoint.m_cd;
                        AFoilLookup[N].m_cm = DPoint.m_cm;
                    }
                }
                else
                {
                    WP_LOG("No airfoil data read.");
                }

                for(size_t N = 0; N < 398; N++)
                {
                    auto &p0 = AFoilLookup[N];
                    auto &p1 = AFoilLookup[N + 1];

                    WP_ASSERT(Math<physics_Num>::isFinite( p0.m_alpha ));
                    WP_ASSERT(Math<physics_Num>::isFinite( p0.m_cl ));
                    WP_ASSERT(Math<physics_Num>::isFinite( p0.m_cd ));
                    WP_ASSERT(Math<physics_Num>::isFinite( p0.m_cm ));

                    WP_ASSERT(Math<physics_Num>::isFinite( p1.m_alpha ));
                    WP_ASSERT(Math<physics_Num>::isFinite( p1.m_cl ));
                    WP_ASSERT(Math<physics_Num>::isFinite( p1.m_cd ));
                    WP_ASSERT(Math<physics_Num>::isFinite( p1.m_cm ));

                    // calculate the slopes in the lookup to save arithmetic during lookup
                    p0.m_dClByAlpha = (p1.m_cl - p0.m_cl) / (p1.m_alpha - p0.m_alpha);
                    p0.m_dCdByAlpha = (p1.m_cd - p0.m_cd) / (p1.m_alpha - p0.m_alpha);
                    p0.m_dCmByAlpha = (p1.m_cm - p0.m_cm) / (p1.m_alpha - p0.m_alpha);

                    WP_ASSERT(Math<physics_Num>::isFinite( p0.m_dClByAlpha ));
                    WP_ASSERT(Math<physics_Num>::isFinite( p0.m_dCdByAlpha ));
                    WP_ASSERT(Math<physics_Num>::isFinite( p0.m_dCmByAlpha ));
                }
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    // takes CL and CD data from
    void HeliAero::readCLandD(TSurface &S, TFoilTable &AFoilLookup)
    {
        const real_dNum DA =
            200.0 / Math<real_dNum>::pi(); // this is the step size for Alpha in the lookup
        const real_dNum IDA = 1.0 / DA; // this reciprocal saves use of divide in the lookup

        int AA = 0;

        WP_ASSERT(Math<physics_Num>::isFinite( S.m_alphaL ));

        // a local to hold the passes AlphaL in a single to try to resolve the Trunc issue
        real_dNum Alpha = S.m_alphaL + Math<real_dNum>::pi();
        // allow for the lookup running from 0= -Math<physics_Num>::pi() to 400=
        // +Math<physics_Num>::pi()

        // optimisation estimate a start value for AA
        AA = ((Alpha / (Math<real_dNum>::pi() * 2.0)) * 400.0) - 1;

        while(((static_cast<real_dNum>(AA) * IDA) < Alpha) && (AA < 398))
        {
            AA++; // to avoid trunc we ramp AA to find the value required!
        }

        AA--;

        AA = Math<int>::clamp(AA, 0, 398);

        real_dNum deltaA = Alpha - (static_cast<real_dNum>(AA) * IDA); // calc small difference

        const TFoilLookup &foilLookup = AFoilLookup[AA];

        WP_ASSERT(Math<physics_Num>::isFinite( foilLookup.m_cl ));
        WP_ASSERT(Math<physics_Num>::isFinite( foilLookup.m_cd ));
        WP_ASSERT(Math<physics_Num>::isFinite( foilLookup.m_dClByAlpha ));
        WP_ASSERT(Math<physics_Num>::isFinite( foilLookup.m_dCdByAlpha ));

        S.m_cl = foilLookup.m_cl + foilLookup.m_dClByAlpha * deltaA;
        S.m_cl = S.m_cl * m_clFiddle;

        S.m_cd = foilLookup.m_cd + foilLookup.m_dCdByAlpha * deltaA;
        S.m_cd = S.m_cd * m_cdFiddle; // note a 'fiddle factor added here for experimentation

        WP_ASSERT(Math<physics_Num>::isFinite( S.m_cl ));
        WP_ASSERT(Math<physics_Num>::isFinite( S.m_cd ));
    }

    // calculate the lift and drag on each surface element
    void HeliAero::calcLandD(TSurface &S)
    {
        const real_dNum K1 = 0.5 * m_roair; // half air density as a constant

        WP_ASSERT(Math<physics_Num>::isFinite( S.m_area ));
        WP_ASSERT(Math<physics_Num>::isFinite( S.getFlowSpeed() ));

        // stagnation pressure x Area = 0.5*Ro* V^2 *Area
        real_dNum QA = K1 * S.m_area * Math<real_dNum>::Sqr(S.getFlowSpeed());

        S.setLift(QA * S.m_cl); // lift = 0.5*Ro* Area *V^2 *CL
        S.m_drag = QA * S.m_cd; // Drag = 0.5*Ro* Area *V^2 *CD

        WP_ASSERT(Math<physics_Num>::isFinite( S.getLift() ));
        WP_ASSERT(Math<physics_Num>::isFinite( S.m_drag ));
    }

    void HeliAero::getForcesFromFlow(TRotor &Rotor)
    {
        for(size_t SS = 1; SS < Rotor.m_sector.size(); SS++)
        {
            TRotorSector &sector = Rotor.m_sector[SS];

            /*with Rotor.m_sector[SS] do*/
            for(size_t RR = 1; RR < sector.m_arc.size(); RR++)
            {
                TSurface &surface = sector.m_arc[RR];

                calcAttack(surface);
                readCLandD(surface, Rotor.getAFoilLookup());
                calcLandD(surface);
            } // for SS and RR
        }
    } // GetForcesFromFlow

    // for flybarless we need to zero the FB forces etc regardless
    void HeliAero::zeroForcesAndMoments(TRotor &Rotor)
    {
        /*with Rotor do*/
        {
            Rotor.m_hubForce.x = 0;
            Rotor.m_hubForce.y = 0;
            Rotor.m_hubForce.z = 0; // zero the total force
            Rotor.m_forceMoments.x = 0;
            Rotor.m_forceMoments.y = 0;
            Rotor.m_forceMoments.z = 0; // zero all the force moments ready for new summation
        } // with Rotor
    }

    // note: Revised the scaling factor for the pitch and roll moments and in-plane forces
    // this is to take account of considered blade positions to be at the cyclic maxima
    //  calculates the total force vector at hub centre
    void HeliAero::sumForcesandMoments(TRotor &Rotor)
    {
        // int SS, RR;                                               //the moment of forces about x,
        // y and z axes

        /*with Rotor do*/
        {
            Rotor.m_hubForce.x = 0;
            Rotor.m_hubForce.y = 0;
            Rotor.m_hubForce.z = 0; // zero the total force
            Rotor.m_forceMoments.x = 0;
            Rotor.m_forceMoments.y = 0;
            Rotor.m_forceMoments.z = 0; // zero all the force moments ready for new summation

            /*with Sector[SS] do*/
            for(int SS = 1; SS < 5; SS++)
            {
                TRotorSector &rotorSector = Rotor.m_sector[SS];

                /*with Arc[RR] do*/
                for(int RR = 1; RR < 21; RR++)
                {
                    TSurface &surface = rotorSector.m_arc[RR];

                    //'local' force (lift and drag) on the element (allowing for tip losses losing
                    // the lift of the last 5% of blade)
                    if(RR != 20)
                    {
                        surface.m_lForce = VSum(VScale(surface.m_liftVec, surface.getLift()),
                                                VScale(surface.m_flowVec, surface.m_drag));
                    }
                    else
                    {
                        surface.m_lForce = VScale(surface.m_flowVec, surface.m_drag);
                    }

                    Rotor.m_hubForce =
                        VSum(Rotor.m_hubForce,
                             surface.m_lForce); // sum in the contribution to hub centre force
                    Rotor.m_forceMoments =
                        VSum(Rotor.m_forceMoments, VCross(surface.m_centre, surface.m_lForce));
                    // get all the force moments summed here (powerful or what!!)
                } // for RR
            } // for SS

            auto scale =
                static_cast<physics_Num>(Rotor.m_blades) / static_cast<physics_Num>(4.0);
            Rotor.m_hubForce = VScale(Rotor.m_hubForce, scale);
            // apply the scaling of all the forces needed because basic calc uses 4 blades
            Rotor.m_forceMoments = VScale(Rotor.m_forceMoments, scale);
            // apply the scaling of all the forces needed because basic calc uses 4 blades

            // now do further reductions to do with the Sector positions being at cyclic maxima
            // Rotor.ForceMoments.x=0.8* Rotor.ForceMoments.x;//note that only the x and z force
            // moments are reduced this way Rotor.ForceMoments.z=0.8* Rotor.ForceMoments.z;//the y
            // component is the torque about the shaft for which no down-scaling is needed
            // Rotor.HubForce.x=0.8* Rotor.HubForce.x;
            // Rotor.HubForce.z = 0.8 * Rotor.HubForce.z;
        } // with Rotor
    }

    // called if the DeltaT is so large the heli gets out of sync with the aerodynamic rotors
    void HeliAero::alignFrames()
    {
        physics_Vec RotVec;
        m_tailRotor->m_frame.m_xAxis =
            VScale(m_body.m_frame.m_yAxis,
                   -1.0); // the x axis of the tail rotor is pointing downwards in the body frame
        m_tailRotor->m_frame.m_yAxis =
            m_body.m_frame
                  .m_xAxis; // the y axis of the tail rotor is pointing left in the body frame
        m_tailRotor->m_frame.m_zAxis =
            m_body.m_frame.m_zAxis; // the tail rotor's Z axis is parallel to the body's
        /*with RotorHead do*/
        {
            // sort out the mainshaft frame WRT the Body frame allowing for rake and tilt
            RotVec.x = m_rotorHead->m_shaftRake;
            RotVec.y = 0;
            RotVec.z = m_rotorHead->m_shaftTilt;
            // feed the rotation components into RotVec ready to call RotateFrame

            m_rotorHead->m_shaftFrame = RotateFrame(
                m_body.m_frame, RotVec); // sort out the mainshaft frame WRT the Body frame
            m_rotorHead->m_linkage.m_swashFrame =
                m_rotorHead
                ->m_shaftFrame; // initialize the Swashplate frame aligned to the main shaft

            // Sync the flybar and main rotor with the shaft
            m_rotorHead->m_flyBar.m_frame = m_rotorHead->m_shaftFrame;
            m_rotorHead->m_mainRotor.m_frame = m_rotorHead->m_shaftFrame;
        } // with RotorHead
    }

    // calculates the FoRs of the of the mainshaft, swashplate, main rotor, and flybar
    void HeliAero::sortFramesAndLinks()
    {
        int ExLoc; // exception location

        physics_Vec RotVec; // for storing Rotation info for RotateFrame call
        ExLoc = 0;
        try
        {
            // Tail rotor orientation
            m_tailRotor->m_frame.m_xAxis = VScale(
                m_body.m_frame.m_yAxis,
                -1); // the x axis of the tail rotor is pointing downwards in the body frame
            m_tailRotor->m_frame.m_yAxis =
                m_body.m_frame
                      .m_xAxis; // the y axis of the tail rotor is pointing left in the body frame
            m_tailRotor->m_frame.m_zAxis =
                m_body.m_frame.m_zAxis; // the tail rotor's Z axis is parallel to the body's
            /*with RotorHead do*/
            {
                // sort out the mainshaft frame WRT the Body frame allowing for rake and tilt
                RotVec.x = m_rotorHead->m_shaftRake;
                RotVec.y = 0;
                RotVec.z = m_rotorHead->m_shaftTilt;
                // feed the rotation components into RotVec ready to call RotateFrame
                m_rotorHead->m_shaftFrame = RotateFrame(m_body.m_frame, RotVec);
                ExLoc = 1;
                // sort out swash frame by rotating the mainshaft frame by swash deflection angles
                RotVec.x = m_rotorHead->m_linkage.m_swashEleAngle;
                RotVec.y = 0;
                RotVec.z = m_rotorHead->m_linkage.m_swashAilAngle;
                m_rotorHead->m_linkage.m_swashFrame =
                    RotateFrame(m_rotorHead->m_shaftFrame, RotVec);
                ExLoc = 2;
                // sort the elevator and aileron axis deflections of the flybar WRT the mainshaft
                m_rotorHead->m_flyBar.m_eleAngle =
                    VDot(m_rotorHead->m_flyBar.m_frame.m_yAxis, m_rotorHead->m_shaftFrame.m_zAxis);
                m_rotorHead->m_flyBar.m_ailAngle =
                    -VDot(m_rotorHead->m_flyBar.m_frame.m_yAxis,
                          m_rotorHead->m_shaftFrame.m_xAxis); // note a sign change here
                ExLoc = 3;
                // sort out the elevator and aileron axis deflections of the main rotor WRT the
                // mainshaft
                m_rotorHead->m_mainRotor.m_eleAngle = VDot(m_rotorHead->m_mainRotor.m_frame.m_yAxis,
                                                           m_rotorHead->m_shaftFrame.m_zAxis);
                m_rotorHead->m_mainRotor.m_ailAngle =
                    -VDot(m_rotorHead->m_mainRotor.m_frame.m_yAxis,
                          m_rotorHead->m_shaftFrame.m_xAxis); // note a sign change here
                ExLoc = 4;
                // calc the resulting cyclic pitches for the flybar
                m_rotorHead->m_flyBar.m_eleCyclic =
                    (m_rotorHead->m_linkage.m_swashEleAngle - m_rotorHead->m_flyBar.m_eleAngle) *
                    m_rotorHead->m_linkage.m_swashToFBMix;
                m_rotorHead->m_flyBar.m_ailCyclic =
                    (m_rotorHead->m_linkage.m_swashAilAngle - m_rotorHead->m_flyBar.m_ailAngle) *
                    m_rotorHead->m_linkage.m_swashToFBMix;
                ExLoc = 5;
                // calc the Main rotor cyclic pitches
                m_rotorHead->m_mainRotor.m_eleCyclic =
                    (m_rotorHead->m_linkage.m_swashEleAngle -
                     m_rotorHead->m_mainRotor.m_eleAngle) *
                    m_rotorHead->m_linkage.m_swashToMainMix +
                    (m_rotorHead->m_flyBar.m_eleAngle - m_rotorHead->m_mainRotor.m_eleAngle) *
                    m_rotorHead->m_linkage.m_fbToMainMix;
                m_rotorHead->m_mainRotor.m_ailCyclic =
                    (m_rotorHead->m_linkage.m_swashAilAngle -
                     m_rotorHead->m_mainRotor.m_ailAngle) *
                    m_rotorHead->m_linkage.m_swashToMainMix +
                    (m_rotorHead->m_flyBar.m_ailAngle - m_rotorHead->m_mainRotor.m_ailAngle) *
                    m_rotorHead->m_linkage.m_fbToMainMix;
            } // with RotorHead
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    } // SortFramesAndLinks

    // this procedure corrects the teeter FC and DC for frame rate issues and for low rotor RPMs
    // AngMom is the angular momentum of the main rotor
    void HeliAero::correctTeeterFactors(physics_Num &FC, physics_Num &DC, physics_Num FrameTime,
                                        physics_Num RPM, physics_Num MaxRPM, physics_Num AngMom,
                                        const physics_Vec &MoI)
    {
        real_dNum LowerMoI;
        real_dNum FCMax, DCMax; // the max FC and DC for the frame rate
        real_dNum FCLimRH; // upper limit to FC based on rotor angular momentum
        real_dNum FT; // a temp to use to trap unrealistic frame times
        // PDouble TFactor; //The reduction factor for the teeter stiffness with RPM

        FT = 1.5E-3;

        if(FrameTime > FT)
        {
            FT = FrameTime; // apply a lower limit to the usable frame time of 1.5ms
        }

        // the max resonant frequency usable for this framerate
        // take the max resonant frequency as having 25 frame time per cycle (conservative!)
        real_dNum Fres = 0.04 / FT;
        LowerMoI = MoI.x;

        if(MoI.z < LowerMoI)
        {
            LowerMoI = MoI.z;
            // establish which is the lower and thus critical MoI between pitch and roll axes
        }

        FCMax = LowerMoI *
                Math<real_dNum>::Sqr(
                    2.0 * Math<real_dNum>::pi() *
                    Fres); // calculate the force constant for this resonant frequency and this MoI
        DCMax = static_cast<physics_Num>(2.0) *
                Math<physics_Num>::Sqrt(
                    LowerMoI * FCMax); // calculate the damping constant assuming critical damping
        if(FC > FCMax)
        {
            FC = FCMax;
            DC = DCMax;
        }

        FCLimRH = AngMom * Fres;
        if(FC > FCLimRH)
        {
            FC = FCLimRH; // apply the rotor head limit to the force constant
            DC = static_cast<physics_Num>(2.0) * Math<physics_Num>::Sqrt(LowerMoI * FC);
            // and calculate the damping for this FC based on the inertia of the body (rather than
            // the rotor AngMom)
        }

        // if RPM < 0.8*MaxRPM then
        // Begin
        //  TFactor:= 1 - 1.2*(0.8 - RPM/MaxRPM); //calc the reduction factor for the teeter constant
        //   if TFactor <0.1 then TFactor:= 0.1;
        //   FC:= FC*TFactor; //apply the reduction factor to the force constant
        //   DC:= 2*Math<physics_Num>::Sqrt(LowerMoI*FC); //calculate the appropriate reduced damping
        //   constant assuming critical damping
        // End;
    }

    // combines the Precession of the flybar and the main rotor and sorts out the teeter forces and
    // kickback
    void HeliAero::precessAndTeeter()
    {
        physics_Vec RotVec;
        physics_Num TorqueLimit; // used to give a model dependent teeter force limit

        // The teeterFC and TeeterDC after RPM correction for low head speeds
        // PFloat TFactor;
        physics_Num TFC;
        physics_Num TDC;

        physics_Num PrecessionLimit; // used to set the maximum permitted precession rate

        TRotorHead &rotorHead = *m_rotorHead;
        TRotor &mainRotor = rotorHead.m_mainRotor;

        cageRotor(rotorHead.m_mainRotor, rotorHead.m_shaftFrame, 0.3);
        // limit the angle between shaft and main rotor to 18 degrees
        PrecessionLimit = (static_cast<physics_Num>(14.0) / static_cast<physics_Num>(200.0)) *
                          mainRotor.getOmega();
        // limit to precession rate in Radians/s (14 Rad/s = 800 deg/s or about 3 degess in a single
        // physics frame)
        TFC = rotorHead.m_teeterForceConstant;
        // copy the unmodified teeter constants to temporary variable for correction
        TDC = rotorHead.m_teeterDampingConstant;

        // CorrectTeeterFactors(TFC, TDC, MeanDeltaT, PDouble(9.55) * mainRotor.getOmega(),
        // GovMaxHeadRPM.m_sVal, mainRotor.m_angularMomentum, ModelMoI.m_vVal);
        correctTeeterFactors(TFC, TDC, m_thisDeltaT, 9.55 * mainRotor.getOmega(),
                             m_govMaxHeadRPM.m_sVal, mainRotor.m_angularMomentum,
                             m_modelMoI.m_vVal);

        // now calculate the teeter force constants for the current headspeed
        if(mainRotor.getOmega() <
           0.005 *
           m_govMaxHeadRPM
           .m_sVal) // make the rotor follow the mainshaft once RPM below 5% of the maximum
        {
            mainRotor.m_frame = rotorHead.m_shaftFrame; // force the rotor frame to the shaft frame
            mainRotor.m_eleAngle = 0;
            mainRotor.m_ailAngle = 0;
            rotorHead.m_torques.x = 0;
            rotorHead.m_torques.z = 0;
        }
        else
        {
            /*with RotorHead->m_mainRotor do*/
            {
                TRotor &mr = m_rotorHead->m_mainRotor;

                if(mr.getOmega() < 10.0)
                {
                    mr.m_angularMomentum = mr.m_momentOfInertia * 10.0;
                }
                else
                {
                    mr.m_angularMomentum = mr.m_momentOfInertia * mr.getOmega();
                    // angular momentum of the rotor not allowed to drop too low at small Omega
                }

                mr.m_precessionRate.x =
                    (-mr.m_forceMoments.z + m_rotorHead->m_torques.z) / mr.m_angularMomentum -
                    0.01 * mr.m_eleAngle; // note this is for a clockwise rotor!!

                if(mr.m_precessionRate.x > PrecessionLimit)
                    mr.m_precessionRate.x = PrecessionLimit; // trap silly values
                if(mr.m_precessionRate.x < -PrecessionLimit)
                    mr.m_precessionRate.x = -PrecessionLimit;

                mr.m_precessionRate.z =
                    (mr.m_forceMoments.x - m_rotorHead->m_torques.x) / mr.m_angularMomentum -
                    0.01 * mr.m_ailAngle; // again this is right for a clockwise rotor

                if(mr.m_precessionRate.z > PrecessionLimit)
                    mr.m_precessionRate.z = PrecessionLimit; // trap silly values
                if(mr.m_precessionRate.z < -PrecessionLimit)
                    mr.m_precessionRate.z = -PrecessionLimit;

                RotVec.x = mr.m_precessionRate.x * m_thisDeltaT;
                RotVec.y = m_body.m_angularVelocity.y * m_thisDeltaT;
                RotVec.z = mr.m_precessionRate.z * m_thisDeltaT;
                mr.m_frame = RotateFrame(mr.m_frame, RotVec);
                // mr.m_frame = RotateFrame(mr.m_frame, mr.m_precessionRate.x*ThisDeltaT,
                // Body.m_angularVelocity.y*ThisDeltaT, mr.m_precessionRate.z*ThisDeltaT); //apply
                // the change in attitude to the Frame note that the Body yaw rate is used in this
                // rotation
            } // with MainRotor
            /*with RotorHead do*/
            {
                TRotorHead &rh = rotorHead;

                TorqueLimit = 0.2 * rh.m_teeterForceConstant;
                // limit the teeter force to that given by the teeter spring at an angular deflection
                // of 0.2 rad. (12 degrees)
                rh.m_torques.x =
                    rh.m_mainRotor.m_eleAngle * TFC -
                    (m_modelAngularVelocity.x - rh.m_mainRotor.m_precessionRate.x) *
                    TDC; // The elevator angular deflection of the head give x axis head torque
                rh.m_torques.z =
                    rh.m_mainRotor.m_ailAngle * TFC -
                    (m_modelAngularVelocity.z - rh.m_mainRotor.m_precessionRate.z) * TDC;
                if(rh.m_torques.x > TorqueLimit)
                    rh.m_torques.x = TorqueLimit;
                if(rh.m_torques.x < -TorqueLimit)
                    rh.m_torques.x = -TorqueLimit; // apply a basic limit to the teeter torque
                if(rh.m_torques.z > TorqueLimit)
                    rh.m_torques.z = TorqueLimit;
                if(rh.m_torques.z < -TorqueLimit)
                    rh.m_torques.z = -TorqueLimit;
            } // with RotorHead
        } // end if rotor speed >10% of max

        cageRotor(m_rotorHead->m_mainRotor, m_rotorHead->m_shaftFrame,
                  0.3);
        // repeat the caging of the angle between shaft and main rotor to 18 degrees
        cageRotor(m_rotorHead->m_flyBar, m_rotorHead->m_shaftFrame,
                  0.5);
        // cage the flybar axis to be within 30 degrees of the shaft axis

        // with RotorHead->m_flyBar do
        {
            TRotor &fb = m_rotorHead->m_flyBar;

            fb.m_angularMomentum =
                fb.m_momentOfInertia * fb.getOmega(); // angular momentum of the rotor about
            fb.m_precessionRate.x =
                -fb.m_forceMoments.z / fb.m_angularMomentum; // note this is for a clockwise rotor!!

            if(fb.m_precessionRate.x > PrecessionLimit)
                fb.m_precessionRate.x = PrecessionLimit; // trap silly values
            if(fb.m_precessionRate.x < -PrecessionLimit)
                fb.m_precessionRate.x = -PrecessionLimit;

            fb.m_precessionRate.z =
                fb.m_forceMoments.x /
                fb.m_angularMomentum; // again this is right for a clockwise rotor
            if(fb.m_precessionRate.z > PrecessionLimit)
                fb.m_precessionRate.z = PrecessionLimit; // trap silly values

            if(fb.m_precessionRate.z < -PrecessionLimit)
                fb.m_precessionRate.z = -PrecessionLimit;

            RotVec.x = fb.m_precessionRate.x * m_thisDeltaT;
            RotVec.y = m_body.m_angularVelocity.y * m_thisDeltaT;
            RotVec.z = fb.m_precessionRate.z * m_thisDeltaT;
            fb.m_frame =
                RotateFrame(fb.m_frame, RotVec); // apply the change in attitude to the Frame
            // note that the Body yaw rate is used in this rotation
        } // with FlyBar
    }

    // need to ensure the the precession at low rpm (when AngMom is low) are delt with and that
    // teeter forces are not unrealistic when aero forces are low combines the Precession of the
    // flybar and the main rotor and sorts out the teeter forces and kickback
    void HeliAero::experimentalPrecessAndTeeter()
    {
        // int Loop;//counter for inner calculation loop     //NOTE: KICKBACK REMOVED BECAUSE OF
        // INSTABILITY - NEEDS INVESTIGATION

        // Vec RotVec;
        ///*with RotorHead->m_mainRotor do*/
        //{
        //	for (Loop = 1; Loop != 40; Loop++)
        //	{
        //		//set a minimum angular momentum to ensure numerical stability
        //		if(Omega < 120)
        //			AngMom = MomOfI * 120;
        //		else
        //			AngMom = MomOfI * Omega;//angular momentum of the rotor about
        //		PrecessionRate.x = 0.95 * PrecessionRate.x + 0.5 * (-ForceMoments.z + 0.0 *
        // RotorHead->Torques.z) / AngMom;//note this is for a clockwise rotor!!

        //   //note that the Body yaw rate is used in this rotation
        //	}//with MainRotor
        //  /*with RotorHead do*/
        //	{
        //		Torques.x = 0.95 * Torques.x + 0.5 * (MainRotor.EleAngle * TeeterFC -
        //(ModelAngularVelocity.x - MainRotor.PrecessionRate.x) * TeeterDC);//The elevator angular
        // deflection of the head give x axis head torque 		Torques.z = 0.95 * Torques.z + 0.5 *
        //(MainRotor.AilAngle * TeeterFC - (ModelAngularVelocity.z - MainRotor.PrecessionRate.z) *
        // TeeterDC); 		if(Torques.x > 1) 			Torques.x = 1; 		if(Torques.x < -1)
        // Torques.x = -1;//apply a basic limit to the teeter torque 		if(Torques.z > 1)
        // Torques.z = 1; 		if(Torques.z < -1) 			Torques.z = -1;
        //	}//for loop
        //	RotVec.x = PrecessionRate.x * ThisDeltaT;
        //	RotVec.y = Body.m_angularVelocity.y * ThisDeltaT;
        //	RotVec.z = PrecessionRate.z * ThisDeltaT;
        //	Frame = RotateFrame(Frame, RotVec); //apply the change in attitude to the Frame
        //}//with rotorhead.mainrotor
        ///*with RotorHead->m_flyBar do*/
        //{
        //	AngMom = MomOfI * Omega;//angular momentum of the rotor about
        //	PrecessionRate.x = -ForceMoments.z / AngMom;//note this is for a clockwise rotor!!
        //	PrecessionRate.z = ForceMoments.x / AngMom;//again this is right for a clockwise rotor
        //	RotVec.x = PrecessionRate.x * ThisDeltaT;
        //	RotVec.y = Body.m_angularVelocity.y * ThisDeltaT;
        //	RotVec.z = PrecessionRate.z * ThisDeltaT;
        //	Frame = RotateFrame(Frame, RotVec); //apply the change in attitude to the Frame
        //	//note that the Body yaw rate is used in this rotation
        //}//with FlyBar
    }

    // updates the Angular momentum value and calculates precession rate and angle
    void HeliAero::doPrecession(physics_Num dt, TRotor &Rotor)
    {
        /*with Rotor do*/
        Rotor.m_angularMomentum =
            Rotor.m_momentOfInertia * Rotor.getOmega(); // angular momentum of the rotor about
        Rotor.m_precessionRate.x = -Rotor.m_forceMoments.z /
                                   Rotor.m_angularMomentum; // note this is for a clockwise rotor!!
        Rotor.m_precessionRate.z =
            Rotor.m_forceMoments.x /
            Rotor.m_angularMomentum; // again this is right for a clockwise rotor

        // hack for ccw rotor
        // Rotor.m_precessionRate.x = -Rotor.m_forceMoments.z / -Rotor.m_angularMomentum;
        // Rotor.m_precessionRate.z = Rotor.m_forceMoments.x / -Rotor.m_angularMomentum;

        physics_Vec RotVec;
        RotVec.x = Rotor.m_precessionRate.x * dt;
        RotVec.y = m_body.m_angularVelocity.y * dt;
        RotVec.z = Rotor.m_precessionRate.z * dt;
        Rotor.m_frame =
            RotateFrame(Rotor.m_frame, RotVec); // apply the change in attitude to the Frame
        // note that the Body yaw rate is used in this rotation
        // with Rotor
    }

    // this uses the angular deflection of the main rotor WRT shaft to calc the teeter forces
    // needs to add the teeter forces to the ForceMoments on the MainRotor
    void HeliAero::calcTeeterForces(TRotorHead &Head)
    {
        // The elevator angular deflection of the head give x axis head torque
        Head.m_torques.x = Head.m_mainRotor.m_eleAngle * Head.m_teeterForceConstant -
                           (m_modelAngularVelocity.x - Head.m_mainRotor.m_precessionRate.x) *
                           Head.m_teeterDampingConstant;
        Head.m_torques.z = Head.m_mainRotor.m_ailAngle * Head.m_teeterForceConstant -
                           (m_modelAngularVelocity.z - Head.m_mainRotor.m_precessionRate.z) *
                           Head.m_teeterDampingConstant;
    }

    void HeliAero::initTailLinkage(TailLinkage &TailLinkage)
    {
        ///*with TailLinkage do*/
        {
            TailLinkage.m_trim =
                m_tailPitchTrim.m_sVal * Math<real_dNum>::pi() / 180.0; // degree tail trim
            TailLinkage.m_leftThrow = m_tailMaxPitch.m_sVal * Math<real_dNum>::pi() / 180.0;
            TailLinkage.m_rightThrow = m_tailMaxPitch.m_sVal * Math<real_dNum>::pi() / 180.0;
            TailLinkage.m_output = 0;
        } // With tail linkage
    } // InitTailLinkage

    physics_Num HeliAero::doTailLinkage(physics_Num InSig, TailLinkage &TL)
    {
        if(InSig > 0)
        {
            return TL.m_trim + TL.m_rightThrow * InSig;
        }

        return TL.m_trim + TL.m_leftThrow * InSig;
    } // TailLinkage

    void HeliAero::initServo(Servo &Servo, physics_Num SecPer60)
    {
        /*with Servo do*/
        {
            Servo.m_slewRate = static_cast<physics_Num>(1.0) / SecPer60;
            // was 12;//the fraction of full travel covered per second
            Servo.m_accelerationTime = static_cast<physics_Num>(5E-3);
            // the characteristic time to reach full speed
            Servo.m_output = 0;
        }
    } // initServo

    // in this simple version we ignore acceleration and deceleration times
    physics_Num HeliAero::servo(physics_Num InSig, physics_Num dt, Servo &ThisServo)
    {
        physics_Num result;
        physics_Num Delta, MaxStep;
        /*with ThisServo do*/
        {
            ThisServo.m_input = InSig; // save passed input signal
            MaxStep =
                ThisServo.m_slewRate * dt; // calculate the maximum movement based on Slew and dt
            Delta = ThisServo.m_input - ThisServo.m_output; // find out how far servo needs to go
            if(Delta > MaxStep)
                Delta = MaxStep; // limit this to the maximum in this time
            if(Delta < -MaxStep)
                Delta = -MaxStep;
            ThisServo.m_output = ThisServo.m_output + Delta; // and add the limited step to the
            // output
            if(ThisServo.m_output > 1)
                ThisServo.m_output = 1; // limit the throw to +-1
            if(ThisServo.m_output < -1)
                ThisServo.m_output = -1;
            result = ThisServo.m_output; // transfer the output
        } // With this servo
        return result;
    } // Servo

    void HeliAero::initGearTrain(GearTrain &Gears)
    {
        Gears.setEngineMainRatio(m_mainGearTeeth.m_sVal /
                                 m_pinionGearTeeth.m_sVal); // now calculated from the teeth ratios
        Gears.m_mainTailRatio = m_tailGearing.m_sVal;
        Gears.m_engineTailRatio = Gears.getEngineMainRatio() / Gears.m_mainTailRatio;
        Gears.m_drivenTail = m_drivenTail.m_bVal; // note the rather tricky common name of the
        // GearTrain member and the TParam!!
        Gears.m_tailDriveLoss = static_cast<physics_Num>(0.1);
        // fractional loss factor i.e 0.1 = 10% loss of torque/power down drive line
        // The assumed friction of the rotor head is scaled from the blade weight and length
        // and gives a torque about 10% of the torque from one blade under gravity (very rough!! but
        // no need to be precise) based on Ash's power loss tests we have the following friction loss
        // function
        Gears.m_headFriction =
            m_bearingFriction.m_sVal * m_mainBladeLength.m_sVal * m_mainBladeWeight.m_sVal;
        // the torque (in N.m at the head) needed to overcome static friction of drive train
    }

    // this is the new one using the clutch engine code
    void HeliAero::doICEnergyBudget()
    {
        physics_Num effectiveFlyWheel; // the flywheel of the rotors reflected through the gears

        physics_Num flywheelOmega;
        physics_Num flyWheelRPM;
        physics_Num rotationalEnergy;

        physics_Num
            mainLoadTorque; // The load torque presented by the main rotor aero loads at the sprag
        physics_Num
            tailLoadTorque; // The load torque presented by the tail rotor aero loads at the sprag
        physics_Num totalLoadTorque; // The total load torque presented by the mechanics at the sprag

        if(m_rotorHead->m_hasFlyBar == true)
        {
            // with flybar account for the flybar inertia
            effectiveFlyWheel = (m_rotorHead->m_mainRotor.m_momentOfInertia +
                                 m_rotorHead->m_flyBar.m_momentOfInertia) /
                                Math<physics_Num>::Sqr(m_theGears.getEngineMainRatio()) +
                                m_tailRotor->m_momentOfInertia /
                                Math<physics_Num>::Sqr(m_theGears.m_engineTailRatio);
        }
        else
        {
            // without flybar do not add in FB inertia
            effectiveFlyWheel = (m_rotorHead->m_mainRotor.m_momentOfInertia) /
                                Math<physics_Num>::Sqr(m_theGears.getEngineMainRatio()) +
                                m_tailRotor->m_momentOfInertia /
                                Math<physics_Num>::Sqr(m_theGears.m_engineTailRatio);
        }

        flywheelOmega = (m_rotorHead->m_mainRotor.getOmega() + m_body.m_angularVelocity.y) *
                        m_theGears.getEngineMainRatio(); // added in the body rotation
        // flyWheelRPM = flywheelOmega * m_omegaToRPM;

        mainLoadTorque = (m_rotorHead->m_mainRotor.m_forceMoments.y + m_theGears.m_headFriction) /
                         m_theGears.getEngineMainRatio();
        tailLoadTorque = (static_cast<physics_Num>(1.0) + m_theGears.m_tailDriveLoss) *
                         m_tailRotor->m_forceMoments.y / m_theGears.m_engineTailRatio;
        totalLoadTorque = mainLoadTorque + tailLoadTorque;

        // note the following call returns an updated 'FlywheelSpeed', engine rpm etc
        AerodymanicsUtil::mainEngineClutchStep(flyWheelRPM, m_engineRPM, effectiveFlyWheel,
                                               totalLoadTorque, m_thisDeltaT,
                                               m_engineThrottlePosition, *m_theEngineClutch);
        // flywheelOmega = flyWheelRPM * m_rpmToOmega;
        rotationalEnergy = static_cast<physics_Num>(0.5) *
                           Math<physics_Num>::Sqr(flywheelOmega) * effectiveFlyWheel;

        if(rotationalEnergy < static_cast<physics_Num>(0))
            flywheelOmega = 1E-3;
        else
            flywheelOmega =
                Math<physics_Num>::Sqrt(static_cast<physics_Num>(2.0) * rotationalEnergy /
                                        effectiveFlyWheel); // calc. new flywheel speed

        // flyWheelRPM = flywheelOmega * m_omegaToRPM;

        physics_Num mainRotorOmega =
            (flywheelOmega / m_theGears.getEngineMainRatio()) - m_body.m_angularVelocity.y;
        m_rotorHead->m_mainRotor.setOmega(
            mainRotorOmega); // transfer the new speed to the rotors accounting for body rotation
        m_rotorHead->m_flyBar.setOmega(mainRotorOmega);
        m_tailRotor->setOmega(flywheelOmega / m_theGears.m_engineTailRatio);

        // now calc the main shaft torque based on what is left after the sprag throughput has the
        // tail torque subtracted
        EngineClutchUnit &rTheEngineClutch = *m_theEngineClutch;
        m_rotorHead->m_torques.y = (rTheEngineClutch.m_transmittedTorque - tailLoadTorque) *
                                   m_theGears.getEngineMainRatio();
    } // new DoICEnergyBudget;

    // electric flight EnergyBudget
    void HeliAero::doElecEnergyBudget()
    {
        physics_Num effectiveFlyWheel; // the flywheel of the rotors reflected through the gears

        physics_Num flywheelOmega;
        physics_Num flyWheelRPM;
        physics_Num rotationalEnergy;

        physics_Num
            mainLoadTorque; // The load torque presented by the main rotor aero loads at the sprag
        physics_Num
            tailLoadTorque; // The load torque presented by the tail rotor aero loads at the sprag
        physics_Num totalLoadTorque; // The total load torque presented by the mechanics at the sprag

        physics_Num omega1, omega2, angAcceleration;
        // used to calc the main rotor speed change this timestep (for torque calc)
        omega1 = m_rotorHead->m_mainRotor.getOmega(); // save the starting omega

        if(m_rotorHead->m_hasFlyBar == true)
        {
            // with flybar account for the flybar inertia
            effectiveFlyWheel = (m_rotorHead->m_mainRotor.m_momentOfInertia +
                                 m_rotorHead->m_flyBar.m_momentOfInertia) /
                                Math<physics_Num>::Sqr(m_theGears.getEngineMainRatio()) +
                                m_tailRotor->m_momentOfInertia /
                                Math<physics_Num>::Sqr(m_theGears.m_engineTailRatio);
        }
        else
        {
            // without flybar do not add in FB inertia
            effectiveFlyWheel = ((m_rotorHead->m_mainRotor.m_momentOfInertia) /
                                 Math<physics_Num>::Sqr(m_theGears.getEngineMainRatio())) +
                                (m_tailRotor->m_momentOfInertia /
                                 Math<physics_Num>::Sqr(m_theGears.m_engineTailRatio));
        }

        flywheelOmega = (m_rotorHead->m_mainRotor.getOmega() + m_body.m_angularVelocity.y) *
                        m_theGears.getEngineMainRatio(); // added in the body rotation
        flyWheelRPM = flywheelOmega * OmegaToRPM;

        mainLoadTorque = (m_rotorHead->m_mainRotor.m_forceMoments.y + m_theGears.m_headFriction) /
                         m_theGears.getEngineMainRatio();
        tailLoadTorque = (static_cast<physics_Num>(1.0) + m_theGears.m_tailDriveLoss) *
                         m_tailRotor->m_forceMoments.y / m_theGears.m_engineTailRatio;
        totalLoadTorque = mainLoadTorque + tailLoadTorque;

        // note the following call returns an updated 'FlywheelSpeed', engine rpm etc
        AerodymanicsUtil::mainEMotorStep(flyWheelRPM, m_motor->m_motorRpm, effectiveFlyWheel,
                                         totalLoadTorque, m_thisDeltaT, *m_motor);

        flywheelOmega = flyWheelRPM * RPMToOmega;
        rotationalEnergy = static_cast<physics_Num>(0.5) *
                           Math<physics_Num>::Sqr(flywheelOmega) * effectiveFlyWheel;

        if(rotationalEnergy < static_cast<physics_Num>(0))
            flywheelOmega = 1E-3;
        else
            flywheelOmega =
                Math<physics_Num>::Sqrt(static_cast<physics_Num>(2.0) * rotationalEnergy /
                                        effectiveFlyWheel); // calc. new flywheel speed

        flyWheelRPM = flywheelOmega * OmegaToRPM;

        physics_Num mainRotorOmega =
            (flywheelOmega / m_theGears.getEngineMainRatio()) - m_body.m_angularVelocity.y;
        m_rotorHead->m_mainRotor.setOmega(
            mainRotorOmega); // transfer the new speed to the rotors accounting for body rotation
        m_rotorHead->m_flyBar.setOmega(mainRotorOmega);
        m_tailRotor->setOmega(flywheelOmega / m_theGears.m_engineTailRatio);

        omega2 = m_rotorHead->m_mainRotor.getOmega(); // save the final omega
        angAcceleration =
            (omega2 - omega1) /
            m_thisDeltaT; // calculate the acceleration of the main rotor in this timestep

        // now calc the mainshaft torque as the aero loads + the inertial component from accel/decel
        // of head inertia
        m_topDownTorque = m_rotorHead->m_mainRotor.m_forceMoments.y +
                          angAcceleration * (m_rotorHead->m_mainRotor.m_momentOfInertia);
        // now calc the main shaft torque based on what is left after the sprag throughput has the
        // tail torque subtracted
        m_bottomUpTorque =
            (m_motor->m_transmittedTorque - tailLoadTorque) * m_theGears.getEngineMainRatio();
        m_rotorHead->m_torques.y = m_bottomUpTorque;
    } // new ElecEnergyBudget;

    /*
        //OLD ENERGY BUDGET DIALING BACK HACKS
        Procedure DoEnergyBudget;
        Var EffectiveFlyWheel:Single;//the flywheel of the rotors reflected through the gears
            FlywheelSpeed:Single;
            RotationalEnergy,HeadAeroLosses, TailAeroLosses,TotalAeroLosses:Single;
            EngineTorque:Single;
        Begin
         EffectiveFlyWheel:= (RotorHead->m_mainRotor.MomOfI +
        RotorHead->m_flyBar.MomOfI)/Math<physics_Num>::Sqr(TheGears.getEngineMainRatio()) +
                             TailRotor->m_momentOfInertia/Math<physics_Num>::Sqr(TheGears.m_engineTailRatio);
         FlyWheelSpeed:= RotorHead->m_mainRotor.Omega*TheGears.getEngineMainRatio();

         EngineRPM:=FlyWheelSpeed*9.549296; //convert flywheel Omega to RPM for engine and governor calls

         GetGovernorOutput;

         EngineThrottlePosition:=GovernorOutput;

         GetEngineOutput;

         RotationalEnergy:= 0.5*Math<physics_Num>::Sqr(FlyWheelSpeed)*EffectiveFlywheel;
         HeadAeroLosses:= (RotorHead->m_mainRotor.ForceMoments.y +
        TheGears.m_headFriction)*RotorHead->m_mainRotor.Omega + //allow for the Head bearing friction
        here RotorHead->m_flyBar.ForceMoments.y*RotorHead->m_flyBar.Omega; TailAeroLosses:=
        (1+TheGears.m_tailDriveLoss)*TailRotor->m_forceMoments.y*TailRotor->m_omega;   //allow for tail
        drive train losses here TotalAeroLosses:= HeadAeroLosses+TailAeroLosses;//generate the total aero
        losses

         RotationalEnergy:= 0.5*Math<physics_Num>::Sqr(FlyWheelSpeed)*EffectiveFlywheel -
        ThisDeltaT*(TotalAeroLosses - EngineOutput); //compute new rotational energy after timestep if
        RotationalEnergy<0 then FlyWheelSpeed:=1e-3 else FlyWheelSpeed:=
        Math<physics_Num>::Sqrt(2*RotationalEnergy/EffectiveFlyWheel);//calc. new flywheel speed
         RotorHead->m_mainRotor.Omega:=FlyWheelSpeed/TheGears.getEngineMainRatio();//transfer the new
        speed to the rotors RotorHead->m_flyBar.Omega:=RotorHead->m_mainRotor.Omega; TailRotor->m_omega:=
        FlyWheelSpeed/TheGears.m_engineTailRatio;

         //EngineTorque:= TheEngine.Output/FlyWheelSpeed; //
         //if EngineTorque> TheEngine.PeakPower/TheEngine.PPOmega1 then  EngineTorque:=
        TheEngine.PeakPower/TheEngine.PPOmega1; //limit the engine torque
         //RotorHead->Torques.y:=EngineTorque* TheGears.EngineMainRatio;//crude calculation of the shaft
        torque between body and head ignoring TR power RotorHead->Torques.y:= (EngineOutput -
        TailAeroLosses)/RotorHead->m_mainRotor.Omega;//deduct the tail losses from engine power and calc
        torque going to head End;
        */

    // sends head RPM to python
    f32 HeliAero::headRPMToPy() /* export */
    {
        return static_cast<f32>(m_rotorHead->m_mainRotor.getOmega() *
                                static_cast<physics_Num>(30.0) /
                                Math<physics_Num>::pi()); // convert Omega to RPM and sent
    } // HeadRPMToPy

    // sends pack charge state to python
    f32 HeliAero::packStateToPy() /* export */
    {
        // WP_LOG('Pack state in delphi :'+ FloatToStrF(Pack.PackState, FFfixed, 6,3));
        return m_pack->m_packState; //
    } // PackStateToPy

    // sends pack charge state to python
    f32 HeliAero::packVoltsToPy() /* export */
    {
        return m_pack->m_packV; //
    } // PackVoltsToPy

    // sends IC engine or electric motor power to python
    f32 HeliAero::motorPowerToPy() /* export */
    {
        if(m_modelIsElectric)
        {
            return m_motor->m_motorPower;
        }

        return m_theEngineClutch->m_enginePower;
    } // MotorPowerToPy

    // sends Engine RPM to python
    f32 HeliAero::engineRPMToPy() /* export */
    {
        if(m_modelIsElectric)
        {
            return m_motor->m_motorRpm / m_theGears.getEngineMainRatio();
        }

        // with IC model pass the engine rpm (but force an Idle of 2000
        if(m_engineThrottlePosition < static_cast<physics_Num>(0.05))
        {
            return 2000.f;
        }

        return m_engineRPM;
    } // EngineRPMToPy

    f32 HeliAero::soundRPMToPy() /* export */
    {
        if(m_modelIsElectric)
        {
            return m_motor->m_motorRpm / m_theGears.getEngineMainRatio();
        }

        // with IC model pass the engine rpm (but force an Idle of 2000
        if(m_engineThrottlePosition < static_cast<physics_Num>(0.05))
        {
            return 2000.f;
        }

        return m_engineRPM / m_theGears.getEngineMainRatio();
    } // SoundRPMToPy

    // sends maximum main rotor attack angle to python
    f32 HeliAero::maxMainAttackToPy() /* export */
    {
        return m_alphaLMaxMain;
    } // EngineRPMToPy

    // sends maximum main rotor attack angle to python
    f32 HeliAero::maxTailAttackToPy() /* export */
    {
        return m_alphaLMaxTail;
    } // EngineRPMToPy

    // Note Smoke flow is the y axis component of hub flow +  average y axis induced flow over arcs 6
    // to 15 (i.e. excluding cuff and tip) sends the current downflow (WTR heli body frame) for smoke
    // speed
    void HeliAero::smokeFlow(physics_Num &RootSmokeFlowX, physics_Num &RootSmokeFlowY,
                             physics_Num &RootSmokeFlowZ, physics_Num &TipSmokeFlowX,
                             physics_Num &TipSmokeFlowY, physics_Num &TipSmokeFlowZ)
    {
        physics_Vec RSF, TSF;
        int SS, AA;
        RSF.x = 0;
        RSF.y = 0;
        RSF.z = 0;
        TSF.x = 0;
        TSF.y = 0;
        TSF.z = 0;

        for(SS = 1; SS < 5; SS++)
        {
            /*with RotorHead->m_mainRotor.m_sector[SS].m_arc[AA] do*/ // sum the tip flows
            for(AA = 17; AA != 20; AA++)
            {
                TSF = VSum(TSF, m_rotorHead->m_mainRotor.m_sector[SS].m_arc[AA].getLFlow());
                // sum the Local induced flow over 4 sectors and 4 Arcs to give 16 x the average y
                // axis induced flow
            } //

            /*with RotorHead->m_mainRotor.m_sector[SS].m_arc[AA] do*/ // sum the root flows
            for(AA = 1; AA != 4; AA++)
            {
                RSF = VSum(RSF, m_rotorHead->m_mainRotor.m_sector[SS].m_arc[AA].getLFlow());
                // sum the Local induced flow over 4 sectors and 4 Arcs to give 16 x the average y
                // axis induced flow
            }
        }

        RSF = VScale(RSF, 0.0625);
        // note by multiplying by 0.0625 we leave the mean induced flow (no acceleration below rotor)
        TSF = VScale(TSF, 0.0625);
        // note by multiplying by 0.0625 we leave the mean induced flow (no acceleration below rotor)
        RootSmokeFlowX = RSF.x;
        RootSmokeFlowY = RSF.y;
        RootSmokeFlowZ = RSF.z; // return the required value
        TipSmokeFlowX = TSF.x;
        TipSmokeFlowY = TSF.y;
        TipSmokeFlowZ = TSF.z; // return the required value
    } // SmokeFlow

    void HeliAero::soundData(physics_Num &TipPress1, physics_Num &TipPress2, physics_Num &TipPress3,
                             physics_Num &TipPress4, physics_Num &TipDrg1, physics_Num &TipDrg2,
                             physics_Num &TipDrg3, physics_Num &TipDrg4)
    {
        ///*with RotorHead->m_mainRotor do*/
        {
            TRotor &mr = m_rotorHead->m_mainRotor;
            TipPress1 = mr.m_sector[1].m_arc[20].getLift() / mr.m_sector[1].m_arc[20].m_area;
            TipPress2 = mr.m_sector[2].m_arc[20].getLift() / mr.m_sector[2].m_arc[20].m_area;
            TipPress3 = mr.m_sector[3].m_arc[20].getLift() / mr.m_sector[3].m_arc[20].m_area;
            TipPress4 = mr.m_sector[4].m_arc[20].getLift() / mr.m_sector[4].m_arc[20].m_area;
            TipDrg1 = mr.m_sector[1].m_arc[20].m_drag / mr.m_sector[1].m_arc[20].m_area;
            TipDrg2 = mr.m_sector[2].m_arc[20].m_drag / mr.m_sector[2].m_arc[20].m_area;
            TipDrg3 = mr.m_sector[3].m_arc[20].m_drag / mr.m_sector[3].m_arc[20].m_area;
            TipDrg4 = mr.m_sector[4].m_arc[20].m_drag / mr.m_sector[4].m_arc[20].m_area;
        } // With
    } // SoundData

    // called to initialize the aerodynamics
    void HeliAero::resetAll() /* export */
    {
        physics_Vec rotVec;
        getDataPath();

        m_bodyForcesOn = true; // by default turn the output of main forces on
        m_rotorForcesOn = true; // and turn on the visual rotor steering forces

        AerodymanicsUtil::readPowerLookups(
            *m_theEngineClutch); // get the engine power curves from file
        AerodymanicsUtil::initGovernor();

        if(m_modelIsElectric)
        {
            AerodymanicsUtil::initMotorAndESC(*m_motor, *m_esc);
        }

        m_engineRPM = 2000; // set the engine rpm to idle

        if(m_modelIsElectric)
        {
            AerodymanicsUtil::chargePack(*m_pack);
        }
        else
        {
            AerodymanicsUtil::startEngine(); // start the engine or charge pack as needed
        }

        m_linkageControl = false; // set default control from the linkages
        m_inducedSmoothing = 0.06; // in case python does not pass a value
        m_interferenceFactor = 1; // set the main-to-flybar interference coupling factor
        m_geMax = 1; // set default ground effect maximum
        m_geDecay = 2; // set default ground effect decay rate
        m_loops = 0; // reset loop counter
        m_sumOfDeltas = 0;
        m_maxDeltaT = 0;
        m_minDeltaT = 1000;

        readModelData(); // get all the model info from the physics engine
        initialize();
        initServo(m_tailServo, 0.08);
        initServo(m_eleServo, 0.15);
        initServo(m_ailServo, 0.15);
        initServo(m_colServo, 0.15);
        initTailLinkage(m_tailLinkage);
        initGearTrain(m_theGears);
        initSurfaces(m_rotorHead->m_mainRotor);
        initSurfaces(m_rotorHead->m_flyBar);
        initSurfaces(*m_tailRotor);
        AerodymanicsUtil::initTailGyro(); // directly to the Gyro unit

        getFoilLookup(m_rotorHead->m_mainRotor.getSection(),
                      m_rotorHead->m_mainRotor.getAFoilLookup());
        getFoilLookup(m_rotorHead->m_flyBar.getSection(), m_rotorHead->m_flyBar.getAFoilLookup());
        getFoilLookup(m_tailRotor->getSection(), m_tailRotor->getAFoilLookup());

        m_rotorHead->m_mainRotor.setOmega(100.0 * 2.0 * Math<real_dNum>::pi() /
                                          60.0); // set rpm of main rotor
        m_rotorHead->m_flyBar.setOmega(m_rotorHead->m_mainRotor.getOmega()); // set rpm of flybar
        m_tailRotor->setOmega(m_theGears.m_mainTailRatio *
                              m_rotorHead->m_mainRotor.getOmega()); // set rpm of tail
        m_rotorHead->m_mainRotor.m_collective = 0.0 * Math<real_dNum>::pi() / 180.0;
        m_rotorHead->m_mainRotor.m_cone = 0.0 * Math<real_dNum>::pi() / 180.0;
        m_rotorHead->m_flyBar.m_cone = 0.0 * Math<real_dNum>::pi() / 180.0;
        m_rotorHead->m_mainRotor.m_frame.m_xAxis.x = 1.0;
        m_rotorHead->m_mainRotor.m_frame.m_xAxis.y = 0.0;
        m_rotorHead->m_mainRotor.m_frame.m_xAxis.z = 0.0;
        m_rotorHead->m_mainRotor.m_frame.m_yAxis.x = 0.0;
        m_rotorHead->m_mainRotor.m_frame.m_yAxis.y = 1.0;
        m_rotorHead->m_mainRotor.m_frame.m_yAxis.z = 0.0;
        m_rotorHead->m_mainRotor.m_frame.m_zAxis.x = 0.0;
        m_rotorHead->m_mainRotor.m_frame.m_zAxis.y = 0.0;
        m_rotorHead->m_mainRotor.m_frame.m_zAxis.z = 1.0;
        m_rotorHead->m_flyBar.m_frame = m_rotorHead->m_mainRotor.m_frame;
        m_rotorHead->m_flyBar.m_hubPosition = m_rotorHead->m_mainRotor.m_hubPosition;
        m_rotorHead->m_torques.x = 0.0;
        m_rotorHead->m_torques.y = 0.0;
        m_rotorHead->m_torques.z = 0.0;
        for(int rr = 1; rr != 4; rr++)
            m_rotorHead->m_mainRotor.m_groundEffect[rr] = 0.0; // zero the ground effect
        for(int rr = 1; rr != 4; rr++)
            m_rotorHead->m_flyBar.m_groundEffect[rr] = 0.0; // zero the ground effect
        for(int rr = 1; rr != 4; rr++)
            m_tailRotor->m_groundEffect[rr] =
                0.0; // zero the ground effect for the tail (permanently!!)
        m_rotorHead->m_mainRotor.m_hubFlow.x = 0;
        m_rotorHead->m_mainRotor.m_hubFlow.y = 0;
        m_rotorHead->m_mainRotor.m_hubFlow.z = 0;
        m_rotorHead->m_flyBar.m_hubFlow = m_rotorHead->m_mainRotor.m_hubFlow;
        m_body.m_frame.m_xAxis.x = 1;
        m_body.m_frame.m_xAxis.y = 0;
        m_body.m_frame.m_xAxis.z = 0; // set body frame
        m_body.m_frame.m_yAxis.x = 0;
        m_body.m_frame.m_yAxis.y = 1;
        m_body.m_frame.m_yAxis.z = 0;
        m_body.m_frame.m_zAxis.x = 0;
        m_body.m_frame.m_zAxis.y = 0;
        m_body.m_frame.m_zAxis.z = 1;
        rotVec.x = 0;
        rotVec.y = 0;
        rotVec.z = 0; // set up any initial rotation needed between main rotor and body
        m_rotorHead->m_mainRotor.m_frame = RotateFrame(m_body.m_frame, rotVec);
        rotVec.x = 0;
        rotVec.y = 0;
        rotVec.z = 0; // set up any initial rotation needed between Flybar and body
        m_rotorHead->m_flyBar.m_frame = RotateFrame(m_body.m_frame, rotVec);
        m_rotorHead->m_linkage.m_swashEleAngle = 0.0;
        m_rotorHead->m_linkage.m_swashAilAngle = 0.0;

        sortFramesAndLinks();
        updateSurfaces(m_rotorHead->m_mainRotor);
        updateSurfaces(m_rotorHead->m_flyBar);
        updateSurfaces(*m_tailRotor);
        calcMainArcFlows(m_rotorHead->m_mainRotor);
        calcMainArcFlows(m_rotorHead->m_flyBar);
        calcMainArcFlows(*m_tailRotor);
        calcInduced();
        mainToFlyBarInterference(m_interferenceFactor);
        getForcesFromFlow(m_rotorHead->m_mainRotor);
        getForcesFromFlow(m_rotorHead->m_flyBar);
        getForcesFromFlow(*m_tailRotor);
        sumForcesandMoments(m_rotorHead->m_mainRotor);
        sumForcesandMoments(m_rotorHead->m_flyBar);
        sumForcesandMoments(*m_tailRotor);
    } // ResetAll

    // Critical damping considerations
    // for damped SHM in rotation with MoI = I we have
    // I*d2Theta/dt2 + C*dTheta/dt + K*Theta = 0;  where C is the damping factor and K is the spring
    // constant natural frequency = SQRT(K/I) /(2*Math<physics_Num>::pi()) Damping Ratio =
    // C/(2*(SQRT(I*K)) and this = 1 for critical damping by default I = 0.002 so if we choose a
    // natural frequency of 20Hz and critical damping K = I*(SQR(2*Math<physics_Num>::pi()*f)) = 31.6
    // N.m per radian C = 2*(SQRT(I*K)) = 0.50 (with critical damping) note that we are using a
    // natural frequency of 10Hz and damping ratio of 1 with 2E-6 MoI in pitch and roll this requires
    // a FC of 0.0079 N.m.rad and a DC of 0.00025 N.m/rad/s

    // these 'ficticious' forces steer the visual rotor to the attitude of the aerodynamic one
    void HeliAero::outputVisualRotorForces()
    {
        physics_Num rollAngleError, pitchAngleError;
        physics_Num rollSpeedError, pitchSpeedError;
        physics_Num rollServoForce, pitchServoForce;
        physics_Num vrfc, vrdc; // hold the force and damping factors for this servo force

        physics_Vec f, loc;
        constexpr auto minDelta = 1.0 / 600.0;
        if(m_thisDeltaT < minDelta)
        {
            vrfc = static_cast<physics_Num>(0.08);
            vrdc = static_cast<physics_Num>(0.00025);
        }
        else
        {
            vrfc = static_cast<physics_Num>(0.004);
            vrdc = static_cast<physics_Num>(0.000125);
        }

        rollAngleError =
            -VDot(m_rotorHead->m_mainRotor.m_frame.m_yAxis, m_visualRotorFrame.m_xAxis);
        Math<physics_Num>::Limit(rollAngleError,
                                 0.15); // limit the angle to about 9 degrees for force limitation
        pitchAngleError =
            VDot(m_rotorHead->m_mainRotor.m_frame.m_yAxis, m_visualRotorFrame.m_zAxis);
        Math<physics_Num>::Limit(pitchAngleError,
                                 0.15); // limit the angle to about 9 degrees for force limitation
        rollSpeedError =
            (m_visualRotorAngularVelocity.z - m_rotorHead->m_mainRotor.m_precessionRate.z);
        Math<physics_Num>::Limit(rollSpeedError, 16); // limit speed error to about 900 degrees/s
        pitchSpeedError =
            (m_visualRotorAngularVelocity.x - m_rotorHead->m_mainRotor.m_precessionRate.x);
        Math<physics_Num>::Limit(pitchSpeedError, 16); // limit speed error to about 900 degrees/s
        rollServoForce = vrfc * rollAngleError - vrdc * rollSpeedError;
        pitchServoForce = vrfc * pitchAngleError - vrdc * pitchSpeedError;
        // The pitch axis forces are generated by looking at the angle between the VisualRotor and
        // the main rotor
        loc.x = 0.5;
        loc.y = 0;
        loc.z = 0;
        f.x = 0;
        f.y = rollServoForce;
        f.z = 0;
        addLocalForce(CB_ROTOR_HEAD, f, loc, 1);
        loc.x = -0.5;
        loc.y = 0;
        loc.z = 0;
        f.x = 0;
        f.y = -rollServoForce;
        f.z = 0;
        addLocalForce(CB_ROTOR_HEAD, f, loc, 2);
        loc.x = 0;
        loc.y = 0;
        loc.z = 0.5;
        f.x = 0;
        f.y = -pitchServoForce;
        f.z = 0;
        addLocalForce(CB_ROTOR_HEAD, f, loc, 3);
        loc.x = 0;
        loc.y = 0;
        loc.z = -0.5;
        f.x = 0;
        f.y = pitchServoForce;
        f.z = 0;
        addLocalForce(CB_ROTOR_HEAD, f, loc, 4);
    } // OutputVisualRotorForces

    void HeliAero::outputVisualRotorTorques()
    {
        physics_Num rollError, pitchError;
        physics_Vec f, loc;
        // The pitch axis forces are generated by looking at the angle between the VisualRotor and
        // the main rotor
        rollError =
            -VDot(m_rotorHead->m_mainRotor.m_frame.m_yAxis, m_visualRotorFrame.m_xAxis) -
            static_cast<physics_Num>(0.1) *
            (m_visualRotorAngularVelocity.z - m_rotorHead->m_mainRotor.m_precessionRate.z);
        pitchError =
            VDot(m_rotorHead->m_mainRotor.m_frame.m_yAxis, m_visualRotorFrame.m_zAxis) -
            static_cast<physics_Num>(0.1) *
            (m_visualRotorAngularVelocity.x - m_rotorHead->m_mainRotor.m_precessionRate.x);

        // loc.x:=0.0; loc.y:=0.5; loc.z:=0;
        // f.x:=-rollError; f.y:=0; f.z:=pitchError;
        // AddLocalForce(CB_ROTOR_HEAD,f,loc);

        // loc.x:=0.0; loc.y:=-0.5; loc.z:=0;
        // f.x:=rollError; f.y:=0; f.z:=-pitchError;
        // AddLocalForce(CB_ROTOR_HEAD,f,loc);
        f.x = static_cast<physics_Num>(0.10); // pitchError;
        f.y = static_cast<physics_Num>(0.0);
        f.z = static_cast<physics_Num>(0.01); //-rollError;
        addLocalTorque(CB_ROTOR_HEAD, f);
    }

    //*******************************************************************************************
    //******************************* OUTPUT FORCES CALL STARTS HERE ****************************
    //*******************************************************************************************
    void HeliAero::outputForces()
    {
        physics_Vec f, loc; // temp vectors for force and location

        bool showVectors;
        physics_Vec tVec;
        showVectors = false;
        // first generate the rotor head torque about the z axis using two forces 1m apart
        loc.x = static_cast<physics_Num>(0.5);
        loc.y = static_cast<physics_Num>(0.0);
        loc.z = static_cast<physics_Num>(0.0);
        f.x = static_cast<physics_Num>(0.0);
        f.y = m_rotorHead->m_torques.z;
        f.z = static_cast<physics_Num>(0.0);
        addLocalForce(CB_MODEL, f, loc, 1);

        if(showVectors)
        {
            displayLocalVector(CB_MODEL, f, loc);
        }

        loc.x = -0.5;
        loc.y = static_cast<physics_Num>(0.0);
        loc.z = static_cast<physics_Num>(0.0);
        f.x = static_cast<physics_Num>(0.0);
        f.y = -m_rotorHead->m_torques.z;
        f.z = static_cast<physics_Num>(0.0);
        addLocalForce(CB_MODEL, f, loc, 2);

        if(showVectors)
        {
            displayLocalVector(CB_MODEL, f, loc);
        }

        // now generate the x axis torque using two forces
        loc.x = static_cast<physics_Num>(0.0);
        loc.y = static_cast<physics_Num>(0.0);
        loc.z = 0.5;
        f.x = static_cast<physics_Num>(0.0);
        f.y = -m_rotorHead->m_torques.x;
        f.z = static_cast<physics_Num>(0.0);
        addLocalForce(CB_MODEL, f, loc, 3);
        // if showVectors then DisplayLocalVector(CB_MODEL,f,loc);
        loc.x = static_cast<physics_Num>(0.0);
        loc.y = static_cast<physics_Num>(0.0);
        loc.z = -0.5;
        f.x = static_cast<physics_Num>(0.0);
        f.y = m_rotorHead->m_torques.x;
        f.z = static_cast<physics_Num>(0.0);
        addLocalForce(CB_MODEL, f, loc, 4);
        // if showVectors then DisplayLocalVector(CB_MODEL,f,loc);

        // now do the y axis torque with two forces (in the x direction at +- 0.5m in z
        loc.x = static_cast<physics_Num>(0.0);
        loc.y = static_cast<physics_Num>(0.0);
        loc.z = 0.5;
        f.x = m_rotorHead->m_torques.y;
        f.y = 0;
        f.z = 0;
        addLocalForce(CB_MODEL, f, loc, 5);
        // if showVectors then DisplayLocalVector(CB_MODEL,f,loc);
        loc.x = static_cast<physics_Num>(0.0);
        loc.y = static_cast<physics_Num>(0.0);
        loc.z = -0.5;
        f.x = -m_rotorHead->m_torques.y;
        f.y = static_cast<physics_Num>(0.0);
        f.z = static_cast<physics_Num>(0.0);
        addLocalForce(CB_MODEL, f, loc, 6);
        // if showVectors then DisplayLocalVector(CB_MODEL,f,loc);

        // now apply the main lift force of the main rotor
        addLocalForce(CB_MODEL, m_rotorHead->m_mainRotor.m_hubForce,
                      m_rotorHead->m_mainRotor.m_hubPosition,
                      7); // main lift/drag of the main rotor
        f = m_rotorHead->m_mainRotor.m_hubForce;
        // f.y:=0;//copy force but zero y component so we can see the x & z components
        // if showVectors then
        // DisplayLocalVector(CB_MODEL,VScale(f,0.1),m_rotorHead->m_mainRotor.m_hubPosition);//display
        // main force scaled by 0.1
        f.x = m_tailRotor->m_hubForce.y;
        f.y = -m_tailRotor->m_hubForce.x;
        f.z = m_tailRotor->m_hubForce.z; // rotate tail force as needed
        VScale(f, 1.0); // artificially increase the tail power
        addLocalForce(CB_MODEL, f, m_tailRotor->m_hubPosition, 8.0); // apply tail force
        // if showVectors then DisplayLocalVector(CB_MODEL,f,m_tailRotor->m_hubPosition); //draw tail
        // force
        m_body.m_dragForce =
            AerodymanicsUtil::bodyForce(m_body.m_airFlow,
                                        m_body.m_cdA); // calculate the drag force on the body
        // and apply this force
        addLocalForce(CB_MODEL, m_body.m_dragForce, m_body.m_dragCentre, 9.0);
        // if showVectors then DisplayLocalVector(CB_MODEL, m_body.m_dragForce, m_body.m_dragCentre);
        tVec = VecToFrame(m_rotorHead->m_flyBar.m_frame.m_yAxis, m_body.m_frame);
        // if showVectors then DisplayLocalVector(CB_MODEL, tVec,
        // m_rotorHead->m_mainRotor.m_hubPosition);
        tVec = VecToFrame(m_rotorHead->m_linkage.m_swashFrame.m_yAxis, m_body.m_frame);
        if(showVectors)
            displayLocalVector(CB_MODEL, VScale(tVec, 0.5),
                               m_rotorHead->m_mainRotor.m_hubPosition);
        tVec = VecToFrame(m_rotorHead->m_mainRotor.m_frame.m_yAxis, m_body.m_frame);
        if(showVectors)
            displayLocalVector(CB_MODEL, VScale(tVec, 1.2),
                               m_rotorHead->m_mainRotor.m_hubPosition);
        tVec = VecToFrame(m_rotorHead->m_shaftFrame.m_yAxis, m_body.m_frame);
        // if showVectors then DisplayLocalVector(CB_MODEL, VScale(tVec, 1.5),
        // m_rotorHead->m_mainRotor.m_hubPosition);
    } // OutputForces;

    // some forces to investigate the 'walk' problem on the ground
    void HeliAero::testForces()
    {
        physics_Vec F, Loc; // temp vectors for force and location
        Loc.x = static_cast<physics_Num>(0.0);
        Loc.y = static_cast<physics_Num>(0.3);
        Loc.z = static_cast<physics_Num>(0.3);
        F.x = Math<physics_Num>::Sin(static_cast<physics_Num>(5.0) *
                                     static_cast<physics_Num>(2.0) * Math<physics_Num>::pi() *
                                     static_cast<physics_Num>(m_simTime));
        F.y = static_cast<physics_Num>(0.0);
        F.z = static_cast<physics_Num>(0.0);
        addLocalForce(CB_MODEL, F, Loc, 0);
    }

    // to investigate the 'hop'
    void HeliAero::specialTestFrame(f32 t, f32 dt)
    {
        m_simTime = t;
        getAllTxData();
        m_rotorHead->m_mainRotor.m_hubForce.x =
            static_cast<physics_Num>(2.0) *
            Math<physics_Num>::Sin(static_cast<physics_Num>(5.0) *
                                   static_cast<physics_Num>(2.0) * Math<physics_Num>::pi() *
                                   static_cast<physics_Num>(m_simTime));
        if(m_txChannel[COL_CHANNEL] > 0)
        {
            m_rotorHead->m_mainRotor.m_hubForce.y =
                static_cast<physics_Num>(70.0) * m_txChannel[COL_CHANNEL];
        }
        else
        {
            m_rotorHead->m_mainRotor.m_hubForce.y = 0;
        }

        m_rotorHead->m_mainRotor.m_hubForce.z =
            static_cast<physics_Num>(2.0) *
            Math<physics_Num>::Cos(static_cast<physics_Num>(5.0) *
                                   static_cast<physics_Num>(2.0) * Math<physics_Num>::pi() *
                                   static_cast<physics_Num>(m_simTime));
        addLocalForce(CB_MODEL, m_rotorHead->m_mainRotor.m_hubForce,
                      m_rotorHead->m_mainRotor.m_hubPosition, 0);
    }

    void HeliAero::drawVBar()
    {
        physics_Vec F, Loc;
        F = VScale(m_theVBar->m_frame.m_yAxis, 0.1);
        Loc.x = 0.0;
        Loc.y = 0.5;
        Loc.z = 0.0;
        displayLocalVector(CB_MODEL, F, Loc);
    }

    //*******************************************************************************************
    //******************************* THE WORKBENCH CALL STARTS HERE ****************************
    //*******************************************************************************************
    // performs a single timestep calculation of specified dt
    void HeliAero::workBenchFrame(f32 t, f32 dt, int ParamID, f32 ParamValue) /* export */
    {
        try
        {
            if(dt > 1.0 / 200.0)
            {
                dt = 1.0 / 200.0;
            }

            // int SS, RR;
            physics_Vec Org, TstV;
            int ExLoc;
            // f32 TRSig;//temp to hold the tail rotor signal
            ExLoc = 0;

            m_loops++;
            debugPrint(); // prints out whatever you want for debugging
            m_simTime = t;
            m_lastDeltaT = m_thisDeltaT; // shuffle the DeltaTs

            if(dt < 0.05)
            {
                m_thisDeltaT = dt;
            }
            else
            {
                // if the dt is too long then realign the frames and limit the DeltaT
                m_thisDeltaT = 0.05; // limit the Delta t to 50ms
                alignFrames();
            }

            m_nextDeltaT = m_thisDeltaT +
                           (m_thisDeltaT - m_lastDeltaT); // assume the trend in DeltaT is linear
            m_meanDeltaT = 0.95 * m_meanDeltaT + 0.05 * m_thisDeltaT; // calculate the meanDeltaT
            m_sumOfDeltas = m_sumOfDeltas + dt; // update the sum of the deltaTs

            if(dt > m_maxDeltaT)
                m_maxDeltaT = dt;
            if(m_minDeltaT > dt)
                m_minDeltaT = dt;

            if(ParamID !=
               0) // test for ParamID<>0 (i.e. a dynamically adjusted parameter is being changed)
            {
                // do any parameter value changes here
                AerodymanicsUtil::adjustGyroParam(ParamID,
                                                  ParamValue); // do the gyro parameter adjustment
                // if the flybarless or gov need dynamic param adjustments they go here
            }

            // if LinkageControl then GetControlInfo else GetAllTxData;
            getControlInfo();
            // now get control angles (Swash, collective and and tail pitch from linkages)
            getTxChannels(); // read the Tx channels for the governor and gyro

            ExLoc = 1;

            // Wind:= GetHeliWind(ModelPosition.y,SimTime); //get the wind (in the ground frame of
            // reference)
            readModelData(); // get all the model info from the physics engine

            if(m_rotorHead->m_hasFlyBar == false)
            {
                AerodymanicsUtil::vBarLoop(m_thisDeltaT, m_txChannel[AIL_CHANNEL],
                                           m_txChannel[ELE_CHANNEL], m_vbAilOut, m_vbEleOut,
                                           *m_theVBar);
                sendFlybarlessControlsToMixer(-m_vbAilOut, -m_vbEleOut);
                // DrawVBar;
            }

            sortFramesAndLinks();
            m_tailGyroInput = -m_txChannel[YAW_CHANNEL]; // pass the yaw channel output to the gyro
            m_tailGyroGain = m_txChannel[GYRO_GAIN_CHANNEL]; // set up the gyro gain (was originally
            // fixed at -0.7)

            // WP_LOG(TailGyroGain);

            // GetTailGyroOutput;//get the gyro output from the external gyro DLL
            // TRSig:=Servo(TailGyroOutput,ThisDeltaT,TailServo);
            // TailRotor->m_collective:=DoTailLinkage(TRSig,TailLinkage); removed to make way for
            // Brian's linkage control
            driveTailServo(AerodymanicsUtil::getTailGyro(
                m_tailGyroInput, m_tailGyroGain, m_thisDeltaT,
                m_modelAngularVelocity.y)); // send the gyro signal to Brian's servo via callback

            // if direct tail control is specified then route the gyro output direct to the pitch
            if(m_directTailControl.m_bVal == true)
            {
                m_tailRotor->m_collective = m_tailGyro.m_output;
            }

            m_rxThrottleSig = static_cast<physics_Num>(0.5 * (1.0 + m_txChannel[THR_CHANNEL]));

            // DoGovernor(ThrotSig,GovTarget,ThisDeltaT,TheEngine,TheGovernor);
            // DoEngine(TheEngine,TheGovernor);
            // WP_LOG('EnginePower '+FloatToStr(TheEngine.Output));

            // DoEnergyBudget; //note now includes calls to governor and engine DLLs
            // CalcAllRotorFrameFlows;
            ExLoc = 2;
            // UpdateSurfaces(RotorHead->m_mainRotor);
            // if RotorHead->HasFlyBar then UpdateSurfaces(RotorHead->m_flyBar);
            // UpdateSurfaces(TailRotor);
            ExLoc = 3;
            // CalcMainArcFlows(RotorHead->m_mainRotor);
            // if RotorHead->HasFlyBar then CalcMainArcFlows(RotorHead->m_flyBar);
            // CalcMainArcFlows(TailRotor);
            ExLoc = 4;
            // CalcInduced;
            // SmoothInduced(RotorHead->m_mainRotor,InducedSmoothing);//added to smear out the
            // azumuthal variation in induced flow if RotorHead->HasFlyBar then
            // SmoothInduced(RotorHead->Flybar,InducedSmoothing);   //added to smear out the
            // azumuthal variation in induced flow if RotorHead->HasFlyBar then
            // MainToFlyBarInterference(InterferenceFactor);
            // GetForcesFromFlow(RotorHead->m_mainRotor);
            // if RotorHead->HasFlyBar then GetForcesFromFlow(RotorHead->m_flyBar);
            // GetForcesFromFlow(TailRotor);
            ExLoc = 8;
            // SumForcesandMoments(RotorHead->m_mainRotor);
            // if RotorHead->HasFlyBar then SumForcesAndMoments(RotorHead->m_flyBar);
            // SumForcesAndMoments(TailRotor);
            // CalcAlphaLMax(AlphaLMaxMain,RotorHead->m_mainRotor);
            // CalcAlphaLMax(AlphaLMaxTail,TailRotor);
            // PrecessAndTeeter;//replaces the seperate DoPrecession and CalcTeeterForces
            // DoPrecession(ThisDeltaT,RotorHead->m_mainRotor);
            // DoPrecession(ThisDeltaT,RotorHead->m_flyBar);
            // CalcTeeterForces(RotorHead);
            // Add in the tail forces to body here
            ExLoc = 9;
            Org.x = 0;
            Org.y = 0.4;
            Org.z = 0;
            ExLoc = 10;

            // if SimTime>2 then OutputForces;
            // OutputVisualRotorForces;
            // TestForces; //output some test force to look for the walk
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    } // WorkBenchFrame

    int getKey()
    {
#if defined WP_PLATFORM_WIN32
        //	for (int i = 8; i <= 256; i++)
        //	{
        //		if (GetAsyncKeyState(i) & 0x7FFF)
        //		{

        //			// This if filters the keys, i want to allow direction arrows
        //			// and q for quit. If you want to add more just add the code for the key,
        //			// to know the key code just coment the if line and print the keycode.
        //			if ((i >= 37 && i <= 40) || i == 81)
        //				return i;
        //	}
        //}

        if((GetAsyncKeyState(Keyleft) & 0x8000) != 0)
        {
            return Keyleft;
        }

        if((GetAsyncKeyState(Keytop) & 0x8000) != 0)
        {
            return Keytop;
        }

        if((GetAsyncKeyState(Keyright) & 0x8000) != 0)
        {
            return Keyright;
        }

        if((GetAsyncKeyState(Keydown) & 0x8000) != 0)
        {
            return Keydown;
        }

        return -1;
#else
        return -1;
#endif
    }

#if defined WP_PLATFORM_WIN32
#elif defined SARACEN_PLATFORM_LINUX
    /**
         *
         * @param ks  like XK_Shift_L, see /usr/include/X11/keysymdef.h
         * @return
         */
    bool key_is_pressed(KeySym ks)
    {
        Display *dpy = XOpenDisplay(":0");
        char keys_return[32];
        XQueryKeymap(dpy, keys_return);
        KeyCode kc2 = XKeysymToKeycode(dpy, ks);
        bool isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
        XCloseDisplay(dpy);
        return isPressed;
    }

    bool ctrl_is_pressed()
    {
        return key_is_pressed(XK_Control_L) || key_is_pressed(XK_Control_R);
    }

    bool shift_is_pressed()
    {
        return key_is_pressed(XK_Shift_L) || key_is_pressed(XK_Shift_R);
    }
#endif

    //*******************************************************************************************
    //******************************* THE MAINFRAME CALL STARTS HERE ****************************
    //*******************************************************************************************
    // performs a single timestep calculation of specified dt
    void HeliAero::mainFrame(real_dNum t, real_dNum dt)
    {
        try
        {
            if(dt > 1.0 / 200.0)
            {
                dt = 1.0 / 200.0;
            }

#if defined WP_PLATFORM_WIN32
            using namespace std;

            auto shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            auto ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

            if(ctrl)
            {
                int keyCode = getKey();
                if(keyCode == Keyleft)
                {
                    m_roair = m_roair - (dt * 1.0);
                    cout << "Roair: " << m_roair << endl;
                }
                else if(keyCode == Keyright)
                {
                    m_roair = m_roair + (dt * 1.0);
                    cout << "Roair: " << m_roair << endl;
                }
            }
            else if(!shift)
            {
                int keyCode = getKey();
                switch(keyCode)
                {
                case Keyleft:
                {
                    m_cdFiddle = m_cdFiddle - (dt * 1.0);
                    cout << "cd: " << m_cdFiddle << endl;
                }
                break;
                case Keytop:
                {
                    m_clFiddle = m_clFiddle + (dt * 1.0);
                    cout << "cl: " << m_clFiddle << endl;
                }
                break;
                case Keyright:
                {
                    m_cdFiddle = m_cdFiddle + (dt * 1.0);
                    cout << "cd: " << m_cdFiddle << endl;
                }
                break;
                case Keydown:
                {
                    m_clFiddle = m_clFiddle - (dt * 1.0);
                    cout << "cl: " << m_clFiddle << endl;
                }
                break;
                case Keyexit:
                {
                }
                break;
                default:
                {
                }
                }
            }
            else
            {
                int keyCode = getKey();
                switch(keyCode)
                {
                case Keytop:
                {
                    m_nitroFiddle = m_nitroFiddle + (dt * 1.0);
                    cout << "nitro: " << m_nitroFiddle << endl;
                }
                break;
                case Keydown:
                {
                    m_nitroFiddle = m_nitroFiddle - (dt * 1.0);
                    cout << "nitro: " << m_nitroFiddle << endl;
                }
                break;
                case Keyexit:
                {
                }
                break;
                default:
                {
                }
                }
            }
#elif defined SARACEN_PLATFORM_LINUX

            if(shift_is_pressed())
            {
                if(key_is_pressed(XK_Left))
                {
                    m_cdFiddle = m_cdFiddle - (dt * 1.0);
                    WP_LOG_ERROR("cd: " + StringUtil::toString( (f32)m_cdFiddle ));
                }

                if(key_is_pressed(XK_Right))
                {
                    m_cdFiddle = m_cdFiddle + (dt * 1.0);
                    WP_LOG_ERROR("cd: " + StringUtil::toString( (f32)m_cdFiddle ));
                }

                if(key_is_pressed(XK_Up))
                {
                    m_clFiddle = m_clFiddle + (dt * 1.0);
                    WP_LOG_ERROR("cl: " + StringUtil::toString( (f32)m_clFiddle ));
                }

                if(key_is_pressed(XK_Down))
                {
                    m_clFiddle = m_clFiddle - (dt * 1.0);
                    WP_LOG_ERROR("cl: " + StringUtil::toString( (f32)m_clFiddle ));
                }
            }
#endif

            m_loops++;
            m_simTime = t;
            m_lastDeltaT = m_thisDeltaT;

            if(dt < 0.05)
            {
                m_thisDeltaT = dt;
            }
            else
            {
                m_thisDeltaT = 0.05;
                alignFrames();
            }

            m_nextDeltaT = m_thisDeltaT + (m_thisDeltaT - m_lastDeltaT);
            m_meanDeltaT = 0.95 * m_meanDeltaT + 0.05 * m_thisDeltaT;
            m_sumOfDeltas = m_sumOfDeltas + dt;

            if(dt > m_maxDeltaT)
                m_maxDeltaT = dt;
            if(m_minDeltaT > dt)
                m_minDeltaT = dt;

            getControlInfo();
            getTxChannels();

            readModelData();

            if(m_rotorHead->m_hasFlyBar == false)
            {
                AerodymanicsUtil::vBarLoop(m_thisDeltaT, m_txChannel[AIL_CHANNEL],
                                           m_txChannel[ELE_CHANNEL], m_vbAilOut, m_vbEleOut,
                                           *m_theVBar);
                sendFlybarlessControlsToMixer(-m_vbAilOut, -m_vbEleOut);
            }

            sortFramesAndLinks();
            m_tailGyroInput = -m_txChannel[YAW_CHANNEL];
            m_tailGyroGain = m_txChannel[GYRO_GAIN_CHANNEL];

            driveTailServo(AerodymanicsUtil::getTailGyro(
                m_tailGyroInput, m_tailGyroGain, m_thisDeltaT, m_modelAngularVelocity.y));

            if(m_directTailControl.m_bVal == true)
                m_tailRotor->m_collective = m_tailGyro.m_output;

            m_rxThrottleSig = static_cast<physics_Num>(0.5) *
                              (static_cast<physics_Num>(1.0) + m_txChannel[THR_CHANNEL]);

            if(m_modelIsElectric)
                doElecEnergyBudget();
            else
                doICEnergyBudget();

            calcAllRotorFrameFlows();
            updateSurfaces(m_rotorHead->m_mainRotor);

            if(m_rotorHead->m_hasFlyBar)
                updateSurfaces(m_rotorHead->m_flyBar);

            updateSurfaces(*m_tailRotor);

            calcMainArcFlows(m_rotorHead->m_mainRotor);
            if(m_rotorHead->m_hasFlyBar)
                calcMainArcFlows(m_rotorHead->m_flyBar);

            calcMainArcFlows(*m_tailRotor);

            calcInduced();
            smoothInduced(m_rotorHead->m_mainRotor, m_inducedSmoothing);

            if(m_rotorHead->m_hasFlyBar)
            {
                smoothInduced(m_rotorHead->m_flyBar, m_inducedSmoothing);
                mainToFlyBarInterference(m_interferenceFactor);
            }

            getForcesFromFlow(m_rotorHead->m_mainRotor);
            if(m_rotorHead->m_hasFlyBar)
            {
                getForcesFromFlow(m_rotorHead->m_flyBar);
            }
            else
            {
                zeroForcesAndMoments(m_rotorHead->m_flyBar);
            }

            getForcesFromFlow(*m_tailRotor);

            sumForcesandMoments(m_rotorHead->m_mainRotor);
            if(m_rotorHead->m_hasFlyBar)
            {
                sumForcesandMoments(m_rotorHead->m_flyBar);
            }

            sumForcesandMoments(*m_tailRotor);
            calcAlphaLMax(m_alphaLMaxMain, m_rotorHead->m_mainRotor);
            calcAlphaLMax(m_alphaLMaxTail, *m_tailRotor);

            precessAndTeeter();

            if((m_simTime > 2) && (m_bodyForcesOn == true))
            {
                outputForces();
            }

            if(m_rotorForcesOn == true)
            {
                outputVisualRotorForces();
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    } // MainFrame

    void HeliAero::testDisplay() /* export */
    {
        physics_Vec Org, TstV;
        Org.x = 0;
        Org.y = 0;
        Org.z = 0;
        // if LinkageControl then GetLinkageAngles else GetAllTxData;
        getControlInfo();
        sortFramesAndLinks();
        // TstV.x = static_cast<physics_Num>( RotorHead->m_mainRotor.AilCyclic );
        // TstV.y = static_cast<physics_Num>( RotorHead->m_mainRotor.Collective );
        // TstV.z = static_cast<physics_Num>( RotorHead->m_mainRotor.EleCyclic );
        displayLocalVector(CB_MODEL, TstV, Org);
    }

    // levitates the model for balance test etc
    void HeliAero::levitate(f32 Time, f32 SpinTime, f32 SpinSpeed)
    {
        physics_Vec E, F, Loc, Target;
        f32 Gain = 10.0f;
        Target.x = static_cast<physics_Num>(1.0); // set the target position (in world frame)
        Target.y = static_cast<physics_Num>(0.8);
        Target.z = static_cast<physics_Num>(-2.0);
        readModelData();
        getAllTxData();
        E = VDif(Target, m_modelPosition);
        E.y = E.y + static_cast<physics_Num>(6.3) * static_cast<physics_Num>(9.81);
        E = VecToFrame(E, m_modelFrame);
        F = VSum(VScale(E, 1), VScale(m_modelVelocity, -1));
        F.z = F.z - static_cast<physics_Num>(0.1) * m_modelAngularVelocity.x;
        F.x = F.x + static_cast<physics_Num>(0.1) * m_modelAngularVelocity.z;
        Loc.x = 0;
        Loc.y = 0.3;
        Loc.z = 0;
        addLocalForce(CB_MODEL, F, Loc, 0);
        displayLocalVector(CB_MODEL, F, Loc);
        WP_LOG(StringUtil::toString( m_modelFrame.m_yAxis.z ));
        // if Time<SpinTime then F.x:= 1*(SpinSpeed+m_modelAngularVelocity.y) else
        // F.x:=1*(m_modelAngularVelocity.y);  //add in a tail force to get the model doing a piro
        F.x = static_cast<physics_Num>(10) * m_txChannel[YAW_CHANNEL];
        F.y = static_cast<physics_Num>(0);
        F.z = static_cast<physics_Num>(0);
        Loc.x = static_cast<physics_Num>(0);
        Loc.y = static_cast<physics_Num>(0);
        Loc.z = static_cast<physics_Num>(-0.9);
        addLocalForce(CB_MODEL, F, Loc, 0);
        displayLocalVector(CB_MODEL, F, Loc);
    } // levitate

    void HeliAero::getModelMassProps(f32 &ModelMass, f32 &PMoI, f32 &RMoI, f32 &YMoI,
                                     f32 &HeadMass)
    {
        ModelMass = m_modelWeight.m_sVal;
        PMoI = m_modelMoI.m_vVal.x;
        RMoI = m_modelMoI.m_vVal.z;
        YMoI = m_modelMoI.m_vVal.y;
        HeadMass = m_mainRotorWeight.m_sVal;
    }

    void HeliAero::setDataPath(const StringW &param)
    {
        m_dataPath = param;

        // LogManager &logManager = LogManager::getInstance();

        ////#ifndef WP_PLATFORM_WIN32
        // std::string filePath = "HeliAero.log";
        // logManager.open( filePath );
        // WP_LOG( "log opened: " + filePath );
        // #endif
    }

    StringW HeliAero::getDataPath() const
    {
        return m_dataPath;
    }

    void HeliAero::setCurrentRPM(f32 rpm)
    {
        auto fFixedRPM = rpm * m_mainGearTeeth.m_sVal / m_pinionGearTeeth.m_sVal;
        m_esc->m_fixedRpm = fFixedRPM;
        m_esc->m_reqRpm = fFixedRPM;
        m_esc->m_targetRpm = fFixedRPM;

        m_theGovernor->m_fixedRpm = fFixedRPM;
        // m_theGovernor->TargetRPM = fFixedRPM;
    }

    // Exports ReadDefaultParameterValues, //
    //	ReadCurrentHeliParameters, setFunc,   //The procedure used to initialize the Python callback
    // function 	callFunc,  //The procedure used only to allow Python to make a test callback to
    // itself! 	SetTeeter, SetFlybar, SetGroundEffect, SetLinkageControl, GateForces,
    // SetCDMultiplier, HeadRPMToPy, PackStateToPy, PackVoltsToPy, MotorPowerToPy, EngineRPMToPy,
    // SoundRPMToPy, MaxMainAttackToPy, MaxTailAttackToPy, SmokeFlow, SoundData, ResetAll,
    // WorkBenchFrame, MainFrame, TestDisplay, Levitate, GetModelMassProps, SetWind, PassWind;

    FILE *pConsole = nullptr;

    HeliAero::HeliAero()
    {
        m_roair = 1.225;

        m_loops = 0;
        m_maxDeltaT = 0.0f;
        m_minDeltaT = 0.0f;
        m_lastDeltaT = 0.0f;
        m_thisDeltaT = 0.0f;
        m_nextDeltaT = 0.0f;
        m_meanDeltaT = 0.0f;
        m_simTime = 0.0f;
        m_sumOfDeltas = 0.0f;
        m_inducedSmoothing = 0.0f;
        m_interferenceFactor = 0.0f;
        m_geMax = 1.0f;
        m_geDecay = 2.0f;
        m_alphaLMaxMain = 0.0f;
        m_alphaLMaxTail = 0.0f;
        m_tailGyroInput = 0.0f;
        m_tailGyroGain = 0.0f;
        m_tailGyroOutput = 0.0f;
        m_rxThrottleSig = 0.0f;
        m_engineRPM = 0.0f;
        m_governorOutput = 0.0f;
        m_engineThrottlePosition = 0.0f;
        m_engineOutput = 0.0f;

        m_clFiddle = 0.5f;
        m_cdFiddle = 0.45f;

        // test
        m_clFiddle = 1.0f;
        m_cdFiddle = 0.6f;

        m_clFiddle = static_cast<physics_Num>(1.0);
        m_cdFiddle = static_cast<physics_Num>(1.0);
        m_nitroFiddle = static_cast<physics_Num>(0.27);

        // m_clFiddle = PFloat(0.959495);
        // m_cdFiddle = PFloat(0.47389);
        // m_nitroFiddle = PFloat(0.164118);

        m_topDownTorque = 0.0f;
        m_bottomUpTorque = 0.0f;

        m_pack = std::make_shared<CBatteryPack>();
        m_motor = std::make_shared<CEMotor>();
        m_esc = std::make_shared<CESController>();

        m_rotorHead = std::make_shared<TRotorHead>();
        m_tailRotor = std::make_shared<TRotor>();
        m_theVBar = std::make_shared<CFlybarlessUnit>();
        m_theEngineClutch = std::make_shared<EngineClutchUnit>();
        m_theGovernor = std::make_shared<CGovernorUnit>();

        for(f32 &i : m_va)
        {
            i = 0.0f;
        }

        m_dataPath = L"";

        defineGovernorParameters();
        defineGyroParameters();

        m_engineFileName = "HeliEngine.csv";
        m_pCurveFileName = "DefaultPowerCurves.csv";

#if defined WP_PLATFORM_WIN32
        AllocConsole();
        freopen_s(&pConsole, "CONOUT$", "wb", stdout);

        // Get Handle to Console
        HWND hConsole = GetConsoleWindow();

        // Set the Console to Hidden
        ShowWindow(hConsole, SW_SHOW);
#endif
    }

    HeliAero::~HeliAero()
    {
    }

    void HeliAero::defineGyroParameters()
    {
        m_gyroServoOffset.m_name = "GyroServoOffset";
        m_gyroServoOffset.m_vType = ASingle;
        m_gyroThrowLimit1.m_name = "GyroThrowLimit1";
        m_gyroThrowLimit1.m_vType = ASingle;
        m_gyroThrowLimit2.m_name = "GyroThrowLimit2";
        m_gyroThrowLimit2.m_vType = ASingle;
        m_gyroStickDeadband.m_name = "GyroStickDeadband";
        m_gyroStickDeadband.m_vType = ASingle;
        m_gyroStickSensitivity.m_name = "GyroStickSensitivity";
        m_gyroStickSensitivity.m_vType = ASingle;
        m_gyroStickExponential.m_name = "GyroStickExponential";
        m_gyroStickExponential.m_vType = ASingle;
        m_gyroDirectCoupling.m_name = "GyroDirectCoupling";
        m_gyroDirectCoupling.m_vType = ASingle;
        m_gyroYawErrorLimit.m_name = "GyroYawErrorLimit";
        m_gyroYawErrorLimit.m_vType = ASingle;
        m_gyroConventionalGain.m_name = "GyroConventionalGain";
        m_gyroConventionalGain.m_vType = ASingle;
        m_gyroHLRange.m_name = "GyroHLRange";
        m_gyroHLRange.m_vType = ASingle;
        m_gyroHLGain.m_name = "GyroHLGain";
        m_gyroHLGain.m_vType = ASingle;
        m_gyroSenseReverse.m_name = "GyroSenseReverse";
        m_gyroSenseReverse.m_vType = ABool;
        m_gyroHLKillTime.m_name = "GyroHLKillTime";
        m_gyroHLKillTime.m_vType = ASingle;
        m_gyroHLDecay.m_name = "GyroHLDecay";
        m_gyroHLDecay.m_vType = ASingle;
        m_gyroAccTimeConst.m_name = "GyroAccTimeConst";
        m_gyroAccTimeConst.m_vType = ASingle;
        m_gyroAccGain.m_name = "GyroAccGain";
        m_gyroAccGain.m_vType = ASingle;
        m_gyroAccTermLimit.m_name = "GyroAccTermLimit";
        m_gyroAccTermLimit.m_vType = ASingle;
        m_gyroLeftStopGain.m_name = "GyroLeftStopGain";
        m_gyroLeftStopGain.m_vType = ASingle;
        m_gyroRightStopGain.m_name = "GyroRightStopGain";
        m_gyroRightStopGain.m_vType = ASingle;
    }

    void HeliAero::gyroFindAllParameterValuesIn(const String &XMLString)
    {
        // holds the lower case conveted version of the parameter XML string
        auto LCPS = StringUtil::make_lower(XMLString);

        AerodymanicsUtil::findValueOf(m_gyroServoOffset, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroThrowLimit1, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroThrowLimit2, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroStickDeadband, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroStickSensitivity, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroStickExponential, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroDirectCoupling, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroYawErrorLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroConventionalGain, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroHLRange, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroHLGain, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroSenseReverse, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroHLKillTime, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroHLDecay, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroAccTimeConst, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroAccGain, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroAccTermLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroLeftStopGain, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroRightStopGain, LCPS);
        m_tailGyro.m_servoOffset = m_gyroServoOffset.m_sVal; // note  lack of scaling
        m_tailGyro.m_throwLimit1 = ThrowLimitScale * m_gyroThrowLimit1.m_sVal;
        m_tailGyro.m_throwLimit2 = ThrowLimitScale * m_gyroThrowLimit2.m_sVal;
        m_tailGyro.m_stickDeadBand = static_cast<physics_Num>(0.001) * m_gyroStickDeadband.m_sVal;
        m_tailGyro.m_stickSensitivity =
            static_cast<physics_Num>(0.15) * m_gyroStickSensitivity.m_sVal;
        m_tailGyro.m_stickExpo = static_cast<physics_Num>(0.01) * m_gyroStickExponential.m_sVal;
        m_tailGyro.m_directGain = static_cast<physics_Num>(0.01) * m_gyroDirectCoupling.m_sVal;
        m_tailGyro.m_yawErrorLimit = static_cast<physics_Num>(0.1) * m_gyroYawErrorLimit.m_sVal;
        m_tailGyro.m_yawErrorGain =
            static_cast<physics_Num>(0.005) * m_gyroConventionalGain.m_sVal;
        m_tailGyro.m_hlRange = static_cast<physics_Num>(1.0) * m_gyroHLRange.m_sVal;
        m_tailGyro.m_hlGain = static_cast<physics_Num>(0.06) * m_gyroHLGain.m_sVal;
        m_tailGyro.m_senseReverse = m_gyroSenseReverse.m_bVal;
        m_tailGyro.m_hlKillTime = static_cast<physics_Num>(1.0) * m_gyroHLKillTime.m_sVal;
        m_tailGyro.m_hlDecay = static_cast<physics_Num>(1.0) * m_gyroHLDecay.m_sVal;
        m_tailGyro.m_accTC = static_cast<physics_Num>(1.0) * m_gyroAccTimeConst.m_sVal;
        m_tailGyro.m_accGain = static_cast<physics_Num>(0.0005) * m_gyroAccGain.m_sVal;
        m_tailGyro.m_accTermLimit = static_cast<physics_Num>(0.005) * m_gyroAccTermLimit.m_sVal;
        m_tailGyro.m_leftStopGain = static_cast<physics_Num>(0.01) * m_gyroLeftStopGain.m_sVal;
        m_tailGyro.m_rightStopGain = static_cast<physics_Num>(0.01) * m_gyroRightStopGain.m_sVal;

        m_tailGyro.m_throwLimit1 =
            ThrowLimitScale * m_gyroThrowLimit1.m_sVal + m_gyroServoOffset.m_sVal;
        m_tailGyro.m_throwLimit2 =
            ThrowLimitScale * m_gyroThrowLimit2.m_sVal + m_gyroServoOffset.m_sVal;
    }

    void HeliAero::gyroUserFindAllParameterValuesIn(const String &XMLString)
    {
        // holds the lower case conveted version of the parameter XML string
        auto LCPS = StringUtil::make_lower(XMLString);

        AerodymanicsUtil::findValueOf(m_gyroServoOffset, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroThrowLimit1, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroThrowLimit2, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroStickDeadband, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroStickSensitivity, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroStickExponential, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroDirectCoupling, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroYawErrorLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroConventionalGain, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroHLRange, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroHLGain, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroSenseReverse, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroHLKillTime, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroHLDecay, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroAccTimeConst, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroAccGain, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroAccTermLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroLeftStopGain, LCPS);
        AerodymanicsUtil::findValueOf(m_gyroRightStopGain, LCPS);
        m_tailGyro.m_servoOffset = m_gyroServoOffset.m_sVal; // note  lack of scaling
        m_tailGyro.m_throwLimit1 = ThrowLimitScale * m_gyroThrowLimit1.m_sVal;
        m_tailGyro.m_throwLimit2 = ThrowLimitScale * m_gyroThrowLimit2.m_sVal;
        m_tailGyro.m_stickDeadBand = static_cast<physics_Num>(0.001) * m_gyroStickDeadband.m_sVal;
        m_tailGyro.m_stickSensitivity =
            static_cast<physics_Num>(0.15) *
            (static_cast<physics_Num>(0.171) * m_gyroStickSensitivity.m_sVal);
        m_tailGyro.m_stickExpo = static_cast<physics_Num>(0.01) * m_gyroStickExponential.m_sVal;
        m_tailGyro.m_directGain = static_cast<physics_Num>(0.01) * m_gyroDirectCoupling.m_sVal;
        m_tailGyro.m_yawErrorLimit = static_cast<physics_Num>(0.1) * m_gyroYawErrorLimit.m_sVal;
        m_tailGyro.m_yawErrorGain =
            static_cast<physics_Num>(0.005) * m_gyroConventionalGain.m_sVal;
        m_tailGyro.m_hlRange = static_cast<physics_Num>(1.0) * m_gyroHLRange.m_sVal;
        m_tailGyro.m_hlGain = static_cast<physics_Num>(0.06) * m_gyroHLGain.m_sVal;
        m_tailGyro.m_senseReverse = m_gyroSenseReverse.m_bVal;
        m_tailGyro.m_hlKillTime = static_cast<physics_Num>(1.0) * m_gyroHLKillTime.m_sVal;
        m_tailGyro.m_hlDecay = static_cast<physics_Num>(1.0) * m_gyroHLDecay.m_sVal;
        m_tailGyro.m_accTC = static_cast<physics_Num>(1.0) * m_gyroAccTimeConst.m_sVal;
        m_tailGyro.m_accGain = static_cast<physics_Num>(0.0005) * m_gyroAccGain.m_sVal;
        m_tailGyro.m_accTermLimit = static_cast<physics_Num>(0.005) * m_gyroAccTermLimit.m_sVal;
        m_tailGyro.m_leftStopGain = static_cast<physics_Num>(0.01) * m_gyroLeftStopGain.m_sVal;
        m_tailGyro.m_rightStopGain = static_cast<physics_Num>(0.01) * m_gyroRightStopGain.m_sVal;

        m_tailGyro.m_throwLimit1 =
            ThrowLimitScale * m_gyroThrowLimit1.m_sVal + m_gyroServoOffset.m_sVal;
        m_tailGyro.m_throwLimit2 =
            ThrowLimitScale * m_gyroThrowLimit2.m_sVal + m_gyroServoOffset.m_sVal;
    }

    // forces the dll to read in the default heli parameters
    void HeliAero::readDefaultGyroParameters()
    {
        /*
            TextFile DFile;
            String XMLStr, RLine;
            try
            {
                DefineGyroParameters;
                GetDataPath;
                AssignFile(DFile, "DefaultHeliParams.xml");
                Reset(DFile);
                XMLStr = "";
                while (!EOF(DFile))
                {
                    Readln(DFile, RLine);//read a line of the XML file
                    XMLStr = XMLStr + RLine; //and add to the end of the XML string
                }
                CloseFile(DFile);
                FindAllParameterValuesIn(XMLStr);
            }
            catch (std::exception & Err)
            {
                WP_LOG_EXCEPTION(Err);
            }
            */

        try
        {
            // DefineGyroParameters();

            std::shared_ptr<HeliAero> heliAero = getSingleton();

#if defined WP_PLATFORM_WIN32
            StringW filePath = L"DefaultHeliParams.xml";
#else
            StringW filePath = heliAero->getDataPath() + L"/DefaultHeliParams.xml";
#endif

            auto xmlData = XmlUtil::getFromFile(filePath);
            gyroFindAllParameterValuesIn(xmlData);
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void HeliAero::readCurrentGyroParameters()
    {
        try
        {
            // DefineGyroParameters();
            std::shared_ptr<HeliAero> heliAero = getSingleton();
            StringW filePath = heliAero->getDataPath() + L"/model_data.xml";
            String xmlData = XmlUtil::getFromFile(filePath);
            gyroUserFindAllParameterValuesIn(xmlData);
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void HeliAero::govFindAllParameterValuesIn(const String &XMLString)
    {
        std::shared_ptr<HeliAero> heliAero = getSingleton();
        CGovernorUnit &governor = *heliAero->m_theGovernor;

        auto LCPS = StringUtil::make_lower(XMLString);

        AerodymanicsUtil::findValueOf(m_govAccelerationGain, LCPS);
        AerodymanicsUtil::findValueOf(m_govRPMGain, LCPS);
        AerodymanicsUtil::findValueOf(m_govPhaseGain, LCPS);
        AerodymanicsUtil::findValueOf(m_govAccelerationLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_govRPMErrorLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_govPhaseErrorLimit, LCPS);
        AerodymanicsUtil::findValueOf(m_govMinControlPoint, LCPS);
        AerodymanicsUtil::findValueOf(m_govReqHeadRPM, LCPS);
        AerodymanicsUtil::findValueOf(m_govRampRate, LCPS);
        AerodymanicsUtil::findValueOf(m_mainGearTeeth, LCPS);
        AerodymanicsUtil::findValueOf(m_pinionGearTeeth, LCPS);
        AerodymanicsUtil::findValueOf(m_govMode, LCPS);
        AerodymanicsUtil::findValueOf(m_govMinHeadRPM, LCPS);
        AerodymanicsUtil::findValueOf(m_govMaxHeadRPM, LCPS);

        // FindValueOf(EngineGearing,LCPS);

        governor.m_accGain = m_govAccelerationGain.m_sVal;
        governor.m_rpmGain = m_govRPMGain.m_sVal;
        governor.m_phaseGain = m_govPhaseGain.m_sVal;
        governor.m_accLimit = m_govAccelerationLimit.m_sVal;
        governor.m_rpmErrorLimit = m_govRPMErrorLimit.m_sVal;
        governor.m_phaseErrorLimit = m_govPhaseErrorLimit.m_sVal;
        governor.m_minControlPoint = m_govMinControlPoint.m_sVal;
        governor.m_rampRate = m_govRampRate.m_sVal;
        governor.m_mode = m_govMode.m_iVal; // apply the mode from the xml
        governor.m_fixedRpm =
            m_govReqHeadRPM.m_sVal * m_mainGearTeeth.m_sVal /
            m_pinionGearTeeth.m_sVal; // calculate the mode 1 (fixed) required engine rpm
        governor.m_rpmRangeBottom = m_govMinHeadRPM.m_sVal * m_mainGearTeeth.m_sVal /
                                    m_pinionGearTeeth.m_sVal; // calc the lowest remote engine rpm
        governor.m_rpmRangeTop = m_govMaxHeadRPM.m_sVal * m_mainGearTeeth.m_sVal /
                                 m_pinionGearTeeth.m_sVal; // calc the highest remote engine rpm
    }

    void HeliAero::defineGovernorParameters()
    {
        m_govAccelerationGain.m_name = "GovAccelerationGain";
        m_govAccelerationGain.m_vType = ASingle;
        m_govRPMGain.m_name = "GovRPMGain";
        m_govRPMGain.m_vType = ASingle;
        m_govPhaseGain.m_name = "GovPhaseGain";
        m_govPhaseGain.m_vType = ASingle;
        m_govAccelerationLimit.m_name = "GovAccelerationLimit";
        m_govAccelerationLimit.m_vType = ASingle;
        m_govRPMErrorLimit.m_name = "GovRPMErrorLimit";
        m_govRPMErrorLimit.m_vType = ASingle;
        m_govPhaseErrorLimit.m_name = "GovPhaseErrorLimit";
        m_govPhaseErrorLimit.m_vType = ASingle;
        m_govMinControlPoint.m_name = "GovMinControlPoint";
        m_govMinControlPoint.m_vType = ASingle;
        m_govReqHeadRPM.m_name = "GovReqHeadRPM";
        m_govReqHeadRPM.m_vType = ASingle;
        m_govRampRate.m_name = "GovRampRate";
        m_govRampRate.m_vType = ASingle;

        m_mainGearTeeth.m_name = "MainGearTeeth";
        m_mainGearTeeth.m_vType = ASingle;
        m_pinionGearTeeth.m_name = "PinionGearTeeth";
        m_pinionGearTeeth.m_vType = ASingle;

        m_govMode.m_name = "GovMode";
        m_govMode.m_vType = AInt;
        m_govMinHeadRPM.m_name = "GovMinHeadRPM";
        m_govMinHeadRPM.m_vType = ASingle;
        m_govMaxHeadRPM.m_name = "GovMaxHeadRPM";
        m_govMaxHeadRPM.m_vType = ASingle;
    }

    // forces the dll to read in the default heli parameters
    void HeliAero::readDefaultGovernorParameters() /* export */
    {
        try
        {
            // DefineGyroParameters();

            std::shared_ptr<HeliAero> heliAero = getSingleton();

#if defined WP_PLATFORM_WIN32
            StringW filePath = L"DefaultHeliParams.xml";
#else
            StringW filePath = heliAero->getDataPath() + L"/DefaultHeliParams.xml";
#endif

            const auto xmlData = XmlUtil::getFromFile(filePath);
            govFindAllParameterValuesIn(xmlData);
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void HeliAero::readCurrentGovernorParameters() /* export */
    {
        try
        {
            // DefineGyroParameters();
            std::shared_ptr<HeliAero> heliAero = getSingleton();
            StringW filePath = heliAero->getDataPath() + L"/model_data.xml";
            String xmlData = XmlUtil::getFromFile(filePath);
            govFindAllParameterValuesIn(xmlData);
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void HeliAero::setBailout(f32 bailGain, f32 collectiveValue, s32 bailValue)
    {
        m_theVBar->m_bailValue = bailValue;
        m_theVBar->m_bailGain = bailGain;
        m_theVBar->m_bailCollective = collectiveValue;
    }
}

extern "C" {
using namespace workphone;
using namespace vehicle;

void GetModelMassProps(f32 &ModelMass, f32 &PMoI, f32 &RMoI, f32 &YMoI, f32 &HeadMass)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->getModelMassProps(ModelMass, PMoI, RMoI, YMoI, HeadMass);
}

void Levitate(f32 Time, f32 SpinTime, f32 SpinSpeed)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->levitate(Time, SpinTime, SpinSpeed);
}

void MainFrame(f32 t, f32 dt)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->mainFrame(t, dt);
}

void WorkBenchFrame(f32 t, f32 dt, int ParamID, f32 ParamValue)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->workBenchFrame(t, dt, ParamID, ParamValue);
}

f32 MaxMainAttackToPy()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    return heliAero->maxMainAttackToPy();
}

f32 MaxTailAttackToPy()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    return heliAero->maxTailAttackToPy();
}

void PassWind()
{
    // saracen::PassWind();
}

void ReadCurrentHeliParameters()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->readCurrentHeliParameters();
}

void ReadDefaultParameterValues()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->readDefaultParameterValues();
}

void BootDLL()
{
#if defined SARACEN_PLATFORM_LINUX

    gHeliAero = std::make_shared<workphone::HeliAero>();

    InitHeliVars();
    InitWeather();
#endif
}

void ResetAll()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->resetAll();
}

void SetCDMultiplier(f32 CDFactor, f32 Gash)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->setCDMultiplier(CDFactor, Gash);
}

void SetFlybar(f32 R1, f32 R2, f32 C1, f32 C2, f32 Weight, f32 IntFac)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->setFlybar(R1, R2, C1, C2, Weight, IntFac);
}

void SetGroundEffect(f32 Max, f32 Decay)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->setGroundEffect(Max, Decay);
}

void SetLinkageControl(bool LC)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->setLinkageControl(LC);
}

void SetTeeter(f32 TeeterSpring, f32 TeeterDamping, f32 IndS)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->setTeeter(TeeterSpring, TeeterDamping, IndS);
}

void SetWeather(f32 speed, f32 direction, f32 turbulence, f32 groundHeight, f32 directionOffset,
                f32 fieldRoughness, f32 temperature, f32 pressure, f32 smallTurbulence)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    // SetWind( speed, direction, turbulence, groundHeight, directionOffset, fieldRoughness );

    //% Sim variables
    //	Sim_Pressure = 1.013 % mbar
    //	Sim_Temperature = 15 % ?C

    //	% Equation variables
    //	p = Pressure * 100000; % convert mbar to Pa
    //	R = 287.05; % constant
    //	T = 15 + 273.15; % convert ?C to Kelvin

    //	pdry = p / (R * T); % Calculate Pressure
    //	pdry =
    //	1.2247

    auto p = (static_cast<double>(pressure) / 1000.0) * 100000; //% convert mbar to Pa
    auto r = 287.05; // constant
    auto t = temperature + 273.15; //% convert ?C to Kelvin
    heliAero->m_roair = p / (r * t);
}

void SmokeFlow(f32 &RootSmokeFlowX, f32 &RootSmokeFlowY, f32 &RootSmokeFlowZ, f32 &TipSmokeFlowX,
               f32 &TipSmokeFlowY, f32 &TipSmokeFlowZ)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();

    physics_Num fRootSmokeFlowX;
    physics_Num fRootSmokeFlowY;
    physics_Num fRootSmokeFlowZ;
    physics_Num fTipSmokeFlowX;
    physics_Num fTipSmokeFlowY;
    physics_Num fTipSmokeFlowZ;

    heliAero->smokeFlow(fRootSmokeFlowX, fRootSmokeFlowY, fRootSmokeFlowZ, fTipSmokeFlowX,
                        fTipSmokeFlowY, fTipSmokeFlowZ);

    RootSmokeFlowX = fRootSmokeFlowX;
    RootSmokeFlowY = fRootSmokeFlowY;
    RootSmokeFlowZ = fRootSmokeFlowZ;
    TipSmokeFlowX = fTipSmokeFlowX;
    TipSmokeFlowY = fTipSmokeFlowY;
    TipSmokeFlowZ = fTipSmokeFlowZ;
}

void SoundData(f32 &TipPress1, f32 &TipPress2, f32 &TipPress3, f32 &TipPress4, f32 &TipDrg1,
               f32 &TipDrg2, f32 &TipDrg3, f32 &TipDrg4)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();

    physics_Num fTipPress1;
    physics_Num fTipPress2;
    physics_Num fTipPress3;
    physics_Num fTipPress4;
    physics_Num fTipDrg1;
    physics_Num fTipDrg2;
    physics_Num fTipDrg3;
    physics_Num fTipDrg4;

    heliAero->soundData(fTipPress1, fTipPress2, fTipPress3, fTipPress4, fTipDrg1, fTipDrg2,
                        fTipDrg3, fTipDrg4);

    TipPress1 = fTipPress1;
    TipPress2 = fTipPress2;
    TipPress3 = fTipPress3;
    TipPress4 = fTipPress4;

    TipDrg1 = fTipDrg1;
    TipDrg2 = fTipDrg2;
    TipDrg3 = fTipDrg3;
    TipDrg4 = fTipDrg4;
}

void TestDisplay()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->testDisplay();
}

void callFunc()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->callFunc();
}

void setFunc(void *FuncPtr)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->setFunc(FuncPtr);
}

f32 HeadRPMToPy()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    return heliAero->headRPMToPy();
}

f32 PackStateToPy()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    return heliAero->packStateToPy();
}

f32 PackVoltsToPy()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    return heliAero->packVoltsToPy();
}

f32 MotorPowerToPy()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    return heliAero->motorPowerToPy();
}

f32 EngineRPMToPy()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    return heliAero->engineRPMToPy();
}

f32 SoundRPMToPy()
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    return heliAero->soundRPMToPy();
}

void SetDataPath(void *str)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();

    auto pStr = static_cast<wchar_t *>(str);
    std::cout << pStr << std::endl;

    heliAero->setDataPath(pStr);
}

void SetCurrentRPM(f32 currentRPM)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->setCurrentRPM(currentRPM);
}

void SetBailout(f32 bailoutValue, f32 bailoutValue2, int bailoutValue3)
{
    std::shared_ptr<HeliAero> &heliAero = HeliAero::getSingleton();
    heliAero->setBailout(bailoutValue, bailoutValue2, bailoutValue3);
}

void SetCL(f32 value)
{
    auto &heliAero = HeliAero::getSingleton();
    heliAero->m_clFiddle = value;
}

void SetCD(f32 value)
{
    auto &heliAero = HeliAero::getSingleton();
    heliAero->m_cdFiddle = value;
}

void SetNitroMultiplier(f32 value)
{
    auto &heliAero = HeliAero::getSingleton();
    heliAero->m_nitroFiddle = value;
}
}
