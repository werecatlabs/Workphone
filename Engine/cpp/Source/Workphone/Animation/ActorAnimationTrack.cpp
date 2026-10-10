#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/ActorAnimationTrack.hpp>
#include <Workphone/Interface/Animation/IAnimationTimeIndex.hpp>
#include <Workphone/Interface/Animation/IAnimationKeyFrame.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Animation/KeyFrameTransform3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone
{
    namespace
    {
        Quaternion<real_Num> blendRotation( Quaternion<real_Num> a, Quaternion<real_Num> b,
                                            real_Num weight )
        {
            a.normalise();
            b.normalise();
            auto cosine = a.dotProduct( b );
            if( cosine < 0 )
            {
                b = -b;
                cosine = -cosine;
            }
            cosine = std::clamp( cosine, real_Num( 0 ), real_Num( 1 ) );
            Quaternion<real_Num> result;
            if( cosine > real_Num( 0.9995 ) )
                result = a * ( real_Num( 1 ) - weight ) + b * weight;
            else
            {
                const auto angle = std::acos( cosine );
                result = a * ( std::sin( ( real_Num( 1 ) - weight ) * angle ) / std::sin( angle ) ) +
                         b * ( std::sin( weight * angle ) / std::sin( angle ) );
            }
            result.normalise();
            return result;
        }
    }

    WP_CLASS_REGISTER_DERIVED( workphone, ActorAnimationTrack, IActorAnimationTrack );

    ActorAnimationTrack::ActorAnimationTrack( IAnimation *parent ) : AnimationTrack( parent )
    {
        // The template base inserts an untyped AnimationKeyFrame. A transform
        // track must expose an editable transform key at time zero instead.
        removeAllKeyFrames();
        createKeyFrame( 0 );
    }

    ActorAnimationTrack::~ActorAnimationTrack() = default;

    void ActorAnimationTrack::setActor( SmartPtr<scene::IGameActor> actor )
    {
        m_actor = actor;
    }

    SmartPtr<scene::IGameActor> ActorAnimationTrack::getActor() const
    {
        return m_actor;
    }

    void ActorAnimationTrack::setBone( SmartPtr<IBone> bone )
    {
        m_bone = bone;
    }

    SmartPtr<IBone> ActorAnimationTrack::getBone() const
    {
        return m_bone;
    }

    void ActorAnimationTrack::setPropertyName( const String &propertyName )
    {
        m_propertyName = propertyName;
    }

    String ActorAnimationTrack::getPropertyName() const
    {
        return m_propertyName;
    }

    void ActorAnimationTrack::setTrackType( TrackType type )
    {
        m_trackType = type;
    }

    IActorAnimationTrack::TrackType ActorAnimationTrack::getTrackType() const
    {
        return m_trackType;
    }

    void ActorAnimationTrack::apply( const SmartPtr<IAnimationTimeIndex> &timeIndex, f32 weight,
                                     f32 scale )
    {
        if( ( !m_actor && !m_bone ) || !timeIndex || weight <= 0.0f )
        {
            return;
        }

        // Get the interpolated keyframe for the given time
        SmartPtr<IAnimationKeyFrame> kf;
        getInterpolatedKeyFrame( timeIndex, kf );

        if( !kf )
        {
            return;
        }

        // Apply the animation based on track type
        switch( m_trackType )
        {
        case TrackType::Transform:
        {
            auto transformKeyFrame = workphone::dynamic_pointer_cast<KeyFrameTransform3>( kf );
            if( !transformKeyFrame )
            {
                break;
            }

            const auto clampedWeight = std::clamp( weight, 0.0f, 1.0f );
            const auto targetPosition = transformKeyFrame->getPosition() * scale;

            if( m_bone )
            {
                const auto currentPosition = m_bone->getPosition();
                m_bone->setPosition( currentPosition +
                                     ( targetPosition - currentPosition ) * clampedWeight );
                    m_bone->setOrientation( blendRotation( m_bone->getOrientation(),
                                                          transformKeyFrame->getOrientation(), clampedWeight ) );
                break;
            }

            auto transform = m_actor->getTransform();
            if( transform )
            {
                const auto currentPosition = transform->getPosition();
                const auto currentScale = transform->getScale();
                const auto targetScale = transformKeyFrame->getScale();

                if( m_propertyName.empty() || m_propertyName == "position" )
                {
                    transform->setPosition( currentPosition +
                                            ( targetPosition - currentPosition ) * clampedWeight );
                }

                if( m_propertyName.empty() || m_propertyName == "orientation" ||
                    m_propertyName == "rotation" )
                {
                    transform->setOrientation( blendRotation( transform->getOrientation(),
                                                               transformKeyFrame->getOrientation(), clampedWeight ) );
                }

                if( m_propertyName.empty() || m_propertyName == "scale" )
                {
                    transform->setScale( currentScale + ( targetScale - currentScale ) * clampedWeight );
                }
            }
            break;
        }
        case TrackType::Visibility:
        {
            // Apply visibility animation
            // This would typically involve showing/hiding the actor
            // Implementation depends on how visibility is handled in your system
            break;
        }
        case TrackType::Custom:
        {
            // Apply custom property animation
            // This would use the m_propertyName to determine what to animate
            // Implementation depends on your property system
            break;
        }
        }

        // Call base class apply for any additional processing
        AnimationTrack::apply( timeIndex, weight, scale );
    }

    void ActorAnimationTrack::applyToBone( const SmartPtr<IBone> &bone,
                                           const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                           f32 weight, f32 scale ) const
    {
        if( !bone || !timeIndex || weight <= 0 || m_trackType != TrackType::Transform ) return;
        SmartPtr<IAnimationKeyFrame> sampled;
        getInterpolatedKeyFrame( timeIndex, sampled );
        const auto key = workphone::dynamic_pointer_cast<KeyFrameTransform3>( sampled );
        if( !key ) return;
        const auto blend = std::clamp( weight, 0.0f, 1.0f );
        auto target = bone; // SmartPtr intentionally propagates constness to its pointee.
        const auto current = target->getPosition();
        target->setPosition( current + ( key->getPosition() * scale - current ) * blend );
        target->setOrientation( blendRotation( target->getOrientation(), key->getOrientation(), blend ) );
    }

    SmartPtr<IAnimationKeyFrame> ActorAnimationTrack::createKeyFrame( f32 timePos )
    {
        timePos = std::max( 0.0f, timePos );

        auto keyFrames = getKeyFrames();
        for( auto &existing : keyFrames )
        {
            if( existing && std::abs( existing->getTime() - timePos ) <= std::numeric_limits<f32>::epsilon() )
            {
                if( workphone::dynamic_pointer_cast<KeyFrameTransform3>( existing ) )
                    return existing;
                // Repair an untyped legacy key without returning an object that
                // optimise() would remove as a duplicate of the old key.
                auto replacement = workphone::make_ptr<KeyFrameTransform3>();
                replacement->setTime( existing->getTime() );
                existing = replacement;
                setKeyFrames( keyFrames );
                return replacement;
            }
        }

        auto keyFrame = workphone::make_ptr<KeyFrameTransform3>();
        keyFrame->setTime( timePos );

        keyFrames.push_back( keyFrame );
        setKeyFrames( keyFrames );

        return keyFrame;
    }

    void ActorAnimationTrack::getInterpolatedKeyFrame( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                                       SmartPtr<IAnimationKeyFrame> &kf ) const
    {
        SmartPtr<IAnimationKeyFrame> kf1;
        SmartPtr<IAnimationKeyFrame> kf2;
        const auto t = getKeyFramesAtTime( timeIndex, kf1, kf2 );

        auto transform1 = workphone::dynamic_pointer_cast<KeyFrameTransform3>( kf1 );
        auto transform2 = workphone::dynamic_pointer_cast<KeyFrameTransform3>( kf2 );

        if( !transform1 || !transform2 )
        {
            AnimationTrack::getInterpolatedKeyFrame( timeIndex, kf );
            return;
        }

        if( t <= std::numeric_limits<f32>::epsilon() )
        {
            kf = transform1;
            return;
        }

        if( t >= 1.0f - std::numeric_limits<f32>::epsilon() )
        {
            kf = transform2;
            return;
        }

        auto interpolated = workphone::make_ptr<KeyFrameTransform3>();
        interpolated->setTime( transform1->getTime() * ( 1.0f - t ) + transform2->getTime() * t );
        interpolated->setPosition( transform1->getPosition() +
                                   ( transform2->getPosition() - transform1->getPosition() ) * t );
        interpolated->setScale( transform1->getScale() +
                                ( transform2->getScale() - transform1->getScale() ) * t );
        interpolated->setOrientation( blendRotation( transform1->getOrientation(),
                                                     transform2->getOrientation(), t ) );
        kf = interpolated;
    }

    f32 ActorAnimationTrack::getLength() const
    {
        return m_length;
    }

    void ActorAnimationTrack::setLength( f32 length )
    {
        m_length = std::max( 0.0f, length );
    }

}  // namespace workphone
