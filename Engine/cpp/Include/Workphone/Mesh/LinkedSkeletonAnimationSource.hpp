#ifndef LinkedSkeletonAnimationSource_h__
#define LinkedSkeletonAnimationSource_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @struct LinkedSkeletonAnimationSource
     * @brief Defines a source for skeleton-based animation linked to a specific skeleton.
     *
     * This structure links an animation source to a skeleton by name or by direct pointer,
     * allowing the animation system to apply movements to the correct skeletal hierarchy
     * with an optional scaling factor.
     */
    struct LinkedSkeletonAnimationSource : public ISharedObject
    {
        /** @brief The name of the skeleton this animation source is linked to. */
        String skeletonName;

        /** @brief Direct pointer to the associated skeleton object. */
        SmartPtr<ISkeleton> pSkeleton;

        /** @brief Scaling factor applied to the animation source. */
        f32 scale = 0.0f;

        /**
         * @brief Constructs a linked animation source using a skeleton name and scale.
         * @param skelName The name of the skeleton.
         * @param scl The scale factor.
         */
        LinkedSkeletonAnimationSource( const String &skelName, f32 scl );

        /**
         * @brief Constructs a linked animation source using a skeleton name, scale, and direct skeleton
         * pointer.
         * @param skelName The name of the skeleton.
         * @param scl The scale factor.
         * @param skelPtr Smart pointer to the skeleton.
         */
        LinkedSkeletonAnimationSource( const String &skelName, f32 scl, SmartPtr<ISkeleton> skelPtr );
    };

}  // namespace workphone

#endif  // LinkedSkeletonAnimationSource_h__
