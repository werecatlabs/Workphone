#ifndef CAnimationVertexTrack_h__
#define CAnimationVertexTrack_h__

#include <Workphone/Interface/Animation/IAnimationVertexTrack.hpp>
#include <Workphone/Animation/AnimationMorphKeyFrame.hpp>
#include <Workphone/Animation/AnimationPoseKeyFrame.hpp>
#include <Workphone/Animation/AnimationTrack.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/Animation/IAnimationTimeIndex.hpp>

namespace workphone
{
    /**
     * @class AnimationVertexTrack
     * @brief Implementation of an animation track specifically for vertex-based animations.
     *
     * This class handles both morph-based and pose-based vertex animations, allowing for
     * vertex deformation over time. It manages a collection of keyframes (either Morph or Pose)
     * and associates them with a vertex buffer.
     */
    class AnimationVertexTrack : public AnimationTrack<IAnimationVertexTrack>
    {
    public:
        AnimationVertexTrack();
        AnimationVertexTrack( VertexAnimationType type, IAnimation *parent = nullptr );
        ~AnimationVertexTrack() override;

        /** @brief Gets the current animation type (Morph or Pose). */
        VertexAnimationType getAnimationType() const override;

        /** @brief Sets the animation type to determine whether to use morph or pose keyframes. */
        void setAnimationType( VertexAnimationType type ) override;

        /** @brief Gets the target mode (e.g., Software or Hardware) for vertex processing. */
        TargetMode getTargetMode() const;
        /** @brief Sets the target mode for vertex processing. */
        void setTargetMode( TargetMode mode );

        /** @brief Retrieves a morph keyframe at the specified index. */
        IAnimationMorphKeyFrame *getVertexMorphKeyFrame( u16 index ) const override;

        /** @brief Retrieves a pose keyframe at the specified index. */
        IAnimationPoseKeyFrame *getVertexPoseKeyFrame( u16 index ) const override;

        /** @brief Associates a vertex buffer with this track for deformation data. */
        void setAssociatedVertexData( IVertexBuffer *data ) override;

        /** @brief Gets the vertex buffer associated with this animation track. */
        IVertexBuffer *getAssociatedVertexData() const override;

        /** @brief Creates a new morph keyframe at the given time position. */
        IAnimationMorphKeyFrame *createVertexMorphKeyFrame( f32 timePos ) override;

        /** @brief Creates a new pose keyframe at the given time position. */
        IAnimationPoseKeyFrame *createVertexPoseKeyFrame( f32 timePos ) override;

        u16 getNumKeyFrames() const override;
        SmartPtr<IAnimationKeyFrame> getKeyFrame( u16 index ) const override;
        SmartPtr<IAnimationKeyFrame> createKeyFrame( f32 timePos ) override;
        void removeKeyFrame( u16 index ) override;
        void removeAllKeyFrames() override;

        /**
         * @brief Finds the two keyframes that bracket the given time index for interpolation.
         * @param timeIndex The time index to query.
         * @param keyFrame1 Output: The first surrounding keyframe.
         * @param keyFrame2 Output: The second surrounding keyframe.
         * @param firstKeyIndex Optional: Output for the index of the first keyframe.
         */
        f32 getKeyFramesAtTime( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                SmartPtr<IAnimationKeyFrame> &keyFrame1,
                                SmartPtr<IAnimationKeyFrame> &keyFrame2,
                                u16 *firstKeyIndex = nullptr ) const override;

        /** @brief Interpolates between keyframes to produce a specific keyframe for the given time. */
        void getInterpolatedKeyFrame( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                      SmartPtr<IAnimationKeyFrame> &kf ) const override;
        bool hasNonZeroKeyFrames() const override;

        /** @brief Optimizes the track by removing redundant keyframes. */
        void optimise() override;

        /** @brief Internal method to collect all keyframe timestamps into an array. */
        void _collectKeyFrameTimes( Array<f32> &keyFrameTimes ) override;

        /** @brief Applies a base keyframe to the track's current state. */
        void _applyBaseKeyFrame( const SmartPtr<IAnimationKeyFrame> &base ) override;

        /** @brief Gets all current morph keyframes. */
        Array<SmartPtr<IAnimationMorphKeyFrame>> getMorphKeyFrames() const;

        /** @brief Sets the collection of morph keyframes for the track. */
        void setMorphKeyFrames( const Array<SmartPtr<IAnimationMorphKeyFrame>> &keyFrames );

        /** @brief Gets all current pose keyframes. */
        Array<SmartPtr<IAnimationPoseKeyFrame>> getPoseKeyFrames() const;

        /** @brief Sets the collection of pose keyframes for the track. */
        void setPoseKeyFrames( const Array<SmartPtr<IAnimationPoseKeyFrame>> &keyFrames );

        WP_CLASS_REGISTER_DECL;

    private:
        VertexAnimationType m_animationType =
            VertexAnimationType::VAT_NONE;                  ///< Current animation type.
        TargetMode m_targetMode = TargetMode::TM_SOFTWARE;  ///< Hardware/Software target mode.
        SmartPtr<IVertexBuffer> m_vertexData;               ///< Associated vertex buffer.
        // Use morph or pose keyframes depending on m_animationType
        Array<SmartPtr<IAnimationMorphKeyFrame>>
            m_morphKeyFrames;  ///< Morph keyframes (used if type is VAT_MORPH).
        Array<SmartPtr<IAnimationPoseKeyFrame>>
            m_poseKeyFrames;  ///< Pose keyframes (used if type is VAT_POSE).
    };
}  // namespace workphone

#endif  // IAnimationVertexTrack_h__
