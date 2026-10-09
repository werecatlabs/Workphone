#ifndef CarDriveTrain_h__
#define CarDriveTrain_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IDriveTrain.hpp>
#include <WPVehiclePhysics/CVehicleComponent.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>

namespace workphone
{
    /**
     * @brief Vehicle drivetrain component that manages engine, transmission, and power distribution to
     * wheels.
     *
     * The CDriveTrain class simulates a complete automotive drivetrain system including:
     * - Engine torque and power characteristics with configurable torque curves
     * - Multi-gear transmission with automatic or manual shifting
     * - Differential behavior with configurable locking characteristics
     * - Power distribution to connected wheels
     * - Engine friction and inertia modeling for realistic behavior
     *
     * The drivetrain approximates engine torque curves using peak torque/power values and RPM points
     * rather than storing complete torque curve data for performance and simplicity.
     *
     * @see VehicleComponent
     * @see IDriveTrain
     * @author Fireblade Engine Team
     * @version 1.0
     */
    class WPVehiclePhysics_API CDriveTrain : public CVehicleComponent<IDriveTrain>
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the drivetrain with default engine characteristics suitable for
         * a mid-range performance vehicle.
         */
        CDriveTrain();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of drivetrain resources.
         */
        ~CDriveTrain() override;

        /**
         * @brief Updates the drivetrain simulation for one frame.
         *
         * Performs per-frame calculations including:
         * - Engine torque calculation based on current RPM and throttle
         * - Automatic gear shifting logic (if enabled)
         * - Power distribution to wheels through gearbox and differential
         * - Engine state updates (RPM, angular velocity)
         */
        void update() override;

        /**
         * @brief Calculates the current engine torque output.
         *
         * Computes engine torque based on the current RPM, throttle position, and
         * the configured torque curve characteristics. Uses interpolation between
         * key torque/power points to approximate a realistic torque curve.
         *
         * @return Current engine torque in Newton-meters (Nm)
         */
        physics_Num calcEngineTorque();

        /**
         * @brief Shifts the transmission to the next higher gear.
         *
         * Increases the current gear by one if not already at the highest gear.
         * Has no effect if the transmission is in automatic mode or already at maximum gear.
         */
        void shiftUp();

        /**
         * @brief Shifts the transmission to the next lower gear.
         *
         * Decreases the current gear by one if not already at the lowest gear.
         * Has no effect if the transmission is in automatic mode or already at minimum gear.
         */
        void shiftDown();

        /**
         * @brief Gets the array of wheels connected to this drivetrain.
         *
         * @return Array of smart pointers to wheel components that receive power from this drivetrain
         */
        Array<SmartPtr<IWheelComponent>> getWheels() const override;

        /**
         * @brief Sets the wheels that this drivetrain will power.
         *
         * @param wheels Array of wheel components to connect to this drivetrain
         */
        void setWheels(Array<SmartPtr<IWheelComponent>> wheels) override;

        /**
         * @brief Gets the gearbox component.
         *
         * @return Smart pointer to the gearbox interface, or nullptr if not set
         */
        SmartPtr<IGearBox> getGearBox() const override;

        /**
         * @brief Sets the gearbox component.
         *
         * @param gearBox Smart pointer to the gearbox implementation
         */
        void setGearBox(SmartPtr<IGearBox> gearBox) override;

        /**
         * @brief Gets the differential component.
         *
         * @return Smart pointer to the differential interface, or nullptr if not set
         */
        SmartPtr<IDifferential> getDifferential() const override;

        /**
         * @brief Sets the differential component.
         *
         * @param differential Smart pointer to the differential implementation
         */
        void setDifferential(SmartPtr<IDifferential> differential) override;

        /**
         * @brief Gets the current throttle position (processed).
         *
         * @return Throttle position as a value between 0.0 (idle) and 1.0 (full throttle)
         */
        f32 getThrottle() const override;

        /**
         * @brief Sets the processed throttle position.
         *
         * This is typically the throttle input after traction control processing.
         *
         * @param throttle Throttle position between 0.0 and 1.0
         */
        void setThrottle(f32 throttle) override;

