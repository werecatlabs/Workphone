#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiManager.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IAiManager, ISharedObject );

    IAiManager::~IAiManager() = default;

}  // namespace workphone
