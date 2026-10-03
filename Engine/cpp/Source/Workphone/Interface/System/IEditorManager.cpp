#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IEditorManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IEditorManager, ISharedObject );

    IEditorManager::~IEditorManager() = default;

}  // namespace workphone
