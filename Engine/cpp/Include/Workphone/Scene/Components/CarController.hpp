#ifndef CarController_h__
#define CarController_h__

#include <Workphone/Scene/Components/VehicleController.hpp>
#include <atomic>
#include <Workphone/Interface/Vehicle/IVehicleCallback.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

/**
 * @file CarController.hpp
 * @brief Declaration of the `CarController` component used to control car-like vehicles.
 *
 * The `CarController` component implements vehicle-specific logic on top of the
 * generic `VehicleController`. It exposes vehicle inputs (throttle, brake, steering),
 * configures wheel/drive parameters, and bridges the scene representation to the
 * underlying physics vehicle via `VehicleCallback`.
 */
namespace workphone
{
    namespace scene
    {

        /**
         * @brief Component providing car-specific vehicle behaviour and physics glue.
         *
         * `CarController` configures a four-wheel vehicle for the scene, exposes
         * runtime inputs (throttle, brake, steering) and delegates physics queries
         * and forces to an `vehicle::IVehicle` implementation via `VehicleCallback`.
         *
         * Responsibilities:
         * - Provide a scene-level component that can be attached to an actor.
         * - Configure wheel positions, suspension and tire parameters used by the
         *   physics vehicle implementation.
         * - Translate input events into vehicle control values and forward them to
         *   the physics controller.
         *
         * This header contains two helper nested classes used by the component:
         * - `InputListener` — receives input and update events and updates control
         *   inputs on the owning `CarController`.
         * - `VehicleCallback` — implements `vehicle::IVehicleCallback` to provide
         *   scene/actor state to the physics vehicle and accept requests to apply
         *   forces/torques or execute rays.
         *
         * See also: `VehicleController` (base class), `vehicle::IVehicle` (physics API).
         */
        class WPCore_API CarController : public VehicleController
        {
        public:
            /**
             * @brief Event listener used to translate input events into controller state.
             *
             * The `InputListener` is attached to the input system and updates the
             * owning `CarController`'s throttle, brake and steering values. The
             * listener keeps a weak reference to the owner to avoid ownership cycles.
             */
            class WPCore_API InputListener : public IEventListener
            {
            public:
                /**
                 * @brief Construct a new InputListener.
                 */
                InputListener();

                /**
                 * @brief Destroy the InputListener.
                 */
                ~InputListener() override;

                /**
                 * @brief Called when the listener is being unloaded.
                 *
                 * Implementations should release any transient resources associated
                 * with `data`.
                 *
                 * @param data Optional shared object with unload context.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Generic event entry point required by the event system.
                 *
                 * This method dispatches incoming events to specialized handlers such
                 * as `inputEvent` and `updateEvent`.
                 *
                 * @param eventType Type of the incoming event.
                 * @param eventValue Hash or identifier value associated with the event.
                 * @param arguments Parameters passed with the event.
                 * @param sender Object that raised the event.
                 * @param object Target object for the event.
                 * @param event Pointer to the event instance.
                 * @return A `Parameter` describing the result or any return value for
                 *         the event dispatch.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Handle a raw input event (e.g. keyboard, gamepad).
                 *
                 * Implementations should read input data and update the owner's
                 * control values (throttle, brake, steering) accordingly.
                 *
                 * @param event Input event to process.
                 * @return true if the event was handled and should not be propagated.
                 */
                bool inputEvent( SmartPtr<IInputEvent> event );

                /**
                 * @brief Handle periodic update events (used to process continuous input).
                 *
                 * @param event Update event object (typically contains delta time).
                 * @return true if the event was handled.
                 */
                bool updateEvent( const SmartPtr<IInputEvent> &event );

                /**
                 * @brief Set the priority of this listener in the event dispatch order.
                 *
                 * Higher priority listeners receive events earlier.
                 *
                 * @param priority Priority value (higher processed first).
                 */
                void setPriority( s32 priority ) override;

                /**
                 * @brief Retrieve the current listener priority.
                 *
                 * @return Current priority value.
                 */
                s32 getPriority() const override;

