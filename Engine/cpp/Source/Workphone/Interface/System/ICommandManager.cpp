#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ICommandManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ICommandManager, ISharedObject );

    ICommandManager::~ICommandManager() = default;
}  // namespace workphone
