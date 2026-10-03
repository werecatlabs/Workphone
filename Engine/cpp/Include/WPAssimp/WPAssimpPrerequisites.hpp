#ifndef WPAssimpPrerequisites_h__
#define WPAssimpPrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPAssimp_EXPORTS
#            define WPAssimp_API __declspec( dllexport )
#        else
#            define WPAssimp_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPAssimp_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPAssimp_API
#endif

namespace workphone
{
    class IOStream;
    class LogStream;
}  // namespace workphone

#endif  // WPAssimpPrerequisites_h__
