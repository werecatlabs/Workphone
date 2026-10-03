#ifndef _IPhysicsBody3_H
#define _IPhysicsBody3_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 3D physics body.
         *
         * This class represents the base interface for all physics bodies in a 3D environment.
         * Physics bodies are the main objects that participate in the physics simulation.
         * They can be static, dynamic, or kinematic, and can have various properties like mass,
         * velocity, and forces applied to them.
         *
         * The interface provides functionality for:
         * - Managing body properties (mass, inertia, etc.)
         * - Controlling body motion and forces
         * - Setting up collision filtering
         * - Managing body state and transformations
         * - Handling kinematic behavior
         *
         * @see IRigidBody3
         * @see IRigidDynamic3
         * @see IRigidStatic3
         */
        class WPCore_API IPhysicsBody3 : public ISharedObject
        {
        public:
            /** Message hash for shape attachment */
            static const hash_type STATE_MESSAGE_ATTACH_SHAPE;

            /** Message hash for shape detachment */
            static const hash_type STATE_MESSAGE_DETACH_SHAPE;

            /** Message hash for mass changes */
            static const hash_type STATE_MESSAGE_MASS;

            /** Message hash for inertia tensor changes */
            static const hash_type STATE_MESSAGE_INERTIA_TENSOR;

            /** Flag indicating if the body is enabled */
            static const u32 PhysicsBodyFlagEnabled;

            /** Flag indicating if the body is kinematic */
            static const u32 PhysicsBodyFlagKinematic;

            /** Flag for clearing forces */
            static const u32 PhysicsBodyMotionFlagClearForce;

            /** Flag for clearing torques */
            static const u32 PhysicsBodyMotionFlagClearTorque;

            /** Flag for setting velocity */
            static const u32 PhysicsBodyMotionFlagSetVelocity;

            /** Flag for setting angular velocity */
            static const u32 PhysicsBodyMotionFlagSetAngularVelocity;

            /** Destructor */
            ~IPhysicsBody3() override;

            /**
             * @brief Gets the physics scene this body belongs to.
             * @return A smart pointer to the physics scene.
             */
            virtual SmartPtr<IPhysicsScene3> getScene() const = 0;

            /**
             * @brief Sets the physics scene this body belongs to.
             * @param scene The physics scene to set.
             */
            virtual void setScene( SmartPtr<IPhysicsScene3> scene ) = 0;

            /**
             * @brief Sets the transform of the body.
             * @param transform The new transform to set.
             */
            virtual void setTransform( const Transform3<real_Num> &transform ) = 0;

            /**
             * @brief Gets the transform of the body.
             * @return The current transform of the body.
             */
            virtual Transform3<real_Num> getTransform() const = 0;

            /**
             * @brief Sets an actor flag for the body.
             * @param flag The flag to set.
             * @param value The value to set the flag to.
             */
            virtual void setActorFlag( ActorFlagEnum flag, bool value ) = 0;

            /**
             * @brief Gets the actor flags for the body.
             * @return The current actor flags.
             */
            virtual ActorFlagEnum getActorFlags() const = 0;

            /**
             * @brief Gets the mass of the body.
             * @return The mass of the body.
             */
            virtual real_Num getMass() const = 0;

            /**
             * @brief Sets the mass of the body.
             * @param mass The mass to set.
             */
            virtual void setMass( real_Num mass ) = 0;

            /**
             * @brief Sets the collision type for the body.
             * The collision type determines which objects this body can collide with.
             * @param type The collision type to set.
             */
            virtual void setCollisionType( u32 type ) = 0;

            /**
             * @brief Gets the collision type for the body.
             * @return The current collision type.
             */
            virtual u32 getCollisionType() const = 0;

            /**
             * @brief Sets the collision mask for the body.
             * The collision mask determines which collision types this body can collide with.
             * @param mask The collision mask to set.
             */
            virtual void setCollisionMask( u32 mask ) = 0;

            /**
             * @brief Gets the collision mask for the body.
             * @return The current collision mask.
             */
            virtual u32 getCollisionMask() const = 0;

            /**
             * @brief Sets whether the body is enabled.
             * When disabled, the body will not participate in the physics simulation.
             * @param enabled True to enable the body, false to disable it.
             */
            virtual void setEnabled( bool enabled ) = 0;

            /**
             * @brief Gets whether the body is enabled.
             * @return True if the body is enabled, false otherwise.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Gets user data associated with the body by ID.
             * @param id The ID of the user data to get.
             * @return A pointer to the user data.
             */
            virtual void *getUserDataById( u32 id ) const = 0;

            /**
             * @brief Sets user data associated with the body by ID.
             * @param id The ID of the user data to set.
             * @param userData The user data to set.
             */
            virtual void setUserDataById( u32 id, void *userData ) = 0;

            /**
             * @brief Gets the user data associated with the body.
             * @return A pointer to the user data.
             */
            void *getUserData() const override = 0;

            /**
             * @brief Sets the user data associated with the body.
             * @param userData The user data to set.
             */
            void setUserData( void *userData ) override = 0;

            /**
             * @brief Gets whether the body is in kinematic mode.
             * Kinematic bodies are moved by setting their transforms directly.
             * @return True if the body is kinematic, false otherwise.
             */
            virtual bool getKinematicMode() const = 0;

            /**
             * @brief Sets whether the body is in kinematic mode.
             * @param kinematicMode True to make the body kinematic, false otherwise.
             */
            virtual void setKinematicMode( bool kinematicMode ) = 0;

            /**
             * @brief Creates a clone of this physics body.
             * @return A smart pointer to the cloned body.
             */
            virtual SmartPtr<IPhysicsBody3> clone() = 0;

            /**
             * @brief Wakes up the body if it is sleeping.
             */
            virtual void wakeUp() = 0;

            /**
             * @brief Gets the state context for the body.
             * @return The current state context.
             */
            virtual SmartPtr<IStateContext> getStateContext() const = 0;

            /**
             * @brief Sets the state context for the body.
             * @param stateContext The state context to set.
             */
            virtual void setStateContext( SmartPtr<IStateContext> stateContext ) = 0;

            /**
             * @brief Gets the underlying implementation object.
             * @param object A pointer to store the implementation object.
             */
            virtual void _getObject( void **object ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif
