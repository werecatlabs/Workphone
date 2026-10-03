#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIHorizontalLayout.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIHorizontalLayout, IUILayoutContainer );

    IUIHorizontalLayout::IUIHorizontalLayout( u32 poolTypeId ) : IUILayoutContainer( poolTypeId )
    {
    }

    IUIHorizontalLayout::IUIHorizontalLayout() : IUILayoutContainer( IUIHorizontalLayout::typeInfo() )
    {
    }

    IUIHorizontalLayout::~IUIHorizontalLayout() = default;

}  // namespace workphone::ui
