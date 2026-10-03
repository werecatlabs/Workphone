#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/AnimationBind.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    int getInterpolationMode( IAnimation *animation )
    {
        return static_cast<int>( animation->getInterpolationMode() );
    }

    void setInterpolationMode( IAnimation *animation, int mode )
    {
        animation->setInterpolationMode( static_cast<InterpolationMode>( mode ) );
    }

    int getRotationInterpolationMode( IAnimation *animation )
    {
        return static_cast<int>( animation->getRotationInterpolationMode() );
    }

    void setRotationInterpolationMode( IAnimation *animation, int mode )
    {
        animation->setRotationInterpolationMode( static_cast<RotationInterpolationMode>( mode ) );
    }

    int getActorTrackType( IActorAnimationTrack *track )
    {
        return static_cast<int>( track->getTrackType() );
    }

    void setActorTrackType( IActorAnimationTrack *track, int type )
    {
        track->setTrackType( static_cast<IActorAnimationTrack::TrackType>( type ) );
    }

    int getVertexAnimationType( IAnimationVertexTrack *track )
    {
        return static_cast<int>( track->getAnimationType() );
    }

    void setVertexAnimationType( IAnimationVertexTrack *track, int type )
    {
        track->setAnimationType( static_cast<VertexAnimationType>( type ) );
    }

    void bindAnimation( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IAnimator, ISharedObject, SmartPtr<IAnimator>>( "Animator" )
                        .def( "setId", &IAnimator::setId )
                        .def( "getId", &IAnimator::getId )
                        .def( "start", &IAnimator::start )
                        .def( "stop", &IAnimator::stop )
                        .def( "setLoop", &IAnimator::setLoop )
                        .def( "isLoop", &IAnimator::isLoop )
                        .def( "setReverse", &IAnimator::setReverse )
                        .def( "isReverse", &IAnimator::isReverse )
                        .def( "isFinished", &IAnimator::isFinished )
                        .def( "setAnimationLength", &IAnimator::setAnimationLength )
                        .scope[def( "typeInfo", IAnimator::typeInfo )]];

        module( L )[class_<InterpolateVector2f, IAnimator, SmartPtr<InterpolateVector2f>>(
                        "InterpolateVector2f" )
                        .def( "getStartValue", &InterpolateVector2f::getStartValue )
                        .def( "setStartValue", &InterpolateVector2f::setStartValue )
                        .def( "getEndValue", &InterpolateVector2f::getEndValue )
                        .def( "setEndValue", &InterpolateVector2f::setEndValue )
                        .def( "getValue", &InterpolateVector2f::getValue )];

        using AnimationVector = Array<SmartPtr<IAnimation>>;

        module( L )[class_<AnimationVector>( "AnimationVector" )
                        .def( "size", &AnimationVector::size )
                        .def( "empty", &AnimationVector::empty )
                        .def( "at", static_cast<SmartPtr<IAnimation> &(AnimationVector::*)( size_t )>(
                                        &AnimationVector::at ) )];

        module(
            L )[class_<IAnimation, ISharedObject, SmartPtr<IAnimation>>( "IAnimation" )
                    .def( "getInterpolationMode", getInterpolationMode )
                    .def( "setInterpolationMode", setInterpolationMode )
                    .def( "getRotationInterpolationMode", getRotationInterpolationMode )
                    .def( "setRotationInterpolationMode", setRotationInterpolationMode )
                    .def( "getLength", &IAnimation::getLength )
                    .def( "setLength", &IAnimation::setLength )
                    .def( "removeTrack", &IAnimation::removeTrack )
                    .def( "addTrack", static_cast<SmartPtr<IAnimationTrack> ( IAnimation::* )(
                                          hash_type, const String &, u16 )>( &IAnimation::addTrack ) )
                    .def( "addBoneTrack",
                          static_cast<SmartPtr<IAnimationTrack> ( IAnimation::* )(
                              hash_type, u16, SmartPtr<IBone> )>( &IAnimation::addTrack ) )
                    .def( "getNumNodeTracks", &IAnimation::getNumNodeTracks )
                    .def( "getNodeTrack", &IAnimation::getNodeTrack )
                    .def( "hasNodeTrack", &IAnimation::hasNodeTrack )
                    .def( "getNumNumericTracks", &IAnimation::getNumNumericTracks )
                    .def( "getNumericTrack", &IAnimation::getNumericTrack )
                    .def( "hasNumericTrack", &IAnimation::hasNumericTrack )
                    .def( "getNumVertexTracks", &IAnimation::getNumVertexTracks )
                    .def( "getVertexTracks", &IAnimation::getVertexTracks )
                    .def( "setVertexTracks", &IAnimation::setVertexTracks )
                    .def( "getVertexTrack", &IAnimation::getVertexTrack )
                    .def( "hasVertexTrack", &IAnimation::hasVertexTrack )
                    .def( "destroyNodeTrack", &IAnimation::destroyNodeTrack )
                    .def( "destroyNumericTrack", &IAnimation::destroyNumericTrack )
                    .def( "destroyVertexTrack", &IAnimation::destroyVertexTrack )
                    .def( "destroyAllTracks", &IAnimation::destroyAllTracks )
                    .def( "destroyAllNodeTracks", &IAnimation::destroyAllNodeTracks )
                    .def( "destroyAllNumericTracks", &IAnimation::destroyAllNumericTracks )
                    .def( "destroyAllVertexTracks", &IAnimation::destroyAllVertexTracks )
                    .def( "apply",
                          static_cast<void ( IAnimation::* )( f32, f32, f32 )>( &IAnimation::apply ) )
                    .def( "optimise", &IAnimation::optimise )
                    .def( "clone", &IAnimation::clone )
                    .def( "getTimeIndex", &IAnimation::_getTimeIndex )
                    .def( "setUseBaseKeyFrame", &IAnimation::setUseBaseKeyFrame )
                    .def( "getUseBaseKeyFrame", &IAnimation::getUseBaseKeyFrame )
                    .def( "getBaseKeyFrameTime", &IAnimation::getBaseKeyFrameTime )
                    .def( "getBaseKeyFrameAnimationName", &IAnimation::getBaseKeyFrameAnimationName )
                    .def( "getContainer", &IAnimation::getContainer )
                    .enum_( "InterpolationMode" )
                        [value( "Linear", static_cast<int>( InterpolationMode::LINEAR ) ),
                         value( "Spline", static_cast<int>( InterpolationMode::SPLINE ) )]
                    .enum_( "RotationInterpolationMode" )
                        [value( "Linear", static_cast<int>( RotationInterpolationMode::LINEAR ) ),
                         value( "Spherical", static_cast<int>( RotationInterpolationMode::SPHERICAL ) )]
                    .scope[def( "typeInfo", IAnimation::typeInfo )]];

        module( L )[class_<IAnimationContainer, ISharedObject, SmartPtr<IAnimationContainer>>(
                        "IAnimationContainer" )
                        .def( "getNumAnimations", &IAnimationContainer::getNumAnimations )
                        .def( "getAnimationByIndex",
                              static_cast<IAnimation *(IAnimationContainer::*)( u16 ) const>(
                                  &IAnimationContainer::getAnimation ) )
                        .def( "getAnimation",
                              static_cast<IAnimation *(IAnimationContainer::*)( const String & ) const>(
                                  &IAnimationContainer::getAnimation ) )
                        .def( "createAnimation", &IAnimationContainer::createAnimation )
                        .def( "hasAnimation", &IAnimationContainer::hasAnimation )
                        .def( "removeAnimation", &IAnimationContainer::removeAnimation )
                        .scope[def( "typeInfo", IAnimationContainer::typeInfo )]];

        module( L )[class_<IAnimationInterface, ISharedObject, SmartPtr<IAnimationInterface>>(
                        "IAnimationInterface" )
                        .def( "createAnimation", &IAnimationInterface::createAnimation )
                        .def( "getAnimation",
                              static_cast<SmartPtr<IAnimation> ( IAnimationInterface::* )(
                                  const String & ) const>( &IAnimationInterface::getAnimation ) )
                        .def( "getAnimationByIndex",
                              static_cast<SmartPtr<IAnimation> ( IAnimationInterface::* )( u16 ) const>(
                                  &IAnimationInterface::getAnimation ) )
                        .def( "hasAnimation", &IAnimationInterface::hasAnimation )
                        .def( "removeAnimation", &IAnimationInterface::removeAnimation )
                        .def( "getNumAnimations", &IAnimationInterface::getNumAnimations )
                        .def( "clone", &IAnimationInterface::clone )
                        .scope[def( "typeInfo", IAnimationInterface::typeInfo )]];

        module( L )[class_<IAnimationKeyFrame, ISharedObject, SmartPtr<IAnimationKeyFrame>>(
                        "IAnimationKeyFrame" )
                        .def( "getTime", &IAnimationKeyFrame::getTime )
                        .def( "setTime", &IAnimationKeyFrame::setTime )
                        .scope[def( "typeInfo", IAnimationKeyFrame::typeInfo )]];

        module(
            L )[class_<IAnimationMorphKeyFrame, IAnimationKeyFrame, SmartPtr<IAnimationMorphKeyFrame>>(
                    "IAnimationMorphKeyFrame" )
                    .def( "setVertexBuffer", &IAnimationMorphKeyFrame::setVertexBuffer )
                    .def( "getVertexBuffer", &IAnimationMorphKeyFrame::getVertexBuffer )
                    .scope[def( "typeInfo", IAnimationMorphKeyFrame::typeInfo )]];

        module( L )[class_<IAnimationPoseKeyFrame, IAnimationKeyFrame, SmartPtr<IAnimationPoseKeyFrame>>(
                        "IAnimationPoseKeyFrame" )
                        .def( "getNumPoses", &IAnimationPoseKeyFrame::getNumPoses )
                        .def( "setNumPoses", &IAnimationPoseKeyFrame::setNumPoses )
                        .def( "setPoseReference", &IAnimationPoseKeyFrame::setPoseReference )
                        .scope[def( "typeInfo", IAnimationPoseKeyFrame::typeInfo )]];

        module( L )[class_<IAnimationNumericTrack, ISharedObject, SmartPtr<IAnimationNumericTrack>>(
                        "IAnimationNumericTrack" )
                        .def( "addKeyFrame", &IAnimationNumericTrack::addKeyFrame )
                        .def( "getValueAt", &IAnimationNumericTrack::getValueAt )
                        .def( "getNumKeyFrames", &IAnimationNumericTrack::getNumKeyFrames )
                        .def( "clearKeyFrames", &IAnimationNumericTrack::clearKeyFrames )
                        .scope[def( "typeInfo", IAnimationNumericTrack::typeInfo )]];

        module( L )[class_<IAnimationTimeIndex, ISharedObject, SmartPtr<IAnimationTimeIndex>>(
                        "IAnimationTimeIndex" )
                        .def( "hasKeyIndex", &IAnimationTimeIndex::hasKeyIndex )
                        .def( "getTimePos", &IAnimationTimeIndex::getTimePos )
                        .def( "getKeyIndex", &IAnimationTimeIndex::getKeyIndex )
                        .scope[def( "typeInfo", IAnimationTimeIndex::typeInfo )]];

        module(
            L )[class_<IAnimationTrack, ISharedObject, SmartPtr<IAnimationTrack>>( "IAnimationTrack" )
                    .def( "getAnimationHandle", &IAnimationTrack::getAnimationHandle )
                    .def( "setAnimationHandle", &IAnimationTrack::setAnimationHandle )
                    .def( "getNumKeyFrames", &IAnimationTrack::getNumKeyFrames )
                    .def( "getKeyFrame", &IAnimationTrack::getKeyFrame )
                    .def( "createKeyFrame", &IAnimationTrack::createKeyFrame )
                    .def( "removeKeyFrame", &IAnimationTrack::removeKeyFrame )
                    .def( "removeAllKeyFrames", &IAnimationTrack::removeAllKeyFrames )
                    .def( "apply", &IAnimationTrack::apply )
                    .def( "hasNonZeroKeyFrames", &IAnimationTrack::hasNonZeroKeyFrames )
                    .def( "optimise", &IAnimationTrack::optimise )
                    .def( "getParent", &IAnimationTrack::getParent )
                    .scope[def( "typeInfo", IAnimationTrack::typeInfo )]];

        module(
            L )[class_<IActorAnimationTrack, IAnimationTrack, SmartPtr<IActorAnimationTrack>>(
                    "IActorAnimationTrack" )
                    .def( "setActor", &IActorAnimationTrack::setActor )
                    .def( "getActor", &IActorAnimationTrack::getActor )
                    .def( "setPropertyName", &IActorAnimationTrack::setPropertyName )
                    .def( "getPropertyName", &IActorAnimationTrack::getPropertyName )
                    .def( "setTrackType", setActorTrackType )
                    .def( "getTrackType", getActorTrackType )
                    .enum_( "TrackType" )
                        [value( "Transform",
                                static_cast<int>( IActorAnimationTrack::TrackType::Transform ) ),
                         value( "Visibility",
                                static_cast<int>( IActorAnimationTrack::TrackType::Visibility ) ),
                         value( "Custom", static_cast<int>( IActorAnimationTrack::TrackType::Custom ) )]
                    .scope[def( "typeInfo", IActorAnimationTrack::typeInfo )]];

        module(
            L )[class_<IAnimationVertexTrack, IAnimationTrack, SmartPtr<IAnimationVertexTrack>>(
                    "IAnimationVertexTrack" )
                    .def( "getAnimationType", getVertexAnimationType )
                    .def( "setAnimationType", setVertexAnimationType )
                    .def( "createVertexMorphKeyFrame",
                          &IAnimationVertexTrack::createVertexMorphKeyFrame )
                    .def( "createVertexPoseKeyFrame", &IAnimationVertexTrack::createVertexPoseKeyFrame )
                    .def( "getVertexMorphKeyFrame", &IAnimationVertexTrack::getVertexMorphKeyFrame )
                    .def( "getVertexPoseKeyFrame", &IAnimationVertexTrack::getVertexPoseKeyFrame )
                    .def( "setAssociatedVertexData", &IAnimationVertexTrack::setAssociatedVertexData )
                    .def( "getAssociatedVertexData", &IAnimationVertexTrack::getAssociatedVertexData )
                    .enum_( "AnimationType" )
                        [value( "None", static_cast<int>( VertexAnimationType::VAT_NONE ) ),
                         value( "Morph", static_cast<int>( VertexAnimationType::VAT_MORPH ) ),
                         value( "Pose", static_cast<int>( VertexAnimationType::VAT_POSE ) )]
                    .scope[def( "typeInfo", IAnimationVertexTrack::typeInfo )]];

        module(
            L )[class_<IAnimationState, ISharedObject, SmartPtr<IAnimationState>>( "IAnimationState" )
                    .scope[def( "typeInfo", IAnimationState::typeInfo )]];

        module(
            L )[class_<render::IAnimationControllerListener, ISharedObject,
                       SmartPtr<render::IAnimationControllerListener>>( "IAnimationControllerListener" )
                    .def( "handleAnimationEnd",
                          &render::IAnimationControllerListener::handleAnimationEnd )
                    .scope[def( "typeInfo", render::IAnimationControllerListener::typeInfo )]];

        module(
            L )[class_<render::IAnimationTextureControl, ISharedObject,
                       SmartPtr<render::IAnimationTextureControl>>( "IAnimationTextureControl" )
                    .def( "setAnimationEnabled",
                          static_cast<bool ( render::IAnimationTextureControl::* )( bool )>(
                              &render::IAnimationTextureControl::setAnimationEnabled ) )
                    .def( "setAnimationEnabledAtTime",
                          static_cast<bool ( render::IAnimationTextureControl::* )( bool, f32 )>(
                              &render::IAnimationTextureControl::setAnimationEnabled ) )
                    .def( "isAnimationEnabled", &render::IAnimationTextureControl::isAnimationEnabled )
                    .def( "hasAnimationEnded", &render::IAnimationTextureControl::hasAnimationEnded )
                    .def( "setAnimationLoop", &render::IAnimationTextureControl::setAnimationLoop )
                    .def( "isAnimationLooping", &render::IAnimationTextureControl::isAnimationLooping )
                    .def( "setAnimationReversed",
                          &render::IAnimationTextureControl::setAnimationReversed )
                    .def( "isAnimationReversed", &render::IAnimationTextureControl::isAnimationReversed )
                    .def( "setTimePosition", &render::IAnimationTextureControl::setTimePosition )
                    .def( "getTimePosition", &render::IAnimationTextureControl::getTimePosition )
                    .scope[def( "typeInfo", render::IAnimationTextureControl::typeInfo )]];
    }
} // namespace workphone
