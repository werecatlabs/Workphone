/**
 * @file CAircraft.hpp
 * @brief High-level aircraft controller and container for aerodynamic components.
 *
 * This header defines the `workphone::CAircraft` class, an implementation of the
 * `IAircraft` interface that composes wings, engines, propellers, control surfaces,
 * wheels and related subsystems. It is used by the aerodynamics subsystem to
 * simulate and update aircraft state, forces and power systems.
 *
 * The class inherits from `CAeroVehicleController<IAircraft>` and provides
 * accessors and mutators for transforms, mass, power units, battery emulation,
 * air density, and callback hooks used by higher-level systems.
 */

#ifndef _WP_CAircraft_h__
#define _WP_CAircraft_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IAircraft.hpp>
#include <WPVehiclePhysics/CAerodynamicsVehicle.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <Workphone/Core/Array.hpp>

#if defined WP_PLATFORM_WIN32
#    include <Windows.h>
#elif defined WP_PLATFORM_LINUX
// #include <X11/Xlib.hpp>
// #include <iostream>
// #include "X11/keysym.hpp"
#endif

namespace workphone::vehicle
{
    /**
         * @class CAircraft
         * @brief Concrete aircraft controller that aggregates aerodynamic subsystems.
         *
         * CAircraft manages components such as wings, engines, propellers, power channels,
         * control surfaces and wheels. It exposes an API to update simulation state,
         * query dynamic values (thrust, RPM, drag), and configure aircraft-level settings
         * (mass, air density, battery emulation, etc.).
         */
    class WPVehiclePhysics_API CAircraft : public CAerodynamicsVehicle<IAircraft>
    {
    public:
        /// Standard RC channel indices for common controls.
        static constexpr int m_thrChannel = 0; ///< Throttle channel index
        static constexpr int m_ailChannel = 1; ///< Aileron (roll) channel index
        static constexpr int m_eleChannel = 2; ///< Elevator (pitch) channel index
        static constexpr int m_yawChannel = 3; ///< Rudder (yaw) channel index
        static constexpr int m_gearChannel = 4; ///< Landing gear channel index
        static constexpr int m_colChannel = 5; ///< Collective / auxiliary channel index
        static constexpr int m_aux1Channel = 6; ///< Auxiliary channel 1
        static constexpr int m_aux2Channel = 7; ///< Auxiliary channel 2

        /// Keyboard key codes used for simple input (platform dependent).
        const short unsigned int m_keyLeft = 37; ///< Left arrow key code
        const short unsigned int m_keyTop = 38; ///< Up arrow key code
        const short unsigned int m_keyRight = 39; ///< Right arrow key code
        const short unsigned int m_keyDown = 40; ///< Down arrow key code
        const short unsigned int m_keyExit = 81; ///< Exit key (Q) code

        CAircraft();
        ~CAircraft() override;

        /**
             * @brief Initializes the default aerodynamic body and environment.
             */
        void load(SmartPtr<ISharedObject> data) override;

        /**
             * @brief Check if the aircraft object is in a valid (usable) state.
             * @return true if valid, false otherwise.
             */
        bool isValid() const override;

        /**
             * @brief Load aircraft configuration from a textual data block.
             * @param data String containing serialized aircraft configuration.
             */
        void loadFromData(const String &data);

        /**
             * @brief Reload internal data from a raw pointer (implementation specific).
             * @param pData Pointer to data used to reinitialize the aircraft.
             */
        void reloadFromData(void *pData);

        /**
             * @brief Unload resources associated with a shared object.
             * @param data Shared object pointer to unload.
             */
        void unload(SmartPtr<ISharedObject> data) override;

        /**
             * @brief Advances aerodynamics using the application timer.
             *
             * This override is the entry point used by IVehicleManager.
             */
        void update() override;

        /**
             * @brief Advance the aircraft simulation one step.
             * @param t Absolute simulation time.
             * @param dt Time step duration.
             */
        void update(const double &t, const double &dt);

        /**
             * @brief Get the aerodynamic drag vector currently applied to the aircraft.
             * @return Drag vector in aircraft-local coordinates.
             */
        Vector3<real_Num> getDrag() const;

        /**
             * @brief Recompute aerodynamic drag for the current state.
             * @param t Absolute simulation time.
             * @param dt Time step duration.
             */
        void updateDrag(const double &t, const double &dt);

