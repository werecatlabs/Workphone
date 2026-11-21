#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIProfileWindow.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIProfileWindow, IUIElement );

    IUIProfileWindow::IUIProfileWindow( u32 poolTypeId ) : IUIElement( poolTypeId )
    {
    }

    IUIProfileWindow::IUIProfileWindow() : IUIElement( IUIProfileWindow::typeInfo() )
    {
    }

    IUIProfileWindow::~IUIProfileWindow() = default;

}  // namespace workphone::ui
