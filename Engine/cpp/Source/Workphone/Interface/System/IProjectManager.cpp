#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IProjectManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IProjectManager, ISharedObject );

    IProjectManager::~IProjectManager() = default;

}  // namespace workphone
