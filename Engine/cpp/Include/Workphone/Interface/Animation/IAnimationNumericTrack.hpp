#ifndef IAnimationNumericTrack_h__
#define IAnimationNumericTrack_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @class IAnimationNumericTrack
     * @brief Represents a numeric animation track for animating scalar values.
     *
     * The AnimationNumericTrack class provides functionality to manage keyframes
     * for animating numeric values over time. It implements the IAnimationNumericTrack interface.
     */
    class IAnimationNumericTrack : public ISharedObject
    {
    public:
        /**
         * @brief Destroys the AnimationNumericTrack object.
         */
        ~IAnimationNumericTrack() override;

        /**
         * @brief Adds a keyframe to the numeric track.
         * @param time The time position of the keyframe.
         * @param value The numeric value at the keyframe.
         */
        virtual void addKeyFrame( f32 time, f32 value ) = 0;

        /**
         * @brief Retrieves the value at a specific time position in the track.
         * @param time The time position to retrieve the value for.
         * @return The numeric value at the specified time position.
         */
        virtual f32 getValueAt( f32 time ) const = 0;

        /**
         * @brief Gets the number of keyframes in the track.
         * @return The number of keyframes.
         */
        virtual u32 getNumKeyFrames() const = 0;

        /**
         * @brief Clears all keyframes from the track.
         */
        virtual void clearKeyFrames() = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IAnimationNumericTrack_h__
