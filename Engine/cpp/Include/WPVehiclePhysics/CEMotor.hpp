#ifndef CEMotor_h__
#define CEMotor_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include <Workphone/Interface/Vehicle/IBatteryPack.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>

namespace workphone::vehicle
{
    /*
    Notes:
    The motor data links various parameters. The KV and the torque constant are linked via the efficiency
    if the efficiency is 100% then GammaI, the torque/amp is related to the KV thus:-
    GammaI = 30/(Math<physics_Num>::pi()*KV)
    And for an efficiency E
    GammaI = 30*E/(Math<physics_Num>::pi()*KV)
    Maxon data shows that they assume the efficiency = 100%
    So it seems we have two handles on the losses in the motor - the winding resistance and the no-load
    current. The winding resistance means that the back EMF of the motor (and hence its speed) is lower
    than the applied voltage by the drop across the winding resistance. This gives the motor its speed
    drop with load. As the load increases the current must increase to provide more torque. This
    increases the voltage drop across the winding resistance and lowers the back EMF of the motor. Checks
    on the Maxon data show that the torque constant, the speed constant(KV), the speed/torque gradient,
    and the winding resistance are in agreement with this calculation to a close degree.

    The no-load current can be related to and equivalent effective internal loss torque though this is
    probably a combination of bearing friction air friction and iron losses in the motor.

    Test on a Maxon motor running light at various voltages showed the no-load current was substantially
    independent of the drive voltage suggesting the no-load current is due to an almost constant bearing
    friction torque.

    Thus we will simply subtract the no-load current from the motor current in calculating the output
    torque.
    */
    class WPVehiclePhysics_API CEMotor
    {
    public:
        CEMotor() = default;
        ~CEMotor() = default;

        physics_Num m_motorMoI = static_cast<physics_Num>( 0.0 ); //
        physics_Num m_motorKv = static_cast<physics_Num>( 0.0 );  // RPM/volt of the motor
        physics_Num m_motorTi =
            static_cast<physics_Num>( 0.0 ); // torque constant (in N.m/A).Derived from KV and efficiency
        physics_Num m_motorEfficiency = static_cast<physics_Num>( 0.0 ); // fractional efficiency
        physics_Num m_noLoadCurrent = static_cast<physics_Num>( 0.0 );   // stated no-load current
        physics_Num m_frictionFactor = static_cast<physics_Num>( 0.0 );  // loss factor derived from the
        physics_Num m_motorR = static_cast<physics_Num>( 0.0 );   // the resistance of the motor windings
        physics_Num m_motorRpm = static_cast<physics_Num>( 0.0 ); // the current motor RPM
        physics_Num m_motorOmega = static_cast<physics_Num>( 0.0 );
        physics_Num m_motorEmf = static_cast<physics_Num>( 0.0 );
        physics_Num m_motorCurrent = static_cast<physics_Num>( 0.0 ); // the instantanious motor current
        physics_Num m_motorTorque = static_cast<physics_Num>( 0.0 );  // the instantanious motor torque
        physics_Num m_motorAccel = static_cast<physics_Num>( 0.0 );   // rate of acceleration of motor:
        physics_Num m_motorPower = static_cast<physics_Num>( 0.0 );
        physics_Num m_spragRpm = static_cast<physics_Num>( 0.0 );
        physics_Num m_spragOmega = static_cast<physics_Num>( 0.0 );
        physics_Num m_transmittedTorque = static_cast<physics_Num>( 0.0 );
        physics_Num m_currentLimit = static_cast<physics_Num>( 0.0 );
        // the stated motor continuous current limit
        s32 m_spragMode;
    }; // TEMotor
} // namespace workphone::vehicle

#endif // CEMotor_h__
