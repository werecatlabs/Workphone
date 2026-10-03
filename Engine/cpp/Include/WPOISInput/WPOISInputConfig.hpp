#ifndef WPOISInputConfig_h__
#define WPOISInputConfig_h__

#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPOISInput_EXPORTS
#            define WPOISInput_API __declspec( dllexport )
#        else
#            define WPOISInput_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPOISInput_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPOISInput_API
#endif

#endif  // WPOISInputConfig_h__
