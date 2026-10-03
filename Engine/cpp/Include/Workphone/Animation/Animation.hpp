#ifndef __Animation_h__
#define __Animation_h__

#include <Workphone/Interface/Animation/IAnimation.hpp>

namespace workphone
{
    /**
     * @class Animation
     * @brief Represents an animation sequence, managing tracks and interpolation modes.
     *
     * The Animation class provides functionality to manage animation tracks,
     * control interpolation modes for both value and rotation, and set the overall
     * length of the animation. It implements the IAnimation interface and supports
     * operations for adding, removing, and applying animation data to various targets.
     */
    class WPCore_API Animation : public IAnimation
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes a new Animation object with default settings.
         */
        Animation();

        /**
         * @brief Destructor.
         *
         * Cleans up resources used by the Animation object.
         */
        ~Animation() override;

        /**
         * @brief Sets the interpolation mode for value changes in the animation.
         * @param im The interpolation mode to use (e.g., linear, spline).
         */
        void setInterpolationMode( InterpolationMode im ) override;

        /**
         * @brief Gets the current interpolation mode for value changes.
         * @return The current interpolation mode.
         */
        InterpolationMode getInterpolationMode() const override;

        /**
         * @brief Sets the interpolation mode for rotation changes in the animation.
         * @param im The rotation interpolation mode to use (e.g., linear, spherical).
         */
        void setRotationInterpolationMode( RotationInterpolationMode im ) override;

        /**
         * @brief Gets the current interpolation mode for rotation changes.
         * @return The current rotation interpolation mode.
         */
        RotationInterpolationMode getRotationInterpolationMode() const override;

        /**
         * @brief Removes an animation track from the animation.
         * @param track Smart pointer to the track to remove.
         */
        void removeTrack( SmartPtr<IAnimationTrack> track ) override;

        /**
         * @brief Adds a new animation track to the animation.
         * @param type The type identifier for the track.
         * @param name The name of the track.
         * @param handle Optional handle for the track (default is 0).
         * @return A smart pointer to the newly added animation track.
         */
        SmartPtr<IAnimationTrack> addTrack( hash_type type, const String &name,
                                            u16 handle = 0 ) override;

        /**
         * @brief Adds a new animation track associated with a bone.
         * @param type The type identifier for the track.
         * @param handle The handle for the track.
         * @param bone Smart pointer to the bone associated with the track.
         * @return A smart pointer to the newly added animation track.
         */
        SmartPtr<IAnimationTrack> addTrack( hash_type type, u16 handle, SmartPtr<IBone> bone ) override;

        /**
         * @brief Gets the length of the animation in seconds.
         * @return The length of the animation.
         */
        f32 getLength() const override;

        /**
         * @brief Sets the length of the animation in seconds.
         * @param length The new length of the animation.
         */
        void setLength( f32 length ) override;

        /**
         * @brief Gets the number of node tracks in the animation.
         * @return The number of node tracks.
         */
        unsigned short getNumNodeTracks( void ) const override;

        /**
         * @brief Gets a node track by its handle.
         * @param handle The handle of the node track.
         * @return Pointer to the node track, or nullptr if not found.
         */
        IActorAnimationTrack *getNodeTrack( unsigned short handle ) const override;

        /**
         * @brief Checks if a node track exists for the given handle.
         * @param handle The handle to check.
         * @return True if the node track exists, false otherwise.
         */
        bool hasNodeTrack( unsigned short handle ) const override;

        /**
         * @brief Gets the number of numeric tracks in the animation.
         * @return The number of numeric tracks.
         */
        unsigned short getNumNumericTracks( void ) const override;

        /**
         * @brief Gets a numeric track by its handle.
         * @param handle The handle of the numeric track.
         * @return Pointer to the numeric track, or nullptr if not found.
         */
        IAnimationNumericTrack *getNumericTrack( unsigned short handle ) const override;

        /**
         * @brief Checks if a numeric track exists for the given handle.
         * @param handle The handle to check.
         * @return True if the numeric track exists, false otherwise.
         */
        bool hasNumericTrack( unsigned short handle ) const override;

        /**
         * @brief Gets the number of vertex tracks in the animation.
         * @return The number of vertex tracks.
         */
        unsigned short getNumVertexTracks( void ) const override;

        /**
         * @brief Gets a vertex track by its handle.
         * @param handle The handle of the vertex track.
         * @return Pointer to the vertex track, or nullptr if not found.
         */
        IAnimationVertexTrack *getVertexTrack( unsigned short handle ) const override;

