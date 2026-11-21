#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIEventWindow.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIEventWindow, IUIWindow );

    IUIEventWindow::IUIEventWindow( u32 poolTypeId ) : IUIWindow( poolTypeId )
    {
    }

    IUIEventWindow::IUIEventWindow() : IUIWindow( IUIEventWindow::typeInfo() )
    {
    }

    IUIEventWindow::~IUIEventWindow() = default;

}  // namespace workphone::ui
