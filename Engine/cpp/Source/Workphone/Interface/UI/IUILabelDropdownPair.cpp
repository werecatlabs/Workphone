#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUILabelDropdownPair.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUILabelDropdownPair, IUIElement );

    IUILabelDropdownPair::IUILabelDropdownPair( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUILabelDropdownPair::IUILabelDropdownPair() : IUIElement( IUILabelDropdownPair::typeInfo() )
    {
    }

    IUILabelDropdownPair::~IUILabelDropdownPair() = default;

}  // namespace workphone::ui
