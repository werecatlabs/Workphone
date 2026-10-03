#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationKeyFrame.hpp>
#include <algorithm>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, AnimationKeyFrame, IAnimationKeyFrame );

    AnimationKeyFrame::AnimationKeyFrame() : m_time( 0.0f )
    {
    }

    AnimationKeyFrame::~AnimationKeyFrame()
    {
    }

    f32 AnimationKeyFrame::getTime() const
    {
        return m_time;
    }

    void AnimationKeyFrame::setTime( f32 time )
    {
        m_time = std::max( 0.0f, time );
    }

}  // namespace workphone
