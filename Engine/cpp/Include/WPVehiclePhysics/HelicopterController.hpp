#ifndef HelicopterController_h__
#define HelicopterController_h__

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
         * @class HelicopterController
         * @brief Straightforward arcade-style helicopter controller.
         *
         * Models a simplified helicopter with four subsystems:
         *
         * **Lift**
         *   Engine throttle is spooled through a first-order lag (`spoolUpSpeed`)
         *   and multiplied by `liftPower` to produce an upward force along the
         *   body-up axis. A ground effect multiplier boosts lift when the aircraft
         *   is within `groundEffectHeight` metres AGL.
         *
         * **Tilt**
         *   Pitch and roll inputs apply torques around the body right and forward
         *   axes respectively, tilting the helicopter and producing translational
         *   motion through the lift vector.
         *
         * **Yaw**
         *   A direct torque around the body-up axis scales with `yawTorque`.
         *
         * **Stabilization**
         *   A restoring torque proportional to the cross product of the body-up
         *   and world-up axes keeps the aircraft level when inputs are released.
         *
         * Input channels (standard CAerodynamicsVehicle layout):
         *  - Channel 0 (THR) – collective / throttle  [0, 1]
         *  - Channel 1 (AIL) – roll                  [-1, 1]
         *  - Channel 2 (ELE) – pitch                 [-1, 1]
         *  - Channel 3 (YAW) – yaw                   [-1, 1]
         */
        class WPVehiclePhysics_API HelicopterController : public CAerodynamicsVehicle<IAircraft>
        {
        public:
            HelicopterController();
            ~HelicopterController() override;

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
            // Engine parameters
            // ------------------------------------------------------------------

            /** @brief Peak engine output force (N). Default: 30 000. */
            real_Num getEnginePower() const;
            void     setEnginePower( real_Num enginePower );

            /**
             * @brief Throttle spool-up rate (1/s). Default: 0.5.
             *
             * Controls how quickly the engine throttle follows the collective
             * input. Higher values give faster engine response.
             */
            real_Num getSpoolUpSpeed() const;
            void     setSpoolUpSpeed( real_Num spoolUpSpeed );

            /** @brief Current engine throttle [0, 1]. Read-only. */
            real_Num getEngineThrottle() const;

            // ------------------------------------------------------------------
            // Lift parameters
            // ------------------------------------------------------------------

            /** @brief Lift force scalar (N). Default: 4 000. */
            real_Num getLiftPower() const;
            void     setLiftPower( real_Num liftPower );

            /**
             * @brief Maximum AGL height at which ground effect is active (m). Default: 8.
             */
            real_Num getGroundEffectHeight() const;
            void     setGroundEffectHeight( real_Num groundEffectHeight );

            // ------------------------------------------------------------------
            // Tilt parameters
            // ------------------------------------------------------------------

            /** @brief Pitch torque magnitude (N·m). Default: 800. */
            real_Num getPitchForce() const;
            void     setPitchForce( real_Num pitchForce );

            /** @brief Roll torque magnitude (N·m). Default: 800. */
            real_Num getRollForce() const;
            void     setRollForce( real_Num rollForce );

            // ------------------------------------------------------------------
            // Yaw parameters
            // ------------------------------------------------------------------

            /** @brief Yaw torque magnitude (N·m). Default: 600. */
            real_Num getYawTorque() const;
            void     setYawTorque( real_Num yawTorque );

            // ------------------------------------------------------------------
            // Stability parameters
            // ------------------------------------------------------------------

            /**
             * @brief Self-levelling torque multiplier. Default: 3.
             *
             * Scales the cross-product restoring torque that returns the
             * helicopter to a level attitude when inputs are neutral.
             */
            real_Num getStabilization() const;
            void     setStabilization( real_Num stabilization );

            // ------------------------------------------------------------------
            // IAircraft / IAircraftPowerUnit interface
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

            real_Num getMaxRotorRPM() const;
            real_Num getAirDensity() const override;
            void     setAirDensity( real_Num airDensity ) override;

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

            /** Apply spooled throttle lift force along body-up, modified by ground effect. */
            void applyLift( const Vector3<real_Num> &up );

            /** Apply pitch and roll torques from cyclic inputs. */
            void applyTilt( const Vector3<real_Num> &fwd, const Vector3<real_Num> &right );

            /** Apply yaw torque from pedal input. */
            void applyYaw( const Vector3<real_Num> &up );

            /** Apply self-levelling cross-product torque. */
            void applyStabilization( const Vector3<real_Num> &up );

            /** Ground effect multiplier from AGL altitude proxy. */
            real_Num computeGroundEffect() const;

            // ------------------------------------------------------------------
            // Parameters — engine
            // ------------------------------------------------------------------
            real_Num m_enginePower = static_cast<real_Num>( 30000.0 );
            real_Num m_spoolUpSpeed = static_cast<real_Num>( 0.5 );

            // Parameters — lift
            real_Num m_liftPower = static_cast<real_Num>( 4000.0 );
            real_Num m_groundEffectHeight = static_cast<real_Num>( 8.0 );

            // Parameters — tilt
            real_Num m_pitchForce = static_cast<real_Num>( 800.0 );
            real_Num m_rollForce = static_cast<real_Num>( 800.0 );

            // Parameters — yaw
            real_Num m_yawTorque = static_cast<real_Num>( 600.0 );

            // Parameters — stability
            real_Num m_stabilization = static_cast<real_Num>( 3.0 );

            // ------------------------------------------------------------------
            // Runtime state
            // ------------------------------------------------------------------
            real_Num m_engineThrottle = static_cast<real_Num>( 0.0 );

            // ------------------------------------------------------------------
            // IAircraft passthrough members
            // ------------------------------------------------------------------
            real_Num m_airDensity = static_cast<real_Num>( 1.225 );
            real_Num m_maxRotorRPM = static_cast<real_Num>( 450.0 );
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

#endif // HelicopterController_h__
