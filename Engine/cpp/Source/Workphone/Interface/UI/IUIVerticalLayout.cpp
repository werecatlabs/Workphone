#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIVerticalLayout.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIVerticalLayout, IUILayoutContainer );

    IUIVerticalLayout::IUIVerticalLayout( u32 poolTypeId ) : IUILayoutContainer( poolTypeId )
    {
    }

    IUIVerticalLayout::IUIVerticalLayout() : IUILayoutContainer( IUIVerticalLayout::typeInfo() )
    {
    }

    IUIVerticalLayout::~IUIVerticalLayout() = default;

}  // namespace workphone::ui
