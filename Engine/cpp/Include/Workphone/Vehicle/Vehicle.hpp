#ifndef __VehicleController_h__
#define __VehicleController_h__

#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @file Vehicle.hpp
         * @brief Concrete physics vehicle controller implementing IVehicle.
         *
         * Provides a physics-driven vehicle implementation used by the engine.
         * The class coordinates the vehicle chassis body, wheels, drivetrain and
         * exposes control channels, force/torque application and debug drawing.
         *
         * Thread-safety:
         * - Some members are explicitly thread-safe (atomic wrappers). Other
         *   operations must be called from the simulation/update thread.
         *
         * Usage:
         * - Construct the object, call `load()` with configuration data, then
         *   call `update()` each simulation frame. Call `unload()` when done.
         *
         * @see IVehicle
         *
         * @author Workphone Development Team
         * @version 1.0
         * @since Engine v1.0
         */
        class WPCore_API Vehicle : public IVehicle
        {
        public:
            static const hash_type modelchanged;

            /**
             * @brief Construct a Vehicle with default, unloaded state.
             *
             * The object is usable after calling `load()`; many methods return
             * default/zero values until the vehicle is loaded.
             */
            Vehicle();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper release of owned components and integration points
             * with the physics system.
             */
            ~Vehicle() override;

            /**
             * @brief Initialize and configure the vehicle from `data`.
             *
             * This method is responsible for:
             * - Creating or attaching the physics body for the chassis.
             * - Creating wheel controllers and configuring suspension/tires.
             * - Initializing control channels and drivetrain.
             *
             * @param data Smart pointer to an ISharedObject that contains
             *             vehicle configuration (JSON, XML or engine-specific data).
             * @note Must be called before using the vehicle in simulation.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Release resources and detach the vehicle from the physics world.
             *
             * After this call the vehicle returns to an unloaded state. Implementation
             * should safely release pointers to the physics body, wheels and callbacks.
             *
             * @param data Optional context/provenance data used during unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reset the vehicle to its initial runtime state.
             *
             * Typically resets transforms, clears accumulated forces/torques,
             * and returns wheels/drivetrain to default state while keeping the
             * vehicle loaded.
             */
            void reset() override;

            /**
             * @brief Perform per-frame simulation updates for the vehicle.
             *
             * Responsibilities include:
             * - Applying accumulated forces/torques to the physics body.
             * - Updating wheel states and suspension callbacks.
             * - Processing channel inputs (steer/throttle/brake).
             * - Emitting debug visualization if enabled.
             *
             * Call once per simulation frame from the physics/update loop.
             */
            void update() override;

            /**
             * @brief Recalculate and apply transforms for the vehicle.
             *
             * This updates internal world/local transforms from the underlying
             * physics body and applies any transform overrides requested by the user.
             */
            void updateTransform() override;

            /**
             * @brief Read a control channel value.
             *
             * Control channels map to inputs such as steering, throttle, brake, gear,
             * handbrake and other analog/digital inputs. Channel semantics depend on
             * the vehicle configuration.
             *
             * @param idx Channel index (valid range depends on vehicle; typical
             *            implementations use 0..11).
             * @return Normalized channel value. Conventionally -1.0 -> 1.0 for
             *         analog channels. Returns 0.0 when index is out of range.
             */
            f32 getChannel( s32 idx ) const override;

            /**
             * @brief Set a control channel value.
             *
             * Value will take effect on the next `update()` call. Index must be
             * within the configured channel array bounds.
             *
             * @param idx Index of the channel to set.
             * @param channel New value for the channel (typically -1.0..1.0).
             */
            void setChannel( s32 idx, f32 channel ) override;

            /**
             * @brief Get the vehicle scale.
             * @return Scale vector for each axis applied to the vehicle model.
             */
            Vector3<real_Num> getScale() const override;

            /**
             * @brief Get the vehicle's world position.
             * @return Position of the vehicle in world coordinates.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Immediately set the vehicle's world position.
             *
             * This will update the internal transform and the attached physics
             * body (if present). Use sparingly while simulation is running.
             *
             * @param position New position in world-space coordinates.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Get the vehicle's orientation.
             * @return Orientation as a quaternion in world coordinates.
             */
            Quaternion<real_Num> getOrientation() const override;

            /**
             * @brief Immediately set the vehicle's orientation.
             *
             * Updates the internal orientation and applies it to the physics body
             * if present. May introduce discontinuities in physics if called during
             * simulation.
             *
             * @param orientation New orientation as a quaternion.
             */
            void setOrientation( const Quaternion<real_Num> &orientation ) override;

            /**
             * @brief Query whether the vehicle is user-controlled.
             * @return true when user input drives the vehicle, false for AI control.
             */
            bool isUserControlled() const override;

            /**
             * @brief Switch between user and AI control modes.
             *
             * Changing this affects how control channels are processed and which
             * systems (input or AI) may write to those channels.
             *
             * @param userControlled true to enable user input control, false to
             *                       hand control to AI systems.
             */
            void setUserControlled( bool userControlled ) override;

            /**
             * @brief Get the physics mass of the vehicle chassis.
             * @return Mass used by the physics engine (in engine units).
             */
            real_Num getMass() const override;

            /**
             * @brief Set the physics mass of the vehicle chassis.
             *
             * This updates the mass used by the physics body and influences
             * acceleration, inertia and collision response.
             *
             * @param mass New mass value in physics units.
             */
            void setMass( real_Num mass ) override;

            /**
             * @brief Get the vehicle's physics body component.
             * @return Smart pointer to an IVehicleBody implementation, or nullptr.
             */
            IVehicleBody *getBodyPtr() const override;

            /**
             * @brief Get the vehicle's physics body component.
             * @return Smart pointer to an IVehicleBody implementation, or nullptr.
             */
            SmartPtr<IVehicleBody> getBody() const override;

            /**
             * @brief Attach a physics body to the vehicle.
             *
             * The IVehicleBody represents the chassis physics object and is used
             * for applying forces/torques and querying kinematic state.
             *
             * @param body Smart pointer to an IVehicleBody to attach.
             */
            void setBody( SmartPtr<IVehicleBody> body ) override;

            /**
             * @brief Get world transform (position, rotation, scale).
             * @return Full world transform for the vehicle.
             */
            Transform3<real_Num> getWorldTransform() const override;

            /**
             * @brief Set the vehicle's world transform.
             *
             * Replaces the current world transform. Applied to physics body if
             * present. Use with caution while simulation is active.
             *
             * @param transform New world transform.
             */
            void setWorldTransform( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Get local transform relative to parent.
             * @return Local transform matrix.
             */
            Transform3<real_Num> getLocalTransform() const override;

            /**
             * @brief Set local transform relative to parent.
             * @param transform New local transform matrix.
             */
            void setLocalTransform( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Draw a persistent debug point in world coordinates.
             *
             * Useful for marking contact points, anchor positions or diagnostic
             * locations. Points are identified by `id` and `body` so they can be
             * updated or removed by the renderer/debug system.
             *
             * @param body Body index the point is associated with (implementation-specific).
             * @param id   Unique identifier for this debug point.
             * @param positon World-space position of the point.
             * @param color RGBA color in packed u32 format.
             */
            void drawPoint( s32 body, int id, const Vector3<real_Num> &positon, u32 color ) override;

            /**
             * @brief Draw a debug point using the vehicle's local coordinate space.
             *
             * The point is transformed to world-space before being rendered.
             *
             * @param body Body index the point is associated with (implementation-specific).
             * @param id   Unique identifier for this debug point.
             * @param positon Local position of the point.
             * @param color RGBA color in packed u32 format.
             */
            void drawLocalPoint( s32 body, int id, const Vector3<real_Num> &positon,
                                 u32 color ) override;

            /**
             * @brief Draw a vector in local coordinates.
             *
             * Overload without id. Vector is transformed to world-space and displayed.
             *
             * @param bodyId Body index the vector is associated with.
             * @param start Start position in local coordinates.
             * @param end   End position in local coordinates.
             * @param colour RGBA color in packed u32 format.
             */
            void displayLocalVector( s32 bodyId, const Vector3<real_Num> &start,
                                     const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Draw a vector in world coordinates.
             *
             * @param bodyId Body index the vector is associated with.
             * @param id     Unique identifier for the debug vector.
             * @param start  Start position in world coordinates.
             * @param end    End position in world coordinates.
             * @param colour RGBA color in packed u32 format.
             */
            void displayVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Draw a vector in local coordinates with an id.
             *
             * @param bodyId Body index the vector is associated with.
             * @param id     Unique identifier for the debug vector.
             * @param start  Start position in local coordinates.
             * @param end    End position in local coordinates.
             * @param colour RGBA color in packed u32 format.
             */
            void displayLocalVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                     const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Query whether debug visualization is enabled.
             * @return true if debug drawing is active for this vehicle.
             */
            bool getDisplayDebugData() const override;

            /**
             * @brief Enable or disable debug visualization for the vehicle.
             *
             * When enabled, diagnostic visuals such as contact points, forces,
             * and suspension rays are submitted to the debug renderer.
             *
             * @param enabled true to enable debug drawing.
             */
            void setDisplayDebugData( bool enabled ) override;

            /**
             * @brief Queue a world-space force to be applied to a vehicle body.
             *
             * The force is applied at the next physics update. If `bodyIdx` refers
             * to the chassis body the force and application point will affect both
             * linear and angular motion.
             *
             * @param bodyIdx Index of the body (implementation-specific).
             * @param force Force vector expressed in world coordinates (Newtons).
             * @param loc World-space point where the force is applied.
             */
            void addForce( s32 bodyIdx, const Vector3<real_Num> &force,
                           const Vector3<real_Num> &loc ) override;

            /**
             * @brief Queue a world-space torque to be applied to a vehicle body.
             *
             * @param bodyIdx Index of the body (implementation-specific).
             * @param Torque Torque vector in world coordinates (N·m).
             */
            void addTorque( s32 bodyIdx, const Vector3<real_Num> &torque ) override;

            /**
             * @brief Apply a local-space force to a body.
             *
             * The force and location are transformed to world-space internally.
             *
             * @param bodyIdx Index of the body (implementation-specific).
             * @param Force Force vector in local vehicle coordinates.
             * @param loc Local-space application point.
             */
            void addLocalForce( s32 bodyIdx, const Vector3<real_Num> &force,
                                const Vector3<real_Num> &loc ) override;

            /**
             * @brief Apply a local-space torque to a body.
             *
             * @param bodyIdx Index of the body (implementation-specific).
             * @param Torque Torque vector in local coordinates.
             */
            void addLocalTorque( s32 bodyIdx, const Vector3<real_Num> &torque ) override;

            /**
             * @brief Compute velocity at an arbitrary point on the vehicle.
             *
             * Accounts for both linear velocity of the chassis and angular
             * velocity about the center of mass.
             *
             * @param p World-space point to query.
             * @return Instantaneous linear velocity of the point in world-space.
             */
            Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) override;

            /**
             * @brief Get chassis angular velocity in world coordinates.
             * @return Angular velocity vector, radians per second.
             */
            Vector3<real_Num> getAngularVelocity() override;

            /**
             * @brief Get chassis linear velocity in world coordinates.
             * @return Linear velocity vector (units per second).
             */
            Vector3<real_Num> getLinearVelocity() override;

            /**
             * @brief Get angular velocity in vehicle-local coordinates.
             * @return Angular velocity vector expressed in local space.
             */
            Vector3<real_Num> getLocalAngularVelocity() override;

            /**
             * @brief Get linear velocity in vehicle-local coordinates.
             * @return Linear velocity vector expressed in local space.
             */
            Vector3<real_Num> getLocalLinearVelocity() override;

            /**
             * @brief Get a mutable reference to the vehicle callback handler.
             *
             * The callback receives events such as collisions or custom vehicle
             * notifications. Returning a reference allows callers to set or swap
             * the handler.
             *
             * @return Reference to SmartPtr<IVehicleCallback>.
             */
            IVehicleCallback *getVehicleCallbackPtr() const override;

            /**
             * @brief Get const access to the vehicle callback handler.
             * @return Const reference to SmartPtr<IVehicleCallback>.
             */
            SmartPtr<IVehicleCallback> getVehicleCallback() const override;

            /**
             * @brief Replace the vehicle callback handler.
             *
             * Passing nullptr disables callbacks.
             *
             * @param callback New callback implementation wrapped in SmartPtr.
             */
            void setVehicleCallback( SmartPtr<IVehicleCallback> callback ) override;

            /**
             * @brief Get the vehicle center of gravity in local coordinates.
             *
             * Center of gravity influences stability, roll/pitch inertia and
             * suspension load distribution.
             *
             * @return Position of the center of gravity relative to the chassis origin.
             */
            Vector3<real_Num> getCG() const override;

            /**
             * @brief Set the high-level simulation state of the vehicle.
             *
             * Typical states: AWAKE, SLEEPING, DISABLED. This setter is thread-safe.
             *
             * @param state New State value.
             */
            void setState( State state ) override;

            /**
             * @brief Get the current high-level simulation state of the vehicle.
             * @return Current State value.
             * @note This operation is thread-safe.
             */
            State getState() const override;

            /**
             * @brief Access an individual wheel controller.
             *
             * Provides access to per-wheel parameters and direct control such as
             * steering angle, torque and brake settings.
             *
             * @param index Wheel index (0..N-1). Typical cars use 0..3.
             * @return SmartPtr<IWheelComponent> for the wheel or nullptr if index is invalid.
             */
            SmartPtr<IWheelComponent> getWheelController( u32 index ) const override;

            /**
             * @brief Get the drivetrain implementation used by this vehicle.
             * @return Smart pointer to IDriveTrain or nullptr if none set.
             */
            SmartPtr<IDriveTrain> getDriveTrain() const override;

            /**
             * @brief Assign a drivetrain implementation to the vehicle.
             *
             * The drivetrain is responsible for distributing engine/torque to wheels.
             *
             * @param driveTrain SmartPtr to an IDriveTrain implementation.
             */
            void setDriveTrain( SmartPtr<IDriveTrain> driveTrain ) override;

            /**
             * @brief Get configured drive type (FWD/RWD/AWD).
             * @return VehicleDriveType enum value describing the drive layout.
             */
            VehicleDriveType getDriveType() const override;

            /**
             * @brief Set the vehicle drive type (FWD/RWD/AWD).
             * @param driveType Desired VehicleDriveType.
             */
            void setDriveType( VehicleDriveType driveType ) override;

            bool isElectric() const override;

            void setElectric( bool electric ) override;

            bool getEnablePowerUnit() const override;

            void setEnablePowerUnit( bool enabled ) override;

            bool getEmulateBattery() const override;

            void setEmulateBattery( bool emulate ) override;

            /**
             * @brief Reflection / registration macro for engine RTTI systems.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Internal helper: apply a world-space force to the chassis.
             *
             * Used by external addForce overloads to accumulate forces affecting
             * the entire vehicle. Accumulators are cleared at the end of the frame.
             *
             * @param force Force vector in world coordinates (Newtons).
             */
            void addForce( const Vector3<real_Num> &force );

            /**
             * @brief Internal helper: apply a world-space torque to the chassis.
             *
             * @param torque Torque vector in world coordinates (N·m).
             */
            void addTorque( const Vector3<real_Num> &torque );

            /**
             * @brief Clear the per-frame force and torque accumulators.
             *
             * Called during or after `update()` to reset accumulated inputs.
             */
            void clearForces();

            /**
             * @brief Configure which wheels are drive wheels based on drive type.
             *
             * Internal helper called during load or when drive type changes.
             */
            void setupDriveWheels();

            ///< Drive train component for power distribution
            SmartPtr<IDriveTrain> m_driveTrain;

            /** @brief Thread-safe vehicle state (AWAKE, SLEEPING, etc.) */
            AtomicValue<State> m_vehicleState = State::AWAKE;

            /** @brief Chassis transform relative to the vehicle origin. */
            Transform3<real_Num> m_bodyTransform;

            /** @brief Cached world transform for rendering / queries. */
            Transform3<real_Num> m_worldTransform;

            /** @brief Local transform relative to parent or scene node. */
            Transform3<real_Num> m_localTransform;

            /** @brief Thread-safe pointer to the vehicle's physics body (chassis). */
            AtomicSmartPtr<IVehicleBody> m_rigidbody;

            /** @brief Optional event/callback handler for vehicle notifications. */
            SmartPtr<IVehicleCallback> m_callback;

            /** @brief Accumulated world-space force for this simulation frame (Newtons). */
            Vector3<real_Num> m_force = Vector3<real_Num>::zero();

            /** @brief Accumulated world-space torque for this simulation frame (N·m). */
            Vector3<real_Num> m_torque = Vector3<real_Num>::zero();

            /** @brief Optional drag force applied to the vehicle each frame. */
            Vector3<real_Num> m_drag = Vector3<real_Num>::zero();

            /** @brief Center of gravity position expressed in local chassis coordinates. */
            Vector3<real_Num> m_cg = Vector3<real_Num>::zero();

            /** @brief Flag controlling whether debug visualization is emitted. Thread-safe. */
            atomic_bool m_displayDebugData = false;

            /** @brief Configured drivetrain layout (AllWheelDrive, FrontWheelDrive, etc.). */
            VehicleDriveType m_driveType = VehicleDriveType::AllWheelDrive;

            /** @brief Control channel array (e.g. steering, throttle, brake). */
            Array<f32> m_channels;

            /** @brief Array of wheel controllers; size depends on vehicle (commonly 4). */
            Array<SmartPtr<IWheelComponent>> m_wheels;

            /** @brief True when the vehicle is controlled by user input, false for AI. */
            atomic_bool m_userControlled = false;

            /** @brief True when the vehicle uses an electric powertrain. */
            atomic_bool m_electric = false;

            /** @brief True when the power unit (engine/motor) is enabled. */
            atomic_bool m_enablePowerUnit = false;

            /** @brief True when battery emulation is active. */
            atomic_bool m_emulateBattery = false;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // VehicleController_h__
