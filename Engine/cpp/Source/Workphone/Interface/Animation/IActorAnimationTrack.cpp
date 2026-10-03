#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IActorAnimationTrack.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IActorAnimationTrack, IAnimationTrack );

    IActorAnimationTrack::~IActorAnimationTrack() = default;

}  // namespace workphone
