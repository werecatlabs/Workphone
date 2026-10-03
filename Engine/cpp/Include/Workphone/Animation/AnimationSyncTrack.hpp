#ifndef AnimationSyncTrack_h__
#define AnimationSyncTrack_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Animation/AnimationEventTimeline.hpp>

namespace workphone::animation
{
    struct WPCore_API AnimationSyncTrackTime
    {
        s32 eventIndex = 0;
        f32 percentageThrough = 0.0f;

        bool isValid() const;
        f32 toFloat() const;
    };

    struct WPCore_API AnimationSyncMarker
    {
        String id;
        f32 startTime = 0.0f;

        bool isValid() const;
    };

    /** Event-based clip synchronization adapted to Workphone normalized time. */
    class WPCore_API AnimationSyncTrack
    {
    public:
        AnimationSyncTrack();
        explicit AnimationSyncTrack( const Array<AnimationSyncMarker> &markers,
                                     s32 startEventOffset = 0 );

        static AnimationSyncTrack fromTimeline( const AnimationEventTimeline &timeline,
                                                s32 startEventOffset = 0 );
        static f32 calculateSynchronizedDuration( f32 duration0, f32 duration1, u32 eventCount0,
                                                  u32 eventCount1, f32 blendWeight );

        bool setMarkers( const Array<AnimationSyncMarker> &markers );
        const Array<AnimationSyncMarker> &getMarkers() const;
        u32 getNumEvents() const;

        bool hasEvent( const String &id ) const;
        s32 getEventIndex( const String &id ) const;
        s32 getClosestEventIndex( const AnimationSyncTrackTime &time, const String &id ) const;

        void setStartEventOffset( s32 offset );
        s32 getStartEventOffset() const;

        f32 getEventDuration( s32 eventIndex ) const;
        AnimationSyncTrackTime getTime( f32 normalizedTime ) const;
        f32 getNormalizedTime( const AnimationSyncTrackTime &time ) const;
        AnimationSyncTrackTime updateTime( const AnimationSyncTrackTime &startTime,
                                           f32 normalizedDelta ) const;

        bool isValid() const;

    private:
        s32 wrapIndex( s32 index ) const;
        s32 logicalToRawIndex( s32 logicalIndex ) const;
        s32 rawToLogicalIndex( s32 rawIndex ) const;

        Array<AnimationSyncMarker> m_markers;
        s32 m_startEventOffset = 0;
    };
}  // namespace workphone::animation

#endif  // AnimationSyncTrack_h__
