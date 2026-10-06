#ifndef CAircraftPropeller_h__
#define CAircraftPropeller_h__

#include <Workphone/Interface/Vehicle/IAircraftPropeller.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include "WPVehiclePhysics/CAircraftAttachment.hpp"

namespace workphone
{
    namespace vehicle
    {

        class WPVehiclePhysics_API CAircraftPropeller : public CAircraftAttachment<IAircraftPropeller>
        {
        public:
            struct FrameOfRef
            {
                Vector3<real_Num> m_xAxis;
                Vector3<real_Num> m_yAxis;
                Vector3<real_Num> m_zAxis;
            };

            CAircraftPropeller();
            ~CAircraftPropeller() override;

            void load( SmartPtr<ISharedObject> data ) override;

            void update( const double &time, const double &deltaTime ) override;

            void reset() override;

            Vector3<real_Num> getThrust() const override;
            void              setThrust( const Vector3<real_Num> &thrust ) override;

            real_Num getPropwash() const override;

            void setPropwash( real_Num propwash ) override;

            real_Num getThrustValue() const override;
            void     setThrustValue( real_Num thrustValue ) override;

            real_Num getRotationRate() const;
            void     setRotationRate( real_Num rotationRate );

            real_Num getDiameter() const override;
            void     setDiameter( real_Num diameter ) override;

            Vector3<real_Num> getThrustLine() const override;
            void              setThrustLine( const Vector3<real_Num> &thrustLine ) override;

            real_Num propwash() const;
            void     propwash( real_Num propwash );

            Vector3<real_Num> getPosition() const;
            void              setPosition( Vector3<real_Num> position );

            Vector3<real_Num> propThrust() const;
            void              propThrust( Vector3<real_Num> propThrust );

            Vector3<real_Num> angMomVector() const;
            void              angMomVector( Vector3<real_Num> angMomVector );

            Vector3<real_Num> reactionVector() const;
            void              reactionVector( Vector3<real_Num> reactionVector );

            Vector3<real_Num> discFlow() const;
            void              discFlow( Vector3<real_Num> discFlow );

            Vector3<real_Num> wakeRotationVector() const;
            void              wakeRotationVector( Vector3<real_Num> wakeRotationVector );

            FrameOfRef propFrame() const;
            void       propFrame( FrameOfRef propFrame );

            s32  propBlades() const;
            void propBlades( s32 propBlades );

            real_Num iProp() const;
            void     iProp( real_Num iProp );

            int  throttleChannel() const;
            void throttleChannel( int throttleChannel );

            real_Num propDia() const;
            void     propDia( real_Num propDia );

            real_Num cuffChord() const;
            void     cuffChord( real_Num cuffChord );

            real_Num tipChord() const;
            void     tipChord( real_Num tipChord );

            real_Num propChord() const;
            void     propChord( real_Num propChord );

            real_Num propPitch() const;
            void     propPitch( real_Num propPitch );

            real_Num propDensity() const;
            void     propDensity( real_Num propDensity );

            real_Num ppAng() const;
            void     ppAng( real_Num ppAng );

            real_Num propCd0() const;
            void     propCd0( real_Num propCd0 );

            real_Num dCdBydCl3() const;
            void     dCdBydCl3( real_Num dCdBydCl3 );

            real_Num dClByAlpha1() const;
            void     dClByAlpha1( real_Num dClByAlpha );

            real_Num propClMax() const;
            void     propClMax( real_Num propClMax );

            real_Num propClMin() const;
            void     propClMin( real_Num propClMin );

            real_Num propCl() const;
            void     propCl( real_Num propCl );

            real_Num propCd() const;
            void     propCd( real_Num propCd );

            real_Num downThrust() const;
            void     downThrust( real_Num downThrust );

            real_Num sideThrust() const;
            void     sideThrust( real_Num sideThrust );

            real_Num propTorque() const;
            void     propTorque( real_Num propTorque );

            real_Num inputTorque() const;
            void     inputTorque( real_Num inputTorque );

            real_Num reaction() const;
            void     reaction( real_Num reaction );

            real_Num wProp() const;
            void     wProp( real_Num wProp );

            real_Num yawPFac() const;
            void     yawPFac( real_Num yawPFac );

            real_Num pitchPFac() const;
            void     pitchPFac( real_Num pitchPFac );

            real_Num yawDrag() const;
            void     yawDrag( real_Num yawDrag );

            real_Num pitchDrag() const;
            void     pitchDrag( real_Num pitchDrag );

            real_Num propWash() const;
            void     propWash( real_Num propWash );

            real_Num propThrustValue() const;
            void     propThrustValue( real_Num propThrustValue );

            real_Num propArea() const;
            void     propArea( real_Num propArea );

