#ifndef DCSHelicopterController_h__
#define DCSHelicopterController_h__

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
         * @class DCSHelicopterController
         * @brief High-fidelity helicopter flight controller modelled after DCS-style physics.
         *
         * Extends the basic rotor RPM and collective/cyclic model with four additional
         * subsystems that significantly improve physical realism:
         *
         * **Dynamic Inflow**
         *   Induced velocity is not computed instantaneously from thrust; instead it
         *   converges toward the momentum-theory target with a first-order lag
         *   (`inflowLag`). This reproduces the real rotor's inability to instantly
         *   respond to collective changes and produces the characteristic "settling
         *   with power" transient.
         *
         * **Blade Flapping**
         *   Forward flight causes asymmetric dynamic pressure across the advancing
         *   and retreating halves of the disc. The resulting aerodynamic moment is
         *   balanced by blade flapping, which tilts the rotor disc backward and to
         *   the side. This is modelled as a low-pass filtered disc-tilt offset driven
         *   by the local lateral velocity components (`flappingStiffness`).
         *
         * **Retreating Blade Stall**
         *   At high forward airspeeds the retreating blade approaches stall. The model
         *   linearly attenuates the available thrust from 1→0.5 between 60 and 100 m/s.
         *
         * **Stability Augmentation System (SAS)**
         *   Applies a damping torque proportional to the current angular velocity to
         *   suppress oscillations (`angularDamping`).
         *
         * Thrust is computed from momentum theory:
         *   T = 2 · � · A · vi · (vi − vy)
         * where `vi` is the lagged induced velocity and `vy` is the vertical body speed.
         *
         * Input channels (standard CAerodynamicsVehicle layout):
         *  - Channel 0 (THR) – throttle     [0, 1]
         *  - Channel 1 (AIL) – roll         [-1, 1]
         *  - Channel 2 (ELE) – pitch        [-1, 1]
         *  - Channel 3 (YAW) – yaw / pedal  [-1, 1]
         *  - Channel 5 (COL) – collective   [-1, 1]
         */
        class WPVehiclePhysics_API DCSHelicopterController : public CAerodynamicsVehicle<IAircraft>
        {
        public:
            DCSHelicopterController();
            ~DCSHelicopterController() override;

            // ------------------------------------------------------------------
            // Lifecycle
            // ------------------------------------------------------------------

            bool isValid() const override;
            void load( SmartPtr<ISharedObject> data ) override;
            void update() override;

            /**
             * @brief Physics step — runs all six subsystems in order.
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

            /** @brief Rotor system moment of inertia (kg·m²). Default: 150. */
            real_Num getRotorInertia() const;
            void     setRotorInertia( real_Num rotorInertia );

            /** @brief Peak engine output torque (N·m). Default: 6500. */
            real_Num getMaxEngineTorque() const;
            void     setMaxEngineTorque( real_Num maxEngineTorque );

            /** @brief Rotor angular drag coefficient (N·m·s). Default: 25. */
            real_Num getRotorDrag() const;
            void     setRotorDrag( real_Num rotorDrag );

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

            // ------------------------------------------------------------------
            // Blade pitch parameters
            // ------------------------------------------------------------------

            /** @brief Maximum collective blade pitch (degrees). Default: 18. */
            real_Num getMaxCollectivePitch() const;
            void     setMaxCollectivePitch( real_Num maxCollectivePitch );

            /** @brief Maximum cyclic disc-tilt angle (degrees). Default: 12. */
            real_Num getCyclicMaxTilt() const;
            void     setCyclicMaxTilt( real_Num cyclicMaxTilt );

            // ------------------------------------------------------------------
            // Dynamic inflow parameters
            // ------------------------------------------------------------------

            /**
             * @brief Dynamic inflow lag coefficient (1/s). Default: 2.5.
             *
             * Controls the rate at which the induced velocity converges toward the
             * momentum-theory target. Higher values give faster response (less lag).
             */
            real_Num getInflowLag() const;
            void     setInflowLag( real_Num inflowLag );

            /** @brief Current lagged induced velocity (m/s). Read-only. */
            real_Num getInducedVelocity() const;

            // ------------------------------------------------------------------
            // Blade flapping parameters
            // ------------------------------------------------------------------

            /**
             * @brief Blade flapping convergence rate (1/s). Default: 6.
             *
             * Controls how quickly the flapping offset follows the aerodynamically
             * driven equilibrium. Higher values give stiffer, faster-responding flapping.
             */
            real_Num getFlappingStiffness() const;
            void     setFlappingStiffness( real_Num flappingStiffness );

            /** @brief Current rotor disc flapping offset vector. Read-only. */
            Vector3<real_Num> getFlappingOffset() const;

            // ------------------------------------------------------------------
            // Tail rotor parameters
            // ------------------------------------------------------------------

            /** @brief Peak tail rotor yaw moment (N·m). Default: 3500. */
            real_Num getTailRotorPower() const;
            void     setTailRotorPower( real_Num tailRotorPower );

            // ------------------------------------------------------------------
            // SAS parameters
            // ------------------------------------------------------------------

            /**
             * @brief Angular velocity damping factor (N·m·s/rad). Default: 2.5.
             *
             * Applied as: torque = −angularDamping × ω, suppressing unwanted
             * oscillations across all axes.
             */
            real_Num getAngularDamping() const;
            void     setAngularDamping( real_Num angularDamping );

            // ------------------------------------------------------------------
            // Retreating blade stall query
            // ------------------------------------------------------------------

            /**
             * @brief Retreating blade stall multiplier for the current frame.
             *
             * Returns a value in [0.5, 1.0]: 1.0 below 60 m/s forward speed,
             * linearly decreasing to 0.5 at 100 m/s.
             */
            real_Num getRetreatingBladeStallFactor() const;

            // ------------------------------------------------------------------
            // IAircraft / IAircraftPowerUnit interface stubs
            // ------------------------------------------------------------------

            real_Num getRPM() const;
            void     setRPM( real_Num rpm );

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
            // Internal simulation steps (called in order from update())
            // ------------------------------------------------------------------

            /** Integrate main rotor RPM (engine torque − drag). */
            void updateRotorRPM( real_Num dt );

            /**
             * @brief Update the lagged induced velocity toward the momentum-theory target.
             * @param dt Time-step (seconds).
             */
            void updateDynamicInflow( real_Num dt );

            /**
             * @brief Update the blade flapping offset from local lateral velocity.
             * @param dt      Time-step (seconds).
             * @param worldFwd Body forward axis in world space.
             * @param worldRight Body right axis in world space.
             */
            void updateBladeFlapping( real_Num dt, const Vector3<real_Num> &worldFwd,
                                      const Vector3<real_Num> &worldRight );

            /**
             * @brief Apply main-rotor thrust, cyclic tilt, flapping, and torque reaction.
             * @param up    Body up    axis in world space.
             * @param fwd   Body forward axis in world space.
             * @param right Body right   axis in world space.
             */
            void applyRotorForces( const Vector3<real_Num> &up, const Vector3<real_Num> &fwd,
                                   const Vector3<real_Num> &right );

            /**
             * @brief Apply tail rotor anti-torque and pedal yaw moment.
             * @param up Body up axis in world space.
             */
            void applyTailRotor( const Vector3<real_Num> &up );

            /**
             * @brief Apply SAS angular velocity damping torque.
             */
            void applySAS();

            /** Translational lift multiplier (1→1.4 over 10–35 m/s). */
            real_Num computeTranslationalLift( real_Num speed ) const;

            /** Retreating blade stall multiplier (1→0.5 over 60–100 m/s). */
            real_Num computeRetreatingBladeStall( real_Num forwardSpeed ) const;

            // ------------------------------------------------------------------
            // Parameters — rotor
            // ------------------------------------------------------------------
            real_Num m_rotorRadius = static_cast<real_Num>( 6.5 );
            real_Num m_rotorArea = static_cast<real_Num>( 0.0 ); // derived
            real_Num m_rotorInertia = static_cast<real_Num>( 150.0 );
            real_Num m_maxEngineTorque = static_cast<real_Num>( 6500.0 );
            real_Num m_rotorDrag = static_cast<real_Num>( 25.0 );
            real_Num m_maxRotorRPM = static_cast<real_Num>( 450.0 );

            // Parameters — aerodynamics
            real_Num m_airDensity = static_cast<real_Num>( 1.225 );

            // Parameters — blade pitch
            real_Num m_maxCollectivePitch = static_cast<real_Num>( 18.0 );
            real_Num m_cyclicMaxTilt = static_cast<real_Num>( 12.0 );

            // Parameters — dynamic inflow
            real_Num m_inflowLag = static_cast<real_Num>( 2.5 );

            // Parameters — blade flapping
            real_Num m_flappingStiffness = static_cast<real_Num>( 6.0 );

            // Parameters — tail rotor
            real_Num m_tailRotorPower = static_cast<real_Num>( 3500.0 );

            // Parameters — SAS
            real_Num m_angularDamping = static_cast<real_Num>( 2.5 );

            // ------------------------------------------------------------------
            // Runtime state
            // ------------------------------------------------------------------
            real_Num          m_rotorRPM = static_cast<real_Num>( 0.0 );
            real_Num          m_engineTorque = static_cast<real_Num>( 0.0 );
            real_Num          m_inducedVelocity = static_cast<real_Num>( 0.0 );
            Vector3<real_Num> m_flappingOffset = Vector3<real_Num>::zero();
            real_Num          m_retreatingFactor = static_cast<real_Num>( 1.0 );

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

#endif // DCSHelicopterController_h__