                /**
                 * @brief Get a strong smart-pointer to the owning CarController.
                 *
                 * Returns a `SmartPtr` created from the internal weak reference if the
                 * owner still exists; otherwise returns a null pointer.
                 *
                 * @return SmartPtr<CarController> Strong pointer to owner or null.
                 */
                SmartPtr<CarController> getOwner() const;

                /**
                 * @brief Set the owning CarController for this listener.
                 *
                 * The listener keeps only a weak reference to avoid reference cycles.
                 *
                 * @param owner Strong pointer to the owning `CarController`.
                 */
                void setOwner( SmartPtr<CarController> owner );

            protected:
                WeakPtr<CarController> m_owner;  ///< Weak reference to the owning CarController
                u32 m_priority = 1000;           ///< Event listener priority (default)
            };

            /**
             * @brief Physics callback implementation used by the vehicle subsystem.
             *
             * `VehicleCallback` implements `vehicle::IVehicleCallback` and acts as the
             * bridge between the physics vehicle and the scene representation. It
             * forwards requests for applying forces/torques, performing raycasts and
             * querying actor transform/velocity to the owning `CarController`.
             */
            class VehicleCallback : public vehicle::IVehicleCallback
            {
            public:
                /**
                 * @brief Default-construct an empty callback (no owner set).
                 */
                VehicleCallback();

                /**
                 * @brief Construct a callback and set a raw owning pointer.
                 *
                 * @param controller Raw pointer to the owning `CarController`.
                 *                   The pointer is only used during construction; the
                 *                   class stores a weak reference via `setOwner`.
                 */
                explicit VehicleCallback( CarController *controller );

                /**
                 * @brief Destroy the callback implementation.
                 */
                ~VehicleCallback() override;

                /** @copydoc IVehicleCallback::getData */
                String getData( const String &filePath ) override;

                /** @copydoc IVehicleCallback::addForce */
                void addForce( s32 bodyId, const Vector3<real_Num> &force,
                               const Vector3<real_Num> &pos ) override;

                /** @copydoc IVehicleCallback::addTorque */
                void addTorque( s32 bodyId, const Vector3<real_Num> &torque ) override;

                /** @copydoc IVehicleCallback::addLocalForce */
                void addLocalForce( s32 bodyId, const Vector3<real_Num> &force,
                                    const Vector3<real_Num> &pos ) override;

                /** @copydoc IVehicleCallback::addLocalTorque */
                void addLocalTorque( s32 bodyId, const Vector3<real_Num> &torque ) override;

                /** @copydoc IVehicleCallback::getAngularVelocity */
                Vector3<real_Num> getAngularVelocity() const override;

                /** @copydoc IVehicleCallback::getLinearVelocity */
                Vector3<real_Num> getLinearVelocity() const override;

                /** @copydoc IVehicleCallback::getLocalAngularVelocity */
                Vector3<real_Num> getLocalAngularVelocity() const override;

                /** @copydoc IVehicleCallback::getLocalLinearVelocity */
                Vector3<real_Num> getLocalLinearVelocity() const override;

                /** @copydoc IVehicleCallback::getScale */
                Vector3<real_Num> getScale() const override;

                /** @copydoc IVehicleCallback::getPosition */
                Vector3<real_Num> getPosition() const override;

                /** @copydoc IVehicleCallback::getOrientation */
                Quaternion<real_Num> getOrientation() const override;

                /** @copydoc IVehicleCallback::displayLocalVector */
                void displayLocalVector( s32 Bdy, const Vector3<real_Num> &V,
                                         const Vector3<real_Num> &Org, s32 colour ) const override;

                /** @copydoc IVehicleCallback::displayVector */
                void displayVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                    const Vector3<real_Num> &Org, s32 colour ) const override;

                /** @copydoc IVehicleCallback::displayLocalVector */
                void displayLocalVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                         const Vector3<real_Num> &Org, s32 colour ) const override;

                /** @copydoc IVehicleCallback::getCallbackFunction */
                void *getCallbackFunction() const override;

                /** @copydoc IVehicleCallback::setCallbackFunction */
                void setCallbackFunction( void *callbackFunction ) override;

                /** @copydoc IVehicleCallback::getCallbackDataFunction */
                void *getCallbackDataFunction() const override;

