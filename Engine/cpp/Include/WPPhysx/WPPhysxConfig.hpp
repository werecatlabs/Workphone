#ifndef WPPhysxConfig_h__
#define WPPhysxConfig_h__

#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        define WP_PHYSX_API __declspec( dllexport )
#    else
#        define WP_PHYSX_API
#    endif
#else
#    define WP_PHYSX_API
#endif

#endif // WPPhysxConfig_h__