        /** @copydoc IDriveTrain::getBrake */
        f32 getBrake() const override;

        /** @copydoc IDriveTrain::setBrake */
        void setBrake(f32 brake) override;

        /** @name Engine Orientation */
        /** @{ */
        /**
         * @brief Gets the engine orientation vector.
         *
         * @return 3D vector representing engine orientation (typically forward or right)
         */
        Vector3F getEngineOrientation() const;

        /**
         * @brief Sets the engine orientation vector.
         *
         * Determines how the vehicle body reacts to engine torque. Typically set to
         * Vector3::UNIT_Z (forward) or Vector3::UNIT_X (right) depending on engine layout.
         *
         * @param orientation 3D vector representing desired engine orientation
         */
        void setEngineOrientation(const Vector3F &orientation);
        /** @} */

        /** @name Gear Ratios */
        /** @{ */
        /**
         * @brief Gets the array of gear ratios.
         *
         * @return Array containing gear ratios for all gears including reverse (negative) and neutral
         * (0)
         */
        Array<physics_Num> getGearRatios() const;

        /**
         * @brief Sets the gear ratios for the transmission.
         *
         * The array should contain ratios for all gears where:
         * - Negative values represent reverse gears
         * - Zero represents neutral
         * - Positive values represent forward gears (typically decreasing for higher gears)
         *
         * @param ratios Array of gear ratios
         */
        void setGearRatios(const Array<physics_Num> &ratios);
        /** @} */

        /** @name Final Drive */
        /** @{ */
        /**
         * @brief Gets the final drive ratio.
         *
         * @return Final drive ratio multiplied with gear ratios
         */
        physics_Num getFinalDriveRatio() const;

        /**
         * @brief Sets the final drive ratio.
         *
         * This ratio is multiplied with individual gear ratios to determine the
         * overall drivetrain reduction. Higher values provide more torque multiplication
         * but reduce top speed.
         *
         * @param ratio Final drive ratio (typically 2.5 to 4.5 for passenger vehicles)
         */
        void setFinalDriveRatio(physics_Num ratio);
        /** @} */

        /** @name RPM Range */
        /** @{ */
        /**
         * @brief Gets the minimum engine RPM.
         *
         * @return Minimum RPM before engine stalls
         */
        physics_Num getMinRPM() const;

        /**
         * @brief Sets the minimum engine RPM.
         *
         * @param rpm Minimum RPM (typically 600-1000 for passenger vehicles)
         */
        void setMinRPM(physics_Num rpm);

        /**
         * @brief Gets the maximum engine RPM.
         *
         * @return Maximum RPM (redline)
         */
        physics_Num getMaxRPM() const;

        /**
         * @brief Sets the maximum engine RPM.
         *
         * @param rpm Maximum RPM before engine damage (typically 6000-8000 for passenger vehicles)
         */
        void setMaxRPM(physics_Num rpm);
        /** @} */

        /** @name Torque Characteristics */
        /** @{ */
        /**
         * @brief Gets the maximum engine torque.
         *
         * @return Peak torque output in Newton-meters (Nm)
         */
        physics_Num getMaxTorque() const;

        /**
         * @brief Sets the maximum engine torque.
         *
         * @param torque Peak torque in Newton-meters (Nm)
         */
        void setMaxTorque(physics_Num torque);

        /**
         * @brief Gets the RPM at which maximum torque occurs.
         *
         * @return RPM where peak torque is produced
         */
        physics_Num getTorqueRPM() const;

        /**
         * @brief Sets the RPM at which maximum torque occurs.
         *
         * @param rpm RPM for peak torque (typically 2000-4000 for passenger vehicles)
         */
        void setTorqueRPM(physics_Num rpm);
        /** @} */

        /** @name Power Characteristics */
        /** @{ */
        /**
         * @brief Gets the maximum engine power.
         *
         * @return Peak power output in Watts
         */
        physics_Num getMaxPower() const;

        /**
         * @brief Sets the maximum engine power.
         *
         * @param power Peak power in Watts
         */
        void setMaxPower(physics_Num power);

