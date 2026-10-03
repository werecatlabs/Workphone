#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IStateManager, ISharedObject );

    IStateManager::~IStateManager() = default;

}  // namespace workphone
