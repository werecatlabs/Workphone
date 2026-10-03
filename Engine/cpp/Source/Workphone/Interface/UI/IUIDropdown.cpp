#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIDropdown, IUIElement );

    IUIDropdown::IUIDropdown( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIDropdown::IUIDropdown() : IUIElement( IUIDropdown::typeInfo() )
    {
    }

    IUIDropdown::~IUIDropdown() = default;

}  // namespace workphone::ui
