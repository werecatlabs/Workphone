#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIToolbar.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIToolbar, IUIElement );

    IUIToolbar::IUIToolbar( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIToolbar::IUIToolbar() : IUIElement( IUIToolbar::typeInfo() )
    {
    }

    IUIToolbar::~IUIToolbar() = default;

}  // namespace workphone::ui
