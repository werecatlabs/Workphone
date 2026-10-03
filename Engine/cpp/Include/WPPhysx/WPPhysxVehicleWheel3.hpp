#ifndef WPPhysxVehicleWheel3_h__
#define WPPhysxVehicleWheel3_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsVehicleWheel3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @class PhysxVehicleWheel3
         * @brief PhysX implementation of a single wheel for a 4-wheel drive vehicle.
         */
        class PhysxVehicleWheel3 : public IPhysicsVehicleWheel3
        {
        public:
            /** Constructor. Initializes the wheel with default parameters. */
            PhysxVehicleWheel3();

            /** Destructor. Cleans up any resources associated with the wheel. */
            ~PhysxVehicleWheel3() override;

            /**
             * @brief Initializes the wheel with physical properties and configuration.
             *
             * @param pos The position of the wheel relative to the vehicle.
             * @param radius The radius of the wheel in meters.
             * @param width The width of the wheel in meters.
             * @param suspensionRestLength The rest length of the suspension in meters.
             * @param suspension_Ks The suspension spring stiffness coefficient.
             * @param suspension_Kd The suspension damping coefficient.
             * @param powered True if this wheel receives engine power.
             * @param steering True if this wheel is affected by steering input.
             * @param brakes True if this wheel has braking capability.
             */
            void initialise( const Vector3F &pos, f32 radius, f32 width, f32 suspensionRestLength,
                             f32 suspension_Ks, f32 suspension_Kd, bool powered, bool steering,
                             bool brakes );

            /**
             * @brief Gets the current world position of the wheel.
             * @return The wheel's position in world coordinates.
             */
            Vector3F getPosition() const;

            /**
             * @brief Gets the current orientation of the wheel.
             * @return The wheel's orientation as a quaternion.
             */
            QuaternionF getOrientation() const;

            /**
             * @brief Gets the current velocity of the wheel.
             * @return The wheel's velocity vector in world space.
             */
            Vector3F getVelocity() const;

            /**
             * @brief Sets the physics material ID for the wheel.
             * @param materialId The material identifier to assign to the wheel.
             */
            void setMaterialId( u32 materialId );

            /**
             * @brief Gets the current physics material ID of the wheel.
             * @return The material identifier currently assigned to the wheel.
             */
            u32 getMaterialId() const;

            /**
             * @brief Gets the angular velocity of the wheel rotation.
             * @return The angular velocity in radians per second.
             */
            f32 getAngularVelocity() const;

            /**
             * @brief Gets the axis-aligned bounding box in local coordinates.
             * @return The AABB in local space.
             */
            AABB3F getLocalAABB() const;

            /**
             * @brief Gets the axis-aligned bounding box in world coordinates.
             * @return The AABB in world space.
             */
            AABB3F getWorldAABB() const;

            /** @name Wheel geometry */
            ///@{
            /**
             * @brief Gets the radius of the wheel.
             * @return The wheel radius in meters.
             */
            physics_Num getRadius() const override;

            /**
             * @brief Sets the radius of the wheel.
             * @param radius The new wheel radius in meters.
             */
            void setRadius( physics_Num radius ) override;

            /**
             * @brief Gets the width of the wheel.
             * @return The wheel width in meters.
             */
            physics_Num getWidth() const override;

            /**
             * @brief Sets the width of the wheel.
             * @param width The new wheel width in meters.
             */
            void setWidth( physics_Num width ) override;
            ///@}

            /** @name Suspension configuration */
            ///@{
            /**
             * @brief Gets the maximum suspension travel distance.
             * @return The maximum suspension travel in centimeters.
             */
            physics_Num getMaxSuspensionTravelCm() const override;

            /**
             * @brief Sets the maximum suspension travel distance.
             * @param maxSuspensionTravelCm The maximum suspension travel in centimeters.
             */
            void setMaxSuspensionTravelCm( physics_Num maxSuspensionTravelCm ) override;

            /**
             * @brief Gets the maximum force the suspension can exert.
             * @return The maximum suspension force in Newtons.
             */
            physics_Num getMaxSuspensionForce() const override;

            /**
             * @brief Sets the maximum force the suspension can exert.
             * @param maxSuspensionForce The maximum suspension force in Newtons.
             */
            void setMaxSuspensionForce( physics_Num maxSuspensionForce ) override;

            /**
             * @brief Gets the suspension spring stiffness coefficient.
             * @return The suspension stiffness value.
             */
            physics_Num getSuspensionStiffness() const override;

            /**
             * @brief Sets the suspension spring stiffness coefficient.
             * @param suspensionStiffness The new suspension stiffness value.
             */
            void setSuspensionStiffness( physics_Num suspensionStiffness ) override;

            /**
             * @brief Gets the suspension damping coefficient.
             * @return The suspension damping value.
             */
            physics_Num getSuspensionDamping() const override;

            /**
             * @brief Sets the suspension damping coefficient.
             * @param suspensionDamping The new suspension damping value.
             */
            void setSuspensionDamping( physics_Num suspensionDamping ) override;
            ///@}

            /** @name Grip and control */
            ///@{
            /**
             * @brief Gets the friction slip coefficient for the wheel.
             * @return The friction slip value (higher values = more grip).
             */
            physics_Num getFrictionSlip() const override;

            /**
             * @brief Sets the friction slip coefficient for the wheel.
             * @param frictionSlip The new friction slip value (higher values = more grip).
             */
            void setFrictionSlip( physics_Num frictionSlip ) override;

            /**
             * @brief Gets the current steering angle.
             * @return The steering angle in radians (positive = left, negative = right).
             */
            physics_Num getSteering() const override;

            /**
             * @brief Sets the steering angle for the wheel.
             * @param steering The steering angle in radians (positive = left, negative = right).
             */
            void setSteering( physics_Num steering ) override;

            /**
             * @brief Gets the current engine force applied to the wheel.
             * @return The engine force in Newtons.
             */
            physics_Num getEngineForce() const override;

            /**
             * @brief Sets the engine force applied to the wheel.
             * @param engineForce The engine force in Newtons (positive = forward, negative = reverse).
             */
            void setEngineForce( physics_Num engineForce ) override;

            /**
             * @brief Gets the current brake force applied to the wheel.
             * @return The brake force in Newtons.
             */
            physics_Num getBrake() const override;

            /**
             * @brief Sets the brake force applied to the wheel.
             * @param brake The brake force in Newtons (0 = no brake, higher = stronger braking).
             */
            void setBrake( physics_Num brake ) override;

            /**
             * @brief Checks if the wheel is currently in contact with the ground.
             * @return True if the wheel is touching the ground, false otherwise.
             */
            bool isInContact() const override;

        protected:
        };

    } // namespace physics
} // namespace workphone

#endif // WPPhysxVehicleWheel3_h__
