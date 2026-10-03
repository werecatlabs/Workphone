#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationNumericTrack.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationNumericTrack, ISharedObject );

    IAnimationNumericTrack::~IAnimationNumericTrack() = default;

}  // namespace workphone
