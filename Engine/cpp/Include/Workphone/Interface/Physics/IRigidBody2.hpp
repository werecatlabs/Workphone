#ifndef __IPhysicsRigidBody2__H
#define __IPhysicsRigidBody2__H

#include <Workphone/Interface/Physics/IPhysicsBody2D.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Interface for a 2D rigid body.
         *
         * This class represents a rigid body in the 2D physics simulation.
         * Rigid bodies are solid objects that can move, rotate, and collide with
         * other objects while maintaining their shape. They can be static, dynamic,
         * or kinematic.
         *
         * The interface provides functionality for:
         * - Managing body properties (mass, inertia, etc.)
         * - Controlling motion and forces
         * - Handling collision detection
         * - Managing body state and constraints
         *
         * @see IPhysicsBody2D
         * @see IPhysicsShape2
         * @see IPhysicsManager2D
         */
        class WPCore_API IRigidBody2 : public IPhysicsBody2D
        {
        public:
            /** Flag to constrain the body within bounds */
            static const u32 RBF_CONSTRAINBOUNDS = ( 1 << 0 );

            /** Flag to cap the body's screen position */
            static const u32 RBF_CAPSCREENPOSITION = ( 1 << 1 );

            /** Flag to enable collision detection */
            static const u32 RBF_ENABLE_COLLISION = ( 1 << 2 );

            /** Flag to dampen linear velocity */
            static const u32 RBF_DAMPLINEARVELOCITY = ( 1 << 3 );

            /** Flag to enable physics simulation */
            static const u32 RBF_ENABLEPHYSICS = ( 1 << 4 );

            /** Flag to constrain motion along X axis */
            static const u32 RBF_CONSTRAIN_X = ( 1 << 5 );

            /** Flag to constrain motion along Y axis */
            static const u32 RBF_CONSTRAIN_Y = ( 1 << 6 );

            /** Flag to allow rotation */
            static const u32 RBF_ALLOWROTATION = ( 1 << 7 );

            /** Flag to enable collision between same type bodies */
            static const u32 RBF_SAME_TYPE_COLLISION = ( 1 << 8 );

            /** Flag to enable collision with particles */
            static const u32 RBF_ENABLE_PARTICLE_COLLISION = ( 1 << 9 );

            /** Flag to use target position for X axis */
            static const u32 RBF_USE_TARGET_POSITION_X = ( 1 << 10 );

            /** Flag to use target position for Y axis */
            static const u32 RBF_USE_TARGET_POSITION_Y = ( 1 << 11 );

            /** Flag to enable the body */
            static const u32 RBF_ENABLE = ( 1 << 12 );

            /** Destructor */
            ~IRigidBody2() override;

            /**
             * @brief Sets the collision shape for the rigid body.
             * @param shape The collision shape to attach to the body.
             */
            virtual void setCollisionShape( const SmartPtr<IPhysicsShape2> &shape ) = 0;

            /**
             * @brief Gets the collision shape of the rigid body.
             * @return A shared pointer to the body's collision shape.
             */
            virtual const SmartPtr<IPhysicsShape2> &getCollisionShape() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif
