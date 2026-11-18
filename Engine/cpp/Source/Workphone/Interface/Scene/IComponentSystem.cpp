#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Scene/IComponentSystem.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, IComponentSystem, IEventListener );

    IComponentSystem::~IComponentSystem() = default;

    TaskId IComponentSystem::getUpdateTask() const
    {
        return TaskId::None;
    }

}  // namespace workphone::scene
