#ifndef CAnimationPoseKeyFrame_h__
#define CAnimationPoseKeyFrame_h__

#include <Workphone/Interface/Animation/IAnimationPoseKeyFrame.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    class AnimationPoseKeyFrame : public IAnimationPoseKeyFrame
    {
    public:
        /** Reference to a pose at a given influence level
         * @remarks
         *   Each keyframe can refer to many poses each at a given influence level.
         */
        struct PoseRef
        {
            /** The linked pose index.
            @remarks
                The Mesh contains all poses for all vertex data in one list, both
                for the shared vertex data and the dedicated vertex data on submeshes.
                The 'target' on the parent track must match the 'target' on the
                linked pose.
            */
            u16 poseIndex;
            /** Influence level of the linked pose.
                1.0 for full influence (full offset), 0.0 for no influence.
            */
            f32 influence;

            PoseRef( u16 p, f32 i ) : poseIndex( p ), influence( i )
            {
            }
        };

        typedef std::vector<PoseRef> PoseRefList;

        AnimationPoseKeyFrame();

        f32 getTime() const override;
        void setTime( f32 time ) override;

        size_t getNumPoses() const override;
        void setNumPoses( size_t numPoses ) override;
        void setPoseReference( size_t index, u16 poseIndex, f32 influence ) override;
        void getPoseReference( size_t index, u16 &poseIndex, f32 &influence ) const override;

        WP_CLASS_REGISTER_DECL;

    private:
        struct PoseReference
        {
            u16 poseIndex = 0;
            f32 influence = 0.0f;
        };

        Array<PoseReference> m_poseReferences;
        f32 m_time = 0.0f;
    };
}  // namespace workphone

#endif  // CAnimationPoseKeyFrame_h__
