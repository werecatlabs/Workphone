#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUILayoutWindow.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUILayoutWindow, IUIElement );

    IUILayoutWindow::IUILayoutWindow( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUILayoutWindow::IUILayoutWindow() : IUIElement( IUILayoutWindow::typeInfo() )
    {
    }

    IUILayoutWindow::~IUILayoutWindow() = default;

}  // namespace workphone::ui
