#ifndef IAnimationPoseKeyFrame_h__
#define IAnimationPoseKeyFrame_h__

#include <Workphone/Interface/Animation/IAnimationKeyFrame.hpp>

namespace workphone
{

    /** Interface for an AnimationPoseKeyFrame. */
    class WPCore_API IAnimationPoseKeyFrame : public IAnimationKeyFrame
    {
    public:
        ~IAnimationPoseKeyFrame() override;

        /** Returns the number of poses referenced by this keyframe. */
        virtual size_t getNumPoses() const = 0;

        /** Sets the number of poses referenced by this keyframe. */
        virtual void setNumPoses( size_t numPoses ) = 0;

        /** Sets the pose reference at the given index.
        @param index The index of the pose reference.
        @param poseIndex The index of the pose.
        @param influence The influence (weight) of the pose.
        */
        virtual void setPoseReference( size_t index, u16 poseIndex, f32 influence ) = 0;

        /** Gets the pose reference at the given index.
        @param index The index of the pose reference.
        @param poseIndex Output: The index of the pose.
        @param influence Output: The influence (weight) of the pose.
        */
        virtual void getPoseReference( size_t index, u16 &poseIndex, f32 &influence ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAnimationPoseKeyFrame_h__
