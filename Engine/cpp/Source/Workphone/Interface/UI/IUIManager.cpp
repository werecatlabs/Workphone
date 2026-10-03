#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIManager, ISharedObject );

    IUIManager::IUIManager() : ISharedObject( IUIManager::typeInfo() )
    {
    }

    IUIManager::IUIManager( u32 poolTypeId ) : ISharedObject( poolTypeId )
    {
    }

    IUIManager::~IUIManager() = default;

}  // namespace workphone::ui
