#ifndef IAnimationTimeIndex_h__
#define IAnimationTimeIndex_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @file IAnimationTimeIndex.hpp
     * @brief Interface for a time-based animation index.
     *
     * This interface describes a single point on an animation timeline. It
     * provides access to the time position used for sampling and, when
     * applicable, the corresponding discrete key (keyframe) index.
     *
     * @note Time units are implementation-defined (commonly seconds). Callers
     *       that require a valid key index should check @c hasKeyIndex() first.
     */

    /**
     * @brief Interface for an AnimationTimeIndex.
     *
     * Implementations represent a specific time position within an animation
     * and optionally map that time to a discrete key index (for example, a
     * keyframe index). This is useful for sampling animations and for systems
     * that need both continuous time and discrete key information.
     */
    class WPCore_API IAnimationTimeIndex : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup through the interface pointer.
         */
        ~IAnimationTimeIndex() override;

        /**
         * @brief Indicates whether a valid key index is available for this time.
         *
         * Some time positions map exactly to a discrete key (keyframe). When
         * this function returns true, @c getKeyIndex() returns a meaningful
         * value. When false, the time position may lie between keys or the
         * implementation may not provide discrete key mapping.
         *
         * @return True if a corresponding key index is available; otherwise false.
         */
        virtual bool hasKeyIndex() const = 0;

        /**
         * @brief Gets the time position for this index.
         *
         * The value is the time used for sampling the animation. Units are
         * implementation-defined (commonly seconds).
         *
         * @return The time position for this animation index.
         */
        virtual f32 getTimePos() const = 0;

        /**
         * @brief Gets the key (keyframe) index corresponding to the time.
         *
         * If @c hasKeyIndex() returns true, this function returns the discrete
         * key index (typically 0-based) that corresponds to the time position.
         * If no key index exists, callers should not rely on the returned value.
         *
         * @return The key index associated with this time position.
         *
         * @note Call @c hasKeyIndex() before using the returned index.
         */
        virtual u32 getKeyIndex() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAnimationTimeIndex_h__
