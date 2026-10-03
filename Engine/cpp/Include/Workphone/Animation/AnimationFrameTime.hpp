#ifndef AnimationFrameTime_h__
#define AnimationFrameTime_h__

#include <Workphone/WorkphoneTypes.hpp>

namespace workphone::animation
{
    /** Frame-accurate animation time used by clip importers and editor tooling. */
    class WPCore_API AnimationFrameTime
    {
    public:
        AnimationFrameTime() = default;
        AnimationFrameTime( s32 frameIndex, f32 percentageThroughFrame = 0.0f );

        static AnimationFrameTime fromNormalizedTime( f32 normalizedTime, u32 frameCount );
        static AnimationFrameTime fromSeconds( f32 time, f32 framesPerSecond, u32 frameCount );

        void reset();
        bool isZero() const;
        bool isExactlyAtKeyFrame() const;

        s32 getFrameIndex() const;
        f32 getPercentageThroughFrame() const;
        s32 getNearestFrameIndex() const;
        s32 getLowerBoundFrameIndex() const;
        s32 getUpperBoundFrameIndex() const;

        f32 toFrame() const;
        f32 toSeconds( f32 framesPerSecond ) const;
        f32 toNormalizedTime( u32 frameCount ) const;

        AnimationFrameTime operator+( const AnimationFrameTime &other ) const;
        AnimationFrameTime operator-( const AnimationFrameTime &other ) const;
        AnimationFrameTime &operator+=( const AnimationFrameTime &other );
        AnimationFrameTime &operator-=( const AnimationFrameTime &other );

        bool operator<( const AnimationFrameTime &other ) const;
        bool operator<=( const AnimationFrameTime &other ) const;
        bool operator>( const AnimationFrameTime &other ) const;
        bool operator>=( const AnimationFrameTime &other ) const;
        bool operator==( const AnimationFrameTime &other ) const;
        bool operator!=( const AnimationFrameTime &other ) const;

    private:
        void normalise();

        s32 m_frameIndex = 0;
        f32 m_percentageThroughFrame = 0.0f;
    };
}  // namespace workphone::animation

#endif  // AnimationFrameTime_h__
