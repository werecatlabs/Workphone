#ifndef __IPhysicsMaterial2__H
#define __IPhysicsMaterial2__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Interface for a 2D physics material.
         *
         * This class represents the physical properties of a material in the 2D physics simulation.
         * Materials define how objects interact with each other during collisions, including
         * properties like friction and restitution (bounciness).
         *
         * The interface provides functionality for:
         * - Setting and getting friction coefficients
         * - Managing restitution (bounciness)
         * - Handling contact information
         * - Managing collision response
         *
         * @see IPhysicsBody2D
         * @see IPhysicsShape2
         * @see IPhysicsManager2D
         */
        class WPCore_API IPhysicsMaterial2 : public ISharedObject
        {
        public:
            IPhysicsMaterial2() : ISharedObject( IPhysicsMaterial2::typeInfo() )
            {
            }

            IPhysicsMaterial2( u32 poolTypeId ) : ISharedObject( poolTypeId )
            {
            }

            /** Destructor */
            ~IPhysicsMaterial2() override;

            /**
             * @brief Sets the friction coefficient for a specific direction.
             *
             * @param friction The friction coefficient to set.
             * @param direction The direction index (0 for X, 1 for Y).
             */
            virtual void setFriction( real_Num friction, s32 direction ) = 0;

            /**
             * @brief Gets the friction coefficient for a specific direction.
             *
             * @param direction The direction index (0 for X, 1 for Y).
             * @return The friction coefficient for the specified direction.
             */
            virtual real_Num getFriction( s32 direction ) const = 0;

            /**
             * @brief Sets the restitution (bounciness) coefficient.
             *
             * @param restitution The restitution coefficient to set (0.0 to 1.0).
             */
            virtual void setRestitution( real_Num restitution ) = 0;

            /**
             * @brief Gets the restitution (bounciness) coefficient.
             *
             * @return The restitution coefficient (0.0 to 1.0).
             */
            virtual real_Num getRestitution() const = 0;

            /**
             * @brief Sets the contact position between two colliding objects.
             *
             * @param position The position of the contact point.
             */
            virtual void setContactPosition( const Vector2<real_Num> &position ) = 0;

            /**
             * @brief Gets the contact position between two colliding objects.
             *
             * @return The position of the contact point.
             */
            virtual Vector2<real_Num> getContactPosition() const = 0;

            /**
             * @brief Sets the contact normal between two colliding objects.
             *
             * @param normal The normal vector at the contact point.
             */
            virtual void setContactNormal( const Vector2<real_Num> &normal ) = 0;

            /**
             * @brief Gets the contact normal between two colliding objects.
             *
             * @return The normal vector at the contact point.
             */
            virtual Vector2<real_Num> getContactNormal() const = 0;

            /**
             * @brief Sets the first physics body involved in the collision.
             *
             * @param body A shared pointer to the first physics body.
             */
            virtual void setPhysicsBodyA( SmartPtr<IPhysicsBody2D> body ) = 0;

            /**
             * @brief Gets the first physics body involved in the collision.
             *
             * @return A shared pointer to the first physics body.
             */
            virtual SmartPtr<IPhysicsBody2D> getPhysicsBodyA() const = 0;

            /**
             * @brief Sets the second physics body involved in the collision.
             *
             * @param body A shared pointer to the second physics body.
             */
            virtual void setPhysicsBodyB( SmartPtr<IPhysicsBody2D> body ) = 0;

            /**
             * @brief Gets the second physics body involved in the collision.
             *
             * @return A shared pointer to the second physics body.
             */
            virtual SmartPtr<IPhysicsBody2D> getPhysicsBodyB() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif
