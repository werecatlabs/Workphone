#ifndef __WPWxUtil_H
#define __WPWxUtil_H

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <Workphone/Core/Properties.hpp>
#include <wx/window.h>

namespace workphone
{
    namespace ui
    {
        //--------------------------------------------
        /**
         */
        class WxUtil
        {
        public:
            /** */
            static SmartPtr<render::IWindow> createRenderWindow(
                wxWindow *window, const String &name, int width, int height,
                const SmartPtr<Properties> &properties );
        };

    }  // namespace ui
}  // namespace workphone

#endif