        /**
             * @brief Get the rigid body representing the aircraft's physical body.
             * @return Pointer to the IVehicleBody instance.
             */
        IVehicleBody *getBodyPtr() const override;

        /**
             * @brief Get the vehicle body object (collision/physics representation).
             * @return Smart pointer to the IVehicleBody instance.
             */
        SmartPtr<IVehicleBody> getBody() const override;

        /**
             * @brief Attach a vehicle body to the aircraft.
             * @param body Smart pointer to an IVehicleBody implementation.
             */
        void setBody(SmartPtr<IVehicleBody> body) override;

        /**
             * @brief Retrieve the aircraft world transform.
             * @return Transform from local aircraft-space to world-space.
             */
        Transform3<real_Num> getWorldTransform() const override;

        /**
             * @brief Set the aircraft world transform.
             * @param worldTransform Transform describing the aircraft pose in world-space.
             */
        void setWorldTransform(const Transform3<real_Num> &worldTransform) override;

        /**
             * @brief Retrieve the aircraft local (parent) transform.
             * @return Local transform applied to the aircraft (relative frame).
             */
        Transform3<real_Num> getLocalTransform() const override;

        /**
             * @brief Set the aircraft local transform.
             * @param localTransform Local transform to apply.
             */
        void setLocalTransform(const Transform3<real_Num> &localTransform) override;

        /**
             * @brief Get damping factor applied around the roll axis.
             * @return Rollwise damping coefficient.
             */
        real_Num getRollwiseDamping() const override;

        /**
             * @brief Set damping factor applied around the roll axis.
             * @param rollwiseDamping Damping coefficient.
             */
        void setRollwiseDamping(real_Num rollwiseDamping) override;

        /**
             * @brief Get engine RPM for a given engine index.
             * @param idx Engine index (0..n-1).
             * @return Engine RPM value.
             */
        real_Num getEngineRPM(int idx) const override;

        /**
             * @brief Get thrust produced by a given engine index.
             * @param idx Engine index (0..n-1).
             * @return Thrust for the engine.
             */
        real_Num getThrust(int idx) const override;

        /**
             * @brief Get the aircraft callback interface.
             * @return Smart pointer to the callback object.
             */
        SmartPtr<IAircraftCallback> getCallback() const override;

        /**
             * @brief Set a callback interface to receive aircraft events.
             * @param callback Smart pointer to the callback implementation.
             */
        void setCallback(SmartPtr<IAircraftCallback> callback) override;

        /**
             * @brief Get transform of the physical body relative to the aircraft.
             * @return Body transform in aircraft-local coordinates.
             */
        Transform3<real_Num> getBodyTransform() const override;

        /**
             * @brief Set transform of the physical body relative to the aircraft.
             * @param bodyTransform Transform to apply to the body.
             */
        void setBodyTransform(Transform3<real_Num> bodyTransform) override;

        /**
             * @brief Set a control surface angle (e.g. aileron, elevator).
             * @param id Identifier of the control surface.
             * @param angle Desired control angle in degrees (or implementation units).
             */
        void setControlAngle(int id, float angle) override;

        /**
             * @brief Get ambient air density used for aerodynamic calculations.
             * @return Air density in kg/m^3.
             */
        real_Num getAirDensity() const override;

        /**
             * @brief Set ambient air density used by the aircraft.
             * @param airDensity Air density in kg/m^3.
             */
        void setAirDensity(real_Num airDensity) override;

        /**
             * @brief Access power channel property containers for motors/ESCs.
             * @return Reference to the array of power channel properties.
             */
        Array<SmartPtr<Properties>> &getPowerChannels();

        /**
             * @brief Const access to power channel properties.
             */
        const Array<SmartPtr<Properties>> &getPowerChannels() const;

        /**
             * @brief Replace the current power channel configuration.
             * @param powerChannels New array of power channel property objects.
             */
        void setPowerChannels(Array<SmartPtr<Properties>> powerChannels);

        /**
             * @brief Query whether battery behavior is emulated (software).
             * @return true if battery emulation is enabled.
             */
        bool getEmulateBattery() const override;

        /**
             * @brief Enable or disable battery emulation.
             * @param emulate true to enable emulation.
             */
        void setEmulateBattery(bool emulate) override;

        /**
             * @brief Check if the aircraft is configured as electric (motors).
             * @return true if electric powertrain is used.
             */
        bool isElectric() const override;

