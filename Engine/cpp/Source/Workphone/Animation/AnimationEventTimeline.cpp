#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationEventTimeline.hpp>
#include <algorithm>
#include <cmath>
#include <type_traits>

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

        // Stable insertion uses only the admitted array. std::stable_sort may
        // allocate an untracked temporary buffer, including during an edit.
        void sortByStartTime( Array<AnimationEvent> &events )
        {
            for( size_t i = 1; i < events.size(); ++i )
            {
                auto event = std::move( events[i] );
                auto position = i;
                while( position && event.startTime < events[position - 1].startTime )
                {
                    events[position] = std::move( events[position - 1] );
                    --position;
                }
                events[position] = std::move( event );
            }
        }
    }  // namespace

    static_assert( std::is_nothrow_move_constructible_v<AnimationEventTrack> &&
                   std::is_nothrow_move_assignable_v<AnimationEventTrack> );
    static_assert( std::is_nothrow_move_constructible_v<AnimationEvent> &&
                   std::is_nothrow_move_assignable_v<AnimationEvent> );
    static_assert( std::is_nothrow_move_constructible_v<AnimationEventTimeline> &&
                   std::is_nothrow_move_assignable_v<AnimationEventTimeline> );

    AnimationEventTrack::AnimationEventTrack( const String &trackName,
                                              const Array<AnimationEvent> &trackEvents,
                                              bool trackEnabled ) :
        name( trackName ),
        events( makeSourceBudgetSnapshot( trackEvents ) ),
        enabled( trackEnabled )
    {
    }

    AnimationEventTrack::AnimationEventTrack( const AnimationEventTrack &other ) :
        AnimationEventTrack( other.name, other.events, other.enabled )
    {
    }

    AnimationEventTrack &AnimationEventTrack::operator=( const AnimationEventTrack &other )
    {
        if( this != &other )
        {
            AnimationEventTrack next( other );
            *this = std::move( next );
        }
        return *this;
    }

    AnimationEventTimeline::AnimationEventTimeline( const AnimationEventTimeline &other ) :
        m_tracks( makeSourceBudgetSnapshot( other.m_tracks ) )
    {
    }

    AnimationEventTimeline &AnimationEventTimeline::operator=( const AnimationEventTimeline &other )
    {
        if( this != &other )
        {
            AnimationEventTimeline next( other );
            m_tracks.swap( next.m_tracks );
        }
        return *this;
    }

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
        m_tracks.push_back( std::move( track ) );
        return static_cast<u32>( m_tracks.size() - 1 );
    }

    bool AnimationEventTimeline::removeTrack( u32 trackIndex )
    {
        if( trackIndex >= m_tracks.size() )
        {
            return false;
        }

        std::move( m_tracks.begin() + trackIndex + 1, m_tracks.end(), m_tracks.begin() + trackIndex );
        m_tracks.pop_back();
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

        // Compute the first matching sorted position before growth or sorting
        // can invalidate an event argument borrowed from this same track.
        size_t resultIndex = 0;
        bool matched = false;
        for( const auto &candidate : track->events )
        {
            if( candidate.startTime < event.startTime )
            {
                ++resultIndex;
            }
            else if( candidate.startTime == event.startTime && !matched )
            {
                matched = candidate.id == event.id && candidate.duration == event.duration &&
                          candidate.payload == event.payload;
                if( !matched )
                {
                    ++resultIndex;
                }
            }
        }

        // A caller can populate getTrack()->events using a borrowed raw Array.
        // Canonicalize that import before publishing any authored mutation.
        AnimationEvent staged( event );
        if( !track->events.get_allocator().isSourceBudgeted() )
        {
            auto next = makeSourceBudgetSnapshot( track->events );
            next.push_back( std::move( staged ) );
            track->events.swap( next );
        }
        else
        {
            track->events.push_back( std::move( staged ) );
        }
        sortEvents( *track );
        return static_cast<u32>( resultIndex );
    }

    bool AnimationEventTimeline::updateEvent( u32 trackIndex, u32 eventIndex,
                                              const AnimationEvent &event )
    {
        auto track = getTrack( trackIndex );
        if( !track || eventIndex >= track->events.size() || !event.isValid() )
        {
            return false;
        }

        AnimationEvent staged( event );
        if( !track->events.get_allocator().isSourceBudgeted() )
        {
            auto next = makeSourceBudgetSnapshot( track->events );
            next[eventIndex] = std::move( staged );
            track->events.swap( next );
        }
        else
        {
            track->events[eventIndex] = std::move( staged );
        }
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

        std::move( track->events.begin() + eventIndex + 1, track->events.end(),
                   track->events.begin() + eventIndex );
        track->events.pop_back();
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
            size_t order = 0;
            SampledAnimationEvent sample;
        };

        auto pending = makeSourceBudgetSnapshot<PendingSample>();
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
                    item.order = pending.size();
                    item.sample.trackIndex = trackIndex;
                    item.sample.eventIndex = eventIndex;
                    item.sample.phase =
                        reverse ? AnimationEventPhase::Ended : AnimationEventPhase::Started;
                    item.sample.event = event;
                    pending.push_back( std::move( item ) );
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
                        item.order = pending.size();
                        item.sample.trackIndex = trackIndex;
                        item.sample.eventIndex = eventIndex;
                        item.sample.phase =
                            reverse ? AnimationEventPhase::Started : AnimationEventPhase::Ended;
                        item.sample.event = event;
                        pending.push_back( std::move( item ) );
                    }
                }
            }
        }

        std::sort( pending.begin(), pending.end(), []( const PendingSample &a, const PendingSample &b ) {
            if( a.distance != b.distance )
            {
                return a.distance < b.distance;
            }
            if( a.sample.trackIndex != b.sample.trackIndex )
            {
                return a.sample.trackIndex < b.sample.trackIndex;
            }
            if( a.sample.eventIndex != b.sample.eventIndex )
            {
                return a.sample.eventIndex < b.sample.eventIndex;
            }
            return a.order < b.order;
        } );

        auto result = makeSourceBudgetSnapshot<SampledAnimationEvent>();
        result.reserve( pending.size() );
        for( auto &item : pending )
        {
            result.push_back( std::move( item.sample ) );
        }
        return result;
    }

    Array<SampledAnimationEvent> AnimationEventTimeline::getActiveEvents( f32 normalizedTime ) const
    {
        auto result = makeSourceBudgetSnapshot<SampledAnimationEvent>();
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
        auto markers = makeSourceBudgetSnapshot<AnimationEvent>();
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

        // Marker exports can concatenate many independently sorted tracks.
        // Sort admitted indices rather than using quadratic insertion or the
        // implementation-owned temporary buffer of std::stable_sort.
        auto order = makeSourceBudgetSnapshot<size_t>();
        order.reserve( markers.size() );
        for( size_t i = 0; i < markers.size(); ++i )
        {
            order.push_back( i );
        }
        std::sort( order.begin(), order.end(), [&markers]( size_t a, size_t b ) {
            if( markers[a].startTime != markers[b].startTime )
            {
                return markers[a].startTime < markers[b].startTime;
            }
            return a < b;
        } );
        for( size_t i = 0; i < order.size(); ++i )
        {
            if( order[i] == i )
            {
                continue;
            }
            auto saved = std::move( markers[i] );
            auto position = i;
            while( order[position] != i )
            {
                const auto next = order[position];
                markers[position] = std::move( markers[next] );
                order[position] = position;
                position = next;
            }
            markers[position] = std::move( saved );
            order[position] = position;
        }
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
        sortByStartTime( track.events );
    }
}  // namespace workphone::animation
