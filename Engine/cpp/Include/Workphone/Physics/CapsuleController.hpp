#ifndef __CapsuleController_h__
#define __CapsuleController_h__

#include <Workphone/Interface/Physics/ICharacterController3.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Implementation of a capsule-based character controller.
         *
         * This class provides a concrete implementation of the ICharacterController3 interface
         * using a capsule shape for collision detection. It's designed for character movement
         * in 3D physics simulations, providing features like ground detection, jumping,
         * gravity application, and smooth movement control.
         *
         * Features:
         * - Capsule-based collision shape
         * - Ground detection and jumping
         * - Gravity simulation
         * - Smooth movement with configurable speed
         * - State management and serialization support
         * - User data management with multiple slots
         */
        class WPCore_API CapsuleController : public ICharacterController3
        {
        public:
            /** Maximum number of user data slots */
            static const u32 MAX_USER_DATA_SLOTS = 16;

            /** Constructor */
            CapsuleController();

            /** Destructor */
            ~CapsuleController() override;

            // ISharedObject interface
            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            // IPhysicsBody3 interface
            SmartPtr<IPhysicsScene3> getScene() const override;
            void setScene( SmartPtr<IPhysicsScene3> scene ) override;
            void setTransform( const Transform3<real_Num> &transform ) override;
            Transform3<real_Num> getTransform() const override;
            void setActorFlag( ActorFlagEnum flag, bool value ) override;
            ActorFlagEnum getActorFlags() const override;
            real_Num getMass() const override;
            void setMass( real_Num mass ) override;
            void setCollisionType( u32 type ) override;
            u32 getCollisionType() const override;
            void setCollisionMask( u32 mask ) override;
            u32 getCollisionMask() const override;
            void setEnabled( bool enabled ) override;
            bool isEnabled() const override;
            void *getUserDataById( u32 id ) const override;
            void setUserDataById( u32 id, void *userData ) override;
            void *getUserData() const override;
            void setUserData( void *userData ) override;
            bool getKinematicMode() const override;
            void setKinematicMode( bool kinematicMode ) override;
            SmartPtr<IPhysicsBody3> clone() override;
            void wakeUp() override;
            SmartPtr<IStateContext> getStateContext() const override;
            void setStateContext( SmartPtr<IStateContext> stateContext ) override;
            void _getObject( void **object ) const override;

            // ICharacterController3 interface
            void setPosition( const Vector3<real_Num> &position ) override;
            Vector3<real_Num> getPosition() const override;
            void setOrientation( const Quaternion<real_Num> &orientation ) override;
            Quaternion<real_Num> getOrientation() const override;
            f32 getMoveSpeed() const override;
            void setMoveSpeed( f32 moveSpeed ) override;
            bool getJump() const override;
            void setJump( bool jump ) override;
            bool isGrounded() const override;
            void _getObject( void **ppObject ) override;
            void setWalkVector( const Vector3<real_Num> &vector ) override;
            void stop() override;

            // Additional functionality specific to CapsuleController

            /**
             * @brief Updates the character controller's physics simulation.
             * This should be called every frame to apply gravity, movement, and collision detection.
             * @param deltaTime The time elapsed since the last update in seconds.
             */
            virtual void update( real_Num deltaTime );

            /**
             * @brief Sets the radius of the capsule collision shape.
             * @param radius The radius to set (must be positive).
             */
            virtual void setRadius( real_Num radius );

            /**
             * @brief Gets the radius of the capsule collision shape.
             * @return The current radius of the capsule.
             */
            virtual real_Num getRadius() const;

            /**
             * @brief Sets the height of the capsule collision shape.
             * @param height The height to set (must be positive).
             */
            virtual void setHeight( real_Num height );

            /**
             * @brief Gets the height of the capsule collision shape.
             * @return The current height of the capsule.
             */
            virtual real_Num getHeight() const;

            /**
             * @brief Sets the initial velocity applied when jumping.
             * @param jumpVelocity The jump velocity to set (must be non-negative).
             */
            virtual void setJumpVelocity( real_Num jumpVelocity );

            /**
             * @brief Gets the initial velocity applied when jumping.
             * @return The current jump velocity.
             */
            virtual real_Num getJumpVelocity() const;

            /**
             * @brief Sets the gravity acceleration applied to the character.
             * @param gravity The gravity acceleration (typically negative for downward force).
             */
            virtual void setGravity( real_Num gravity );

            /**
             * @brief Gets the gravity acceleration applied to the character.
             * @return The current gravity acceleration.
             */
            virtual real_Num getGravity() const;

            /**
             * @brief Gets the current walk direction vector.
             * @return The normalized walk direction vector.
             */
            virtual Vector3<real_Num> getWalkVector() const;

            /**
             * @brief Gets the current vertical velocity of the character.
             * @return The vertical velocity (positive = upward, negative = downward).
             */
            virtual real_Num getVerticalVelocity() const;

            /**
             * @brief Sets the vertical velocity of the character.
             * @param velocity The vertical velocity to set.
             */
            virtual void setVerticalVelocity( real_Num velocity );

            WP_CLASS_REGISTER_DECL;

            AABB3<real_Num> getAABB() const override;

            void setAABB( const AABB3<real_Num> &aabb ) override;

        protected:
            /** The physics scene this controller belongs to */
            SmartPtr<IPhysicsScene3> m_scene;

            /** State context for managing object state */
            SmartPtr<IStateContext> m_stateContext;

            /** Current transform (position and orientation) */
            Transform3<physics_Num> m_transform;

            /** Current position in world space */
            Vector3<physics_Num> m_position;

            /** Current orientation */
            Quaternion<physics_Num> m_orientation;

            /** Current walking direction vector (normalized) */
            Vector3<physics_Num> m_walkVector;

            /** Character mass in kilograms */
            physics_Num m_mass;

            /** Movement speed in units per second */
            physics_Num m_moveSpeed;

            /** Collision type for filtering */
            u32 m_collisionType;

            /** Collision mask for filtering */
            u32 m_collisionMask;

            /** Actor flags */
            ActorFlagEnum m_actorFlags;

            /** General user data pointer */
            void *m_userData;

            /** User data by ID slots */
            void *m_userDataById[MAX_USER_DATA_SLOTS];

            /** Whether the controller is enabled */
            bool m_isEnabled;

            /** Whether the controller is in kinematic mode */
            bool m_isKinematic;

            /** Whether the character is currently jumping */
            bool m_isJumping;

            /** Whether the character is touching the ground */
            bool m_isGrounded;

            /** Radius of the capsule collision shape */
            physics_Num m_radius = static_cast<physics_Num>( 0.5 );

            /** Height of the capsule collision shape */
            physics_Num m_height = static_cast<physics_Num>( 1.8 );

            /** Initial velocity applied when jumping */
            physics_Num m_jumpVelocity;

            /** Gravity acceleration (typically negative) */
            physics_Num m_gravity;

            /** Current vertical velocity */
            physics_Num m_verticalVelocity;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // __CapsuleController_h__
