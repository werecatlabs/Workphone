#ifndef KeyFrameAnimator_h__
#define KeyFrameAnimator_h__

#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Math/LinearSpline3.hpp>
#include <Workphone/Math/RotationalSpline3.hpp>
#include <Workphone/Animation/KeyFrameTransform3.hpp>

namespace workphone
{
    /**
     * @class KeyFrameAnimator3
     * @brief Manages 3D animations by interpolating between a series of KeyFrameTransform3 keyframes.
     */
    class KeyFrameAnimator3 : public Animator
    {
    public:
        /** @brief Default constructor. */
        KeyFrameAnimator3();
        ~KeyFrameAnimator3() override;

        /** @brief Updates the animation time and interpolates transformation values. */
        void update() override;

        /** @brief Adds a single keyframe to the animator. */
        void addKeyFrame( const SmartPtr<KeyFrameTransform3> &keyFrame );

        /** @brief Adds a collection of keyframes to the animator. */
        void addKeyFrames( const Array<SmartPtr<KeyFrameTransform3>> &keyFrames );

        /** @brief Sets the total duration of the animation. */
        void setAnimationLength( f32 animationLength ) override;

        /** @brief Gets the keyframe currently being processed. */
        SmartPtr<KeyFrameTransform3> getCurrentKeyFrame() const;

        /** @brief Gets the list of all keyframes in the animation. */
        const Array<SmartPtr<KeyFrameTransform3>> &getKeyFrames() const;

        /** @brief Sets the list of keyframes for the animation. */
        void setKeyFrames( const Array<SmartPtr<KeyFrameTransform3>> &keyFrames );

        WP_CLASS_REGISTER_DECL;

    private:
        SmartPtr<KeyFrameTransform3> m_currentKeyFrame;   ///< The currently active keyframe
        Array<SmartPtr<KeyFrameTransform3>> m_keyFrames;  ///< Collection of keyframes for the animation

        LinearSpline3<real_Num> m_positionSpline;         ///< Spline used for position interpolation
        RotationalSpline3<real_Num> m_orientationSpline;  ///< Spline used for orientation interpolation
    };
}  // namespace workphone

#endif  // KeyFrameAnimator_h__
