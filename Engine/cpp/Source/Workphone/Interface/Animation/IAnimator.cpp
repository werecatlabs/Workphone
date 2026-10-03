#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimator, ISharedObject );

    IAnimator::~IAnimator() = default;

}  // namespace workphone
