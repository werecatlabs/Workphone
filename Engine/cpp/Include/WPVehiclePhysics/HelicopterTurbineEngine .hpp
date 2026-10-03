#ifndef HelicopterTurbineEngine_h__
#define HelicopterTurbineEngine_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @class HelicopterTurbineEngine
         * @brief Two-spool turboshaft engine simulation for helicopter applications.
         *
         * Models a turboshaft powerplant with the following subsystems, executed in
         * order each physics step (mirroring the original C# FixedUpdate sequence):
         *
         * **Fuel Control**
         *   A first-order lag filters the raw throttle demand toward the commanded
         *   fuel flow (`fuelResponse`, 1/s). This represents the hydromechanical
         *   fuel control unit (FCU) response time.
         *
         * **Spool Dynamics**
         *   - *N1 (Gas Generator)* – spools toward `fuelFlow × maxN1` at rate
         *     `n1Accel` (%/s). Represents the compressor/combustion turbine shaft.
         *   - *N2 (Power Turbine)* – tracks N1 at rate `n2Accel` (%/s). Drives
         *     the main gearbox through `gearRatio`.
         *
         * **Governor**
         *   A proportional rotor-speed governor compares the actual rotor RPM
         *   (written each frame via `setRotorRPM()`) against `targetRotorRPM` and
         *   trims `fuelFlow` by `governorGain × error × dt` to maintain constant
         *   rotor head speed, as is standard on all turbine helicopters.
         *
         * **Temperature**
         *   Inter-turbine temperature (ITT) is estimated as `fuelFlow × 900 °C`
         *   with a first-order lag. If ITT exceeds `maxTemp`, fuel flow is
         *   automatically reduced by 10 % per step to simulate the over-temperature
         *   protection / power-available limit.
         *
         * **Torque Output**
         *   Output shaft torque is: `outputTorque = (N2 / maxN2) × maxTorque`
         *
         * The component derives from `CAircraftAttachment<IVehicleComponent>` so it
         * can be owned by any `IVehicleBody` and queried for world/local transforms.
         * Call `update(t, dt)` from the owning vehicle controller's physics step.
         */
        class WPVehiclePhysics_API HelicopterTurbineEngine
            : public CAircraftAttachment<IVehicleComponent>
        {
        public:
            HelicopterTurbineEngine();
            ~HelicopterTurbineEngine() override;

            // ------------------------------------------------------------------
            // Lifecycle
            // ------------------------------------------------------------------

            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Advance the engine simulation by one physics step.
             *
             * Runs all five subsystems in order:
             *   1. UpdateFuelSystem
             *   2. UpdateSpoolDynamics
             *   3. UpdateGovernor
             *   4. UpdateTemperature
             *   5. ComputeOutputTorque
             *
             * @param t  Absolute simulation time (seconds).
             * @param dt Time-step duration (seconds).
             */
            void update( const double &t, const double &dt ) override;

            // ------------------------------------------------------------------
            // Throttle input (written by owning controller each frame)
            // ------------------------------------------------------------------

            /**
             * @brief Set normalised throttle demand [0, 1].
             *
             * Equivalent to `Input.GetAxis("Jump")` in the original C# class.
             * Call this once per frame before `update()`.
             */
            void setThrottleInput( real_Num throttle );

            /** @brief Current normalised throttle demand [0, 1]. */
            real_Num getThrottleInput() const;

            // ------------------------------------------------------------------
            // Rotor RPM feedback (governor input)
            // ------------------------------------------------------------------

            /**
             * @brief Provide the actual main rotor RPM to the governor.
             *
             * Must be updated each physics step so the governor can trim fuel
             * flow to maintain `targetRotorRPM`.
             */
            void setRotorRPM( real_Num rpm );

            /** @brief Last rotor RPM value supplied to the governor. */
            real_Num getRotorRPM() const;

            // ------------------------------------------------------------------
            // Outputs (read after update)
            // ------------------------------------------------------------------

            /**
             * @brief Computed output shaft torque from the last physics step (N·m).
             *
             * Formula: `(N2 / maxN2) × maxTorque`
             */
            real_Num getOutputTorque() const;

            /** @brief Current gas generator spool speed (N1, %). */
            real_Num getN1() const;

            /** @brief Current power turbine spool speed (N2, %). */
            real_Num getN2() const;

            /** @brief Current inter-turbine temperature estimate (°C). */
            real_Num getTemperature() const;

            /** @brief Current filtered fuel flow [0, 1]. */
            real_Num getFuelFlow() const;

            // ------------------------------------------------------------------
            // Spool parameters
            // ------------------------------------------------------------------

            /** @brief Maximum gas generator spool speed (%). Default: 100. */
            real_Num getMaxN1() const;
            void     setMaxN1( real_Num maxN1 );

            /** @brief Maximum power turbine spool speed (%). Default: 100. */
            real_Num getMaxN2() const;
            void     setMaxN2( real_Num maxN2 );

            /** @brief N1 spool-up/down rate (%/s). Default: 15. */
            real_Num getN1Accel() const;
            void     setN1Accel( real_Num n1Accel );

            /** @brief N2 spool-up/down rate (%/s). Default: 8. */
            real_Num getN2Accel() const;
            void     setN2Accel( real_Num n2Accel );

            /**
             * @brief Fuel control lag coefficient (1/s). Default: 2.
             *
             * Higher values give faster fuel flow response.
             */
            real_Num getFuelResponse() const;
            void     setFuelResponse( real_Num fuelResponse );

            // ------------------------------------------------------------------
            // Governor parameters
            // ------------------------------------------------------------------

            /** @brief Target main rotor head speed (RPM). Default: 400. */
            real_Num getTargetRotorRPM() const;
            void     setTargetRotorRPM( real_Num targetRotorRPM );

            /**
             * @brief Governor proportional gain. Default: 0.015.
             *
             * Fuel correction per RPM error per second: `correction = error × gain`.
             */
            real_Num getGovernorGain() const;
            void     setGovernorGain( real_Num governorGain );

            // ------------------------------------------------------------------
            // Transmission parameters
            // ------------------------------------------------------------------

            /** @brief N2-to-rotor gear ratio. Default: 25. */
            real_Num getGearRatio() const;
            void     setGearRatio( real_Num gearRatio );

            /** @brief Peak available output shaft torque (N·m). Default: 7000. */
            real_Num getMaxTorque() const;
            void     setMaxTorque( real_Num maxTorque );

            // ------------------------------------------------------------------
            // Engine limits
            // ------------------------------------------------------------------

            /**
             * @brief Maximum permissible ITT (°C). Default: 850.
             *
             * When temperature exceeds this threshold the over-temperature
             * protection trims fuel flow by 10 % per physics step.
             */
            real_Num getMaxTemp() const;
            void     setMaxTemp( real_Num maxTemp );

            WP_CLASS_REGISTER_DECL;

        private:
            // ------------------------------------------------------------------
            // Internal simulation steps
            // ------------------------------------------------------------------

            /** First-order lag fuel flow toward throttle demand. */
            void updateFuelSystem( real_Num dt );

            /** MoveTowards spool dynamics for N1 and N2. */
            void updateSpoolDynamics( real_Num dt );

            /** Proportional governor trims fuel flow to maintain target rotor RPM. */
            void updateGovernor( real_Num dt );

            /** Estimate ITT; apply over-temperature fuel cut if limit exceeded. */
            void updateTemperature( real_Num dt );

            /** Derive output torque from N2 fraction. */
            void computeOutputTorque();

            // ------------------------------------------------------------------
            // Parameters — spool
            // ------------------------------------------------------------------
            real_Num m_maxN1 = static_cast<real_Num>( 100.0 );
            real_Num m_maxN2 = static_cast<real_Num>( 100.0 );
            real_Num m_n1Accel = static_cast<real_Num>( 15.0 );
            real_Num m_n2Accel = static_cast<real_Num>( 8.0 );
            real_Num m_fuelResponse = static_cast<real_Num>( 2.0 );

            // Parameters — governor
            real_Num m_targetRotorRPM = static_cast<real_Num>( 400.0 );
            real_Num m_governorGain = static_cast<real_Num>( 0.015 );

            // Parameters — transmission
            real_Num m_gearRatio = static_cast<real_Num>( 25.0 );
            real_Num m_maxTorque = static_cast<real_Num>( 7000.0 );

            // Parameters — limits
            real_Num m_maxTemp = static_cast<real_Num>( 850.0 );

            // ------------------------------------------------------------------
            // Runtime state
            // ------------------------------------------------------------------
            real_Num m_throttleInput = static_cast<real_Num>( 0.0 );
            real_Num m_rotorRPM = static_cast<real_Num>( 0.0 );
            real_Num m_fuelFlow = static_cast<real_Num>( 0.0 );
            real_Num m_n1 = static_cast<real_Num>( 0.0 );
            real_Num m_n2 = static_cast<real_Num>( 0.0 );
            real_Num m_temperature = static_cast<real_Num>( 0.0 );
            real_Num m_outputTorque = static_cast<real_Num>( 0.0 );
        };

    } // namespace vehicle
} // namespace workphone

#endif // HelicopterTurbineEngine_h__
