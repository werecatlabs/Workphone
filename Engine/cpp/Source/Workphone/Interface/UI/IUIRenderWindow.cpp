#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIRenderWindow.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIRenderWindow, IUIWindow );

    IUIRenderWindow::IUIRenderWindow( u32 poolTypeId ) : IUIWindow( poolTypeId )
    {
    }

    IUIRenderWindow::IUIRenderWindow() : IUIWindow( IUIRenderWindow::typeInfo() )
    {
    }

    IUIRenderWindow::~IUIRenderWindow() = default;

}  // namespace workphone::ui
