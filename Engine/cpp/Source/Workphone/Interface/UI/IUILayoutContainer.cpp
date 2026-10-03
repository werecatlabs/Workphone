#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUILayoutContainer.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUILayoutContainer, IUIElement );

    IUILayoutContainer::IUILayoutContainer( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUILayoutContainer::IUILayoutContainer() : IUIElement( IUILayoutContainer::typeInfo() )
    {
    }

    IUILayoutContainer::~IUILayoutContainer() = default;

}  // namespace workphone::ui
