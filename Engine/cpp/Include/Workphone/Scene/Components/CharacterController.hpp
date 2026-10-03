#ifndef CharacterController_h__
#define CharacterController_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Physics/ICharacterController3.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <memory>
#include <unordered_map>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Component that represents a physics-driven character controller.
         *
         * This component encapsulates position, orientation and physics related
         * properties for an actor that can be moved and controlled in the
         * physical simulation. It exposes a small API to query and update the
         * controller's transform, scene association, collision settings,
         * kinematic behaviour and movement-related flags.
         */
        class WPCore_API CharacterController : public Component
        {
        public:
            /**
             * @brief Construct a new Character Controller
             *
             * Initializes internal defaults (mass, move speed, flags, etc.).
             */
            CharacterController();

            /**
             * @brief Destroy the Character Controller
             */
            ~CharacterController() override;

            /**
             * @brief Get the physics scene this controller is attached to.
             * @return SmartPtr to the physics scene, or nullptr if none.
             */
            SmartPtr<physics::IPhysicsScene3> getScene() const;

            /**
             * @brief Attach this controller to a physics scene.
             * @param scene SmartPtr to the target physics scene.
             */
            void setScene( SmartPtr<physics::IPhysicsScene3> scene );

            /**
             * @brief Set the full 3D transform for the controller.
             * @param transform The new transform (position, rotation, scale).
             */
            void setTransform( const Transform3<real_Num> &transform );

            /**
             * @brief Get the current full transform of the controller.
             * @return Transform3 current transform.
             */
            Transform3<real_Num> getTransform() const;

            /**
             * @brief Enable or disable a physics actor flag.
             * @param flag The flag to change.
             * @param value True to enable, false to disable.
             */
            void setActorFlag( physics::ActorFlagEnum flag, bool value );

            /**
             * @brief Get the currently set actor flags.
             * @return The bitmask of actor flags.
             */
            physics::ActorFlagEnum getActorFlags() const;

            /**
             * @brief Get the mass of the controller.
             * @return real_Num mass in engine units.
             */
            real_Num getMass() const;

            /**
             * @brief Set the mass of the controller.
             * @param mass New mass value (must be > 0 in most physics engines).
             */
            void setMass( real_Num mass );

            /**
             * @brief Set the collision type/category for this actor.
             * @param type Collision type bit(s).
             */
            void setCollisionType( u32 type );

            /**
             * @brief Get the collision type/category of this actor.
             * @return u32 Bitmask representing the collision type.
             */
            u32 getCollisionType() const;

            /**
             * @brief Set the collision mask used for collision filtering.
             * @param mask Bitmask specifying which collision layers to interact with.
             */
            void setCollisionMask( u32 mask );

            /**
             * @brief Get the collision mask used for filtering.
             * @return u32 collision mask bitmask.
             */
            u32 getCollisionMask() const;

            /**
             * @brief Retrieve user data previously stored by id.
             * @param id Identifier used to store the user data.
             * @return Void pointer previously set, or nullptr if none.
             */
            void *getUserDataById( u32 id ) const;

            /**
             * @brief Store custom user data associated with an id.
             * @param id Identifier to store the pointer under.
             * @param userData Pointer to user-owned data (not managed by this class).
             */
            void setUserDataById( u32 id, void *userData );

            /**
             * @brief Get the generic user data pointer attached to this controller.
             * @return Pointer previously set via setUserData, or nullptr.
             */
            void *getUserData() const override;

            /**
             * @brief Attach a generic user data pointer to this controller.
             * @param userData Pointer to user-managed data.
             */
            void setUserData( void *userData ) override;

            /**
             * @brief Query whether the controller is currently kinematic.
             * @return true if kinematic (not driven by simulation forces).
             */
            bool getKinematicMode() const;

            /**
             * @brief Set whether the controller should operate in kinematic mode.
             * @param kinematicMode True to set kinematic, false to enable dynamics.
             */
            void setKinematicMode( bool kinematicMode );

            /**
             * @brief Wake up the physics actor (clear sleeping state).
             *
             * Use this to ensure the actor will be simulated immediately after
             * significant programmatic changes.
             */
            void wakeUp();

            /**
             * @brief Set the world-space position of the controller.
             * @param position New position vector.
             */
            void setPosition( const Vector3F &position );

            /**
             * @brief Get the current world-space position.
             * @return Vector3F world position.
             */
            Vector3F getPosition() const;

            /**
             * @brief Set the orientation (rotation) of the controller.
             * @param orientation New orientation quaternion.
             */
            void setOrientation( const Quaternion<real_Num> &orientation );

            /**
             * @brief Get the current orientation quaternion.
             * @return Quaternion current rotation.
             */
            Quaternion<real_Num> getOrientation() const;

            /**
             * @brief Get the configured movement speed multiplier.
             * @return f32 move speed.
             */
            f32 getMoveSpeed() const;

            /**
             * @brief Set the movement speed multiplier used when moving the actor.
             * @param moveSpeed Movement speed scalar.
             */
            void setMoveSpeed( f32 moveSpeed );

            /**
             * @brief Query whether a jump has been requested.
             * @return true if jump is currently requested.
             */
            bool getJump() const;

            /**
             * @brief Set or clear the jump request flag.
             * @param jump True to request a jump, false to clear.
             */
            void setJump( bool jump );

            /**
             * @brief Test whether the controller is considered grounded.
             * @return true if the controller is on the ground.
             */
            bool isGrounded() const;

            /**
             * @brief Set the desired walk vector for character movement.
             * @param vector Direction and magnitude of desired walk movement.
             */
            void setWalkVector( const Vector3<real_Num> &vector );

            /**
             * @brief Stop all movement and clear walk/jump state.
             */
            void stop();

            WP_CLASS_REGISTER_DECL;

        private:
            // Cached transform for the controller (position, rotation, scale).
            Transform3<real_Num> m_transform;

            // Public-facing position and orientation values.
            Vector3F m_position;
            Quaternion<real_Num> m_orientation;

            // Physics scene this controller is registered with.
            SmartPtr<physics::IPhysicsScene3> m_scene;

            // Generic user data pointer (single-slot).
            void *m_userData = nullptr;

            // Movement related properties.
            f32 m_moveSpeed = 1.0f;

            // Physics properties and collision filtering.
            real_Num m_mass = 1.0f;
            u32 m_collisionType = 0;
            u32 m_collisionMask = 0;

            // Actor flags used by the underlying physics implementation.
            physics::ActorFlagEnum m_actorFlags = static_cast<physics::ActorFlagEnum>( 0 );

            // Current input/state flags for this controller.
            bool m_jump = false;
            bool m_grounded = true;
            bool m_kinematicMode = false;
            Vector3<real_Num> m_walkVector;

            // Map of user data pointers keyed by id for multiple attachments.
            std::unordered_map<u32, void *> m_userDataById;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // CharacterController_h__
