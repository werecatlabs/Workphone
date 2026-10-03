#ifndef IPhysicsEffect2_h__
#define IPhysicsEffect2_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Interface for physics effects in 2D.
         *
         * This class represents the base interface for all physics effects in 2D space.
         * Effects are used to apply forces, impulses, or other influences to physics bodies,
         * such as wind, gravity, or other environmental forces.
         *
         * The interface provides functionality for:
         * - Managing effect properties and behavior
         * - Applying forces and influences to bodies
         * - Controlling effect lifetime and state
         * - Handling effect visualization
         *
         * Effects are commonly used for:
         * - Environmental forces (wind, gravity, etc.)
         * - Force fields and attractors
         * - Explosions and impacts
         * - Special effects and gameplay mechanics
         *
         * @see IPhysicsBodyEffect2
         * @see IPhysicsBody2D
         * @see IPhysicsManager2D
         */
        class WPCore_API IPhysicsEffect2 : public ISharedObject
        {
        public:
            /** Destructor */
            ~IPhysicsEffect2() override;

            /**
             * @brief Gets whether the effect is enabled.
             *
             * When enabled, the effect will actively influence physics bodies.
             * When disabled, the effect will not apply any forces or influences.
             *
             * @return True if the effect is enabled, false otherwise.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Sets whether the effect is enabled.
             *
             * When enabled, the effect will actively influence physics bodies.
             * When disabled, the effect will not apply any forces or influences.
             *
             * @param enabled True to enable the effect, false to disable it.
             */
            virtual void setEnabled( bool enabled ) = 0;

            /**
             * @brief Gets the strength of the effect.
             *
             * The strength determines how strongly the effect influences physics bodies.
             * Higher values make the effect more powerful, while lower values make it weaker.
             *
             * @return The current strength value.
             */
            virtual real_Num getStrength() const = 0;

            /**
             * @brief Sets the strength of the effect.
             *
             * The strength determines how strongly the effect influences physics bodies.
             * Higher values make the effect more powerful, while lower values make it weaker.
             *
             * @param strength The new strength value to set.
             */
            virtual void setStrength( real_Num strength ) = 0;

            /**
             * @brief Handles an event related to the physics effect.
             * @param event A smart pointer to the event to handle.
             */
            virtual void handleEvent( const SmartPtr<IEvent> &event ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsEffect2_h__
