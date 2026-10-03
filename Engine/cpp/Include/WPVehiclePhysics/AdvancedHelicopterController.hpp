#ifndef AdvancedHelicopterController_h__
#define AdvancedHelicopterController_h__

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
         * @class AdvancedHelicopterController
         * @brief Physically-based helicopter flight controller.
         *
         * Simulates the principal aerodynamic subsystems of a single-main-rotor
         * helicopter:
         *  - Main rotor RPM integration (engine torque, rotor drag, inertia)
         *  - Blade-pitch lift with collective and cyclic disc-tilt
         *  - Ground effect (IGE) boost
         *  - Effective Translational Lift (ETL / Translational Lift)
         *  - Tail rotor anti-torque and yaw authority
         *  - Torque reaction from the main rotor
         *
         * Input is read from the standard 8-channel array inherited from
         * CAerodynamicsVehicle:
         *  - Channel 0 (THR) – throttle  [0, 1]
         *  - Channel 1 (AIL) – roll      [-1, 1]
         *  - Channel 2 (ELE) – pitch     [-1, 1]
         *  - Channel 3 (YAW) – yaw       [-1, 1]
         *  - Channel 5 (COL) – collective[-1, 1]
         *
         * Forces and torques are accumulated via the CAerodynamicsVehicle helpers
         * and dispatched to the physics body each frame.
         */
        class WPVehiclePhysics_API AdvancedHelicopterController : public CAerodynamicsVehicle<IAircraft>
        {
        public:
            AdvancedHelicopterController();
            ~AdvancedHelicopterController() override;

            // ---------------------------------------------------------------
            // ISharedObject / IVehicle lifecycle
            // ---------------------------------------------------------------

            bool isValid() const override;

            void load( SmartPtr<ISharedObject> data ) override;

            void update() override;

            /**
             * @brief Physics step — integrates rotor RPM and applies all forces.
             * @param t  Absolute simulation time (seconds).
             * @param dt Time-step duration (seconds).
             */
            void update( const double &t, const double &dt );

            // ---------------------------------------------------------------
            // Rotor physics parameters
            // ---------------------------------------------------------------

            /** @brief Rotational inertia of the main rotor system (kg·m²). */
            real_Num getRotorInertia() const;
            void     setRotorInertia( real_Num rotorInertia );

            /** @brief Peak torque the engine can deliver to the rotor (N·m). */
            real_Num getMaxEngineTorque() const;
            void     setMaxEngineTorque( real_Num maxEngineTorque );

            /** @brief Rotor angular drag coefficient (N·m·s/rad). */
            real_Num getRotorDrag() const;
            void     setRotorDrag( real_Num rotorDrag );

            /** @brief Rotor speed ceiling (RPM). */
            real_Num getMaxRotorRPM() const;
            void     setMaxRotorRPM( real_Num maxRotorRPM );

            /** @brief Current main rotor speed (RPM). Read-only query. */
            real_Num getRotorRPM() const;

            // ---------------------------------------------------------------
            // Lift parameters
            // ---------------------------------------------------------------

            /** @brief Effective rotor disc area (m²). */
            real_Num getRotorArea() const;
            void     setRotorArea( real_Num rotorArea );

            /** @brief Ambient air density (kg/m³). Defaults to ISA sea-level 1.225. */
            real_Num getAirDensity() const override;
            void     setAirDensity( real_Num airDensity ) override;

            /** @brief Non-dimensional rotor lift coefficient. */
            real_Num getLiftCoefficient() const;
            void     setLiftCoefficient( real_Num liftCoefficient );

            // ---------------------------------------------------------------
            // Blade pitch parameters
            // ---------------------------------------------------------------

            /** @brief Maximum collective blade pitch (degrees). */
            real_Num getMaxCollectivePitch() const;
            void     setMaxCollectivePitch( real_Num maxCollectivePitch );

            /** @brief Maximum cyclic disc-tilt angle from stick deflection (degrees). */
            real_Num getCyclicTiltAngle() const;
            void     setCyclicTiltAngle( real_Num cyclicTiltAngle );

            // ---------------------------------------------------------------
            // Tail rotor
            // ---------------------------------------------------------------

            /** @brief Peak side-force the tail rotor can produce (N). */
            real_Num getTailRotorForce() const;
            void     setTailRotorForce( real_Num tailRotorForce );

            // ---------------------------------------------------------------
            // Ground effect
            // ---------------------------------------------------------------

            /**
             * @brief AGL height below which ground-effect lift boost applies (m).
             *
             * The IGE boost is computed from the vehicle body's world-space Y
             * position when no raycast provider is available.
             */
            real_Num getGroundEffectHeight() const;
            void     setGroundEffectHeight( real_Num groundEffectHeight );

            // ---------------------------------------------------------------
            // Effective Translational Lift (ETL)
            // ---------------------------------------------------------------

            /** @brief Airspeed at which ETL begins (m/s). */
            real_Num getETLStartSpeed() const;
            void     setETLStartSpeed( real_Num etlStartSpeed );

            /** @brief Peak ETL lift multiplier (>= 1). */
            real_Num getETLMaxBoost() const;
            void     setETLMaxBoost( real_Num etlMaxBoost );

            // ---------------------------------------------------------------
            // IAircraft stubs (unused subsystems return safe defaults)
            // ---------------------------------------------------------------

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
            // ---------------------------------------------------------------
            // Internal simulation helpers
            // ---------------------------------------------------------------

            /** Integrate rotor RPM for this time-step. */
            void updateRotorPhysics( real_Num dt );

            /**
             * Compute and accumulate main-rotor lift and cyclic forces.
             * @param up    Body up direction in world space.
             * @param fwd   Body forward direction in world space.
             * @param right Body right direction in world space.
             */
            void applyLift( const Vector3<real_Num> &up, const Vector3<real_Num> &fwd,
                            const Vector3<real_Num> &right );

            /**
             * Compute and accumulate tail rotor anti-torque and yaw control.
             * @param up Body up direction in world space.
             */
            void applyTailRotor( const Vector3<real_Num> &up );

            /**
             * Ground-in-effect lift multiplier.
             * Approximated from the body world-space Y position.
             * @return Multiplier >= 1.
             */
            real_Num computeGroundEffect() const;

            /**
             * Effective Translational Lift multiplier.
             * @param speed Current world-space airspeed magnitude (m/s).
             * @return Multiplier in [1, ETLMaxBoost].
             */
            real_Num computeTranslationalLift( real_Num speed ) const;

            // ---------------------------------------------------------------
            // Parameters (rotor physics)
            // ---------------------------------------------------------------
            real_Num m_rotorInertia = static_cast<real_Num>( 120.0 );
            real_Num m_maxEngineTorque = static_cast<real_Num>( 5000.0 );
            real_Num m_rotorDrag = static_cast<real_Num>( 15.0 );
            real_Num m_maxRotorRPM = static_cast<real_Num>( 450.0 );

            // Parameters (lift)
            real_Num m_rotorArea = static_cast<real_Num>( 120.0 );
            real_Num m_airDensity = static_cast<real_Num>( 1.225 );
            real_Num m_liftCoefficient = static_cast<real_Num>( 0.6 );

            // Parameters (blade pitch)
            real_Num m_maxCollectivePitch = static_cast<real_Num>( 15.0 );
            real_Num m_cyclicTiltAngle = static_cast<real_Num>( 10.0 );

            // Parameters (tail rotor)
            real_Num m_tailRotorForce = static_cast<real_Num>( 2000.0 );

            // Parameters (ground effect)
            real_Num m_groundEffectHeight = static_cast<real_Num>( 10.0 );

            // Parameters (ETL)
            real_Num m_etlStartSpeed = static_cast<real_Num>( 8.0 );
            real_Num m_etlMaxBoost = static_cast<real_Num>( 1.35 );

            // ---------------------------------------------------------------
            // Runtime state
            // ---------------------------------------------------------------
            real_Num m_rotorRPM = static_cast<real_Num>( 0.0 );
            real_Num m_engineTorque = static_cast<real_Num>( 0.0 );

            // Passthrough IAircraft members (not used by this controller)
            real_Num m_moi = static_cast<real_Num>( 1.0 );
            real_Num m_thrustMultiplier = static_cast<real_Num>( 1.0 );
            real_Num m_torqueMultiplier = static_cast<real_Num>( 1.0 );
            real_Num m_peakPowerW = static_cast<real_Num>( 0.0 );
            real_Num m_rollwiseDamping = static_cast<real_Num>( 0.0 );

            SmartPtr<IAircraftCallback> m_aircraftCallback;
            SmartPtr<IBatteryPack>      m_batteryPack;
            SmartPtr<IAerodymanicsWind> m_wind;
            Transform3<real_Num>        m_bodyTransform;

            Array<SmartPtr<IAircraftPropellerUnit>> m_propellerUnits;
            Array<SmartPtr<IWheelComponent>>        m_wheels;

            real_Num m_sectionMultiplier = static_cast<real_Num>( 1.0 );
            String   m_modelDataFilePath;
        };

    } // namespace vehicle
} // namespace workphone

#endif // AdvancedHelicopterController_h__
