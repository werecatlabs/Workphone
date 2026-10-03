#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ISelectionManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ISelectionManager, ISharedObject );

    ISelectionManager::~ISelectionManager() = default;

}  // namespace workphone
