#ifndef WPPhysxVehicle_h__
#define WPPhysxVehicle_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsVehicle3.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class PhysxVehicle3
         * @brief PhysX implementation of a 4-wheel drive vehicle simulation.
         *
         * This class provides a concrete implementation of the IPhysicsVehicle3 interface
         * using NVIDIA PhysX vehicle dynamics. It supports 4-wheel drive vehicles with
         * engine, braking, and steering control for each wheel. The class manages
         * vehicle state including position, orientation, velocity, and wheel transformations.
         */
        class PhysxVehicle3 : public IPhysicsVehicle3
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes a new PhysX vehicle instance with default values.
             */
            PhysxVehicle3();

            /**
             * @brief Destructor.
             *
             * Cleans up vehicle resources and releases PhysX objects.
             */
            ~PhysxVehicle3() override;

            /**
             * @brief Initializes the vehicle with a director template.
             *
             * Sets up the vehicle's initial configuration based on the provided
             * scene director object template.
             *
             * @param objectTemplate Smart pointer to the director template containing
             *                       vehicle configuration data
             */
            void initialise( SmartPtr<IBuildDirector> objectTemplate );

            /**
             * @brief Updates the vehicle simulation state.
             *
             * Advances the vehicle physics simulation by the given time step.
             * This should be called each frame to update vehicle dynamics.
             *
             * @param task Task identifier for the update operation
             * @param t Current absolute time
             * @param dt Time delta since last update
             */
            void update( const s32 &task, const time_interval &t, const time_interval &dt );

            /**
             * @brief Adds a new wheel to the vehicle.
             *
             * Creates and attaches a new wheel to the vehicle. Wheels should be added
             * before finalizing the vehicle configuration.
             *
             * @return Pointer to the newly created wheel interface
             */
            IPhysicsVehicleWheel3 *addWheel() override;

            /**
             * @brief Retrieves a specific wheel by index.
             *
             * @param wheelIndex Zero-based index of the wheel to retrieve
             * @return Pointer to the wheel interface, or nullptr if index is invalid
             */
            IPhysicsVehicleWheel3 *getWheel( u32 wheelIndex ) const override;

            /**
             * @brief Gets the total number of wheels attached to the vehicle.
             *
             * @return Total wheel count
             */
            u32 getNumWheels() const override;

            /**
             * @brief Finalizes the vehicle setup.
             *
             * Completes the vehicle configuration after all wheels have been added.
             * This must be called before the vehicle can be simulated.
             */
            void finalize() override;

            /**
             * @brief Applies engine force to a specific wheel.
             *
             * Controls the drive torque applied to the specified wheel.
             * Positive values accelerate the vehicle, negative values provide engine braking.
             *
             * @param engineForce Force magnitude to apply (typically in Newtons)
             * @param wheelIndex Zero-based index of the wheel to apply force to
             */
            void applyEngineForce( f32 engineForce, u32 wheelIndex ) override;

            /**
             * @brief Sets the brake force for a specific wheel.
             *
             * Controls the braking torque applied to the specified wheel.
             * Higher values result in stronger braking.
             *
             * @param brakeForce Brake force magnitude (0.0 = no braking)
             * @param wheelIndex Zero-based index of the wheel to apply braking to
             */
            void setBrake( f32 brakeForce, u32 wheelIndex ) override;

            /**
             * @brief Sets the steering angle for a specific wheel.
             *
             * Controls the steering angle of the specified wheel.
             * Typically used for front wheels in conventional vehicles.
             *
             * @param steeringValue Steering angle in radians (positive = right, negative = left)
             * @param wheelIndex Zero-based index of the wheel to steer
             */
            void setSteeringValue( f32 steeringValue, u32 wheelIndex ) override;

            /**
             * @brief Sets the vehicle's world position.
             *
             * @param position New position in world space coordinates
             */
            void setPosition( const Vector3F &position ) override;

            /**
             * @brief Gets the vehicle's current world position.
             *
             * @return Current position in world space coordinates
             */
            Vector3F getPosition() const override;

            /**
             * @brief Sets the vehicle's orientation.
             *
             * @param orientation Quaternion representing the vehicle's rotation
             */
            void setOrientation( const QuaternionF &orientation ) override;

            /**
             * @brief Gets the vehicle's current orientation.
             *
             * @return Quaternion representing the current rotation
             */
            QuaternionF getOrientation() const override;

            /**
             * @brief Sets the vehicle's linear velocity.
             *
             * @param velocity Velocity vector in world space (m/s)
             */
            void setVelocity( const Vector3F &velocity ) override;

            /**
             * @brief Gets the vehicle's current linear velocity.
             *
             * @return Current velocity vector in world space (m/s)
             */
            Vector3F getVelocity() const override;

            /**
             * @brief Sets the physics material ID for the vehicle.
             *
             * The material ID determines friction and restitution properties
             * for collision interactions.
             *
             * @param materialId Identifier for the physics material to use
             */
            void setMaterialId( u32 materialId ) override;

            /**
             * @brief Gets the current physics material ID.
             *
             * @return Current material identifier
             */
            u32 getMaterialId() const override;

            /**
             * @brief Gets the vehicle's axis-aligned bounding box in local space.
             *
             * @return AABB in local (object) coordinates
             */
            AABB3F getLocalAABB() const override;

            /**
             * @brief Gets the vehicle's axis-aligned bounding box in world space.
             *
             * @return AABB in world coordinates
             */
            AABB3F getWorldAABB() const override;

            /**
             * @brief Enables or disables the vehicle simulation.
             *
             * When disabled, the vehicle will not be updated or interact with
             * the physics world.
             *
             * @param enabled True to enable simulation, false to disable
             */
            void setEnabled( bool enabled ) override;

            /**
             * @brief Checks if the vehicle simulation is enabled.
             *
             * @return True if enabled, false otherwise
             */
            bool isEnabled() const override;

            /**
             * @brief Gets the vehicle input controller (mutable).
             *
             * @return Reference to smart pointer containing the vehicle input interface
             */
            SmartPtr<IPhysicsVehicleInput3> &getVehicleInput() override;

            /**
             * @brief Gets the vehicle input controller (const).
             *
             * @return Const reference to smart pointer containing the vehicle input interface
             */
            const SmartPtr<IPhysicsVehicleInput3> &getVehicleInput() const override;

            /**
             * @brief Gets the underlying PhysX 4-wheel drive vehicle object.
             *
             * Provides direct access to the PhysX vehicle implementation for
             * advanced operations.
             *
             * @return Pointer to the PhysX vehicle object, or nullptr if not initialized
             */
            physx::PxVehicleDrive4W *getVehicle() const;

            /**
             * @brief Sets the underlying PhysX 4-wheel drive vehicle object.
             *
             * @param vehicle Pointer to the PhysX vehicle object to use
             */
            void setVehicle( physx::PxVehicleDrive4W *vehicle );

            /**
             * @brief Gets the current transformation matrices for all wheels.
             *
             * Returns an array of transforms representing the position and
             * orientation of each wheel, useful for visual representation.
             *
             * @return Array of wheel transforms in world space
             */
            Array<Transform3F> getWheelTransformations() const override;

        protected:
            /// Vehicle input controller for processing user commands
            SmartPtr<IPhysicsVehicleInput3> m_vehicleInput;

            /// Pointer to the underlying PhysX 4-wheel drive vehicle
            physx::PxVehicleDrive4W *m_vehicle;

            /// Cached world position of the vehicle
            Vector3F m_position;

            /// Cached orientation of the vehicle
            QuaternionF m_orientation;

            /// Array storing current wheel transforms for rendering
            Array<Transform3F> m_wheelTransforms;
        };

    } // namespace physics
} // namespace workphone

#endif // WPPhysxVehicle_h__
