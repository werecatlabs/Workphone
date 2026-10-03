#ifndef _IAnimationTextureCtrl_H_
#define _IAnimationTextureCtrl_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief The IAnimationTextureControl class provides control over animation for textures.
         *
         * The animation can be enabled, disabled, looped, reversed, and its time position set.
         */
        class WPCore_API IAnimationTextureControl : public ISharedObject
        {
        public:
            ~IAnimationTextureControl() override;

            /**
             * @brief Enables or disables the texture animation.
             *
             * @param enabled Whether the animation should be enabled.
             * @return true if the animation was successfully enabled or disabled, false otherwise.
             */
            virtual bool setAnimationEnabled( bool enabled ) = 0;

            /**
             * @brief Enables or disables the texture animation at a given time position.
             *
             * @param enabled Whether the animation should be enabled.
             * @param timePosition The time position to set.
             * @return true if the animation was successfully enabled or disabled, false otherwise.
             */
            virtual bool setAnimationEnabled( bool enabled, f32 timePosition ) = 0;

            /**
             * @brief Returns whether the texture animation is enabled or not.
             *
             * @return true if the animation is enabled, false otherwise.
             */
            virtual bool isAnimationEnabled() = 0;

            /**
             * @brief Returns whether the texture animation has ended.
             *
             * @return true if the animation has ended, false otherwise.
             */
            virtual bool hasAnimationEnded() = 0;

            /**
             * @brief Sets whether the texture animation should loop or not.
             *
             * @param loop Whether the animation should loop.
             */
            virtual void setAnimationLoop( bool loop ) = 0;

            /**
             * @brief Returns whether the texture animation is looping or not.
             *
             * @return true if the animation is looping, false otherwise.
             */
            virtual bool isAnimationLooping() const = 0;

            /**
             * @brief Sets the texture animation to play in reverse.
             */
            virtual void setAnimationReversed() = 0;

            /**
             * @brief Returns whether the texture animation is playing in reverse or not.
             *
             * @return true if the animation is playing in reverse, false otherwise.
             */
            virtual bool isAnimationReversed() = 0;

            /**
             * @brief Sets the time position of the texture animation.
             *
             * @param timePosition The time position to set.
             * @return true if the time position was successfully set, false otherwise.
             */
            virtual bool setTimePosition( f32 timePosition ) = 0;

            /**
             * @brief Returns the current time position of the texture animation.
             *
             * @return The current time position.
             */
            virtual f32 getTimePosition() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
