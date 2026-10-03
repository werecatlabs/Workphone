#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Animation/AnimationPoseKeyFrame.hpp"
#include <algorithm>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, AnimationPoseKeyFrame, IAnimationPoseKeyFrame );

    AnimationPoseKeyFrame::AnimationPoseKeyFrame() = default;

    f32 AnimationPoseKeyFrame::getTime() const
    {
        return m_time;
    }

    void AnimationPoseKeyFrame::setTime( f32 time )
    {
        m_time = std::max( 0.0f, time );
    }

    size_t AnimationPoseKeyFrame::getNumPoses() const
    {
        return m_poseReferences.size();
    }

    void AnimationPoseKeyFrame::setNumPoses( size_t numPoses )
    {
        m_poseReferences.resize( numPoses );
    }

    void AnimationPoseKeyFrame::setPoseReference( size_t index, u16 poseIndex, f32 influence )
    {
        if( index >= m_poseReferences.size() )
        {
            m_poseReferences.resize( index + 1 );
        }
        m_poseReferences[index].poseIndex = poseIndex;
        m_poseReferences[index].influence = std::clamp( influence, 0.0f, 1.0f );
    }

    void AnimationPoseKeyFrame::getPoseReference( size_t index, u16 &poseIndex, f32 &influence ) const
    {
        if( index < m_poseReferences.size() )
        {
            poseIndex = m_poseReferences[index].poseIndex;
            influence = m_poseReferences[index].influence;
        }
        else
        {
            poseIndex = 0;
            influence = 0.0f;
        }
    }

}  // namespace workphone
