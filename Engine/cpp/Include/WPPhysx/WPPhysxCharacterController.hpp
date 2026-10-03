#ifndef WPPhysxCharacterController_h__
#define WPPhysxCharacterController_h__

#include <characterkinematic/PxController.h>
#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Physics/CapsuleController.hpp>
#include <foundation/PxVec3.h>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief PhysX implementation of a capsule-based character controller.
         *
         * This class implements the workphone::physics::CapsuleController interface
         * using NVIDIA PhysX primitives. It manages a single `PxCapsuleController`
         * instance and exposes configuration and control functions such as position,
         * orientation, movement, jumping and kinematic/rigid behavior.
         *
         * The controller is tied to a `physx::PxScene` and a `physx::PxControllerManager`.
         * The underlying PhysX objects are created by `createController()` and
         * cleaned up in the destructor (and unload). Callers should set the
         * scene via `setPxScene()` before attempting to load or update the controller.
         */
        class PhysxCharacterController : public CapsuleController
        {
        public:
            /**
             * @brief Construct an empty PhysxCharacterController.
             *
             * The controller object is not fully functional until a PhysX scene
             * has been assigned and `load()` has been called (or `createController()`
             * invoked internally).
             */
            PhysxCharacterController();

            /**
             * @brief Destroy the controller and release any PhysX resources.
             */
            ~PhysxCharacterController() override;

            /**
             * @brief Load controller configuration or resources from shared data.
             * @param data Shared object containing persistent configuration.
             *
             * This should create or initialize any internal PhysX objects
             * required by the controller.
             */
            void load( SmartPtr<ISharedObject> data );

            /**
             * @brief Unload and free resources associated with `data`.
             * @param data Shared object previously passed to `load()`.
             */
            void unload( SmartPtr<ISharedObject> data );

            /**
             * @brief Per-frame update. Synchronizes state with the PhysX controller.
             */
            void update() override;

            /**
             * @brief Set the world-space position of the character.
             * @param position New position to place the controller at.
             */
            void setPosition( const Vector3F &position ) override;

            /**
             * @brief Get the current world-space position of the character.
             * @return The current position.
             */
            Vector3F getPosition() const override;

            /**
             * @brief Set the character's local orientation.
             * @param orientation Quaternion describing orientation.
             */
            void setOrientation( const QuaternionF &orientation ) override;

            /**
             * @brief Get the current local orientation of the character.
             * @return Current orientation quaternion.
             */
            QuaternionF getOrientation() const override;

            /**
             * @brief Get the configured movement speed.
             * @return Linear movement speed in units per second.
             */
            f32 getMoveSpeed() const override;

            /**
             * @brief Set the desired movement speed.
             * @param moveSpeed Linear movement speed in units per second.
             */
            void setMoveSpeed( f32 moveSpeed ) override;

            /**
             * @brief Query whether a jump has been requested.
             * @return True if a jump is pending/active.
             */
            bool getJump() const override;

            /**
             * @brief Request or cancel a jump.
             * @param jump True to request a jump, false to cancel.
             */
            void setJump( bool jump ) override;

            /**
             * @brief Check whether the controller is currently grounded.
             * @return True if controller is touching the ground.
             */
            bool isGrounded() const override;

            /**
             * @brief Retrieve the raw underlying object pointer.
             * @param ppObject Out-pointer which will be set to the underlying object
             *                 (implementation-defined).
             */
            void _getObject( void **ppObject ) override;

            /**
             * @brief Set the desired walking direction vector.
             * @param vector Direction and magnitude of desired walk motion.
             */
            void setWalkVector( const Vector3F &vector ) override;

            /**
             * @brief Stop all movement immediately.
             */
            void stop() override;

            /**
             * @brief Get the physical mass of the controller (if applicable).
             * @return Mass value used by physics simulations.
             */
            physics_Num getMass() const override;

            /**
             * @brief Set the physical mass of the controller.
             * @param mass Mass value to set.
             */
            void setMass( physics_Num mass ) override;

            /**
             * @brief Set the collision type (user-defined layer/type).
             * @param type Collision type identifier.
             */
            void setCollisionType( u32 type ) override;

            /**
             * @brief Get the collision type identifier for this controller.
             * @return Collision type id.
             */
            u32 getCollisionType() const override;

            /**
             * @brief Set the collision mask (bitmask for filtering collisions).
             * @param mask Bitmask defining which collision types to interact with.
             */
            void setCollisionMask( u32 mask ) override;

            /**
             * @brief Get the collision mask used for collision filtering.
             * @return Collision mask bitfield.
             */
            u32 getCollisionMask() const override;

            /**
             * @brief Enable or disable the controller.
             * @param enabled True to enable, false to disable.
             */
            void setEnabled( bool enabled ) override;

            /**
             * @brief Query whether the controller is enabled.
             * @return True if enabled.
             */
            bool isEnabled() const override;

            /**
             * @brief Get the user-defined pointer associated with this controller.
             * @return Pointer previously set with `setUserData()`.
             */
            void *getUserData() const override;

            /**
             * @brief Attach a user-defined pointer to this controller.
             * @param userData Arbitrary pointer stored with the controller.
             */
            void setUserData( void *userData ) override;

            /**
             * @brief Query whether the controller is operating in kinematic mode.
             * @return True when kinematic mode is enabled.
             */
            bool getKinematicMode() const override;

            /**
             * @brief Toggle kinematic mode. Kinematic mode typically means the
             *        controller is driven by user code rather than being fully
             *        simulated by PhysX dynamics.
             * @param kinematicMode True to enable kinematic mode.
             */
            void setKinematicMode( bool kinematicMode ) override;

            /**
             * @brief Get the PxScene associated with this controller.
             * @return Pointer to the PhysX scene (may be null).
             */
            physx::PxScene *getPxScene() const;

            /**
             * @brief Associate the controller with a PhysX scene.
             * @param scene PxScene to use. Must outlive the controller while in use.
             */
            void setPxScene( physx::PxScene *scene );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Helper that creates and configures the internal PxCapsuleController
             *        and related PhysX objects. Called during load/initialization.
             */
            void createController();

            /// Last known state reported by the PhysX controller.
            physx::PxControllerState m_controllerState;
            /// PhysX capsule controller instance owned by this object.
            physx::PxCapsuleController *m_controller;
            /// Controller manager used to create/destroy controllers in the scene.
            physx::PxControllerManager *m_controllerManager;
            /// The PhysX scene this controller is registered with (may be null).
            physx::PxScene *m_scene = nullptr;
            /// Initial position used when creating the controller.
            physx::PxVec3 m_controllerInitialPosition;
            /// Desired movement vector (walk direction and magnitude).
            Vector3F m_moveVector;
            /// When true the controller is driven kinematically rather than by physics.
            bool m_kinematicMode;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxCharacterController_h__
