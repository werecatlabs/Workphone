#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationTrack.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationTrack, ISharedObject );

    IAnimationTrack::~IAnimationTrack() = default;

}  // namespace workphone
