#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIMenubar.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIMenubar, IUIElement );

    IUIMenubar::IUIMenubar( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIMenubar::IUIMenubar() : IUIElement( IUIMenubar::typeInfo() )
    {
    }

    IUIMenubar::~IUIMenubar() = default;

}  // namespace workphone::ui
