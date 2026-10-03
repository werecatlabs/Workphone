#ifndef WPNetworkPrerequisites_h__
#define WPNetworkPrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPNetwork_EXPORTS
#            define WPNetwork_API __declspec( dllexport )
#        else
#            define WPNetwork_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPNetwork_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPNetwork_API
#endif

#endif  // WPNetworkPrerequisites_h__
