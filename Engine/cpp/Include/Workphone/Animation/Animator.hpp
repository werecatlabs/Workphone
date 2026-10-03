#ifndef _BaseAnimator_H
#define _BaseAnimator_H

#include <Workphone/Interface/Animation/IAnimator.hpp>
#include <functional>

namespace workphone
{
    /**
     * @class Animator
     * @brief Base class for all animators, providing core logic for timing, playback control, and
     * loop/reverse functionality.
     */
    class WPCore_API Animator : public IAnimator
    {
    public:
        /** @brief Callback type executed when the animation completes. */
        using OnCompleteCallback = std::function<void()>;

        static const hash_type START_HASH;
        static const hash_type STOP_HASH;
        static const hash_type PAUSE_HASH;
        static const hash_type RESUME_HASH;
        static const hash_type COMPLETE_HASH;

        /** @brief Default constructor. */
        Animator();
        ~Animator() override;

        /** @brief Core update logic called per frame to advance the animation. */
        void update() override;

        /** @brief Starts or restarts the animation. */
        void start() override;

        /** @brief Stops the animation and resets its state. */
        void stop() override;

        /** @brief Pauses the animation playback. */
        virtual void pause();

        /** @brief Resumes the animation from a paused state. */
        virtual void resume();

        /** @brief Returns true if the animation is currently playing. */
        bool isPlaying() const;

        /** @brief Returns true if the animation is currently paused. */
        bool isPaused() const;

        /** @brief Enables or disables looping. */
        void setLoop( bool loop ) override;

        /** @brief Checks if looping is enabled. */
        bool isLoop() const override;

        /** @brief Enables or disables reverse playback. */
        void setReverse( bool reverse ) override;

        /** @brief Checks if reverse playback is enabled. */
        bool isReverse() const override;

        /** @brief Returns true if the animation has reached the end of its duration. */
        bool isFinished() const override;

        /** @brief Sets the total duration of the animation in seconds. */
        void setAnimationLength( f32 animationLength ) override;

        /** @brief Gets the total duration of the animation in seconds. */
        virtual f32 getAnimationLength() const;

        /** @brief Gets the current playback position in seconds. */
        virtual f32 getAnimationTime() const;

        /** @brief Sets the current playback position in seconds. */
        virtual void setAnimationTime( f32 animationTime );

        /** @brief Sets the playback speed multiplier. */
        virtual void setAnimationSpeed( f32 speed );

        /** @brief Gets the current playback speed multiplier. */
        virtual f32 getAnimationSpeed() const;

        /** @brief Sets the callback to be invoked upon animation completion. */
        virtual void setOnCompleteCallback( OnCompleteCallback callback );

        WP_CLASS_REGISTER_DECL;

    protected:
        /** @brief Internal hook called when animation starts. */
        virtual void onStart();

        /** @brief Internal hook called when animation stops. */
        virtual void onStop();

        /** @brief Internal hook called when animation is paused. */
        virtual void onPause();

        /** @brief Internal hook called when animation is resumed. */
        virtual void onResume();

        /** @brief Internal hook called when animation completes. */
        virtual void onComplete();

        f32 m_animationLength;  ///< Total duration of the animation.
        f32 m_animationTime;    ///< Current elapsed time.
        f32 m_animationSpeed;   ///< Playback speed multiplier.

        bool m_isPlaying;  ///< Playback status.
        bool m_isPaused;   ///< Pause status.
        bool m_loop;       ///< Looping flag.
        bool m_reverse;    ///< Reverse playback flag.

        OnCompleteCallback m_onCompleteCallback;  ///< Callback to execute on completion.

        static u32 m_idExt;  ///< External ID counter for registration.
    };
}  // namespace workphone

#endif
