#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationNumericTrack.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, AnimationNumericTrack, IAnimationNumericTrack );

    AnimationNumericTrack::AnimationNumericTrack()
    {
        // Initialize with a keyframe at time 0.0 with value 0.0
        m_keyFrames.emplace_back( 0.0f, 0.0f );
    }

    AnimationNumericTrack::~AnimationNumericTrack() = default;

    void AnimationNumericTrack::addKeyFrame( f32 time, f32 value )
    {
        time = std::max( 0.0f, time );

        auto existing =
            std::find_if( m_keyFrames.begin(), m_keyFrames.end(), [time]( const KeyFrame &keyFrame ) {
                return std::abs( keyFrame.first - time ) <= std::numeric_limits<f32>::epsilon();
            } );
        if( existing != m_keyFrames.end() )
        {
            existing->second = value;
            return;
        }

        m_keyFrames.emplace_back( time, value );
        sortKeyFrames();
    }

    f32 AnimationNumericTrack::getValueAt( f32 time ) const
    {
        if( m_keyFrames.empty() )
        {
            return 0.0f;
        }

        if( m_keyFrames.size() == 1 )
        {
            return m_keyFrames[0].second;
        }

        // If time is before the first keyframe, return the first value
        if( time <= m_keyFrames[0].first )
        {
            return m_keyFrames[0].second;
        }

        // If time is after the last keyframe, return the last value
        if( time >= m_keyFrames.back().first )
        {
            return m_keyFrames.back().second;
        }

        // Find the keyframes that bracket the given time
        u32 index1, index2;
        f32 factor = findKeyFrameIndices( time, index1, index2 );

        // Interpolate between the two keyframe values
        return interpolate( m_keyFrames[index1].second, m_keyFrames[index2].second, factor );
    }

    u32 AnimationNumericTrack::getNumKeyFrames() const
    {
        return static_cast<u32>( m_keyFrames.size() );
    }

    void AnimationNumericTrack::clearKeyFrames()
    {
        m_keyFrames.clear();
    }

    f32 AnimationNumericTrack::findKeyFrameIndices( f32 time, u32 &index1, u32 &index2 ) const
    {
        // Find the first keyframe whose time is greater than the given time
        auto it =
            std::upper_bound( m_keyFrames.begin(), m_keyFrames.end(), workphone::make_pair( time, 0.0f ),
                              []( const KeyFrame &a, const KeyFrame &b ) { return a.first < b.first; } );

        if( it == m_keyFrames.begin() )
        {
            // Time is before the first keyframe
            index1 = index2 = 0;
            return 0.0f;
        }

        if( it == m_keyFrames.end() )
        {
            // Time is after the last keyframe
            index1 = index2 = static_cast<u32>( m_keyFrames.size() - 1 );
            return 0.0f;
        }

        // Get the indices of the keyframes that bracket the time
        index2 = static_cast<u32>( std::distance( m_keyFrames.begin(), it ) );
        index1 = index2 - 1;

        f32 time1 = m_keyFrames[index1].first;
        f32 time2 = m_keyFrames[index2].first;

        // Calculate interpolation factor
        if( time2 == time1 )
        {
            return 0.0f;
        }

        return ( time - time1 ) / ( time2 - time1 );
    }

    f32 AnimationNumericTrack::interpolate( f32 value1, f32 value2, f32 factor ) const
    {
        // Linear interpolation: value1 + factor * (value2 - value1)
        return value1 + factor * ( value2 - value1 );
    }

    void AnimationNumericTrack::sortKeyFrames()
    {
        std::sort( m_keyFrames.begin(), m_keyFrames.end(),
                   []( const KeyFrame &a, const KeyFrame &b ) { return a.first < b.first; } );
    }

    const Array<AnimationNumericTrack::KeyFrame> &AnimationNumericTrack::getKeyFrames() const
    {
        return m_keyFrames;
    }

    void AnimationNumericTrack::setKeyFrames( const Array<KeyFrame> &keyFrames )
    {
        m_keyFrames = keyFrames;
        sortKeyFrames();
    }

}  // namespace workphone