                /** @copydoc IVehicleCallback::setCallbackDataFunction */
                void setCallbackDataFunction( void *callbackDataFunction ) override;

                /** @copydoc IVehicleCallback::getInputData */
                FixedArray<f32, 8> getInputData() const override;

                /** @copydoc IVehicleCallback::getControlAngles */
                FixedArray<f32, 11> getControlAngles() const override;

                /** @copydoc IVehicleCallback::getPointVelocity */
                Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) override;

                /** @copydoc IVehicleCallback::castLocalRay */
                bool castLocalRay( const Ray3<real_Num> &ray,
                                   SmartPtr<physics::IRaycastHit> &data ) override;

                /** @copydoc IVehicleCallback::castWorldRay */
                bool castWorldRay( const Ray3<real_Num> &ray,
                                   SmartPtr<physics::IRaycastHit> &data ) override;

                /**
                 * @brief Get the owning CarController as a strong pointer.
                 *
                 * @return SmartPtr<CarController> Strong pointer to owning controller
                 *         or null if the owner was destroyed.
                 */
                SmartPtr<CarController> getOwner() const;

                /**
                 * @brief Set the owning CarController.
                 *
                 * The callback stores a weak reference to avoid circular ownership.
                 *
                 * @param owner Strong pointer to the CarController that owns this callback.
                 */
                void setOwner( SmartPtr<CarController> owner );

