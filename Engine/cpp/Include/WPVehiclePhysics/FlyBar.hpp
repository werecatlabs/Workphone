#ifndef FlyBar_h__
#define FlyBar_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "WPVehiclePhysics/FrameOfRef.hpp"
#include "VecMath.hpp"
#include <string>
#include <array>
#include <memory>

namespace workphone
{
    namespace vehicle
    {
        // Note: TFlyBar was created to allow future improvements in flybar
        // dynamics. In the current version the flybar is often treated as a
        // TRotor within the main rotor code but this structure keeps model
        // details isolated for later refinement.
        /**
         * @brief Represents a simplified flybar (stabiliser bar) assembly.
         *
         * TFlyBar contains geometric, inertial and aerodynamic state used by
         * the rotor model. Many members are POD-style for efficiency in the
         * simulator. Indexable arrays use 1-based element conventions (indices
         * 1..4) with index 0 unused to match other rotor data structures.
         */
        class TFlyBar
        {
        public:
            TFlyBar();
            TFlyBar( const TFlyBar &other ) = delete;

            FrameOfRef  m_frame;                         ///< Frame of reference defining flybar attitude
            physics_Vec m_hubFlow = physics_Vec::zero(); ///< Flow at flybar hub centre
            physics_Vec m_hubForce = physics_Vec::zero();       ///< Resultant force (lift+drag) at hub
            physics_Vec m_precessionRate = physics_Vec::zero(); ///< Precession rate (rad/s)
            physics_Vec m_hubPosition = physics_Vec::zero();    ///< Hub position in body frame

            /* Physical rod & paddle geometry */
            physics_Num m_rodDia = static_cast<physics_Num>( 0.0 ); ///< Flybar rod diameter
            physics_Num m_rodLength =
                static_cast<physics_Num>( 0.0 ); ///< Total rod length (including embedded portion)
            physics_Num m_rodDensity =
                static_cast<physics_Num>( 0.0 ); ///< Rod material density (kg/m^3)
            physics_Num m_flyBarSpan = static_cast<physics_Num>( 0.0 ); ///< Outer diameter of the flybar
            physics_Num m_paddleSpan = static_cast<physics_Num>( 0.0 ); ///< Paddle span
            physics_Num m_rootChord = static_cast<physics_Num>( 0.0 );  ///< Paddle chord at root
            physics_Num m_tipChord = static_cast<physics_Num>( 0.0 );   ///< Paddle chord at tip
            physics_Num m_paddleWeight = static_cast<physics_Num>( 0.0 ); ///< Weight of a single paddle
            physics_Num m_radOfGyr =
                static_cast<physics_Num>( 0.0 ); ///< Effective radius of gyration (rod+paddles)

            /* Dynamic state */
            physics_Num m_omega = static_cast<physics_Num>( 0.0 ); ///< Rotation rate (rad/s)
            physics_Num m_ailCyclic =
                static_cast<physics_Num>( 0.0 ); ///< Flybar aileron cyclic pitch (rad)
            physics_Num m_eleCyclic =
                static_cast<physics_Num>( 0.0 ); ///< Flybar elevator cyclic pitch (rad)
            physics_Num m_momOfI = static_cast<physics_Num>( 0.0 ); ///< Moment of inertia about hub
            physics_Num m_angMom = static_cast<physics_Num>( 0.0 ); ///< Angular momentum about hub
            physics_Num m_forceMoments = static_cast<physics_Num>(
                0.0 ); ///< Scalar placeholder for force moments (model-specific)

            /* Angular deflections of flybar relative to the mainshaft
             * Sign convention: eleAngle + = flybar tipped nose down, ailAngle
             * + = flybar rolled right (both relative to body axes). */
            physics_Num m_eleAngle = static_cast<physics_Num>( 0.0 );
            physics_Num m_ailAngle = static_cast<physics_Num>( 0.0 );

            /* Per-sector ground effect and ground distance arrays.
             * Arrays are sized 5 with valid indices 1..4 to match rotor sector
             * conventions. Index 0 is unused. Flybar may not use collective so
             * these are often unused but retained for symmetry with TRotor. */
            std::array<physics_Num, 5> m_gndEffect;
            std::array<physics_Num, 5> m_groundDistance;
        }; // TFlyBar
    } // namespace vehicle
} // namespace workphone
#endif // FlyBar_h__
