#ifndef KeyFrameAnimator2_h__
#define KeyFrameAnimator2_h__

#include <Workphone/Animation/KeyFrameTransform2.hpp>
#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Math/LinearSpline2.hpp>

namespace workphone
{
    /**
     * @class KeyFrameAnimator2
     * @brief Manages 2D animations by interpolating between a series of KeyFrameTransform2 keyframes.
     */
    class KeyFrameAnimator2 : public Animator
    {
    public:
        /** @brief Default constructor. */
        KeyFrameAnimator2();

        ~KeyFrameAnimator2() override;

        /** @brief Updates the animation time and interpolates transformation values. */
        void update() override;

        /** @brief Adds a single keyframe to the animator. */
        void addKeyFrame( const SmartPtr<KeyFrameTransform2> &keyFrame );

        /** @brief Adds a collection of keyframes to the animator. */
        void addKeyFrames( const Array<SmartPtr<KeyFrameTransform2>> &keyFrames );

        /** @brief Sets the total duration of the animation. */
        void setAnimationLength( f32 animationLength ) override;

        /** @brief Gets the keyframe currently being processed. */
        SmartPtr<KeyFrameTransform2> getCurrentKeyFrame() const;

        /** @brief Gets the list of all keyframes in the animation. */
        const Array<SmartPtr<KeyFrameTransform2>> &getKeyFrames() const;

        /** @brief Sets the list of keyframes for the animation. */
        void setKeyFrames( const Array<SmartPtr<KeyFrameTransform2>> &keyFrames );

        WP_CLASS_REGISTER_DECL;

    private:
        SmartPtr<KeyFrameTransform2> m_currentKeyFrame;   ///< The currently active keyframe
        Array<SmartPtr<KeyFrameTransform2>> m_keyFrames;  ///< Collection of keyframes for the animation
        LinearSpline2<real_Num> m_positionSpline;         ///< Spline used for position interpolation
        // For scale and rotation, we will interpolate manually in update()
    };
}  // namespace workphone

#endif  // KeyFrameAnimator2_h__
