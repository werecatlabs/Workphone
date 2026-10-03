#ifndef IAircraft_h__
#define IAircraft_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Abstract interface that represents an aircraft.
         *
         * The IAircraft interface extends IVehicle with aircraft-specific
         * properties and components such as propeller units, wheels,
         * aerodynamics wind, battery pack and control surfaces.
         *
         * Implementations are responsible for managing component lifetimes,
         * performing physics updates and exposing telemetry (RPM, thrust, etc.).
         *
         * @note All getters that return SmartPtr or Array should return valid,
         *       properly-initialized objects or empty containers as appropriate.
         */
        class WPCore_API IAircraft : public IVehicle
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures derived aircraft implementations are destructed correctly
             * through the interface pointer.
             */
            ~IAircraft() override;

            /**
             * @brief Get the current ambient air density used by aerodynamic calculations.
             * @return Air density in the same units used throughout the simulation (e.g. kg/m^3).
             */
            virtual real_Num getAirDensity() const = 0;

            /**
             * @brief Set the ambient air density used for aerodynamics.
             * @param airDensity Air density (e.g. kg/m^3).
             */
            virtual void setAirDensity( real_Num airDensity ) = 0;

            /**
             * @brief Get the battery pack associated with this aircraft (if any).
             * @return SmartPtr to the IBatteryPack or null/empty SmartPtr if none assigned.
             */
            virtual SmartPtr<IBatteryPack> getBatteryPack() const = 0;

            /**
             * @brief Assign a battery pack to the aircraft.
             * @param batteryPack SmartPtr to an IBatteryPack implementation.
             */
            virtual void setBatteryPack( SmartPtr<IBatteryPack> batteryPack ) = 0;

            /**
             * @brief Get the aircraft callback handler.
             * @return SmartPtr to an IAircrafCallback used for notifications or control callbacks.
             */
            virtual SmartPtr<IAircraftCallback> getCallback() const = 0;

            /**
             * @brief Set a callback handler for aircraft-level events.
             * @param callback SmartPtr to an IAircrafCallback implementation.
             */
            virtual void setCallback( SmartPtr<IAircraftCallback> callback ) = 0;

            /**
             * @brief Get the aerodynamics wind model used by the aircraft.
             * @return SmartPtr to the IAerodymanicsWind instance.
             */
            virtual SmartPtr<IAerodymanicsWind> getWind() const = 0;

            /**
             * @brief Set the wind model used for aerodynamic computations.
             * @param wind SmartPtr to an IAerodymanicsWind implementation.
             */
            virtual void setWind( SmartPtr<IAerodymanicsWind> wind ) = 0;

            /**
             * @brief Add a propeller unit to the aircraft.
             * @param propellerUnit SmartPtr to the propeller unit to add.
             * @remarks Ownership semantics follow SmartPtr conventions used across the project.
             */
            virtual void addPropellerUnit( SmartPtr<IAircraftPropellerUnit> propellerUnit ) = 0;

            /**
             * @brief Remove a propeller unit from the aircraft.
             * @param propellerUnit SmartPtr to the propeller unit to remove.
             * @remarks Implementations should handle no-op if the unit is not present.
             */
            virtual void removePropellerUnit( SmartPtr<IAircraftPropellerUnit> propellerUnit ) = 0;

            /**
             * @brief Get the list of propeller units currently attached to the aircraft.
             * @return Array of SmartPtr<IAircraftPropellerUnit>.
             */
            virtual Array<SmartPtr<IAircraftPropellerUnit>> getPropellerUnits() const = 0;

            /**
             * @brief Replace the aircraft's propeller units with the provided array.
             * @param propellerUnits Array of SmartPtr<IAircraftPropellerUnit> to set.
             */
            virtual void setPropellerUnits(
                const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits ) = 0;

            /**
             * @brief Add a wheel component (landing gear) to the aircraft.
             * @param wheel SmartPtr to the IWheelComponent to add.
             */
            virtual void addWheel( SmartPtr<IWheelComponent> wheel ) = 0;

            /**
             * @brief Remove a wheel component from the aircraft.
             * @param wheel SmartPtr to the IWheelComponent to remove.
             */
            virtual void removeWheel( SmartPtr<IWheelComponent> wheel ) = 0;

            /**
             * @brief Get the array of wheel components (landing gear).
             * @return Array of SmartPtr<IWheelComponent>.
             */
            virtual Array<SmartPtr<IWheelComponent>> getWheels() const = 0;

            /**
             * @brief Set the aircraft's wheels (landing gear).
             * @param wheels Array of SmartPtr<IWheelComponent> to assign.
             */
            virtual void setWheels( const Array<SmartPtr<IWheelComponent>> &wheels ) = 0;

            /**
             * @brief Get the engine RPM for the engine identified by index.
             * @param idx Zero-based engine/propeller index.
             * @return Engine RPM for the specified index. Returns 0 or a sensible default if the index
             * is invalid.
             */
            virtual real_Num getEngineRPM( s32 idx ) const = 0;

            /**
             * @brief Get the thrust produced by the engine/propeller at the given index.
             * @param idx Zero-based engine/propeller index.
             * @return Thrust value for the specified index (units consistent with project conventions).
             */
            virtual real_Num getThrust( s32 idx ) const = 0;

            /**
             * @brief Set a control surface or actuator angle.
             * @param id Identifier of the control element (convention defined by implementation).
             * @param angle Angle in radians (or degrees if project convention uses degrees) — follow
             * project units.
             * @note Caller and implementation must agree on the units used for angle.
             */
            virtual void setControlAngle( s32 id, f32 angle ) = 0;

            /**
             * @brief Get the section multiplier used to scale aerodynamic sections.
             * @return Multiplier applied to sectional aerodynamic coefficients.
             */
            virtual real_Num getSectionMultiplier() const = 0;

            /**
             * @brief Set the section multiplier used for aerodynamic scaling.
             * @param sectionMultiplier Multiplier value.
             */
            virtual void setSectionMultiplier( real_Num sectionMultiplier ) = 0;

            /**
             * @brief Get the model data file path associated with this aircraft.
             * @return File path string used to load model/visual data.
             */
            virtual String getModelDataFilePath() const = 0;

            /**
             * @brief Set the model data file path for the aircraft.
             * @param filePath Path to the model or configuration file.
             */
            virtual void setModelDataFilePath( const String &filePath ) = 0;

            /**
             * @brief Get the current body transform of the aircraft.
             * @return Transform3<real_Num> representing position and orientation of the aircraft body.
             */
            virtual Transform3<real_Num> getBodyTransform() const = 0;

            /**
             * @brief Set the aircraft's body transform.
             * @param bodyTransform Transform3<real_Num> representing new body pose (position +
             * orientation).
             */
            virtual void setBodyTransform( Transform3<real_Num> bodyTransform ) = 0;

            /**
             * @brief Get damping applied around the roll axis.
             * @return Roll-wise damping coefficient used by the physics model.
             */
            virtual real_Num getRollwiseDamping() const = 0;

            /**
             * @brief Set roll-wise damping used by the flight dynamics model.
             * @param rollwiseDamping Damping coefficient to apply for roll motion.
             */
            virtual void setRollwiseDamping( real_Num rollwiseDamping ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAircraft_h__
