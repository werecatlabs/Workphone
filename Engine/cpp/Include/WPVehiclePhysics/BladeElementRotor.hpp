#ifndef BladeElementRotor_h__
#define BladeElementRotor_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @class BladeElementRotor
         * @brief Blade Element Theory (BET) rotor aerodynamics component.
         *
         * Computes the net aerodynamic force and torque produced by a multi-blade
         * rotor using the blade element method. The rotor disc is divided radially
         * into a configurable number of annular segments per blade. For each segment
         * the local airspeed (tangential rotor velocity + body velocity at that
         * point) is resolved, a section lift and drag are evaluated from the local
         * angle of attack, and the results are summed into a net force and torque
         * applied to the parent rigid body.
         *
         * Cyclic pitch variation is modelled through a first-harmonic approximation:
         *   pitch(ψ) = collective + cyclicX·sin(ψ) + cyclicY·cos(ψ)
         * where ψ is the azimuth angle of the blade.
         *
         * The component plugs into the engine via @c CAircraftAttachment so it can
         * be owned by any @c IVehicleBody and queried for its world/local transforms.
         *
         * @par Coordinate conventions
         *  - The rotor hub is at the local origin of this attachment.
         *  - Blades extend along the local X axis; the rotor plane is X–Z (Y is up).
         *  - Positive rotor angular velocity corresponds to CCW rotation when viewed
         *    from above (standard main-rotor convention).
         *
         * @par Thread-safety
         *  Not thread-safe; call @c update() from a single physics thread.
         */
        class WPVehiclePhysics_API BladeElementRotor : public CAircraftAttachment<IVehicleComponent>
        {
        public:
            BladeElementRotor();
            ~BladeElementRotor() override;

            // ------------------------------------------------------------------
            // Lifecycle
            // ------------------------------------------------------------------

            /**
             * @brief Load configuration from a shared properties object.
             * @param data SmartPtr to a properties/config object. May be null.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Advance the rotor simulation by one physics step.
             * @param t  Absolute simulation time (seconds).
             * @param dt Time-step duration (seconds).
             *
             * Iterates over all blade segments, computes per-segment lift and drag,
             * accumulates the results, and applies the net force and torque to the
             * parent @c IVehicleBody.
             */
            void update( const double &t, const double &dt ) override;

            // ------------------------------------------------------------------
            // Outputs (read after update)
            // ------------------------------------------------------------------

            /**
             * @brief Net aerodynamic force produced during the last @c update() call.
             * @return World-space force vector (N).
             */
            Vector3<real_Num> getTotalForce() const;

            /**
             * @brief Net aerodynamic torque produced during the last @c update() call.
             * @return World-space torque vector (N·m).
             */
            Vector3<real_Num> getTotalTorque() const;

            // ------------------------------------------------------------------
            // Rotor geometry
            // ------------------------------------------------------------------

            /** @brief Number of rotor blades. Default: 4. */
            s32  getBladeCount() const;
            void setBladeCount( s32 bladeCount );

            /**
             * @brief Number of radial integration segments per blade. Default: 12.
             *
             * Higher values improve accuracy at the cost of compute time.
             */
            s32  getSegmentsPerBlade() const;
            void setSegmentsPerBlade( s32 segmentsPerBlade );

            /** @brief Tip radius of the rotor disc (m). Default: 6.5. */
            real_Num getRotorRadius() const;
            void     setRotorRadius( real_Num rotorRadius );

            /** @brief Blade chord length (m). Assumed constant along the span. Default: 0.35. */
            real_Num getChordLength() const;
            void     setChordLength( real_Num chordLength );

            // ------------------------------------------------------------------
            // Aerodynamic coefficients
            // ------------------------------------------------------------------

            /** @brief Ambient air density (kg/m³). Default: 1.225 (ISA sea-level). */
            real_Num getAirDensity() const;
            void     setAirDensity( real_Num airDensity );

            /**
             * @brief Lift-curve slope (dCL/dα, per radian). Default: 5.7.
             *
             * Thin-aerofoil theory gives 2π (~6.28); 5.7 is a common empirical
             * value for NACA symmetric sections.
             */
            real_Num getLiftSlope() const;
            void     setLiftSlope( real_Num liftSlope );

            /**
             * @brief Zero-lift profile drag coefficient (CD0). Default: 0.01.
             *
             * Total section drag is modelled as CD = CD0 + CL²·k, where k is a
             * small induced-drag factor (0.02 by default, matching the C# source).
             */
            real_Num getDragCoeff() const;
            void     setDragCoeff( real_Num dragCoeff );

            // ------------------------------------------------------------------
            // Rotor state inputs
            // ------------------------------------------------------------------

            /**
             * @brief Main rotor speed (RPM).
             *
             * Set each frame by the owning controller before calling @c update().
             */
            real_Num getRotorRPM() const;
            void     setRotorRPM( real_Num rotorRPM );

            /**
             * @brief Collective blade pitch (degrees).
             *
             * Applied uniformly to every blade segment. Positive collective
             * produces upward thrust for a CCW-rotating rotor viewed from above.
             */
            real_Num getCollectivePitch() const;
            void     setCollectivePitch( real_Num collectivePitch );

            /**
             * @brief Cyclic pitch inputs (dimensionless, typically −1 … +1).
             *
             * x component: lateral cyclic (roll control)
             * y component: longitudinal cyclic (pitch control)
             *
             * Internally the maximum physical cyclic deflection is scaled by
             * @c getCyclicPitchMax().
             */
            Vector2<real_Num> getCyclicInput() const;
            void              setCyclicInput( const Vector2<real_Num> &cyclicInput );

            /**
             * @brief Peak cyclic pitch authority (degrees). Default: 10.
             *
             * The raw cyclic input is multiplied by this angle to obtain the
             * actual per-blade pitch modulation amplitude.
             */
            real_Num getCyclicPitchMax() const;
            void     setCyclicPitchMax( real_Num cyclicPitchMax );

            WP_CLASS_REGISTER_DECL;

        private:
            // ------------------------------------------------------------------
            // Internal BET simulation
            // ------------------------------------------------------------------

            /**
             * @brief Core BET integration — called by @c update().
             * @param omega   Rotor angular velocity (rad/s), positive = CCW from above.
             * @param worldUp Body up direction in world space.
             * @param worldRight Body right direction in world space.
             */
            void simulateRotor( real_Num omega, const Vector3<real_Num> &worldUp,
                                const Vector3<real_Num> &worldRight );

            // ------------------------------------------------------------------
            // Parameters — geometry
            // ------------------------------------------------------------------
            s32      m_bladeCount = 4;
            s32      m_segmentsPerBlade = 12;
            real_Num m_rotorRadius = static_cast<real_Num>( 6.5 );
            real_Num m_chordLength = static_cast<real_Num>( 0.35 );

            // Parameters — aerodynamics
            real_Num m_airDensity = static_cast<real_Num>( 1.225 );
            real_Num m_liftSlope = static_cast<real_Num>( 5.7 );
            real_Num m_dragCoeff = static_cast<real_Num>( 0.01 );

            // State inputs (set by owning controller each frame)
            real_Num          m_rotorRPM = static_cast<real_Num>( 0.0 );
            real_Num          m_collectivePitch = static_cast<real_Num>( 0.0 );
            Vector2<real_Num> m_cyclicInput =
                Vector2<real_Num>( static_cast<real_Num>( 0.0 ), static_cast<real_Num>( 0.0 ) );
            real_Num m_cyclicPitchMax = static_cast<real_Num>( 10.0 );

            // Derived / cached
            real_Num m_segmentLength = static_cast<real_Num>( 0.0 );

            // Outputs
            Vector3<real_Num> m_totalForce = Vector3<real_Num>::zero();
            Vector3<real_Num> m_totalTorque = Vector3<real_Num>::zero();
        };

    } // namespace vehicle
} // namespace workphone

#endif // BladeElementRotor_h__