            protected:
                WeakPtr<CarController> m_owner;  ///< Weak reference to the owning CarController
            };

            /**
             * @brief Wheel index enumeration for the four common wheel positions.
             *
             * Used to index into wheel arrays and to express per-wheel configuration.
             */
            enum class Wheels
            {
                FRONT_LEFT,   ///< Front-left wheel index
                FRONT_RIGHT,  ///< Front-right wheel index
                REAR_LEFT,    ///< Rear-left wheel index
                REAR_RIGHT    ///< Rear-right wheel index
            };

            static const String radiusStr;
            static const String suspensionTravelStr;
            static const String dampingStr;
            static const String driveTypeStr;
            static const Array<String> driveTypesEnumTypes;

            /**
             * @brief Construct a CarController with default tuning values.
             *
             * Tuning properties (wheel radius, suspension, damping, etc.) are set to
             * reasonable defaults for a typical passenger car. The physics vehicle
             * itself is not created here; `setVehicleController` may be used to
             * attach a concrete physics implementation.
             */
            CarController();

            /**
             * @brief Destroy the CarController and release resources.
             */
            ~CarController() override;

            /** @copydoc VehicleController::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc VehicleController::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc VehicleController::update */
            void update() override;

            /** @copydoc VehicleController::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc VehicleController::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc VehicleController::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the physics vehicle interface used by this controller.
             *
             * Returns the `vehicle::IVehicle` implementation that receives forces
             * and queries from this controller. May be null if not set.
             *
             * @return SmartPtr<vehicle::IVehicle> Current physics vehicle or null.
             */
            SmartPtr<vehicle::IVehicle> getVehicleController() const;

            /** Rebind visual wheel components after replacing a generated wheel hierarchy. */
            void refreshWheels()
            {
                setupWheels();
            }

            /**
             * @brief Attach a physics vehicle implementation to this controller.
             *
             * @param vehicleController Smart pointer to the physics vehicle instance.
             */
            void setVehicleController( SmartPtr<vehicle::IVehicle> vehicleController );

            /**
             * @brief Read current throttle input.
             *
             * Value is in the range [0.0, 1.0].
             *
             * @return f32 Current throttle.
             */
            f32 getThrottle() const;

            /**
             * @brief Set throttle input.
             *
             * Input will typically be clamped to [0.0, 1.0] by the implementation.
             *
             * @param throttle Desired throttle value.
             */
            void setThrottle( f32 throttle );

            /**
             * @brief Read current brake input.
             *
             * Value is in the range [0.0, 1.0].
             *
             * @return f32 Current brake.
             */
            f32 getBrake() const;

            /**
             * @brief Set brake input.
             *
             * @param brake Desired brake value (0.0 = none, 1.0 = full).
             */
            void setBrake( f32 brake );

            /**
             * @brief Read the steering input in degrees.
             *
             * Negative values indicate left steering, positive values indicate right.
             *
             * @return f32 Steering angle (degrees).
             */
            f32 getSteering() const;

            /**
             * @brief Set steering angle in degrees.
             *
             * The value will be clamped to the configured `m_maxSteeringAngle`.
             *
             * @param steering Steering angle in degrees.
             */
            void setSteering( f32 steering );

            /** Programmatic controls exclude keyboard/gamepad input until usePlayerControls(). */
            void setControls( f32 throttle, f32 brake, f32 steering );
            void usePlayerControls();

            /**
             * @brief Get the configured drive type (FWD/RWD/AWD).
             *
             * @return VehicleDriveType Current drive type.
             */
            VehicleDriveType getDriveType() const;

            /**
             * @brief Set the vehicle drive type.
             *
             * @param driveType Drive configuration to use for power distribution.
             */
            void setDriveType( VehicleDriveType driveType );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handle finite-state-machine events specific to the component.
             *
             * Called by the base `VehicleController` when FSM transitions occur.
             *
             * @param state Current FSM state id.
             * @param eventType FSM event being processed.
             * @return FSMReturnType Result code indicating success or next action.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Create and configure the wheel controllers used by the vehicle.
             *
             * This will populate `m_wheels` and `m_poweredWheels` and set per-wheel
             * parameters such as radius, suspension travel and mass fraction.
             */
            void setupWheels();

            // Vehicle dimensions (meters)
            f32 m_wheelBase = 2.530f;  ///< Distance between front and rear axles
            f32 m_length = 4.405f;     ///< Vehicle overall length
            f32 m_width = 1.810f;      ///< Vehicle overall width
            f32 m_height = 1.170f;     ///< Vehicle overall height

            f32 m_wheelChassisOffset = 0.0f;  ///< Vertical offset of wheels relative to chassis

            // Wheel & suspension tuning parameters
            f32 m_radius = 0.35f;               ///< Wheel radius (m)
            f32 m_suspensionTravel = 0.3f;      ///< Max suspension travel (m)
            f32 m_damping = 1000.0f;            ///< Suspension damping coefficient
            f32 m_inertia = 2.2f;               ///< Wheel rotational inertia (kg*m^2)
            f32 m_grip = 1.0f;                  ///< Tire grip multiplier (0..1)
            f32 m_brakeFrictionTorque = 4000;   ///< Peak service brake torque (N*m)
            f32 m_handbrakeFrictionTorque = 0;  ///< Peak handbrake torque (N*m)
            f32 m_frictionTorque = 10;          ///< Rolling resistance torque (N*m)
            f32 m_maxSteeringAngle = 28.f;      ///< Max steering angle (degrees)
            f32 m_visualSteeringSign = 1.f;     ///< Match the selected physics steering convention
            f32 m_massFraction = 0.25f;         ///< Fraction of vehicle mass allocated per wheel

            // Runtime control inputs
            std::atomic<bool> m_playerControls{ true }, m_joystickActive{ false };
            std::atomic<f32> m_throttle{ 0.0f };  ///< Throttle input [0..1]
            std::atomic<f32> m_brake{ 0.0f };     ///< Brake input [0..1]
            std::atomic<f32> m_steering{ 0.0f };  ///< Steering angle (degrees)

            VehicleDriveType m_driveType = VehicleDriveType::AllWheelDrive;  ///< Drive configuration

            SmartPtr<vehicle::IVehicle> m_vehicleController;  ///< Attached physics vehicle
            SmartPtr<VehicleCallback> m_vehicleCallback;      ///< Callback instance for physics

            // Wheel management
            Array<SmartPtr<WheelController>> m_wheels;  ///< All wheel controllers (size == 4)
            Array<SmartPtr<WheelController>>
                m_poweredWheels;                ///< Subset of wheels that receive drive torque
            Array<real_Num> m_wheelSpinAngles;  ///< Accumulated visual wheel spin in radians
            Array<Quaternion<real_Num>>
                m_wheelBaseOrientations;  ///< Authored local wheel orientations before animation
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CarController_h__
