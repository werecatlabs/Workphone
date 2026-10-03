#ifndef AnimationEventTimeline_h__
#define AnimationEventTimeline_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone::animation
{
    /** An authored event using normalized clip time. Duration may wrap past clip end. */
    struct WPCore_API AnimationEvent
    {
        String id;
        String payload;
        f32 startTime = 0.0f;
        f32 duration = 0.0f;
        bool isSyncMarker = false;

        bool isValid() const;
        bool isActive( f32 normalizedTime ) const;
    };

    struct WPCore_API AnimationEventTrack
    {
        String name;
        Array<AnimationEvent> events;
        bool enabled = true;
    };

    enum class AnimationEventPhase
    {
        Started,
        Ended
    };

    struct WPCore_API SampledAnimationEvent
    {
        u32 trackIndex = 0;
        u32 eventIndex = 0;
        AnimationEventPhase phase = AnimationEventPhase::Started;
        AnimationEvent event;
    };

    /**
     * Runtime/editor-neutral event timeline adapted from Esoterica's event tracks.
     * The class owns authored tracks and can sample forward, reverse, and wrapped ranges.
     */
    class WPCore_API AnimationEventTimeline
    {
    public:
        static constexpr u32 InvalidIndex = static_cast<u32>( -1 );

        u32 addTrack( const String &name );
        bool removeTrack( u32 trackIndex );
        void clear();

        u32 getNumTracks() const;
        AnimationEventTrack *getTrack( u32 trackIndex );
        const AnimationEventTrack *getTrack( u32 trackIndex ) const;
        const Array<AnimationEventTrack> &getTracks() const;

        u32 addEvent( u32 trackIndex, const AnimationEvent &event );
        bool updateEvent( u32 trackIndex, u32 eventIndex, const AnimationEvent &event );
        bool removeEvent( u32 trackIndex, u32 eventIndex );

        Array<SampledAnimationEvent> sampleRange( f32 previousTime, f32 currentTime, bool looped = false,
                                                  bool reverse = false ) const;
        Array<SampledAnimationEvent> getActiveEvents( f32 normalizedTime ) const;
        Array<AnimationEvent> getSyncMarkers() const;

        bool isValid() const;

    private:
        static void sortEvents( AnimationEventTrack &track );

        Array<AnimationEventTrack> m_tracks;
    };
}  // namespace workphone::animation

#endif  // AnimationEventTimeline_h__
