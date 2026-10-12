#ifndef IDriveTrain_h__
#define IDriveTrain_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Interface for a vehicle drive train system.
         *
         * The IDriveTrain interface defines the contract for vehicle drivetrain components,
         * which are responsible for transmitting power from the engine to the wheels.
         * It manages wheels, gearbox, differential, and throttle control including
         * traction control functionality.
         *
         * @see IVehicleComponent
         * @see IWheelComponent
         * @see IGearBox
         * @see IDifferential
         */
        class WPCore_API IDriveTrain : public IVehicleComponent
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of derived classes.
             */
            ~IDriveTrain() override;

            /**
             * @brief Gets the array of wheel components attached to this drivetrain.
             *
             * Retrieves all wheel components that are managed by this drivetrain system.
             * Each wheel component represents a physical wheel that receives power from
             * the drivetrain.
             *
             * @return Array of smart pointers to wheel components.
             */
            virtual Array<SmartPtr<IWheelComponent>> getWheels() const = 0;

            /**
             * @brief Sets the array of wheel components for this drivetrain.
             *
             * Assigns a new set of wheel components to be managed by this drivetrain.
             * This method is typically called during vehicle initialization or when
             * reconfiguring the wheel setup.
             *
             * @param wheels Array of smart pointers to wheel components to be managed.
             */
            virtual void setWheels( Array<SmartPtr<IWheelComponent>> wheels ) = 0;

            /**
             * @brief Gets the gearbox component of this drivetrain.
             *
             * Retrieves the gearbox that handles gear ratios and transmission logic
             * for this drivetrain system.
             *
             * @return Smart pointer to the gearbox component, or nullptr if not set.
             */
            virtual SmartPtr<IGearBox> getGearBox() const = 0;

            /**
             * @brief Sets the gearbox component for this drivetrain.
             *
             * Assigns a gearbox component to handle transmission and gear ratio
             * management for this drivetrain system.
             *
             * @param gearBox Smart pointer to the gearbox component to be used.
             */
            virtual void setGearBox( SmartPtr<IGearBox> gearBox ) = 0;

            /**
             * @brief Gets the differential component of this drivetrain.
             *
             * Retrieves the differential that manages power distribution between
             * wheels, allowing them to rotate at different speeds during turns.
             *
             * @return Smart pointer to the differential component, or nullptr if not set.
             */
            virtual SmartPtr<IDifferential> getDifferential() const = 0;

            /**
             * @brief Sets the differential component for this drivetrain.
             *
             * Assigns a differential component to manage power distribution
             * between wheels in this drivetrain system.
             *
             * @param differential Smart pointer to the differential component to be used.
             */
            virtual void setDifferential( SmartPtr<IDifferential> differential ) = 0;

            /**
             * @brief Gets the current throttle position after traction control.
             *
             * This method retrieves the throttle position that has been processed by traction control.
             * The value should be between 0.0 (idle) and 1.0 (full throttle).
             *
             * @return The current throttle position as a normalized float value [0.0, 1.0].
             */
            virtual f32 getThrottle() const = 0;

            /**
             * @brief Sets the throttle position after traction control.
             *
             * The throttle value should be between 0.0 (idle) and 1.0 (full throttle).
             * This method processes the input throttle value to apply it to the drivetrain.
             *
             * @param throttle The new throttle position as a normalized float value [0.0, 1.0].
             */
            virtual void setThrottle( f32 throttle ) = 0;

            /**
             * @brief Gets the raw throttle input.
             *
             * This method retrieves the raw throttle input value, which may be used for traction control
             * or other processing before applying it to the drivetrain. This represents the unprocessed
             * input from the driver or control system.
             *
             * @return The raw throttle input value as a normalized float [0.0, 1.0].
             */
            virtual f32 getThrottleInput() const = 0;

            /**
             * @brief Sets the raw throttle input.
             *
             * This method sets the throttle input directly, which may be used for traction control
             * or other processing before applying it to the drivetrain. This is typically the
             * raw input from the driver or control system before any processing.
             *
             * @param throttle The raw throttle input value as a normalized float [0.0, 1.0].
             */
            virtual void setThrottleInput( f32 throttle ) = 0;

            /**
             * @brief Gets the current brake input.
             * @return The brake input value as a normalized float [0.0, 1.0].
             */
            virtual f32 getBrake() const = 0;

            /**
             * @brief Sets the brake input.
             * @param brake The brake input value as a normalized float [0.0, 1.0].
             */
            virtual void setBrake( f32 brake ) = 0;

            /**
             * @brief Gets the raw brake input.
             *
             * This method retrieves the raw brake input value, which may be used for traction control
             * or other processing before applying it to the drivetrain. This represents the unprocessed
             * input from the driver or control system.
             *
             * @return The raw brake input value as a normalized float [0.0, 1.0].
             */
            virtual f32 getBrakeInput() const = 0;

            /**
             * @brief Sets the raw brake input.
             *
             * This method sets the brake input directly, which may be used for traction control
             * or other processing before applying it to the drivetrain. This is typically the
             * raw input from the driver or control system before any processing.
             *
             * @param brakeInput The raw brake input value as a normalized float [0.0, 1.0].
             */
            virtual void setBrakeInput( f32 brakeInput ) = 0;

            /** @name Engine Orientation */
            /** @{ */
            /**
             * @brief Gets the engine orientation vector.
             *
             * @return 3D vector representing engine orientation (typically forward or right)
             */
            virtual Vector3F getEngineOrientation() const = 0;

            /**
             * @brief Sets the engine orientation vector.
             *
             * Determines how the vehicle body reacts to engine torque. Typically set to
             * Vector3::UNIT_Z (forward) or Vector3::UNIT_X (right) depending on engine layout.
             *
             * @param orientation 3D vector representing desired engine orientation
             */
            virtual void setEngineOrientation( const Vector3F &orientation ) = 0;
            /** @} */

            /** @name Gear Ratios */
            /** @{ */
            /**
             * @brief Gets the array of gear ratios.
             *
             * @return Array containing gear ratios for all gears including reverse (negative) and neutral
             * (0)
             */
            virtual Array<physics_Num> getGearRatios() const = 0;

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
            virtual void setGearRatios( const Array<physics_Num> &ratios ) = 0;

            /** @name Final Drive */
            /** @{ */
            /**
             * @brief Gets the final drive ratio.
             *
             * @return Final drive ratio multiplied with gear ratios
             */
            virtual physics_Num getFinalDriveRatio() const = 0;

            /**
             * @brief Sets the final drive ratio.
             *
             * This ratio is multiplied with individual gear ratios to determine the
             * overall drivetrain reduction. Higher values provide more torque multiplication
             * but reduce top speed.
             *
             * @param ratio Final drive ratio (typically 2.5 to 4.5 for passenger vehicles)
             */
            virtual void setFinalDriveRatio( physics_Num ratio ) = 0;
            /** @} */

            /** @name RPM Range */
            /** @{ */
            /**
             * @brief Gets the minimum engine RPM.
             *
             * @return Minimum RPM before engine stalls
             */
            virtual physics_Num getMinRPM() const = 0;

            /**
             * @brief Sets the minimum engine RPM.
             *
             * @param rpm Minimum RPM (typically 600-1000 for passenger vehicles)
             */
            virtual void setMinRPM( physics_Num rpm ) = 0;

            /**
             * @brief Gets the maximum engine RPM.
             *
             * @return Maximum RPM (redline)
             */
            virtual physics_Num getMaxRPM() const = 0;

            /**
             * @brief Sets the maximum engine RPM.
             *
             * @param rpm Maximum RPM before engine damage (typically 6000-8000 for passenger vehicles)
             */
            virtual void setMaxRPM( physics_Num rpm ) = 0;
            /** @} */

            /** @name Torque Characteristics */
            /** @{ */
            /**
             * @brief Gets the maximum engine torque.
             *
             * @return Peak torque output in Newton-meters (Nm)
             */
            virtual physics_Num getMaxTorque() const = 0;

            /**
             * @brief Sets the maximum engine torque.
             *
             * @param torque Peak torque in Newton-meters (Nm)
             */
            virtual void setMaxTorque( physics_Num torque ) = 0;

            /**
             * @brief Gets the RPM at which maximum torque occurs.
             *
             * @return RPM where peak torque is produced
             */
            virtual physics_Num getTorqueRPM() const = 0;

            /**
             * @brief Sets the RPM at which maximum torque occurs.
             *
             * @param rpm RPM for peak torque (typically 2000-4000 for passenger vehicles)
             */
            virtual void setTorqueRPM( physics_Num rpm ) = 0;
            /** @} */

            /** @name Power Characteristics */
            /** @{ */
            /**
             * @brief Gets the maximum engine power.
             *
             * @return Peak power output in Watts
             */
            virtual physics_Num getMaxPower() const = 0;

            /**
             * @brief Sets the maximum engine power.
             *
             * @param power Peak power in Watts
             */
            virtual void setMaxPower( physics_Num power ) = 0;

            /**
             * @brief Gets the RPM at which maximum power occurs.
             *
             * @return RPM where peak power is produced
             */
            virtual physics_Num getPowerRPM() const = 0;

            /**
             * @brief Sets the RPM at which maximum power occurs.
             *
             * @param rpm RPM for peak power (typically 5000-7000 for passenger vehicles)
             */
            virtual void setPowerRPM( physics_Num rpm ) = 0;
            /** @} */

            /** @name Engine Inertia */
            /** @{ */
            /**
             * @brief Gets the engine rotational inertia.
             *
             * @return Engine inertia in kg⋅m²
             */
            virtual physics_Num getEngineInertia() const = 0;

            /**
             * @brief Sets the engine rotational inertia.
             *
             * Controls how quickly the engine can change RPM. Higher values make the engine
             * more sluggish to rev up or down.
             *
             * @param inertia Engine inertia in kg⋅m² (typically 0.1-1.0 for passenger vehicles)
             */
            virtual void setEngineInertia( physics_Num inertia ) = 0;
            /** @} */

            /** @name Engine Friction */
            /** @{ */
            /**
             * @brief Gets the base engine friction coefficient.
             *
             * @return Constant friction coefficient
             */
            virtual physics_Num getEngineBaseFriction() const = 0;

            /**
             * @brief Sets the base engine friction coefficient.
             *
             * Constant friction that opposes engine rotation regardless of RPM.
             *
             * @param friction Base friction coefficient
             */
            virtual void setEngineBaseFriction( physics_Num friction ) = 0;

            /**
             * @brief Gets the RPM-dependent engine friction coefficient.
             *
             * @return Linear friction coefficient that scales with RPM
             */
            virtual physics_Num getEngineRPMFriction() const = 0;

            /**
             * @brief Sets the RPM-dependent engine friction coefficient.
             *
             * Linear friction that increases with engine RPM, simulating increased
             * internal friction at higher speeds.
             *
             * @param friction RPM-dependent friction coefficient
             */
            virtual void setEngineRPMFriction( physics_Num friction ) = 0;
            /** @} */

            /** @name Differential */
            /** @{ */
            /**
             * @brief Gets the differential lock coefficient.
             *
             * @return Coefficient determining torque transfer between wheels (0.0-1.0)
             */
            virtual physics_Num getDifferentialLockCoefficient() const = 0;

            /**
             * @brief Sets the differential lock coefficient.
             *
             * Controls how much torque is transferred between wheels when they rotate
             * at different speeds. 0.0 = open differential, 1.0 = locked differential.
             *
             * @param coefficient Lock coefficient (0.0 for open diff, 1.0 for locked diff)
             */
            virtual void setDifferentialLockCoefficient( physics_Num coefficient ) = 0;
            /** @} */

            /** @name Transmission Mode */
            /** @{ */
            /**
             * @brief Checks if the transmission is in automatic mode.
             *
             * @return true if automatic transmission is enabled, false for manual
             */
            virtual bool isAutomatic() const = 0;

            /**
             * @brief Sets the transmission mode.
             *
             * @param automatic true for automatic shifting, false for manual control
             */
            virtual void setAutomatic( bool automatic ) = 0;
            /** @} */

            /** @name Gear State */
            /** @{ */
            /**
             * @brief Gets the current gear.
             *
             * @return Current gear index (negative for reverse, 0 for neutral, positive for forward)
             */
            virtual s32 getGear() const = 0;

            /**
             * @brief Sets the current gear.
             *
             * @param gear Gear index to select
             */
            virtual void setGear( s32 gear ) = 0;
            /** @} */

            /** @name Engine State */
            /** @{ */
            /**
             * @brief Gets the current engine RPM.
             *
             * @return Current engine revolutions per minute
             */
            virtual physics_Num getRPM() const = 0;

            /**
             * @brief Sets the current engine RPM.
             *
             * @param rpm Engine RPM to set
             */
            virtual void setRPM( physics_Num rpm ) = 0;

            /**
             * @brief Gets the current slip ratio.
             *
             * @return Slip ratio between engine and wheels
             */
            virtual physics_Num getSlipRatio() const = 0;

            /**
             * @brief Sets the slip ratio.
             *
             * @param ratio Slip ratio to set
             */
            virtual void setSlipRatio( physics_Num ratio ) = 0;

            /**
             * @brief Gets the engine angular velocity.
             *
             * @return Angular velocity in radians per second
             */
            virtual physics_Num getEngineAngularVelocity() const = 0;

            /**
             * @brief Sets the engine angular velocity.
             *
             * @param velocity Angular velocity in radians per second
             */
            virtual void setEngineAngularVelocity( physics_Num velocity ) = 0;
            /** @} */

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IDriveTrain_h__
