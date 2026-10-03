#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationSyncTrack.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace workphone::animation
{
    bool AnimationSyncTrackTime::isValid() const
    {
        return eventIndex >= 0 && std::isfinite( percentageThrough ) && percentageThrough >= 0.0f &&
               percentageThrough <= 1.0f;
    }

    f32 AnimationSyncTrackTime::toFloat() const
    {
        return static_cast<f32>( eventIndex ) + percentageThrough;
    }

    bool AnimationSyncMarker::isValid() const
    {
        return !id.empty() && std::isfinite( startTime ) && startTime >= 0.0f && startTime < 1.0f;
    }

    AnimationSyncTrack::AnimationSyncTrack()
    {
        m_markers.push_back( { String( "Default" ), 0.0f } );
    }

    AnimationSyncTrack::AnimationSyncTrack( const Array<AnimationSyncMarker> &markers,
                                            s32 startEventOffset )
    {
        if( !setMarkers( markers ) )
        {
            m_markers.push_back( { String( "Default" ), 0.0f } );
        }
        setStartEventOffset( startEventOffset );
    }

    AnimationSyncTrack AnimationSyncTrack::fromTimeline( const AnimationEventTimeline &timeline,
                                                         s32 startEventOffset )
    {
        Array<AnimationSyncMarker> markers;
        for( const auto &event : timeline.getSyncMarkers() )
        {
            markers.push_back( { event.id, event.startTime >= 1.0f ? 0.0f : event.startTime } );
        }
        return AnimationSyncTrack( markers, startEventOffset );
    }

    f32 AnimationSyncTrack::calculateSynchronizedDuration( f32 duration0, f32 duration1, u32 eventCount0,
                                                           u32 eventCount1, f32 blendWeight )
    {
        if( eventCount0 == 0 || eventCount1 == 0 )
        {
            return 0.0f;
        }

        const auto commonEventCount = std::lcm( eventCount0, eventCount1 );
        const auto scaledDuration0 =
            std::max( 0.0f, duration0 ) * static_cast<f32>( commonEventCount ) / eventCount0;
        const auto scaledDuration1 =
            std::max( 0.0f, duration1 ) * static_cast<f32>( commonEventCount ) / eventCount1;
        const auto weight = std::clamp( blendWeight, 0.0f, 1.0f );
        return scaledDuration0 + ( scaledDuration1 - scaledDuration0 ) * weight;
    }

    bool AnimationSyncTrack::setMarkers( const Array<AnimationSyncMarker> &markers )
    {
        if( markers.empty() ||
            std::any_of( markers.begin(), markers.end(),
                         []( const AnimationSyncMarker &marker ) { return !marker.isValid(); } ) )
        {
            return false;
        }

        auto sortedMarkers = markers;
        std::stable_sort( sortedMarkers.begin(), sortedMarkers.end(),
                          []( const AnimationSyncMarker &a, const AnimationSyncMarker &b ) {
                              return a.startTime < b.startTime;
                          } );

        const auto duplicate = std::adjacent_find(
            sortedMarkers.begin(), sortedMarkers.end(),
            []( const AnimationSyncMarker &a, const AnimationSyncMarker &b ) {
                return std::abs( a.startTime - b.startTime ) <= std::numeric_limits<f32>::epsilon();
            } );
        if( duplicate != sortedMarkers.end() )
        {
            return false;
        }

        m_markers = sortedMarkers;
        setStartEventOffset( m_startEventOffset );
        return true;
    }

    const Array<AnimationSyncMarker> &AnimationSyncTrack::getMarkers() const
    {
        return m_markers;
    }

    u32 AnimationSyncTrack::getNumEvents() const
    {
        return static_cast<u32>( m_markers.size() );
    }

    bool AnimationSyncTrack::hasEvent( const String &id ) const
    {
        return std::any_of( m_markers.begin(), m_markers.end(),
                            [&id]( const AnimationSyncMarker &marker ) { return marker.id == id; } );
    }

    s32 AnimationSyncTrack::getEventIndex( const String &id ) const
    {
        for( s32 logicalIndex = 0; logicalIndex < static_cast<s32>( m_markers.size() ); ++logicalIndex )
        {
            if( m_markers[logicalToRawIndex( logicalIndex )].id == id )
            {
                return logicalIndex;
            }
        }
        return 0;
    }

    s32 AnimationSyncTrack::getClosestEventIndex( const AnimationSyncTrackTime &time,
                                                  const String &id ) const
    {
        if( m_markers.empty() )
        {
            return 0;
        }

        s32 bestIndex = getEventIndex( id );
        auto bestDistance = std::numeric_limits<f32>::max();
        const auto eventCount = static_cast<f32>( m_markers.size() );
        const auto timePosition = std::fmod( time.toFloat(), eventCount );
        for( s32 logicalIndex = 0; logicalIndex < static_cast<s32>( m_markers.size() ); ++logicalIndex )
        {
            if( m_markers[logicalToRawIndex( logicalIndex )].id != id )
            {
                continue;
            }

            const auto directDistance = std::abs( static_cast<f32>( logicalIndex ) - timePosition );
            const auto wrappedDistance = eventCount - directDistance;
            const auto distance = std::min( directDistance, wrappedDistance );
            if( distance < bestDistance )
            {
                bestDistance = distance;
                bestIndex = logicalIndex;
            }
        }
        return bestIndex;
    }

    void AnimationSyncTrack::setStartEventOffset( s32 offset )
    {
        m_startEventOffset = wrapIndex( offset );
    }

    s32 AnimationSyncTrack::getStartEventOffset() const
    {
        return m_startEventOffset;
    }

    f32 AnimationSyncTrack::getEventDuration( s32 eventIndex ) const
    {
        if( m_markers.empty() )
        {
            return 1.0f;
        }

        const auto rawIndex = logicalToRawIndex( eventIndex );
        const auto nextRawIndex = wrapIndex( rawIndex + 1 );
        auto endTime = m_markers[nextRawIndex].startTime;
        if( nextRawIndex <= rawIndex )
        {
            endTime += 1.0f;
        }
        return endTime - m_markers[rawIndex].startTime;
    }

    AnimationSyncTrackTime AnimationSyncTrack::getTime( f32 normalizedTime ) const
    {
        if( m_markers.empty() )
        {
            return {};
        }

        normalizedTime = normalizedTime - std::floor( normalizedTime );
        s32 rawIndex = static_cast<s32>( m_markers.size() - 1 );
        for( s32 i = 0; i < static_cast<s32>( m_markers.size() ); ++i )
        {
            const auto nextIndex = wrapIndex( i + 1 );
            const auto start = m_markers[i].startTime;
            auto end = m_markers[nextIndex].startTime;
            auto sampleTime = normalizedTime;
            if( nextIndex <= i )
            {
                end += 1.0f;
                if( sampleTime < start )
                {
                    sampleTime += 1.0f;
                }
            }

            if( sampleTime >= start && sampleTime < end )
            {
                rawIndex = i;
                break;
            }
        }

        const auto logicalIndex = rawToLogicalIndex( rawIndex );
        const auto duration = getEventDuration( logicalIndex );
        auto sampleTime = normalizedTime;
        const auto startTime = m_markers[rawIndex].startTime;
        if( sampleTime < startTime )
        {
            sampleTime += 1.0f;
        }
        return { logicalIndex, duration > std::numeric_limits<f32>::epsilon()
                                   ? std::clamp( ( sampleTime - startTime ) / duration, 0.0f, 1.0f )
                                   : 0.0f };
    }

    f32 AnimationSyncTrack::getNormalizedTime( const AnimationSyncTrackTime &time ) const
    {
        if( m_markers.empty() || !time.isValid() )
        {
            return 0.0f;
        }

        const auto logicalIndex = wrapIndex( time.eventIndex );
        const auto rawIndex = logicalToRawIndex( logicalIndex );
        auto normalizedTime =
            m_markers[rawIndex].startTime + getEventDuration( logicalIndex ) * time.percentageThrough;
        normalizedTime -= std::floor( normalizedTime );
        return normalizedTime;
    }

    AnimationSyncTrackTime AnimationSyncTrack::updateTime( const AnimationSyncTrackTime &startTime,
                                                           f32 normalizedDelta ) const
    {
        return getTime( getNormalizedTime( startTime ) + normalizedDelta );
    }

    bool AnimationSyncTrack::isValid() const
    {
        return !m_markers.empty() &&
               std::all_of( m_markers.begin(), m_markers.end(),
                            []( const AnimationSyncMarker &marker ) { return marker.isValid(); } );
    }

    s32 AnimationSyncTrack::wrapIndex( s32 index ) const
    {
        if( m_markers.empty() )
        {
            return 0;
        }
        const auto count = static_cast<s32>( m_markers.size() );
        return ( index % count + count ) % count;
    }

    s32 AnimationSyncTrack::logicalToRawIndex( s32 logicalIndex ) const
    {
        return wrapIndex( logicalIndex + m_startEventOffset );
    }

    s32 AnimationSyncTrack::rawToLogicalIndex( s32 rawIndex ) const
    {
        return wrapIndex( rawIndex - m_startEventOffset );
    }
}  // namespace workphone::animation
