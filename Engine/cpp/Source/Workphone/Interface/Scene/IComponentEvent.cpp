#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IComponentEvent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IComponentEvent, IEvent );

    IComponentEvent::~IComponentEvent() = default;

}  // namespace workphone::scene
