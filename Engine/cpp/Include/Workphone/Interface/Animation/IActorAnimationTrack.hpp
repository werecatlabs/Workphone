#ifndef INodeAnimationTrack_h__
#define INodeAnimationTrack_h__

#include <Workphone/Interface/Animation/IAnimationTrack.hpp>

namespace workphone
{
    /** Used to animate an actor's properties over time. */
    class WPCore_API IActorAnimationTrack : public IAnimationTrack
    {
    public:
        /** Types of actor animation tracks. */
        enum class TrackType
        {
            Transform,   // Position, rotation, scale
            Visibility,  // Show/hide
            Custom       // Arbitrary property
        };

        /** Destructor */
        ~IActorAnimationTrack() override;

        /** Sets the actor this track animates. */
        virtual void setActor( SmartPtr<scene::IGameActor> actor ) = 0;

        /** Gets the actor this track animates. */
        virtual SmartPtr<scene::IGameActor> getActor() const = 0;

        /** Sets the property name this track animates (e.g., "position", "rotation", "customProperty").
         */
        virtual void setPropertyName( const String &propertyName ) = 0;

        /** Gets the property name this track animates. */
        virtual String getPropertyName() const = 0;

        /** Sets the type of this track. */
        virtual void setTrackType( TrackType type ) = 0;

        /** Gets the type of this track. */
        virtual TrackType getTrackType() const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // INodeAnimationTrack_h__
