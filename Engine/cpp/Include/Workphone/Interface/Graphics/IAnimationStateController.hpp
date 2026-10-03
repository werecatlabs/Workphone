#ifndef _IAnimationStateController_H_
#define _IAnimationStateController_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace render
    {

        /** The class is used to control animations states.
         */
        class WPCore_API IAnimationStateController : public ISharedObject
        {
        public:
            ~IAnimationStateController() override;

            /** Sets whether or not the animation is enabled. */
            virtual bool setAnimationEnabled( const String &animationName, bool enabled ) = 0;

            /** Sets whether or not the animation is enabled. */
            virtual bool setAnimationEnabled( const String &animationName, bool enabled,
                                              f32 timePosition ) = 0;

            /** Gets a boolean indicating whether or not the animation is enabled. */
            virtual bool isAnimationEnabled( const String &animationName ) = 0;

            /** Stops all the animations. */
            virtual void stopAllAnimations() = 0;

            /** Returns a boolean indicating whether or not the animation has ended. */
            virtual bool hasAnimationEnded( const String &animationName ) = 0;

            /** Gets boolean a indicating whether the animation is present. */
            virtual bool hasAnimation( const String &animationName ) const = 0;

            /** Sets a boolean indicating whether or not an animation loops. */
            virtual void setAnimationLoop( const String &animationName, bool loop ) = 0;

            /** Gets a boolean indicating whether or not an animation loops. */
            virtual bool isAnimationLooping( const String &animationName ) = 0;

            /** Sets whether or not the animation is reversed. */
            virtual void setAnimationReversed( const String &animationName, bool reverse ) = 0;

            /** Gets a boolean indicating whether or not the animation is reversed. */
            virtual bool isAnimationReversed( const String &animationName ) = 0;

            /** Sets the current time position of the animation. */
            virtual bool setTimePosition( const String &animationName, f32 timePosition ) = 0;

            /** Gets the current time position of the animation. */
            virtual f32 getTimePosition( const String &animationName ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
