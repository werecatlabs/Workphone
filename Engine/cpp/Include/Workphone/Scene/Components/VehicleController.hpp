#ifndef VehicleController_h__
#define VehicleController_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Base class for vehicle control components in the scene graph.
         *
         * The VehicleController class provides a foundation for implementing vehicle
         * physics and control systems. It manages the basic chassis representation
         * and serves as a base class for more specific vehicle implementations like
         * CarController.
         *
         * This component integrates with the physics system through a Rigidbody
         * chassis that represents the main body of the vehicle. Derived classes
         * should implement specific vehicle behaviors such as wheel management,
         * input handling, and physics simulation.
         *
         * @note This is an abstract base class. Use derived classes like CarController
         *       for specific vehicle implementations.
         *
         * @see CarController
         * @see Component
         * @see physics::IRigidbody
         *
         * @author Engine Team
         * @version 1.0
         * @since Engine 1.0
         */
        class WPCore_API VehicleController : public Component
        {
        public:
            /**
             * @brief Physics implementation selected for this scene component.
             *
             * Car preserves the historic behaviour used by CarController. Aircraft
             * creates an IAircraft implementation supplied by WPVehiclePhysics and
             * connects it to the scene rigidbody through an aerodynamic callback.
             */
            enum class VehicleType : s32
            {
                Car = 0,
                Aircraft = 1
            };

            // Add these static const property key declarations
            static const String cgStr;
            static const String massStr;
            static const String moiStr;
            static const String collisionStr;
            static const String chassisStr;
            static const String resetStr;
            static const String resetTransformStr;
            static const String tireModelStr;
            static const String vehicleTypeStr;
            static const String airDensityStr;
            static const String sectionMultiplierStr;
            static const String rollwiseDampingStr;

            /**
             * @brief Default constructor.
             *
             * Initializes the vehicle controller with a null chassis reference.
             * The chassis must be set using setChassis() before the controller
             * can be used for physics simulation.
             */
            VehicleController();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of resources and allows safe destruction
             * of derived vehicle controller types.
             */
            ~VehicleController() override;

            /**
             * @brief Loads the vehicle controller with the provided data.
             *
             * Initializes the vehicle controller component and sets up any
             * necessary resources based on the provided shared object data.
             * This method is called during the component loading phase.
             *
             * @param data Shared object containing initialization data for the controller.
             *             May contain chassis configuration, physics properties, or
             *             other vehicle-specific parameters.
             *
             * @see Component::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the vehicle controller and cleans up resources.
             *
             * Releases any resources held by the vehicle controller and
             * performs necessary cleanup operations. This method is called
             * during the component unloading phase.
             *
             * @param data Shared object containing unloading context data.
             *             May be used to coordinate cleanup with other components.
             *
             * @see Component::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::update */
            void update() override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc Component::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the chassis rigidbody of the vehicle.
             *
             * The chassis represents the main physical body of the vehicle
             * and is used for physics simulation, collision detection, and
             * force application.
             *
             * @return Smart pointer to the chassis Rigidbody, or nullptr if no
             *         chassis has been assigned.
             *
             * @see setChassis
             * @see physics::IRigidbody
             */
            SmartPtr<Rigidbody> getChassis() const;

            /**
             * @brief Sets the chassis rigidbody for the vehicle.
             *
             * Assigns a Rigidbody to act as the vehicle's chassis. This rigidbody
             * will be used for all physics simulation, including force application,
             * collision detection, and movement calculations.
             *
             * @param chassis Smart pointer to the Rigidbody to use as the chassis.
             *                Can be nullptr to clear the current chassis assignment.
             *
             * @note The chassis should be properly configured with appropriate
             *       mass, inertia, and collision shapes before assignment.
             *
             * @see getChassis
             * @see physics::IRigidbody
             */
            void setChassis( SmartPtr<Rigidbody> chassis );

            /**
             * @brief Gets the collision detection component for the vehicle.
             *
             * The collision component is responsible for detecting collisions
             * between the vehicle and other objects in the scene. It can be used
             * to implement custom collision handling logic.
             *
             * @return Smart pointer to the Collision component, or nullptr if no
             *         collision component has been assigned.
             */
            SmartPtr<Collision> getCollision() const;

            /**
             * @brief Sets the collision detection component for the vehicle.
             *
             * Assigns a Collision component to handle collision detection for
             * the vehicle. This component should be configured with appropriate
             * collision shapes and properties to ensure accurate physics interactions.
             *
             * @param collision Smart pointer to the Collision component to use.
             *                  Can be nullptr to clear the current collision assignment.
             */
            void setCollision( SmartPtr<Collision> collision );

            /**
             * @brief Gets the moment of inertia tensor for the vehicle.
             *
             * The moment of inertia defines how the vehicle responds to rotational forces
             * around each axis (X, Y, Z).
             *
             * @return 3D vector representing the moment of inertia components in kg·m².
             */
            Vector3<real_Num> getMOI() const;

            /**
             * @brief Sets the moment of inertia tensor for the vehicle.
             *
             * @param moi 3D vector representing the moment of inertia components in kg·m².
             *            Each component should be positive.
             */
            void setMOI( const Vector3<real_Num> &moi );

            /**
             * @brief Gets the center of gravity offset for the vehicle.
             *
             * The center of gravity (CG) is the point where the vehicle's mass is
             * considered to be concentrated. It affects how the vehicle handles
             * during turns and maneuvers.
             *
             * @return 3D vector representing the center of gravity offset in meters.
             */
            Vector3<real_Num> getCg() const;

            /**
             * @brief Gets the center of gravity offset for the vehicle.
             *
             * The center of gravity (CG) is the point where the vehicle's mass is
             * considered to be concentrated. It affects how the vehicle handles
             * during turns and maneuvers.
             *
             * @return 3D vector representing the center of gravity offset in meters.
             */
            void setCg( const Vector3<real_Num> &cg );

            /**
             * @brief Gets the mass of the vehicle.
             *
             * The mass is used in physics calculations to determine how the vehicle
             * responds to forces and collisions.
             *
             * @return The mass of the vehicle in kilograms (kg).
             */
            f32 getMass() const;

            /**
             * @brief Sets the mass of the vehicle.
             *
             * The mass is used in physics calculations to determine how the vehicle
             * responds to forces and collisions. It should be set before starting
             * any physics simulation.
             *
             * @param mass The mass of the vehicle in kilograms (kg).
             *             Must be a positive value.
             */
            void setMass( f32 mass );

            /**
             * @brief Gets the transform to reset the vehicle to its initial state.
             * @return Transform3 representing the reset position and orientation.
             */
            Transform3<real_Num> getResetTransform() const;

            /**
             * @brief Sets the transform to reset the vehicle to its initial state.
             * @param resetTransform Transform3 representing the reset position and orientation.
             */
            void setResetTransform( Transform3<real_Num> resetTransform );

            /**
             * @brief Handles component-specific finite state machine events.
             *
             * @param state Current FSM state.
             * @param eventType Type of FSM event to handle.
             * @return Result of the FSM event processing.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Gets the current tire model used for simulation.
             * @return The active tire model
             */
            TireModel getTireModel() const;

            /**
             * @brief Sets the tire model to use for simulation.
             * @param tireModel The tire model to activate
             */
            void setTireModel( TireModel tireModel );

            /**
             * @brief Gets the selected vehicle physics type as an integer for editor/Lua use.
             * @return 0 for car physics, 1 for aircraft aerodynamics.
             */
            s32 getVehicleType() const;

            /**
             * @brief Selects car or aircraft physics.
             *
             * Values outside the VehicleType range are ignored. Switching away
             * from Aircraft releases the aerodynamic controller safely.
             *
             * @param vehicleType 0 for car physics, 1 for aircraft aerodynamics.
             */
            void setVehicleType( s32 vehicleType );

            /** @return True when an aerodynamic aircraft implementation is active. */
            bool isAircraftPhysicsEnabled() const;

            /** @return The active WPVehiclePhysics aircraft interface, if any. */
            SmartPtr<vehicle::IAircraft> getAircraftController() const;

            /** @brief Sets all normalized aircraft controls in one call. */
            void setAircraftControls( f32 throttle, f32 pitch, f32 roll, f32 yaw );

            f32 getAircraftThrottle() const;
            void setAircraftThrottle( f32 throttle );

            f32 getAircraftPitch() const;
            void setAircraftPitch( f32 pitch );

            f32 getAircraftRoll() const;
            void setAircraftRoll( f32 roll );

            f32 getAircraftYaw() const;
            void setAircraftYaw( f32 yaw );

            real_Num getAirDensity() const;
            void setAirDensity( real_Num airDensity );

            real_Num getAerodynamicSectionMultiplier() const;
            void setAerodynamicSectionMultiplier( real_Num sectionMultiplier );

            real_Num getRollwiseDamping() const;
            void setRollwiseDamping( real_Num rollwiseDamping );

            /// @brief Class registration declaration for the type system.
            WP_CLASS_REGISTER_DECL;

        protected:
            void createAircraftController();
            void destroyAircraftController( SmartPtr<ISharedObject> data = nullptr );
            void applyAircraftConfiguration();

            SmartPtr<Rigidbody> m_chassis;             ///< Vehicle chassis rigidbody
            SmartPtr<Collision> m_collision;           ///< Collision detection component
            SmartPtr<IEventListener> m_inputListener;  ///< Input event listener

            SmartPtr<vehicle::IAircraft> m_aircraftController;
            SmartPtr<vehicle::IAircraftCallback> m_aircraftCallback;

            Vector3<real_Num> m_moi = Vector3<real_Num>::unit();  ///< Moment of inertia tensor (kg·m²)
            Vector3<real_Num> m_cg = Vector3<real_Num>::zero();   ///< Center of gravity offset (meters)

            TireModel m_tireModel = TireModel::Simple;  ///< Current tire model for simulation

            f32 m_mass = 1370.0f;  ///< Vehicle mass (kg)

            VehicleType m_vehicleType = VehicleType::Car;
            f32 m_aircraftThrottle = 0.0f;
            f32 m_aircraftPitch = 0.0f;
            f32 m_aircraftRoll = 0.0f;
            f32 m_aircraftYaw = 0.0f;

            real_Num m_airDensity = static_cast<real_Num>( 1.225 );
            real_Num m_sectionMultiplier = static_cast<real_Num>( 16.0 );
            real_Num m_rollwiseDamping = static_cast<real_Num>( 1.0 );

            Transform3<real_Num> m_resetTransform;  ///< Transform to reset the vehicle to
        };
    }  // namespace scene
}  // namespace workphone

#endif  // VehicleController_h__