            real_Num bladeArea() const;
            void     bladeArea( real_Num bladeArea );

            real_Num propMoI() const;
            void     propMoI( real_Num propMoI );

            real_Num totMoI() const;
            void     totMoI( real_Num totMoI );

            real_Num angMomentum() const;
            void     angMomentum( real_Num angMomentum );

            real_Num propSolRatio() const;
            void     propSolRatio( real_Num propSolRatio );

            real_Num propDragPowerFactor() const;
            void     propDragPowerFactor( real_Num propDragPowerFactor );

            real_Num massFlow() const;
            void     massFlow( real_Num massFlow );

            real_Num wakeMoI() const;
            void     wakeMoI( real_Num wakeMoI );

            real_Num wakeRotation() const;
            void     wakeRotation( real_Num wakeRotation );

            real_Num propClCoef() const;
            void     propClCoef( real_Num propClCoef );

            real_Num proprThrustKg() const;
            void     proprThrustKg( real_Num proprThrustKg );

            Array<real_Num> propwashFactor() const;
            void            propwashFactor( Array<real_Num> propwashFactor );

            Array<Vector3<real_Num>> pWash() const;
            void                     pWash( Array<Vector3<real_Num>> pWash );

            Array<Vector3<real_Num>> wakeVec() const;
            void                     wakeVec( Array<Vector3<real_Num>> wakeVec );

            Array<real_Num> propwashFactorFus() const;
            void            propwashFactorFus( Array<real_Num> propwashFactorFus );

            Array<Vector3<real_Num>> pWashFus() const;
            void                     pWashFus( Array<Vector3<real_Num>> pWashFus );

            Array<Vector3<real_Num>> wakeVecFus() const;
            void                     wakeVecFus( Array<Vector3<real_Num>> wakeVecFus );

            bool reversed() const;
            void reversed( bool reversed );

            bool ducted() const;
            void ducted( bool ducted );

            bool folding() const;
            void folding( bool folding );

            real_Num getDownThrust() const override;

            void setDownThrust( real_Num downThrust ) override;

            real_Num getSideThrust() const override;

            void setSideThrust( real_Num sideThrust ) override;

        public:
            Vector3<real_Num> m_position; // Position vector WRT datum
            Vector3<real_Num> m_thrustLine;

            // Unit vector in thrust direction
            Vector3<real_Num> m_propThrust;

            // the thrust in vector form
            Vector3<real_Num> m_angMomVector;
            // angular momentum vector accounting for prop rotation direction for use in applying gyro
            // effect
            Vector3<real_Num> m_reactionVector;
            // the torque reaction aligned with the thrust line and with prop rotation direction
            // accounted for
            Vector3<real_Num> m_discFlow; // CFFlow in the prop FoR
            Vector3<real_Num> m_wakeRotationVector;
            // the wake rotation as a vector in the direction of the thrust line with sign as required by
            // the prop rotation direction. with conventional rotation this points forwards

            FrameOfRef m_propFrame; // defines the axis and in-plane directions of the prop.

            s32 m_propBlades = 0; // number of blades

            real_Num m_iProp = static_cast<real_Num>( 0.0 ); // moment of inertia of the prop

            int m_throttleChannel = 0; // throttle channel allocated to the motor/engine driving this
                                       // prop

            real_Num m_propDia = static_cast<real_Num>( 0.0 );   // diameter of the prop (in metres)
            real_Num m_cuffChord = static_cast<real_Num>( 0.0 ); // the chord at the cuff of the blades
            real_Num m_tipChord = static_cast<real_Num>( 0.0 );  // the chord at the tip ofthe blades
            real_Num m_propChord =
                static_cast<real_Num>( 0.0 ); // effective chord of 'working' part of blade
            real_Num m_propPitch =
                static_cast<real_Num>( 0.0 ); // the stated pitch of the prop (in metres)
            real_Num m_propDensity = static_cast<real_Num>( 0.0 ); // the density of the prop material

            // effective pitch angle of the working part of the prop
            real_Num m_geomerticPitch = static_cast<real_Num>( 0.0 );

            real_Num m_propCd0 = static_cast<real_Num>( 0.0 ); // the minimum Cd of the prop profile
            real_Num m_dCdBydCl2 = static_cast<real_Num>( 0.0 );
            // used in PropCd:= PropCd0 + dCdByCl2*Power(PropCl,2) to calc working value for Cd
            real_Num m_dClByAlpha = static_cast<real_Num>( 0.0 ); // the lift slope (typically 2*Pi)
            real_Num m_propClMax = static_cast<real_Num>( 0.0 );  // the max positive Cl for the prop
            real_Num m_propClMin = static_cast<real_Num>( 0.0 );  // the minimum (negative) Cl for the
                                                                  // prop
            real_Num m_propCl =
                static_cast<real_Num>( 0.0 ); // current Cl of the working part of the prop
            real_Num m_propCd = static_cast<real_Num>( 0.0 ); // working Cd at current Cl

