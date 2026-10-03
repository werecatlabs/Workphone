#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFSMManager, ISharedObject );

    IFSMManager::~IFSMManager() = default;
}  // namespace workphone
