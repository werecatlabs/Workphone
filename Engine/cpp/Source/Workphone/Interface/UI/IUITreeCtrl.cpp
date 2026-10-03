#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUITreeCtrl.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUITreeCtrl, IUIElement );

    IUITreeCtrl::IUITreeCtrl( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUITreeCtrl::IUITreeCtrl() : IUIElement( IUITreeCtrl::typeInfo() )
    {
    }

    const hash_type IUITreeCtrl::clearHash = StringUtil::getHash( "clear" );

    IUITreeCtrl::~IUITreeCtrl() = default;

}  // namespace workphone::ui
