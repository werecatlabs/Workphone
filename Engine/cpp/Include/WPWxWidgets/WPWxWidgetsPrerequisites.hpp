#ifndef WPWxWidgetsPrerequisites_h__
#define WPWxWidgetsPrerequisites_h__

#include <Workphone/WorkphoneConfig.hpp>

#if defined FB_PLATFORM_WIN32
#    define _WINSOCKAPI_  // stops windows.h including winsock.h
#    include <windows.h>
#endif

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

class GLContext;

class wxWindow;
class wxAuiNotebook;
class wxStaticText;
class wxTextCtrl;

class wxPropertyGrid;
class wxPGProperty;

#if FB_BUILD_WXWIDGETS
// forward decs
class wxAuiManager;
class wxAuiNotebook;
class wxButton;
class wxBoxSizer;
class wxBitmapComboBox;
class wxComboBox;
class wxCheckBox;
class wxFrameManager;
class wxListCtrl;
class wxMenu;
class wxNotebook;
class wxPropertyGrid;
class wxSlider;
class wxTextCtrl;
class wxToggleButton;
class wxTreeCtrl;
class wxToggleButton;
class wxTimeline;
class wxTimer;
class wxToggleButton;
class wxWindow;
#endif

namespace workphone
{
    namespace ui
    {

        // forward declarations
        class wxViewWindow;
        class wxFourWaySplitter;
        class wxApplicationWindow;
        class wxLabelCheckboxPair;
        class wxLabelTextInputPair;

    }  // end namespace ui
}  // namespace workphone

#endif  // WPWxWidgetsPrerequisites_h__
