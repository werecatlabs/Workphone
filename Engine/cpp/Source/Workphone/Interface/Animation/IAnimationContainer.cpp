#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Animation/IAnimationContainer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAnimationContainer, ISharedObject );

    IAnimationContainer::~IAnimationContainer() = default;

}  // namespace workphone
