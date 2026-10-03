#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include <WPWxWidgets/EventLoop.hpp>

namespace workphone
{
    namespace ui
    {

        EventLoop::EventLoop() : wxGUIEventLoop()
        {
        }

        EventLoop::~EventLoop()
        {
        }

        bool EventLoop::Dispatch()
        {
            return DispatchTimeout( 10 ) != 0;
        }

    }  // end namespace ui
}  // namespace workphone
