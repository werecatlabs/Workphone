#ifndef ExtremeHelicopterController_h__
#define ExtremeHelicopterController_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/CAerodynamicsVehicle.hpp>
#include <Workphone/Interface/Vehicle/IAircraft.hpp>
#include <Workphone/Interface/Vehicle/IAircraftPropellerUnit.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @class ExtremeHelicopterController
         * @brief Blade-element-informed helicopter controller with ground effect and
         *        vortex ring state modelling.
         *
         * Extends the basic rotor RPM and collective/cyclic model with three
         * physically-motivated subsystems:
         *
         * **Blade Lift Slope**
         *   Thrust is computed using a thin-aerofoil lift slope coefficient
         *   (`bladeLiftSlope`, default 5.7 rad�¹) applied directly to the
         *   collective pitch angle, giving a more realistic aerodynamic scaling
         *   than a fixed CL:
         *     CL = bladeLiftSlope × θ_collective
         *     T  = 0.5 · � · A · v_tip² · CL
         *
         * **Ground Effect**
         *   When the aircraft is within `groundEffectHeight` metres of the ground,
         *   a proximity multiplier boosts effective lift. The factor is approximated
         *   from the world-space altitude (AGL), rising as the aircraft descends.
         *
         * **Vortex Ring State (VRS)**
         *   During steep powered descents, recirculation of rotor wake causes a
         *   severe reduction in effective lift. When the downward velocity component
         *   exceeds `vortexDescentRate` while collective is above 30 %, thrust is
         *   reduced to 40 % of its steady-state value.
         *
         * Input channels (standard CAerodynamicsVehicle layout):
         *  - Channel 0 (THR) – throttle     [0, 1]
         *  - Channel 1 (AIL) – roll         [-1, 1]
         *  - Channel 2 (ELE) – pitch        [-1, 1]
         *  - Channel 3 (YAW) – yaw / pedal  [-1, 1]
         *  - Channel 5 (COL) – collective   [-1, 1]
         */
        class WPVehiclePhysics_API ExtremeHelicopterController : public CAerodynamicsVehicle<IAircraft>
        {
        public:
            ExtremeHelicopterController();
            ~ExtremeHelicopterController() override;

            // ------------------------------------------------------------------
            // Lifecycle
            // ------------------------------------------------------------------

            bool isValid() const override;
            void load( SmartPtr<ISharedObject> data ) override;
            void update() override;

            /**
             * @brief Physics step — runs all four subsystems in order.
             * @param t  Absolute simulation time (seconds).
             * @param dt Time-step duration (seconds).
             */
            void update( const double &t, const double &dt );

            // ------------------------------------------------------------------
            // Rotor parameters
            // ------------------------------------------------------------------

            /** @brief Outer rotor tip radius (m). Default: 6.5.
             *  Setting this also recomputes the derived disc area. */
            real_Num getRotorRadius() const;
            void     setRotorRadius( real_Num rotorRadius );

            /** @brief Disc area (m²) — derived from radius, read-only. */
            real_Num getRotorArea() const;

            /** @brief Rotor system moment of inertia (kg·m²). Default: 140. */
            real_Num getRotorInertia() const;
            void     setRotorInertia( real_Num rotorInertia );

            /** @brief Peak engine output torque (N·m). Default: 6000. */
            real_Num getMaxEngineTorque() const;
            void     setMaxEngineTorque( real_Num maxEngineTorque );

            /** @brief Rotor angular drag coefficient (N·m·s). Default: 20. */
            real_Num getRotorDragCoeff() const;
            void     setRotorDragCoeff( real_Num rotorDragCoeff );

            /** @brief Rotor speed ceiling (RPM). Default: 450. */
            real_Num getMaxRotorRPM() const;
            void     setMaxRotorRPM( real_Num maxRotorRPM );

            /** @brief Current main rotor speed (RPM). Read-only. */
            real_Num getRotorRPM() const;

            // ------------------------------------------------------------------
            // Aerodynamic parameters
            // ------------------------------------------------------------------

            /** @brief Ambient air density (kg/m³). Default: 1.225. */
            real_Num getAirDensity() const override;
            void     setAirDensity( real_Num airDensity ) override;

            /**
             * @brief Thin-aerofoil lift slope (per radian). Default: 5.7.
             *
             * Applied as: CL = bladeLiftSlope × collectiveRad
             */
            real_Num getBladeLiftSlope() const;
            void     setBladeLiftSlope( real_Num bladeLiftSlope );

            // ------------------------------------------------------------------
            // Blade pitch parameters
            // ------------------------------------------------------------------

            /** @brief Maximum collective blade pitch (degrees). Default: 18. */
            real_Num getMaxCollectivePitch() const;
            void     setMaxCollectivePitch( real_Num maxCollectivePitch );

            /** @brief Maximum cyclic disc-tilt angle (degrees). Default: 12. */
            real_Num getCyclicTiltMax() const;
            void     setCyclicTiltMax( real_Num cyclicTiltMax );

            // ------------------------------------------------------------------
            // Tail rotor parameters
            // ------------------------------------------------------------------

            /** @brief Peak tail rotor yaw moment (N·m). Default: 3000. */
            real_Num getTailRotorPower() const;
            void     setTailRotorPower( real_Num tailRotorPower );

            // ------------------------------------------------------------------
            // Ground effect parameters
            // ------------------------------------------------------------------

            /**
             * @brief Maximum AGL height at which ground effect is active (m). Default: 10.
             */
            real_Num getGroundEffectHeight() const;
            void     setGroundEffectHeight( real_Num groundEffectHeight );

            // ------------------------------------------------------------------
            // Vortex ring state parameters
            // ------------------------------------------------------------------

            /**
             * @brief Downward velocity threshold that triggers vortex ring state (m/s, negative).
             *        Default: −5. Must be negative (descent).
             */
            real_Num getVortexDescentRate() const;
            void     setVortexDescentRate( real_Num vortexDescentRate );

            // ------------------------------------------------------------------
            // Runtime state queries
            // ------------------------------------------------------------------

            /** @brief Instantaneous induced velocity computed this frame (m/s). Read-only. */
            real_Num getInducedVelocity() const;

            /** @brief True when the vortex ring state condition is active this frame. */
            bool isVortexRingStateActive() const;

            // ------------------------------------------------------------------
            // IAircraft / IAircraftPowerUnit interface
            // ------------------------------------------------------------------

            real_Num getRPM() const;
            ;
            void setRPM( real_Num rpm );

            real_Num getThrottle() const;
            void     setThrottle( real_Num throttle );

            real_Num getMoi() const;
            void     setMoi( real_Num moi );

            real_Num getThrustMultiplier() const;
            void     setThrustMultiplier( real_Num thrustMultiplier );

            real_Num getTorqueMultiplier() const;
            void     setTorqueMultiplier( real_Num torqueMultiplier );

            real_Num getPeakPowerW() const;
            void     setPeakPowerW( real_Num peakPowerW );

            real_Num getTorque( f32 throttlePosition ) const;

            real_Num getMaxTorque( u32 rpm ) const;

            real_Num getMinTorque( u32 rpm ) const;

            real_Num getTorque() const;

            real_Num getEngineRPM( int idx ) const override;
            real_Num getThrust( int idx ) const override;

            SmartPtr<IAircraftCallback> getCallback() const override;
            void                        setCallback( SmartPtr<IAircraftCallback> callback ) override;

            SmartPtr<IBatteryPack> getBatteryPack() const override;
            void                   setBatteryPack( SmartPtr<IBatteryPack> batteryPack ) override;

            SmartPtr<IAerodymanicsWind> getWind() const override;
            void                        setWind( SmartPtr<IAerodymanicsWind> wind ) override;

            void addPropellerUnit( SmartPtr<IAircraftPropellerUnit> propellerUnit ) override;
            void removePropellerUnit( SmartPtr<IAircraftPropellerUnit> propellerUnit ) override;
            Array<SmartPtr<IAircraftPropellerUnit>> getPropellerUnits() const override;
            void                                    setPropellerUnits(
                const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits ) override;

            void                             addWheel( SmartPtr<IWheelComponent> wheel ) override;
            void                             removeWheel( SmartPtr<IWheelComponent> wheel ) override;
            Array<SmartPtr<IWheelComponent>> getWheels() const override;
            void setWheels( const Array<SmartPtr<IWheelComponent>> &wheels ) override;

            void setControlAngle( s32 id, f32 angle ) override;

            real_Num getSectionMultiplier() const override;
            void     setSectionMultiplier( real_Num sectionMultiplier ) override;

            String getModelDataFilePath() const override;
            void   setModelDataFilePath( const String &filePath ) override;

            Transform3<real_Num> getBodyTransform() const override;
            void                 setBodyTransform( Transform3<real_Num> bodyTransform ) override;

            real_Num getRollwiseDamping() const override;
            void     setRollwiseDamping( real_Num rollwiseDamping ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            // ------------------------------------------------------------------
            // Internal simulation steps
            // ------------------------------------------------------------------

            /** Integrate main rotor RPM (engine torque − drag). */
            void updateRotorRPM( real_Num dt );

            /** Compute instantaneous induced velocity from blade-element thrust estimate. */
            void calculateInducedVelocity();

            /**
             * @brief Apply main-rotor thrust, cyclic tilt, and torque reaction.
             * @param up    Body up    axis in world space.
             * @param fwd   Body forward axis in world space.
             * @param right Body right   axis in world space.
             */
            void applyMainRotorForces( const Vector3<real_Num> &up, const Vector3<real_Num> &fwd,
                                       const Vector3<real_Num> &right );

            /**
             * @brief Apply tail rotor anti-torque and pedal yaw moment.
             * @param up Body up axis in world space.
             */
            void applyTailRotor( const Vector3<real_Num> &up );

            // ------------------------------------------------------------------
            // Modifier helpers
            // ------------------------------------------------------------------

            /** Translational lift multiplier (1 → 1.35 over 10–30 m/s). */
            real_Num computeTranslationalLift( real_Num speed ) const;

            /** Ground effect multiplier (> 1.0 within groundEffectHeight AGL). */
            real_Num computeGroundEffect() const;

            /** Vortex ring state multiplier (0.4 when active, else 1.0). */
            real_Num computeVortexRingState( real_Num collectiveIn ) const;

            // ------------------------------------------------------------------
            // Parameters — rotor
            // ------------------------------------------------------------------
            real_Num m_rotorRadius = static_cast<real_Num>( 6.5 );
            real_Num m_rotorArea = static_cast<real_Num>( 0.0 ); // derived
            real_Num m_rotorInertia = static_cast<real_Num>( 140.0 );
            real_Num m_maxEngineTorque = static_cast<real_Num>( 6000.0 );
            real_Num m_rotorDragCoeff = static_cast<real_Num>( 20.0 );
            real_Num m_maxRotorRPM = static_cast<real_Num>( 450.0 );

            // Parameters — aerodynamics
            real_Num m_airDensity = static_cast<real_Num>( 1.225 );
            real_Num m_bladeLiftSlope = static_cast<real_Num>( 5.7 );

            // Parameters — blade pitch
            real_Num m_maxCollectivePitch = static_cast<real_Num>( 18.0 );
            real_Num m_cyclicTiltMax = static_cast<real_Num>( 12.0 );

            // Parameters — tail rotor
            real_Num m_tailRotorPower = static_cast<real_Num>( 3000.0 );

            // Parameters — ground effect
            real_Num m_groundEffectHeight = static_cast<real_Num>( 10.0 );

            // Parameters — vortex ring state
            real_Num m_vortexDescentRate = static_cast<real_Num>( -5.0 );

            // ------------------------------------------------------------------
            // Runtime state
            // ------------------------------------------------------------------
            real_Num m_rotorRPM = static_cast<real_Num>( 0.0 );
            real_Num m_engineTorque = static_cast<real_Num>( 0.0 );
            real_Num m_inducedVelocity = static_cast<real_Num>( 0.0 );
            bool     m_vrsActive = false;

            // ------------------------------------------------------------------
            // IAircraft passthrough members
            // ------------------------------------------------------------------
            real_Num m_moi = static_cast<real_Num>( 1.0 );
            real_Num m_thrustMultiplier = static_cast<real_Num>( 1.0 );
            real_Num m_torqueMultiplier = static_cast<real_Num>( 1.0 );
            real_Num m_peakPowerW = static_cast<real_Num>( 0.0 );
            real_Num m_rollwiseDamping = static_cast<real_Num>( 0.0 );
            real_Num m_sectionMultiplier = static_cast<real_Num>( 1.0 );

            SmartPtr<IAircraftCallback> m_aircraftCallback;
            SmartPtr<IBatteryPack>      m_batteryPack;
            SmartPtr<IAerodymanicsWind> m_wind;
            Transform3<real_Num>        m_bodyTransform;
            String                      m_modelDataFilePath;

            Array<SmartPtr<IAircraftPropellerUnit>> m_propellerUnits;
            Array<SmartPtr<IWheelComponent>>        m_wheels;
        };

    } // namespace vehicle
} // namespace workphone

#endif // ExtremeHelicopterController_h__
