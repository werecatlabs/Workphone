#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Database/IDatabaseManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDatabaseManager, ISharedObject );

    IDatabaseManager::~IDatabaseManager() = default;
}  // namespace workphone