        /**
         * @brief Checks if a vertex track exists for the given handle.
         * @param handle The handle to check.
         * @return True if the vertex track exists, false otherwise.
         */
        bool hasVertexTrack( unsigned short handle ) const override;

        /**
         * @brief Destroys a node track by its handle.
         * @param handle The handle of the node track to destroy.
         */
        void destroyNodeTrack( unsigned short handle ) override;

        /**
         * @brief Destroys a numeric track by its handle.
         * @param handle The handle of the numeric track to destroy.
         */
        void destroyNumericTrack( unsigned short handle ) override;

        /**
         * @brief Destroys a vertex track by its handle.
         * @param handle The handle of the vertex track to destroy.
         */
        void destroyVertexTrack( unsigned short handle ) override;

        /**
         * @brief Destroys all animation tracks.
         */
        void destroyAllTracks( void ) override;

        /**
         * @brief Destroys all node tracks.
         */
        void destroyAllNodeTracks( void ) override;

        /**
         * @brief Destroys all numeric tracks.
         */
        void destroyAllNumericTracks( void ) override;

        /**
         * @brief Destroys all vertex tracks.
         */
        void destroyAllVertexTracks( void ) override;

        /**
         * @brief Applies the animation to all targets at a specific time position.
         * @param timePos The time position in seconds.
         * @param weight The weight of the animation (default is 1.0).
         * @param scale The scale factor for the animation (default is 1.0f).
         */
        void apply( f32 timePos, f32 weight = 1.0, f32 scale = 1.0f ) override;

        /**
         * @brief Applies the animation to a specific node at a given time position.
         * @param node The target node.
         * @param timePos The time position in seconds.
         * @param weight The weight of the animation (default is 1.0).
         * @param scale The scale factor for the animation (default is 1.0f).
         */
        void applyToNode( scene::IGameActor *node, f32 timePos, f32 weight = 1.0,
                          f32 scale = 1.0f ) override;

        /**
         * @brief Applies the animation to a skeleton at a specific time position.
         * @param skeleton The target skeleton.
         * @param timePos The time position in seconds.
         * @param weight The weight of the animation (default is 1.0).
         * @param scale The scale factor for the animation (default is 1.0f).
         */
        void apply( ISkeleton *skeleton, f32 timePos, f32 weight = 1.0, f32 scale = 1.0f ) override;

        /**
         * @brief Applies the animation to a skeleton with a bone blend mask.
         * @param skeleton The target skeleton.
         * @param timePos The time position in seconds.
         * @param weight The weight of the animation.
         * @param blendMask The bone blend mask to use.
         * @param scale The scale factor for the animation.
         */
        void apply( ISkeleton *skeleton, f32 timePos, float weight,
                    const IAnimationState::BoneBlendMask *blendMask, f32 scale ) override;

        /**
         * @brief Applies the animation to a mesh entity.
         * @param entity The mesh entity.
         * @param timePos The time position in seconds.
         * @param weight The weight of the animation.
         * @param software Whether to apply using software skinning.
         * @param hardware Whether to apply using hardware skinning.
         */
        void apply( scene::Mesh *entity, f32 timePos, f32 weight, bool software,
                    bool hardware ) override;

        /**
         * @brief Applies the animation to an animable value.
         * @param anim The animable value.
         * @param timePos The time position in seconds.
         * @param weight The weight of the animation (default is 1.0).
         * @param scale The scale factor for the animation (default is 1.0f).
         */
        void applyToAnimable( const IAnimableValue *anim, f32 timePos, f32 weight = 1.0,
                              f32 scale = 1.0f ) override;

        /**
         * @brief Gets the list of node tracks.
         * @return Reference to the node track list.
         */
        const NodeTrackList &_getNodeTrackList( void ) const override;

        /**
         * @brief Gets the list of numeric tracks.
         * @return Reference to the numeric track list.
         */
        const NumericTrackList &_getNumericTrackList( void ) const override;

        /**
         * @brief Gets the list of vertex tracks.
         * @return Reference to the vertex track list.
         */
        const VertexTrackList &_getVertexTrackList( void ) const override;

        /**
         * @brief Optimizes the animation by discarding identity node tracks if specified.
         * @param discardIdentityNodeTracks If true, removes tracks with no changes.
         */
        void optimise( bool discardIdentityNodeTracks = true ) override;

