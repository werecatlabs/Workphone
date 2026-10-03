#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Animation/AnimationTimeIndex.hpp"
#include <algorithm>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, AnimationTimeIndex, IAnimationTimeIndex );

    AnimationTimeIndex::AnimationTimeIndex() = default;

    AnimationTimeIndex::AnimationTimeIndex( f32 timePos ) :
        m_timePos( std::max( 0.0f, timePos ) ),
        m_keyIndex( 0 ),
        m_hasKeyIndex( false )
    {
    }

    AnimationTimeIndex::AnimationTimeIndex( f32 timePos, u32 keyIndex ) :
        m_timePos( std::max( 0.0f, timePos ) ),
        m_keyIndex( keyIndex ),
        m_hasKeyIndex( true )
    {
    }

    bool AnimationTimeIndex::hasKeyIndex() const
    {
        return m_hasKeyIndex;
    }

    f32 AnimationTimeIndex::getTimePos() const
    {
        return m_timePos;
    }

    u32 AnimationTimeIndex::getKeyIndex() const
    {
        return m_keyIndex;
    }

}  // namespace workphone
