#ifndef CAircraftEngine_h__
#define CAircraftEngine_h__

#include <Workphone/Interface/Vehicle/IAircraftPowerUnit.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @file CAircraftEngine.hpp
         *
         * @brief Represents an aircraft engine / power unit attached to an aircraft.
         *
         * @details
         * CAircraftEngine implements IAircraftPowerUnit via CAircraftAttachment and
         * encapsulates behavior and state for an engine: propeller reference,
         * RPM, torque, throttle, peak and maximum power values, multipliers and
         * inertia. This class is intended to be used by higher-level aircraft
         * simulation code to query engine outputs (torque, thrust) and to update
         * engine state over time.
         */
        class WPVehiclePhysics_API CAircraftEngine : public CAircraftAttachment<IAircraftPowerUnit>
        {
        public:
            /**
             * @brief Constructs a CAircraftEngine with default parameters.
             */
            CAircraftEngine();

            /**
             * @brief Virtual destructor.
             */
            ~CAircraftEngine() override;

            /**
             * @brief Load engine configuration from a shared object.
             *
             * @param data Smart pointer to an ISharedObject containing engine data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Load engine configuration from raw data pointer.
             *
             * @param pData Pointer to engine configuration data (format determined by caller).
             */
            void load( void *pData ) override;

            /**
             * @brief Update engine internal state for the current simulation step.
             *
             * @param t  Current simulation time (seconds).
             * @param dt Time step since last update (seconds).
             */
            void update( const double &t, const double &dt ) override;

            /**
             * @brief Get raw pointer to the attached propeller (non-owning).
             *
             * @return Pointer to IAircraftPropeller or nullptr if none attached.
             */
            IAircraftPropeller *getPropellerPtr() const override;

            /**
             * @brief Get smart pointer to the attached propeller.
             *
             * @return SmartPtr to IAircraftPropeller; may be empty.
             */
            SmartPtr<IAircraftPropeller> getPropeller() const override;

            /**
             * @brief Attach a propeller to this engine.
             *
             * @param propeller SmartPtr to the propeller to attach.
             */
            void setPropeller( SmartPtr<IAircraftPropeller> propeller ) override;

            /**
             * @brief Get the current engine speed in revolutions per minute (RPM).
             *
             * @return Current RPM as real_Num.
             */
            real_Num getRPM() const override;

            /**
             * @brief Set the current engine RPM.
             *
             * @param rpm Engine speed in RPM.
             */
            void setRPM( real_Num rpm ) override;

            /**
             * @brief Query whether this power unit is an electric motor.
             *
             * @return true if electric, false if internal-combustion.
             */
            bool isElectric() const override;

            /**
             * @brief Set whether this power unit is electric.
             *
             * @param electric true for electric motor, false for non-electric.
             */
            void setElectric( bool electric ) override;

            /**
             * @brief Check whether the engine configuration/state is valid.
             *
             * @return true if engine parameters are valid for simulation.
             */
            bool isValid() const override;

            /**
             * @brief Query whether the engine is currently running.
             *
             * @return true if running, false if stopped.
             */
            bool isRunning() const;

            /**
             * @brief Set the running state of the engine.
             *
             * @param running true to mark engine running, false to stop it.
             */
            void setRunning( bool running );

            /**
             * @brief Get the configured maximum power output of the engine.
             *
             * @return Maximum power in the same units as used by the engine (typically watts).
             */
            real_Num getMaxPower() const;

            /**
             * @brief Set the configured maximum power output.
             *
             * @param maxPower Maximum engine power (units consistent with getPeakPowerW()).
             */
            void setMaxPower( real_Num maxPower );

            /**
             * @brief Get the maximum allowable RPM for the engine.
             *
             * @return Maximum RPM as real_Num.
             */
            real_Num getMaxRPM() const override;

            /**
             * @brief Set the current torque value on the engine (instantaneous).
             *
             * @param torque Torque value (units consistent with rest of simulation).
             */
            void setTorque( real_Num torque );

            /**
             * @brief Get the current engine torque.
             *
             * @return Engine torque as real_Num.
             */
            real_Num getTorque() const override;

            /**
             * @brief Compute torque at a given throttle position.
             *
             * @param throttlePosition Normalized throttle [0.0, 1.0].
             * @return Torque (f32) produced at the specified throttle position.
             */
            f32 getTorque( f32 throttlePosition ) const override;

            /**
             * @brief Get the maximum torque available at the specified RPM index.
             *
             * @param rpm RPM value (integer representation used by lookup).
             * @return Maximum torque (f32) at the requested RPM.
             */
            f32 getMaxTorque( u32 rpm ) const override;

            /**
             * @brief Get the minimum torque available at the specified RPM index.
             *
             * @param rpm RPM value (integer representation used by lookup).
             * @return Minimum torque (f32) at the requested RPM.
             */
            f32 getMinTorque( u32 rpm ) const override;

            /**
             * @brief Get the current throttle position.
             *
             * @return Throttle value as real_Num in range [0,1] (convention used by caller).
             */
            real_Num getThrottle() const override;

            /**
             * @brief Set the current throttle position.
             *
             * @param throttle Normalized throttle value (typically 0.0 to 1.0).
             */
            void setThrottle( real_Num throttle ) override;

            /**
             * @brief Get engine revs at which peak power occurs.
             *
             * @return Revs (RPM) where peak power is expected.
             */
            real_Num getPeakPowerRevs() const;

            /**
             * @brief Set the engine revs corresponding to peak power.
             *
             * @param peak_power_revs RPM value at peak power.
             */
            void setPeakPowerRevs( real_Num peak_power_revs );

            /**
             * @brief Get peak power of the engine in watts (W).
             *
             * @return Peak power in watts.
             */
            real_Num getPeakPowerW() const override;

            /**
             * @brief Set peak power in watts (W).
             *
             * @param peak_power_w Peak power expressed in watts.
             */
            void setPeakPowerW( real_Num peak_power_w ) override;

            /**
             * @brief Get the current calculated engine output power.
             *
             * @return Engine power (units as used by simulation, typically watts).
             */
            real_Num getEnginePower() const;

            /**
             * @brief Set the current calculated engine output power.
             *
             * @param engine_power Engine output power.
             */
            void setEnginePower( real_Num engine_power );

            /**
             * @brief Get the thrust multiplier applied to generated thrust.
             *
             * @return Multiplier scalar for thrust.
             */
            real_Num getThrustMultiplier() const override;

            /**
             * @brief Set the thrust multiplier applied to generated thrust.
             *
             * @param thrustMultiplier Multiplier scalar; 1.0 means no change.
             */
            void setThrustMultiplier( real_Num thrustMultiplier ) override;

            /**
             * @brief Get the torque multiplier applied to produced torque.
             *
             * @return Multiplier scalar for torque.
             */
            real_Num getTorqueMultiplier() const override;

            /**
             * @brief Set the torque multiplier applied to produced torque.
             *
             * @param torqueMultiplier Multiplier scalar; 1.0 means no change.
             */
            void setTorqueMultiplier( real_Num torqueMultiplier ) override;

            /**
             * @brief Get the moment of inertia (MOI) of the engine/rotating assembly.
             *
             * @return Moment of inertia as real_Num.
             */
            real_Num getMoi() const override;

            /**
             * @brief Set the moment of inertia (MOI) of the engine/rotating assembly.
             *
             * @param moi Moment of inertia.
             */
            void setMoi( real_Num moi ) override;

        private:
            SmartPtr<IAircraftPropeller> m_propeller; ///< Attached propeller (smart pointer).

            real_Num m_thrustMultiplier =
                static_cast<real_Num>( 1.0 ); ///< Multiplier applied to computed thrust.
            real_Num m_torqueMultiplier =
                static_cast<real_Num>( 1.0 ); ///< Multiplier applied to produced torque.

            real_Num m_maxPower = static_cast<real_Num>( 0.0 ); ///< Configured maximum power.
            real_Num m_throttle = static_cast<real_Num>( 0.0 ); ///< Current throttle position [0..1].

            real_Num m_maxRPM = static_cast<real_Num>( 0.0 );        ///< Maximum allowed RPM.
            real_Num m_peakPowerRevs = static_cast<real_Num>( 0.0 ); ///< RPM where peak power occurs.
            real_Num m_peakPowerW = static_cast<real_Num>( 0.0 );    ///< Peak power in watts.
            real_Num m_enginePower = static_cast<real_Num>( 0.0 );   ///< Current engine power.
            real_Num m_engineTorque = static_cast<real_Num>( 0.0 );  ///< Current engine torque.
            real_Num m_moi = static_cast<real_Num>( 0.0 ); ///< Moment of inertia of rotating parts.

            real_Num m_rpm = static_cast<real_Num>( 0.0 ); ///< Current RPM value.

            bool m_running = true; ///< Whether engine is currently running.
        };
    } // namespace vehicle
} // namespace workphone

#endif // CAircraftEngine_h__
