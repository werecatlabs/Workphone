#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationMorphKeyFrame.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <algorithm>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AnimationMorphKeyFrame, IAnimationMorphKeyFrame );

    AnimationMorphKeyFrame::AnimationMorphKeyFrame() = default;

    f32 AnimationMorphKeyFrame::getTime() const
    {
        return m_time;
    }

    void AnimationMorphKeyFrame::setTime( f32 time )
    {
        m_time = std::max( 0.0f, time );
    }

    void AnimationMorphKeyFrame::setVertexBuffer( SmartPtr<IVertexBuffer> vertexBuffer )
    {
        m_vertexBuffer = vertexBuffer;
    }

    SmartPtr<IVertexBuffer> AnimationMorphKeyFrame::getVertexBuffer() const
    {
        return m_vertexBuffer;
    }
}  // namespace workphone
