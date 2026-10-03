#ifndef AnimationNumericTrack_h__
#define AnimationNumericTrack_h__

#include <Workphone/Interface/Animation/IAnimationNumericTrack.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>

namespace workphone
{
    /**
     * @brief The AnimationNumericTrack class
     * @details Implements a numeric animation track for animating scalar values over time.
     * Stores keyframes as time-value pairs and provides interpolation between them.
     */
    class AnimationNumericTrack : public IAnimationNumericTrack
    {
    public:
        using KeyFrame = Pair<f32, f32>;

        /**
         * @brief Constructor
         */
        AnimationNumericTrack();

        /**
         * @brief Destructor
         */
        ~AnimationNumericTrack() override;

        /**
         * @brief Adds a keyframe to the numeric track.
         * @param time The time position of the keyframe.
         * @param value The numeric value at the keyframe.
         */
        void addKeyFrame( f32 time, f32 value ) override;

        /**
         * @brief Retrieves the value at a specific time position in the track.
         * @param time The time position to retrieve the value for.
         * @return The numeric value at the specified time position.
         */
        f32 getValueAt( f32 time ) const override;

        /**
         * @brief Gets the number of keyframes in the track.
         * @return The number of keyframes.
         */
        u32 getNumKeyFrames() const override;

        /**
         * @brief Clears all keyframes from the track.
         */
        void clearKeyFrames() override;

        const Array<KeyFrame> &getKeyFrames() const;

        void setKeyFrames( const Array<KeyFrame> &keyFrames );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Finds the keyframe indices that bracket the given time
         * @param time The time to search for
         * @param index1 Output: index of the keyframe at or before the time
         * @param index2 Output: index of the keyframe at or after the time
         * @return Interpolation factor between the two keyframes (0.0 to 1.0)
         */
        f32 findKeyFrameIndices( f32 time, u32 &index1, u32 &index2 ) const;

        /**
         * @brief Performs linear interpolation between two values
         * @param value1 First value
         * @param value2 Second value
         * @param factor Interpolation factor (0.0 to 1.0)
         * @return Interpolated value
         */
        f32 interpolate( f32 value1, f32 value2, f32 factor ) const;

        /**
         * @brief Sorts keyframes by time to maintain correct order
         */
        void sortKeyFrames();

        /**
         * @brief Array of keyframes sorted by time
         */
        Array<KeyFrame> m_keyFrames;
    };

}  // namespace workphone

#endif  // AnimationNumericTrack_h__
