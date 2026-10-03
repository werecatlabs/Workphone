#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationPoseKeyFrame.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationPoseKeyFrame, IAnimationKeyFrame );

    IAnimationPoseKeyFrame::~IAnimationPoseKeyFrame() = default;

}  // namespace workphone