        /**
         * @brief Gets the RPM at which maximum power occurs.
         *
         * @return RPM where peak power is produced
         */
        physics_Num getPowerRPM() const;

        /**
         * @brief Sets the RPM at which maximum power occurs.
         *
         * @param rpm RPM for peak power (typically 5000-7000 for passenger vehicles)
         */
        void setPowerRPM(physics_Num rpm);
        /** @} */

        /** @name Engine Inertia */
        /** @{ */
        /**
         * @brief Gets the engine rotational inertia.
         *
         * @return Engine inertia in kg⋅m²
         */
        physics_Num getEngineInertia() const;

        /**
         * @brief Sets the engine rotational inertia.
         *
         * Controls how quickly the engine can change RPM. Higher values make the engine
         * more sluggish to rev up or down.
         *
         * @param inertia Engine inertia in kg⋅m² (typically 0.1-1.0 for passenger vehicles)
         */
        void setEngineInertia(physics_Num inertia);
        /** @} */

        /** @name Engine Friction */
        /** @{ */
        /**
         * @brief Gets the base engine friction coefficient.
         *
         * @return Constant friction coefficient
         */
        physics_Num getEngineBaseFriction() const;

        /**
         * @brief Sets the base engine friction coefficient.
         *
         * Constant friction that opposes engine rotation regardless of RPM.
         *
         * @param friction Base friction coefficient
         */
        void setEngineBaseFriction(physics_Num friction);

        /**
         * @brief Gets the RPM-dependent engine friction coefficient.
         *
         * @return Linear friction coefficient that scales with RPM
         */
        physics_Num getEngineRPMFriction() const;

        /**
         * @brief Sets the RPM-dependent engine friction coefficient.
         *
         * Linear friction that increases with engine RPM, simulating increased
         * internal friction at higher speeds.
         *
         * @param friction RPM-dependent friction coefficient
         */
        void setEngineRPMFriction(physics_Num friction);
        /** @} */

        /** @name Differential */
        /** @{ */
        /**
         * @brief Gets the differential lock coefficient.
         *
         * @return Coefficient determining torque transfer between wheels (0.0-1.0)
         */
        physics_Num getDifferentialLockCoefficient() const;

        /**
         * @brief Sets the differential lock coefficient.
         *
         * Controls how much torque is transferred between wheels when they rotate
         * at different speeds. 0.0 = open differential, 1.0 = locked differential.
         *
         * @param coefficient Lock coefficient (0.0 for open diff, 1.0 for locked diff)
         */
        void setDifferentialLockCoefficient(physics_Num coefficient);
        /** @} */

        /** @name Throttle Input */
        /** @{ */
        /**
         * @brief Gets the raw throttle input.
         *
         * @return Raw throttle input before traction control processing
         */
        physics_Num getThrottleInput() const override;

        /**
         * @brief Sets the raw throttle input.
         *
         * This is the direct driver input before any traction control or other
         * processing is applied.
         *
         * @param throttle Raw throttle input (0.0-1.0)
         */
        void setThrottleInput(physics_Num throttle) override;
        /** @} */

        physics_Num getBrakeInput() const override;

        void setBrakeInput(physics_Num brakeInput) override;

        /** @name Transmission Mode */
        /** @{ */
        /**
         * @brief Checks if the transmission is in automatic mode.
         *
         * @return true if automatic transmission is enabled, false for manual
         */
        bool isAutomatic() const;

        /**
         * @brief Sets the transmission mode.
         *
         * @param automatic true for automatic shifting, false for manual control
         */
        void setAutomatic(bool automatic);
        /** @} */

        /** @name Gear State */
        /** @{ */
        /**
         * @brief Gets the current gear.
         *
         * @return Current gear index (negative for reverse, 0 for neutral, positive for forward)
         */
        s32 getGear() const;

        /**
         * @brief Sets the current gear.
         *
         * @param gear Gear index to select
         */
        void setGear(s32 gear);
        /** @} */

        /** @name Engine State */
        /** @{ */
        /**
         * @brief Gets the current engine RPM.
         *
         * @return Current engine revolutions per minute
         */
        physics_Num getRPM() const;