        /**
             * @brief Configure whether the aircraft is electric.
             * @param bIsElectric true to mark as electric.
             */
        void setElectric(bool bIsElectric) override;

        /**
             * @brief Get the battery pack used by the aircraft (if any).
             * @return Smart pointer to IBatteryPack.
             */
        SmartPtr<IBatteryPack> getBatteryPack() const override;

        /**
             * @brief Assign a battery pack to the aircraft.
             * @param batteryPack Smart pointer to the battery pack implementation.
             */
        void setBatteryPack(SmartPtr<IBatteryPack> batteryPack) override;

        /**
             * @brief Query whether the power unit (motors/engines) are enabled.
             * @return true if power units are enabled.
             */
        bool getEnablePowerUnit() const override;

        /**
             * @brief Enable or disable the entire power unit assembly.
             * @param enabled true to enable power units.
             */
        void setEnablePowerUnit(bool enabled) override;

        /**
             * @brief Get the current wind model applied to the aircraft.
             * @return Smart pointer to IAerodymanicsWind instance.
             */
        SmartPtr<IAerodymanicsWind> getWind() const override;

        /**
             * @brief Assign a wind model to the aircraft.
             * @param wind Smart pointer to the wind model implementation.
             */
        void setWind(SmartPtr<IAerodymanicsWind> wind) override;

        /**
             * @brief Add a propeller unit (a grouped propeller + motor assembly).
             * @param propellerUnit Propeller unit to add.
             */
        void addPropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit) override;

