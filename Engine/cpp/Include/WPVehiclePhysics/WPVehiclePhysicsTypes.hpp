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
#include "VehiclePhysicsMath.hpp"
#include <string>
#include <memory>

namespace workphone::vehicle
{
    using TPassArray = float[11];
    /* range 0..10*/
    // was an array 0..5 type for the array used for callback data exchange with Python

    using TPassAPtr = float *;  // type for pointer to the callback data exchange array

    // declares a a function pointer type for the callback
    using TProcedurePtr = void ( * )( int N1, int N2, float *TP );

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
    extern const int GET_ENGINE_OUTPUT;  // VA[0] = Thr, VA[1] = RPM. Returns VA[0] = Power (Watts)

    extern const int SET_SERVO_INPUT;  // VA[0] = the servo position and sunfunc sets the channel

    extern const int
        RESET_GOVERNOR;  // used to reset governor and load its parameters from the XML files

    extern const int GET_HELI_CTRL_INFO;
    // returns swashplate collective position, pitch angle, roll angle and tailrotor blade angle in
    // v[0] through v[3]

    extern const int FLYBAR_CTRL_UPDATE;
    // inputs -> v[0] = pitch gyro output , v[1] = roll gyro output, v[2] = yaw gyro output, .
    // outputs   v[0] = aileron , v[1] = elevator, v[2] = collective, v[3] rudder, v[4] = aux
    // (channel 5)

    extern const int GET_TX_CHANNELS;
    extern const int RESET_GYRO;
    extern const int THR_CHANNEL;  // Channel constants for callback #1

    extern const int AIL_CHANNEL;
    extern const int ELE_CHANNEL;
    extern const int YAW_CHANNEL;
    extern const int GEAR_CHANNEL;
    extern const int GYRO_GAIN_CHANNEL;  // added to make the gyro code clearer

    extern const int COL_CHANNEL;
    extern const int AUX1_CHANNEL;
    extern const int GOV_RPM_CHANNEL;  // added to make the governor code clearer

    extern const int AUX2_CHANNEL;
    extern const int CB_MODEL;  // callback constants for subFunc

    extern const int CB_ROTOR_HEAD;

    extern const double RPMToOmega;
    extern const double OmegaToRPM;
    // extern const int Locked;
    // extern const int Overrun;

}  // namespace workphone::vehicle

#endif  //  HeliVarsH
