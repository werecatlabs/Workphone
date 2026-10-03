#ifndef IAnimation_h__
#define IAnimation_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Animation/IAnimationState.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{

    /**
     * @class IAnimation
     * @brief Interface for a keyframe-based animation system.
     *
     * Provides core functionality for managing animations, including interpolation modes,
     * animation tracks, and timing control. Animations are composed of multiple tracks
     * that animate different properties of objects over time.
     *
     * @note This interface is not thread-safe. External synchronization is required when
     * accessing the same animation instance from multiple threads.
     *
     * @see IAnimationTrack, IAnimationState, IBone
     * @since Version 1.0
     */
    class WPCore_API IAnimation : public ISharedObject
    {
    public:
        /**
         * @brief Map of node track handles to their corresponding animation tracks.
         */
        typedef std::map<u16, IActorAnimationTrack *> NodeTrackList;

        /**
         * @brief Map of numeric track handles to their corresponding animation tracks.
         */
        typedef std::map<u16, IAnimationNumericTrack *> NumericTrackList;

        /**
         * @brief Map of vertex track handles to their corresponding animation tracks.
         */
        typedef std::map<u16, IAnimationVertexTrack *> VertexTrackList;

        /**
         * @brief Set of track handles.
         */
        typedef Set<u16> TrackHandleList;

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived animation implementations.
         * All associated animation tracks will be cleaned up automatically.
         */
        ~IAnimation() override;

        /**
         * @brief Sets the interpolation mode for this animation.
         *
         * The interpolation mode determines how values are calculated between keyframes.
         * Different modes offer trade-offs between performance and quality.
         *
         * @param im The interpolation mode to use.
         *
         * @see getInterpolationMode(), InterpolationMode
         */
        virtual void setInterpolationMode( InterpolationMode im ) = 0;

        /**
         * @brief Gets the current interpolation mode of this animation.
         *
         * @return The current interpolation mode.
         *
         * @see setInterpolationMode()
         */
        virtual InterpolationMode getInterpolationMode() const = 0;

        /**
         * @brief Sets how rotations are interpolated in this animation.
         *
         * By default, animations interpolate linearly between rotations. For more accurate
         * interpolation, use spherical interpolation (SLERP), which is more computationally expensive.
         *
         * @param im The rotation interpolation mode to use.
         *
         * @see getRotationInterpolationMode(), RotationInterpolationMode
         */
        virtual void setRotationInterpolationMode( RotationInterpolationMode im ) = 0;

        /**
         * @brief Gets the current rotation interpolation mode of this animation.
         *
         * @return The current rotation interpolation mode.
         *
         * @see setRotationInterpolationMode()
         */
        virtual RotationInterpolationMode getRotationInterpolationMode() const = 0;

        /**
         * @brief Gets the total length of the animation in time units.
         *
         * @return The length of the animation as a floating-point value (typically in seconds).
         *
         * @note The time units depend on your application's time scale.
         * Commonly this represents seconds, but it could be frames or any other time measurement.
         *
         * @see setLength()
         */
        virtual f32 getLength() const = 0;

        /**
         * @brief Sets the length of the animation in time units.
         *
         * @param length The new length for the animation as a floating-point value.
         *
         * @warning Changing the length of an animation may invalidate existing AnimationState
         * instances, which will need to be recreated. Ensure all animation states are
         * properly updated after changing the animation length.
         *
         * @see getLength()
         */
        virtual void setLength( f32 length ) = 0;

        /**
         * @brief Removes an animation track from this animation.
         *
         * @param track Shared pointer to the track to be removed.
         *
         * @note The track will be removed from the animation's track list.
         * If the track is not found, this method has no effect.
         * The track object itself may continue to exist if other references exist.
         *
         * @warning Removing a track while the animation is playing may cause
         * unexpected behavior. Consider pausing the animation before removing tracks.
         *
         * @see addTrack()
         */
        virtual void removeTrack( SmartPtr<IAnimationTrack> track ) = 0;

        /**
         * @brief Adds a new animation track to this animation.
         *
         * @param type The hash type identifier for the track type.
         * @param name The name identifier for the track.
         * @param handle Optional handle for the track (default: 0).
         * @return Shared pointer to the newly created animation track.
         *
         * @note The type parameter should correspond to a registered track type in the
         * animation system. Track names should be unique within the animation.
         *
         * @see addTrackByType(), removeTrack()
         */
        virtual SmartPtr<IAnimationTrack> addTrack( hash_type type, const String &name,
                                                    u16 handle = 0 ) = 0;

        /**
         * @brief Adds a new animation track associated with a specific bone.
         *
         * @param type The hash type identifier for the track type.
         * @param handle The handle identifier for the track.
         * @param bone Shared pointer to the bone this track will animate.
         * @return Shared pointer to the newly created animation track.
         *
         * @note This method is typically used for skeletal animation where tracks are
         * directly associated with specific bones in a skeleton hierarchy.
         *
         * @see addTrack(), addTrackByType(), IBone
         */
        virtual SmartPtr<IAnimationTrack> addTrack( hash_type type, u16 handle,
                                                    SmartPtr<IBone> bone ) = 0;

        /**
         * @brief Template method to add a typed animation track by name.
         *
         * @tparam T The specific animation track type to create.
         * @param name The name identifier for the track.
         * @param handle Optional handle for the track (default: 0).
         * @return Shared pointer to the newly created track, cast to the specified type.
         *
         * @note This method provides compile-time type safety by ensuring the returned
         * track is of the correct type, eliminating the need for manual casting.
         *
         * @see addTrack()
         */
        template <typename T>
        SmartPtr<T> addTrackByType( const String &name, u16 handle = 0 );

        /**
         * @brief Template method to add a typed animation track for a specific bone.
         *
         * @tparam T The specific animation track type to create.
         * @param handle The handle identifier for the track.
         * @param bone Shared pointer to the bone this track will animate.
         * @return Shared pointer to the newly created track, cast to the specified type.
         *
         * @note This method is particularly useful for skeletal animation systems where
         * you need to create typed tracks for specific bones.
         *
         * @see addTrack(), IBone
         */
        template <typename T>
        SmartPtr<T> addTrackByType( u16 handle, SmartPtr<IBone> bone );

        /** @name Track Query and Management */
        ///@{

        /**
         * @brief Gets the number of NodeAnimationTrack objects contained in this animation.
         * @return The number of node tracks.
         */
        virtual u16 getNumNodeTracks() const = 0;

        /**
         * @brief Gets a node track by its handle.
         * @param handle The handle of the node track.
         * @return Pointer to the node animation track, or nullptr if not found.
         */
        virtual IActorAnimationTrack *getNodeTrack( u16 handle ) const = 0;

        /**
         * @brief Checks if a node track exists with the given handle.
         * @param handle The handle to check.
         * @return True if the node track exists, false otherwise.
         */
        virtual bool hasNodeTrack( u16 handle ) const = 0;

        /**
         * @brief Gets the number of NumericAnimationTrack objects contained in this animation.
         * @return The number of numeric tracks.
         */
        virtual u16 getNumNumericTracks() const = 0;

        /**
         * @brief Gets a numeric track by its handle.
         * @param handle The handle of the numeric track.
         * @return Pointer to the numeric animation track, or nullptr if not found.
         */
        virtual IAnimationNumericTrack *getNumericTrack( u16 handle ) const = 0;

        /**
         * @brief Checks if a numeric track exists with the given handle.
         * @param handle The handle to check.
         * @return True if the numeric track exists, false otherwise.
         */
        virtual bool hasNumericTrack( u16 handle ) const = 0;

        /**
         * @brief Gets the number of VertexAnimationTrack objects contained in this animation.
         * @return The number of vertex tracks.
         */
        virtual u16 getNumVertexTracks() const = 0;

        /**
         * @brief Gets the list of vertex tracks.
         * @return Array of smart pointers to the vertex animation tracks.
         */
        virtual Array<SmartPtr<IAnimationVertexTrack>> getVertexTracks() const = 0;

        /**
         * @brief Sets the list of vertex tracks.
         * @param vertexTracks Array of smart pointers to the vertex animation tracks.
         */
        virtual void setVertexTracks( const Array<SmartPtr<IAnimationVertexTrack>> &vertexTracks ) = 0;

        /**
         * @brief Gets a vertex track by its handle.
         * @param handle The handle of the vertex track.
         * @return Pointer to the vertex animation track, or nullptr if not found.
         */
        virtual IAnimationVertexTrack *getVertexTrack( u16 handle ) const = 0;

        /**
         * @brief Checks if a vertex track exists with the given handle.
         * @param handle The handle to check.
         * @return True if the vertex track exists, false otherwise.
         */
        virtual bool hasVertexTrack( u16 handle ) const = 0;

        /**
         * @brief Destroys the node track with the given handle.
         * @param handle The handle of the node track to destroy.
         */
        virtual void destroyNodeTrack( u16 handle ) = 0;

        /**
         * @brief Destroys the numeric track with the given handle.
         * @param handle The handle of the numeric track to destroy.
         */
        virtual void destroyNumericTrack( u16 handle ) = 0;

        /**
         * @brief Destroys the vertex track with the given handle.
         * @param handle The handle of the vertex track to destroy.
         */
        virtual void destroyVertexTrack( u16 handle ) = 0;

        /**
         * @brief Removes and destroys all tracks making up this animation.
         */
        virtual void destroyAllTracks() = 0;

        /**
         * @brief Removes and destroys all node tracks in this animation.
         */
        virtual void destroyAllNodeTracks() = 0;

        /**
         * @brief Removes and destroys all numeric tracks in this animation.
         */
        virtual void destroyAllNumericTracks() = 0;

        /**
         * @brief Removes and destroys all vertex tracks in this animation.
         */
        virtual void destroyAllVertexTracks() = 0;

        ///@}

        /** @name Animation Application */
        ///@{

        /**
         * @brief Applies the animation at a specific time point and weight.
         *
         * Applies all associated animation tracks to their targets.
         *
         * @param timePos The time position in the animation to apply.
         * @param weight The influence to give to this track (1.0 for full influence).
         * @param scale The scale to apply to translations and scalings.
         */
        virtual void apply( f32 timePos, f32 weight = 1.0f, f32 scale = 1.0f ) = 0;

        /**
         * @brief Applies all node tracks at a specific time point and weight to the specified node.
         *
         * @param node The node to apply the animation to.
         * @param timePos The time position in the animation to apply.
         * @param weight The influence to give to this track.
         * @param scale The scale to apply to translations and scalings.
         */
        virtual void applyToNode( scene::IGameActor *node, f32 timePos, f32 weight = 1.0f,
                                  f32 scale = 1.0f ) = 0;

        /**
         * @brief Applies all node tracks at a specific time point and weight to a given skeleton.
         *
         * @param skeleton The skeleton to apply the animation to.
         * @param timePos The time position in the animation to apply.
         * @param weight The influence to give to this track.
         * @param scale The scale to apply to translations and scalings.
         */
        virtual void apply( ISkeleton *skeleton, f32 timePos, f32 weight = 1.0, f32 scale = 1.0f ) = 0;

        /**
         * @brief Applies all node tracks at a specific time point and weight to a given skeleton, with a
         * blend mask.
         *
         * @param skeleton The skeleton to apply the animation to.
         * @param timePos The time position in the animation to apply.
         * @param weight The influence to give to this track.
         * @param blendMask The influence array defining additional per bone weights.
         * @param scale The scale to apply to translations and scalings.
         */
        virtual void apply( ISkeleton *skeleton, f32 timePos, float weight,
                            const IAnimationState::BoneBlendMask *blendMask, f32 scale ) = 0;

        /**
         * @brief Applies all vertex tracks at a specific time point and weight to a given entity.
         *
         * @param entity The entity to which this animation should be applied.
         * @param timePos The time position in the animation to apply.
         * @param weight The weight at which the animation should be applied.
         * @param software Whether to populate the software morph vertex data.
         * @param hardware Whether to populate the hardware morph vertex data.
         */
        virtual void apply( scene::Mesh *entity, f32 timePos, f32 weight, bool software,
                            bool hardware ) = 0;

        /**
         * @brief Applies all numeric tracks at a specific time point and weight to the specified
         * animable value.
         *
         * @param anim The animable value to apply the animation to.
         * @param timePos The time position in the animation to apply.
         * @param weight The influence to give to this track.
         * @param scale The scale to apply to translations and scalings.
         */
        virtual void applyToAnimable( const IAnimableValue *anim, f32 timePos, f32 weight = 1.0f,
                                      f32 scale = 1.0f ) = 0;

        ///@}

        /** @name Track List Access */
        ///@{

        /**
         * @brief Provides fast access to the non-updateable node track list.
         * @return Reference to the node track list.
         */
        virtual const NodeTrackList &_getNodeTrackList() const = 0;

        /**
         * @brief Provides fast access to the non-updateable numeric track list.
         * @return Reference to the numeric track list.
         */
        virtual const NumericTrackList &_getNumericTrackList() const = 0;

        /**
         * @brief Provides fast access to the non-updateable vertex track list.
         * @return Reference to the vertex track list.
         */
        virtual const VertexTrackList &_getVertexTrackList() const = 0;

        ///@}

        /** @name Optimization and Internal Methods */
        ///@{

        /**
         * @brief Optimizes the animation by removing unnecessary tracks and keyframes.
         *
         * @param discardIdentityNodeTracks If true, discard identity node tracks.
         *
         * @note This can reduce memory usage and improve performance.
         */
        virtual void optimise( bool discardIdentityNodeTracks = true ) = 0;

        /**
         * @brief Internal method for collecting identity node tracks.
         *
         * @param tracks A list of track handles; non-identity node track handles will be removed.
         */
        virtual void _collectIdentityNodeTracks( TrackHandleList &tracks ) const = 0;

        /**
         * @brief Internal method for destroying given node tracks.
         *
         * @param tracks The set of track handles to destroy.
         */
        virtual void _destroyNodeTracks( const TrackHandleList &tracks ) = 0;

        /**
         * @brief Clones this animation.
         *
         * @param newName The name for the cloned animation.
         * @return Pointer to the cloned animation.
         *
         * @note The caller is responsible for managing the returned pointer.
         */
        virtual IAnimation *clone( const String &newName ) const = 0;

        /**
         * @brief Internal method used to notify that the keyframe list has changed.
         *
         * May cause the animation to rebuild some internal data.
         */
        virtual void _keyFrameListChanged() = 0;

        /**
         * @brief Internal method used to convert a time position to a time index object.
         *
         * @param timePos The time position.
         * @return Smart pointer to the time index object.
         *
         * @note The returned time index is associated with the current state of the animation object.
         * If the animation object is altered, all related time indices will be invalidated.
         */
        virtual SmartPtr<IAnimationTimeIndex> _getTimeIndex( f32 timePos ) const = 0;

        ///@}

        /** @name Base Keyframe Management */
        ///@{

        /**
         * @brief Sets a base keyframe for skeletal or pose keyframes in this animation.
         *
         * @param useBaseKeyFrame Whether a base keyframe should be used.
         * @param keyframeTime The time corresponding to the base keyframe, if any.
         * @param baseAnimName Optionally a different base animation (must contain the same tracks).
         *
         * @note This is useful for additive or cumulative animation blending.
         */
        virtual void setUseBaseKeyFrame( bool useBaseKeyFrame, f32 keyframeTime = 0.0f,
                                         const String &baseAnimName = StringUtil::EmptyString ) = 0;

        /**
         * @brief Checks whether a base keyframe is being used for this animation.
         * @return True if a base keyframe is used, false otherwise.
         */
        virtual bool getUseBaseKeyFrame() const = 0;

        /**
         * @brief Gets the time of the base keyframe, if used.
         * @return The time of the base keyframe.
         */
        virtual f32 getBaseKeyFrameTime() const = 0;

        /**
         * @brief Gets the name of the animation providing the base keyframe, if used.
         * @return The name of the base keyframe animation.
         */
        virtual const String &getBaseKeyFrameAnimationName() const = 0;

        /**
         * @brief Internal method to adjust keyframes relative to a base keyframe.
         *
         * @see setUseBaseKeyFrame()
         */
        virtual void _applyBaseKeyFrame() = 0;

        ///@}

        /** @name Container Management */
        ///@{

        /**
         * @brief Notifies the animation of its container.
         * @param c Pointer to the animation container.
         */
        virtual void _notifyContainer( IAnimationContainer *c ) = 0;

        /**
         * @brief Retrieves the container of this animation.
         * @return Pointer to the animation container.
         */
        virtual IAnimationContainer *getContainer() = 0;

        ///@}

        WP_CLASS_REGISTER_DECL;
    };

    /**
     * @copydoc IAnimation::addTrackByType(const String&, u16)
     */
    template <typename T>
    SmartPtr<T> IAnimation::addTrackByType( const String &name, u16 handle )
    {
        auto type = T::typeInfo();
        auto track = addTrack( type, name, handle );
        return workphone::static_pointer_cast<T>( track );
    }

    /**
     * @copydoc IAnimation::addTrackByType(u16, SmartPtr<IBone>)
     */
    template <typename T>
    SmartPtr<T> IAnimation::addTrackByType( u16 handle, SmartPtr<IBone> bone )
    {
        auto type = T::typeInfo();
        auto track = addTrack( type, handle, bone );
        return workphone::static_pointer_cast<T>( track );
    }

}  // namespace workphone

#endif  // IAnimation_h__