        /**
             * @brief Remove a propeller unit from the aircraft.
             * @param propellerUnit Propeller unit to remove.
             */
        void removePropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit) override;

        /**
             * @brief Retrieve current propeller units owned by the aircraft.
             * @return Array of smart pointers to propeller units.
             */
        Array<SmartPtr<IAircraftPropellerUnit>> getPropellerUnits() const override;

        /**
             * @brief Replace propeller units with a new set.
             * @param propellerUnits New array of propeller units.
             */
        void setPropellerUnits(
            const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits) override;

        /**
             * @brief Add a wheel component (landing gear).
             * @param wheel Wheel component to add.
             */
        void addWheel(SmartPtr<IWheelComponent> wheel) override;

        /**
             * @brief Remove a wheel component (landing gear).
             * @param wheel Wheel component to remove.
             */
        void removeWheel(SmartPtr<IWheelComponent> wheel) override;

        /**
             * @brief Get the currently attached wheels.
             * @return Array of wheel components.
             */
        Array<SmartPtr<IWheelComponent>> getWheels() const override;

        /**
             * @brief Replace the wheel set with the provided array.
             * @param wheels New wheel components array.
             */
        void setWheels(const Array<SmartPtr<IWheelComponent>> &wheels) override;

        /**
             * @brief Query whether the aircraft is under user control.
             * @return true if user control is active.
             */
        bool isUserControlled() const override;

        /**
             * @brief Mark the aircraft as user-controlled or autonomous.
             * @param userControlled true for user control.
             */
        void setUserControlled(bool userControlled) override;

        /**
             * @brief Get aircraft mass used in dynamics calculations.
             * @return Mass in kilograms.
             */
        real_Num getMass() const override;

        /**
             * @brief Set the aircraft mass.
             * @param mass Mass in kilograms.
             */
        void setMass(real_Num mass) override;

        /**
             * @brief Get the section multiplier used for wing/section scaling.
             * @return Section multiplier factor.
             */
        real_Num getSectionMultiplier() const override;

        /**
             * @brief Set the section multiplier used when computing wing forces.
             * @param sectionMultiplier Multiplier factor.
             */
        void setSectionMultiplier(real_Num sectionMultiplier) override;

        /**
             * @brief Get the file path to the aircraft model data (if any).
             * @return Path to the model data file.
             */
        String getModelDataFilePath() const override;

        /**
             * @brief Set the model data file path for this aircraft.
             * @param filePath Path to model data.
             */
        void setModelDataFilePath(const String &filePath) override;

        /**
             * @brief Reset the aircraft to its default state.
             *
             * This clears transient simulation state and restores defaults loaded
             * from properties / model data.
             */
        void reset() override;

    private:
        /**
             * @brief Supplies stable lift, thrust and control torques when a model
             *        has not provided detailed wing/propeller sections.
             */
        void updateDefaultAerodynamics(real_Num dt);

        /**
             * @brief Helper to retrieve keyboard input / key code for simple controls.
             * @return Integer key code read from platform-specific input.
             */
        int getKey();

        /**
             * @brief Collect RC transmitter channel data into internal structures.
             *
             * Implementation-specific helper that polls or reads all channel inputs.
             */
        void getAllTxData();

        /**
             * @brief Load aircraft properties from configured source (internal helper).
             */
        void loadProperties();

        /**
             * @brief Populate default values for aircraft properties when none are provided.
             */
        void loadDefaults();

        /**
             * @brief Parse aircraft configuration from a string representation.
             * @param data Serialized configuration string.
             */
        void loadFromString(const String &data);

        /**
             * @brief Update derived transforms for subsystems (overrides base).
             *
             * Called during transform changes to propagate new transforms to attached
             * components (wings, engines, propellers, etc.).
             */
        void updateTransform() override;

        /**
             * @brief Return a mirrored copy of wing properties (used for symmetric wings).
             * @param wingData Source wing properties to mirror.
             * @return Smart pointer to mirrored properties.
             */
        SmartPtr<Properties> getMirror(SmartPtr<Properties> wingData);

        // Model data file path (if loaded from disk).
        String m_modelDataFilePath;

        // Wind model currently applied to the aircraft (optional).
        SmartPtr<IAerodymanicsWind> m_wind = nullptr;

        // Primary property container for the aircraft (parsed from data).
        SmartPtr<Properties> m_properties = nullptr;

        // Collections of subsystems and property containers.
        Array<SmartPtr<Properties>> m_powerChannels; ///< Motor / ESC channel properties
        Array<SmartPtr<IAircraftControlSurface>>
        m_controlSurfaces; ///< Control surfaces (ailerons, elevators, rudder)
        Array<SmartPtr<IAircraftPowerUnit>> m_engines; ///< Engine / motor units
        Array<SmartPtr<IAircraftWing>> m_wings; ///< Wing definitions and sections
        Array<SmartPtr<IESController>> m_escs; ///< Electronic speed controllers
        Array<SmartPtr<IAircraftPropeller>> m_propellers; ///< Propeller definitions
        Array<SmartPtr<IAircraftPropellerUnit>> m_propellerUnits; ///< Propeller+motor groupings
        Array<SmartPtr<IWheelComponent>> m_wheels; ///< Landing gear wheel components
        Array<SmartPtr<IAircraftPropWash>> m_propwashes; ///< Propwash effect objects

        // Scaling factor applied to wing sections (defaults to 1.0).
        real_Num m_sectionMultiplier = 1.0;

        // Damping and environment parameters.
        real_Num m_rollwiseDamping = 1.0; ///< Roll axis damping
        real_Num m_airDensity = 1.225; ///< Default air density (kg/m^3)

        // Power and simulation flags.
        bool m_emulateBattery = true; ///< Whether battery behavior is emulated
        bool m_bIsElectric = true; ///< True if electric powertrain
        bool m_displayDebugData = false; ///< Debug display toggle
        bool m_enablePowerUnit = true; ///< Master enable for power units

        // Battery pack (if used).
        SmartPtr<IBatteryPack> m_batteryPack = nullptr;

        // Raw aircraft property container loaded from model file or string.
        SmartPtr<Properties> m_aircraftData;
    };

    /**
         * @brief Convert a vector from the internal coordinate frame to the Y-up frame used elsewhere.
         *
         * The conversion performs: (X,Y,Z) -> (-Y, -Z, X) in terms of the input Vector3.
         *
         * @param CV Vector in current internal frame.
         * @return Vector in Y-up frame.
         */
    inline Vector3<real_Num> vecToYFrame(const Vector3<real_Num> &CV)
    {
        return Vector3<real_Num>(-CV.Y(), -CV.Z(), CV.X());
    }

    /**
         * @brief Convert a vector from the Y-up frame back to the internal coordinate frame.
         *
         * The conversion performs: (X,Y,Z) -> (Z, -X, -Y) relative to the input Vector3.
         *
         * @param SV Vector in Y-up frame.
         * @return Vector in internal frame.
         */
    inline Vector3<real_Num> vecFromYFrame(const Vector3<real_Num> &SV)
    {
        return Vector3<real_Num>(SV.Z(), -SV.X(), -SV.Y());
    }
}

#endif // Aircraft_h__
