#ifndef Flybarless1H
#define Flybarless1H

#include "VecMath.hpp"
#include "HeliVars.hpp"
#include "WPVehiclePhysics/FrameOfRef.hpp"

/*
THOUGHTS ETC
This unit deals with the flybarless controller simulation and uses a world-referenced axis to act like
the axis of a conventional flybar I have allocated a full Frame of Reference to it though perhaps only
the (Saracen) YAxis of this is really needed. The control to the VBar is as follows:- HLDEcay causes the
VBar Axis to precess towards the RotorHead Shaft frame The Cyclic commands precess the VBar towards the
shaft frame x axis (roll) and -shaftframe Zaxis (elevator) We need to decide what if any caging of the
VBar will be needed to limit the angles between shaft and VBar We may additionally need to limit the VBar
- to mainblade pitch linkage range.

With Swash to main blade mixing about 1:1 we have a situation where, if we stabilise the swashplate in
space then the target tip path plane of the head will also be gyro stabilised in space (lesser ratios
would give the TPP a shaft axis dependence) So perhaps if we use the VBar vector as the direction in
space we intend the swashplate to point then we can achieve the stability we need simply by driving the
servos to maintain the swashplate pointing along the VBar vector Control then becomes a case of
controlling the vbar vector. In a total HL situation with no limits we would precess vbar about the
instantanious transverse and longitudinal axes at rates depending on the elevator and aileron commands
respectively

In 'good' conditions the rotor (followed by the heli) would precess towards the direction dictated by the
vbar and the attitude of the heli in space would be stabilised and controlled.

As with the tail, limits to the HL range must be provided to prevent 'wind-up' and excessive departure of
the vbar direction and the attitude of the heli.

Also, to give a real flybar feel to the heli we must decay the vbar attitude towards the heli's current
attitude just as a real flybar attitude 'decay's towards the shaft attitude (or more accurately, with
control applied, to the flybar control plane)

PID loop
We have need to consider the control loop to deal with servo delays and heli inertia.
*/

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API CFlybarlessUnit
        {
        public:
            CFlybarlessUnit();
            CFlybarlessUnit( const CFlybarlessUnit &vbar ) = delete;

            FrameOfRef  m_frame;
            physics_Vec m_errorVector = physics_Vec::zero();
            physics_Num m_errorMag = static_cast<physics_Num>( 0.0 );
            physics_Num m_rollErrorAngle = static_cast<physics_Num>( 0.0 );
            physics_Num m_pitchErrorAngle = static_cast<physics_Num>( 0.0 );
            physics_Num m_stickDeadBand = static_cast<physics_Num>( 0.0 );
            physics_Num m_stickSensitivity = static_cast<physics_Num>( 0.0 );
            // Multiply by normalized aileron and elevator signal to get desired precession rate in
            // Radians/s
            physics_Num m_stickExpo = static_cast<physics_Num>( 0.0 );
            physics_Num m_rollGain = static_cast<physics_Num>( 0.0 );
            // The roll servo gain i.e. servo normalized deflection per radian of the roll error between
            // shaft and vbar vector
            physics_Num m_pitchGain = static_cast<physics_Num>( 0.0 );
            // The pitch servo gain i.e. servo normalized deflection per radian of the pitch error
            // between shaft and vbar vector
            physics_Num m_angleLimit = static_cast<physics_Num>( 0.0 );
            // Sets a limit to the angle between VBar Yaxis and shaft Yaxis in Radians
            physics_Num m_decay = static_cast<physics_Num>(
                0.0 ); // The time constant for the decay of the VBar-to-Shaft angle
            physics_Num m_stabGain = static_cast<physics_Num>( 0.0 );
            physics_Num m_directMix = static_cast<physics_Num>( 0.0 );
            physics_Num m_ailCommand = static_cast<physics_Num>( 0.0 );
            // The exposed and filtered aileron command value derived from the aileron channel value
            // passed into the vbar unit
            physics_Num m_eleCommand = static_cast<physics_Num>( 0.0 );
            // The exposed and filtered elevator command value derived from the elevator channel value
            // passed into the vbar unit
            physics_Num m_ailDemand = static_cast<physics_Num>(
                0.0 ); // This is the requested aileron rate (roll rate) in Radians/s
            physics_Num m_eleDemand = static_cast<physics_Num>( 0.0 );
            // This is the requested elevator rate (pitching rate) in Radians/s
            physics_Num m_ailFilter = static_cast<physics_Num>( 0.0 );
            // This is the slew time in seconds for the aileron signal to go from 0 to 1
            physics_Num m_eleFilter = static_cast<physics_Num>( 0.0 );
            physics_Num m_bailGain = static_cast<physics_Num>( 0.0 );
            physics_Num m_bailCollective = static_cast<physics_Num>( 0.0 );
            s32         m_bailValue = 0;
            bool        m_stabilize = false;
        }; // CFlybarlessUnit
    } // namespace vehicle
} // namespace workphone

#endif //  Flybarless1H