        /**
         * @brief Sets the current engine RPM.
         *
         * @param rpm Engine RPM to set
         */
        void setRPM(physics_Num rpm);

        /**
         * @brief Gets the current slip ratio.
         *
         * @return Slip ratio between engine and wheels
         */
        physics_Num getSlipRatio() const;

        /**
         * @brief Sets the slip ratio.
         *
         * @param ratio Slip ratio to set
         */
        void setSlipRatio(physics_Num ratio);

        /**
         * @brief Gets the engine angular velocity.
         *
         * @return Angular velocity in radians per second
         */
        physics_Num getEngineAngularVelocity() const;

        /**
         * @brief Sets the engine angular velocity.
         *
         * @param velocity Angular velocity in radians per second
         */
        void setEngineAngularVelocity(physics_Num velocity);
        /** @} */

        SmartPtr<Properties> getProperties() const override;
        void setProperties(SmartPtr<Properties> properties) override;

        physics_Num getStarterGearRatio() const;
        void setStarterGearRatio(physics_Num starterGearRatio);

        physics_Num getClutchThrottleRPMBoost() const;
        void setClutchThrottleRPMBoost(physics_Num clutchThrottleRPMBoost);

        physics_Num getUpShiftThrottleRPMScale() const;
        void setUpShiftThrottleRPMScale(physics_Num upShiftThrottleRPMScale);

        physics_Num getUpShiftBaseRPMScale() const;
        void setUpShiftBaseRPMScale(physics_Num upShiftBaseRPMScale);

        physics_Num getDownShiftThrottleRPMScale() const;
        void setDownShiftThrottleRPMScale(physics_Num downShiftThrottleRPMScale);

        physics_Num getDownShiftBaseRPMScale() const;
        void setDownShiftBaseRPMScale(physics_Num downShiftBaseRPMScale);

        physics_Num getOverRevTorqueFalloff() const;
        void setOverRevTorqueFalloff(physics_Num overRevTorqueFalloff);

        WP_CLASS_REGISTER_DECL;

    protected:
        /** @name Engine Configuration */
        /** @{ */
        /**
         * @brief Engine orientation vector.
         *
         * Determines how the vehicle body reacts to engine torque.
         * Typically Vector3::UNIT_Z (forward) or Vector3::UNIT_X (right).
         */
        Vector3F m_engineOrientation;

        /**
         * @brief Gear ratios for all gears.
         *
         * Array containing ratios for reverse (negative), neutral (0), and forward gears.
         * Higher gear ratios provide more torque multiplication but lower top speed.
         */
        Array<physics_Num> m_gearRatios;

        /**
         * @brief Final drive ratio.
         *
         * Multiplied with gear ratios for overall drivetrain reduction.
         * Default: 3.23 (typical for passenger vehicles).
         */
        physics_Num m_finalDriveRatio = 3.23;
        /** @} */

        /** @name Engine Performance Characteristics */
        /** @{ */
        /**
         * @brief Minimum engine RPM before stalling.
         *
         * Default: 800 RPM (typical idle speed).
         */
        physics_Num m_minRPM = static_cast<physics_Num>(800);

        /**
         * @brief Maximum engine RPM (redline).
         *
         * Default: 6400 RPM (typical for passenger vehicles).
         */
        physics_Num m_maxRPM = static_cast<physics_Num>(6400);

        /**
         * @brief Maximum engine torque output.
         *
         * Peak torque in Newton-meters. Default: 664 Nm.
         */
        physics_Num m_maxTorque = static_cast<physics_Num>(664);

        /**
         * @brief RPM at which maximum torque occurs.
         *
         * Default: 4000 RPM (typical for naturally aspirated engines).
         */
        physics_Num m_torqueRPM = static_cast<physics_Num>(4000);

        /**
         * @brief Maximum engine power output.
         *
         * Peak power in Watts. Default: 317000 W (317 kW).
         */
        physics_Num m_maxPower = static_cast<physics_Num>(317000);