        /**
         * @brief Collects handles of identity node tracks.
         * @param tracks List to populate with handles of identity node tracks.
         */
        void _collectIdentityNodeTracks( TrackHandleList &tracks ) const override;

        /**
         * @brief Destroys node tracks specified by a list of handles.
         * @param tracks List of handles for node tracks to destroy.
         */
        void _destroyNodeTracks( const TrackHandleList &tracks ) override;

        /**
         * @brief Clones the animation with a new name.
         * @param newName The name for the cloned animation.
         * @return Pointer to the cloned animation.
         */
        IAnimation *clone( const String &newName ) const override;

        /**
         * @brief Notifies the animation that its keyframe list has changed.
         */
        void _keyFrameListChanged( void ) override;

        /**
         * @brief Gets the time index for a given time position.
         * @param timePos The time position in seconds.
         * @return Smart pointer to the time index object.
         */
        SmartPtr<IAnimationTimeIndex> _getTimeIndex( f32 timePos ) const override;

        /**
         * @brief Sets whether to use a base keyframe for the animation.
         * @param useBaseKeyFrame Whether to use a base keyframe.
         * @param keyframeTime The time of the base keyframe (default is 0.0f).
         * @param baseAnimName The name of the base animation (default is empty).
         */
        void setUseBaseKeyFrame( bool useBaseKeyFrame, f32 keyframeTime = 0.0f,
                                 const String &baseAnimName = StringUtil::EmptyString ) override;

        /**
         * @brief Checks if a base keyframe is used.
         * @return True if a base keyframe is used, false otherwise.
         */
        bool getUseBaseKeyFrame() const override;

        /**
         * @brief Gets the time of the base keyframe.
         * @return The base keyframe time in seconds.
         */
        f32 getBaseKeyFrameTime() const override;

        /**
         * @brief Gets the name of the base keyframe animation.
         * @return Reference to the base keyframe animation name.
         */
        const String &getBaseKeyFrameAnimationName() const override;

        /**
         * @brief Applies the base keyframe to the animation.
         */
        void _applyBaseKeyFrame() override;

        /**
         * @brief Notifies the animation of its container.
         * @param c Pointer to the animation container.
         */
        void _notifyContainer( IAnimationContainer *c ) override;

        /**
         * @brief Gets the container of the animation.
         * @return Pointer to the animation container.
         */
        IAnimationContainer *getContainer() override;

        Array<SmartPtr<IAnimationVertexTrack>> getVertexTracks() const override;

        void setVertexTracks( const Array<SmartPtr<IAnimationVertexTrack>> &vertexTracks ) override;

        Array<SmartPtr<IActorAnimationTrack>> getNodeTracks() const;

        void setNodeTracks( const Array<SmartPtr<IActorAnimationTrack>> &nodeTracks );

        Array<SmartPtr<IAnimationNumericTrack>> getNumericTracks() const;

        void setNumericTracks( const Array<SmartPtr<IAnimationNumericTrack>> &numericTracks );

        /**
         * @brief Registers the Animation class for reflection or serialization.
         */
        WP_CLASS_REGISTER_DECL;

    protected:
        InterpolationMode
            m_interpolationMode;  ///< Interpolation mode for value changes in this animation.
        RotationInterpolationMode
            m_rotationInterpolationMode;  ///< Interpolation mode for rotation changes in this animation.
        f32 m_length;                     ///< Length of the animation in seconds.

        NodeTrackList m_nodeTracks;        ///< Map of node track handles to tracks.
        NumericTrackList m_numericTracks;  ///< Map of numeric track handles to tracks.
        VertexTrackList m_vertexTracks;    ///< Map of vertex track handles to tracks.

        Array<SmartPtr<IActorAnimationTrack>> m_ownedNodeTracks;
        Array<SmartPtr<IAnimationNumericTrack>> m_ownedNumericTracks;
        Array<SmartPtr<IAnimationVertexTrack>> m_ownedVertexTracks;

        bool m_useBaseKeyFrame;              ///< Whether a base keyframe is used.
        f32 m_baseKeyFrameTime;              ///< Time of the base keyframe.
        String m_baseKeyFrameAnimationName;  ///< Name of the base keyframe animation.

        IAnimationContainer *m_container;  ///< Pointer to the animation container.
    };
}  // namespace workphone

#endif  // __Animation_h__
