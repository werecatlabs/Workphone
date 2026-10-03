#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationMorphKeyFrame.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationMorphKeyFrame, IAnimationKeyFrame );

    IAnimationMorphKeyFrame::~IAnimationMorphKeyFrame() = default;

}  // namespace workphone