        /**
         * @brief RPM at which maximum power occurs.
         *
         * Default: 5000 RPM (typically higher than peak torque RPM).
         */
        physics_Num m_powerRPM = static_cast<physics_Num>(5000);
        /** @} */

        /** @name Engine Dynamics */
        /** @{ */
        /**
         * @brief Engine rotational inertia.
         *
         * Controls engine acceleration/deceleration responsiveness in kg⋅m².
         * Default: 0.3 kg⋅m² (typical for passenger vehicles).
         */
        physics_Num m_engineInertia = 0.3;

        /**
         * @brief Constant engine friction coefficient.
         *
         * Base friction opposing engine rotation regardless of RPM.
         * Default: 25.0 (dimensionless coefficient).
         */
        physics_Num m_engineBaseFriction = 25.0;

        /**
         * @brief RPM-dependent engine friction coefficient.
         *
         * Linear friction that increases with engine speed.
         * Default: 0.02 (friction per RPM).
         */
        physics_Num m_engineRPMFriction = 0.02;
        /** @} */

        /** @name Differential Configuration */
        /** @{ */
        /**
         * @brief Differential lock coefficient.
         *
         * Controls torque transfer between wheels (0.0 = open, 1.0 = locked).
         * Default: 0.0 (open differential).
         */
        physics_Num m_differentialLockCoefficient = static_cast<physics_Num>(0);
        /** @} */

        /** @name Input State */
        /** @{ */
        /**
         * @brief Processed throttle position.
         *
         * Throttle after traction control processing (0.0-1.0).
         */
        physics_Num m_throttle = 0.0;

        /**
         * @brief Raw throttle input.
         *
         * Direct driver input before processing (0.0-1.0).
         */
        physics_Num m_throttleInput = 0.0;

        /**
         * @brief Processed brake position.
         *
         * Brake after abs and engine braking processing (0.0-1.0).
         */
        physics_Num m_brake = 0.0;

        /**
         * @brief Raw brake input.
         *
         * Direct driver input before processing (0.0-1.0).
         */
        physics_Num m_brakeInput = 0.0;

        /**
         * @brief Automatic transmission flag.
         *
         * true for automatic shifting, false for manual control.
         * Default: true.
         */
        bool m_automatic = true;

        /** @} */

        /** @name Runtime State */
        /** @{ */
        /**
         * @brief Current gear selection.
         *
         * Gear index (negative=reverse, 0=neutral, positive=forward).
         * Default: 2 (second gear for hill starts).
         */
        s32 m_gear = 2;

        /**
         * @brief Current engine RPM.
         *
         * Engine revolutions per minute.
         */
        physics_Num m_rpm = 0.0;

        /**
         * @brief Current slip ratio.
         *
         * Ratio representing slip between engine and wheels.
         */
        physics_Num m_slipRatio = 0.0;

        /**
         * @brief Engine angular velocity.
         *
         * Current engine angular velocity in radians per second.
         */
        physics_Num m_engineAngularVelo = 0.0;
        /** @} */

        physics_Num m_starterGearRatio = 3.0;
        physics_Num m_clutchThrottleRPMBoost = 3000.0;
        physics_Num m_upShiftBaseRPMScale = 0.5;
        physics_Num m_upShiftThrottleRPMScale = 0.5;
        physics_Num m_downShiftBaseRPMScale = 0.25;
        physics_Num m_downShiftThrottleRPMScale = 0.4;
        physics_Num m_overRevTorqueFalloff = 0.006;

        /** @name Component References */
        /** @{ */
        /**
         * @brief Connected wheel components.
         *
         * Array of wheels that receive power from this drivetrain.
         */
        Array<SmartPtr<IWheelComponent>> m_wheels;

        /**
         * @brief Gearbox component reference.
         *
         * Smart pointer to the gearbox implementation.
         */
        SmartPtr<IGearBox> m_gearBox;

        /**
         * @brief Differential component reference.
         *
         * Smart pointer to the differential implementation.
         */
        SmartPtr<IDifferential> m_differential;
        /** @} */
    };
} // namespace workphone

#endif // CarDriveTrain_h__
