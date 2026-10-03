#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimation, ISharedObject );

    IAnimation::~IAnimation() = default;

}  // namespace workphone
