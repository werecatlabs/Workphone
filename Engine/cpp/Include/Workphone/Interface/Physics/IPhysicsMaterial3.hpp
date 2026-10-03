#ifndef __IPhysicsMaterial3__H
#define __IPhysicsMaterial3__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 3D physics material.
         *
         * This class represents the physical properties of a material in the 3D physics simulation.
         * Materials define how objects interact with each other during collisions, including
         * properties like friction and restitution (bounciness).
         *
         * The interface provides functionality for:
         * - Setting and getting friction coefficients (static, dynamic, and directional)
         * - Managing restitution (bounciness)
         * - Handling contact information (position and normal)
         * - Managing collision response
         *
         * Materials are typically assigned to physics shapes to control how they behave when
         * colliding with other objects. Friction determines how much objects resist sliding
         * against each other, while restitution determines how much they bounce after impact.
         *
         * @see IPhysicsShape3
         * @see IRigidBody3
         * @see IPhysicsManager
         */
        class WPCore_API IPhysicsMaterial3 : public IResource
        {
        public:
            IPhysicsMaterial3() : IResource( IPhysicsMaterial3::typeInfo() )
            {
            }

            IPhysicsMaterial3( u32 poolTypeId ) : IResource( poolTypeId )
            {
            }

            /** Destructor */
            ~IPhysicsMaterial3() override;

            /**
             * @brief Gets the friction of the material for a given direction.
             * @param direction The direction index (e.g., 0 for X, 1 for Y, 2 for Z).
             * @return The friction coefficient for the specified direction.
             */
            virtual f32 getFriction( s32 direction ) const = 0;

            /**
             * @brief Sets the friction of the material for a given direction.
             * @param friction The friction coefficient to set.
             * @param direction The direction index (e.g., 0 for X, 1 for Y, 2 for Z).
             */
            virtual void setFriction( f32 friction, s32 direction ) = 0;

            /**
             * @brief Gets the dynamic friction of the material for a given direction.
             * @param direction The direction index.
             * @return The dynamic friction coefficient.
             */
            virtual f32 getDynamicFriction( s32 direction ) const = 0;

            /**
             * @brief Sets the dynamic friction of the material for a given direction.
             * @param friction The dynamic friction coefficient to set.
             * @param direction The direction index.
             */
            virtual void setDynamicFriction( f32 friction, s32 direction ) = 0;

            /**
             * @brief Gets the static friction of the material for a given direction.
             * @param direction The direction index.
             * @return The static friction coefficient.
             */
            virtual f32 getStaticFriction( s32 direction ) const = 0;

            /**
             * @brief Sets the static friction of the material for a given direction.
             * @param friction The static friction coefficient to set.
             * @param direction The direction index.
             */
            virtual void setStaticFriction( f32 friction, s32 direction ) = 0;

            /**
             * @brief Gets the restitution (bounciness) of the material.
             * @return The restitution coefficient (0 = no bounce, 1 = maximum bounce).
             */
            virtual f32 getRestitution() const = 0;

            /**
             * @brief Sets the restitution (bounciness) of the material.
             * @param restitution The restitution coefficient to set (0 = no bounce, 1 = maximum bounce).
             */
            virtual void setRestitution( f32 restitution ) = 0;

            /**
             * @brief Gets the contact position for the current collision.
             * @return The contact position as a 3D vector.
             */
            virtual Vector3<real_Num> getContactPosition() const = 0;

            /**
             * @brief Gets the contact normal for the current collision.
             * @return The contact normal as a 3D vector.
             */
            virtual Vector3<real_Num> getContactNormal() const = 0;

            /**
             * @brief Gets the first rigid body involved in the contact.
             * @return A smart pointer to the first rigid body.
             */
            virtual SmartPtr<IRigidBody3> getPhysicsBodyA() const = 0;

            /**
             * @brief Gets the second rigid body involved in the contact.
             * @return A smart pointer to the second rigid body.
             */
            virtual SmartPtr<IRigidBody3> getPhysicsBodyB() const = 0;

            /**
             * @brief Gets the rolling friction (for spheres/capsules).
             * @return Rolling friction coefficient in [0, 1].
             */
            virtual f32 getRollingFriction() const = 0;

            /**
             * @brief Sets the rolling friction (for spheres/capsules).
             * @param friction Rolling friction coefficient (clamped to [0, 1]).
             */
            virtual void setRollingFriction( f32 friction ) = 0;

            /**
             * @brief Gets the combine mode used when this material's friction
             *        meets another material's friction.
             */
            virtual FrictionCombineMode getFrictionCombineMode() const = 0;

            /**
             * @brief Sets the friction combine mode for this material.
             */
            virtual void setFrictionCombineMode( FrictionCombineMode mode ) = 0;

            /**
             * @brief Gets the combine mode used when this material's restitution
             *        meets another material's restitution.
             */
            virtual RestitutionCombineMode getRestitutionCombineMode() const = 0;

            /**
             * @brief Sets the restitution combine mode for this material.
             */
            virtual void setRestitutionCombineMode( RestitutionCombineMode mode ) = 0;

            /**
             * @brief Gets the data-driven material name (registry lookup key).
             */
            virtual String getMaterialName() const = 0;

            /**
             * @brief Sets the data-driven material name (registry lookup key).
             */
            virtual void setMaterialName( const String &name ) = 0;
            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif
