#ifndef __ActorAnimationTrack_h__
#define __ActorAnimationTrack_h__

#include <Workphone/Interface/Animation/IActorAnimationTrack.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Animation/AnimationTrack.hpp>

namespace workphone
{
    /**
     * @brief The ActorAnimationTrack class.
     *
     * This class implements an animation track specifically for game actors.
     * It allows animating a specific property of a scene actor over time.
     */
    class ActorAnimationTrack : public AnimationTrack<IActorAnimationTrack>
    {
    public:
        /**
         * @brief Constructor for ActorAnimationTrack.
         * @param parent Pointer to the parent animation this track belongs to.
         */
        ActorAnimationTrack( IAnimation *parent = nullptr );

        /**
         * @brief Destructor for ActorAnimationTrack.
         */
        ~ActorAnimationTrack() override;

        // IActorAnimationTrack interface

        /**
         * @brief Sets the game actor that this track will animate.
         * @param actor Smart pointer to the game actor.
         */
        void setActor( SmartPtr<scene::IGameActor> actor ) override;

        /**
         * @brief Gets the game actor that this track is animating.
         * @return Smart pointer to the game actor.
         */
        SmartPtr<scene::IGameActor> getActor() const override;

        void setBone( SmartPtr<IBone> bone );

        SmartPtr<IBone> getBone() const;

        /**
         * @brief Sets the name of the property on the actor to be animated.
         * @param propertyName The name of the property (e.g., "Position", "Rotation").
         */
        void setPropertyName( const String &propertyName ) override;

        /**
         * @brief Gets the name of the property currently being animated.
         * @return The property name as a string.
         */
        String getPropertyName() const override;

        /**
         * @brief Sets the type of track (e.g., Transform, Float, Color).
         * @param type The TrackType to set.
         */
        void setTrackType( TrackType type ) override;

        /**
         * @brief Gets the current type of the track.
         * @return The current TrackType.
         */
        TrackType getTrackType() const override;

        /**
         * @brief Applies the interpolated animation value to the target actor.
         * @param timeIndex The current time index within the animation.
         * @param weight The blending weight to apply (0.0 to 1.0).
         * @param scale A scale factor for the animation effect.
         */
        void apply( const SmartPtr<IAnimationTimeIndex> &timeIndex, f32 weight = 1.0,
                    f32 scale = 1.0f ) override;

        /**
         * @brief Creates a new keyframe at the specified time position.
         * @param timePos The time position in seconds.
         * @return A smart pointer to the created keyframe.
         */
        SmartPtr<IAnimationKeyFrame> createKeyFrame( f32 timePos ) override;

        /**
         * @brief Calculates and retrieves the interpolated keyframe for a given time index.
         * @param timeIndex The current time index.
         * @param kf Output parameter to receive the interpolated keyframe.
         */
        void getInterpolatedKeyFrame( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                      SmartPtr<IAnimationKeyFrame> &kf ) const override;

        /**
         * @brief Gets the total length of the animation track.
         * @return Length of the track in seconds.
         */
        f32 getLength() const;

        /**
         * @brief Sets the total length of the animation track.
         * @param length Length in seconds.
         */
        void setLength( f32 length );

        WP_CLASS_REGISTER_DECL;

    protected:
        f32 m_length = 0.0f;                  ///< The length of the animation track in seconds.
        SmartPtr<scene::IGameActor> m_actor;  ///< The actor this track animates.
        SmartPtr<IBone> m_bone;               ///< The skeleton bone this track animates.
        String m_propertyName;                ///< The property name of the actor this track animates.
        TrackType m_trackType =
            TrackType::Transform;  ///< The type of this track (e.g., Transform, Float).
    };
}  // namespace workphone

#endif  // ActorAnimationTrack_h__
