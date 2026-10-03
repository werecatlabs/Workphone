#ifndef IAnimationVertexTrack_h__
#define IAnimationVertexTrack_h__

#include <Workphone/Interface/Animation/IAnimationTrack.hpp>

namespace workphone
{
    /**
     * @brief Interface for an animation track that modifies vertex data.
     *
     * A vertex animation track contains key frames which modify vertex-level
     * attributes (for example morph targets or pose-based deformations).
     * Implementations are responsible for storing key frames, applying them
     * to the associated vertex data and interpolating between key frames.
     */
    class WPCore_API IAnimationVertexTrack : public IAnimationTrack
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures derived destructors are called when deleting via a base pointer.
         */
        ~IAnimationVertexTrack() override;

        /**
         * @brief Get the vertex animation type for this track.
         *
         * The animation type identifies how this track treats vertex key frames
         * (for example morph animation or single-pose animation).
         *
         * @return The current VertexAnimationType for this track.
         */
        virtual VertexAnimationType getAnimationType() const = 0;

        /**
         * @brief Set the vertex animation type for this track.
         *
         * Changing the animation type may affect how key frames are created,
         * interpreted and applied.
         *
         * @param type The VertexAnimationType to use for this track.
         */
        virtual void setAnimationType( VertexAnimationType type ) = 0;

        /**
         * @brief Create a morph key frame at the specified time position.
         *
         * The created key frame is typically added to this track's internal
         * collection. The returned pointer refers to the newly created key frame.
         *
         * @param timePos Time position (in the same time units used by the animation system)
         *                at which the key frame will be placed.
         * @return Pointer to the created IAnimationMorphKeyFrame, or nullptr on failure.
         */
        virtual IAnimationMorphKeyFrame *createVertexMorphKeyFrame( f32 timePos ) = 0;

        /**
         * @brief Create the single-pose key frame at the specified time position and add it to this
         * animation.
         *
         * Pose key frames typically represent a full vertex pose rather than per-vertex deltas.
         * Use this when the animation type expects a single pose entry.
         *
         * @param timePos Time position (in the same time units used by the animation system)
         *                at which the pose key frame will be placed.
         * @return Pointer to the created IAnimationPoseKeyFrame, or nullptr on failure.
         */
        virtual IAnimationPoseKeyFrame *createVertexPoseKeyFrame( f32 timePos ) = 0;

        /**
         * @brief Retrieve the morph key frame at the given index.
         *
         * Indexing is zero-based. Implementations should define the order of key frames
         * (usually chronological).
         *
         * @param index Zero-based index of the requested morph key frame.
         * @return Pointer to the IAnimationMorphKeyFrame at the index, or nullptr if index is out of
         * range.
         */
        virtual IAnimationMorphKeyFrame *getVertexMorphKeyFrame( u16 index ) const = 0;

        /**
         * @brief Retrieve the pose key frame at the given index.
         *
         * Indexing is zero-based. Implementations should define the order of key frames
         * (usually chronological).
         *
         * @param index Zero-based index of the requested pose key frame.
         * @return Pointer to the IAnimationPoseKeyFrame at the index, or nullptr if index is out of
         * range.
         */
        virtual IAnimationPoseKeyFrame *getVertexPoseKeyFrame( u16 index ) const = 0;

        /**
         * @brief Associate the vertex buffer that this track will update.
         *
         * The associated vertex data is the target that key frames will modify
         * when the track is applied. Passing nullptr typically clears the association.
         *
         * @param data Pointer to an IVertexBuffer that this track should update, or nullptr to unset.
         */
        virtual void setAssociatedVertexData( IVertexBuffer *data ) = 0;

        /**
         * @brief Get the vertex buffer currently associated with this track.
         *
         * @return Pointer to the associated IVertexBuffer, or nullptr if none is set.
         */
        virtual IVertexBuffer *getAssociatedVertexData() const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IAnimationVertexTrack_h__
