#ifndef CAnimationKeyFrame_h__
#define CAnimationKeyFrame_h__

#include <Workphone/Interface/Animation/IAnimationKeyFrame.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>

namespace workphone
{
    /**
     * @class AnimationKeyFrame
     * @brief Base implementation of an animation keyframe, storing the timestamp for a specific point in
     * an animation sequence.
     */
    class WPCore_API AnimationKeyFrame : public IAnimationKeyFrame, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;
        /** @brief Default constructor. */
        AnimationKeyFrame();
        ~AnimationKeyFrame() override;

        /** @brief Gets the timestamp of this keyframe. */
        f32 getTime() const override;

        /** @brief Sets the timestamp of this keyframe. */
        void setTime( f32 time ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        f32 m_time = 0.0f;  ///< The timestamp of the keyframe in seconds.
    };
}  // namespace workphone

#endif  // IAnimationKeyFrame_h__
