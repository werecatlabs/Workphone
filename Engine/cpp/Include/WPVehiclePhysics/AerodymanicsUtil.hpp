#ifndef AerodymanicsUtil_h__
#define AerodymanicsUtil_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>
#include <WPVehiclePhysics/EngineClutchUnit.hpp>
#include "WPVehiclePhysics/CBatteryPack.hpp"
#include <WPVehiclePhysics/HeliVars.hpp>
#include "WPVehiclePhysics/VecMath.hpp"
#include <Workphone/Interface/Vehicle/IAerodymanicsWind.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @class AerodymanicsUtil
         * @brief Utility class providing static methods for aerodynamics and helicopter physics
         * calculations.
         *
         * This comprehensive utility class contains methods for simulating various helicopter
         * subsystems:
         * - Aerodynamic drag forces on aircraft bodies
         * - Gyroscope systems (heading hold, rate mode)
         * - Flybarless units (VBar) for stabilization
         * - Engine and centrifugal clutch dynamics
         * - Governor systems for RPM control
         * - Electric motor and ESC (Electronic Speed Controller) simulation
         * - LiPo battery pack discharge characteristics
         *
         * All methods are static, making this a stateless utility class suitable for
         * functional-style physics calculations.
         *
         * @note This class is used primarily in helicopter flight simulation but can be
         *       adapted for other rotorcraft applications.
         */
        class WPVehiclePhysics_API AerodymanicsUtil
        {
        public:
            /**
             * @name Unit Conversion Constants
             * @brief Conversion factors between RPM and angular velocity (rad/s).
             * @note Conversion formulas:
             *       - RPM to ?: ? = RPM × ?/30
             *       - ? to RPM: RPM = ? × 30/?
             * @{
             */
            // const float RPMToOmega = Math<physics_Num>::pi() / 30.0f;
            // const float OmegaToRPM = 30 / Math<physics_Num>::pi();
            /** @} */

            /**
             * @name Clutch State Constants
             * @brief State machine values for centrifugal clutch engagement.
             * @{
             */

            /** @brief Clutch state: fully disengaged, no torque transmission */
            static const int m_kDisengaged = 0;

            /** @brief Clutch state: slipping between engaged and disengaged, partial torque transmission
             * with friction losses */
            static const int m_kSlipping = 1;

            /** @brief Clutch state: fully locked/engaged, rigid torque transmission */
            static const int m_kLocked = 2;

            /** @brief Clutch state: overrun condition where output speed exceeds engine speed */
            static const int m_kOverrun = 3;

            /** @} */

            /**
             * @name Body Drag Variables (Legacy)
             * @brief Global variables for body drag calculations (commented out, kept for reference).
             * @note These were previously used for:
             *       - BodyFlowInGF: Body airflow (including wind) in ground frame of reference
             *       - BodyCdA: Drag coefficient × area (CdA) vector for three orthogonal flow directions
             *       - BodyDrag: Resulting body drag force vector
             * @{
             */
            // extern Vec BodyFlowInGF;
            // extern Vec BodyCdA;
            // extern Vec BodyDrag;
            /** @} */

            /**
             * @brief Calculates aerodynamic drag force on the aircraft body.
             *
             * Computes drag force using the simplified formula: F_drag = 0.5 × ? × v² × CdA
             * where the airflow velocity and drag coefficients are provided per axis.
             *
             * @param SaracenFlow Airflow velocity vector [m/s] in the Saracen airframe reference frame
             * (body-fixed). Includes both aircraft motion and environmental wind contribution.
             * @param BodyCdA Drag coefficient-area product vector [m²] for three orthogonal flow
             * directions (X, Y, Z). Represents the effective drag area in each body axis direction.
             * @return The resulting body drag force vector [N] in the same reference frame as
             * SaracenFlow.
             *
             * @note Air density is typically assumed or embedded in the CdA values.
             * @note Force direction opposes the airflow direction.
             */
            static physics_Vec bodyForce( const physics_Vec &SaracenFlow, const physics_Vec &BodyCdA );

            /**
             * @name Gyroscope Control Methods
             * @brief Methods for simulating RC helicopter tail gyroscopes.
             *
             * These methods implement both heading-hold and rate modes for tail rotor control.
             * Gyroscopes measure yaw rate and provide stabilization by correcting for unwanted rotation.
             * @{
             */

            /**
             * @brief Adjusts a gyroscope parameter by identifier.
             *
             * @param ParamID The parameter identifier to adjust (system-specific constant).
             * @param ParamVal The new value for the parameter.
             *
             * @note Parameter IDs and ranges are defined in the gyro configuration system.
             */
            static void adjustGyroParam( int ParamID, float ParamVal );

            /**
             * @brief Resets gyroscope to default values.
             *
             * Clears accumulated errors, integrator states, and resets all internal
             * variables to their initial conditions.
             *
             * @param Gyro The gyroscope unit to reset.
             *
             * @note Called when switching modes or reinitializing the system.
             */
            static void resetGyro( CGyroUnit &Gyro );

            /**
             * @brief Handles stop control activity for the gyroscope.
             *
             * Implements decay and damping of gyro response when nearing the target heading
             * or when in transition states.
             *
             * @param TG The gyroscope unit to control.
             * @param dt Delta time step in seconds.
             *
             * @note This prevents oscillation and overshoot in heading-hold mode.
             */
            static void stopControl( CGyroUnit &TG, physics_Num dt );

            /**
             * @brief Calculates the acceleration compensation for the gyroscope.
             *
             * Computes rate of change of error signal to provide derivative control
             * action, improving response time and damping.
             *
             * @param TG The gyroscope unit to calculate acceleration for.
             */
            static void calcAcceleration( CGyroUnit &TG );

            /**
             * @brief Calculates the stop gain for the gyroscope.
             *
             * Determines the proportional gain used in stop control based on
             * current error magnitude and gyro mode.
             *
             * @param TG The gyroscope unit to calculate stop gain for.
             *
             * @note Gain scheduling improves stability across different operating conditions.
             */
            static void calcStopGain( CGyroUnit &TG );

            /**
             * @brief Processes gyroscope input from Tx channel and outputs to servo.
             *
             * Main gyro processing function that:
             * 1. Reads transmitter input signal
             * 2. Applies gain/sensitivity
             * 3. Compares with rate sensor feedback
             * 4. Computes corrective output
             * 5. Updates internal state
             *
             * @param InSig Input signal from transmitter channel (typically -1.0 to +1.0).
             * @param GainSig Gain signal for gyroscope sensitivity (0.0 to 1.0, where higher = more
             * aggressive).
             * @param dt Delta time step in seconds.
             * @param ThisGyro The gyroscope unit to process (contains output in its state).
             *
             * @note Output servo position is stored in ThisGyro.Output after processing.
             */
            static void gyroIn( physics_Num InSig, physics_Num GainSig, physics_Num dt,
                                CGyroUnit &ThisGyro );

            /**
             * @brief Gets the tail gyroscope output based on input signals and yaw rate.
             *
             * Convenience function that processes gyro inputs and returns the servo output
             * signal directly without requiring explicit state management.
             *
             * @param InSig Input signal from transmitter channel (typically -1.0 to +1.0).
             * @param GainSig Gain signal for gyroscope sensitivity (0.0 to 1.0).
             * @param dt Delta time step in seconds.
             * @param YawRate Current yaw rate of the helicopter in radians/second (positive = nose
             * right).
             * @return The computed tail gyroscope output signal for the tail rotor servo (-1.0 to +1.0).
             *
             * @note This is a stateless wrapper around gyroIn() using a static gyro instance.
             */
            static physics_Num getTailGyro( physics_Num InSig, physics_Num GainSig, physics_Num dt,
                                            physics_Num YawRate );

            /**
             * @brief Initializes the tail gyroscope system.
             *
             * Sets up default parameters, allocates resources, and prepares the gyro
             * for first use. Must be called before getTailGyro().
             */
            static void initTailGyro();

            /** @} */

            /**
             * @name Flybarless (VBar) Control Methods
             * @brief Methods for simulating flybarless stabilization systems.
             *
             * Flybarless units (like the VBar) replace mechanical flybars with electronic
             * stabilization. They use rate sensors and accelerometers to provide precise
             * cyclic control and self-leveling capabilities.
             * @{
             */

            /**
             * @brief Initializes a flybarless (VBar) unit.
             *
             * Loads configuration parameters from XML, initializes sensor fusion,
             * and sets up control gains for stabilization modes.
             *
             * @param VB The flybarless unit to initialize.
             *
             * @note Parameter values (gains, limits, response curves) are loaded from
             *       XML file with other helicopter parameters.
             * @see HeliVars.hpp for parameter structure
             */
            static void initVBar( CFlybarlessUnit &VB );

            /**
             * @brief Applies limits and decay to the VBar system.
             *
             * Enforces physical and safety limits on the virtual flybar vector,
             * and applies exponential decay to prevent runaway integration.
             *
             * @param dt Delta time step in seconds.
             * @param VB The flybarless unit to process.
             *
             * @note Decay prevents wind-up of the integrator in sustained maneuvers.
             * @note Limits prevent excessive control deflection.
             */
            static void limitAndDecayVBar( physics_Num dt, CFlybarlessUnit &VB );

            /**
             * @brief Applies precession of the VBar vector based on control signals and orientation.
             *
             * Simulates the virtual flybar's angular momentum and gyroscopic precession.
             * Control inputs cause the virtual flybar to tilt, which then generates
             * cyclic control commands.
             *
             * @param dt Delta time step in seconds.
             * @param AilInSig Aileron input signal from transmitter (-1.0 to +1.0, positive = right
             * roll).
             * @param EleInSig Elevator input signal from transmitter (-1.0 to +1.0, positive = nose
             * down).
             * @param VB The flybarless unit to control.
             *
             * @note Phase angle and precession rate are configured per helicopter model.
             * @note This creates the characteristic "feel" of the flybarless system.
             */
            static void controlVBar( physics_Num dt, physics_Num AilInSig, physics_Num EleInSig,
                                     CFlybarlessUnit &VB );

            /**
             * @brief Gets the VBar output signals for aileron and elevator servos.
             *
             * Reads the current virtual flybar state and converts it to servo
             * position commands with appropriate scaling and mixing.
             *
             * @param[out] AilOut Aileron output signal (-1.0 to +1.0).
             * @param[out] EleOut Elevator output signal (-1.0 to +1.0).
             * @param VB The flybarless unit to read from.
             *
             * @note Output includes stabilization corrections and pilot inputs.
             */
            static void getVBarOutputs( physics_Num &AilOut, physics_Num &EleOut, CFlybarlessUnit &VB );

            /**
             * @brief Main VBar processing loop combining control and output.
             *
             * Convenience function that executes the complete VBar cycle:
             * 1. Process control inputs
             * 2. Update virtual flybar state
             * 3. Apply limits and decay
             * 4. Generate servo outputs
             *
             * @param dt Delta time step in seconds.
             * @param AilInSig Aileron input signal from transmitter (-1.0 to +1.0).
             * @param EleInSig Elevator input signal from transmitter (-1.0 to +1.0).
             * @param[out] AilOut Aileron output signal to servos (-1.0 to +1.0).
             * @param[out] EleOut Elevator output signal to servos (-1.0 to +1.0).
             * @param VB The flybarless unit to process.
             *
             * @note This is the typical entry point for VBar simulation in the main loop.
             */
            static void vBarLoop( physics_Num dt, physics_Num AilInSig, physics_Num EleInSig,
                                  physics_Num &AilOut, physics_Num &EleOut, CFlybarlessUnit &VB );

            /** @} */

            /**
             * @name Parameter Parsing
             * @{
             */

            /**
             * @brief Searches for and extracts a parameter value from a configuration string.
             *
             * Parses a string for a named parameter in the format "ParamName=Value"
             * and extracts the value if found.
             *
             * @param[out] Param The parameter structure to populate with name and value.
             * @param PS The configuration string to search for the parameter value.
             * @return True if the parameter was found and successfully parsed, false otherwise.
             *
             * @note Used for parsing XML attributes and configuration file entries.
             */
            static bool findValueOf( VehicleParam &Param, const String &PS );

            /** @} */

            /**
             * @name Engine and Clutch System Methods
             * @brief Methods for simulating nitro/glow fuel engines with centrifugal clutches.
             *
             * These methods model:
             * - Engine power curves based on throttle and RPM
             * - Centrifugal clutch engagement and slipping
             * - Power transmission from engine to main rotor
             * - Engine inertia and load dynamics
             * @{
             */

            /**
             * @brief Calculates clutch torque based on current engine RPM.
             *
             * Models a centrifugal clutch where shoes expand outward with RPM,
             * engaging when centrifugal force overcomes spring tension.
             *
             * Formula: Torque = K × (RPM² - RPM_engage²) for RPM > RPM_engage
             *          Torque = 0 for RPM ? RPM_engage
             *
             * where K depends on clutch dimensions, number of shoes, friction coefficient,
             * and spring characteristics.
             *
             * @param RPM Current engine RPM (revolutions per minute).
             * @param EC The engine clutch unit parameters containing engagement RPM and coefficients.
             * @return The clutch torque value in Newton-meters [N?m].
             *
             * @note Positive torque accelerates the output (main rotor).
             * @note Torque is zero when disengaged, rises quadratically with RPM when slipping.
             */
            static float clutchTorque( float RPM, const EngineClutchUnit &EC );

            /**
             * @brief Looks up engine power based on throttle position and updates the unit state.
             *
             * Uses a 2D lookup table (throttle × RPM) to determine engine power output,
             * accounting for engine characteristics like torque curve and power band.
             *
             * @param Thr Throttle position (0.0 = idle, 1.0 = full throttle).
             * @param EC The engine clutch unit to update with interpolated power value [W or hp].
             *
             * @note Power lookup table is loaded from engine data files via readPowerLookups().
             * @note Updated power is stored in EC.Power.
             */
            static void lookupEnginePower( float Thr, EngineClutchUnit &EC );

            /**
             * @brief Reads power lookup table data for the engine from file.
             *
             * Loads the 2D power map (throttle vs RPM) that characterizes the
             * engine's performance across its operating range.
             *
             * @param EC The engine clutch unit to populate with lookup table data.
             *
             * @note Typically called once during initialization.
             * @see getLookups()
             */
            static void readPowerLookups( EngineClutchUnit &EC );

            /**
             * @brief Loads all lookup tables for the engine system.
             *
             * Master function that loads power curves, efficiency maps, and other
             * tabulated engine data required for simulation.
             *
             * @note Called during engine system initialization.
             */
            static void getLookups();

            /**
             * @brief Reads engine parameters from data file.
             *
             * Loads engine specifications including:
             * - Displacement and cylinder count
             * - Max power and RPM range
             * - Clutch engagement parameters
             * - Inertia values
             *
             * @param EC The engine clutch unit to populate with configuration data.
             */
            static void readEngineDataFile( EngineClutchUnit &EC );

            /**
             * @brief Retrieves engine data from storage.
             *
             * Convenience function that loads all engine parameters and lookup tables
             * from the configured data source.
             */
            static void getEngineData();

            /**
             * @brief Starts the engine system.
             *
             * Initializes engine state for running (typically sets RPM to idle,
             * resets accumulators, and prepares for throttle input).
             */
            static void startEngine();

            /**
             * @brief Calculates engine power output for given throttle setting.
             *
             * Simplified power calculation based on current throttle position,
             * using the configured power curve.
             *
             * @param Thr Throttle position (0.0 to 1.0).
             * @return The engine power output in watts [W].
             *
             * @note This is a stateless version using default engine parameters.
             */
            static float enginePower( float Thr );

            /**
             * @brief Main simulation step for engine and clutch system.
             *
             * Performs one time step of the complete engine-clutch-load dynamics:
             * 1. Calculate engine power from throttle
             * 2. Determine clutch engagement state
             * 3. Compute torques on engine and output shafts
             * 4. Integrate angular accelerations to update RPM values
             * 5. Handle state transitions (disengaged/slipping/locked)
             *
             * @param[in,out] OPRPM Output RPM (load side, e.g., main rotor) [rev/min].
             * @param[in,out] ERPM Engine RPM (crankshaft speed) [rev/min].
             * @param LoadInertia Rotational moment of inertia of the load (rotor + gearbox) [kg?m²].
             * @param LoadTorque Torque applied by the load (aerodynamic + friction) [N?m], positive
             * opposes rotation.
             * @param dt Delta time step in seconds [s].
             * @param Throttle Current throttle position (0.0 to 1.0).
             * @param EC The engine clutch unit containing all system parameters and state.
             *
             * @note Uses semi-implicit Euler integration for numerical stability.
             * @note Clutch state machine handles engagement, slipping, and overrun conditions.
             */
            static void mainEngineClutchStep( physics_Num &OPRPM, physics_Num &ERPM,
                                              physics_Num LoadInertia, physics_Num LoadTorque,
                                              physics_Num dt, physics_Num Throttle,
                                              EngineClutchUnit &EC );

            /** @} */

            /**
             * @name Governor System Methods
             * @brief Methods for simulating RPM governor systems.
             *
             * Governors maintain constant rotor RPM by automatically adjusting throttle
             * or pitch in response to load changes. Uses PID control to minimize RPM error.
             * @{
             */

            /**
             * @brief Resets governor parameters and clears transient state.
             *
             * Clears PID integrator, error history, and other variables not
             * persisted in XML configuration. Prepares governor for fresh engagement.
             *
             * @param Gov The governor unit to reset.
             *
             * @note Called when switching governor modes or re-engaging after manual flight.
             */
            static void resetGovernor( CGovernorUnit &Gov );

            /**
             * @brief Executes governor control loop for one time step.
             *
             * Implements a PID controller that:
             * 1. Measures RPM error (target - actual)
             * 2. Computes proportional, integral, and derivative terms
             * 3. Generates throttle correction
             * 4. Applies limits and anti-windup
             *
             * @param InSig Input signal (baseline throttle or collective pitch) [-1.0 to +1.0].
             * @param SpeedSig Speed signal from RPM sensor or magnetic pickup [0.0 to 1.0, normalized].
             * @param dt Delta time step in seconds.
             * @param RPM Current RPM reading from sensor [rev/min].
             * @param Gov The governor unit containing target RPM, gains, and state.
             *
             * @note Output is stored in Gov.Output and typically feeds to throttle or pitch.
             * @note Gain scheduling may adjust PID coefficients based on RPM range.
             */
            static void doGovernor( physics_Num InSig, physics_Num SpeedSig, physics_Num dt,
                                    physics_Num RPM, CGovernorUnit &Gov );

            /**
             * @brief Initializes the governor system.
             *
             * Loads governor parameters from configuration, allocates state,
             * and prepares for first engagement.
             *
             * @note Must be called before using getGovernor().
             */
            static void initGovernor() /* export */;

            /**
             * @brief Sets governor target RPM directly (legacy method).
             *
             * @param RPM Target RPM value to maintain [rev/min].
             * @param Gash Governor gain/sensitivity adjustment multiplier.
             *
             * @deprecated This method is deprecated. Governor parameters should be
             *             configured via XML configuration files for better maintainability.
             * @note Retained for backward compatibility with older configurations.
             */
            static void setGovernorRPM( float RPM, float Gash );

            /**
             * @brief Gets governor output for given inputs (stateless interface).
             *
             * Convenience function that processes governor logic and returns the
             * corrected throttle signal using a static governor instance.
             *
             * @param InSig Input signal (baseline throttle or collective pitch) [-1.0 to +1.0].
             * @param SpeedSig Speed signal from RPM sensor [0.0 to 1.0, normalized].
             * @param dt Delta time step in seconds.
             * @param RPM Current RPM reading [rev/min].
             * @return The governor output signal (corrected throttle) [-1.0 to +1.0].
             *
             * @note Uses internal static governor state, not suitable for multiple helicopters.
             */
            static physics_Num getGovernor( physics_Num InSig, physics_Num SpeedSig, physics_Num dt,
                                            physics_Num RPM ) /* export */;

            /** @} */

            /**
             * @name Electric Motor and ESC System Methods
             * @brief Methods for simulating brushless electric motors, ESCs, and LiPo batteries.
             *
             * These methods model:
             * - Brushless DC motor electrical and mechanical dynamics
             * - ESC (Electronic Speed Controller) with governor and soft-start
             * - LiPo battery discharge characteristics and voltage sag
             * - Low-voltage cutoff protection
             * @{
             */

            /**
             * @brief Controls ESC cutout based on battery pack state.
             *
             * Monitors battery voltage and implements low-voltage cutoff (LVC)
             * to prevent over-discharge damage to LiPo cells. Typically cuts
             * power or reduces throttle when cell voltage drops below ~3.0-3.3V.
             *
             * @param dt Delta time step in seconds.
             * @param Pack The battery pack to monitor for voltage levels.
             * @param ESC The ESC to control (cutout state updated).
             *
             * @note May implement soft cutoff (gradual power reduction) or hard cutoff (immediate stop).
             * @warning Proper LVC is critical to prevent permanent battery damage.
             */
            static void escCutoutControl( physics_Num dt, const CBatteryPack &Pack, CESController &ESC );

            /**
             * @brief Initializes electric motor and ESC systems.
             *
             * Loads configuration parameters including:
             * - Motor KV rating and internal resistance
             * - ESC timing, braking, and governor settings
             * - Soft-start ramp rates
             *
             * @param Motor The electric motor to initialize.
             * @param ESC The ESC to initialize.
             *
             * @note Typically called once during helicopter setup.
             */
            static void initMotorAndESC( CEMotor &Motor, CESController &ESC );

            /**
             * @brief Resets electric motor to initial state.
             *
             * Clears motor RPM, current, temperature, and other transient state
             * variables. Prepares motor for a fresh flight.
             *
             * @param Motor The electric motor to reset.
             */
            static void resetMotor( CEMotor &Motor );

            /**
             * @brief Charges the battery pack to full capacity.
             *
             * Sets pack state to 100% charge with nominal cell voltage (4.2V for LiPo).
             * Resets capacity counters and health metrics.
             *
             * @param Pack The battery pack to charge.
             *
             * @note In simulation, this is instant. Real charging follows CC-CV profile.
             */
            static void chargePack( CBatteryPack &Pack );

            /**
             * @brief Discharges the battery pack based on current draw.
             *
             * Updates battery state of charge (SoC) and voltage based on current
             * consumption. Models LiPo discharge characteristics with voltage
             * dependent on load and SoC.
             *
             * @param Pack The battery pack to discharge.
             * @param I Current draw in amperes [A] (positive = discharge).
             * @param dt Delta time step in seconds.
             *
             * @details LiPo discharge curve approximation per cell:
             * - 4.20V at 100% SoC (fully charged)
             * - 3.87V at 90% SoC
             * - 3.70V at 50% SoC (nominal voltage under load)
             * - 3.53V at 5% SoC
             * - 3.00V at 0% SoC (discharged, cutoff voltage)
             *
             * Voltage slopes per unit pack state:
             * - 100% to 90%: 3.3 V/unit (steep initial drop)
             * - 90% to 5%: 0.4 V/unit (flat middle region)
             * - 5% to 0%: 10.6 V/unit (steep final drop)
             *
             * @note Pack voltage = NumCells × CellVoltage
             * @note Internal resistance causes additional voltage sag under load.
             */
            static void dischargePack( CBatteryPack &Pack, physics_Num I, physics_Num dt );

            /**
             * @brief Calculates motor performance based on battery and ESC state.
             *
             * Computes motor torque, RPM, efficiency, and power draw using:
             * - Motor electrical equations (back-EMF, armature current)
             * - ESC throttle command and timing
             * - Battery voltage and internal resistance
             *
             * @param Pack The battery pack providing power (voltage source).
             * @param Motor The electric motor to calculate (torque/RPM updated).
             * @param ESC The ESC controlling the motor (throttle input).
             *
             * @note Motor equations:
             *       V = I×R + ?/Kv (voltage balance)
             *       Torque = Kt×I (torque constant)
             *       Power_out = Torque × ?
             */
            static void motorCalc( const CBatteryPack &Pack, CEMotor &Motor, const CESController &ESC );

            /**
             * @brief Controls ESC throttle ramping (soft-start).
             *
             * Implements gradual throttle increase on startup to prevent:
             * - Mechanical shock to drivetrain
             * - Excessive current spikes
             * - Voltage sag and brownout
             *
             * @param dt Delta time step in seconds.
             * @param ESC The ESC to control (ramp state updated).
             *
             * @note Ramp rate is configurable (e.g., 0-100% in 1-3 seconds).
             * @note Also handles throttle decay on shutdown.
             */
            static void rampControl( physics_Num dt, CESController &ESC );

            /**
             * @brief Executes ESC governor control loop.
             *
             * Maintains constant rotor RPM by adjusting motor throttle using PID control.
             * Compensates for load changes and battery voltage drop.
             *
             * @param InSig Input throttle signal from receiver [-1.0 to +1.0].
             * @param dt Delta time step in seconds.
             * @param RPM Current motor RPM reading [rev/min].
             * @param Pack The battery pack state (affects available power).
             * @param ESC The ESC to control (governor output updated).
             *
             * @note ESC governor is typically more aggressive than engine governor.
             * @note Accounts for battery voltage drop under load.
             */
            static void escDoGovernor( physics_Num InSig, physics_Num dt, physics_Num RPM,
                                       const CBatteryPack &Pack, CESController &ESC );

            /**
             * @brief Main simulation step for electric motor system.
             *
             * Performs one time step of the complete electric drive dynamics:
             * 1. Calculate motor electrical behavior (current, back-EMF)
             * 2. Compute motor torque from current
             * 3. Integrate angular acceleration to update RPM
             * 4. Update battery discharge
             * 5. Apply ESC governor and limits
             *
             * @param[in,out] OPRPM Output RPM (load side, e.g., main rotor via gearbox) [rev/min].
             * @param[in,out] ERPM Motor electrical RPM (commutation frequency) [rev/min].
             * @param LoadInertia Rotational moment of inertia of the load [kg?m²].
             * @param LoadTorque Torque applied by the load (aerodynamic + friction) [N?m].
             * @param dt Delta time step in seconds.
             * @param EM The electric motor containing all system parameters and state.
             *
             * @note ERPM = mechanical RPM × pole pairs for brushless motors.
             * @note Uses motor equations: V = IR + Ke×?, T = Kt×I
             */
            static void mainEMotorStep( physics_Num &OPRPM, physics_Num &ERPM, physics_Num LoadInertia,
                                        physics_Num LoadTorque, physics_Num dt, CEMotor &EM );

            /** @} */
        };
    } // namespace vehicle
} // namespace workphone

#endif // AerodymanicsUtil_h__
