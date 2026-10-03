#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IFactoryManager, ISharedObject );

    IFactoryManager::~IFactoryManager() = default;

}  // namespace workphone
