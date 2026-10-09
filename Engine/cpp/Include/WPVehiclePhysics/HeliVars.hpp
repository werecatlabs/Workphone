#ifndef HeliVarsH
#define HeliVarsH

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "WPVehiclePhysics/HelicopterBody.hpp"
#include "WPVehiclePhysics/ClimbTransitionData.hpp"
#include "WPVehiclePhysics/FoilLookup.hpp"
#include "WPVehiclePhysics/GearTrain.hpp"
#include "WPVehiclePhysics/Lookup.hpp"
#include "WPVehiclePhysics/Servo.hpp"
#include "WPVehiclePhysics/TailLinkage.hpp"
#include <Workphone/Core/StringTypes.hpp>
#include "VecMath.hpp"
#include <string>
#include <memory>

namespace workphone::vehicle
{
    using TPassArray = float[11];
    /* range 0..10*/
    // was an array 0..5 type for the array used for callback data exchange with Python

    using TPassAPtr = float *; // type for pointer to the callback data exchange array

    // declares a a function pointer type for the callback
    using TProcedurePtr = void (*)(int N1, int N2, float *TP);

    //******************************* END of TYPE DECLARATIONS ************************************

    //***************************** CONSTANTS DECLARATIONS ****************************************

    /// Primary RC channel indices (throttle, aileron, elevator, yaw, gear, collective, aux1, aux2)
    extern const int THR_CHANNEL;
    extern const int AIL_CHANNEL;
    extern const int ELE_CHANNEL;
    extern const int YAW_CHANNEL;
    extern const int GEAR_CHANNEL;
    extern const int COL_CHANNEL;
    extern const int AUX1_CHANNEL;
    extern const int AUX2_CHANNEL;

    /// IPC / message identifiers for querying or commanding the aircraft
    extern const int GET_TX_CHANNEL;
    extern const int GET_LOCAL_ANGULAR_VELOCITY;
    extern const int GET_LOCAL_LINEAR_VELOCITY;
    extern const int ADD_LOCAL_FORCE;
    extern const int ADD_LOCAL_TORQUE;
    extern const int DISPLAY_LOCAL_VECTOR;
    extern const int DISPLAY_LOCAL_VECTOR_BY_ID;
    extern const int SET_MASS_PROPS;
    extern const int GET_GLOBAL_POSITION;
    extern const int GET_GLOBAL_ORIENTATION;
    extern const int CAST_LOCAL_RAY;
    extern const int GET_CONTROL_ANGLES;
    extern const int GET_GYRO_OUTPUT;
    extern const int GET_GOVERNOR_OUTPUT;
    extern const int GET_ENGINE_OUTPUT;
    extern const int GET_TX_CHANNELS;

    extern const int GET_WORLD_ANGULAR_VELOCITY;
    extern const int GET_WORLD_LINEAR_VELOCITY;
    extern const int ADD_FORCE;
    extern const int ADD_TORQUE;

    extern const int GET_ANGULAR_VELOCITY;
    extern const int GET_LINEAR_VELOCITY;
    extern const int ADD_LOCAL_FORCE;
    extern const int ADD_LOCAL_TORQUE;
    extern const int DISPLAY_LOCAL_VECTOR;
    extern const int SET_MASS_PROPS;
    extern const int GET_GLOBAL_POSITION;
    extern const int GET_GLOBAL_ORIENTATION;
    extern const int CAST_LOCAL_RAY;
    extern const int GET_CONTROL_ANGLES;
    // For getting the output from the linkages V[0] = col, V[1] = ail, V[2] = ele, V[3] = tail pitch

    extern const int GET_GYRO_OUTPUT;
    // VA[0] = Rud, VA[1] = gain, VA[2] = dt, VA[3] = yaw rate. Returns gyro out in VA[0]

    extern const int GET_GOVERNOR_OUTPUT;
    extern const int GET_ENGINE_OUTPUT; // VA[0] = Thr, VA[1] = RPM. Returns VA[0] = Power (Watts)

    extern const int SET_SERVO_INPUT; // VA[0] = the servo position and sunfunc sets the channel

    extern const int
    RESET_GOVERNOR; // used to reset governor and load its parameters from the XML files

    extern const int GET_HELI_CTRL_INFO;
    // returns swashplate collective position, pitch angle, roll angle and tailrotor blade angle in
    // v[0] through v[3]

    extern const int FLYBAR_CTRL_UPDATE;
    // inputs -> v[0] = pitch gyro output , v[1] = roll gyro output, v[2] = yaw gyro output, .
    // outputs   v[0] = aileron , v[1] = elevator, v[2] = collective, v[3] rudder, v[4] = aux
    // (channel 5)

    extern const int GET_TX_CHANNELS;
    extern const int RESET_GYRO;
    extern const int THR_CHANNEL; // Channel constants for callback #1

    extern const int AIL_CHANNEL;
    extern const int ELE_CHANNEL;
    extern const int YAW_CHANNEL;
    extern const int GEAR_CHANNEL;
    extern const int GYRO_GAIN_CHANNEL; // added to make the gyro code clearer

