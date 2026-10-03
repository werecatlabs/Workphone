#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsWindow, IRenderTarget );

    const hash_type IGraphicsWindow::RESIZE_HASH = StringUtil::getHash( "resize" );
    const hash_type IGraphicsWindow::REPOSITION_HASH = StringUtil::getHash( "reposition" );
    const hash_type IGraphicsWindow::MOVED_OR_RESIZED_HASH = StringUtil::getHash( "MOVED_OR_RESIZED" );

    const u32 IGraphicsWindow::WINDOW_FLAG_FULLSCREEN = 0x00000001;
    const u32 IGraphicsWindow::WINDOW_FLAG_VISIBLE = 0x00000002;
    const u32 IGraphicsWindow::WINDOW_FLAG_BORDERLESS = 0x00000004;
    const u32 IGraphicsWindow::WINDOW_FLAG_RESIZABLE = 0x00000008;
    const u32 IGraphicsWindow::WINDOW_FLAG_HIDDEN = 0x00000010;
    const u32 IGraphicsWindow::WINDOW_FLAG_MAXIMIZED = 0x00000020;
    const u32 IGraphicsWindow::WINDOW_FLAG_MINIMIZED = 0x00000040;
    const u32 IGraphicsWindow::WINDOW_FLAG_NO_TASKBAR = 0x00000080;
    const u32 IGraphicsWindow::WINDOW_FLAG_ALWAYS_ON_TOP = 0x00000100;
    const u32 IGraphicsWindow::WINDOW_FLAG_DEACTIVATE_ON_FOCUS_CHANGE = 0x00000200;
    const u32 IGraphicsWindow::WINDOW_FLAG_IS_PRIMARY = 0x00000400;
    const u32 IGraphicsWindow::WINDOW_FLAG_IS_CLOSED = 0x00000800;

    IGraphicsWindow::~IGraphicsWindow() = default;

}  // namespace workphone::render
