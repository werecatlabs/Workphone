#ifndef _IAnimator_H
#define _IAnimator_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Use to animate an object.
     */
    class WPCore_API IAnimator : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IAnimator() override;

        /** Starts the animation. */
        virtual void start() = 0;

        /** Stops the animation. */
        virtual void stop() = 0;

        /** Sets whether the animation is looped. */
        virtual void setLoop( bool loop ) = 0;

        /** Gets whether the animation is looped. */
        virtual bool isLoop() const = 0;

        /** Sets whether the animation is reversed. */
        virtual void setReverse( bool reverse ) = 0;

        /** Gets whether the animation is reversed. */
        virtual bool isReverse() const = 0;

        /** Gets whether the animation is finished. */
        virtual bool isFinished() const = 0;

        /** Sets the animation length. */
        virtual void setAnimationLength( f32 animationLength ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
