#ifndef IAnimationTrack_h__
#define IAnimationTrack_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for an animation track.
     *
     * An animation track contains an ordered sequence of keyframes that together
     * describe how a single animated target (e.g. a bone, vertex group, or scalar)
     * changes over time. Implementations are responsible for storing keyframes,
     * providing interpolation between them and applying the result to the
     * target when requested.
     *
     * Typical responsibilities:
     * - create, remove and enumerate keyframes
     * - compute interpolated keyframes for arbitrary time indices
     * - apply the track to its target with optional weight/scale for blending
     *
     * Notes:
     * - KeyFrame time indices should be treated as ordered; creating keyframes
     *   out-of-order may trigger expensive reordering in implementations.
     * - Some methods are marked as internal helpers (prefixed with `_`) and
     *   are intended for use by higher-level animation systems.
     */
    class WPCore_API IAnimationTrack : public ISharedObject
    {
    public:
        /**
         * @brief Listener that allows overriding or augmenting track behaviour.
         *
         * A Listener can be attached to a track to provide custom interpolation,
         * procedural animation or to intercept requests for interpolated frames.
         * Implementations should call the listener where appropriate and fall
         * back to the track's normal interpolation if the listener does not
         * provide data.
         */
        class Listener : public ISharedObject
        {
        public:
            ~Listener() override = default;

            /**
             * @brief Request an interpolated keyframe from the listener.
             *
             * This method is invoked by the track when an interpolated keyframe
             * is required. The listener may fill @p kf with the computed data.
             *
             * @param t The track requesting the interpolation (may be null).
             * @param timeIndex The time index for which to interpolate.
             * @param kf Smart pointer to a KeyFrame object that the listener
             *           should populate with the interpolated result.
             * @return true if @p kf was populated by the listener, false to
             *         indicate the track should perform its normal interpolation.
             */
            virtual bool getInterpolatedKeyFrame( SmartPtr<IAnimationTrack> t,
                                                  const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                                  SmartPtr<IAnimationKeyFrame> kf ) = 0;
        };

        /**
         * @brief Virtual destructor.
         *
         * Implementations must release resources and detach from any parent
         * animation or event listeners as required.
         */
        ~IAnimationTrack() override;

        /**
         * @brief Get a numeric handle identifying the owning animation within a larger system.
         * @return animation handle (u16)
         */
        virtual u16 getAnimationHandle() const = 0;

        /**
         * @brief Set the numeric handle identifying the owning animation.
         * @param handle Numeric handle to set.
         */
        virtual void setAnimationHandle( u16 handle ) = 0;

        /**
         * @brief Get the number of keyframes stored in this track.
         * @return number of keyframes (u16)
         */
        virtual u16 getNumKeyFrames() const = 0;

        /**
         * @brief Return the KeyFrame at the given zero-based index.
         *
         * The returned SmartPtr may be null if the index is out of range.
         *
         * @param index Zero-based keyframe index.
         * @return SmartPtr to the requested IAnimationKeyFrame or null.
         */
        virtual SmartPtr<IAnimationKeyFrame> getKeyFrame( u16 index ) const = 0;

        /**
         * @brief Get the two keyframes surrounding a time index and the blend factor.
         *
         * At any point in time a track has either:
         * - a single active keyframe (when time matches exactly a keyframe), or
         * - two active keyframes (the previous and next) when time falls between keyframes.
         *
         * This method populates @p keyFrame1 and @p keyFrame2 with the "from" and
         * "to" keyframes respectively and returns a parametric blend value t in
         * the half-open range [0.0, 1.0) indicating where @p timeIndex lies
         * between them:
         * - 0.0 => exactly at keyFrame1
         * - 0.5 => halfway between keyFrame1 and keyFrame2
         *
         * @param timeIndex Time index for which to query keyframes (relative to the whole animation).
         * @param keyFrame1 Output: keyframe at or immediately before @p timeIndex.
         * @param keyFrame2 Output: keyframe immediately after @p timeIndex.
         * @param firstKeyIndex Optional output: index of the 'from' keyframe (keyFrame1).
         * @return Parametric blend value t where 0.0 <= t < 1.0
         */
        virtual f32 getKeyFramesAtTime( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                        SmartPtr<IAnimationKeyFrame> &keyFrame1,
                                        SmartPtr<IAnimationKeyFrame> &keyFrame2,
                                        u16 *firstKeyIndex = nullptr ) const = 0;

        /**
         * @brief Create and insert a new keyframe at the specified time position.
         *
         * Implementations should insert the new keyframe into the internal list
         * maintaining time order. Creating keyframes in time order is preferred:
         * creating them out-of-order may cause expensive reordering operations.
         *
         * A keyframe at time 0.0 is guaranteed to exist; callers do not need to
         * create it and should use `getKeyFrame(0)` instead.
         *
         * @param timePos Time position (in seconds or the track's time units) for the new keyframe.
         * @return SmartPtr to the newly created IAnimationKeyFrame.
         */
        virtual SmartPtr<IAnimationKeyFrame> createKeyFrame( f32 timePos ) = 0;

        /**
         * @brief Remove the keyframe at the given index.
         * @param index Zero-based index of the keyframe to remove.
         */
        virtual void removeKeyFrame( u16 index ) = 0;

        /**
         * @brief Remove all keyframes from this track.
         *
         * After this call the track will contain no keyframes. Implementations
         * may choose to leave a default keyframe at time 0.0 depending on policy.
         */
        virtual void removeAllKeyFrames() = 0;

        /**
         * @brief Populate @p kf with the interpolated transforms at @p timeIndex.
         *
         * This performs 'tweening' between surrounding keyframes to produce
         * a smooth transformation snapshot at an arbitrary time within the
         * animation range.
         *
         * @param timeIndex Time index for interpolation (relative to the whole animation).
         * @param kf Output SmartPtr to receive the interpolated keyframe data.
         */
        virtual void getInterpolatedKeyFrame( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                              SmartPtr<IAnimationKeyFrame> &kf ) const = 0;

        /**
         * @brief Apply the track to its target at the specified time.
         *
         * This will evaluate the track at @p timeIndex and apply the resulting
         * transform(s) to the track's target. The application is modulated by
         * @p weight and @p scale to allow blending and size adjustments:
         * - weight: 1.0 = full influence, 0.0 = no influence (blend factor).
         * - scale: scales translation/scale components to adapt animation to a differently sized target.
         *
         * @param timeIndex Time position to evaluate and apply.
         * @param weight Blending weight for this track (default 1.0).
         * @param scale Scale factor applied to translations/scales (default 1.0f).
         */
        virtual void apply( const SmartPtr<IAnimationTimeIndex> &timeIndex, f32 weight = 1.0,
                            f32 scale = 1.0f ) = 0;

        /**
         * @brief Internal notification that keyframe data has changed.
         *
         * Implementations should mark or rebuild any internal acceleration structures
         * (e.g. caches, index maps) when this is called.
         *
         * @internal
         */
        virtual void _keyFrameDataChanged() const = 0;

        /**
         * @brief Determine whether the track contains any non-zero/keyed data.
         *
         * Can be used by higher-level systems to skip applying or storing tracks
         * that have no effect (all keyframes represent identity/no-op).
         *
         * @return true if there is meaningful (non-zero) keyframe data; false otherwise.
         */
        virtual bool hasNonZeroKeyFrames() const = 0;

        /**
         * @brief Optimise the track by removing redundant or duplicate keyframes.
         *
         * Implementations may perform tolerance-based merging or other techniques
         * to reduce keyframe count while preserving perceived animation fidelity.
         */
        virtual void optimise() = 0;

        /**
         * @brief Collect unique, ordered list of keyframe times.
         *
         * The method populates @p keyFrameTimes with the distinct time positions
         * used by keyframes in this track. This is used by timeline merging and
         * global indexing operations.
         *
         * @param keyFrameTimes Output array that will receive unique sorted times.
         * @internal
         */
        virtual void _collectKeyFrameTimes( Array<f32> &keyFrameTimes ) = 0;

        /**
         * @brief Build a mapping from global keyframe time indices to local keyframe indices.
         *
         * Given a global set of keyframe times (from multiple tracks), this method
         * should build any internal maps needed to translate a global lower-bound
         * index to the corresponding local lower-bound index for this track.
         *
         * @param keyFrameTimes Global, ordered list of unique keyframe times.
         * @internal
         */
        virtual void _buildKeyFrameIndexMap( const Array<f32> &keyFrameTimes ) = 0;

        /**
         * @brief Rebase this track's keyframes relative to @p base.
         *
         * Some animation systems require keyframes to be applied relative to a
         * base transform. This method allows the track to adjust its stored
         * keyframes so the base transform is applied/removed as needed.
         *
         * @param base KeyFrame representing the base transform to apply.
         * @internal
         */
        virtual void _applyBaseKeyFrame( const SmartPtr<IAnimationKeyFrame> &base ) = 0;

        /**
         * @brief Set an event listener for this track.
         *
         * The listener may be used to receive notifications or to override default
         * behaviour. The pointer type is implementation-defined; ownership is not
         * transferred by this call unless documented by the implementation.
         *
         * @param l Pointer to an IEventListener implementation (may be null).
         */
        virtual void setListener( IEventListener *l ) = 0;

        /**
         * @brief Get the parent animation that owns this track.
         * @return Pointer to the parent IAnimation (may be null).
         */
        virtual IAnimation *getParent() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAnimationTrack_h__
