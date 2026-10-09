#ifndef EngClutchUnitH
#define EngClutchUnitH

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include "HeliVars.hpp"

namespace workphone::vehicle
{
    /**
     * @brief Fixed-size array representing a single engine power curve.
     *
     * Each curve contains 41 sampled points. The exact mapping from index to
     * RPM or throttle depends on the consuming code; treat this as an ordered
     * lookup table for engine power values.
     */
    using TPowerCurve = FixedArray<physics_Num, 41>;

    /**
     * @brief Collection of power curves.
     *
     * The array length is 3 (valid indices 0..2). Typically each entry may
     * represent a different throttle setting, condition or map used by the
     * engine model.
     */
    using TPowerCurves = FixedArray<TPowerCurve, 3>;

    /**
     * @brief Models the engine, clutch and sprag interaction for a drivetrain.
     *
     * This class stores engine/clutch state (RPM, angular velocity, torque,
     * moments of inertia, power curves and clutch transmission capability)
     * and provides simple accessors for key runtime quantities.
     *
     * Units:
     * - RPM fields are in revolutions per minute.
     * - Omega fields are in radians per second.
     * - Power is in watts.
     * - Torque is in newton-meters.
     * - Moments of inertia are in kg*m^2 (or the physics_Num unit used project-wide).
     */
    class WPVehiclePhysics_API EngineClutchUnit
    {
    public:
        /**
         * @brief Construct a default CEngineClutchUnit.
         *
         * Initializes numeric members to zero and prepares default power curve
         * storage. No heavy runtime work is performed in the default ctor.
         */
        EngineClutchUnit();

        /**
         * @brief Copy construction is disabled.
         */
        EngineClutchUnit(const EngineClutchUnit &other) = delete;

        ~EngineClutchUnit();

        /**
         * @brief Get the crank speed in revolutions per minute (RPM).
         * @return Current crank speed (RPM).
         */
        physics_Num getCrankRPM() const;

        /**
         * @brief Set the crank speed in revolutions per minute (RPM).
         * @param rpm Crank speed to set (RPM).
         */
        void setCrankRPM(physics_Num rpm);

        /**
         * @brief Get the crank angular velocity in radians per second.
         * @return Current crank angular velocity (rad/s).
         */
        physics_Num getCrankOmega() const;

        /**
         * @brief Set the crank angular velocity in radians per second.
         * @param omega Angular velocity to set (rad/s).
         */
        void setCrankOmega(physics_Num omega);

        /**
         * @brief Get the engine torque produced at the crank.
         * @return Engine torque (N·m).
         */
        physics_Num getEngineTorque() const;

        /**
         * @brief Set the engine torque produced at the crank.
         * @param torque Torque value to set (N·m).
         */
        void setEngineTorque(physics_Num torque);

        /**
         * @brief Engine power lookup tables.
         *
         * Each of the three curves contains 41 sample points. Consumers should
         * perform interpolation as needed.
         */
        TPowerCurves m_powerCurves =
            TPowerCurves({ TPowerCurve({ static_cast<physics_Num>(0.0) }) });

        /**
         * @brief Peak power output (watts) at full throttle and at m_peakPowerRPM.
         */
        physics_Num m_peakPower = 0.0;

        /**
         * @brief Engine RPM at which peak power (m_peakPower) occurs.
         */
        physics_Num m_peakPowerRPM = 0.0;

        /**
         * @brief Moment of inertia of the engine (kg*m^2 or project physics unit).
         */
        physics_Num m_moiEngine = 0.0;

        /**
         * @brief Moment of inertia of the clutch (kg*m^2 or project physics unit).
         */
        physics_Num m_moiClutch = 0.0;

        /**
         * @brief Total rotational inertia seen by the crank (engine + clutch).
         */
        physics_Num m_moiTotal = 0.0;

        /**
         * @brief Current throttle position, normalized 0.0 (closed) to 1.0 (wide open).
         */
        physics_Num m_throttlePos = 0.0;

        /**
         * @brief Current computed engine power (watts) based on throttle/curves.
         */
        physics_Num m_enginePower = 0.0;

        /**
         * @brief Output RPM at the sprag clutch (after gearing / mechanics).
         *
         * This is the rotational speed at the sprag/clutch output side, expressed
         * in revolutions per minute.
         */
        physics_Num m_spragRPM = 0.0;

        /**
         * @brief Output angular velocity at the sprag clutch (rad/s).
         */
        physics_Num m_spragOmega = 0.0;

        /**
         * @brief Torque transmitted to the load via the sprag/clutch (N·m).
         */
        physics_Num m_transmittedTorque = 0.0;

        /**
         * @brief Current clutch torque transmission capability (N·m).
         *
         * Represents the maximum torque the clutch can convey at the current
         * conditions (engagement, wear, preload, etc.).
         */
        physics_Num m_clutchCapability = 0.0;

        /**
         * @brief Clutch characteristic constant relating torque change to RPM^2.
         *
         * Used as a factor in clutch engagement calculations: d(torque) / d(RPM^2).
         */
        physics_Num m_clutchConst = 0.0;

        /**
         * @brief Engine RPM at which the clutch begins to "bite" (engage).
         */
        physics_Num m_biteRPM = 0.0;

        /**
         * @brief Cached squared value of m_biteRPM (RPM^2) to avoid repeated calculation.
         */
        physics_Num m_biteRPMSq = 0.0;

        /**
         * @brief Clutch mode or state indicator (implementation-defined integer codes).
         *
         * Values and meanings are defined by the higher-level simulation code.
         */
        s32 m_clutchMode = 0;

        /**
         * @brief Current engine crank speed in revolutions per minute (RPM).
         */
        physics_Num m_crankRPM = 0.0;

        /**
         * @brief Current engine crank angular velocity in radians per second (rad/s).
         */
        physics_Num m_crankOmega = 0.0;

        /**
         * @brief Engine torque produced at the crank (N·m).
         */
        physics_Num m_engineTorque = 0.0;
    }; // CEngineClutchUnit
} // namespace workphone::vehicle

#endif  //  EngClutchUnitH
