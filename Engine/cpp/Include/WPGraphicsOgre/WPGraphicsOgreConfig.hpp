#ifndef WPGraphicsOgreConfig_h__
#define WPGraphicsOgreConfig_h__

#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPGraphicsOgre_EXPORTS
#            define WPGraphicsOgre_API __declspec( dllexport )
#        else
#            define WPGraphicsOgre_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPGraphicsOgre_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPGraphicsOgre_API
#endif

#endif  // WPGraphicsOgreConfig_h__
