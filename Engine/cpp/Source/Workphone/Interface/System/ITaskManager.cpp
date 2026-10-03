#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ITaskManager, ISharedObject );

    ITaskManager::~ITaskManager() = default;

}  // namespace workphone
