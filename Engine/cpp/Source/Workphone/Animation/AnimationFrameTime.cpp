#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationFrameTime.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::animation
{
    AnimationFrameTime::AnimationFrameTime( s32 frameIndex, f32 percentageThroughFrame ) :
        m_frameIndex( frameIndex ),
        m_percentageThroughFrame( percentageThroughFrame )
    {
        normalise();
    }

    AnimationFrameTime AnimationFrameTime::fromNormalizedTime( f32 normalizedTime, u32 frameCount )
    {
        if( frameCount <= 1 )
        {
            return {};
        }

        const auto clampedTime = std::clamp( normalizedTime, 0.0f, 1.0f );
        const auto frame = clampedTime * static_cast<f32>( frameCount - 1 );
        const auto frameIndex = static_cast<s32>( std::floor( frame ) );
        return AnimationFrameTime( frameIndex, frame - static_cast<f32>( frameIndex ) );
    }

    AnimationFrameTime AnimationFrameTime::fromSeconds( f32 time, f32 framesPerSecond, u32 frameCount )
    {
        if( framesPerSecond <= std::numeric_limits<f32>::epsilon() || frameCount <= 1 )
        {
            return {};
        }

        const auto duration = static_cast<f32>( frameCount - 1 ) / framesPerSecond;
        return fromNormalizedTime( duration > 0.0f ? time / duration : 0.0f, frameCount );
    }

    void AnimationFrameTime::reset()
    {
        m_frameIndex = 0;
        m_percentageThroughFrame = 0.0f;
    }

    bool AnimationFrameTime::isZero() const
    {
        return m_frameIndex == 0 && m_percentageThroughFrame == 0.0f;
    }

    bool AnimationFrameTime::isExactlyAtKeyFrame() const
    {
        return m_percentageThroughFrame == 0.0f;
    }

    s32 AnimationFrameTime::getFrameIndex() const
    {
        return m_frameIndex;
    }

    f32 AnimationFrameTime::getPercentageThroughFrame() const
    {
        return m_percentageThroughFrame;
    }

    s32 AnimationFrameTime::getNearestFrameIndex() const
    {
        return m_frameIndex + ( m_percentageThroughFrame >= 0.5f ? 1 : 0 );
    }

    s32 AnimationFrameTime::getLowerBoundFrameIndex() const
    {
        return m_frameIndex;
    }

    s32 AnimationFrameTime::getUpperBoundFrameIndex() const
    {
        return m_frameIndex + ( m_percentageThroughFrame > 0.0f ? 1 : 0 );
    }

    f32 AnimationFrameTime::toFrame() const
    {
        return static_cast<f32>( m_frameIndex ) + m_percentageThroughFrame;
    }

    f32 AnimationFrameTime::toSeconds( f32 framesPerSecond ) const
    {
        return framesPerSecond > std::numeric_limits<f32>::epsilon() ? toFrame() / framesPerSecond
                                                                     : 0.0f;
    }

    f32 AnimationFrameTime::toNormalizedTime( u32 frameCount ) const
    {
        return frameCount > 1 ? std::clamp( toFrame() / static_cast<f32>( frameCount - 1 ), 0.0f, 1.0f )
                              : 0.0f;
    }

    AnimationFrameTime AnimationFrameTime::operator+( const AnimationFrameTime &other ) const
    {
        return AnimationFrameTime( m_frameIndex + other.m_frameIndex,
                                   m_percentageThroughFrame + other.m_percentageThroughFrame );
    }

    AnimationFrameTime AnimationFrameTime::operator-( const AnimationFrameTime &other ) const
    {
        return AnimationFrameTime( m_frameIndex - other.m_frameIndex,
                                   m_percentageThroughFrame - other.m_percentageThroughFrame );
    }

    AnimationFrameTime &AnimationFrameTime::operator+=( const AnimationFrameTime &other )
    {
        m_frameIndex += other.m_frameIndex;
        m_percentageThroughFrame += other.m_percentageThroughFrame;
        normalise();
        return *this;
    }

    AnimationFrameTime &AnimationFrameTime::operator-=( const AnimationFrameTime &other )
    {
        m_frameIndex -= other.m_frameIndex;
        m_percentageThroughFrame -= other.m_percentageThroughFrame;
        normalise();
        return *this;
    }

    bool AnimationFrameTime::operator<( const AnimationFrameTime &other ) const
    {
        return toFrame() < other.toFrame();
    }

    bool AnimationFrameTime::operator<=( const AnimationFrameTime &other ) const
    {
        return toFrame() <= other.toFrame();
    }

    bool AnimationFrameTime::operator>( const AnimationFrameTime &other ) const
    {
        return toFrame() > other.toFrame();
    }

    bool AnimationFrameTime::operator>=( const AnimationFrameTime &other ) const
    {
        return toFrame() >= other.toFrame();
    }

    bool AnimationFrameTime::operator==( const AnimationFrameTime &other ) const
    {
        return m_frameIndex == other.m_frameIndex &&
               m_percentageThroughFrame == other.m_percentageThroughFrame;
    }

    bool AnimationFrameTime::operator!=( const AnimationFrameTime &other ) const
    {
        return !( *this == other );
    }

    void AnimationFrameTime::normalise()
    {
        if( !std::isfinite( m_percentageThroughFrame ) )
        {
            reset();
            return;
        }

        const auto wholeFrames = static_cast<s32>( std::floor( m_percentageThroughFrame ) );
        m_frameIndex += wholeFrames;
        m_percentageThroughFrame -= static_cast<f32>( wholeFrames );

        if( m_frameIndex < 0 )
        {
            reset();
        }
        else if( std::abs( m_percentageThroughFrame ) <= std::numeric_limits<f32>::epsilon() )
        {
            m_percentageThroughFrame = 0.0f;
        }
    }
}  // namespace workphone::animation
