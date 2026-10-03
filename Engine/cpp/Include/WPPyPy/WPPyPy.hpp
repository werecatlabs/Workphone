#ifndef WPPyPy_h__
#define WPPyPy_h__

#include "Workphone/WorkphoneAutolink.hpp"

#if FB_USE_AUTO_LINK
#ifdef _DEBUG
#    pragma comment( lib, "WPPyPy.lib" )
#elif NDEBUG
#    pragma comment( lib, "WPPyPy.lib" )
#else
#    pragma comment( lib, "WPPyPy.lib" )
#endif
#endif

#endif  // WPPyPy_h__
