#ifndef IPhysicsBodyEffectSnap2_h__
#define IPhysicsBodyEffectSnap2_h__

#include <Workphone/Interface/Physics/IPhysicsBodyEffect2.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a snap effect in 2D physics.
         *
         * This class represents a snap effect that can be applied to physics bodies in 2D space.
         * A snap effect causes a body to move towards a target position, with optional
         * constraints on which axes are affected by the snapping behavior.
         *
         * The interface provides functionality for:
         * - Setting and getting the target position
         * - Controlling which axes are affected by snapping
         * - Managing snap effect properties
         *
         * @see IPhysicsBodyEffect2
         * @see IPhysicsBody2D
         * @see IPhysicsManager2D
         */
        class WPCore_API IPhysicsBodyEffectSnap2 : public IPhysicsBodyEffect2
        {
        public:
            /** Destructor */
            ~IPhysicsBodyEffectSnap2() override;

            /**
             * @brief Gets the target position for the snap effect.
             * @return The target position vector.
             */
            virtual Vector2<real_Num> getTarget() const = 0;

            /**
             * @brief Sets the target position for the snap effect.
             * @param target The target position vector to set.
             */
            virtual void setTarget( const Vector2<real_Num> &target ) = 0;

            /**
             * @brief Checks if snapping is enabled for a specific axis.
             * @param axis The axis to check (0 for X, 1 for Y).
             * @return True if snapping is enabled for the specified axis.
             */
            virtual bool getUseAxis( int axis ) const = 0;

            /**
             * @brief Sets whether snapping is enabled for a specific axis.
             * @param axis The axis to set (0 for X, 1 for Y).
             * @param useAxis True to enable snapping for the axis, false to disable.
             */
            virtual void setUseAxis( int axis, bool useAxis ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // IPhysicsBodyEffectSnap2_h__