    extern const int COL_CHANNEL;
    extern const int AUX1_CHANNEL;
    extern const int GOV_RPM_CHANNEL; // added to make the governor code clearer

    extern const int AUX2_CHANNEL;
    extern const int CB_MODEL; // callback constants for subFunc

    extern const int CB_ROTOR_HEAD;

    //***************************** GLOBAL VARS DECLARATIONS **************************************

    // extern TPassArray VA; //declares the data passing array for the callback

    // extern float TxChannel[8/* range 0..7*/];//transmitter data

    // extern TProcedurePtr FnCallback; //function pointer instance for callback

    // extern TPassAPtr TP; //array for passing callback data

    // extern int CBFun, CBSub;//used as Function and SubFunction values for callback

    // extern TParam RotorHeadHubPosition;
    // extern TParam ModelWeight;
    // extern TParam ModelMoI;
    // extern TParam MainRotorWeight;
    // extern TParam RotorHeadBoltRadius;
    // extern TParam RotorHeadNumBlades;
    // extern TParam RotorHeadSwashToMainMix;
    // extern TParam RotorHeadSwashToFBMix;
    // extern TParam RotorHeadFBtoMainMix;
    // extern TParam RotorHeadMaxSwashEle;
    // extern TParam RotorHeadMaxSwashAil;
    // extern TParam RotorHeadCollectivePerMM;
    // extern TParam RotorHeadShaftRake;
    // extern TParam RotorHeadShaftTilt;
    // extern TParam RotorHeadTeeterFC;
    // extern TParam RotorHeadTeeterDC;
    // extern TParam MainBladeLength;
    // extern TParam MainBladeRootRadius;
    // extern TParam MainBladeTipChord;
    // extern TParam MainBladeRootChord;
    // extern TParam MainBladeTwist;
    // extern TParam MainBladeWeight;
    // extern TParam MainBladeRadOfGyr;
    // extern TParam TailHubPosition;
    // extern TParam TailHubBoltRadius;
    // extern TParam TailNumBlades;
    // extern TParam TailMaxPitch;
    // extern TParam TailPitchTrim;
    // extern TParam TailBladeLength;
    // extern TParam TailBladeRootRadius;
    // extern TParam TailBladeTipChord;
    // extern TParam TailBladeRootChord;
    // extern TParam TailBladeTwist;
    // extern TParam TailBladeWeight;
    // extern TParam TailBladeRadOfGyr;
    // extern TParam Flybarless;
    // extern TParam PaddleWeight;
    // extern TParam PaddleSpan;
    // extern TParam PaddleRootChord;
    // extern TParam PaddleTipChord;
    // extern TParam PaddleThreadOnLength;
    // extern TParam FlybarRodDiameter;
    // extern TParam FlybarRodDensity;
    // extern TParam FlybarRodLength;
    // extern TParam MainToFBInterference;
    // extern TParam GroundEffectMax;
    // extern TParam GroundEffectDecay;
    // extern TParam VRMultiplier;
    // extern TParam CDMultiplier;

    //// RotorHeadTeeterStiffness: TParam;
        //// RotorHeadTeeterDamping: TParam;

    // extern TParam MainGearTeeth;
    // extern TParam PinionGearTeeth;
    ////EngineGearing: TParam;  //replaced by the teeth counts

    // extern TParam TailGearing;
    // extern TParam DrivenTail;
    // extern TParam BodyCdA;
    // extern TParam BodyDragCentre;
    // extern TParam EnginePeakPower;
    // extern TParam EnginePeakPowerRPM;
    // extern TParam EngineMoI;
    // extern TParam ClutchMoI;
    // extern TParam ClutchConst;
    // extern TParam ClutchBiteRPM;
    // extern TParam VBarStickDeadBand;
    // extern TParam VBarStickSensitivity;
    // extern TParam VBarStickExponential;
    // extern TParam VBarAilGain;
    // extern TParam VBarEleGain;
    // extern TParam VBarAngleLimit;
    // extern TParam VBarDecayTime;
    // extern TParam VBarStabilize;
    // extern TParam VBarStabGain;
    // extern TParam VBarDirectMix;
    // extern TParam VBarAilStickFilter;
    // extern TParam VBarEleStickFilter;

    // Electric power Parameters
    // Battery pack params

    // extern TParam ElectricPower; //true if model has electric power

    // extern TParam EmulateBattery; //true if battery discharge is to be modelled

    // extern TParam CellsInPack;
    // extern TParam CellFullV; //cell off-load voltage when fully charged

    // extern TParam CellFlatV;//cell off-load voltage when flat

    // extern TParam CellR; //ESR of an individual cell of the pack when full

    // extern TParam CellAHr;  //cell capacity in Amp.Hrs
    //		//Electric motor parameters

    // extern TParam MotorMoI; //

    // extern TParam MotorKV; //RPM/volt of the motor

    // extern TParam MotorEfficiency; //fractional efficiency

    // extern TParam MotorNoLoadCurrent;  //stated no-load current

    // extern TParam MotorR; //the resistance of the motor windings

    // extern TParam MotorILimit;  //the stated max continuous current
    //		//ESC parameters

    // extern TParam ESCSlowRampTC;
    // extern TParam ESCFastRampTC;
    // extern TParam ESCSoftStartDelay;
    // extern TParam ESCAccelerationGain;
    // extern TParam ESCRPMGain;
    // extern TParam ESCPhaseGain;
    // extern TParam ESCAccelerationLimit;
    // extern TParam ESCRPMErrorLimit;
    // extern TParam ESCPhaseErrorLimit;
    // extern TParam ESCMinControlPoint;
    // extern TParam ESCILimit;
    // extern TParam ESCCutoffV;
    // extern TParam ESCResistance;
    // extern TParam VisualTailReverse;
    // extern TParam DirectTailControl;
    // extern TParam BearingFriction;

    // extern std::shared_ptr<TRotorHead> RotorHead;  //declare the main rotor
    // extern std::shared_ptr<TRotor> TailRotor;      //declare the tail rotor

    // extern Body Body;
    // extern Servo TailServo;
    // extern TailLinkage TailLinkage;
    // extern Servo EleServo;
    // extern Servo AilServo;
    // extern Servo ColServo;
    // extern Vec Wind; //wind vector at model (in ground frame of reference)

    // extern Vec ModelVelocity;  //the velocity of the model in its own frame of ref as fetched from
    // the physics engine

    // extern Vec ModelAngularVelocity; //the model angular velocity in its own frame as fetched from
    // the physics engine

    // extern Vec ModelPosition; //the position of the model in the ground frame

    // extern FrameOfRef ModelFrame;  //the orientation of the model in the ground frame as fetched
    // from the physics engine

    // extern FrameOfRef VisualRotorFrame; //gives the attitude of the visual rotor so we can servo
    // it to attitude of the aerodynamic MainRotor

    // extern Vec VisualRotorAngularVelocity; //give the AV of the visual rotor to allow damping

    // extern GearTrain TheGears;

    ////AFoilLookup:TFoilTable; //note lookup 0 corresponds to Alpha = -Math<physics_Num>::pi(), 200
        /// to Alpha = 0 and 400 to Alpha = Math<physics_Num>::pi()

    // extern LookupArray ClimbLookup; //note 0 = -10Vt climb (i.e. descent!) 99 = zero climb.  199
    // = +10Vt climb

    // extern LookupArray TransLookup; //note 0 = 0 translation and 199 = +20Vt translation

    // extern float MaxDeltaT, MinDeltaT, LastDeltaT, ThisDeltaT, NextDeltaT, MeanDeltaT; //globals
    // to hold past, current and predicted time-steps

    // extern float SimTime, SumOfDeltas; //The time as passed from the physics engine

    // extern size_t Loops;//holds the number of frames called

    // extern float InducedSmoothing; //The factor by which the inflow field is smoothed round the
    // disk

    // extern float InterferenceFactor;//used to set the proportion of the main rotor induced flow is
    // used as flybar interference

    //		//Ground effect factors The Ground effect 'upflow' = rotor induced flow * GEMax
    //*Exp(-GEDecay*Height/RotorRadius)

    // extern float GEMax; //the maximum value of the effective upflow (as a fraction of the main
    // rotor induced flow)

    // extern float GEDecay; //the decay rate of the ground effect

    // extern float AlphaLMaxMain; //holds the max value of the local angle of attack in the outer
    // part of the blade (used for sound generation)

    // extern float AlphaLMaxTail; //holds the max value of the local angle of attack in the outer
    // part of the blade (used for sound generation)

    // extern bool LinkageControl; //set to force aerodynamics to take control inputs as control
    // angles

    // extern float TailGyroInput; //used in dealing with external gyro DLL

    // extern float TailGyroGain;   //used in dealing with external gyro DLL

    // extern float TailGyroOutput; //used in dealing with external gyro DLL

    // extern float RxThrottleSig; //the throttle signal at the Rx output

    // extern float EngineRPM; //the current engine RPM

    // extern float GovernorOutput;//the returned governor output from external governor DLL

    //		//MotorRPM:Single; //the current electric motor RPM

    // extern float EngineThrottlePosition;//

    // extern float EngineOutput; // the returned engine output from external engine DLL

    // extern float CDFiddle; // a 'fiddle factor' passed from the python to change the CD values of
    // the lookup by a factor

    // extern Vec HopTestForce; //to be used in the hop test ramp

    // extern Vec TotalFlow;  //holds the airframe flow with wind added

    // extern bool BodyForcesOn; //set to true to turn on the outputting of body forces

    // extern bool RotorForcesOn; //set to turn on the outputting of the visual rotor steering forces

    // extern float TopDownTorque, BottomUpTorque; //hold the mainshaft torque calculated two ways

    // extern std::wstring DataPath; //used to access all the data files read by the dll

    extern const double RPMToOmega;
    extern const double OmegaToRPM;
    // extern const int Locked;
    // extern const int Overrun;

    extern void InitHeliVars();
}

#endif //  HeliVarsH
