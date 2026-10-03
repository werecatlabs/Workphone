#ifndef __IPhysicsVehicle__H
#define __IPhysicsVehicle__H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Abstract interface for a 3D physics vehicle.
         *
         * This interface models a vehicle composed of a rigid chassis and multiple
         * wheels. Implementations are responsible for integrating with a physics
         * engine, managing wheel bodies, chassis transforms and vehicle-specific
         * forces (engine, brake, steering).
         *
         * Responsibilities:
         * - Create and manage wheel objects.
         * - Provide access to and control of vehicle transforms and velocities.
         * - Apply per-wheel control inputs (engine force, brake, steering).
         * - Expose bounding volumes and material identification for collision/scene use.
         *
         * Usage notes:
         * - Add all wheels via `addWheel()` before calling `finalize()`.
         * - After `finalize()` the vehicle is expected to be ready for simulation.
         *
         * @see IPhysicsBody3
         * @see IPhysicsVehicleWheel3
         * @see IPhysicsVehicleInput3
         * @see IPhysicsManager
         */
        class WPCore_API IPhysicsVehicle3 : public ISharedObject
        {
        public:
            /** Virtual destructor. Implementations should perform any required cleanup. */
            ~IPhysicsVehicle3() override;

            /**
             * @brief Create and add a new wheel to the vehicle.
             *
             * The returned wheel pointer is owned/managed by the vehicle implementation.
             * Callers should not delete the pointer. Configure wheel properties on the
             * returned object before calling `finalize()`.
             *
             * @return Pointer to the newly created wheel interface instance.
             */
            virtual IPhysicsVehicleWheel3 *addWheel() = 0;

            /**
             * @brief Retrieve a wheel by zero-based index.
             *
             * @param wheelIndex Zero-based index of the wheel to retrieve.
             * @return Pointer to the wheel at the specified index, or `nullptr` if the index is out of
             * range.
             */
            virtual IPhysicsVehicleWheel3 *getWheel( u32 wheelIndex ) const = 0;

            /**
             * @brief Get the number of wheels attached to the vehicle.
             *
             * @return The count of wheels currently added to the vehicle.
             */
            virtual u32 getNumWheels() const = 0;

            /**
             * @brief Finalize the vehicle setup.
             *
             * This must be called after adding and configuring all wheels and before
             * the vehicle is used in the physics simulation. Implementations should
             * use this call to construct internal physics joints, allocate runtime
             * resources and register the vehicle with the physics manager if required.
             *
             * @note Calling physics control methods before `finalize()` may have no effect.
             */
            virtual void finalize() = 0;

            /**
             * @brief Apply engine (drive) force to a wheel.
             *
             * Typical units are force or torque depending on implementation. Positive
             * or negative values may be used for forward/reverse propulsion.
             *
             * @param engineForce Magnitude of engine force to apply.
             * @param wheelIndex Index of the wheel to receive the force.
             */
            virtual void applyEngineForce( f32 engineForce, u32 wheelIndex ) = 0;

            /**
             * @brief Set braking force for a specific wheel.
             *
             * Brake values are implementation-defined (force, torque or normalized).
             * Consider clamping values in the caller or implementation as appropriate.
             *
             * @param brakeForce Brake magnitude to apply.
             * @param wheelIndex Index of the wheel to brake.
             */
            virtual void setBrake( f32 brakeForce, u32 wheelIndex ) = 0;

            /**
             * @brief Set the steering angle for a specific wheel.
             *
             * Steering values represent the wheel rotation about its steering axis
             * (typically yaw). Units are radians unless otherwise documented by the
             * implementation.
             *
             * @param steeringValue Steering angle to set (radians).
             * @param wheelIndex Index of the wheel to steer.
             */
            virtual void setSteeringValue( f32 steeringValue, u32 wheelIndex ) = 0;

            /**
             * @brief Set the world-space position of the vehicle chassis.
             *
             * Implementations should update internal transforms and, if required,
             * sync the physics engine's rigid body transform to this value.
             *
             * @param position New world-space position for the vehicle chassis.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Get the current world-space position of the vehicle chassis.
             *
             * @return Current chassis position in world coordinates.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Set the world-space orientation of the vehicle chassis.
             *
             * @param orientation New world-space orientation for the chassis.
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /**
             * @brief Get the current world-space orientation of the vehicle chassis.
             *
             * @return Current chassis orientation as a quaternion.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Set the linear velocity of the vehicle chassis.
             *
             * Implementations apply this to the underlying rigid body. Units are
             * world-space linear velocity (units per second).
             *
             * @param velocity Linear velocity vector to set.
             */
            virtual void setVelocity( const Vector3<real_Num> &velocity ) = 0;

            /**
             * @brief Get the current linear velocity of the vehicle chassis.
             *
             * @return Current linear velocity in world-space units per second.
             */
            virtual Vector3<real_Num> getVelocity() const = 0;

            /**
             * @brief Assign a material identifier to the vehicle for collision or rendering.
             *
             * This ID can be used by systems that query surface/material properties
             * (e.g. to select friction, audio or visual effects).
             *
             * @param materialId Numeric material identifier.
             */
            virtual void setMaterialId( u32 materialId ) = 0;

            /**
             * @brief Get the current material identifier assigned to the vehicle.
             *
             * @return Numeric material identifier.
             */
            virtual u32 getMaterialId() const = 0;

            /**
             * @brief Get the vehicle's local axis-aligned bounding box.
             *
             * The local AABB is expressed in the vehicle's local/chassis coordinate space.
             * Useful for culling, editing and non-physics queries.
             *
             * @return Local-space axis-aligned bounding box (AABB).
             */
            virtual AABB3F getLocalAABB() const = 0;

            /**
             * @brief Get the vehicle's world-space axis-aligned bounding box.
             *
             * This box is the local AABB transformed into world coordinates and is
             * suitable for scene queries and collision broad-phase tests.
             *
             * @return World-space axis-aligned bounding box (AABB).
             */
            virtual AABB3F getWorldAABB() const = 0;

            /**
             * @brief Enable or disable the vehicle.
             *
             * When disabled the vehicle should not participate in simulation updates,
             * collision handling or scene queries depending on implementation.
             *
             * @param enabled `true` to enable simulation; `false` to disable.
             */
            virtual void setEnabled( bool enabled ) = 0;

            /**
             * @brief Query whether the vehicle is enabled for simulation.
             *
             * @return `true` if the vehicle is enabled; otherwise `false`.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Get a modifiable reference to the vehicle input interface.
             *
             * The vehicle input interface aggregates controls (throttle, brake,
             * steering, gear) and may be used by AI or player controllers to drive
             * the vehicle. The returned SmartPtr may be null if no input object has
             * been attached.
             *
             * @return SmartPtr reference to the mutable `IPhysicsVehicleInput3`.
             */
            virtual SmartPtr<IPhysicsVehicleInput3> &getVehicleInput() = 0;

            /**
             * @brief Get a read-only reference to the vehicle input interface.
             *
             * @return const SmartPtr reference to the `IPhysicsVehicleInput3`.
             */
            virtual const SmartPtr<IPhysicsVehicleInput3> &getVehicleInput() const = 0;

            /**
             * @brief Retrieve the current world-space transforms for all wheels.
             *
             * The returned array contains `Transform3F` entries for each wheel in
             * the same order as they were added. These transforms typically represent
             * the physical location and orientation of the wheel contact point or
             * visual wheel bone for rendering.
             *
             * @return Array of wheel transforms in world space.
             */
            virtual Array<Transform3F> getWheelTransformations() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif
