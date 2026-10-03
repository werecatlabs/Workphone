#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationVertexTrack.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationVertexTrack, IAnimationTrack );

    IAnimationVertexTrack::~IAnimationVertexTrack() = default;

}  // namespace workphone
