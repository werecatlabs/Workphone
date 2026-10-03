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

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IDriveTrain_h__
