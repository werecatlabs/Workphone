#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IProcessManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IProcessManager, ISharedObject );

    IProcessManager::~IProcessManager() = default;

}  // namespace workphone
