#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ILogManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ILogManager, ISharedObject );

    ILogManager::~ILogManager() = default;
}  // namespace workphone
