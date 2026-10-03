#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationKeyFrame.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationKeyFrame, ISharedObject );

    IAnimationKeyFrame::~IAnimationKeyFrame() = default;

}  // namespace workphone
