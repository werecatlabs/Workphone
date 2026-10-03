#ifndef __IVehicleController_h__
#define __IVehicleController_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @file IVehicle.hpp
         * @brief Vehicle interface used by the vehicle system.
         *
         * This interface defines the public contract for all vehicle implementations.
         * Implementations are expected to provide access to physics-related state,
         * apply forces/torques, expose transformation and velocity, and support
         * debug visualisation hooks. The interface is intentionally minimal and
         * designed to be implemented by engine-specific vehicle controllers.
         *
         * Units:
         *  - Positions and linear velocities are in meters (m) and meters/second (m/s).
         *  - Angular velocities are in radians/second (rad/s).
         *  - Forces are in Newtons (N) and torques in Newton-meters (N·m).
         *
         * Thread-safety:
         *  - Implementations should document their own thread-safety guarantees.
         *
         * @see IVehicleBody
         * @see IWheelComponent
         * @see IDriveTrain
         * @see IVehicleCallback
         */
        class WPCore_API IVehicle : public ISharedObject
        {
        public:
            /**
             * @brief Input channels for vehicle control.
             *
             * These channels represent the common control axes exposed by the
             * vehicle for both player and AI control.
             */
            enum class Input
            {
                THROTTLE, /**< Accelerator/throttle (expected range 0.0 — 1.0). */
                BRAKE,    /**< Brake (expected range 0.0 — 1.0). */
                STEERING, /**< Steering input (expected range -1.0 — 1.0, left to right). */

                COUNT /**< Number of input channels. */
            };

            /**
             * @brief Vehicle operational state.
             *
             * Represents high-level states that can affect simulation behaviour
             * (e.g. whether the vehicle should be simulated, reset, or edited).
             */
            enum class State
            {
                AWAKE,     /**< Active and simulated. */
                DESTROYED, /**< Permanently destroyed / not functional. */
                EDIT,      /**< In editor mode (may disable simulation). */
                PLAY,      /**< Normal gameplay mode. */
                RESET,     /**< Being reset to initial config. */

                COUNT /**< Number of states. */
            };

            /**
             * @brief Virtual destructor.
             *
             * Ensure derived destructors are called during cleanup.
             */
            ~IVehicle() override;

            /**
             * @brief Recalculate and update the world transform from simulation state.
             *
             * Called each frame (or physics tick) to ensure the transform exposed
             * to rendering and other systems matches the physics state.
             */
            virtual void updateTransform() = 0;

            /**
             * @brief Reset the vehicle to its initial configured state.
             *
             * Should restore transform, velocities, internal state and optionally
             * the components (wheels, drivetrain) to initial values.
             */
            virtual void reset() = 0;

            /**
             * @brief Read an input channel value.
             *
             * @param idx Index of the input channel (cast from Input enum).
             * @return Current value of the channel. Range and meaning depend on the channel:
             *         THROTTLE/BRAKE: 0.0 — 1.0, STEERING: -1.0 — 1.0.
             * @see Input
             */
            virtual f32 getChannel( s32 idx ) const = 0;

            /**
             * @brief Set an input channel value.
             *
             * Implementations should clamp/validate the value where appropriate.
             *
             * @param idx Index of the input channel (cast from Input enum).
             * @param channel New value for the channel.
             * @see Input
             */
            virtual void setChannel( s32 idx, f32 channel ) = 0;

            /**
             * @brief Get world-space position of the vehicle root.
             *
             * @return Position in world coordinates (meters).
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Set world-space position of the vehicle root.
             *
             * Teleporting the vehicle by setting position should update any
             * internal physics state to avoid tunnelling / inconsistent state.
             *
             * @param position New world position (meters).
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Get world-space orientation.
             *
             * @return Orientation as a normalized quaternion.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Set world-space orientation.
             *
             * @param orientation New orientation quaternion (should be normalized).
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /**
             * @brief Get vehicle scale.
             *
             * @return Scale factors for each axis.
             */
            virtual Vector3<real_Num> getScale() const = 0;

            /**
             * @brief Query whether the vehicle is controlled by the user.
             *
             * @return True if user input controls the vehicle; false if controlled by AI or script.
             */
            virtual bool isUserControlled() const = 0;

            /**
             * @brief Set whether the vehicle is controlled by user input.
             *
             * @param userControlled True to enable user control; false for AI.
             */
            virtual void setUserControlled( bool userControlled ) = 0;

            /**
             * @brief Get the vehicle mass.
             *
             * @return Mass in kilograms (kg).
             */
            virtual real_Num getMass() const = 0;

            /**
             * @brief Set the vehicle mass.
             *
             * Changing mass may require recomputing inertia and other derived values.
             *
             * @param mass New mass in kilograms (kg).
             */
            virtual void setMass( real_Num mass ) = 0;

            /**
             * @brief Access the vehicle body component.
             *
             * The body component represents the physical rigid body of the vehicle.
             *
             * @return Pointer to IVehicleBody instance, may be null.
             * @see IVehicleBody
             */
            virtual IVehicleBody *getBodyPtr() const = 0;

            /**
             * @brief Access the vehicle body component.
             *
             * The body component represents the physical rigid body of the vehicle.
             *
             * @return Smart pointer to IVehicleBody instance, may be null.
             * @see IVehicleBody
             */
            virtual SmartPtr<IVehicleBody> getBody() const = 0;

            /**
             * @brief Assign a vehicle body component.
             *
             * @param body Smart pointer to a new IVehicleBody instance.
             * @see IVehicleBody
             */
            virtual void setBody( SmartPtr<IVehicleBody> body ) = 0;

            /**
             * @brief Get full world transform (position, rotation, scale).
             *
             * @return World-space transform of the vehicle root.
             */
            virtual Transform3<real_Num> getWorldTransform() const = 0;

            /**
             * @brief Set full world transform.
             *
             * @param transform New world transform to apply.
             */
            virtual void setWorldTransform( const Transform3<real_Num> &transform ) = 0;

            /**
             * @brief Get local transform relative to parent.
             *
             * @return Local-space transform.
             */
            virtual Transform3<real_Num> getLocalTransform() const = 0;

            /**
             * @brief Set local transform relative to parent.
             *
             * @param transform New local transform to apply.
             */
            virtual void setLocalTransform( const Transform3<real_Num> &transform ) = 0;

            /**
             * @brief Draw a debug point in world coordinates.
             *
             * Implementations typically forward to the engine's debug renderer.
             *
             * @param body Index of the body part to associate with this point (implementation-defined).
             * @param id Unique identifier for this debug point (can be used to update/remove).
             * @param position World-space position where the point is drawn.
             * @param color RGBA color packed into a 32-bit unsigned integer.
             */
            virtual void drawPoint( s32 body, int id, const Vector3<real_Num> &position, u32 color ) = 0;

            /**
             * @brief Draw a debug point in the vehicle's local space.
             *
             * @param body Index of the body part.
             * @param id Unique identifier for this debug point.
             * @param position Local-space position.
             * @param color RGBA color packed into a 32-bit unsigned integer.
             */
            virtual void drawLocalPoint( s32 body, int id, const Vector3<real_Num> &position,
                                         u32 color ) = 0;

            /**
             * @brief Display a debug vector in local coordinates (no identifier).
             *
             * @param bodyId Body index to which the vector belongs.
             * @param start Local-space start position.
             * @param end Local-space end position.
             * @param colour RGBA color packed into a 32-bit unsigned integer.
             */
            virtual void displayLocalVector( s32 bodyId, const Vector3<real_Num> &start,
                                             const Vector3<real_Num> &end, u32 colour ) = 0;

            /**
             * @brief Display a debug vector in world coordinates.
             *
             * @param bodyId Body index to which the vector belongs.
             * @param id Identifier for the debug vector (used to update/remove).
             * @param start World-space start position.
             * @param end World-space end position.
             * @param colour RGBA color packed into a 32-bit unsigned integer.
             */
            virtual void displayVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                        const Vector3<real_Num> &end, u32 colour ) = 0;

            /**
             * @brief Display a debug vector in local coordinates (with identifier).
             *
             * @param bodyId Body index to which the vector belongs.
             * @param id Identifier for the debug vector.
             * @param start Local-space start position.
             * @param end Local-space end position.
             * @param colour RGBA color packed into a 32-bit unsigned integer.
             */
            virtual void displayLocalVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                             const Vector3<real_Num> &end, u32 colour ) = 0;

            /**
             * @brief Query whether debug visualisation is enabled for this vehicle.
             *
             * @return True when debug drawing is active.
             */
            virtual bool getDisplayDebugData() const = 0;

            /**
             * @brief Enable or disable debug visualisation for this vehicle.
             *
             * @param enabled True to enable debug drawing; false to disable.
             */
            virtual void setDisplayDebugData( bool enabled ) = 0;

            /**
             * @brief Apply a world-space force to a specific body part at a world position.
             *
             * @param bodyIdx Index of the body part to apply the force to.
             * @param force Force vector in world coordinates (Newtons).
             * @param loc World-space application point (meters).
             */
            virtual void addForce( s32 bodyIdx, const Vector3<real_Num> &force,
                                   const Vector3<real_Num> &loc ) = 0;

            /**
             * @brief Apply a world-space torque to a specific body part.
             *
             * @param bodyIdx Index of the body part.
             * @param torque Torque vector in world coordinates (N·m).
             */
            virtual void addTorque( s32 bodyIdx, const Vector3<real_Num> &torque ) = 0;

            /**
             * @brief Apply a force expressed in the local coordinate space of the body.
             *
             * @param bodyIdx Index of the body part.
             * @param force Force vector in local coordinates (Newtons).
             * @param loc Local-space application point (meters).
             */
            virtual void addLocalForce( s32 bodyIdx, const Vector3<real_Num> &force,
                                        const Vector3<real_Num> &loc ) = 0;

            /**
             * @brief Apply a torque expressed in the local coordinate space of the body.
             *
             * @param bodyIdx Index of the body part.
             * @param torque Torque vector in local coordinates (N·m).
             */
            virtual void addLocalTorque( s32 bodyIdx, const Vector3<real_Num> &torque ) = 0;

            /**
             * @brief Get linear velocity of a specific world-space point on the vehicle.
             *
             * @param p World-space point to query (meters).
             * @return Linear velocity at point p (m/s).
             */
            virtual Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) = 0;

            /**
             * @brief Get vehicle angular velocity in world space.
             *
             * @return Angular velocity vector (radians/second).
             */
            virtual Vector3<real_Num> getAngularVelocity() = 0;

            /**
             * @brief Get vehicle linear velocity of the root in world space.
             *
             * @return Linear velocity vector (m/s).
             */
            virtual Vector3<real_Num> getLinearVelocity() = 0;

            /**
             * @brief Get angular velocity expressed in local coordinates.
             *
             * @return Angular velocity in local space (rad/s).
             */
            virtual Vector3<real_Num> getLocalAngularVelocity() = 0;

            /**
             * @brief Get linear velocity expressed in local coordinates.
             *
             * @return Linear velocity in local space (m/s).
             */
            virtual Vector3<real_Num> getLocalLinearVelocity() = 0;

            /**
             * @brief Get a mutable reference to the vehicle callback pointer.
             *
             * The callback is used to notify external systems about vehicle events.
             *
             * @return Reference to the smart pointer holding the IVehicleCallback.
             * @see IVehicleCallback
             */
            virtual IVehicleCallback *getVehicleCallbackPtr() const = 0;

            /**
             * @brief Get a const reference to the vehicle callback pointer.
             *
             * @return Const reference to the smart pointer holding the IVehicleCallback.
             * @see IVehicleCallback
             */
            virtual SmartPtr<IVehicleCallback> getVehicleCallback() const = 0;

            /**
             * @brief Assign the vehicle callback object.
             *
             * @param callback Smart pointer to the IVehicleCallback implementation.
             * @see IVehicleCallback
             */
            virtual void setVehicleCallback( SmartPtr<IVehicleCallback> callback ) = 0;

            /**
             * @brief Get the vehicle center-of-gravity in local coordinates.
             *
             * @return CG position relative to vehicle root (meters).
             */
            virtual Vector3<real_Num> getCG() const = 0;

            /**
             * @brief Set the active operational state for the vehicle.
             *
             * @param state New vehicle state (see State enum).
             * @see State
             */
            virtual void setState( State state ) = 0;

            /**
             * @brief Query the current operational state of the vehicle.
             *
             * @return Current State enum value.
             * @see State
             */
            virtual State getState() const = 0;

            /**
             * @brief Get wheel controller for a specific wheel index.
             *
             * @param index Wheel index (implementation dependent; often 0..N-1).
             * @return Smart pointer to the wheel controller; may be null if index is invalid.
             * @see IWheelComponent
             */
            virtual SmartPtr<IWheelComponent> getWheelController( u32 index ) const = 0;

            /**
             * @brief Get drivetrain component.
             *
             * @return Smart pointer to IDriveTrain; may be null.
             * @see IDriveTrain
             */
            virtual SmartPtr<IDriveTrain> getDriveTrain() const = 0;

            /**
             * @brief Assign the drivetrain component.
             *
             * @param driveTrain Smart pointer to an IDriveTrain implementation.
             * @see IDriveTrain
             */
            virtual void setDriveTrain( SmartPtr<IDriveTrain> driveTrain ) = 0;

            /**
             * @brief Get the current drive type (e.g. FWD, RWD, AWD).
             *
             * @return Current VehicleDriveType value.
             */
            virtual VehicleDriveType getDriveType() const = 0;

            /**
             * @brief Set the vehicle drive type.
             *
             * @param driveType New drive type to apply.
             */
            virtual void setDriveType( VehicleDriveType driveType ) = 0;

            virtual bool isElectric() const = 0;
            virtual void setElectric( bool electric ) = 0;

            virtual bool getEnablePowerUnit() const = 0;
            virtual void setEnablePowerUnit( bool enabled ) = 0;

            virtual bool getEmulateBattery() const = 0;
            virtual void setEmulateBattery( bool emulate ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // __IVehicleController_h__
