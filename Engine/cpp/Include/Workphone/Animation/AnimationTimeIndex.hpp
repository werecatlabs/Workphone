#ifndef CAnimationTimeIndex_h__
#define CAnimationTimeIndex_h__

#include <Workphone/Interface/Animation/IAnimationTimeIndex.hpp>

namespace workphone
{
    /**
     * @class AnimationTimeIndex
     * @brief Represents a specific point in an animation's timeline, optionally caching a keyframe index
     * for optimization.
     */
    class AnimationTimeIndex : public IAnimationTimeIndex
    {
    public:
        /** @brief Default constructor. */
        AnimationTimeIndex();

        /**
         * @brief Constructs a time index at a specific position.
         * @param timePos The playback position in seconds.
         */
        AnimationTimeIndex( f32 timePos );

        /**
         * @brief Constructs a time index at a specific position with a known keyframe index.
         * @param timePos The playback position in seconds.
         * @param keyIndex The index of the keyframe at this time.
         */
        AnimationTimeIndex( f32 timePos, u32 keyIndex );

        /** @brief Returns true if a valid keyframe index is cached. */
        bool hasKeyIndex() const override;

        /** @brief Gets the playback position in seconds. */
        f32 getTimePos() const override;

        /** @brief Gets the cached keyframe index. */
        u32 getKeyIndex() const override;

        WP_CLASS_REGISTER_DECL;

    private:
        f32 m_timePos = 0.0f;        ///< Playback position in seconds.
        u32 m_keyIndex = 0;          ///< Cached keyframe index.
        bool m_hasKeyIndex = false;  ///< Whether the key index is currently valid.
    };
}  // namespace workphone

#endif  // CAnimationTimeIndex_h__
