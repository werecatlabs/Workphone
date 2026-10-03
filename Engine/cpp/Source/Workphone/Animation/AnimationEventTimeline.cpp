#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationEventTimeline.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::animation
{
    namespace
    {
        constexpr f32 TIME_EPSILON = 1e-6f;

        f32 clampNormalized( f32 value )
        {
            return std::clamp( value, 0.0f, 1.0f );
        }

        bool crossesForward( f32 previousTime, f32 currentTime, f32 point, bool looped )
        {
            return looped ? point > previousTime + TIME_EPSILON || point <= currentTime
                          : point > previousTime + TIME_EPSILON && point <= currentTime + TIME_EPSILON;
        }

        bool crossesReverse( f32 previousTime, f32 currentTime, f32 point, bool looped )
        {
            return looped ? point < previousTime - TIME_EPSILON || point >= currentTime
                          : point < previousTime - TIME_EPSILON && point >= currentTime - TIME_EPSILON;
        }

        // Adjust the comparison time so that fractional drift from operations like
        // fmod(startTime + duration, 1.0) does not cause an endpoint that
        // mathematically equals previousTime to be treated as crossing the range.
        f32 adjustPreviousTime( f32 previousTime, f32 currentTime, bool looped, bool reverse )
        {
            if( !looped )
            {
                return previousTime;
            }

            // For wrap-around forward (0.9 -> 0.2), the start of the range is just
            // past previousTime. Pull previousTime in by an epsilon so an endTime
            // that rounds back to previousTime is considered 'inside' the wrap.
            if( !reverse && currentTime < previousTime )
            {
                return previousTime + TIME_EPSILON;
            }

            // For wrap-around reverse (0.2 -> 0.9), the start of the range is just
            // before previousTime. Push previousTime up by an epsilon.
            if( reverse && currentTime > previousTime )
            {
                return previousTime - TIME_EPSILON;
            }

            return previousTime;
        }

        bool crossesForwardAdjusted( f32 previousTime, f32 currentTime, f32 point, bool looped )
        {
            const auto adjustedPrevious = adjustPreviousTime( previousTime, currentTime, looped, false );
            return crossesForward( adjustedPrevious, currentTime, point, looped );
        }

        bool crossesReverseAdjusted( f32 previousTime, f32 currentTime, f32 point, bool looped )
        {
            const auto adjustedPrevious = adjustPreviousTime( previousTime, currentTime, looped, true );
            return crossesReverse( adjustedPrevious, currentTime, point, looped );
        }

        f32 traversalDistance( f32 previousTime, f32 point, bool reverse )
        {
            if( reverse )
            {
                return point <= previousTime ? previousTime - point : previousTime + ( 1.0f - point );
            }

            return point >= previousTime ? point - previousTime : ( 1.0f - previousTime ) + point;
        }
    }  // namespace

    bool AnimationEvent::isValid() const
    {
        return !id.empty() && std::isfinite( startTime ) && std::isfinite( duration ) &&
               startTime >= 0.0f && startTime <= 1.0f && duration >= 0.0f && duration <= 1.0f;
    }

    bool AnimationEvent::isActive( f32 normalizedTime ) const
    {
        if( !isValid() || duration <= 0.0f )
        {
            return false;
        }

        const auto time = clampNormalized( normalizedTime );
        if( duration >= 1.0f )
        {
            return true;
        }

        const auto endTime = std::fmod( startTime + duration, 1.0f );
        return endTime < startTime ? time >= startTime || time < endTime
                                   : time >= startTime && time < endTime;
    }

    u32 AnimationEventTimeline::addTrack( const String &name )
    {
        AnimationEventTrack track;
        track.name = name;
        m_tracks.push_back( track );
        return static_cast<u32>( m_tracks.size() - 1 );
    }

    bool AnimationEventTimeline::removeTrack( u32 trackIndex )
    {
        if( trackIndex >= m_tracks.size() )
        {
            return false;
        }

        m_tracks.erase( m_tracks.begin() + trackIndex );
        return true;
    }

    void AnimationEventTimeline::clear()
    {
        m_tracks.clear();
    }

    u32 AnimationEventTimeline::getNumTracks() const
    {
        return static_cast<u32>( m_tracks.size() );
    }

    AnimationEventTrack *AnimationEventTimeline::getTrack( u32 trackIndex )
    {
        return trackIndex < m_tracks.size() ? &m_tracks[trackIndex] : nullptr;
    }

    const AnimationEventTrack *AnimationEventTimeline::getTrack( u32 trackIndex ) const
    {
        return trackIndex < m_tracks.size() ? &m_tracks[trackIndex] : nullptr;
    }

    const Array<AnimationEventTrack> &AnimationEventTimeline::getTracks() const
    {
        return m_tracks;
    }

    u32 AnimationEventTimeline::addEvent( u32 trackIndex, const AnimationEvent &event )
    {
        auto track = getTrack( trackIndex );
        if( !track || !event.isValid() )
        {
            return InvalidIndex;
        }

        track->events.push_back( event );
        sortEvents( *track );
        const auto found = std::find_if(
            track->events.begin(), track->events.end(), [&event]( const AnimationEvent &candidate ) {
                return candidate.id == event.id && candidate.startTime == event.startTime &&
                       candidate.duration == event.duration && candidate.payload == event.payload;
            } );
        return found != track->events.end()
                   ? static_cast<u32>( std::distance( track->events.begin(), found ) )
                   : InvalidIndex;
    }

    bool AnimationEventTimeline::updateEvent( u32 trackIndex, u32 eventIndex,
                                              const AnimationEvent &event )
    {
        auto track = getTrack( trackIndex );
        if( !track || eventIndex >= track->events.size() || !event.isValid() )
        {
            return false;
        }

        track->events[eventIndex] = event;
        sortEvents( *track );
        return true;
    }

    bool AnimationEventTimeline::removeEvent( u32 trackIndex, u32 eventIndex )
    {
        auto track = getTrack( trackIndex );
        if( !track || eventIndex >= track->events.size() )
        {
            return false;
        }

        track->events.erase( track->events.begin() + eventIndex );
        return true;
    }

    Array<SampledAnimationEvent> AnimationEventTimeline::sampleRange( f32 previousTime, f32 currentTime,
                                                                      bool looped, bool reverse ) const
    {
        previousTime = clampNormalized( previousTime );
        currentTime = clampNormalized( currentTime );

        struct PendingSample
        {
            f32 distance = 0.0f;
            f32 phaseTime = 0.0f;
            SampledAnimationEvent sample;
        };

        Array<PendingSample> pending;
        for( u32 trackIndex = 0; trackIndex < m_tracks.size(); ++trackIndex )
        {
            const auto &track = m_tracks[trackIndex];
            if( !track.enabled )
            {
                continue;
            }

            for( u32 eventIndex = 0; eventIndex < track.events.size(); ++eventIndex )
            {
                const auto &event = track.events[eventIndex];
                if( !event.isValid() )
                {
                    continue;
                }

                const auto crossedStart =
                    reverse ? crossesReverse( previousTime, currentTime, event.startTime, looped )
                            : crossesForward( previousTime, currentTime, event.startTime, looped );
                if( crossedStart )
                {
                    PendingSample item;
                    item.distance = traversalDistance( previousTime, event.startTime, reverse );
                    item.phaseTime = event.startTime;
                    item.sample.trackIndex = trackIndex;
                    item.sample.eventIndex = eventIndex;
                    item.sample.phase =
                        reverse ? AnimationEventPhase::Ended : AnimationEventPhase::Started;
                    item.sample.event = event;
                    pending.push_back( item );
                }

                if( event.duration > 0.0f && event.duration < 1.0f )
                {
                    const auto endTime = std::fmod( event.startTime + event.duration, 1.0f );
                    const auto crossedEnd =
                        reverse ? crossesReverse( previousTime, currentTime, endTime, looped )
                                : crossesForward( previousTime, currentTime, endTime, looped );
                    if( crossedEnd )
                    {
                        PendingSample item;
                        item.distance = traversalDistance( previousTime, endTime, reverse );
                        item.phaseTime = endTime;
                        item.sample.trackIndex = trackIndex;
                        item.sample.eventIndex = eventIndex;
                        item.sample.phase =
                            reverse ? AnimationEventPhase::Started : AnimationEventPhase::Ended;
                        item.sample.event = event;
                        pending.push_back( item );
                    }
                }
            }
        }

        std::stable_sort( pending.begin(), pending.end(),
                          []( const PendingSample &a, const PendingSample &b ) {
                              if( a.distance != b.distance )
                              {
                                  return a.distance < b.distance;
                              }
                              if( a.sample.trackIndex != b.sample.trackIndex )
                              {
                                  return a.sample.trackIndex < b.sample.trackIndex;
                              }
                              return a.sample.eventIndex < b.sample.eventIndex;
                          } );

        Array<SampledAnimationEvent> result;
        result.reserve( pending.size() );
        for( const auto &item : pending )
        {
            result.push_back( item.sample );
        }
        return result;
    }

    Array<SampledAnimationEvent> AnimationEventTimeline::getActiveEvents( f32 normalizedTime ) const
    {
        Array<SampledAnimationEvent> result;
        for( u32 trackIndex = 0; trackIndex < m_tracks.size(); ++trackIndex )
        {
            const auto &track = m_tracks[trackIndex];
            if( !track.enabled )
            {
                continue;
            }

            for( u32 eventIndex = 0; eventIndex < track.events.size(); ++eventIndex )
            {
                if( track.events[eventIndex].isActive( normalizedTime ) )
                {
                    result.push_back( { trackIndex, eventIndex, AnimationEventPhase::Started,
                                        track.events[eventIndex] } );
                }
            }
        }
        return result;
    }

    Array<AnimationEvent> AnimationEventTimeline::getSyncMarkers() const
    {
        Array<AnimationEvent> markers;
        for( const auto &track : m_tracks )
        {
            if( !track.enabled )
            {
                continue;
            }
            for( const auto &event : track.events )
            {
                if( event.isSyncMarker && event.isValid() )
                {
                    markers.push_back( event );
                }
            }
        }

        std::stable_sort( markers.begin(), markers.end(),
                          []( const AnimationEvent &a, const AnimationEvent &b ) {
                              return a.startTime < b.startTime;
                          } );
        return markers;
    }

    bool AnimationEventTimeline::isValid() const
    {
        for( const auto &track : m_tracks )
        {
            for( const auto &event : track.events )
            {
                if( !event.isValid() )
                {
                    return false;
                }
            }
        }
        return true;
    }

    void AnimationEventTimeline::sortEvents( AnimationEventTrack &track )
    {
        std::stable_sort( track.events.begin(), track.events.end(),
                          []( const AnimationEvent &a, const AnimationEvent &b ) {
                              return a.startTime < b.startTime;
                          } );
    }
}  // namespace workphone::animation
