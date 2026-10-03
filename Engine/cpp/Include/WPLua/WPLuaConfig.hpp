#ifndef WPLuaConfig_h__
#define WPLuaConfig_h__

#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPLua_EXPORTS
#            define WPLua_API __declspec( dllexport )
#        else
#            define WPLua_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPLua_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPLua_API
#endif

#endif  // WPLuaConfig_h__
