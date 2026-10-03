#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIInputManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIInputManager, IUIElement );

    IUIInputManager::IUIInputManager( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIInputManager::IUIInputManager() : IUIElement( IUIInputManager::typeInfo() )
    {
    }

    IUIInputManager::~IUIInputManager() = default;

}  // namespace workphone::ui
