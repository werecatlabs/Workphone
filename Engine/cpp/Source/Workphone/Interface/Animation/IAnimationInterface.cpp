#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationInterface.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationInterface, ISharedObject );

    IAnimationInterface::~IAnimationInterface() = default;

}  // namespace workphone
