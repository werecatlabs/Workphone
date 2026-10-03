#ifndef __WP_IRigidBody3__H
#define __WP_IRigidBody3__H

#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a rigid body.
         *
         * This class represents a rigid body in a 3D physics simulation. It is an abstract base class
         * that defines the basic interface for a rigid body, including methods for setting and getting
         * rigid body flags, adding and removing shapes to the body, and getting an array of the attached
         * shapes. It also provides methods for setting and getting the linear and angular velocity of
         * the body, as well as for adding forces and torques to the body.
         *
         * The `IRigidBody3` class is intended to be subclassed to create specific types of rigid bodies,
         * such as static or dynamic bodies. Each subclass should implement the pure virtual methods
         * defined in this class to provide the specific functionality for the body.
         *
         * @see IPhysicsBody3
         */
        class WPCore_API IRigidBody3 : public IPhysicsBody3
        {
        public:
            /** Destructor. */
            ~IRigidBody3() override;

            /**
             * @brief Sets a rigid body flag.
             *
             * @param flag The flag to set.
             * @param value The value to set the flag to.
             */
            virtual void setRigidBodyFlag( RigidBodyFlagEnum flag, bool value ) = 0;

            /**
             * @brief Gets the rigid body flags.
             *
             * @return The rigid body flags.
             */
            virtual RigidBodyFlagEnum getRigidBodyFlags() const = 0;

            /**
             * @brief Adds a shape to the body.
             * @param shape The shape to add.
             */
            virtual void addShape( SmartPtr<IPhysicsShape3> shape ) = 0;

            /**
             * @brief Removes a shape from the body.
             * @param shape The shape to remove.
             * @param wakeOnLostTouch Whether to wake the body if it loses contact with another object.
             */
            virtual void removeShape( SmartPtr<IPhysicsShape3> shape, bool wakeOnLostTouch = true ) = 0;

            /**
             * @brief Gets an array of the attached shapes.
             * @return The array of attached shapes.
             */
            virtual Array<SmartPtr<IPhysicsShape3>> getShapes() const = 0;

            /**
             * @brief Returns the number of shapes attached to the actor.
             * @return The number of shapes attached to the actor.
             */
            virtual u32 getNumShapes() const = 0;

            /**
             * @brief Sets the linear velocity of the body.
             *
             * @param linVel The linear velocity to set.
             * @param autowake Whether to automatically wake the body.
             */
            virtual void setLinearVelocity( const Vector3<real_Num> &linVel, bool autowake = true ) = 0;

            /**
             * @brief Gets the linear velocity of the body.
             *
             * @return The linear velocity of the body.
             */
            virtual Vector3<real_Num> getLinearVelocity() const = 0;

            /**
             * @brief Sets the angular velocity of the body.
             *
             * @param angVel The angular velocity to set.
             * @param autowake Whether to automatically wake the body.
             */
            virtual void setAngularVelocity( const Vector3<real_Num> &angVel, bool autowake = true ) = 0;

            /**
             * @brief Gets the current angular velocity of the rigid body.
             *
             * This function returns the current angular velocity of the rigid body in radians per
             * second.
             *
             * @return The current angular velocity of the rigid body.
             */
            virtual Vector3<real_Num> getAngularVelocity() const = 0;

            /** Applies a force to the rigid body at its center of mass.
             * @param force The force to apply.
             */
            virtual void addForce( const Vector3<real_Num> &force ) = 0;

            /** Clears the accumulated forces applied to the rigid body.
             * @param mode The mode of the force to clear (Force, Impulse, VelocityChange, Acceleration).
             */
            virtual void clearForce( ForceModeEnum mode = ForceModeEnum::Force ) = 0;

            /** Applies a torque to the rigid body.
             * @param torque The torque to apply.
             */
            virtual void addTorque( const Vector3<real_Num> &torque ) = 0;

            /** Clears the accumulated torques applied to the rigid body.
             * @param mode The mode of the torque to clear (Force, Impulse, VelocityChange,
             * Acceleration).
             */
            virtual void clearTorque( ForceModeEnum mode = ForceModeEnum::Force ) = 0;

            /** Gets the local axis-aligned bounding box of the rigid body.
             * @return The local AABB.
             */
            virtual AABB3<real_Num> getLocalAABB() const = 0;

            /** Gets the world axis-aligned bounding box of the rigid body.
             * @return The world AABB.
             */
            virtual AABB3<real_Num> getWorldAABB() const = 0;

            /** Sets the center of mass pose of the rigid body.
             * @param pose The new center of mass pose.
             */
            virtual void setCMassLocalPose( const Transform3<real_Num> &pose ) = 0;

            /** Gets the center of mass pose of the rigid body.
             * @return The center of mass pose.
             */
            virtual Transform3<real_Num> getCMassLocalPose() const = 0;

            /** Sets the mass space inertia tensor of the rigid body.
             * @param m The new mass space inertia tensor.
             */
            virtual void setMassSpaceInertiaTensor( const Vector3<real_Num> &m ) = 0;

            /** Gets the mass space inertia tensor of the rigid body.
             * @return The mass space inertia tensor.
             */
            virtual Vector3<real_Num> getMassSpaceInertiaTensor() const = 0;

            /** Gets the inverse mass space inertia tensor of the rigid body.
             * @return The inverse mass space inertia tensor.
             */
            virtual Vector3<real_Num> getMassSpaceInvInertiaTensor() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif
