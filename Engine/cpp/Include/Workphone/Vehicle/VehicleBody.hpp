#ifndef __VehicleBody_h__
#define __VehicleBody_h__

#include <Workphone/Vehicle/VehicleComponent.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Implementation of the vehicle body physics component.
         *
         * The VehicleBody class represents the main rigid body of a vehicle, handling
         * physics properties such as mass, velocity, forces, and torques. It serves as
         * the central physics representation for vehicle simulation, managing both linear
         * and angular motion properties.
         *
         * @details This class provides functionality for:
         * - Managing linear and angular velocities in both world and local coordinate systems
         * - Applying forces and torques at specific positions
         * - Handling mass and center of mass calculations
         * - Performing raycasting operations
         * - Integrating with the parent vehicle system
         *
         * @see IVehicleBody
         * @see VehicleComponent
         * @author WorkPhone Vehicle System
         * @since 1.0
         */
        class WPCore_API VehicleBody : public VehicleComponent<IVehicleBody>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes a new VehicleBody instance with default values.
             * Sets the default mass to 1500.0 units.
             */
            VehicleBody();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup by calling unload() to release resources.
             */
            ~VehicleBody() override;

            /**
             * @brief Unloads and cleans up the vehicle body resources.
             *
             * @param data Optional shared object data for cleanup context
             *
             * @copydoc IVehicleBody::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the vehicle body state.
             *
             * Performs per-frame updates for the vehicle body physics state.
             *
             * @copydoc IVehicleBody::update
             */
            void update() override;

            /**
             * @brief Checks if the vehicle body is in a valid state.
             *
             * @return true if the vehicle body is valid and properly initialized
             * @return false if the vehicle body is in an invalid state
             *
             * @copydoc IVehicleBody::isValid
             */
            bool isValid() const override;

            /**
             * @brief Gets the current linear velocity in world coordinates.
             *
             * @return Vector3<real_Num> The current world velocity vector
             *
             * @copydoc IVehicleBody::getVelocity
             */
            Vector3<real_Num> getVelocity() const override;

            /**
             * @brief Sets the linear velocity in world coordinates.
             *
             * @param velocity The new world velocity vector to set
             *
             * @copydoc IVehicleBody::setVelocity
             */
            void setVelocity( const Vector3<real_Num> &velocity ) override;

            /**
             * @brief Gets the current angular velocity in world coordinates.
             *
             * @return Vector3<real_Num> The current world angular velocity vector (rad/s)
             *
             * @copydoc IVehicleBody::getAngularVelocity
             */
            Vector3<real_Num> getAngularVelocity() const override;

            /**
             * @brief Sets the angular velocity in world coordinates.
             *
             * @param angularVelocity The new world angular velocity vector to set (rad/s)
             *
             * @copydoc IVehicleBody::setAngularVelocity
             */
            void setAngularVelocity( const Vector3<real_Num> &angularVelocity ) override;

            /**
             * @brief Gets the world position of the center of mass.
             *
             * @return Vector3<real_Num> The world position of the center of mass
             */
            Vector3<real_Num> getWorldCenterOfMass() const override;

            /**
             * @brief Sets the world position of the center of mass.
             *
             * @param force The new world position for the center of mass
             */
            void setWorldCenterOfMass( const Vector3<real_Num> &force ) override;

            /**
             * @brief Applies a force at a specific position in local coordinates.
             *
             * Applies a force vector at the specified local position using the given force mode.
             * The force and position are interpreted in the vehicle's local coordinate system.
             *
             * @param force The force vector to apply (in local coordinates)
             * @param pos The position where the force is applied (in local coordinates)
             * @param forceMode The mode determining how the force is applied (impulse, force, etc.)
             */
            void addLocalForceAtPosition( const Vector3<real_Num> &force, const Vector3<real_Num> &pos,
                                          physics::ForceModeEnum forceMode );

            /**
             * @brief Applies a force at a specific position in world coordinates.
             *
             * Applies a force vector at the specified world position using the given force mode.
             * The force and position are interpreted in world coordinate system.
             *
             * @param force The force vector to apply (in world coordinates)
             * @param pos The position where the force is applied (in world coordinates)
             * @param forceMode The mode determining how the force is applied (impulse, force, etc.)
             */
            void addForceAtPosition( const Vector3<real_Num> &force, const Vector3<real_Num> &pos,
                                     physics::ForceModeEnum forceMode );

            /**
             * @brief Applies a torque to the vehicle body in world coordinates.
             *
             * @param torque The torque vector to apply (in world coordinates)
             *
             * @copydoc IVehicleBody::addTorque
             */
            void addTorque( const Vector3<real_Num> &torque ) override;

            /**
             * @brief Applies a torque to the vehicle body in local coordinates.
             *
             * @param localTorque The torque vector to apply (in local coordinates)
             *
             * @copydoc IVehicleBody::addLocalTorque
             */
            void addLocalTorque( const Vector3<real_Num> &localTorque ) override;

            /**
             * @brief Gets a reference to the parent vehicle.
             *
             * @return SmartPtr<IVehicle>& Reference to the parent vehicle smart pointer
             */
            SmartPtr<IVehicle> &getParentVehicle();

            /**
             * @brief Gets a const reference to the parent vehicle.
             *
             * @return const SmartPtr<IVehicle>& Const reference to the parent vehicle smart pointer
             */
            const SmartPtr<IVehicle> &getParentVehicle() const;

            /**
             * @brief Sets the parent vehicle for this body.
             *
             * @param parentVehicle Smart pointer to the parent vehicle to set
             */
            void setParentVehicle( SmartPtr<IVehicle> parentVehicle );

            /**
             * @brief Gets the velocity of a specific point on the vehicle body.
             *
             * Calculates the velocity of a point taking into account both linear and angular motion.
             *
             * @param p The point position (in world coordinates)
             * @return Vector3<real_Num> The velocity of the specified point
             *
             * @copydoc IVehicleBody::getPointVelocity
             */
            Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) override;

            /**
             * @brief Applies a force at a local position using local coordinates.
             *
             * Both the force vector and position are specified in the vehicle's local coordinate system.
             *
             * @param force The force vector to apply (in local coordinates)
             * @param pos The local position where the force is applied
             *
             * @copydoc IVehicleBody::addLocalForceAtLocalPosition
             */
            void addLocalForceAtLocalPosition( const Vector3<real_Num> &force,
                                               const Vector3<real_Num> &pos ) override;

            /**
             * @brief Applies a force at a specific position (overloaded method).
             *
             * This overloaded version applies force without specifying the force mode,
             * using default force application behavior.
             *
             * @param force The force vector to apply
             * @param pos The position where the force is applied
             *
             * @copydoc IVehicleBody::addForceAtPosition
             */
            void addForceAtPosition( const Vector3<real_Num> &force,
                                     const Vector3<real_Num> &pos ) override;

            /**
             * @brief Performs a raycast in local coordinate system.
             *
             * Casts a ray from the vehicle's local coordinate system and returns hit information.
             *
             * @param ray The ray to cast (in local coordinates)
             * @param data Output parameter containing hit information if collision occurs
             * @return true if the ray hit something, false otherwise
             *
             * @copydoc IVehicleBody::castLocalRay
             */
            bool castLocalRay( const Ray3<real_Num> &ray,
                               SmartPtr<physics::IRaycastHit> &data ) override;

            /**
             * @brief Performs a raycast in world coordinate system.
             *
             * Casts a ray in world coordinates and returns hit information.
             *
             * @param ray The ray to cast (in world coordinates)
             * @param data Output parameter containing hit information if collision occurs
             * @return true if the ray hit something, false otherwise
             *
             * @copydoc IVehicleBody::castWorldRay
             */
            bool castWorldRay( const Ray3<real_Num> &ray,
                               SmartPtr<physics::IRaycastHit> &data ) override;

            /**
             * @brief Gets the total mass of the vehicle body.
             *
             * @return real_Num The mass of the vehicle body in appropriate units
             *
             * @copydoc IVehicleBody::getMass
             */
            real_Num getMass() const override;

            /**
             * @brief Sets the total mass of the vehicle body.
             *
             * @param mass The new mass value to set for the vehicle body
             *
             * @copydoc IVehicleBody::setMass
             */
            void setMass( real_Num mass ) override;

            /**
             * @brief Gets the linear velocity in local coordinates.
             *
             * @return Vector3<real_Num> The current local velocity vector
             *
             * @copydoc IVehicleBody::getLocalVelocity
             */
            Vector3<real_Num> getLocalVelocity() const override;

            /**
             * @brief Sets the linear velocity in local coordinates.
             *
             * @param localVelocity The new local velocity vector to set
             *
             * @copydoc IVehicleBody::setLocalVelocity
             */
            void setLocalVelocity( const Vector3<real_Num> &localVelocity ) override;

            /**
             * @brief Gets the angular velocity in local coordinates.
             *
             * @return Vector3<real_Num> The current local angular velocity vector (rad/s)
             *
             * @copydoc IVehicleBody::getLocalAngularVelocity
             */
            Vector3<real_Num> getLocalAngularVelocity() const override;

            /**
             * @brief Sets the angular velocity in local coordinates.
             *
             * @param localAngularVelocity The new local angular velocity vector to set (rad/s)
             *
             * @copydoc IVehicleBody::setLocalAngularVelocity
             */
            void setLocalAngularVelocity( const Vector3<real_Num> &localAngularVelocity ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Smart pointer to the parent vehicle that owns this body */
            SmartPtr<IVehicle> m_parentVehicle;

            /** @brief Current linear velocity in world coordinates */
            Vector3<real_Num> m_velocity;

            /** @brief Current angular velocity in world coordinates (rad/s) */
            Vector3<real_Num> m_angularVelocity;

            /** @brief Current linear velocity in local coordinates */
            Vector3<real_Num> m_localVelocity;

            /** @brief Current angular velocity in local coordinates (rad/s) */
            Vector3<real_Num> m_localAngularVelocity;

            /** @brief World position of the center of mass */
            Vector3<real_Num> m_worldCenterOfMass;

            /** @brief Total mass of the vehicle body (default: 1500.0 units) */
            real_Num m_mass = static_cast<real_Num>( 1500.0 );
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // Rigidbody_h__
