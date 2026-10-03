#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIBar.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIBar, IUIElement );

    IUIBar::IUIBar( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIBar::IUIBar() : IUIElement( IUIBar::typeInfo() )
    {
    }

    IUIBar::~IUIBar() = default;
}  // namespace workphone::ui
