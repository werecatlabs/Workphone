#ifndef FB_EVENTLOOP_H
#define FB_EVENTLOOP_H

#include "wx/evtloop.h"

namespace fb
{
    namespace ui
    {

        class EventLoop : public wxGUIEventLoop
        {
        public:
            EventLoop();
            ~EventLoop();

            bool Dispatch();
        };

    }  // end namespace ui
}  // end namespace fb

#endif  //FB_EVENTLOOP_H
