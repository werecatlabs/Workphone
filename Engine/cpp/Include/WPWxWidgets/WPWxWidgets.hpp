#ifndef WPWxWidgets_H
#define WPWxWidgets_H

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>

#if FB_USE_AUTO_LINK
#    ifdef _DEBUG
#        pragma comment( lib, "Workphone.lib" )
#        pragma comment( lib, "WPWxWidgets.lib" )
#    elif NDEBUG
#        pragma comment( lib, "Workphone.lib" )
#        pragma comment( lib, "WPWxWidgets.lib" )
#    else
#        pragma comment( lib, "Workphone.lib" )
#        pragma comment( lib, "WPWxWidgets.lib" )
#    endif
#endif

#include "WPWxWidgets/WPWxWidgetsPrerequisites.hpp"
#include "WPWxWidgets/WPWxViewWindow.hpp"
#include "WPWxWidgets/WPWxApplication.hpp"
#include "WPWxWidgets/WPWxFourWaySplitter.hpp"
#include "WPWxWidgets/WPWxUtil.hpp"
#include "WPWxWidgets/WPWxInputManager.hpp"
#include <WPWxWidgets/wxLabelCheckboxPair.hpp>
#include <WPWxWidgets/wxLabelTextInputPair.hpp>

#endif
