#ifndef WPGraphicsOgreNextConfig_h__
#define WPGraphicsOgreNextConfig_h__

#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPGraphicsOgreNext_EXPORTS
#            define WPGraphicsOgreNext_API __declspec( dllexport )
#        else
#            define WPGraphicsOgreNext_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPGraphicsOgreNext_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPGraphicsOgreNext_API
#endif

#endif  // WPGraphicsOgreNextConfig_h__