            real_Num m_inputTorque = static_cast<real_Num>( 0.0 );
            // The torque received by the prop from the Motor or Engine
            real_Num m_reaction = static_cast<real_Num>( 0.0 );
            // the torque applied to the airframe by the engine/motor and includes component due to
            // accelerating the total MoI of the assembly and applies the reversing as needed
            real_Num m_wProp = static_cast<real_Num>( 0.0 );     // rotation rate in radians/s
            real_Num m_yawPFac = static_cast<real_Num>( 0.0 );   // the Yaw torque due to 'P-factor'
            real_Num m_pitchPFac = static_cast<real_Num>( 0.0 ); // The Pitch torque due to 'P-factor'
            real_Num m_yawDrag = static_cast<real_Num>( 0.0 );
            // drag inbalance in yaw direction due to in-plane flow component
            real_Num m_pitchDrag = static_cast<real_Num>( 0.0 );
            // drag inbalance in pitch direction due to in-plane flow component
            real_Num m_propWash = static_cast<real_Num>( 0.0 );        // propwash speed in m/s
            real_Num m_propThrustValue = static_cast<real_Num>( 0.0 ); // prop thrust in N
            real_Num m_propArea = static_cast<real_Num>( 0.0 );        // area oft he prop disc
            real_Num m_bladeArea =
                static_cast<real_Num>( 0.0 ); // total area of the working parts of the blades
            real_Num m_propMoI = static_cast<real_Num>( 0.0 ); // moment of inertia of the prop
            real_Num m_totMoI = static_cast<real_Num>( 0.0 );
            // the MoI of the prop and either the IC engine or the electric motor as appropriate
            real_Num m_angMomentum = static_cast<real_Num>( 0.0 );
            // the angular momentum of the prop/engine/motor assembly for calculating the gyro effects
            real_Num m_propSolRatio = static_cast<real_Num>( 0.0 ); // Prop solidity ratio
            real_Num m_propDragPowerFactor = static_cast<real_Num>( 0.0 );
            // precalculated factor to get Pd of prop
            real_Num m_massFlow = static_cast<real_Num>( 0.0 );
            // the mass per unit time flowing through the prop disc
            real_Num m_wakeMoI = static_cast<real_Num>( 0.0 );
            // the moment of inertia of the mass flowing through the prop per unit time
            real_Num m_wakeRotation = static_cast<real_Num>( 0.0 );
            // the MAGNITUDE OF angular velocity of the propwash - does not account for reverse rotation
            // props
            real_Num m_propCLCoef = static_cast<real_Num>( 0.0 );
            real_Num m_proprThrustKg = static_cast<real_Num>( 0.0 );

            Array<real_Num> m_propwashFactor;
            // holds the multiplying factors for the propwash at each of the CPs
            Array<Vector3<real_Num>> m_pWash;
            // the vector component of the flow at each CP due to propwash
            Array<Vector3<real_Num>> m_wakeVec;
            // The contributions to the flow at each CP due to the wake rotation
            Array<real_Num> m_propwashFactorFus; // the propwash multiplying factors for the fus cones
            Array<Vector3<real_Num>> m_pWashFus;
            // the vector component of the flow at each fus cone due to propwash
            Array<Vector3<real_Num>> m_wakeVecFus;
            // The contributions to the flow at each fus cone due to the wake rotation

            bool m_reversed = false; // true if prop is non-standard rotation
            bool m_ducted = false;
            // set if the propeller is actually a ducted fan unit (turn off propwash over the airframe)
            bool m_folding = false;
            // sets if the prop is a folding one (eliminates the stopped prop drag if the motor brake
            // slows the prop enough though this drag is currently not in the model)

            real_Num m_propTorque = static_cast<real_Num>( 0.0 );
            // the torque transmitted to the air by prop DOES NOT INCLUDE ACCELERATION OF
            // PROP/ENGINE/MOTOR

            real_Num m_downThrust = static_cast<real_Num>( 0.0 ); // in degrees
            real_Num m_sideThrust = static_cast<real_Num>( 0.0 ); // Right side thrust
        };

        inline Vector3<real_Num> CAircraftPropeller::getThrustLine() const
        {
            return m_thrustLine;
        }

        inline void CAircraftPropeller::setThrustLine( const Vector3<real_Num> &thrustLine )
        {
            m_thrustLine = thrustLine;
        }
    } // namespace vehicle
} // namespace workphone

#endif // CAircraftPropeller_h__
