#ifndef _WP_CAircraftFast_h__
#define _WP_CAircraftFast_h__

#include <Workphone/Interface/Vehicle/IAircraft.hpp>
#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <WPVehiclePhysics/CAircraftWing.hpp>
#include <WPVehiclePhysics/CAircraftWingFast.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @brief Fast, lightweight aircraft implementation used by the aerodynamics module.
         *
         * CAircraftFast provides a compact IAircraft implementation optimized for
         * aerodynamics simulations. It stores wings, control surfaces, propulsion
         * units and applies forces/torques to a vehicle body. Thread-safe updates
         * are provided for accumulating forces and torques.
         */
        class WPVehiclePhysics_API CAircraftFast : public IAircraft
        {
        public:
            /**
             * @brief Construct a new CAircraftFast.
             *
             * Initializes internal arrays and default parameters.
             */
            CAircraftFast();
            ~CAircraftFast() override;

            /**
             * @brief Returns true if the aircraft has been properly initialized and is usable.
             * @return true when valid, false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Load aircraft configuration from a serialized string.
             * @param data Serialized configuration (format depends on model loader).
             */
            void load( const String &data );

            /**
             * @brief Reload aircraft using raw data pointer (internal use).
             * @param pData Pointer to raw data block.
             */
            void reload( void *pData );

            /**
             * @brief Unload shared object data previously provided to the aircraft.
             * @param data Shared object to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Advance internal simulation state.
             * @param t Absolute simulation time.
             * @param dt Time step to integrate.
             */
            void update( const double &t, const double &dt );

            /**
             * @brief Get the last computed aerodynamic drag vector (in local/body frame).
             * @return Drag vector.
             */
            Vector3<real_Num> getDrag() const;

            /**
             * @brief Recompute and update aerodynamic drag for the current state.
             * @param t Absolute simulation time.
             * @param dt Time step used to compute drag (for time-dependent effects).
             */
            void updateDrag( const double &t, const double &dt );

            /**
             * @brief Get the associated rigid body representing the vehicle.
             * @return SmartPtr to IVehicleBody or nullptr if not set.
             */
            SmartPtr<IVehicleBody> getBody() const override;

            /**
             * @brief Assign the vehicle body used for force/torque application.
             * @param body SmartPtr to IVehicleBody instance.
             */
            void setBody( SmartPtr<IVehicleBody> body ) override;

            /**
             * @brief Get the world-space transform of the aircraft.
             * @return World transform.
             */
            Transform3<real_Num> getWorldTransform() const override;

            /**
             * @brief Set the world-space transform of the aircraft.
             * @param worldTransform New world transform.
             */
            void setWorldTransform( const Transform3<real_Num> &worldTransform ) override;

            /**
             * @brief Get the local transform of the aircraft (relative to body transform).
             * @return Local transform.
             */
            Transform3<real_Num> getLocalTransform() const override;

            /**
             * @brief Set the local transform of the aircraft.
             * @param localTransform New local transform.
             */
            void setLocalTransform( const Transform3<real_Num> &localTransform ) override;

            /**
             * @brief Read a control channel value.
             * @param idx Channel index (0..7).
             * @return Channel value in normalized range (implementation-defined).
             */
            f32 getChannel( s32 idx ) const override;

            /**
             * @brief Set a control channel value.
             * @param idx Channel index.
             * @param channel New channel value.
             */
            void setChannel( s32 idx, f32 channel ) override;

            /**
             * @brief Add a world-space force at a location on the specified body.
             * @param Bdy Body index or identifier (implementation-specific).
             * @param Force Force vector in world coordinates.
             * @param Loc Application point in world coordinates.
             */
            void addForce( int Bdy, const Vector3<real_Num> &Force,
                           const Vector3<real_Num> &Loc ) override;

            /**
             * @brief Add a world-space torque to the specified body.
             * @param Bdy Body index or identifier.
             * @param Torque Torque vector in world coordinates.
             */
            void addTorque( int Bdy, const Vector3<real_Num> &Torque ) override;

            /**
             * @brief Add a force defined in the aircraft's local frame.
             * @param Bdy Body index.
             * @param Force Force vector in local coordinates.
             * @param Loc Location (local) where force is applied.
             */
            void addLocalForce( int Bdy, const Vector3<real_Num> &Force,
                                const Vector3<real_Num> &Loc ) override;

            /**
             * @brief Add a torque defined in the aircraft's local frame.
             * @param Bdy Body index.
             * @param Torque Torque vector in local coordinates.
             */
            void addLocalTorque( int Bdy, const Vector3<real_Num> &Torque ) override;

            /**
             * @brief Get roll-wise damping coefficient used by the simulator.
             * @return Roll damping coefficient.
             */
            real_Num getRollwiseDamping() const override;

            /**
             * @brief Set roll-wise damping coefficient.
             * @param rollwiseDamping New damping value.
             */
            void setRollwiseDamping( real_Num rollwiseDamping ) override;

            /**
             * @brief Get current angular velocity in world coordinates.
             * @return Angular velocity vector (rad/s).
             */
            Vector3<real_Num> getAngularVelocity() override;

            /**
             * @brief Get current linear velocity in world coordinates.
             * @return Linear velocity vector.
             */
            Vector3<real_Num> getLinearVelocity() override;

            /**
             * @brief Get current angular velocity in aircraft-local coordinates.
             * @return Local angular velocity vector.
             */
            Vector3<real_Num> getLocalAngularVelocity() override;

            /**
             * @brief Get current linear velocity in aircraft-local coordinates.
             * @return Local linear velocity vector.
             */
            Vector3<real_Num> getLocalLinearVelocity() override;

            /**
             * @brief Get aircraft position (non-const variant).
             * @note Prefer the const overload for read-only access.
             * @return Position vector.
             */
            Vector3<real_Num> getPosition();

            /**
             * @brief Get aircraft orientation as a quaternion (non-const).
             * @return Orientation quaternion.
             */
            Quaternion<real_Num> getOrientation();

            /**
             * @brief Get engine RPM for given engine index.
             * @param idx Engine index.
             * @return Engine RPM (implementation-defined units).
             */
            real_Num getEngineRPM( int idx ) const override;

            /**
             * @brief Get thrust produced by the engine/propeller unit at index.
             * @param idx Index of the thrust-producing unit.
             * @return Thrust scalar (units depend on simulation scale).
             */
            real_Num getThrust( int idx ) const override;

            /**
             * @brief Draw a debug point in world coordinates.
             * @param Bdy Body index.
             * @param id Identifier for persistent debug point (or -1 for transient).
             * @param positon Point position in world coordinates.
             * @param color ARGB color.
             */
            void drawPoint( int Bdy, int id, const Vector3<real_Num> &positon, u32 color ) override;

            /**
             * @brief Draw a debug point in local coordinates.
             * @param Bdy Body index.
             * @param id Identifier for the point.
             * @param positon Point position in local coordinates.
             * @param color ARGB color.
             */
            void drawLocalPoint( int Bdy, int id, const Vector3<real_Num> &positon, u32 color ) override;

            /**
             * @brief Draw a debug vector in local coordinates.
             * @param bodyId Body index.
             * @param start Start point in local coords.
             * @param end End point in local coords.
             * @param colour ARGB color.
             */
            void displayLocalVector( s32 bodyId, const Vector3<real_Num> &start,
                                     const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Draw a debug vector in world coordinates and optionally identify it.
             * @param bodyId Body index.
             * @param id Identifier for the vector (for updates/removal).
             * @param start Start world point.
             * @param end End world point.
             * @param colour ARGB color.
             */
            void displayVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Draw a debug vector in local coordinates with explicit id.
             * @param bodyId Body index.
             * @param id Identifier for the vector.
             * @param start Start local point.
             * @param end End local point.
             * @param colour ARGB color.
             */
            void displayLocalVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                     const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Get aircraft callback interface.
             * @return SmartPtr to IAircraftCallback or nullptr if none set.
             */
            SmartPtr<IAircraftCallback> getCallback() const override;

            /**
             * @brief Set aircraft callback interface for notifications or queries.
             * @param callback Callback instance.
             */
            void setCallback( SmartPtr<IAircraftCallback> callback ) override;

            /**
             * @brief Get body transform (transform applied to physics body).
             * @return Body transform.
             */
            Transform3<real_Num> getBodyTransform() const override;

            /**
             * @brief Set body transform.
             * @param bodyTransform New body transform.
             */
            void setBodyTransform( Transform3<real_Num> bodyTransform ) override;

            /**
             * @brief Set the requested control surface angle.
             * @param id Control surface identifier.
             * @param angle Angle in radians (or degrees depending on system convention).
             */
            void setControlAngle( int id, float angle ) override;

            /**
             * @brief Get velocity of a point attached to the aircraft (in world coordinates).
             * @param p Point position in local or body coordinates (implementation-defined).
             * @return Velocity vector of point p.
             */
            Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) override;

            /**
             * @brief Get the air density currently used for aerodynamic calculations.
             * @return Air density in kg/m^3 (default ~1.225).
             */
            real_Num getAirDensity() const override;

            /**
             * @brief Set the air density used by aerodynamic computations.
             * @param airDensity New air density (kg/m^3).
             */
            void setAirDensity( real_Num airDensity ) override;

            /**
             * @brief Access non-const collection of power channel Properties.
             * @return Reference to the array of power channel Properties.
             */
            Array<SmartPtr<Properties>> &getPowerChannels();

            /**
             * @brief Access const collection of power channel Properties.
             * @return Const reference to power channels.
             */
            const Array<SmartPtr<Properties>> &getPowerChannels() const;

            /**
             * @brief Replace power channel Properties collection.
             * @param powerChannels New array of Properties for power channels.
             */
            void setPowerChannels( Array<SmartPtr<Properties>> powerChannels );

            /**
             * @brief Whether the aircraft simulates battery behavior (voltage sag, etc).
             * @return true if battery emulation is enabled.
             */
            bool getEmulateBattery() const override;

            /**
             * @brief Enable or disable battery emulation.
             * @param emulateBattery true to emulate battery, false to disable.
             */
            void setEmulateBattery( bool emulateBattery ) override;

            /**
             * @brief Query whether powertrain is electric.
             * @return true if electric (motors/ESCs) is used.
             */
            bool isElectric() const override;

            /**
             * @brief Set whether the aircraft uses electric powertrain components.
             * @param bIsElectric true for electric systems.
             */
            void setElectric( bool bIsElectric ) override;

            /**
             * @brief Whether power units (motors/propellers) are enabled.
             * @return true if power units are enabled.
             */
            bool getEnablePowerUnit() const override;

            /**
             * @brief Enable/disable power unit processing.
             * @param enablePowerUnits true to enable power units.
             */
            void setEnablePowerUnit( bool enablePowerUnit ) override;

            /**
             * @brief Get the configured battery pack.
             * @return SmartPtr to IBatteryPack or nullptr.
             */
            SmartPtr<IBatteryPack> getBatteryPack() const override;

            /**
             * @brief Set the battery pack used for power simulation.
             * @param batteryPack Battery pack instance.
             */
            void setBatteryPack( SmartPtr<IBatteryPack> batteryPack ) override;

            /**
             * @brief Get wind model used by the aerodynamics simulation.
             * @return SmartPtr to IAerodymanicsWind or nullptr if none.
             */
            SmartPtr<IAerodymanicsWind> getWind() const override;

            /**
             * @brief Set wind model for aerodynamic calculations.
             * @param wind Wind model instance.
             */
            void setWind( SmartPtr<IAerodymanicsWind> wind ) override;

            /**
             * @brief Add a propeller unit to the aircraft.
             * @param propellerUnit Propeller unit to add.
             */
            void addPropellerUnit( SmartPtr<IAircraftPropellerUnit> propellerUnit ) override;

            /**
             * @brief Remove a propeller unit from the aircraft.
             * @param propellerUnit Propeller unit to remove.
             */
            void removePropellerUnit( SmartPtr<IAircraftPropellerUnit> propellerUnit ) override;

            /**
             * @brief Get the list of propeller units.
             * @return Array of propeller unit smart pointers.
             */
            Array<SmartPtr<IAircraftPropellerUnit>> getPropellerUnits() const override;

            /**
             * @brief Replace the current set of propeller units.
             * @param propellerUnits New array of propeller units.
             */
            void setPropellerUnits(
                const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits ) override;

            /**
             * @brief Attach a wheel component to the aircraft.
             * @param wheel Wheel component to add.
             */
            void addWheel( SmartPtr<IWheelComponent> wheel ) override;

            /**
             * @brief Remove a wheel component from the aircraft.
             * @param wheel Wheel component to remove.
             */
            void removeWheel( SmartPtr<IWheelComponent> wheel ) override;

            /**
             * @brief Get wheel components attached to the aircraft.
             * @return Array of wheel components.
             */
            Array<SmartPtr<IWheelComponent>> getWheels() const override;

            /**
             * @brief Replace wheel components collection.
             * @param wheels New array of wheels.
             */
            void setWheels( const Array<SmartPtr<IWheelComponent>> &wheels ) override;

            /**
             * @brief Get aircraft position (const overload).
             * @return Position vector in world coordinates.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Set aircraft position in world coordinates.
             * @param position New world position.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Whether the aircraft is currently controlled by a user input source.
             * @return true if user-controlled.
             */
            bool isUserControlled() const override;

            /**
             * @brief Mark the aircraft as user-controlled or autonomous.
             * @param userControl true for user control.
             */
            void setUserControlled( bool userControlled ) override;

            /**
             * @brief Get current mass used by physics computations.
             * @return Mass (kg or project units).
             */
            real_Num getMass() const override;

            /**
             * @brief Set mass used by physics computations.
             * @param mass New mass value.
             */
            void setMass( real_Num mass ) override;

            /**
             * @brief Whether debug drawing/display of internal data is enabled.
             * @return true if debug display is enabled.
             */
            bool getDisplayDebugData() const override;

            /**
             * @brief Enable or disable debug display of aerodynamic/internal data.
             * @param enableDebugDisplay true to enable debug display.
             */
            void setDisplayDebugData( bool displayDebugData ) override;

            /**
             * @brief Get aircraft center of gravity in local coordinates.
             * @return Center of gravity vector.
             */
            Vector3<real_Num> getCG() const override;

            /**
             * @brief Section multiplier scales sectional aerodynamic forces/areas.
             * @return Section multiplier.
             */
            real_Num getSectionMultiplier() const override;

            /**
             * @brief Set section multiplier used for scaling wing sections.
             * @param sectionMultiplier Multiplier value.
             */
            void setSectionMultiplier( real_Num sectionMultiplier ) override;

            /**
             * @brief Path to the model data file used to build this aircraft.
             * @return File path string.
             */
            String getModelDataFilePath() const override;

            /**
             * @brief Set the path to the model data file.
             * @param filePath File path.
             */
            void setModelDataFilePath( const String &modelDataFilePath ) override;

        private:
            /**
             * @brief Collect and update all transmitter (TX) related data from channels.
             *
             * Internal helper that converts channel values into control angles, throttle,
             * and other derived inputs used by sub-systems.
             */
            void getAllTxData();

            /**
             * @brief Load Properties objects from model data or defaults.
             */
            void loadProperties();

            /**
             * @brief Reset or initialize default values for aircraft parameters.
             */
            void loadDefaults();

            /**
             * @brief Parse aircraft description from a textual representation and configure members.
             * @param data Serialized aircraft description.
             */
            void loadFromString( const String &data );

            /**
             * @brief Update internal world/local transforms from the rigid body.
             *
             * Called by the system when transforms need to be recalculated.
             */
            void updateTransform() override;

            /**
             * @brief Add a force to the internal accumulated force accumulator (body-local).
             * @param force Force in body/local coordinates.
             */
            void addForce( const Vector3<real_Num> &force );

            /**
             * @brief Add a torque to the internal accumulated torque accumulator (body-local).
             * @param torque Torque in body/local coordinates.
             */
            void addTorque( const Vector3<real_Num> &torque );

            /**
             * @brief Clear accumulated forces and torques.
             */
            void clearForces();

            // data::aircraft_wing_data getMirror( const data::aircraft_wing_data &wingData );

            /// File path to the model data used to construct the aircraft.
            String m_modelDataFilePath;

            /// Optional wind model used for aerodynamic calculations.
            SmartPtr<IAerodymanicsWind> m_wind = nullptr;
            /// Properties container for top-level aircraft parameters.
            SmartPtr<Properties> m_properties = nullptr;

            /// Fixed-size storage for wing objects (fast representation).
            std::array<CAircraftWingFast, 20> m_wings;
            /// Actual number of wings in use (<= m_wings.size()).
            int m_numWings = 0;

            Array<SmartPtr<Properties>>              m_powerChannels;
            Array<SmartPtr<IAircraftControlSurface>> m_controlSurfaces;
            Array<SmartPtr<IAircraftPowerUnit>>      m_engines;
            Array<SmartPtr<IESController>>           m_escs;
            Array<SmartPtr<IAircraftPropeller>>      m_propellers;
            Array<SmartPtr<IAircraftPropellerUnit>>  m_propellerUnits;
            Array<SmartPtr<IWheelComponent>>         m_wheels;
            Array<SmartPtr<IAircraftPropWash>>       m_propwashes;

            /// Accumulated force in body-local coordinates (thread-protected).
            Vector3<real_Num> m_force = Vector3<real_Num>::zero();
            /// Accumulated torque in body-local coordinates (thread-protected).
            Vector3<real_Num> m_torque = Vector3<real_Num>::zero();

            /// Last computed aerodynamic drag in body-local coordinates.
            Vector3<real_Num> m_drag = Vector3<real_Num>::zero();
            /// Center of gravity in local coordinates.
            Vector3<real_Num> m_cg = Vector3<real_Num>::zero();

            /// Global multiplier applied to section-level forces/areas.
            real_Num m_sectionMultiplier = static_cast<real_Num>( 1.0 );

            /// Damping applied about roll axis.
            real_Num m_rollwiseDamping = static_cast<real_Num>( 1.0 );
            /// Air density used for aerodynamic calculations (kg/m^3).
            real_Num m_airDensity = static_cast<real_Num>( 1.225 );

            /// Whether to emulate battery behaviour.
            bool m_emulateBattery = true;
            /// Whether the aircraft uses electric propulsion.
            bool m_bIsElectric = true;
            /// If true, show debug drawing and data displays.
            bool m_displayDebugData = false;
            /// Whether processing of power units is enabled.
            bool m_enablePowerUnit = true;

            SmartPtr<IBatteryPack>      m_batteryPack = nullptr;
            SmartPtr<IAircraftCallback> m_callback = nullptr;

            Transform3<real_Num> m_bodyTransform;
            Transform3<real_Num> m_worldTransform;
            Transform3<real_Num> m_localTransform;

            /// Underlying rigid body used for applying physics impulses/forces.
            SmartPtr<IVehicleBody> m_rigidbody = nullptr;

            /// RC / control channels storage.
            FixedArray<f32, 8> m_channels = std::array<f32, 8>( { 0.0f } );

            /// Mutex protecting concurrent access to m_force.
            SpinRWMutex m_forceMutex;
            /// Mutex protecting concurrent access to m_torque.
            SpinRWMutex m_torqueMutex;
        };

        inline Vector3<real_Num> CAircraftFast::getCG() const
        {
            return m_cg;
        }

        /**
         * @brief Convert a vector from canonical frame to an internal "Y-frame".
         *
         * This helper flips and rotates components to match the codebase's expected
         * coordinate ordering used for some calculations and visualization helpers.
         *
         * @param CV Input vector in canonical frame.
         * @return Vector converted to Y-frame.
         */
        inline Vector3<real_Num> vecToYFrame( const Vector3<real_Num> &CV )
        {
            // WP_ASSERT(CV.isFinite());
            return Vector3<real_Num>( -CV.Y(), -CV.Z(), CV.X() );
        }

        inline bool CAircraftFast::getDisplayDebugData() const
        {
            return m_displayDebugData;
        }
    } // namespace vehicle
} // namespace workphone

#endif // Aircraft_h__
