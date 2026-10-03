#ifndef __IPhysicsVehicleWheel__H
#define __IPhysicsVehicleWheel__H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 3D vehicle wheel.
         *
         * This class represents a wheel in a 3D physics vehicle simulation.
         * Wheels are complex physics objects that handle suspension, steering,
         * braking, and engine force application.
         *
         * The interface provides functionality for:
         * - Managing wheel properties (radius, width, etc.)
         * - Controlling suspension behavior
         * - Handling steering and braking
         * - Managing wheel state and transformations
         * - Applying engine forces
         *
         * Wheels are used to:
         * - Simulate realistic vehicle movement
         * - Handle terrain interaction
         * - Control vehicle dynamics
         * - Manage wheel-specific effects
         *
         * @see IPhysicsVehicle3
         * @see IPhysicsBody3
         * @see IPhysicsManager
         */
        class WPCore_API IPhysicsVehicleWheel3 : public ISharedObject
        {
        public:
            /** Destructor */
            ~IPhysicsVehicleWheel3() override;

            /**
             * @brief Gets the radius of the wheel.
             *
             * The radius determines the size of the wheel and affects
             * its interaction with the ground and overall vehicle behavior.
             *
             * @return The current wheel radius.
             */
            virtual real_Num getRadius() const = 0;

            /**
             * @brief Sets the radius of the wheel.
             *
             * The radius determines the size of the wheel and affects
             * its interaction with the ground and overall vehicle behavior.
             *
             * @param radius The new wheel radius to set.
             */
            virtual void setRadius( real_Num radius ) = 0;

            /**
             * @brief Gets the width of the wheel.
             *
             * The width determines the contact area of the wheel with the ground
             * and affects traction and handling.
             *
             * @return The current wheel width.
             */
            virtual real_Num getWidth() const = 0;

            /**
             * @brief Sets the width of the wheel.
             *
             * The width determines the contact area of the wheel with the ground
             * and affects traction and handling.
             *
             * @param width The new wheel width to set.
             */
            virtual void setWidth( real_Num width ) = 0;

            /**
             * @brief Gets the maximum suspension travel distance.
             *
             * This determines how far the wheel can move up and down
             * in response to terrain changes.
             *
             * @return The maximum suspension travel distance.
             */
            virtual real_Num getMaxSuspensionTravelCm() const = 0;

            /**
             * @brief Sets the maximum suspension travel distance.
             *
             * This determines how far the wheel can move up and down
             * in response to terrain changes.
             *
             * @param maxSuspensionTravelCm The new maximum suspension travel distance.
             */
            virtual void setMaxSuspensionTravelCm( real_Num maxSuspensionTravelCm ) = 0;

            /**
             * @brief Gets the maximum suspension force.
             *
             * This determines how strongly the suspension resists compression
             * and affects the vehicle's handling over rough terrain.
             *
             * @return The maximum suspension force.
             */
            virtual real_Num getMaxSuspensionForce() const = 0;

            /**
             * @brief Sets the maximum suspension force.
             *
             * This determines how strongly the suspension resists compression
             * and affects the vehicle's handling over rough terrain.
             *
             * @param maxSuspensionForce The new maximum suspension force.
             */
            virtual void setMaxSuspensionForce( real_Num maxSuspensionForce ) = 0;

            /**
             * @brief Gets the suspension stiffness.
             *
             * This determines how quickly the suspension responds to terrain changes
             * and affects the vehicle's ride quality.
             *
             * @return The suspension stiffness.
             */
            virtual real_Num getSuspensionStiffness() const = 0;

            /**
             * @brief Sets the suspension stiffness.
             *
             * This determines how quickly the suspension responds to terrain changes
             * and affects the vehicle's ride quality.
             *
             * @param suspensionStiffness The new suspension stiffness.
             */
            virtual void setSuspensionStiffness( real_Num suspensionStiffness ) = 0;

            /**
             * @brief Gets the suspension damping.
             *
             * This determines how quickly the suspension oscillations are damped
             * and affects the vehicle's stability.
             *
             * @return The suspension damping.
             */
            virtual real_Num getSuspensionDamping() const = 0;

            /**
             * @brief Sets the suspension damping.
             *
             * This determines how quickly the suspension oscillations are damped
             * and affects the vehicle's stability.
             *
             * @param suspensionDamping The new suspension damping.
             */
            virtual void setSuspensionDamping( real_Num suspensionDamping ) = 0;

            /**
             * @brief Gets the friction slip value.
             *
             * This determines how much the wheel can slip on the ground
             * and affects traction and handling.
             *
             * @return The friction slip value.
             */
            virtual real_Num getFrictionSlip() const = 0;

            /**
             * @brief Sets the friction slip value.
             *
             * This determines how much the wheel can slip on the ground
             * and affects traction and handling.
             *
             * @param frictionSlip The new friction slip value.
             */
            virtual void setFrictionSlip( real_Num frictionSlip ) = 0;

            /**
             * @brief Gets the steering angle.
             *
             * This determines how much the wheel is turned for steering
             * and affects the vehicle's turning ability.
             *
             * @return The current steering angle.
             */
            virtual real_Num getSteering() const = 0;

            /**
             * @brief Sets the steering angle.
             *
             * This determines how much the wheel is turned for steering
             * and affects the vehicle's turning ability.
             *
             * @param steering The new steering angle.
             */
            virtual void setSteering( real_Num steering ) = 0;

            /**
             * @brief Gets the engine force.
             *
             * This determines how much force is applied to drive the wheel
             * and affects the vehicle's acceleration.
             *
             * @return The current engine force.
             */
            virtual real_Num getEngineForce() const = 0;

            /**
             * @brief Sets the engine force.
             *
             * This determines how much force is applied to drive the wheel
             * and affects the vehicle's acceleration.
             *
             * @param engineForce The new engine force.
             */
            virtual void setEngineForce( real_Num engineForce ) = 0;

            /**
             * @brief Gets the brake force.
             *
             * This determines how much force is applied to slow down the wheel
             * and affects the vehicle's braking ability.
             *
             * @return The current brake force.
             */
            virtual real_Num getBrake() const = 0;

            /**
             * @brief Sets the brake force.
             *
             * This determines how much force is applied to slow down the wheel
             * and affects the vehicle's braking ability.
             *
             * @param brake The new brake force.
             */
            virtual void setBrake( real_Num brake ) = 0;

            /**
             * @brief Gets whether the wheel is in contact with the ground.
             *
             * @return True if the wheel is in contact with the ground, false otherwise.
             */
            virtual bool isInContact() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif
